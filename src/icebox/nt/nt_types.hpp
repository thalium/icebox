#pragma once

#include "types.hpp"

#include <string.h>

namespace nt
{
    struct Access_Mask
    {
        uint32_t   mask;
        const char mask_name[32];
    };

    const Access_Mask g_access_mask[] =
    {
            {0x00010000, "DELETE"},
            {0x00000001, "FILE_READ_DATA"},
            {0x00000080, "FILE_READ_ATTRIBUTES"},
            {0x00000008, "FILE_READ_EA"},
            {0x00020000, "READ_CONTROL"},
            {0x00000002, "FILE_WRITE_DATA"},
            {0x00000100, "FILE_WRITE_ATTRIBUTES"},
            {0x00000010, "FILE_WRITE_EA"},
            {0x00000004, "FILE_APPEND_DATA"},
            {0x00040000, "WRITE_DAC"},
            {0x00080000, "WRITE_OWNER"},
            {0x00100000, "SYNCHRONIZE"},
            {0x00000020, "FILE_EXECUTE"},
    };

    struct afd_status
    {
        uint32_t   status;
        const char status_name[32];
    };

    const afd_status g_afd_status[] =
    {
            {0x1201F, "AfdSend"},
            {0x12017, "AfdReceive"},
            {0x12024, "AfdPoll"},
            {0x12047, "AfdDispatchImmediateIrp"},
            {0x12003, "AfdBind"},
    };

    using ACCESS_MASK                          = uint32_t;
    using ALPC_HANDLE                          = uint64_t;
    using ALPC_MESSAGE_INFORMATION_CLASS       = uint64_t;
    using ALPC_PORT_INFORMATION_CLASS          = uint64_t;
    using APPHELPCOMMAND                       = uint64_t;
    using ATOM_INFORMATION_CLASS               = uint64_t;
    using AUDIT_EVENT_TYPE                     = uint32_t;
    using BOOLEAN                              = bool;
    using DEBUGOBJECTINFOCLASS                 = uint64_t;
    using DEVICE_POWER_STATE                   = uint32_t;
    using ENLISTMENT_INFORMATION_CLASS         = uint32_t;
    using EVENT_INFORMATION_CLASS              = uint64_t;
    using EVENT_TYPE                           = uint32_t;
    using EXECUTION_STATE                      = uint32_t;
    using FILE_INFORMATION_CLASS               = uint32_t;
    using FS_INFORMATION_CLASS                 = uint32_t;
    using HANDLE                               = uint64_t;
    using IO_COMPLETION_INFORMATION_CLASS      = uint64_t;
    using IO_SESSION_STATE                     = uint32_t;
    using JOBOBJECTINFOCLASS                   = uint32_t;
    using KAFFINITY                            = uint64_t;
    using KEY_INFORMATION_CLASS                = uint32_t;
    using KEY_SET_INFORMATION_CLASS            = uint32_t;
    using KEY_VALUE_INFORMATION_CLASS          = uint32_t;
    using KPROFILE_SOURCE                      = uint32_t;
    using KTMOBJECT_TYPE                       = uint32_t;
    using LANGID                               = uint16_t;
    using LCID                                 = uint32_t;
    using LONG                                 = uint32_t;
    using LPGUID                               = uint64_t;
    using MEMORY_INFORMATION_CLASS             = uint32_t;
    using MEMORY_RESERVE_TYPE                  = uint64_t;
    using MUTANT_INFORMATION_CLASS             = uint64_t;
    using NOTIFICATION_MASK                    = uint32_t;
    using NTSTATUS                             = uint32_t;
    using OBJECT_INFORMATION_CLASS             = uint32_t;
    using PACCESS_MASK                         = uint64_t;
    using PALPC_CONTEXT_ATTR                   = uint64_t;
    using PALPC_DATA_VIEW_ATTR                 = uint64_t;
    using PALPC_HANDLE                         = uint64_t;
    using PALPC_MESSAGE_ATTRIBUTES             = uint64_t;
    using PALPC_PORT_ATTRIBUTES                = uint64_t;
    using PALPC_SECURITY_ATTR                  = uint64_t;
    using PBOOLEAN                             = uint64_t;
    using PBOOT_ENTRY                          = uint64_t;
    using PBOOT_OPTIONS                        = uint64_t;
    using PCHAR                                = uint64_t;
    using PCLIENT_ID                           = uint64_t;
    using PCONTEXT                             = uint64_t;
    using PCRM_PROTOCOL_ID                     = uint64_t;
    using PDBGUI_WAIT_STATE_CHANGE             = uint64_t;
    using PEFI_DRIVER_ENTRY                    = uint64_t;
    using PEXCEPTION_RECORD                    = uint64_t;
    using PFILE_BASIC_INFORMATION              = uint64_t;
    using PFILE_IO_COMPLETION_INFORMATION      = uint64_t;
    using PFILE_NETWORK_OPEN_INFORMATION       = uint64_t;
    using PFILE_PATH                           = uint64_t;
    using PFILE_SEGMENT_ELEMENT                = uint64_t;
    using PGENERIC_MAPPING                     = uint64_t;
    using PGROUP_AFFINITY                      = uint64_t;
    using PHANDLE                              = uint64_t;
    using PINITIAL_TEB                         = uint64_t;
    using PIO_APC_ROUTINE                      = uint64_t;
    using PIO_STATUS_BLOCK                     = uint64_t;
    using PJOB_SET_ARRAY                       = uint64_t;
    using PKEY_VALUE_ENTRY                     = uint64_t;
    using PKTMOBJECT_CURSOR                    = uint64_t;
    using PLARGE_INTEGER                       = uint64_t;
    using PLCID                                = uint64_t;
    using PLONG                                = uint64_t;
    using PLUGPLAY_CONTROL_CLASS               = uint64_t;
    using PLUID                                = uint64_t;
    using PNTSTATUS                            = uint64_t;
    using POBJECT_ATTRIBUTES                   = uint64_t;
    using POBJECT_TYPE_LIST                    = uint64_t;
    using PORT_INFORMATION_CLASS               = uint64_t;
    using POWER_ACTION                         = uint32_t;
    using POWER_INFORMATION_LEVEL              = uint32_t;
    using PPLUGPLAY_EVENT_BLOCK                = uint64_t;
    using PPORT_MESSAGE                        = uint64_t;
    using PPORT_VIEW                           = uint64_t;
    using PPRIVILEGE_SET                       = uint64_t;
    using PPROCESS_ATTRIBUTE_LIST              = uint64_t;
    using PPROCESS_CREATE_INFO                 = uint64_t;
    using PPS_APC_ROUTINE                      = uint64_t;
    using PPS_ATTRIBUTE_LIST                   = uint64_t;
    using PREMOTE_PORT_VIEW                    = uint64_t;
    using PROCESSINFOCLASS                     = uint32_t;
    using PRTL_ATOM                            = uint64_t;
    using PRTL_USER_PROCESS_PARAMETERS         = uint64_t;
    using PSECURITY_DESCRIPTOR                 = uint64_t;
    using PSECURITY_QUALITY_OF_SERVICE         = uint64_t;
    using PSID                                 = uint64_t;
    using PSIZE_T                              = uint64_t;
    using PTIMER_APC_ROUTINE                   = uint64_t;
    using PTOKEN_DEFAULT_DACL                  = uint64_t;
    using PTOKEN_GROUPS                        = uint64_t;
    using PTOKEN_OWNER                         = uint64_t;
    using PTOKEN_PRIMARY_GROUP                 = uint64_t;
    using PTOKEN_PRIVILEGES                    = uint64_t;
    using PTOKEN_SOURCE                        = uint64_t;
    using PTOKEN_USER                          = uint64_t;
    using PTRANSACTION_NOTIFICATION            = uint64_t;
    using PULARGE_INTEGER                      = uint64_t;
    using PULONG                               = uint64_t;
    using PULONG_PTR                           = uint64_t;
    using PUNICODE_STRING                      = uint64_t;
    using PUSHORT                              = uint64_t;
    using PVOID                                = uint64_t;
    using PWSTR                                = uint64_t;
    using RESOURCEMANAGER_INFORMATION_CLASS    = uint32_t;
    using RTL_ATOM                             = uint64_t;
    using SECTION_INFORMATION_CLASS            = uint64_t;
    using SECTION_INHERIT                      = uint32_t;
    using SECURITY_INFORMATION                 = uint32_t;
    using SEMAPHORE_INFORMATION_CLASS          = uint64_t;
    using SHUTDOWN_ACTION                      = uint64_t;
    using SIZE_T                               = uint64_t;
    using SYSDBG_COMMAND                       = uint64_t;
    using SYSTEM_INFORMATION_CLASS             = uint32_t;
    using SYSTEM_POWER_STATE                   = uint32_t;
    using THREADINFOCLASS                      = uint32_t;
    using TIMER_INFORMATION_CLASS              = uint64_t;
    using TIMER_SET_INFORMATION_CLASS          = uint32_t;
    using TIMER_TYPE                           = uint32_t;
    using TOKEN_INFORMATION_CLASS              = uint32_t;
    using TOKEN_TYPE                           = uint32_t;
    using TRANSACTIONMANAGER_INFORMATION_CLASS = uint32_t;
    using TRANSACTION_INFORMATION_CLASS        = uint32_t;
    using ULONG                                = uint32_t;
    using ULONG_PTR                            = uint64_t;
    using USHORT                               = uint16_t;
    using VDMSERVICECLASS                      = uint64_t;
    using WAIT_TYPE                            = uint32_t;
    using WIN32_PROTECTION_MASK                = uint64_t;
    using WORKERFACTORYINFOCLASS               = uint64_t;

    // special return types
    using VOID = void;

    template <typename T>
    constexpr T cast_to(arg_t src)
    {
        T value = 0;
        static_assert(sizeof value <= sizeof src.val, "invalid size");
        memcpy(&value, &src.val, sizeof value);
        return value;
    };
} // namespace nt
