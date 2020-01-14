#include "core.hpp"
#include "interfaces/if_os.hpp"

#include <array>

enum offset_e
{
    CLIENT_ID_UniqueThread,
    CONTROL_AREA_FilePointer,
    DEVICE_OBJECT_DriverObject,
    DRIVER_OBJECT_DriverName,
    EPROCESS_ActiveProcessLinks,
    EPROCESS_ImageFileName,
    EPROCESS_InheritedFromUniqueProcessId,
    EPROCESS_ObjectTable,
    EPROCESS_Pcb,
    EPROCESS_Peb,
    EPROCESS_SeAuditProcessCreationInfo,
    EPROCESS_ThreadListHead,
    EPROCESS_UniqueProcessId,
    EPROCESS_VadRoot,
    EPROCESS_Wow64Process,
    ETHREAD_Cid,
    ETHREAD_Tcb,
    ETHREAD_ThreadListEntry,
    EWOW64PROCESS_NtdllType,
    EWOW64PROCESS_Peb,
    FILE_OBJECT_DeviceObject,
    FILE_OBJECT_FileName,
    HANDLE_TABLE_TableCode,
    KPCR_Prcb,
    KPRCB_CurrentThread,
    KPRCB_KernelDirectoryTableBase,
    KPRCB_RspBase,
    KPRCB_RspBaseShadow,
    KPRCB_UserRspShadow,
    KPROCESS_DirectoryTableBase,
    KPROCESS_UserDirectoryTableBase,
    KTHREAD_Process,
    KTHREAD_TrapFrame,
    KTRAP_FRAME_Rip,
    KUSER_SHARED_DATA_NtMajorVersion,
    KUSER_SHARED_DATA_NtMinorVersion,
    MMVAD_FirstPrototypePte,
    MMVAD_SubSection,
    OBJECT_ATTRIBUTES_ObjectName,
    OBJECT_HEADER_Body,
    OBJECT_HEADER_InfoMask,
    OBJECT_HEADER_TypeIndex,
    OBJECT_NAME_INFORMATION_Name,
    OBJECT_TYPE_Name,
    PEB_Ldr,
    PEB32_Ldr,
    SE_AUDIT_PROCESS_CREATION_INFO_ImageFileName,
    SUBSECTION_ControlArea,
    OFFSET_COUNT,
};

enum symbol_e
{
    KiKernelSysretExit,
    KiKvaShadow,
    KiSwapThread,
    KiSystemCall64,
    KiSystemCall64Shadow,
    MiProcessLoaderEntry,
    ObHeaderCookie,
    ObpInfoMaskToOffset,
    ObpKernelHandleTable,
    ObpRootDirectoryObject,
    ObTypeIndexTable,
    PsActiveProcessHead,
    PsLoadedModuleList,
    PspExitProcess,
    PspExitThread,
    PspInsertThread,
    SwapContext,
    SYMBOL_COUNT,
};

namespace nt
{
    using Offsets = std::array<uint64_t, OFFSET_COUNT>;
    using Symbols = std::array<opt<uint64_t>, SYMBOL_COUNT>;

    struct Os;
    bool            load_kernel_symbols (nt::Os& os);
    opt<proc_t>     make_proc           (nt::Os& os, uint64_t eproc);
    opt<uint64_t>   read_vad_root_addr  (nt::Os& os, const memory::Io& io, proc_t proc, uint64_t vad_root_offset);
    bool            is_user_mode        (uint64_t cs);

    struct Os
        : public os::Module
    {
        Os(core::Core& core);

        // os::IModule
        bool        setup               () override;
        bool        is_kernel_address   (uint64_t ptr) override;
        bool        read_page           (void* dst, uint64_t ptr, proc_t* proc, dtb_t dtb) override;
        bool        write_page          (uint64_t ptr, const void* src, proc_t* proc, dtb_t dtb) override;
        opt<phy_t>  virtual_to_physical (proc_t* proc, dtb_t dtb, uint64_t ptr) override;
        dtb_t       kernel_dtb          () override;

        bool                proc_list       (process::on_proc_fn on_process) override;
        opt<proc_t>         proc_current    () override;
        opt<proc_t>         proc_find       (std::string_view name, flags_t flags) override;
        opt<proc_t>         proc_find       (uint64_t pid) override;
        opt<std::string>    proc_name       (proc_t proc) override;
        bool                proc_is_valid   (proc_t proc) override;
        flags_t             proc_flags      (proc_t proc) override;
        uint64_t            proc_id         (proc_t proc) override;
        void                proc_join       (proc_t proc, mode_e mode) override;
        opt<proc_t>         proc_parent     (proc_t proc) override;

        bool            thread_list     (proc_t proc, threads::on_thread_fn on_thread) override;
        opt<thread_t>   thread_current  () override;
        opt<proc_t>     thread_proc     (thread_t thread) override;
        opt<uint64_t>   thread_pc       (proc_t proc, thread_t thread) override;
        uint64_t        thread_id       (proc_t proc, thread_t thread) override;

        bool                mod_list(proc_t proc, modules::on_mod_fn on_module) override;
        opt<std::string>    mod_name(proc_t proc, mod_t mod) override;
        opt<span_t>         mod_span(proc_t proc, mod_t mod) override;
        opt<mod_t>          mod_find(proc_t proc, uint64_t addr) override;

        bool                vm_area_list    (proc_t proc, vm_area::on_vm_area_fn on_vm_area) override;
        opt<vm_area_t>      vm_area_find    (proc_t proc, uint64_t addr) override;
        opt<span_t>         vm_area_span    (proc_t proc, vm_area_t vm_area) override;
        vma_access_e        vm_area_access  (proc_t proc, vm_area_t vm_area) override;
        vma_type_e          vm_area_type    (proc_t proc, vm_area_t vm_area) override;
        opt<std::string>    vm_area_name    (proc_t proc, vm_area_t vm_area) override;

        bool                driver_list (drivers::on_driver_fn on_driver) override;
        opt<std::string>    driver_name (driver_t drv) override;
        opt<span_t>         driver_span (driver_t drv) override;

        opt<bpid_t> listen_proc_create  (const process::on_event_fn& on_create) override;
        opt<bpid_t> listen_proc_delete  (const process::on_event_fn& on_delete) override;
        opt<bpid_t> listen_thread_create(const threads::on_event_fn& on_create) override;
        opt<bpid_t> listen_thread_delete(const threads::on_event_fn& on_delete) override;
        opt<bpid_t> listen_mod_create   (proc_t proc, flags_t flags, const modules::on_event_fn& on_load) override;
        opt<bpid_t> listen_drv_create   (const drivers::on_event_fn& on_load) override;

        opt<arg_t>  read_stack  (size_t index) override;
        opt<arg_t>  read_arg    (size_t index) override;
        bool        write_arg   (size_t index, arg_t arg) override;

        void debug_print() override;

        // members
        core::Core& core_;
        Offsets     offsets_;
        Symbols     symbols_;
        std::string last_dump_;
        uint64_t    kpcr_;
        memory::Io  io_;
        size_t      num_page_faults_;

        // constants
        phy_t    LdrpInitializeProcess_;
        phy_t    LdrpSendDllNotifications_;
        uint32_t NtMajorVersion_;
        uint32_t NtMinorVersion_;
        uint64_t PhysicalMemoryLimitMask_;
    };
} // namespace nt
