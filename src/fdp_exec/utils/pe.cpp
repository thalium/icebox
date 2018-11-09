#include "pe.hpp"

#define FDP_MODULE "pe"
#include "log.hpp"
#include "endian.hpp"
#include "core/helpers.hpp"
#include "utils/utils.hpp"

#include <array>

namespace
{
    enum member_pe_offset_e
    {
        IMAGE_NT_HEADERS64_FileHeader,
        IMAGE_NT_HEADERS64_OptionalHeader,
        IMAGE_FILE_HEADER_SizeOfOptionalHeader,
        IMAGE_FILE_HEADER_NumberOfSections,
        IMAGE_OPTIONAL_HEADER_DataDirectory,
        IMAGE_OPTIONAL_HEADER_NumberOfRvaAndSizes,
        IMAGE_DATA_DIRECTORY_VirtualAddress,
        IMAGE_DATA_DIRECTORY_Size,
        IMAGE_DEBUG_DIRECTORY_Type,
        IMAGE_DEBUG_DIRECTORY_SizeOfData,
        IMAGE_DEBUG_DIRECTORY_AddressOfRawData,
        IMAGE_SECTION_HEADER_Name,
        IMAGE_SECTION_HEADER_Misc,
        IMAGE_SECTION_HEADER_VirtualAddress,
        MEMBER_PE_OFFSET_COUNT,
    };
    struct MemberPeOffset
    {
        member_pe_offset_e e_id;
        const char      module[16];
        const char      struc[32];
        const char      member[32];
    };
    const MemberPeOffset g_member_pe_offsets[] =
    {
        {IMAGE_NT_HEADERS64_FileHeader,                         "nt", "_IMAGE_NT_HEADERS64",                          "FileHeader"},
        {IMAGE_NT_HEADERS64_OptionalHeader,                     "nt", "_IMAGE_NT_HEADERS64",                          "OptionalHeader"},
        {IMAGE_FILE_HEADER_SizeOfOptionalHeader,                "nt", "_IMAGE_FILE_HEADER",                           "SizeOfOptionalHeader"},
        {IMAGE_FILE_HEADER_NumberOfSections,                    "nt", "_IMAGE_FILE_HEADER",                           "NumberOfSections"},
        {IMAGE_OPTIONAL_HEADER_DataDirectory,                   "nt", "_IMAGE_OPTIONAL_HEADER64",                     "DataDirectory"},
        {IMAGE_OPTIONAL_HEADER_NumberOfRvaAndSizes,             "nt", "_IMAGE_OPTIONAL_HEADER64",                     "NumberOfRvaAndSizes"},
        {IMAGE_DATA_DIRECTORY_VirtualAddress,                   "nt", "_IMAGE_DATA_DIRECTORY",                        "VirtualAddress"},
        {IMAGE_DATA_DIRECTORY_Size,                             "nt", "_IMAGE_DATA_DIRECTORY",                        "Size"},
        {IMAGE_DEBUG_DIRECTORY_Type,                            "nt", "_IMAGE_DEBUG_DIRECTORY",                       "Type"},
        {IMAGE_DEBUG_DIRECTORY_SizeOfData,                      "nt", "_IMAGE_DEBUG_DIRECTORY",                       "SizeOfData"},
        {IMAGE_DEBUG_DIRECTORY_AddressOfRawData,                "nt", "_IMAGE_DEBUG_DIRECTORY",                       "AddressOfRawData"},
        {IMAGE_SECTION_HEADER_Name,                             "nt", "_IMAGE_SECTION_HEADER",                        "Name"},
        {IMAGE_SECTION_HEADER_Misc,                             "nt", "_IMAGE_SECTION_HEADER",                        "Misc"},
        {IMAGE_SECTION_HEADER_VirtualAddress,                   "nt", "_IMAGE_SECTION_HEADER",                        "VirtualAddress"},
    };
    static_assert(COUNT_OF(g_member_pe_offsets) == MEMBER_PE_OFFSET_COUNT, "invalid members");

    using MemberPeOffsets = std::array<uint64_t, MEMBER_PE_OFFSET_COUNT>;

    enum unwind_op_codes_e
    {
        UWOP_PUSH_NONVOL,                         // info : register number
        UWOP_ALLOC_LARGE,                         // no info, alloc size in next 2 slots
        UWOP_ALLOC_SMALL,                         // info : size of allocation / 8 - 1
        UWOP_SET_FPREG,                           // no info, FP = RSP + UNWIND_INFO.FPRegOffset*16
        UWOP_SAVE_NONVOL,                         // info : register number, offset in next slot
        UWOP_SAVE_NONVOL_FAR,                     // info : register number, offset in next 2 slots
        UWOP_SAVE_XMM128,                         // info : XMM reg number, offset in next slot
        UWOP_SAVE_XMM128_FAR,                     // info : XMM reg number, offset in next 2 slots
        UWOP_PUSH_MACHFRAME,                      // info : 0: no error-code, 1: error-code
    };

    enum register_numbers_e
    {
        UWINFO_RAX,
        UWINFO_RCX,
        UWINFO_RDX,
        UWINFO_RBX,
        UWINFO_RSP,
        UWINFO_RBP,
        UWINFO_RSI,
        UWINFO_RDI,
    };

    const int UNWIND_VERSION_MASK       = 0b0111;
    const int UNWIND_CHAINED_FLAG_MASK  = 0b00100000;

    typedef uint8_t UnwindInfo[4];

    struct RuntimeFunction
    {
        uint32_t    start_address;
        uint32_t    end_address;
        uint32_t    unwind_info;
    };

}

struct pe::Pe::Data
{
    MemberPeOffsets members_pe_;
};

pe::Pe::Pe()
    : d_(std::make_unique<Data>())
{
}

pe::Pe::~Pe()
{
}

bool pe::Pe::setup(core::Core& core)
{
    bool fail = false;
    for(size_t i = 0; i < MEMBER_PE_OFFSET_COUNT; ++i)
    {
        const auto offset = core.sym.struc_offset(g_member_pe_offsets[i].module, g_member_pe_offsets[i].struc, g_member_pe_offsets[i].member);
        if(!offset)
        {
            fail = true;
            LOG(ERROR, "unable to read %s!%s.%s member offset", g_member_pe_offsets[i].module, g_member_pe_offsets[i].struc, g_member_pe_offsets[i].member);
            continue;
        }
        d_->members_pe_[i] = *offset;
    }

    if(fail)
        return false;

    return true;
}

opt<span_t> pe::Pe::get_directory_entry(core::Core& core, const span_t span, const pe_directory_entries_e directory_entry_id)
{
    static const auto e_lfanew_offset = 0x3C;
    const auto e_lfanew = core::read_le32(core, span.addr+e_lfanew_offset);
    if(!e_lfanew)
        FAIL({}, "unable to read e_lfanew");

    const auto image_nt_header = span.addr+*e_lfanew;   //IMAGE_NT_HEADER
    const auto image_optional_header = image_nt_header + d_->members_pe_[IMAGE_NT_HEADERS64_OptionalHeader];

    const auto size_image_data_directory = 0x08;
    const auto data_directory = image_optional_header + d_->members_pe_[IMAGE_OPTIONAL_HEADER_DataDirectory]
                                + size_image_data_directory*directory_entry_id;
    const auto data_directory_virtual_address = core::read_le32(core, data_directory + d_->members_pe_[IMAGE_DATA_DIRECTORY_VirtualAddress]);
    if(!data_directory_virtual_address)
        FAIL({}, "unable to read DataDirectory.VirtualAddress");

    const auto data_directory_size = core::read_le32(core, data_directory + d_->members_pe_[IMAGE_DATA_DIRECTORY_Size]);
    if(!data_directory_size)
        FAIL({}, "unable to read DataDirectory.Size");

    // LOG(INFO, "exception_dir addr %" PRIx64 " section size %" PRIx32, span.addr + *data_directory_virtual_address, *data_directory_size);

    return span_t{span.addr + *data_directory_virtual_address, *data_directory_size};
}

opt<span_t> pe::Pe::parse_debug_dir(void* vsrc, const uint64_t mod_base_addr, const span_t debug_dir)
{
    const auto src = reinterpret_cast<const uint8_t*>(vsrc);

    const auto sizeof_IMAGE_DEBUG_DIRECTORY = 0x1C;
    if (debug_dir.size<sizeof_IMAGE_DEBUG_DIRECTORY)
        FAIL({}, "Debug directory to small");

    const auto type = read_le32(&src[d_->members_pe_[IMAGE_DEBUG_DIRECTORY_Type]]);
    if (type != 2)
        FAIL({}, "Unknown IMAGE_DEBUG_TYPE, should be IMAGE_DEBUG_TYPE_CODEVIEW (=2), it's the one for pdb");

    const auto size_rawdata = read_le32(&src[d_->members_pe_[IMAGE_DEBUG_DIRECTORY_SizeOfData]]);
    const auto addr_rawdata = read_le32(&src[d_->members_pe_[IMAGE_DEBUG_DIRECTORY_AddressOfRawData]]);

    return span_t{mod_base_addr + addr_rawdata, size_rawdata};
}

opt<std::map<uint32_t, pe::FunctionEntry>> pe::Pe::parse_exception_dir(core::Core& core, void* vsrc, const uint64_t mod_base_addr, const span_t exception_dir)
{
    const auto src = reinterpret_cast<const uint8_t*>(vsrc);

    std::map<uint32_t, FunctionEntry> function_table;
    std::map<uint32_t, FunctionEntry> orphan_function_entries;

    for (size_t i=0; i<exception_dir.size; i=i+sizeof(RuntimeFunction)){

        const auto start_address = read_le32(&src[i+offsetof(RuntimeFunction, start_address)]);
        const auto end_address = read_le32(&src[i+offsetof(RuntimeFunction, end_address)]);
        const auto unwind_info_ptr = read_le32(&src[i+offsetof(RuntimeFunction, unwind_info)]);

        UnwindInfo unwind_info;
        const auto to_read = mod_base_addr + unwind_info_ptr;

        auto ok = core.mem.virtual_read(unwind_info, to_read, sizeof unwind_info);
        if(!ok)
            FAIL({}, "unable to read unwind info");

        const bool chained_flag = unwind_info[0] & UNWIND_CHAINED_FLAG_MASK;
        const auto prolog_size = unwind_info[1];
        const auto nbr_of_unwind_code = unwind_info[2];

        //Deal with frame register
        const auto frame_register = unwind_info[3] & 0x0F;           // register used as frame pointer
        const uint8_t frame_reg_offset = 8*(unwind_info[3] >> 4);    // offset of frame register
        if (frame_reg_offset != 0 && frame_register != UWINFO_RBP)
            LOG(ERROR, "WARNING : the used framed register is not rbp (code %d), this case is never used and not implemented", frame_register);

        const auto SIZE_UC = 2;

        const auto r = chained_flag ? sizeof(RuntimeFunction) : 0;
        const auto unwind_codes_size = nbr_of_unwind_code*SIZE_UC + r;
        if(unwind_codes_size == 0){
            std::vector<UnwindCode> unwind_codes;
            FunctionEntry function_entry = {start_address, end_address, prolog_size, 0, 0, 0, 0, unwind_codes};
            function_table.emplace(start_address, function_entry);
            continue;
        }

        std::vector<uint8_t> buffer;
        buffer.resize(unwind_codes_size);
        core.mem.virtual_read(&buffer[0], mod_base_addr + unwind_info_ptr + sizeof(UnwindInfo), unwind_codes_size);
        if(!ok)
            FAIL({}, "unable to read unwind codes");

        uint32_t register_size = 0x08;            //TODO Defined this somewhere else

        std::vector<UnwindCode>     unwind_codes;
        uint32_t stack_frame_size = 0;
        int prev_frame_reg = 0;
        size_t idx = 0;
        while(idx < unwind_codes_size - r)
        {
            UnwindCode unwind_code = {buffer[idx], buffer[idx+1], 0};

            const auto unwind_operation = unwind_code.unwind_op_and_info & 0x0F;
            const auto unwind_code_info = unwind_code.unwind_op_and_info >> 4;

            switch(unwind_operation){
                case UWOP_PUSH_NONVOL :
                    if (unwind_code_info == UWINFO_RBP)
                        prev_frame_reg = stack_frame_size;

                    stack_frame_size += register_size;
                    break;
                case UWOP_ALLOC_LARGE :
                    if (unwind_code_info == 0){
                        const auto extra_info = read_le16(&buffer[idx+2]);
                        stack_frame_size += extra_info * 8;
                        idx += 2;
                    } else if (unwind_code_info == 1){
                        const auto extra_info = read_le32(&buffer[idx+2]);
                        stack_frame_size += extra_info;
                        idx += 4;
                    }
                    break;
                case UWOP_ALLOC_SMALL :
                    stack_frame_size += unwind_code_info*8 + 8;
                    break;
                case UWOP_SET_FPREG :
                    break;
                case UWOP_SAVE_NONVOL :
                    if (unwind_code_info == UWINFO_RBP)
                        prev_frame_reg = 8*read_le16(&buffer[idx+2]);

                    idx += 2;
                    break;
                case UWOP_SAVE_NONVOL_FAR :
                    if (unwind_code_info == UWINFO_RBP)
                        prev_frame_reg = read_le32(&buffer[idx+2]);

                    idx += 4;
                    break;
                case UWOP_SAVE_XMM128 :
                    idx += 2;
                    break;
                case UWOP_SAVE_XMM128_FAR :
                    idx += 4;
                    break;
                case UWOP_PUSH_MACHFRAME :
                    break;
            }

            unwind_codes.push_back({buffer[idx], buffer[idx+1],stack_frame_size});

            idx += 2;
        }

        // Deal with the runtime func at the end
        uint32_t mother_start_addr = 0;
        if (chained_flag != 0){
            mother_start_addr = read_le32(&buffer[idx+offsetof(RuntimeFunction, start_address)]);

            const auto mother_function_entry = lookup_function_entry(mother_start_addr, function_table);
            if (!mother_function_entry){
                orphan_function_entries.emplace(start_address, FunctionEntry{start_address, end_address, prolog_size,
                                                                stack_frame_size, prev_frame_reg, frame_reg_offset, mother_start_addr, unwind_codes});
                continue;
            }
            stack_frame_size += mother_function_entry->stack_frame_size;
        }

        FunctionEntry function_entry = {start_address, end_address, prolog_size, stack_frame_size, prev_frame_reg,
                                        frame_reg_offset, mother_start_addr,  unwind_codes};

        if(false){
            LOG(INFO, "Function entry : start %" PRIx32 " end %" PRIx32 " prolog size %" PRIx8 " number of codes %" PRIx8  " unwind info pointer %" PRIx32
                    " stack frame size %x", start_address, end_address, prolog_size, nbr_of_unwind_code, unwind_info_ptr, stack_frame_size);
        }

        function_table.emplace(start_address, function_entry);
    }

    if (orphan_function_entries.size() == 0)
        return function_table;

    for (auto orphan_fe : orphan_function_entries){
        const auto mother_start_addr = orphan_fe.second.mother_start_addr;
        auto function_entry = orphan_fe.second;

        const auto mother_function_entry = lookup_function_entry(mother_start_addr, function_table);
        if (!mother_function_entry)
            continue;   //Should never happend

        function_entry.stack_frame_size += mother_function_entry->stack_frame_size;
        function_table.emplace(function_entry.start_address, function_entry);
    }

    return function_table;
}

namespace{
    template<typename T>
    const pe::FunctionEntry* check_previous_exist(const T& it, const T& end, const uint64_t addr)
    {
        if(it == end)
            return nullptr;

        if(addr > it->second.end_address)
            return nullptr;

        return &(it->second);
    }
}

const pe::FunctionEntry* pe::Pe::lookup_function_entry(const uint64_t addr, std::map<uint32_t, pe::FunctionEntry> function_table)
{
    // lower bound returns first item greater or equal
    auto it = function_table.lower_bound(addr);
    const auto end = function_table.end();
    if(it == end)
        return check_previous_exist(function_table.rbegin(), function_table.rend(), addr);

    // equal
    if(it->first == addr)
        return check_previous_exist(it, end, addr);

    // stricly greater, go to previous item
    return check_previous_exist(--it, end, addr);
}

return_t<size_t> pe::read_image_size(const void* vsrc, size_t size)
{
    const auto src = reinterpret_cast<const uint8_t*>(vsrc);

    const auto e_magic = read_be16(&src[0]);
    static const auto image_dos_signature = 0x4D5A;     //MZ
    if(e_magic != image_dos_signature)
        return err::make(err_e::invalid_input);

    static const auto e_lfanew_offset = 0x3C;
    size_t idx = e_lfanew_offset;
    if(idx + 4 > size)
        return err::make(err_e::input_too_small);

    const auto e_lfanew = read_le32(&src[idx]);
    idx = e_lfanew;

    static const uint32_t image_nt_signature = 0X5045 << 16;    //PE
    if(idx + sizeof image_nt_signature > size)
        return err::make(err_e::input_too_small);

    const auto signature = read_be32(&src[idx]);
    if(signature != image_nt_signature)
        return err::make(err_e::invalid_input);

    static const uint16_t image_file_machine_amd64 = 0x8664;
    idx += sizeof signature;
    if(idx + sizeof image_file_machine_amd64 > size)
        return err::make(err_e::input_too_small);

    const auto machine = read_le16(&src[idx]);
    if(machine != image_file_machine_amd64)
        return err::make(err_e::invalid_input);

    static const int image_file_header_size = 20;
    static const uint16_t image_nt_optional_hdr64_magic = 0x20B;
    idx += image_file_header_size;
    if(idx + sizeof image_nt_optional_hdr64_magic > size)
        return err::make(err_e::input_too_small);

    const auto magic = read_le16(&src[idx]);
    if(magic != image_nt_optional_hdr64_magic)
        return err::make(err_e::invalid_input);

    static const auto size_of_image_offset = 14 * 4;
    idx += size_of_image_offset;
    if(idx + 4 > size)
        return err::make(err_e::input_too_small);

    return read_le32(&src[idx]);
}
