#include "nt_os.hpp"

#define FDP_MODULE "nt::mod"
#include "log.hpp"
#include "nt_objects.hpp"
#include "wow64.hpp"

namespace
{
    constexpr auto x86_cs = 0x23;

    void on_LdrpInsertDataTableEntry(nt::Os& os, const modules::on_event_fn& on_mod)
    {
        const auto cs       = registers::read(os.core_, reg_e::cs);
        const auto is_32bit = cs == x86_cs;

        // LdrpInsertDataTableEntry has a fastcall calling convention whether it's in ntdll or ntdll32
        const auto rcx      = registers::read(os.core_, reg_e::rcx);
        const auto mod_addr = is_32bit ? static_cast<uint32_t>(rcx) : rcx;
        const auto flags    = is_32bit ? flags::x86 : flags::x64;
        on_mod({mod_addr, flags, {}});
    }

    opt<bpid_t> replace_bp(nt::Os& os, bpid_t bpid, const state::Breakpoint& bp)
    {
        state::drop_breakpoint(os.core_, bpid);
        return state::save_breakpoint_with(os.core_, bpid, bp);
    }

    opt<bpid_t> try_on_LdrpProcessMappedModule(nt::Os& os, proc_t proc, bpid_t bpid, const modules::on_event_fn& on_mod)
    {
        const auto where = symbols::address(os.core_, proc, "wntdll", "_LdrpProcessMappedModule@16");
        if(!where)
            return {};

        const auto ptr = &os;
        const auto bp  = state::break_on_process(os.core_, "wntdll!_LdrpProcessMappedModule@16", proc, *where, [=]
        {
            on_LdrpInsertDataTableEntry(*ptr, on_mod);
        });
        return replace_bp(os, bpid, bp);
    }

    bool try_load_wntdll(nt::Os& os, proc_t proc)
    {
        const auto objects = objects::make(os.core_, proc);
        if(!objects)
            return false;

        const auto entry = objects::find(*objects, 0, "\\KnownDlls32\\ntdll.dll");
        if(!entry)
            return false;

        const auto section = objects::as_section(*entry);
        if(!section)
            return false;

        const auto control_area = objects::section_control_area(*objects, *section);
        if(!control_area)
            return false;

        const auto segment = objects::control_area_segment(*objects, *control_area);
        if(!segment)
            return false;

        const auto span = objects::segment_span(*objects, *segment);
        if(!span)
            return false;

        const auto io = memory::make_io(os.core_, proc);
        return symbols::load_module_memory(os.core_, symbols::kernel, io, *span);
    }

    void on_LdrpInsertDataTableEntry_wow64(nt::Os& os, proc_t proc, bpid_t bpid, const modules::on_event_fn& on_mod)
    {
        if(!symbols::address(os.core_, proc, "wntdll", "_LdrpSendDllNotifications@12"))
            if(!try_load_wntdll(os, proc))
                return;

        try_on_LdrpProcessMappedModule(os, proc, bpid, on_mod);
    }
}

opt<bpid_t> nt::Os::listen_mod_create(proc_t proc, flags_t flags, const modules::on_event_fn& on_load)
{
    const auto name = "ntdll!LdrpSendDllNotifications";
    if(flags.is_x86)
    {
        const auto bpid     = state::acquire_breakpoint_id(core_);
        const auto opt_bpid = try_on_LdrpProcessMappedModule(*this, proc, bpid, on_load);
        if(opt_bpid)
            return opt_bpid;

        const auto bp = state::break_on_physical_process(core_, name, proc.udtb, LdrpSendDllNotifications_, [=]
        {
            on_LdrpInsertDataTableEntry_wow64(*this, proc, bpid, on_load);
        });
        return replace_bp(*this, bpid, bp);
    }

    const auto bp = state::break_on_physical_process(core_, name, proc.udtb, LdrpSendDllNotifications_, [=]
    {
        on_LdrpInsertDataTableEntry(*this, on_load);
    });
    return state::save_breakpoint(core_, bp);
}

opt<bpid_t> nt::Os::listen_drv_create(const drivers::on_event_fn& on_load)
{
    const auto bp = state::break_on(core_, "MiProcessLoaderEntry", *symbols_[MiProcessLoaderEntry], [=]
    {
        const auto drv_addr   = registers::read(core_, reg_e::rcx);
        const auto drv_loaded = registers::read(core_, reg_e::rdx);
        on_load({drv_addr}, drv_loaded);
    });
    return state::save_breakpoint(core_, bp);
}

namespace
{
    opt<walk_e> mod_list_64(const nt::Os& os, proc_t proc, const memory::Io& io, const modules::on_mod_fn& on_mod)
    {
        const auto peb = io.read(proc.id + os.offsets_[EPROCESS_Peb]);
        if(!peb)
            return FAIL(std::nullopt, "unable to read EPROCESS.Peb");

        // no PEB on system process
        if(!*peb)
            return walk_e::next;

        const auto ldr = io.read(*peb + os.offsets_[PEB_Ldr]);
        if(!ldr)
            return FAIL(std::nullopt, "unable to read PEB.Ldr");

        // Ldr = 0 before the process loads it's first module
        if(!*ldr)
            return walk_e::next;

        const auto head = *ldr + offsetof(nt::_PEB_LDR_DATA, InLoadOrderModuleList);
        for(auto link = io.read(head); link && link != head; link = io.read(*link))
        {
            const auto ret = on_mod({*link - offsetof(nt::_LDR_DATA_TABLE_ENTRY, InLoadOrderLinks), flags::x64, {}});
            if(ret == walk_e::stop)
                return ret;
        }

        return walk_e::next;
    }

    opt<uint64_t> read_wow64_peb(const nt::Os& os, const memory::Io& io, proc_t proc)
    {
        const auto wowp = io.read(proc.id + os.offsets_[EPROCESS_Wow64Process]);
        if(!wowp)
            return FAIL(std::nullopt, "unable to read EPROCESS.Wow64Process");

        if(!*wowp)
            return {};

        if(!os.offsets_[EWOW64PROCESS_NtdllType])
            return wowp;

        const auto peb32 = io.read(*wowp + os.offsets_[EWOW64PROCESS_Peb]);
        if(!peb32)
            return FAIL(std::nullopt, "unable to read EWOW64PROCESS.Peb");

        return *peb32;
    }

#define offsetof32(x, y) static_cast<uint32_t>(offsetof(x, y))

    opt<walk_e> mod_list_32(const nt::Os& os, proc_t proc, const memory::Io& io, const modules::on_mod_fn& on_mod)
    {
        const auto peb32 = read_wow64_peb(os, io, proc);
        if(!peb32)
            return {};

        // no PEB on system process
        if(!*peb32)
            return walk_e::next;

        const auto ldr32 = io.le32(*peb32 + os.offsets_[PEB32_Ldr]);
        if(!ldr32)
            return FAIL(std::nullopt, "unable to read PEB32.Ldr");

        // Ldr = 0 before the process loads it's first module
        if(!*ldr32)
            return walk_e::next;

        const auto head = *ldr32 + offsetof32(wow64::_PEB_LDR_DATA, InLoadOrderModuleList);
        for(auto link = io.le32(head); link && link != head; link = io.le32(*link))
        {
            const auto ret = on_mod({*link - offsetof32(wow64::_LDR_DATA_TABLE_ENTRY, InLoadOrderLinks), flags::x86, {}});
            if(ret == walk_e::stop)
                return ret;
        }

        return walk_e::next;
    }
}

bool nt::Os::mod_list(proc_t proc, modules::on_mod_fn on_mod)
{
    const auto io = memory::make_io(core_, proc);
    auto ret      = mod_list_64(*this, proc, io, on_mod);
    if(!ret)
        return false;
    if(*ret == walk_e::stop)
        return true;

    ret = mod_list_32(*this, proc, io, on_mod);
    return !!ret;
}

opt<std::string> nt::Os::mod_name(proc_t proc, mod_t mod)
{
    const auto io = memory::make_io(core_, proc);
    if(mod.flags.is_x86)
        return wow64::read_unicode_string(io, mod.id + offsetof32(wow64::_LDR_DATA_TABLE_ENTRY, FullDllName));

    return nt::read_unicode_string(io, mod.id + offsetof(nt::_LDR_DATA_TABLE_ENTRY, FullDllName));
}

opt<mod_t> nt::Os::mod_find(proc_t proc, uint64_t addr)
{
    opt<mod_t> found = {};
    mod_list(proc, [&](mod_t mod)
    {
        const auto span = mod_span(proc, mod);
        if(!span)
            return walk_e::next;

        if(!(span->addr <= addr && addr < span->addr + span->size))
            return walk_e::next;

        found = mod;
        return walk_e::stop;
    });
    return found;
}

namespace
{
    template <typename T>
    opt<span_t> read_ldr_span(const memory::Io& io, uint64_t ptr)
    {
        auto entry    = T{};
        const auto ok = io.read_all(&entry, ptr, sizeof entry);
        if(!ok)
            return {};

        return span_t{entry.DllBase, entry.SizeOfImage};
    }
}

opt<span_t> nt::Os::mod_span(proc_t proc, mod_t mod)
{
    const auto io = memory::make_io(core_, proc);
    if(mod.flags.is_x86)
        return read_ldr_span<wow64::_LDR_DATA_TABLE_ENTRY>(io, mod.id);

    return read_ldr_span<nt::_LDR_DATA_TABLE_ENTRY>(io, mod.id);
}

bool nt::Os::driver_list(drivers::on_driver_fn on_driver)
{
    const auto head = *symbols_[PsLoadedModuleList];
    for(auto link = io_.read(head); link != head; link = io_.read(*link))
        if(on_driver({*link - offsetof(nt::_LDR_DATA_TABLE_ENTRY, InLoadOrderLinks)}) == walk_e::stop)
            break;
    return true;
}

opt<std::string> nt::Os::driver_name(driver_t drv)
{
    return nt::read_unicode_string(io_, drv.id + offsetof(nt::_LDR_DATA_TABLE_ENTRY, FullDllName));
}

opt<span_t> nt::Os::driver_span(driver_t drv)
{
    return read_ldr_span<nt::_LDR_DATA_TABLE_ENTRY>(io_, drv.id);
}
