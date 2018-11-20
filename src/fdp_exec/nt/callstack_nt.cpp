#include "callstack.hpp"

#define FDP_MODULE "callstack_nt"
#include "core/helpers.hpp"
#include "utils/sanitizer.hpp"
#include "log.hpp"
#include "os.hpp"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    struct unwind_code_t
    {
        uint32_t   stack_size_used;
        uint8_t    code_offset;
        uint8_t    unwind_op_and_info;
    };

    struct function_entry_t
    {
        uint32_t    start_address;
        uint32_t    end_address;
        uint32_t    stack_frame_size;
        uint32_t    prev_frame_reg;
        uint32_t    mother_start_addr;
        uint8_t     prolog_size;
        uint8_t     frame_reg_offset;
        size_t      unwind_codes_idx;
        int         unwind_codes_nb;
    };

    struct FunctionTable
    {
        std::vector<function_entry_t>  function_entries;
        std::vector<unwind_code_t>     unwinds;
    };

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

    struct CallstackNt
        : public callstack::ICallstack
    {
        CallstackNt(core::Core& core, pe::Pe& pe);

        // methods
        bool setup();
        opt<FunctionTable>     get_mod_functiontable(const std::string& name, const span_t module);
        opt<FunctionTable>     insert(const std::string& name, const span_t module);
        opt<mod_t>             find_mod(proc_t proc, uint64_t addr);

        opt<FunctionTable>      parse_exception_dir  (core::Core& core, const void* src, uint64_t mod_base_addr, span_t exception_dir);
        const function_entry_t* lookup_function_entry(uint32_t offset_in_mod, const std::vector<function_entry_t>& function_entries);

        // os::IModule
        bool    get_callstack        (proc_t proc, uint64_t rip, uint64_t rsp, uint64_t rbp,
                                      const on_callstep_fn& on_callstep) override;

        // members
        core::Core&     core_;
        pe::Pe&         pe_;

        // private data
        struct Data;
        std::unique_ptr<Data> d_;
    };
}

namespace
{
    using ModsByProc    = std::unordered_map<uint64_t, std::map<uint64_t, mod_t>>;
    using ExceptionDirs = std::unordered_map<std::string, FunctionTable>;
}

struct CallstackNt::Data
{
    ModsByProc    modules_by_proc_;
    ExceptionDirs exception_dirs_;
};

CallstackNt::CallstackNt(core::Core& core, pe::Pe& pe)
    : core_(core), pe_(pe), d_(std::make_unique<Data>())
{
}

std::shared_ptr<callstack::ICallstack> callstack::make_callstack_nt(core::Core& core, pe::Pe& pe)
{
    auto cs_nt = std::make_shared<CallstackNt>(core, pe);
    if(!cs_nt)
        return nullptr;

    const auto ok = cs_nt->setup();
    if(!ok)
        return nullptr;

    return cs_nt;
}

bool CallstackNt::setup()
{
    return true;
}

namespace {
    struct CurrentContext
    {
        uint64_t rip;
        uint64_t rsp;
        uint64_t rbp;
    };

    bool compare_function_entries(function_entry_t a, function_entry_t b)
    {
        return a.start_address<b.start_address;
    }

    opt<uint64_t> get_stack_frame_size(const uint64_t off_in_mod, const FunctionTable& function_table, const function_entry_t& function_entry)
    {
        const auto off_in_prolog = off_in_mod - function_entry.start_address;
        if (off_in_prolog == 0)
            return 0;

        if (off_in_prolog > function_entry.prolog_size)
            return function_entry.stack_frame_size;

        const auto idx = function_entry.unwind_codes_idx;
        const auto nb  = function_entry.unwind_codes_nb;
        for (auto it = function_table.unwinds.begin()+idx; it < function_table.unwinds.begin()+idx+nb; ++it){
            if (off_in_prolog > it->code_offset){
                return function_entry.stack_frame_size - it->stack_size_used;
            }
        }

        return {};
    }

    opt<mod_t> find_prev(const uint64_t addr, std::map<uint64_t, mod_t>& mod_map)
    {
        if (mod_map.empty())
            return {};

        // lower bound returns first item greater or equal
        auto it = mod_map.lower_bound(addr);
        const auto end = mod_map.end();
        if(it == end)
            return mod_map.rbegin()->second;

        // equal
        if(it->first == addr)
            return it->second;

        // stricly greater, go to previous item
        if(it == mod_map.begin())
            return {};

        return (--it)->second;
    }
}


opt<mod_t> CallstackNt::find_mod(proc_t proc, uint64_t addr)
{
    std::map<uint64_t, mod_t> mod_map;

    auto it = d_->modules_by_proc_.find(proc.id);

    if(it == d_->modules_by_proc_.end())
        it = d_->modules_by_proc_.emplace(proc.id, mod_map).first;

    mod_map = it->second;

    auto mod = find_prev(addr, mod_map);
    if (mod)
    {
        const auto span = core_.os->mod_span(proc, *mod);
        if (addr <= (span->addr + span->size))
            return mod;
    }

    //Insert mod
    mod = core_.os->mod_find(proc, addr);
    if(!mod)
        return {};

    const auto span = core_.os->mod_span(proc, *mod);
    it->second.emplace(span->addr, *mod);

    return mod;
}

opt<FunctionTable> CallstackNt::insert(const std::string& name, const span_t span)
{
    const auto exception_dir = pe_.get_directory_entry(core_, span, pe::pe_directory_entries_e::IMAGE_DIRECTORY_ENTRY_EXCEPTION);
    if (!exception_dir)
        FAIL({}, "Unable to get span of exception_dir");

    std::vector<uint8_t> buffer_excep;
    buffer_excep.resize(exception_dir->size);
    auto ok = core_.mem.virtual_read(&buffer_excep[0], exception_dir->addr, exception_dir->size);
    if (!ok)
        FAIL({}, "unable to read exception dir of %s", name.c_str());

    const auto function_table = parse_exception_dir(core_, &buffer_excep[0], span.addr, *exception_dir);

    const auto ret = d_->exception_dirs_.emplace(name, *function_table);
    if(!ret.second)
        return {};

    return function_table;
}

opt<FunctionTable> CallstackNt::get_mod_functiontable(const std::string& name, const span_t span)
{
    const auto it = d_->exception_dirs_.find(name);
    if(it != d_->exception_dirs_.end())
        return it->second;

    return insert(name, span);
}

bool CallstackNt::get_callstack (proc_t proc, uint64_t rip, uint64_t rsp, uint64_t rbp, const on_callstep_fn& on_callstep)
{
    const auto proc_ctx = core_.mem.switch_process(proc);
    CurrentContext ctx = {rip, rsp, rbp};

    int i = 0;
    std::vector<uint8_t> buffer_mod;
    int max_cs_depth = 150;
    while(i<max_cs_depth){
        //Get module from address
        const auto mc = find_mod(proc, ctx.rip);
        if(!mc)
            FAIL(false, "Unable to find module");
        const auto modname  = core_.os->mod_name(proc, *mc);
        const auto span     = core_.os->mod_span(proc, *mc);

        //Load PDB
        if(false)
        {
            const auto debug_dir = pe_.get_directory_entry(core_, *span, pe::pe_directory_entries_e::IMAGE_DIRECTORY_ENTRY_DEBUG);
            buffer_mod.resize(debug_dir->size);
            auto ok = core_.mem.virtual_read(&buffer_mod[0], debug_dir->addr, debug_dir->size);
            if(!ok)
                return WALK_NEXT;

            const auto codeview = pe_.parse_debug_dir(&buffer_mod[0], span->addr, *debug_dir);
            buffer_mod.resize(codeview->size);
            ok = core_.mem.virtual_read(&buffer_mod[0], codeview->addr, codeview->size);
            if (!ok)
                FAIL(WALK_NEXT, "Unable to read IMAGE_CODEVIEW (RSDS)");

            ok = core_.sym.insert(sanitizer::sanitize_filename(*modname).data(), *span, &buffer_mod[0], buffer_mod.size());
        }

        // Get function table of the module
        const auto function_table = get_mod_functiontable(*modname, *span);
        if (!function_table)
            FAIL(false, "unable to get function table of %s", modname->c_str());

        const auto off_in_mod = (ctx.rip-span->addr) & 0xffffffff;
        const auto function_entry = lookup_function_entry(off_in_mod, function_table->function_entries);
        if (!function_entry)
            FAIL(false, "No matching function entry");

        const auto stack_frame_size = get_stack_frame_size(off_in_mod, *function_table, *function_entry);
        if (!stack_frame_size)
            FAIL(false, "Can't calculate stack frame size");

        if (function_entry->frame_reg_offset != 0)
            ctx.rsp = ctx.rbp - function_entry->frame_reg_offset;

        if (function_entry->prev_frame_reg != 0)
            ctx.rbp = *(core::read_ptr(core_, ctx.rsp-function_entry->prev_frame_reg));

        const auto caller_addr_on_stack = ctx.rsp + *stack_frame_size;

        // print stack
#ifdef USE_DEBUG_PRINT
        const auto print_d = 25;
        for (int k = 0; k < print_d*8; k += 8){
            LOG(INFO, "%" PRIx64 " - %" PRIx64, ctx.rsp+k,*core::read_ptr(core_, ctx.rsp+k));
        }
#endif

        const auto return_addr = core::read_ptr(core_, caller_addr_on_stack);

#ifdef USE_DEBUG_PRINT
        LOG(INFO, "Chosen chosen %" PRIx64 " start address %" PRIx32 " end %" PRIx32, off_in_mod, function_entry->start_address, function_entry->end_address);
        LOG(INFO, "Offset of current func %" PRIx64 ", Caller address on stack %" PRIx64 " so %" PRIx64, off_in_mod, caller_addr_on_stack, *return_addr);
#endif

        if(on_callstep(callstack::callstep_t{ctx.rip}) == WALK_STOP)
            return true;

        ctx.rip = *return_addr;
        ctx.rsp = caller_addr_on_stack + 8;

        i++;
    }

    return true;
}

namespace
{
    void get_unwind_codes(std::vector<unwind_code_t>& unwind_codes, function_entry_t& function_entry, const std::vector<uint8_t>& buffer, uint32_t unwind_codes_size, uint32_t chained_info_size)
    {
        uint32_t register_size = 0x08;            //TODO Defined this somewhere else

        size_t idx = 0;
        while(idx < unwind_codes_size - chained_info_size)
        {
            const auto unwind_operation = buffer[idx+1] & 0x0F;
            const auto unwind_code_info = buffer[idx+1] >> 4;

            switch(unwind_operation){
                case UWOP_PUSH_NONVOL :
                    if (unwind_code_info == UWINFO_RBP)
                        function_entry.prev_frame_reg = function_entry.stack_frame_size;

                    function_entry.stack_frame_size += register_size;
                    break;
                case UWOP_ALLOC_LARGE :
                    if (unwind_code_info == 0){
                        const auto extra_info = read_le16(&buffer[idx+2]);
                        function_entry.stack_frame_size += extra_info * 8;
                        idx += 2;
                    } else if (unwind_code_info == 1){
                        const auto extra_info = read_le32(&buffer[idx+2]);
                        function_entry.stack_frame_size += extra_info;
                        idx += 4;
                    }
                    break;
                case UWOP_ALLOC_SMALL :
                    function_entry.stack_frame_size += unwind_code_info*8 + 8;
                    break;
                case UWOP_SET_FPREG :
                    break;
                case UWOP_SAVE_NONVOL :
                    if (unwind_code_info == UWINFO_RBP)
                        function_entry.prev_frame_reg = 8*read_le16(&buffer[idx+2]);

                    idx += 2;
                    break;
                case UWOP_SAVE_NONVOL_FAR :
                    if (unwind_code_info == UWINFO_RBP)
                        function_entry.prev_frame_reg = read_le32(&buffer[idx+2]);

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

            unwind_codes.push_back({function_entry.stack_frame_size, buffer[idx], buffer[idx+1]});

            idx += 2;
        }
    }

    opt<function_entry_t> lookup_mother_function_entry(uint32_t mother_start_addr, const std::vector<function_entry_t>& function_entries)
    {
        for (const auto& fe : function_entries)
        {
            if(mother_start_addr == fe.start_address)
                return fe;
        }
        return ext::nullopt;
    }
}

opt<FunctionTable> CallstackNt::parse_exception_dir(core::Core& core, const void* vsrc, uint64_t mod_base_addr, span_t exception_dir)
{
    const auto src = reinterpret_cast<const uint8_t*>(vsrc);

    FunctionTable function_table;
    std::vector<function_entry_t> orphan_function_entries;

    for (size_t i=0; i<exception_dir.size; i=i+sizeof(RuntimeFunction)){

        function_entry_t function_entry = {0,0,0,0,0,0,0,0,0};
        function_entry.start_address = read_le32(&src[i+offsetof(RuntimeFunction, start_address)]);
        function_entry.end_address = read_le32(&src[i+offsetof(RuntimeFunction, end_address)]);
        const auto unwind_info_ptr = read_le32(&src[i+offsetof(RuntimeFunction, unwind_info)]);

        UnwindInfo unwind_info;
        const auto to_read = mod_base_addr + unwind_info_ptr;

        auto ok = core.mem.virtual_read(unwind_info, to_read, sizeof unwind_info);
        if(!ok)
            FAIL({}, "unable to read unwind info");

        const bool chained_flag        = unwind_info[0] & UNWIND_CHAINED_FLAG_MASK;
        function_entry.prolog_size     = unwind_info[1];
        function_entry.unwind_codes_nb = unwind_info[2];

        //Deal with frame register
        const auto frame_register = unwind_info[3] & 0x0F;            // register used as frame pointer
        function_entry.frame_reg_offset = 8*(unwind_info[3] >> 4);    // offset of frame register
        if (function_entry.frame_reg_offset != 0 && frame_register != UWINFO_RBP)
            LOG(ERROR, "WARNING : the used framed register is not rbp (code %d), this case is never used and not implemented", frame_register);

        const auto SIZE_UC = 2;

        const auto chained_info_size = chained_flag ? sizeof(RuntimeFunction) : 0;
        const auto unwind_codes_size = function_entry.unwind_codes_nb*SIZE_UC + chained_info_size;
        if(unwind_codes_size == 0){
            function_entry.stack_frame_size = 0;
            function_table.function_entries.push_back(function_entry);
            continue;
        }

        std::vector<uint8_t> buffer;
        buffer.resize(unwind_codes_size);
        core.mem.virtual_read(&buffer[0], mod_base_addr + unwind_info_ptr + sizeof(UnwindInfo), unwind_codes_size);
        if(!ok)
            FAIL({}, "unable to read unwind codes");

        function_entry.unwind_codes_idx = function_table.unwinds.size();
        get_unwind_codes(function_table.unwinds, function_entry, buffer, unwind_codes_size, chained_info_size);

        // Deal with the runtime func at the end
        uint32_t mother_start_addr = 0;
        if (chained_flag != 0)
        {
            mother_start_addr = read_le32(&buffer[unwind_codes_size-chained_info_size+offsetof(RuntimeFunction, start_address)]);

            const auto mother_function_entry = lookup_mother_function_entry(mother_start_addr, function_table.function_entries);
            if (!mother_function_entry)
            {
                function_entry.mother_start_addr = mother_start_addr;
                orphan_function_entries.push_back(function_entry);
                continue;
            }
            function_entry.stack_frame_size += mother_function_entry->stack_frame_size;
        }

        function_table.function_entries.push_back(function_entry);

#ifdef USE_DEBUG_PRINT
        LOG(INFO, "Function entry : start %" PRIx32 " end %" PRIx32 " prolog size %" PRIx8 " number of codes %" PRIx8  " unwind info pointer %" PRIx32
                " stack frame size %x", function_entry.start_address, function_entry.end_address, function_entry.prolog_size, function_entry.unwind_codes_nb,
                 unwind_info_ptr, function_entry.stack_frame_size);
#endif
    }

    for (auto& orphan_fe : orphan_function_entries){
        const auto mother_function_entry = lookup_mother_function_entry(orphan_fe.mother_start_addr, function_table.function_entries);
        if (!mother_function_entry)
            continue;   //Should never happend

        orphan_fe.stack_frame_size += mother_function_entry->stack_frame_size;
        function_table.function_entries.push_back(orphan_fe);
    }

    sort(function_table.function_entries.begin(), function_table.function_entries.end(), compare_function_entries);
    return function_table;
}

namespace{
    template<typename T>
    const function_entry_t* check_previous_exist(const T& it, const T& end, const uint32_t offset_in_mod)
    {
        if(it == end)
            return nullptr;

        if(offset_in_mod > it->end_address)
            return nullptr;

        return &(*it);
    }
}

const function_entry_t* CallstackNt::lookup_function_entry(uint32_t offset_in_mod, const std::vector<function_entry_t>& function_entries)
{
    // lower bound returns first item greater or equal
    auto it = lower_bound(function_entries.begin(), function_entries.end(), function_entry_t{offset_in_mod,0,0,0,0,0,0,0,0}, compare_function_entries);
    const auto end = function_entries.end();
    if(it == end)
        return check_previous_exist(function_entries.rbegin(), function_entries.rend(), offset_in_mod);

    // equal
    if(it->start_address == offset_in_mod)
        return check_previous_exist(it, end, offset_in_mod);

    // stricly greater, go to previous item
    return check_previous_exist(--it, end, offset_in_mod);
}
