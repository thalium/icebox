#include "nt_os.hpp"

#define FDP_MODULE "nt"
#include "log.hpp"
#include "utils/utils.hpp"

#include <cstring>

namespace
{
    enum class cat_e
    {
        REQUIRED,
        OPTIONAL,
    };

    struct NtOffset
    {
        offset_e    e_id;
        cat_e       e_cat;
        const char* module;
        const char* struc;
        const char* member;
    };
    // clang-format off
    const NtOffset g_offsets[] =
    {
        {CLIENT_ID_UniqueThread,                       cat_e::REQUIRED, "nt", "_CLIENT_ID",                       "UniqueThread"},
        {CONTROL_AREA_FilePointer,                     cat_e::REQUIRED, "nt", "_CONTROL_AREA",                    "FilePointer"},
        {DEVICE_OBJECT_DriverObject,                   cat_e::REQUIRED, "nt", "_DEVICE_OBJECT",                   "DriverObject"},
        {DRIVER_OBJECT_DriverName,                     cat_e::REQUIRED, "nt", "_DRIVER_OBJECT",                   "DriverName"},
        {EPROCESS_ActiveProcessLinks,                  cat_e::REQUIRED, "nt", "_EPROCESS",                        "ActiveProcessLinks"},
        {EPROCESS_ImageFileName,                       cat_e::REQUIRED, "nt", "_EPROCESS",                        "ImageFileName"},
        {EPROCESS_InheritedFromUniqueProcessId,        cat_e::REQUIRED, "nt", "_EPROCESS",                        "InheritedFromUniqueProcessId"},
        {EPROCESS_ObjectTable,                         cat_e::REQUIRED, "nt", "_EPROCESS",                        "ObjectTable"},
        {EPROCESS_Pcb,                                 cat_e::REQUIRED, "nt", "_EPROCESS",                        "Pcb"},
        {EPROCESS_Peb,                                 cat_e::REQUIRED, "nt", "_EPROCESS",                        "Peb"},
        {EPROCESS_SeAuditProcessCreationInfo,          cat_e::REQUIRED, "nt", "_EPROCESS",                        "SeAuditProcessCreationInfo"},
        {EPROCESS_ThreadListHead,                      cat_e::REQUIRED, "nt", "_EPROCESS",                        "ThreadListHead"},
        {EPROCESS_UniqueProcessId,                     cat_e::REQUIRED, "nt", "_EPROCESS",                        "UniqueProcessId"},
        {EPROCESS_VadRoot,                             cat_e::REQUIRED, "nt", "_EPROCESS",                        "VadRoot"},
        {EPROCESS_Wow64Process,                        cat_e::REQUIRED, "nt", "_EPROCESS",                        "Wow64Process"},
        {ETHREAD_Cid,                                  cat_e::REQUIRED, "nt", "_ETHREAD",                         "Cid"},
        {ETHREAD_Tcb,                                  cat_e::REQUIRED, "nt", "_ETHREAD",                         "Tcb"},
        {ETHREAD_ThreadListEntry,                      cat_e::REQUIRED, "nt", "_ETHREAD",                         "ThreadListEntry"},
        {EWOW64PROCESS_NtdllType,                      cat_e::OPTIONAL, "nt", "_EWOW64PROCESS",                   "NtdllType"},
        {EWOW64PROCESS_Peb,                            cat_e::OPTIONAL, "nt", "_EWOW64PROCESS",                   "Peb"},
        {FILE_OBJECT_DeviceObject,                     cat_e::REQUIRED, "nt", "_FILE_OBJECT",                     "DeviceObject"},
        {FILE_OBJECT_FileName,                         cat_e::REQUIRED, "nt", "_FILE_OBJECT",                     "FileName"},
        {HANDLE_TABLE_TableCode,                       cat_e::REQUIRED, "nt", "_HANDLE_TABLE",                    "TableCode"},
        {KPCR_Prcb,                                    cat_e::REQUIRED, "nt", "_KPCR",                            "Prcb"},
        {KPRCB_CurrentThread,                          cat_e::REQUIRED, "nt", "_KPRCB",                           "CurrentThread"},
        {KPRCB_KernelDirectoryTableBase,               cat_e::OPTIONAL, "nt", "_KPRCB",                           "KernelDirectoryTableBase"},
        {KPRCB_RspBase,                                cat_e::REQUIRED, "nt", "_KPRCB",                           "RspBase"},
        {KPRCB_RspBaseShadow,                          cat_e::OPTIONAL, "nt", "_KPRCB",                           "RspBaseShadow"},
        {KPRCB_UserRspShadow,                          cat_e::OPTIONAL, "nt", "_KPRCB",                           "UserRspShadow"},
        {KPROCESS_DirectoryTableBase,                  cat_e::REQUIRED, "nt", "_KPROCESS",                        "DirectoryTableBase"},
        {KPROCESS_UserDirectoryTableBase,              cat_e::OPTIONAL, "nt", "_KPROCESS",                        "UserDirectoryTableBase"},
        {KTHREAD_Process,                              cat_e::REQUIRED, "nt", "_KTHREAD",                         "Process"},
        {KTHREAD_TrapFrame,                            cat_e::REQUIRED, "nt", "_KTHREAD",                         "TrapFrame"},
        {KTRAP_FRAME_Rip,                              cat_e::REQUIRED, "nt", "_KTRAP_FRAME",                     "Rip"},
        {KUSER_SHARED_DATA_NtMajorVersion,             cat_e::REQUIRED, "nt", "_KUSER_SHARED_DATA",               "NtMajorVersion"},
        {KUSER_SHARED_DATA_NtMinorVersion,             cat_e::REQUIRED, "nt", "_KUSER_SHARED_DATA",               "NtMinorVersion"},
        {MMVAD_FirstPrototypePte,                      cat_e::REQUIRED, "nt", "_MMVAD",                           "FirstPrototypePte"},
        {MMVAD_SubSection,                             cat_e::REQUIRED, "nt", "_MMVAD",                           "Subsection"},
        {OBJECT_ATTRIBUTES_ObjectName,                 cat_e::REQUIRED, "nt", "_OBJECT_ATTRIBUTES",               "ObjectName"},
        {OBJECT_HEADER_Body,                           cat_e::REQUIRED, "nt", "_OBJECT_HEADER",                   "Body"},
        {OBJECT_HEADER_InfoMask,                       cat_e::REQUIRED, "nt", "_OBJECT_HEADER",                   "InfoMask"},
        {OBJECT_HEADER_TypeIndex,                      cat_e::REQUIRED, "nt", "_OBJECT_HEADER",                   "TypeIndex"},
        {OBJECT_NAME_INFORMATION_Name,                 cat_e::REQUIRED, "nt", "_OBJECT_NAME_INFORMATION",         "Name"},
        {OBJECT_TYPE_Name,                             cat_e::REQUIRED, "nt", "_OBJECT_TYPE",                     "Name"},
        {PEB_Ldr,                                      cat_e::REQUIRED, "nt", "_PEB",                             "Ldr"},
        {PEB32_Ldr,                                    cat_e::REQUIRED, "nt", "_PEB32",                           "Ldr"},
        {SE_AUDIT_PROCESS_CREATION_INFO_ImageFileName, cat_e::REQUIRED, "nt", "_SE_AUDIT_PROCESS_CREATION_INFO",  "ImageFileName"},
        {SUBSECTION_ControlArea,                       cat_e::REQUIRED, "nt", "_SUBSECTION",                      "ControlArea"},
    };
    // clang-format on
    STATIC_ASSERT_EQ(COUNT_OF(g_offsets), OFFSET_COUNT);

    struct NtSymbol
    {
        symbol_e    e_id;
        cat_e       e_cat;
        const char* module;
        const char* name;
    };
    // clang-format off
    const NtSymbol g_symbols[] =
    {
        {KiKernelSysretExit,                    cat_e::OPTIONAL, "nt", "KiKernelSysretExit"},
        {KiKvaShadow,                           cat_e::OPTIONAL, "nt", "KiKvaShadow"},
        {KiSwapThread,                          cat_e::REQUIRED, "nt", "KiSwapThread"},
        {KiSystemCall64,                        cat_e::REQUIRED, "nt", "KiSystemCall64"},
        {KiSystemCall64Shadow,                  cat_e::OPTIONAL, "nt", "KiSystemCall64Shadow"},
        {MiProcessLoaderEntry,                  cat_e::REQUIRED, "nt", "MiProcessLoaderEntry"},
        {ObHeaderCookie,                        cat_e::OPTIONAL, "nt", "ObHeaderCookie"},
        {ObpInfoMaskToOffset,                   cat_e::REQUIRED, "nt", "ObpInfoMaskToOffset"},
        {ObpKernelHandleTable,                  cat_e::REQUIRED, "nt", "ObpKernelHandleTable"},
        {ObpRootDirectoryObject,                cat_e::REQUIRED, "nt", "ObpRootDirectoryObject"},
        {ObTypeIndexTable,                      cat_e::REQUIRED, "nt", "ObTypeIndexTable"},
        {PsActiveProcessHead,                   cat_e::REQUIRED, "nt", "PsActiveProcessHead"},
        {PsLoadedModuleList,                    cat_e::REQUIRED, "nt", "PsLoadedModuleList"},
        {PspExitProcess,                        cat_e::REQUIRED, "nt", "PspExitProcess"},
        {PspExitThread,                         cat_e::REQUIRED, "nt", "PspExitThread"},
        {PspInsertThread,                       cat_e::REQUIRED, "nt", "PspInsertThread"},
        {SwapContext,                           cat_e::REQUIRED, "nt", "SwapContext"},
    };
    // clang-format on
    static_assert(COUNT_OF(g_symbols) == SYMBOL_COUNT, "invalid symbols");
}

bool nt::load_kernel_symbols(nt::Os& os)
{
    bool fail   = false;
    int i       = -1;
    os.symbols_ = {};
    for(const auto& sym : g_symbols)
    {
        fail |= sym.e_id != ++i;
        const auto addr = symbols::address(os.core_, symbols::kernel, sym.module, sym.name);
        if(!addr)
        {
            fail |= sym.e_cat == cat_e::REQUIRED;
            if(sym.e_cat == cat_e::REQUIRED)
                LOG(ERROR, "unable to read %s!%s symbol offset", sym.module, sym.name);
            else
                LOG(WARNING, "unable to read optional %s!%s symbol offset", sym.module, sym.name);
            continue;
        }
        os.symbols_[i] = *addr;
    }

    i           = -1;
    os.offsets_ = {};
    for(const auto& off : g_offsets)
    {
        fail |= off.e_id != ++i;
        const auto opt_mb = symbols::read_member(os.core_, symbols::kernel, off.module, off.struc, off.member);
        if(!opt_mb)
        {
            fail |= off.e_cat == cat_e::REQUIRED;
            if(off.e_cat == cat_e::REQUIRED)
                LOG(ERROR, "unable to read %s!%s.%s member", off.module, off.struc, off.member);
            else
                LOG(WARNING, "unable to read optional %s!%s.%s member", off.module, off.struc, off.member);
            continue;
        }
        os.offsets_[i] = opt_mb->offset;
    }
    return !fail;
}
