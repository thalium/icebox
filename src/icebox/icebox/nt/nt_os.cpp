#include "nt_os.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "nt"
#include "core/core_private.hpp"
#include "interfaces/if_symbols.hpp"
#include "log.hpp"
#include "utils/hex.hpp"
#include "utils/pe.hpp"
#include "utils/utils.hpp"

nt::Os::Os(core::Core& core)
    : core_(core)
    , kpcr_(0)
    , io_(memory::make_io_current(core))
    , num_page_faults_(0)
    , LdrpInitializeProcess_{0}
    , LdrpSendDllNotifications_{0}
    , NtMajorVersion_{0}
    , NtMinorVersion_{0}
    , PhysicalMemoryLimitMask_{0}
{
}

namespace
{
    bool is_kernel(uint64_t ptr)
    {
        return !!(ptr & 0xFFF0000000000000);
    }

    opt<span_t> find_kernel_at(nt::Os& os, uint64_t needle)
    {
        auto buf = std::array<uint8_t, PAGE_SIZE>{};
        for(auto ptr = utils::align<PAGE_SIZE>(needle); ptr > PAGE_SIZE; ptr -= PAGE_SIZE)
        {
            auto ok = os.io_.read_all(&buf[0], ptr, sizeof buf);
            if(!ok)
                return {};

            const auto size = pe::read_image_size(&buf[0], sizeof buf);
            if(!size)
                continue;

            return span_t{ptr, *size};
        }

        return {};
    }

    opt<span_t> find_kernel(nt::Os& os, core::Core& core)
    {
        // try until we get a CR3 able to read kernel base address & size
        const auto lstar = registers::read_msr(core, msr_e::lstar);
        for(size_t i = 0; i < 64; ++i)
        {
            if(const auto kernel = find_kernel_at(os, lstar))
                return kernel;

            const auto ok = state::run_to_cr_write(core, reg_e::cr3);
            if(!ok)
                break;

            os.io_.dtb = dtb_t{registers::read(core, reg_e::cr3)};
        }

        return {};
    }

    bool read_phy_symbol(nt::Os& os, phy_t& dst, const memory::Io& io, const char* module, const char* name)
    {
        const auto where = symbols::address(os.core_, symbols::kernel, module, name);
        if(!where)
            return false;

        const auto phy = io.physical(*where);
        if(!phy)
            return false;

        dst = *phy;
        return true;
    }

    constexpr uint8_t inst_retn = 0xc3;

    opt<mod_t> wait_for_ntdll(nt::Os& os, core::Core& core)
    {
        const auto csrss = process::find_name(core, "csrss.exe", flags::x64);
        if(csrss)
        {
            process::join(core, *csrss, mode_e::kernel);
            return modules::find_name(core, *csrss, "ntdll.dll", flags::x64);
        }

        const auto sysret_exit = os.symbols_[KiKernelSysretExit] ? *os.symbols_[KiKernelSysretExit] : registers::read_msr(core, msr_e::lstar);
        auto ntdll             = opt<mod_t>{};
        auto breakpoints       = std::vector<state::Breakpoint>{};
        auto rips              = std::unordered_set<uint64_t>{};
        const auto bp          = state::break_on(core, "KiKernelSysretExit", sysret_exit, [&]
        {
            const auto opt_ret = os.read_arg(0);
            if(!opt_ret)
                return;

            const auto ret_addr = opt_ret->val;
            if(rips.count(ret_addr))
                return;

            const auto ret_bp = state::break_on(core, "KiKernelSysretExit return", ret_addr, [&]
            {
                const auto proc = process::current(core);
                if(!proc)
                    return;

                const auto proc_name = process::name(core, *proc);
                if(!proc_name)
                    return;

                if(*proc_name == "csrss.exe")
                {
                    ntdll = modules::find_name(core, *proc, "ntdll.dll", flags::x64);
                }
            });
            rips.insert(ret_addr);
            breakpoints.emplace_back(ret_bp);
        });
        // disable paging files
        const auto ntCreatePagingFile    = symbols::address(core, symbols::kernel, "nt", "NtCreatePagingFile");
        const auto bp_ntCreatePagingFile = state::break_on(core, "NtCreatePagingFile", *ntCreatePagingFile, [&]
        {
            auto proc = process::current(core);
            if(!proc)
                return;

            auto name = process::name(core, *proc);
            if(!name)
                return;

            LOG(INFO, "%s NtCreatePagingFile", name->c_str());
            auto rip    = registers::read(core, reg_e::rip);
            auto buffer = std::vector<uint8_t>(1024);
            auto ok     = os.io_.read_all(&buffer[0], *ntCreatePagingFile, buffer.size());
            if(!ok)
                return;

            for(size_t i = 0; i < buffer.size(); ++i)
                if(buffer[i] == inst_retn)
                {
                    rip += i;
                    registers::write(core, reg_e::rip, rip);
                    registers::write(core, reg_e::rax, 0); // force STATUS_SUCCESS
                    break;
                }
        });
        while(!ntdll)
            state::exec(core);

        return ntdll;
    }

    bool try_load_ntdll(nt::Os& os, core::Core& core)
    {
        const auto ntdll = wait_for_ntdll(os, core);
        if(!ntdll)
            return false;

        const auto proc = process::current(core);
        if(!proc)
            return false;

        const auto proc_name = process::name(core, *proc);
        if(!proc_name)
            return false;

        LOG(INFO, "loading ntdll from process %s", proc_name->data());
        const auto span = modules::span(core, *proc, *ntdll);
        if(!span)
            return false;

        const auto io = memory::make_io(core, *proc);
        auto ok       = symbols::load_module_memory(core, symbols::kernel, io, *span);
        if(!ok)
            return false;

        ok = read_phy_symbol(os, os.LdrpInitializeProcess_, io, "ntdll", "LdrpInitializeProcess");
        if(!ok)
            return false;

        ok = read_phy_symbol(os, os.LdrpSendDllNotifications_, io, "ntdll", "LdrpSendDllNotifications");
        if(!ok)
            return false;

        return true;
    }

    bool update_kernel_dtb(nt::Os& os)
    {
        auto gdtb = dtb_t{registers::read(os.core_, reg_e::cr3)};
        if(os.offsets_[KPRCB_KernelDirectoryTableBase])
        {
            const auto ok = memory::read_virtual_with_dtb(os.core_, gdtb, &gdtb, os.kpcr_ + os.offsets_[KPCR_Prcb] + os.offsets_[KPRCB_KernelDirectoryTableBase], sizeof gdtb);
            if(!ok)
                return FAIL(false, "unable to read KPRCB.KernelDirectoryTableBase");
        }
        os.io_.dtb = gdtb;
        return true;
    }

    void init_nt_mmu(nt::Os& os)
    {
        // see MiInitNucleus
        // PhysicalMemoryLimitMask = 1LL << ((uint8_t) KiImplementedPhysicalBits - 1);
        const auto KiImplementedPhysicalBits = symbols::address(os.core_, symbols::kernel, "nt", "KiImplementedPhysicalBits");
        if(!KiImplementedPhysicalBits)
            return;

        const auto physical_bits = os.io_.read(*KiImplementedPhysicalBits);
        if(!physical_bits)
            return;

        os.PhysicalMemoryLimitMask_ = 1LL << (*physical_bits - 1);
    }

    bool wait_for_kernel_startup(core::Core& core, memory::Io& io)
    {
        // check if the kernel is initialized...
        auto lstar = registers::read_msr(core, msr_e::lstar);
        if(lstar)
            return true;

        while(true)
        {
            auto ok = state::run_to_cr_write(core, reg_e::cr3);
            if(!ok)
                return false;

            lstar = registers::read_msr(core, msr_e::lstar);
            if(!lstar)
                continue;

            io.dtb.val = registers::read(core, reg_e::cr3);
            return true;
        }
    }

    opt<proc_t> wait_for_system_process(nt::Os& os)
    {
        const auto PsInitialSystemProcess = symbols::address(os.core_, symbols::kernel, "nt", "PsInitialSystemProcess");
        if(!PsInitialSystemProcess)
            return {};

        while(true)
        {
            const auto system_process = os.io_.read(*PsInitialSystemProcess);
            if(system_process && *system_process)
                return nt::make_proc(os, *system_process);

            const auto ok = state::run_to_cr_write(os.core_, reg_e::cr3);
            if(!ok)
                return {};
        }
    }

    bool force_winpe_mode(core::Core& core, memory::Io& io)
    {
        const auto InitWinPEModeType = symbols::address(core, symbols::kernel, "nt", "InitWinPEModeType");
        if(!InitWinPEModeType)
            return false;

        const auto mode_type = io.le32(*InitWinPEModeType);
        if(!mode_type)
            return false;

        constexpr auto INIT_WINPEMODE_INRAM = 0x80000000;
        if(*mode_type & INIT_WINPEMODE_INRAM)
        {
            LOG(INFO, "ram-only mode detected");
            return true;
        }

        const auto ok = io.write_le32(*InitWinPEModeType, *mode_type | INIT_WINPEMODE_INRAM);
        if(!ok)
            FAIL(false, "unable to set the global InitWinPEModeType to INIT_WINPEMODE_INRAM");

        LOG(INFO, "ram-only mode enabled");
        return true;
    }
}

bool nt::Os::setup()
{
    auto ok = wait_for_kernel_startup(core_, io_);
    if(!ok)
        return false;

    kpcr_ = registers::read_msr(core_, msr_e::gs_base);
    if(!is_kernel(kpcr_))
        kpcr_ = registers::read_msr(core_, msr_e::kernel_gs_base);
    if(!is_kernel(kpcr_))
        return FAIL(false, "unable to read KPCR");

    const auto kernel = find_kernel(*this, core_);
    if(!kernel)
        return FAIL(false, "unable to find kernel");

    LOG(INFO, "kernel: 0x%" PRIx64 "-0x%" PRIx64 " size:0x%" PRIx64,
        kernel->addr, kernel->addr + kernel->size, kernel->size);
    const auto opt_id = symbols::identify_pdb(*kernel, io_);
    if(!opt_id)
        return FAIL(false, "unable to identify kernel PDB");

    const auto pdb = symbols::make_pdb(opt_id->name, opt_id->id);
    if(!pdb)
        return FAIL(false, "unable to read kernel PDB");

    ok = core_.symbols_->insert(symbols::kernel, "nt", *kernel, pdb);
    if(!ok)
        return FAIL(false, "unable to load symbols from kernel module");

    ok = nt::load_kernel_symbols(*this);
    if(!ok)
        return false;

    // cr3 is same in user & kernel mode
    if(!offsets_[KPROCESS_UserDirectoryTableBase])
        offsets_[KPROCESS_UserDirectoryTableBase] = offsets_[KPROCESS_DirectoryTableBase];

    // read current kernel dtb, just to have a good one to join system process
    ok = update_kernel_dtb(*this);
    if(!ok)
        return false;

    // now we have kernel symbols, break after the PsSystemProcess creation if required
    const auto proc = wait_for_system_process(*this);
    if(!proc)
        return FAIL(false, "unable to find system process");

    // now update kernel dtb with the one read from system process
    proc_join(*proc, mode_e::kernel);
    io_.dtb.val = proc->kdtb.val;

    // read the NtMajorVersion and NtMinorVersion from the _KUSER_SHARED_DATA
    static constexpr uint64_t user_shared_data_addr = 0xFFFFF78000000000ULL;

    ok = memory::read_virtual(core_, *proc, &NtMajorVersion_, user_shared_data_addr + offsets_[KUSER_SHARED_DATA_NtMajorVersion], sizeof NtMajorVersion_);
    if(!ok)
        return false;

    ok = memory::read_virtual(core_, *proc, &NtMinorVersion_, user_shared_data_addr + offsets_[KUSER_SHARED_DATA_NtMinorVersion], sizeof NtMinorVersion_);
    if(!ok)
        return false;

    init_nt_mmu(*this);

    ok = force_winpe_mode(core_, io_);
    if(!ok)
        return false;

    LOG(WARNING, "kernel: kpcr:0x%" PRIx64 " kdtb:0x%" PRIx64 " version:%d.%d", kpcr_, io_.dtb.val, NtMajorVersion_, NtMinorVersion_);
    return try_load_ntdll(*this, core_);
}

std::shared_ptr<nt::Os> os::make_nt(core::Core& core)
{
    return std::make_shared<nt::Os>(core);
}

void os::attach(core::Core& core, nt::Os& os)
{
    core.os_ = &os;
}

dtb_t nt::Os::kernel_dtb()
{
    return io_.dtb;
}

namespace
{
    std::string irql_to_text(uint64_t value)
    {
        switch(value)
        {
            case 0: return "passive";
            case 1: return "apc";
            case 2: return "dispatch";
        }
        return std::to_string(value);
    }

    std::string to_hex(uint64_t x)
    {
        char buf[sizeof x * 2 + 1];
        return hex::convert<hex::LowerCase | hex::RemovePadding>(buf, x);
    }
}

void nt::Os::debug_print()
{
    const auto irql   = registers::read(core_, reg_e::cr8);
    const auto cs     = registers::read(core_, reg_e::cs);
    const auto rip    = registers::read(core_, reg_e::rip);
    const auto cr3    = registers::read(core_, reg_e::cr3);
    const auto thread = thread_current();
    const auto proc   = thread_proc(*thread);
    const auto ripsym = symbols::string(core_, *proc, rip);
    const auto name   = proc_name(*proc);
    const auto dump   = "rip:" + to_hex(rip)
                      + " cr3:" + to_hex(cr3)
                      + " kdtb:" + to_hex(proc ? proc->kdtb.val : 0)
                      + " udtb:" + to_hex(proc ? proc->udtb.val : 0)
                      + ' ' + irql_to_text(irql)
                      + ' ' + (nt::is_user_mode(cs) ? "user" : "kernel")
                      + (name ? " " + *name : "")
                      + (ripsym.empty() ? "" : " " + ripsym)
                      + " p:" + to_hex(proc ? proc->id : 0)
                      + " t:" + to_hex(thread ? thread->id : 0)
                      + " pf: " + std::to_string(num_page_faults_);
    if(dump != last_dump_)
        LOG(INFO, "%s", dump.data());
    last_dump_ = dump;
}
