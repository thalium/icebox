#ifndef nt_namespace
#    error do not include this header directly
#endif

#include "icebox/types.hpp"

namespace memory { struct Io; }

namespace nt_namespace
{
#ifdef WOW64
    using ptr_t = uint32_t;
#else
    using ptr_t = uint64_t;
#endif

    using ACCESS_MASK                          = nt_types::ACCESS_MASK;
    using ALPC_HANDLE                          = ptr_t;
    using ALPC_MESSAGE_INFORMATION_CLASS       = ptr_t;
    using ALPC_PORT_INFORMATION_CLASS          = ptr_t;
    using APPHELPCOMMAND                       = ptr_t;
    using ATOM_INFORMATION_CLASS               = ptr_t;
    using AUDIT_EVENT_TYPE                     = uint32_t;
    using BOOLEAN                              = bool;
    using BYTE                                 = uint8_t;
    using DEBUGOBJECTINFOCLASS                 = ptr_t;
    using DEVICE_POWER_STATE                   = uint32_t;
    using ENLISTMENT_INFORMATION_CLASS         = uint32_t;
    using EVENT_INFORMATION_CLASS              = ptr_t;
    using EVENT_TYPE                           = uint32_t;
    using EXECUTION_STATE                      = uint32_t;
    using FILE_INFORMATION_CLASS               = uint32_t;
    using FS_INFORMATION_CLASS                 = uint32_t;
    using HANDLE                               = ptr_t;
    using IOCTL_CODE                           = nt_types::IOCTL_CODE;
    using IO_COMPLETION_INFORMATION_CLASS      = ptr_t;
    using IO_SESSION_STATE                     = uint32_t;
    using JOBOBJECTINFOCLASS                   = uint32_t;
    using KAFFINITY                            = ptr_t;
    using KEY_INFORMATION_CLASS                = uint32_t;
    using KEY_SET_INFORMATION_CLASS            = uint32_t;
    using KEY_VALUE_INFORMATION_CLASS          = uint32_t;
    using KPROFILE_SOURCE                      = uint32_t;
    using KTMOBJECT_TYPE                       = uint32_t;
    using LANGID                               = uint16_t;
    using LCID                                 = uint32_t;
    using LONG                                 = uint32_t;
    using LPGUID                               = ptr_t;
    using MEMORY_INFORMATION_CLASS             = uint32_t;
    using MEMORY_RESERVE_TYPE                  = ptr_t;
    using MUTANT_INFORMATION_CLASS             = ptr_t;
    using NOTIFICATION_MASK                    = uint32_t;
    using NTSTATUS                             = uint32_t;
    using OBJECT_INFORMATION_CLASS             = uint32_t;
    using PACCESS_MASK                         = ptr_t;
    using PAGE_ACCESS                          = nt_types::PAGE_ACCESS;
    using PALPC_CONTEXT_ATTR                   = ptr_t;
    using PALPC_DATA_VIEW_ATTR                 = ptr_t;
    using PALPC_HANDLE                         = ptr_t;
    using PALPC_MESSAGE_ATTRIBUTES             = ptr_t;
    using PALPC_PORT_ATTRIBUTES                = ptr_t;
    using PALPC_SECURITY_ATTR                  = ptr_t;
    using PBOOLEAN                             = ptr_t;
    using PBOOT_ENTRY                          = ptr_t;
    using PBOOT_OPTIONS                        = ptr_t;
    using PCHAR                                = ptr_t;
    using PCLIENT_ID                           = ptr_t;
    using PCONTEXT                             = ptr_t;
    using PCRM_PROTOCOL_ID                     = ptr_t;
    using PDBGUI_WAIT_STATE_CHANGE             = ptr_t;
    using PEFI_DRIVER_ENTRY                    = ptr_t;
    using PEXCEPTION_RECORD                    = ptr_t;
    using PFILE_BASIC_INFORMATION              = ptr_t;
    using PFILE_IO_COMPLETION_INFORMATION      = ptr_t;
    using PFILE_NETWORK_OPEN_INFORMATION       = ptr_t;
    using PFILE_PATH                           = ptr_t;
    using PFILE_SEGMENT_ELEMENT                = ptr_t;
    using PGENERIC_MAPPING                     = ptr_t;
    using PGROUP_AFFINITY                      = ptr_t;
    using PHANDLE                              = ptr_t;
    using PINITIAL_TEB                         = ptr_t;
    using PIO_APC_ROUTINE                      = ptr_t;
    using PIO_STATUS_BLOCK                     = ptr_t;
    using PJOB_SET_ARRAY                       = ptr_t;
    using PKEY_VALUE_ENTRY                     = ptr_t;
    using PKTMOBJECT_CURSOR                    = ptr_t;
    using PLARGE_INTEGER                       = ptr_t;
    using PLCID                                = ptr_t;
    using PLONG                                = ptr_t;
    using PLUGPLAY_CONTROL_CLASS               = ptr_t;
    using PLUID                                = ptr_t;
    using PNTSTATUS                            = ptr_t;
    using POBJECT_ATTRIBUTES                   = ptr_t;
    using POBJECT_TYPE_LIST                    = ptr_t;
    using PORT_INFORMATION_CLASS               = ptr_t;
    using POWER_ACTION                         = uint32_t;
    using POWER_INFORMATION_LEVEL              = uint32_t;
    using PPLUGPLAY_EVENT_BLOCK                = ptr_t;
    using PPORT_MESSAGE                        = ptr_t;
    using PPORT_VIEW                           = ptr_t;
    using PPRIVILEGE_SET                       = ptr_t;
    using PPROCESS_ATTRIBUTE_LIST              = ptr_t;
    using PPROCESS_CREATE_INFO                 = ptr_t;
    using PPS_APC_ROUTINE                      = ptr_t;
    using PPS_ATTRIBUTE_LIST                   = ptr_t;
    using PREMOTE_PORT_VIEW                    = ptr_t;
    using PROCESSINFOCLASS                     = uint32_t;
    using PRTL_ATOM                            = ptr_t;
    using PRTL_USER_PROCESS_PARAMETERS         = ptr_t;
    using PSECURITY_DESCRIPTOR                 = ptr_t;
    using PSECURITY_QUALITY_OF_SERVICE         = ptr_t;
    using PSID                                 = ptr_t;
    using PSIZE_T                              = ptr_t;
    using PTIMER_APC_ROUTINE                   = ptr_t;
    using PTOKEN_DEFAULT_DACL                  = ptr_t;
    using PTOKEN_GROUPS                        = ptr_t;
    using PTOKEN_OWNER                         = ptr_t;
    using PTOKEN_PRIMARY_GROUP                 = ptr_t;
    using PTOKEN_PRIVILEGES                    = ptr_t;
    using PTOKEN_SOURCE                        = ptr_t;
    using PTOKEN_USER                          = ptr_t;
    using PTRANSACTION_NOTIFICATION            = ptr_t;
    using PULARGE_INTEGER                      = ptr_t;
    using PULONG                               = ptr_t;
    using PULONG_PTR                           = ptr_t;
    using PUNICODE_STRING                      = ptr_t;
    using PUSHORT                              = ptr_t;
    using PVOID                                = ptr_t;
    using PWSTR                                = ptr_t;
    using RESOURCEMANAGER_INFORMATION_CLASS    = uint32_t;
    using RTL_ATOM                             = ptr_t;
    using SECTION_INFORMATION_CLASS            = ptr_t;
    using SECTION_INHERIT                      = uint32_t;
    using SECURITY_INFORMATION                 = uint32_t;
    using SEMAPHORE_INFORMATION_CLASS          = ptr_t;
    using SHUTDOWN_ACTION                      = ptr_t;
    using SIZE_T                               = ptr_t;
    using SYSDBG_COMMAND                       = ptr_t;
    using SYSTEM_INFORMATION_CLASS             = uint32_t;
    using SYSTEM_POWER_STATE                   = uint32_t;
    using THREADINFOCLASS                      = uint32_t;
    using TIMER_INFORMATION_CLASS              = ptr_t;
    using TIMER_SET_INFORMATION_CLASS          = uint32_t;
    using TIMER_TYPE                           = uint32_t;
    using TOKEN_INFORMATION_CLASS              = uint32_t;
    using TOKEN_TYPE                           = uint32_t;
    using TRANSACTIONMANAGER_INFORMATION_CLASS = uint32_t;
    using TRANSACTION_INFORMATION_CLASS        = uint32_t;
    using ULONG                                = uint32_t;
    using ULONG_PTR                            = ptr_t;
    using USHORT                               = uint16_t;
    using VDMSERVICECLASS                      = ptr_t;
    using WAIT_TYPE                            = uint32_t;
    using WIN32_PROTECTION_MASK                = uint32_t;
    using WORKERFACTORYINFOCLASS               = ptr_t;

    // special return types
    using VOID = ptr_t;

    const auto ioctl_code_dump = nt_types::ioctl_code_dump;
    const auto access_mask_all = nt_types::access_mask_all;
    const auto page_access_all = nt_types::page_access_all;

    union _LARGE_INTEGER
    {
        struct
        {
            uint32_t LowPart;
            uint32_t HighPart;
        } u;
        uint64_t QuadPart;
    };

    struct _UNICODE_STRING
    {
        uint16_t Length;
        uint16_t MaximumLength;
        ptr_t    Buffer;
    };

    struct _LIST_ENTRY
    {
        ptr_t flink;
        ptr_t blink;
    };

    struct _PEB_LDR_DATA
    {
        uint32_t    Length;
        uint8_t     Initialized;
        uint32_t    SsHandle;
        _LIST_ENTRY InLoadOrderModuleList;
        _LIST_ENTRY InMemoryOrderModuleList;
    };

    struct _LDR_DATA_TABLE_ENTRY
    {
        _LIST_ENTRY     InLoadOrderLinks;
        _LIST_ENTRY     InMemoryOrderLinks;
        _LIST_ENTRY     InInitializationOrderLinks;
        ptr_t           DllBase;
        ptr_t           EntryPoint;
        uint32_t        SizeOfImage;
        _UNICODE_STRING FullDllName;
        _UNICODE_STRING BaseDllName;
        ptr_t           _[3];
        uint32_t        TimeDateStamp;
    };

    struct _RTL_USER_PROCESS_PARAMETERS
    {
        BYTE            Reserved1[16];
        PVOID           Reserved2[10];
        _UNICODE_STRING ImagePathName;
        _UNICODE_STRING CommandLine;
    };

    struct _OBJECT_ATTRIBUTES
    {
        uint32_t Length;
        ptr_t    RootDirectory;
        ptr_t    ObjectName;
        uint32_t Attributes;
        ptr_t    SecurityDescriptor;
        ptr_t    SecurityQualityOfService;
    };

    opt<std::string> read_unicode_string(const memory::Io& io, uint64_t addr);

    struct _OBJECT_DIRECTORY_ENTRY
    {
        ptr_t    ChainLink;
        ptr_t    Object;
        uint32_t HashValue;
    };

    struct _OBJECT_DIRECTORY
    {
        ptr_t HashBuckets[37];
    };

    struct _OBJECT_HEADER_NAME_INFO
    {
        ptr_t           Directory;
        _UNICODE_STRING Name;
        uint32_t        QueryReferences;
    };

    struct _RTL_BALANCED_NODE
    {
        ptr_t Left;
        ptr_t Right;
        ptr_t ParentValue;
    };

    struct _SEGMENT
    {
        ptr_t    ControlArea;
        uint32_t TotalNumberOfPtes;
        uint32_t SegmentFlags;
        uint64_t NumberOfCommittedPages;
        uint64_t SizeOfSegment;
        union
        {
            ptr_t ExtendInfo;
            ptr_t BasedAddress;
        } u;
        // truncated...
    };

    struct _CONTROL_AREA
    {
        ptr_t Segment;
        // truncated...
    };

    struct _SECTION
    {
        _RTL_BALANCED_NODE SectionNode;
        uint64_t           StartingVpn;
        uint64_t           EndingVpn;
        union
        {
            ptr_t ControlArea;
            ptr_t FileObject;
        } u;
        // truncated...
    };

    namespace win10
    {
        struct _MMVAD_SHORT
        {
            _RTL_BALANCED_NODE VadNode;
            uint32_t           StartingVpn;
            uint32_t           EndingVpn;
            uint8_t            StartingVpnHigh;
            uint8_t            EndingVpnHigh;
            uint8_t            CommitChargeHigh;
            uint8_t            SpareNT64VadUChar;
            uint32_t           ReferenceCount;
            ptr_t              PushLock;
            uint32_t           VadFlags; // _MMVAD_FLAGS
            uint32_t           _;
            // truncated
        };
    } // namespace win10

    namespace win7
    {
        struct _MMVAD_SHORT
        {
            ptr_t Parent;
            ptr_t LeftChild;
            ptr_t RightChild;
            ptr_t StartingVpn;
            ptr_t EndingVpn;
        };
    } // namespace win7

} // namespace nt_namespace
#undef nt_namespace
#undef WOW64
