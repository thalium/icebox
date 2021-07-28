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

    opt<span_t> find_kernel_at_up(nt::Os& os, uint64_t start, uint64_t end)
    {
        auto buf = std::array<uint8_t, PAGE_SIZE>{};
        for(auto ptr = utils::align<PAGE_SIZE>(start); ptr < end; ptr += PAGE_SIZE)
        {
            auto ok = os.io_.read_all(&buf[0], ptr, sizeof buf);
            if(!ok)
                continue;

            const auto size = pe::read_image_size(&buf[0], sizeof buf);
            if(!size)
                continue;

            return span_t{ptr, *size};
        }

        return {};
    }

    bool update_kernel_dtb(nt::Os& os)
    {
        auto gdtb = dtb_t{registers::read(os.core_, reg_e::cr3)};
        if(os.offsets_[KPRCB_KernelDirectoryTableBase])
        {
            auto kgdtb    = uint64_t{};
            const auto ok = memory::read_virtual_with_dtb(os.core_, gdtb, &kgdtb, os.kpcr_ + os.offsets_[KPCR_Prcb] + os.offsets_[KPRCB_KernelDirectoryTableBase], sizeof kgdtb);
            if(!ok)
                return FAIL(false, "unable to read KPRCB.KernelDirectoryTableBase");

            if(kgdtb)
                gdtb.val = kgdtb;
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

    struct kernel_t
    {
        span_t            span;
        symbols::Identity pdb;
    };

    constexpr uint64_t user_shared_data_addr = 0xFFFFF78000000000ULL;

    opt<dtb_t> find_some_dtb(nt::Os& os)
    {
        auto target = user_shared_data_addr;
        auto cr3    = registers::read(os.core_, reg_e::cr3);
        auto arg    = uint64_t{};
        auto ok     = memory::read_virtual_with_dtb(os.core_, dtb_t{cr3}, &arg, target, sizeof arg);
        if(ok)
            return dtb_t{cr3};

        for(uint64_t i = 0; i < UINT32_MAX; i += 1)
            if(memory::read_virtual_with_dtb(os.core_, dtb_t{i}, &arg, target, sizeof arg))
                return dtb_t{i};

        return std::nullopt;
    }

    opt<kernel_t> find_kernel_pdb(nt::Os& os)
    {
        auto lstar                   = registers::read_msr(os.core_, msr_e::lstar);
        auto const small_kernel_size = uint64_t{0x100000};
        auto max_kernel_size         = small_kernel_size;
        auto ea                      = lstar - max_kernel_size;

        auto dtb = find_some_dtb(os);
        if(!dtb)
            return FAIL(std::nullopt, "unable to find suitable cr3");

        LOG(INFO, "kernel: find with kdtb:%" PRIx64, dtb->val);
        os.io_.dtb = *dtb;
        while(true)
        {
            auto mod = find_kernel_at_up(os, ea, lstar);
            if(!mod)
            {
                if(max_kernel_size != small_kernel_size)
                    return FAIL(std::nullopt, "unable to read kernel");

                max_kernel_size <<= 4;
                ea = lstar - max_kernel_size;
                continue;
            }

            ea          = mod->addr + mod->size;
            auto opt_id = symbols::identify_pdb(*mod, os.io_);
            if(!opt_id)
                continue;

            if(opt_id->name != "ntkrnlmp.pdb")
                continue;

            LOG(INFO, "kernel: %" PRIx64 "-%" PRIx64 " size:0x%" PRIx64,
                mod->addr, mod->addr + mod->size, mod->size);
            return kernel_t{*mod, *opt_id};
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

    template <typename T>
    auto wait_for_predicate(core::Core& core, const T& predicate)
    {
        while(true)
        {
            const auto ret = predicate();
            if(ret)
                return ret;

            const auto ok = state::run_to_cr_write(core, reg_e::cr3);
            if(!ok)
                LOG(ERROR, "unable to run to cr3 write");
        }
    }

    bool wait_for_kernel_startup(core::Core& core, memory::Io& io)
    {
        return wait_for_predicate(core, [&]()
        {
            auto lstar = registers::read_msr(core, msr_e::lstar);
            if(!lstar)
                return false;

            io.dtb.val = registers::read(core, reg_e::cr3);
            LOG(INFO, "kernel started: lstar:%" PRIx64 " cr3:%" PRIx64, lstar, io.dtb.val);
            return true;
        });
    }

    opt<proc_t> wait_for_system_process(nt::Os& os)
    {
        const auto PsInitialSystemProcess = symbols::address(os.core_, symbols::kernel, "nt", "PsInitialSystemProcess");
        if(!PsInitialSystemProcess)
            return {};

        return wait_for_predicate(os.core_, [&]() -> opt<proc_t>
        {
            const auto system_process = os.io_.read(*PsInitialSystemProcess);
            if(!system_process || !*system_process)
                return {};

            LOG(INFO, "system process found: %" PRIx64, *system_process);
            return nt::make_proc(os, *system_process);
        });
    }

    bool try_load_ntdll(nt::Os& os, core::Core& core)
    {
        const auto proc = process::current(core);
        if(!proc)
            return false;

        const auto ntdll = modules::find_name(core, *proc, "ntdll.dll", flags::x64);
        if(!ntdll)
            return false;

        const auto proc_name = process::name(core, *proc);
        if(!proc_name)
            return false;

        LOG(INFO, "process %s: loading ntdll...", proc_name->data());
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

        LOG(INFO, "process %s: ntdll loaded", proc_name->data());
        return true;
    }

    bool wait_for_nt_version(nt::Os& os)
    {
        return wait_for_predicate(os.core_, [&]
        {
            const auto proc = process::current(os.core_);
            if(!proc)
                return false;

            // read the NtMajorVersion and NtMinorVersion from the _KUSER_SHARED_DATA
            auto ok = memory::read_virtual(os.core_, *proc, &os.NtMajorVersion_, user_shared_data_addr + os.offsets_[KUSER_SHARED_DATA_NtMajorVersion], sizeof os.NtMajorVersion_);
            if(!ok)
                return false;

            if(!os.NtMajorVersion_)
                return false;

            ok = memory::read_virtual(os.core_, *proc, &os.NtMinorVersion_, user_shared_data_addr + os.offsets_[KUSER_SHARED_DATA_NtMinorVersion], sizeof os.NtMinorVersion_);
            if(!ok)
                return false;

            LOG(INFO, "NT version: %d.%d", os.NtMajorVersion_, os.NtMinorVersion_);
            return true;
        });
    }

    bool wait_for_ntdll(nt::Os& os)
    {
        return wait_for_predicate(os.core_, [&] { return try_load_ntdll(os, os.core_); });
    }

    bool patch_nt_create_paging_file(nt::Os& os, core::Core& core, uint64_t where)
    {
        auto proc = process::current(core);
        if(!proc)
            return false;

        auto name = process::name(core, *proc);
        if(!name)
            return false;

        LOG(INFO, "patching NtCreatePagingFile from %s", name->c_str());
        auto rip    = registers::read(core, reg_e::rip);
        auto buffer = std::vector<uint8_t>(1024);
        auto ok     = os.io_.read_all(&buffer[0], where, buffer.size());
        if(!ok)
            return false;

        static constexpr uint8_t inst_retn = 0xc3;
        for(size_t i = 0; i < buffer.size(); ++i)
            if(buffer[i] == inst_retn)
            {
                rip += i;
                registers::write(core, reg_e::rip, rip);
                registers::write(core, reg_e::rax, 0); // force STATUS_SUCCESS
                return true;
            }

        return false;
    }

    void try_patch_nt_create_paging_file(nt::Os& os, core::Core& core)
    {
        const auto has_csrss = process::find_name(os.core_, "csrss.exe", flags::x64);
        if(has_csrss)
            return;

        LOG(INFO, "hot-patching NtCreatePagingFile...");
        const auto where = symbols::address(core, symbols::kernel, "nt", "NtCreatePagingFile");
        auto done        = false;
        const auto bp    = state::break_on(core, "NtCreatePagingFile", *where, [&] { done = patch_nt_create_paging_file(os, core, *where); });
        while(!done)
            state::exec(core);
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

    LOG(INFO, "kernel: kpcr:%" PRIx64, kpcr_);
    auto kernel = find_kernel_pdb(*this);
    if(!kernel)
        return FAIL(false, "unable to find kernel");

    kernel_ = kernel->span;
    LOG(INFO, "%s %s", kernel->pdb.name.data(), kernel->pdb.id.data());
    const auto pdb = symbols::make_pdb(kernel->pdb.name, kernel->pdb.id);
    if(!pdb)
        return FAIL(false, "unable to read kernel PDB");

    ok = core_.symbols_->insert(symbols::kernel, "nt", kernel->span, pdb);
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
        return FAIL(false, "unable to update to kernel dtb");

    // now we have kernel symbols, break after the PsSystemProcess creation if required
    const auto proc = wait_for_system_process(*this);
    if(!proc)
        return FAIL(false, "unable to find system process");

    // now update kernel dtb with the one read from system process
    io_.dtb.val = proc->kdtb.val;
    LOG(WARNING, "kernel: kdtb:%" PRIx64, io_.dtb.val);

    init_nt_mmu(*this);
    ok = force_winpe_mode(core_, io_);
    if(!ok)
        return FAIL(false, "unable to force winpe mode");

    // we need to set os major & minor version as soon as possible
    // because it is used during vad to pte resolution
    ok = wait_for_nt_version(*this);
    if(!ok)
        return FAIL(false, "unable to read nt major & minor version");

    // wait for any process containing ntdll.dll
    // usually smss.exe during boot
    const auto opt_ntdll = wait_for_ntdll(*this);
    if(!opt_ntdll)
        return FAIL(false, "unable to wait for ntdll.dll");

    // if csrss does not exist patch NtCreatePagingFile to return false
    // and disable completely paging to disk
    if(false)
        try_patch_nt_create_paging_file(*this, core_);

    return true;
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
