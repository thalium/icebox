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
        cat_e       e_cat;
        offset_e    e_id;
        const char* module;
        const char* struc;
        const char* member;
    };
    // clang-format off
    const NtOffset g_offsets[] =
    {
        {cat_e::REQUIRED,   CLIENT_ID_UniqueThread,                       "nt", "_CLIENT_ID",                       "UniqueThread"},
        {cat_e::REQUIRED,   EPROCESS_ActiveProcessLinks,                  "nt", "_EPROCESS",                        "ActiveProcessLinks"},
        {cat_e::REQUIRED,   EPROCESS_ImageFileName,                       "nt", "_EPROCESS",                        "ImageFileName"},
        {cat_e::REQUIRED,   EPROCESS_Pcb,                                 "nt", "_EPROCESS",                        "Pcb"},
        {cat_e::REQUIRED,   EPROCESS_Peb,                                 "nt", "_EPROCESS",                        "Peb"},
        {cat_e::REQUIRED,   EPROCESS_SeAuditProcessCreationInfo,          "nt", "_EPROCESS",                        "SeAuditProcessCreationInfo"},
        {cat_e::REQUIRED,   EPROCESS_ThreadListHead,                      "nt", "_EPROCESS",                        "ThreadListHead"},
        {cat_e::REQUIRED,   EPROCESS_UniqueProcessId,                     "nt", "_EPROCESS",                        "UniqueProcessId"},
        {cat_e::REQUIRED,   EPROCESS_InheritedFromUniqueProcessId,        "nt", "_EPROCESS",                        "InheritedFromUniqueProcessId"},
        {cat_e::REQUIRED,   EPROCESS_VadRoot,                             "nt", "_EPROCESS",                        "VadRoot"},
        {cat_e::REQUIRED,   EPROCESS_Wow64Process,                        "nt", "_EPROCESS",                        "Wow64Process"},
        {cat_e::REQUIRED,   ETHREAD_Cid,                                  "nt", "_ETHREAD",                         "Cid"},
        {cat_e::REQUIRED,   ETHREAD_Tcb,                                  "nt", "_ETHREAD",                         "Tcb"},
        {cat_e::REQUIRED,   ETHREAD_ThreadListEntry,                      "nt", "_ETHREAD",                         "ThreadListEntry"},
        {cat_e::REQUIRED,   KPCR_Prcb,                                    "nt", "_KPCR",                            "Prcb"},
        {cat_e::REQUIRED,   KPRCB_CurrentThread,                          "nt", "_KPRCB",                           "CurrentThread"},
        {cat_e::OPTIONAL,   KPRCB_KernelDirectoryTableBase,               "nt", "_KPRCB",                           "KernelDirectoryTableBase"},
        {cat_e::REQUIRED,   KPROCESS_DirectoryTableBase,                  "nt", "_KPROCESS",                        "DirectoryTableBase"},
        {cat_e::OPTIONAL,   KPROCESS_UserDirectoryTableBase,              "nt", "_KPROCESS",                        "UserDirectoryTableBase"},
        {cat_e::REQUIRED,   KTHREAD_Process,                              "nt", "_KTHREAD",                         "Process"},
        {cat_e::REQUIRED,   KTHREAD_TrapFrame,                            "nt", "_KTHREAD",                         "TrapFrame"},
        {cat_e::REQUIRED,   KTRAP_FRAME_Rip,                              "nt", "_KTRAP_FRAME",                     "Rip"},
        {cat_e::REQUIRED,   OBJECT_NAME_INFORMATION_Name,                 "nt", "_OBJECT_NAME_INFORMATION",         "Name"},
        {cat_e::REQUIRED,   PEB_Ldr,                                      "nt", "_PEB",                             "Ldr"},
        {cat_e::REQUIRED,   PEB32_Ldr,                                    "nt", "_PEB32",                           "Ldr"},
        {cat_e::REQUIRED,   SE_AUDIT_PROCESS_CREATION_INFO_ImageFileName, "nt", "_SE_AUDIT_PROCESS_CREATION_INFO",  "ImageFileName"},
        {cat_e::OPTIONAL,   EWOW64PROCESS_Peb,                            "nt", "_EWOW64PROCESS",                   "Peb"},
        {cat_e::OPTIONAL,   EWOW64PROCESS_NtdllType,                      "nt", "_EWOW64PROCESS",                   "NtdllType"},
        {cat_e::REQUIRED,   MMVAD_SubSection,                             "nt", "_MMVAD",                           "Subsection"},
        {cat_e::REQUIRED,   MMVAD_FirstPrototypePte,                      "nt", "_MMVAD",                           "FirstPrototypePte"},
        {cat_e::REQUIRED,   SUBSECTION_ControlArea,                       "nt", "_SUBSECTION",                      "ControlArea"},
        {cat_e::REQUIRED,   CONTROL_AREA_FilePointer,                     "nt", "_CONTROL_AREA",                    "FilePointer"},
        {cat_e::REQUIRED,   FILE_OBJECT_FileName,                         "nt", "_FILE_OBJECT",                     "FileName"},
        {cat_e::REQUIRED,   KUSER_SHARED_DATA_NtMajorVersion,             "nt", "_KUSER_SHARED_DATA",               "NtMajorVersion"},
        {cat_e::REQUIRED,   KUSER_SHARED_DATA_NtMinorVersion,             "nt", "_KUSER_SHARED_DATA",               "NtMinorVersion"},
    };
    // clang-format on
    STATIC_ASSERT_EQ(COUNT_OF(g_offsets), OFFSET_COUNT);

    struct NtSymbol
    {
        cat_e       e_cat;
        symbol_e    e_id;
        const char* module;
        const char* name;
    };
    // clang-format off
    const NtSymbol g_symbols[] =
    {
        {cat_e::OPTIONAL, KiKernelSysretExit,                  "nt", "KiKernelSysretExit"},
        {cat_e::REQUIRED, PsActiveProcessHead,                 "nt", "PsActiveProcessHead"},
        {cat_e::REQUIRED, PsLoadedModuleList,                  "nt", "PsLoadedModuleList"},
        {cat_e::REQUIRED, PspInsertThread,                     "nt", "PspInsertThread"},
        {cat_e::REQUIRED, PspExitProcess,                      "nt", "PspExitProcess"},
        {cat_e::REQUIRED, PspExitThread,                       "nt", "PspExitThread"},
        {cat_e::REQUIRED, MiProcessLoaderEntry,                "nt", "MiProcessLoaderEntry"},
        {cat_e::REQUIRED, SwapContext,                         "nt", "SwapContext"},
        {cat_e::REQUIRED, KiSwapThread,                         "nt", "KiSwapThread"},
    };
    // clang-format on
    static_assert(COUNT_OF(g_symbols) == SYMBOL_COUNT, "invalid symbols");
}

bool nt::load_kernel_symbols(nt::Os& os)
{
    bool fail = false;
    int i     = -1;
    memset(&os.symbols_[0], 0, sizeof os.symbols_);
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

    i = -1;
    memset(&os.offsets_[0], 0, sizeof os.offsets_);
    for(const auto& off : g_offsets)
    {
        fail |= off.e_id != ++i;
        const auto offset = symbols::member_offset(os.core_, symbols::kernel, off.module, off.struc, off.member);
        if(!offset)
        {
            fail |= off.e_cat == cat_e::REQUIRED;
            if(off.e_cat == cat_e::REQUIRED)
                LOG(ERROR, "unable to read %s!%s.%s member offset", off.module, off.struc, off.member);
            else
                LOG(WARNING, "unable to read optional %s!%s.%s member offset", off.module, off.struc, off.member);
            continue;
        }
        os.offsets_[i] = *offset;
    }
    return !fail;
}
