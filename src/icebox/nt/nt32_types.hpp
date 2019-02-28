#pragma once

#include "types.hpp"
#include <string>
#include <vector>

namespace nt32
{
    enum class access_mask_e : uint32_t
    {
        DELETE                = 0x00010000,
        FILE_READ_DATA        = 0x00000001,
        FILE_READ_ATTRIBUTES  = 0x00000080,
        FILE_READ_EA          = 0x00000008,
        READ_CONTROL          = 0x00020000,
        FILE_WRITE_DATA       = 0x00000002,
        FILE_WRITE_ATTRIBUTES = 0x00000100,
        FILE_WRITE_EA         = 0x00000010,
        FILE_APPEND_DATA      = 0x00000004,
        WRITE_DAC             = 0x00040000,
        WRITE_OWNER           = 0x00080000,
        SYNCHRONIZE           = 0x00100000,
        FILE_EXECUTE          = 0x00000020,
    };

    // use string and not string_view because result goes in json (nlohmann)
    std::string                 access_mask_str (access_mask_e access_mask);
    std::vector<std::string>    access_mask_dump(uint32_t mask);

    enum class afd_status_e : uint32_t
    {
        AfdSend                 = 0x1201F,
        AfdReceive              = 0x12017,
        AfdPoll                 = 0x12024,
        AfdDispatchImmediateIrp = 0x12047,
        AfdBind                 = 0x12003,
    };

    // use string and not string_view because result goes in json (nlohmann)
    std::string afd_status_str  (afd_status_e afd_status);
    std::string afd_status_dump (uint32_t afd_status);

    using ACCESS_MASK                          = uint32_t;
    using ALPC_HANDLE                          = uint32_t;
    using ALPC_MESSAGE_INFORMATION_CLASS       = uint32_t;
    using ALPC_PORT_INFORMATION_CLASS          = uint32_t;
    using APPHELPCOMMAND                       = uint32_t;
    using ATOM_INFORMATION_CLASS               = uint32_t;
    using AUDIT_EVENT_TYPE                     = uint32_t;
    using BOOLEAN                              = bool;
    using DEBUGOBJECTINFOCLASS                 = uint32_t;
    using DEVICE_POWER_STATE                   = uint32_t;
    using ENLISTMENT_INFORMATION_CLASS         = uint32_t;
    using EVENT_INFORMATION_CLASS              = uint32_t;
    using EVENT_TYPE                           = uint32_t;
    using EXECUTION_STATE                      = uint32_t;
    using FILE_INFORMATION_CLASS               = uint32_t;
    using FS_INFORMATION_CLASS                 = uint32_t;
    using HANDLE                               = uint32_t;
    using IO_COMPLETION_INFORMATION_CLASS      = uint32_t;
    using IO_SESSION_STATE                     = uint32_t;
    using JOBOBJECTINFOCLASS                   = uint32_t;
    using KAFFINITY                            = uint32_t;
    using KEY_INFORMATION_CLASS                = uint32_t;
    using KEY_SET_INFORMATION_CLASS            = uint32_t;
    using KEY_VALUE_INFORMATION_CLASS          = uint32_t;
    using KPROFILE_SOURCE                      = uint32_t;
    using KTMOBJECT_TYPE                       = uint32_t;
    using LANGID                               = uint16_t;
    using LCID                                 = uint32_t;
    using LONG                                 = uint32_t;
    using LPGUID                               = uint32_t;
    using MEMORY_INFORMATION_CLASS             = uint32_t;
    using MEMORY_RESERVE_TYPE                  = uint32_t;
    using MUTANT_INFORMATION_CLASS             = uint32_t;
    using NOTIFICATION_MASK                    = uint32_t;
    using NTSTATUS                             = uint32_t;
    using OBJECT_INFORMATION_CLASS             = uint32_t;
    using PACCESS_MASK                         = uint32_t;
    using PALPC_CONTEXT_ATTR                   = uint32_t;
    using PALPC_DATA_VIEW_ATTR                 = uint32_t;
    using PALPC_HANDLE                         = uint32_t;
    using PALPC_MESSAGE_ATTRIBUTES             = uint32_t;
    using PALPC_PORT_ATTRIBUTES                = uint32_t;
    using PALPC_SECURITY_ATTR                  = uint32_t;
    using PBOOLEAN                             = uint32_t;
    using PBOOT_ENTRY                          = uint32_t;
    using PBOOT_OPTIONS                        = uint32_t;
    using PCHAR                                = uint32_t;
    using PCLIENT_ID                           = uint32_t;
    using PCONTEXT                             = uint32_t;
    using PCRM_PROTOCOL_ID                     = uint32_t;
    using PDBGUI_WAIT_STATE_CHANGE             = uint32_t;
    using PEFI_DRIVER_ENTRY                    = uint32_t;
    using PEXCEPTION_RECORD                    = uint32_t;
    using PFILE_BASIC_INFORMATION              = uint32_t;
    using PFILE_IO_COMPLETION_INFORMATION      = uint32_t;
    using PFILE_NETWORK_OPEN_INFORMATION       = uint32_t;
    using PFILE_PATH                           = uint32_t;
    using PFILE_SEGMENT_ELEMENT                = uint32_t;
    using PGENERIC_MAPPING                     = uint32_t;
    using PGROUP_AFFINITY                      = uint32_t;
    using PHANDLE                              = uint32_t;
    using PINITIAL_TEB                         = uint32_t;
    using PIO_APC_ROUTINE                      = uint32_t;
    using PIO_STATUS_BLOCK                     = uint32_t;
    using PJOB_SET_ARRAY                       = uint32_t;
    using PKEY_VALUE_ENTRY                     = uint32_t;
    using PKTMOBJECT_CURSOR                    = uint32_t;
    using PLARGE_INTEGER                       = uint32_t;
    using PLCID                                = uint32_t;
    using PLONG                                = uint32_t;
    using PLUGPLAY_CONTROL_CLASS               = uint32_t;
    using PLUID                                = uint32_t;
    using PNTSTATUS                            = uint32_t;
    using POBJECT_ATTRIBUTES                   = uint32_t;
    using POBJECT_TYPE_LIST                    = uint32_t;
    using PORT_INFORMATION_CLASS               = uint32_t;
    using POWER_ACTION                         = uint32_t;
    using POWER_INFORMATION_LEVEL              = uint32_t;
    using PPLUGPLAY_EVENT_BLOCK                = uint32_t;
    using PPORT_MESSAGE                        = uint32_t;
    using PPORT_VIEW                           = uint32_t;
    using PPRIVILEGE_SET                       = uint32_t;
    using PPROCESS_ATTRIBUTE_LIST              = uint32_t;
    using PPROCESS_CREATE_INFO                 = uint32_t;
    using PPS_APC_ROUTINE                      = uint32_t;
    using PPS_ATTRIBUTE_LIST                   = uint32_t;
    using PREMOTE_PORT_VIEW                    = uint32_t;
    using PROCESSINFOCLASS                     = uint32_t;
    using PRTL_ATOM                            = uint32_t;
    using PRTL_USER_PROCESS_PARAMETERS         = uint32_t;
    using PSECURITY_DESCRIPTOR                 = uint32_t;
    using PSECURITY_QUALITY_OF_SERVICE         = uint32_t;
    using PSID                                 = uint32_t;
    using PSIZE_T                              = uint32_t;
    using PTIMER_APC_ROUTINE                   = uint32_t;
    using PTOKEN_DEFAULT_DACL                  = uint32_t;
    using PTOKEN_GROUPS                        = uint32_t;
    using PTOKEN_OWNER                         = uint32_t;
    using PTOKEN_PRIMARY_GROUP                 = uint32_t;
    using PTOKEN_PRIVILEGES                    = uint32_t;
    using PTOKEN_SOURCE                        = uint32_t;
    using PTOKEN_USER                          = uint32_t;
    using PTRANSACTION_NOTIFICATION            = uint32_t;
    using PULARGE_INTEGER                      = uint32_t;
    using PULONG                               = uint32_t;
    using PULONG_PTR                           = uint32_t;
    using PUNICODE_STRING                      = uint32_t;
    using PUSHORT                              = uint32_t;
    using PVOID                                = uint32_t;
    using PWSTR                                = uint32_t;
    using RESOURCEMANAGER_INFORMATION_CLASS    = uint32_t;
    using RTL_ATOM                             = uint32_t;
    using SECTION_INFORMATION_CLASS            = uint32_t;
    using SECTION_INHERIT                      = uint32_t;
    using SECURITY_INFORMATION                 = uint32_t;
    using SEMAPHORE_INFORMATION_CLASS          = uint32_t;
    using SHUTDOWN_ACTION                      = uint32_t;
    using SIZE_T                               = uint32_t;
    using SYSDBG_COMMAND                       = uint32_t;
    using SYSTEM_INFORMATION_CLASS             = uint32_t;
    using SYSTEM_POWER_STATE                   = uint32_t;
    using THREADINFOCLASS                      = uint32_t;
    using TIMER_INFORMATION_CLASS              = uint32_t;
    using TIMER_SET_INFORMATION_CLASS          = uint32_t;
    using TIMER_TYPE                           = uint32_t;
    using TOKEN_INFORMATION_CLASS              = uint32_t;
    using TOKEN_TYPE                           = uint32_t;
    using TRANSACTIONMANAGER_INFORMATION_CLASS = uint32_t;
    using TRANSACTION_INFORMATION_CLASS        = uint32_t;
    using ULONG                                = uint32_t;
    using ULONG_PTR                            = uint32_t;
    using USHORT                               = uint16_t;
    using VDMSERVICECLASS                      = uint32_t;
    using WAIT_TYPE                            = uint32_t;
    using WIN32_PROTECTION_MASK                = uint32_t;
    using WORKERFACTORYINFOCLASS               = uint32_t;

    // special return types
    using VOID = uint32_t;

    template <typename T>
    constexpr T cast_to(arg_t src)
    {
        T value = 0;
        static_assert(sizeof value <= sizeof src.val, "invalid size");
        memcpy(&value, &src.val, sizeof value);
        return value;
    };
} // namespace nt32
