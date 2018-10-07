#include "os.hpp"

#define FDP_MODULE "os_nt"
#include "log.hpp"
#include "core.hpp"
#include "endian.hpp"
#include "utils.hpp"
#include "sym.hpp"
#include "utf8.hpp"
#include "core_helpers.hpp"

#include <algorithm>
#include <array>
#include <string>

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

namespace
{
    struct span_t
    {
        uint64_t    addr;
        size_t      size;
    };

    enum member_offset_e
    {
        EPROCESS_SeAuditProcessCreationInfo,
        EPROCESS_ImageFileName,
        EPROCESS_ActiveProcessLinks,
        EPROCESS_Pcb,
        EPROCESS_Peb,
        KPCR_CurrentPrcb,
        KPRCB_CurrentThread,
        KPROCESS_DirectoryTableBase,
        KTHREAD_Process,
        PEB_ProcessParameters,
        RTL_USER_PROCESS_PARAMETERS_ImagePathName,
        SE_AUDIT_PROCESS_CREATION_INFO_ImageFileName,
        OBJECT_NAME_INFORMATION_Name,
        MEMBER_OFFSET_COUNT,
    };

    struct MemberOffset
    {
        member_offset_e e_id;
        const char      struc[32];
        const char      member[32];
    };
    const MemberOffset g_member_offsets[] =
    {
        {EPROCESS_SeAuditProcessCreationInfo,           "_EPROCESS",                        "SeAuditProcessCreationInfo"},
        {EPROCESS_ImageFileName,                        "_EPROCESS",                        "ImageFileName"},
        {EPROCESS_ActiveProcessLinks,                   "_EPROCESS",                        "ActiveProcessLinks"},
        {EPROCESS_Pcb,                                  "_EPROCESS",                        "Pcb"},
        {EPROCESS_Peb,                                  "_EPROCESS",                        "Peb"},
        {KPCR_CurrentPrcb,                              "_KPCR",                            "CurrentPrcb"},
        {KPRCB_CurrentThread,                           "_KPRCB",                           "CurrentThread"},
        {KPROCESS_DirectoryTableBase,                   "_KPROCESS",                        "DirectoryTableBase"},
        {KTHREAD_Process,                               "_KTHREAD",                         "Process"},
        {PEB_ProcessParameters,                         "_PEB",                             "ProcessParameters"},
        {RTL_USER_PROCESS_PARAMETERS_ImagePathName,     "_RTL_USER_PROCESS_PARAMETERS",     "ImagePathName"},
        {SE_AUDIT_PROCESS_CREATION_INFO_ImageFileName,  "_SE_AUDIT_PROCESS_CREATION_INFO",  "ImageFileName"},
        {OBJECT_NAME_INFORMATION_Name,                  "_OBJECT_NAME_INFORMATION",         "Name"},
    };
    static_assert(COUNT_OF(g_member_offsets) == MEMBER_OFFSET_COUNT, "invalid members");

    enum symbol_offset_e
    {
        KiSystemCall64,
        PsActiveProcessHead,
        PsInitialSystemProcess,
        SYMBOL_OFFSET_COUNT,
    };

    struct SymbolOffset
    {
        symbol_offset_e e_id;
        const char      name[32];
    };
    const SymbolOffset g_symbol_offsets[] =
    {
        {KiSystemCall64,            "KiSystemCall64"},
        {PsActiveProcessHead,       "PsActiveProcessHead"},
        {PsInitialSystemProcess,    "PsInitialSystemProcess"},
    };
    static_assert(COUNT_OF(g_symbol_offsets) == SYMBOL_OFFSET_COUNT, "invalid symbols");

    using Sym           = std::unique_ptr<sym::IModule>;
    using MemberOffsets = std::array<uint64_t, MEMBER_OFFSET_COUNT>;
    using SymbolOffsets = std::array<uint64_t, SYMBOL_OFFSET_COUNT>;

    struct OsNt
        : public os::IHelper
    {
        OsNt(ICore& core);

        // methods
        bool setup();

        // os::IHelper
        std::string_view name() const override;
        bool             list_procs(const on_process_fn& on_process) override;
        os::proc_t       get_current_proc() override;
        os::proc_t       get_proc(const std::string& name) override;
        std::string      get_name(os::proc_t proc) override;

        // members
        ICore&          core_;
        span_t          kernel_;
        MemberOffsets   members_;
        SymbolOffsets   symbols_;
    };
}

OsNt::OsNt(ICore& core)
    : core_(core)
{
}

namespace
{
    std::optional<size_t> validate_pe_header(const uint8_t (&buf)[PAGE_SIZE])
    {
        static const auto e_lfanew_offset = 0x3C;
        int idx = e_lfanew_offset;
        if(idx + 4 > sizeof buf)
            return std::nullopt;

        const auto e_lfanew = read_le32(&buf[idx]);
        idx = e_lfanew;

        static const uint32_t image_nt_signature = 'PE' << 16;
        if(idx + sizeof image_nt_signature > sizeof buf)
            return std::nullopt;

        const auto signature = read_be32(&buf[idx]);
        if(signature != image_nt_signature)
            return std::nullopt;

        static const uint16_t image_file_machine_amd64 = 0x8664;
        idx += sizeof signature;
        if(idx + sizeof image_file_machine_amd64 > sizeof buf)
            return std::nullopt;

        const auto machine = read_le16(&buf[idx]);
        if(machine != image_file_machine_amd64)
            return std::nullopt;

        static const int image_file_header_size = 20;
        static const uint16_t image_nt_optional_hdr64_magic = 0x20B;
        idx += image_file_header_size;
        if(idx + sizeof image_nt_optional_hdr64_magic > sizeof buf)
            return std::nullopt;

        const auto magic = read_le16(&buf[idx]);
        if(magic != image_nt_optional_hdr64_magic)
            return std::nullopt;

        static const auto size_of_image_offset = 14 * 4;
        idx += size_of_image_offset;
        if(idx + 4 > sizeof buf)
            return std::nullopt;

        return read_le32(&buf[idx]);
    }

    std::optional<span_t> find_kernel(ICore& core, uint64_t lstar)
    {
        uint8_t buf[PAGE_SIZE];
        for(auto ptr = align<PAGE_SIZE>(lstar); ptr < lstar; ptr -= PAGE_SIZE)
        {
            auto ok = core.read_mem(buf, ptr, sizeof buf);
            if(!ok)
                return std::nullopt;

            const auto e_magic = read_be16(buf);
            static const auto image_dos_signature = 'MZ';
            if(e_magic != image_dos_signature)
                continue;

            const auto size = validate_pe_header(buf);
            if(!size)
            {
                LOG(ERROR, "invalid PE header at 0x%llx", ptr);
                continue;
            }

            return span_t{ptr, *size};
        }

        return std::nullopt;
    }

    std::vector<uint8_t> read_buffer(ICore& core, span_t span)
    {
        uint8_t page[PAGE_SIZE];

        std::vector<uint8_t> buffer;
        buffer.reserve(span.size);
        for(size_t i = 0; i < span.size; i += sizeof page)
        {
            const auto chunk = std::min<size_t>(PAGE_SIZE, std::max<int64_t>(0, span.size - i));
            const auto ok = core.read_mem(page, span.addr + i, chunk);
            if(!ok)
            {
                buffer.clear();
                return buffer;
            }

            const auto old = buffer.size();
            buffer.resize(old + sizeof page);
            memcpy(&buffer[old], page, sizeof page);
        }

        return buffer;
    }

    struct PdbCtx
    {
        std::string guid;
        std::string name;
    };

    void binhex(char* dst, const void* vsrc, size_t size)
    {
        static const char hexchars_upper[] = "0123456789ABCDEF";
        const uint8_t* src = static_cast<const uint8_t*>(vsrc);
        for (size_t i = 0; i < size; ++i)
        {
            dst[i * 2 + 0] = hexchars_upper[src[i] >> 4];
            dst[i * 2 + 1] = hexchars_upper[src[i] & 0x0F];
        }
    }

    std::optional<PdbCtx> read_pdb(ICore& core, span_t kernel)
    {
        const auto buf = read_buffer(core, kernel);

        std::vector<uint8_t> magic = { 'R', 'S', 'D', 'S' };
        const auto it = std::search(buf.begin(), buf.end(), std::boyer_moore_horspool_searcher(magic.begin(), magic.end()));
        if(it == buf.end())
            FAIL(std::nullopt, "unable to find RSDS pattern into kernel module");

        const auto rsds = &*it;
        const auto size = std::distance(it, buf.end());
        if(size < 4 /*magic*/ + 16 /*guid*/ + 4 /*age*/ + 2 /*name*/)
            FAIL(std::nullopt, "kernel module is too small for pdb header");

        const auto end = reinterpret_cast<const uint8_t*>(memchr(&rsds[4 + 16 + 4], 0x00, size));
        if(!end)
            FAIL(std::nullopt, "missing null-terminating byte on PDB header module name");

        uint8_t guid[16];
        write_be32(&guid[0], read_le32(&rsds[4 + 0]));  // Data1
        write_be16(&guid[4], read_le16(&rsds[4 + 4]));  // Data2
        write_be16(&guid[6], read_le16(&rsds[4 + 6 ])); // Data3
        memcpy(&guid[8], &rsds[4 + 8], 8);              // Data4

        char strguid[sizeof guid * 2];
        binhex(strguid, &guid, sizeof guid);

        uint32_t age = read_le32(&rsds[4 + 16]);
        const auto name = &rsds[4 + 16 + 4];
        return PdbCtx{std::string{strguid, sizeof strguid} + std::to_string(age), {name, end}};
    }
}

bool OsNt::setup()
{
    const auto lstar = core_.read_msr(MSR_LSTAR);
    if(!lstar)
        return false;

    const auto kernel = find_kernel(core_, *lstar);
    if(!kernel)
        FAIL(false, "unable to find kernel");

    LOG(INFO, "kernel: 0x%016llx - 0x%016llx (%lld 0x%llx)", kernel->addr, kernel->addr + kernel->size, kernel->size, kernel->size);
    const auto pdb = read_pdb(core_, *kernel);
    if(!pdb)
        FAIL(false, "unable to read pdb in kernel module");

    LOG(INFO, "kernel: pdb: %s %s", pdb->guid.data(), pdb->name.data());
    auto sym = sym::make_pdb(pdb->name.data(), pdb->guid.data());
    if(!sym)
        FAIL(false, "unable to read pdb from %s %s", pdb->name.data(), pdb->guid.data());

    bool fail = false;
    for(size_t i = 0; i < SYMBOL_OFFSET_COUNT; ++i)
    {
        const auto offset = sym->get_offset(g_symbol_offsets[i].name);
        if(!offset)
        {
            fail = true;
            LOG(ERROR, "unable to read %s symbol offset from pdb", g_symbol_offsets[i].name);
            continue;
        }

        symbols_[i] = *offset;
    }
    for(size_t i = 0; i < MEMBER_OFFSET_COUNT; ++i)
    {
        const auto offset = sym->get_struc_member_offset(g_member_offsets[i].struc, g_member_offsets[i].member);
        if(!offset)
        {
            fail = true;
            LOG(ERROR, "unable to read %s.%s member offset from pdb", g_member_offsets[i].struc, g_member_offsets[i].member);
            continue;
        }

        members_[i] = *offset;
    }
    if(fail)
        return false;

    const auto KiSystemCall64 = kernel->addr + symbols_[::KiSystemCall64];
    if(*lstar != KiSystemCall64)
        FAIL(false, "PDB mismatch lstar: 0x%llx pdb: 0x%llx\n", *lstar, KiSystemCall64);

    kernel_ = *kernel;
    return true;
}

std::unique_ptr<os::IHelper> os::make_nt(ICore& core)
{
    auto nt = std::make_unique<OsNt>(core);
    if(!nt)
        return std::nullptr_t();

    const auto ok = nt->setup();
    if(!ok)
        return std::nullptr_t();

    return nt;
}

std::string_view OsNt::name() const
{
    return "nt";
}

bool OsNt::list_procs(const on_process_fn& on_process)
{
    const auto head = kernel_.addr + symbols_[PsActiveProcessHead];
    for(auto link = core::read_ptr(core_, head); link != head; link = core::read_ptr(core_, *link))
        if(on_process(*link - members_[EPROCESS_ActiveProcessLinks]) == WALK_STOP)
            break;
    return true;
}

namespace
{
    std::optional<uint64_t> read_gs_base(ICore& core)
    {
        auto gs = core.read_msr(MSR_GS_BASE);
        if(!gs)
            return std::nullopt;

        if(*gs & 0xFFF0000000000000)
            return gs;

        gs = core.read_msr(MSR_KERNEL_GS_BASE);
        if(!gs)
            return std::nullopt;

        return gs;
    }
}

os::proc_t OsNt::get_current_proc()
{
    const auto gs = read_gs_base(core_);
    if(!gs)
        return 0;

    const auto current_prcb = core::read_ptr(core_, *gs + members_[KPCR_CurrentPrcb]);
    if(!current_prcb)
        FAIL(0, "unable to read KPCR.CurrentPrcb");

    const auto current_thread = core::read_ptr(core_, *current_prcb + members_[KPRCB_CurrentThread]);
    if(!current_thread)
        FAIL(0, "unable to read KPRCB.CurrentThread");

    const auto process = core::read_ptr(core_, *current_thread + members_[KTHREAD_Process]);
    if(!process)
        FAIL(0, "unable to read KTHREAD.Process");

    return *process - members_[EPROCESS_Pcb];
}

os::proc_t OsNt::get_proc(const std::string& name)
{
    os::proc_t found = 0;
    list_procs([&](os::proc_t proc)
    {
        const auto got = get_name(proc);
        if(got != name)
            return WALK_NEXT;

        found = proc;
        return WALK_STOP;
    });
    return found;
}

namespace
{
    struct Context
    {
        Context(OsNt& os, uint64_t value)
            : core_(os.core_)
            , value_(value)
        {
        }

        ~Context()
        {
            core_.write_reg(FDP_CR3_REGISTER, value_);
        }

        ICore&   core_;
        uint64_t value_;
    };

    std::optional<Context> set_context(OsNt& os, os::proc_t proc)
    {
        const auto directory_table_base = core::read_ptr(os.core_, proc + os.members_[EPROCESS_Pcb] + os.members_[KPROCESS_DirectoryTableBase]);
        if(!directory_table_base)
            FAIL(std::nullopt, "unable to read KPROCESS.DirectoryTableBase");

        const auto backup = os.core_.read_reg(FDP_CR3_REGISTER);
        if(!backup)
            return std::nullopt;

        const auto ok = os.core_.write_reg(FDP_CR3_REGISTER, *directory_table_base);
        if(!ok)
            return std::nullopt;

        return Context{os, *directory_table_base};
    }

    std::string read_unicode_string(ICore& core, uint64_t unicode_string)
    {
        using UnicodeString = struct
        {
            uint16_t length;
            uint16_t max_length;
            uint64_t buffer;
        };
        UnicodeString us;
        auto ok = core.read_mem(&us, unicode_string, sizeof us);
        if(!ok)
            FAIL(std::string{}, "unable to read UNICODE_STRING");

        us.length = read_le16(&us.length);
        us.max_length = read_le16(&us.max_length);
        us.buffer = read_le64(&us.buffer);

        std::wstring wname;
        wname.resize(us.length);
        ok = core.read_mem(wname.data(), us.buffer, us.length);
        if(!ok)
            FAIL(std::string{}, "uanble to read UNICODE_STRING.buffer");

        const auto name = utf8::convert(wname);
        if(!name)
            return std::string{};

        return *name;
    }
}

std::string OsNt::get_name(os::proc_t proc)
{
    static const std::string empty;
    if(!proc)
        return empty;

    // EPROCESS.ImageFileName is 16 bytes, but only 14 are actually used
    char buffer[14+1];
    const auto ok = core_.read_mem(buffer, proc + members_[EPROCESS_ImageFileName], sizeof buffer);
    buffer[sizeof buffer - 1] = 0;
    if(!ok)
        return empty;

    const auto name = std::string{buffer};
    if(name.size() < sizeof buffer - 1)
        return name;

    const auto image_file_name = core::read_ptr(core_, proc + members_[EPROCESS_SeAuditProcessCreationInfo] + members_[SE_AUDIT_PROCESS_CREATION_INFO_ImageFileName]);
    if(!image_file_name)
        return name;

    const auto path = read_unicode_string(core_, *image_file_name + members_[OBJECT_NAME_INFORMATION_Name]);
    if(path.empty())
        return name;

    return fs::path(path).filename().generic_string();
}
