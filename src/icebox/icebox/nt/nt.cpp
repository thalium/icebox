#include "nt_types.hpp"

#include "reader.hpp"
#include "utils/hex.hpp"
#include "utils/utf8.hpp"

#include "nt.hpp"
#include "wow64.hpp"

namespace
{
    template <typename T>
    opt<std::string> read_unicode_string(const reader::Reader& reader, uint64_t addr)
    {
        T str;
        auto ok = reader.read_all(&str, addr, sizeof str);
        if(!ok)
            return {};

        str.Length = std::min(str.Length, str.MaximumLength) & ~1;
        if(!str.Length)
            return {};

        auto buffer = std::vector<uint8_t>(str.Length);
        ok          = reader.read_all(&buffer[0], str.Buffer, str.Length);
        if(!ok)
            return {};

        const auto p = &buffer[0];
        return utf8::from_utf16(p, &p[str.Length]);
    }
}

opt<std::string> wow64::read_unicode_string(const reader::Reader& reader, uint64_t addr)
{
    return ::read_unicode_string<wow64::_UNICODE_STRING>(reader, addr);
}

opt<std::string> nt::read_unicode_string(const reader::Reader& reader, uint64_t addr)
{
    return ::read_unicode_string<nt::_UNICODE_STRING>(reader, addr);
}

STATIC_ASSERT_EQ(sizeof(wow64::_UNICODE_STRING), 8);
STATIC_ASSERT_EQ(sizeof(nt::_UNICODE_STRING), 16);

STATIC_ASSERT_EQ(sizeof(wow64::_OBJECT_ATTRIBUTES), 24);
STATIC_ASSERT_EQ(sizeof(nt::_OBJECT_ATTRIBUTES), 48);

const char* nt_types::access_mask_str(nt::ACCESS_MASK arg)
{
    switch(arg)
    {
        case ACCESS_MASK::DELETE:                   return "DELETE";
        case ACCESS_MASK::FILE_READ_DATA:           return "FILE_READ_DATA";
        case ACCESS_MASK::FILE_READ_ATTRIBUTES:     return "FILE_READ_ATTRIBUTES";
        case ACCESS_MASK::FILE_READ_EA:             return "FILE_READ_EA";
        case ACCESS_MASK::READ_CONTROL:             return "READ_CONTROL";
        case ACCESS_MASK::FILE_WRITE_DATA:          return "FILE_WRITE_DATA";
        case ACCESS_MASK::FILE_WRITE_ATTRIBUTES:    return "FILE_WRITE_ATTRIBUTES";
        case ACCESS_MASK::FILE_WRITE_EA:            return "FILE_WRITE_EA";
        case ACCESS_MASK::FILE_APPEND_DATA:         return "FILE_APPEND_DATA";
        case ACCESS_MASK::WRITE_DAC:                return "WRITE_DAC";
        case ACCESS_MASK::WRITE_OWNER:              return "WRITE_OWNER";
        case ACCESS_MASK::SYNCHRONIZE:              return "SYNCHRONIZE";
        case ACCESS_MASK::FILE_EXECUTE:             return "FILE_EXECUTE";
    }
    return "";
}

namespace
{
    constexpr nt_types::ACCESS_MASK all_ACCESS_MASK[] =
    {
            nt::ACCESS_MASK::DELETE,
            nt::ACCESS_MASK::FILE_READ_DATA,
            nt::ACCESS_MASK::FILE_READ_ATTRIBUTES,
            nt::ACCESS_MASK::FILE_READ_EA,
            nt::ACCESS_MASK::READ_CONTROL,
            nt::ACCESS_MASK::FILE_WRITE_DATA,
            nt::ACCESS_MASK::FILE_WRITE_ATTRIBUTES,
            nt::ACCESS_MASK::FILE_WRITE_EA,
            nt::ACCESS_MASK::FILE_APPEND_DATA,
            nt::ACCESS_MASK::WRITE_DAC,
            nt::ACCESS_MASK::WRITE_OWNER,
            nt::ACCESS_MASK::SYNCHRONIZE,
            nt::ACCESS_MASK::FILE_EXECUTE,
    };

    template <typename T, size_t N>
    std::vector<const char*> dump_all(const T (&masks)[N], uint32_t arg, const char*(to_str)(T))
    {
        std::vector<const char*> reply;
        for(auto mask : masks)
            if(arg & static_cast<uint32_t>(mask))
                reply.push_back(to_str(mask));
        return reply;
    }
}

std::vector<const char*> nt_types::access_mask_all(uint32_t arg)
{
    return dump_all(all_ACCESS_MASK, arg, &access_mask_str);
}

const char* nt_types::ioctl_code_str(nt_types::IOCTL_CODE arg)
{
    switch(arg)
    {
        case IOCTL_CODE::AFD_BIND:                          return "AFD_BIND";
        case IOCTL_CODE::AFD_CONNECT:                       return "AFD_CONNECT";
        case IOCTL_CODE::AFD_START_LISTEN:                  return "AFD_START_LISTEN";
        case IOCTL_CODE::AFD_WAIT_FOR_LISTEN:               return "AFD_WAIT_FOR_LISTEN";
        case IOCTL_CODE::AFD_ACCEPT:                        return "AFD_ACCEPT";
        case IOCTL_CODE::AFD_RECV:                          return "AFD_RECV";
        case IOCTL_CODE::AFD_RECV_DATAGRAM:                 return "AFD_RECV_DATAGRAM";
        case IOCTL_CODE::AFD_SEND:                          return "AFD_SEND";
        case IOCTL_CODE::AFD_SEND_DATAGRAM:                 return "AFD_SEND_DATAGRAM";
        case IOCTL_CODE::AFD_SELECT:                        return "AFD_SELECT";
        case IOCTL_CODE::AFD_DISCONNECT:                    return "AFD_DISCONNECT";
        case IOCTL_CODE::AFD_GET_SOCK_NAME:                 return "AFD_GET_SOCK_NAME";
        case IOCTL_CODE::AFD_GET_PEER_NAME:                 return "AFD_GET_PEER_NAME";
        case IOCTL_CODE::AFD_GET_TDI_HANDLES:               return "AFD_GET_TDI_HANDLES";
        case IOCTL_CODE::AFD_SET_INFO:                      return "AFD_SET_INFO";
        case IOCTL_CODE::AFD_GET_CONTEXT_SIZE:              return "AFD_GET_CONTEXT_SIZE";
        case IOCTL_CODE::AFD_GET_CONTEXT:                   return "AFD_GET_CONTEXT";
        case IOCTL_CODE::AFD_SET_CONTEXT:                   return "AFD_SET_CONTEXT";
        case IOCTL_CODE::AFD_SET_CONNECT_DATA:              return "AFD_SET_CONNECT_DATA";
        case IOCTL_CODE::AFD_SET_CONNECT_OPTIONS:           return "AFD_SET_CONNECT_OPTIONS";
        case IOCTL_CODE::AFD_SET_DISCONNECT_DATA:           return "AFD_SET_DISCONNECT_DATA";
        case IOCTL_CODE::AFD_SET_DISCONNECT_OPTIONS:        return "AFD_SET_DISCONNECT_OPTIONS";
        case IOCTL_CODE::AFD_GET_CONNECT_DATA:              return "AFD_GET_CONNECT_DATA";
        case IOCTL_CODE::AFD_GET_CONNECT_OPTIONS:           return "AFD_GET_CONNECT_OPTIONS";
        case IOCTL_CODE::AFD_GET_DISCONNECT_DATA:           return "AFD_GET_DISCONNECT_DATA";
        case IOCTL_CODE::AFD_GET_DISCONNECT_OPTIONS:        return "AFD_GET_DISCONNECT_OPTIONS";
        case IOCTL_CODE::AFD_SET_CONNECT_DATA_SIZE:         return "AFD_SET_CONNECT_DATA_SIZE";
        case IOCTL_CODE::AFD_SET_CONNECT_OPTIONS_SIZE:      return "AFD_SET_CONNECT_OPTIONS_SIZE";
        case IOCTL_CODE::AFD_SET_DISCONNECT_DATA_SIZE:      return "AFD_SET_DISCONNECT_DATA_SIZE";
        case IOCTL_CODE::AFD_SET_DISCONNECT_OPTIONS_SIZE:   return "AFD_SET_DISCONNECT_OPTIONS_SIZE";
        case IOCTL_CODE::AFD_GET_INFO:                      return "AFD_GET_INFO";
        case IOCTL_CODE::AFD_EVENT_SELECT:                  return "AFD_EVENT_SELECT";
        case IOCTL_CODE::AFD_ENUM_NETWORK_EVENTS:           return "AFD_ENUM_NETWORK_EVENTS";
        case IOCTL_CODE::AFD_DEFER_ACCEPT:                  return "AFD_DEFER_ACCEPT";
        case IOCTL_CODE::AFD_GET_PENDING_CONNECT_DATA:      return "AFD_GET_PENDING_CONNECT_DATA";
        case IOCTL_CODE::AFD_VALIDATE_GROUP:                return "AFD_VALIDATE_GROUP";
    }
    return "";
}

std::string nt_types::ioctl_code_dump(nt_types::IOCTL_CODE arg)
{
    const auto value = ioctl_code_str(arg);
    if(*value)
        return value;

    char buf[2 + sizeof arg * 2 + 1];
    const auto ptr = hex::convert<hex::RemovePadding | hex::HexaPrefix>(buf, static_cast<uint32_t>(arg));
    return ptr;
}

const char* nt_types::page_access_str(PAGE_ACCESS arg)
{
    switch(arg)
    {
        case nt_types::PAGE_ACCESS::PAGE_NOACCESS:              return "PAGE_NOACCESS";
        case nt_types::PAGE_ACCESS::PAGE_READONLY:              return "PAGE_READONLY";
        case nt_types::PAGE_ACCESS::PAGE_READWRITE:             return "PAGE_READWRITE";
        case nt_types::PAGE_ACCESS::PAGE_WRITECOPY:             return "PAGE_WRITECOPY";
        case nt_types::PAGE_ACCESS::PAGE_EXECUTE:               return "PAGE_EXECUTE";
        case nt_types::PAGE_ACCESS::PAGE_EXECUTE_READ:          return "PAGE_EXECUTE_READ";
        case nt_types::PAGE_ACCESS::PAGE_EXECUTE_READWRITE:     return "PAGE_EXECUTE_READWRITE";
        case nt_types::PAGE_ACCESS::PAGE_EXECUTE_WRITECOPY:     return "PAGE_EXECUTE_WRITECOPY";
        case nt_types::PAGE_ACCESS::PAGE_GUARD:                 return "PAGE_GUARD";
        case nt_types::PAGE_ACCESS::PAGE_NOCACHE:               return "PAGE_NOCACHE";
        case nt_types::PAGE_ACCESS::PAGE_WRITECOMBINE:          return "PAGE_WRITECOMBINE";
        case nt_types::PAGE_ACCESS::PAGE_REVERT_TO_FILE_MAP:    return "PAGE_REVERT_TO_FILE_MAP";
        case nt_types::PAGE_ACCESS::PAGE_TARGETS_INVALID:       return "PAGE_TARGETS_INVALID";
        case nt_types::PAGE_ACCESS::PAGE_ENCLAVE_UNVALIDATED:   return "PAGE_ENCLAVE_UNVALIDATED";
        case nt_types::PAGE_ACCESS::PAGE_ENCLAVE_DECOMMIT:      return "PAGE_ENCLAVE_DECOMMIT";
    }
    return "";
}

namespace
{
    constexpr nt_types::PAGE_ACCESS all_PAGE_ACCESS[] =
    {
            nt_types::PAGE_ACCESS::PAGE_NOACCESS,
            nt_types::PAGE_ACCESS::PAGE_READONLY,
            nt_types::PAGE_ACCESS::PAGE_READWRITE,
            nt_types::PAGE_ACCESS::PAGE_WRITECOPY,
            nt_types::PAGE_ACCESS::PAGE_EXECUTE,
            nt_types::PAGE_ACCESS::PAGE_EXECUTE_READ,
            nt_types::PAGE_ACCESS::PAGE_EXECUTE_READWRITE,
            nt_types::PAGE_ACCESS::PAGE_EXECUTE_WRITECOPY,
            nt_types::PAGE_ACCESS::PAGE_GUARD,
            nt_types::PAGE_ACCESS::PAGE_NOCACHE,
            nt_types::PAGE_ACCESS::PAGE_WRITECOMBINE,
            nt_types::PAGE_ACCESS::PAGE_REVERT_TO_FILE_MAP,
            nt_types::PAGE_ACCESS::PAGE_TARGETS_INVALID,
            nt_types::PAGE_ACCESS::PAGE_ENCLAVE_UNVALIDATED,
            nt_types::PAGE_ACCESS::PAGE_ENCLAVE_DECOMMIT,
    };
}

std::vector<const char*> nt_types::page_access_all(uint32_t arg)
{
    return dump_all(all_PAGE_ACCESS, arg, &page_access_str);
}

STATIC_ASSERT_EQ(sizeof(nt::_MMVAD_SHORT), 56);
STATIC_ASSERT_EQ(offsetof(nt::_MMVAD_SHORT, VadFlags), 48);

#ifdef _MSC_VER

namespace
{
    constexpr nt_types::IOCTL_METHOD all_IOCTL_METHOD[] =
    {
            nt_types::IOCTL_METHOD::METHOD_BUFFERED,
            nt_types::IOCTL_METHOD::METHOD_IN_DIRECT,
            nt_types::IOCTL_METHOD::METHOD_OUT_DIRECT,
            nt_types::IOCTL_METHOD::METHOD_NEITHER,
    };

    constexpr nt_types::DEVICE_TYPE all_DEVICE_TYPE[] =
    {
            nt_types::DEVICE_TYPE::FILE_DEVICE_NETWORK,
    };
}

#    include <windows.h>
#    include <winternl.h>

STATIC_ASSERT_EQ(sizeof(nt::_UNICODE_STRING), sizeof(_UNICODE_STRING));
STATIC_ASSERT_EQ(sizeof(nt::_LIST_ENTRY), sizeof(_LIST_ENTRY));
STATIC_ASSERT_EQ(sizeof(nt::_PEB_LDR_DATA), sizeof(_PEB_LDR_DATA));
STATIC_ASSERT_EQ(sizeof(nt::_LDR_DATA_TABLE_ENTRY), sizeof(_LDR_DATA_TABLE_ENTRY));
STATIC_ASSERT_EQ(sizeof(nt::_OBJECT_ATTRIBUTES), sizeof(_OBJECT_ATTRIBUTES));

STATIC_ASSERT_EQ(static_cast<uint32_t>(all_ACCESS_MASK[0]), DELETE);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_ACCESS_MASK[1]), FILE_READ_DATA);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_ACCESS_MASK[2]), FILE_READ_ATTRIBUTES);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_ACCESS_MASK[3]), FILE_READ_EA);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_ACCESS_MASK[4]), READ_CONTROL);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_ACCESS_MASK[5]), FILE_WRITE_DATA);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_ACCESS_MASK[6]), FILE_WRITE_ATTRIBUTES);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_ACCESS_MASK[7]), FILE_WRITE_EA);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_ACCESS_MASK[8]), FILE_APPEND_DATA);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_ACCESS_MASK[9]), WRITE_DAC);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_ACCESS_MASK[10]), WRITE_OWNER);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_ACCESS_MASK[11]), SYNCHRONIZE);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_ACCESS_MASK[12]), FILE_EXECUTE);

STATIC_ASSERT_EQ(static_cast<uint32_t>(all_IOCTL_METHOD[0]), METHOD_BUFFERED);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_IOCTL_METHOD[1]), METHOD_IN_DIRECT);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_IOCTL_METHOD[2]), METHOD_OUT_DIRECT);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_IOCTL_METHOD[3]), METHOD_NEITHER);

STATIC_ASSERT_EQ(static_cast<uint32_t>(all_DEVICE_TYPE[0]), FILE_DEVICE_NETWORK);

STATIC_ASSERT_EQ(static_cast<uint32_t>(all_PAGE_ACCESS[0]), PAGE_NOACCESS);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_PAGE_ACCESS[1]), PAGE_READONLY);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_PAGE_ACCESS[2]), PAGE_READWRITE);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_PAGE_ACCESS[3]), PAGE_WRITECOPY);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_PAGE_ACCESS[4]), PAGE_EXECUTE);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_PAGE_ACCESS[5]), PAGE_EXECUTE_READ);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_PAGE_ACCESS[6]), PAGE_EXECUTE_READWRITE);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_PAGE_ACCESS[7]), PAGE_EXECUTE_WRITECOPY);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_PAGE_ACCESS[8]), PAGE_GUARD);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_PAGE_ACCESS[9]), PAGE_NOCACHE);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_PAGE_ACCESS[10]), PAGE_WRITECOMBINE);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_PAGE_ACCESS[11]), PAGE_REVERT_TO_FILE_MAP);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_PAGE_ACCESS[12]), PAGE_TARGETS_INVALID);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_PAGE_ACCESS[13]), PAGE_ENCLAVE_UNVALIDATED);
STATIC_ASSERT_EQ(static_cast<uint32_t>(all_PAGE_ACCESS[14]), PAGE_ENCLAVE_DECOMMIT);

// STATIC_ASSERT_EQ(sizeof(nt::ALPC_HANDLE), sizeof(ALPC_HANDLE));
// STATIC_ASSERT_EQ(sizeof(nt::ALPC_MESSAGE_INFORMATION_CLASS), sizeof(ALPC_MESSAGE_INFORMATION_CLASS));
// STATIC_ASSERT_EQ(sizeof(nt::ALPC_PORT_INFORMATION_CLASS), sizeof(ALPC_PORT_INFORMATION_CLASS));
// STATIC_ASSERT_EQ(sizeof(nt::APPHELPCOMMAND), sizeof(APPHELPCOMMAND));
// STATIC_ASSERT_EQ(sizeof(nt::ATOM_INFORMATION_CLASS), sizeof(ATOM_INFORMATION_CLASS));
STATIC_ASSERT_EQ(sizeof(nt::AUDIT_EVENT_TYPE), sizeof(AUDIT_EVENT_TYPE));
STATIC_ASSERT_EQ(sizeof(nt::BOOLEAN), sizeof(BOOLEAN));
// STATIC_ASSERT_EQ(sizeof(nt::DEBUGOBJECTINFOCLASS), sizeof(DEBUGOBJECTINFOCLASS));
STATIC_ASSERT_EQ(sizeof(nt::DEVICE_POWER_STATE), sizeof(DEVICE_POWER_STATE));
STATIC_ASSERT_EQ(sizeof(nt::ENLISTMENT_INFORMATION_CLASS), sizeof(ENLISTMENT_INFORMATION_CLASS));
// STATIC_ASSERT_EQ(sizeof(nt::EVENT_INFORMATION_CLASS), sizeof(EVENT_INFORMATION_CLASS));
// STATIC_ASSERT_EQ(sizeof(nt::EVENT_TYPE), sizeof(EVENT_TYPE));
STATIC_ASSERT_EQ(sizeof(nt::EXECUTION_STATE), sizeof(EXECUTION_STATE));
STATIC_ASSERT_EQ(sizeof(nt::FILE_INFORMATION_CLASS), sizeof(FILE_INFORMATION_CLASS));
// STATIC_ASSERT_EQ(sizeof(nt::FS_INFORMATION_CLASS), sizeof(FS_INFORMATION_CLASS));
STATIC_ASSERT_EQ(sizeof(nt::HANDLE), sizeof(HANDLE));
// STATIC_ASSERT_EQ(sizeof(nt::IO_COMPLETION_INFORMATION_CLASS), sizeof(IO_COMPLETION_INFORMATION_CLASS));
// STATIC_ASSERT_EQ(sizeof(nt::IO_SESSION_STATE), sizeof(IO_SESSION_STATE));
STATIC_ASSERT_EQ(sizeof(nt::JOBOBJECTINFOCLASS), sizeof(JOBOBJECTINFOCLASS));
STATIC_ASSERT_EQ(sizeof(nt::KAFFINITY), sizeof(KAFFINITY));
// STATIC_ASSERT_EQ(sizeof(nt::KEY_INFORMATION_CLASS), sizeof(KEY_INFORMATION_CLASS));
STATIC_ASSERT_EQ(sizeof(nt::KEY_SET_INFORMATION_CLASS), sizeof(KEY_SET_INFORMATION_CLASS));
// STATIC_ASSERT_EQ(sizeof(nt::KEY_VALUE_INFORMATION_CLASS), sizeof(KEY_VALUE_INFORMATION_CLASS));
// STATIC_ASSERT_EQ(sizeof(nt::KPROFILE_SOURCE), sizeof(KPROFILE_SOURCE));
STATIC_ASSERT_EQ(sizeof(nt::KTMOBJECT_TYPE), sizeof(KTMOBJECT_TYPE));
STATIC_ASSERT_EQ(sizeof(nt::LANGID), sizeof(LANGID));
STATIC_ASSERT_EQ(sizeof(nt::LCID), sizeof(LCID));
STATIC_ASSERT_EQ(sizeof(nt::LONG), sizeof(LONG));
STATIC_ASSERT_EQ(sizeof(nt::LPGUID), sizeof(LPGUID));
// STATIC_ASSERT_EQ(sizeof(nt::MEMORY_INFORMATION_CLASS), sizeof(MEMORY_INFORMATION_CLASS));
// STATIC_ASSERT_EQ(sizeof(nt::MEMORY_RESERVE_TYPE), sizeof(MEMORY_RESERVE_TYPE));
// STATIC_ASSERT_EQ(sizeof(nt::MUTANT_INFORMATION_CLASS), sizeof(MUTANT_INFORMATION_CLASS));
STATIC_ASSERT_EQ(sizeof(nt::NOTIFICATION_MASK), sizeof(NOTIFICATION_MASK));
STATIC_ASSERT_EQ(sizeof(nt::NTSTATUS), sizeof(NTSTATUS));
STATIC_ASSERT_EQ(sizeof(nt::OBJECT_INFORMATION_CLASS), sizeof(OBJECT_INFORMATION_CLASS));
STATIC_ASSERT_EQ(sizeof(nt::PACCESS_MASK), sizeof(PACCESS_MASK));
// STATIC_ASSERT_EQ(sizeof(nt::PALPC_CONTEXT_ATTR), sizeof(PALPC_CONTEXT_ATTR));
// STATIC_ASSERT_EQ(sizeof(nt::PALPC_DATA_VIEW_ATTR), sizeof(PALPC_DATA_VIEW_ATTR));
// STATIC_ASSERT_EQ(sizeof(nt::PALPC_HANDLE), sizeof(PALPC_HANDLE));
// STATIC_ASSERT_EQ(sizeof(nt::PALPC_MESSAGE_ATTRIBUTES), sizeof(PALPC_MESSAGE_ATTRIBUTES));
// STATIC_ASSERT_EQ(sizeof(nt::PALPC_PORT_ATTRIBUTES), sizeof(PALPC_PORT_ATTRIBUTES));
// STATIC_ASSERT_EQ(sizeof(nt::PALPC_SECURITY_ATTR), sizeof(PALPC_SECURITY_ATTR));
STATIC_ASSERT_EQ(sizeof(nt::PBOOLEAN), sizeof(PBOOLEAN));
// STATIC_ASSERT_EQ(sizeof(nt::PBOOT_ENTRY), sizeof(PBOOT_ENTRY));
// STATIC_ASSERT_EQ(sizeof(nt::PBOOT_OPTIONS), sizeof(PBOOT_OPTIONS));
STATIC_ASSERT_EQ(sizeof(nt::PCHAR), sizeof(PCHAR));
// STATIC_ASSERT_EQ(sizeof(nt::PCLIENT_ID), sizeof(PCLIENT_ID));
STATIC_ASSERT_EQ(sizeof(nt::PCONTEXT), sizeof(PCONTEXT));
STATIC_ASSERT_EQ(sizeof(nt::PCRM_PROTOCOL_ID), sizeof(PCRM_PROTOCOL_ID));
// STATIC_ASSERT_EQ(sizeof(nt::PDBGUI_WAIT_STATE_CHANGE), sizeof(PDBGUI_WAIT_STATE_CHANGE));
// STATIC_ASSERT_EQ(sizeof(nt::PEFI_DRIVER_ENTRY), sizeof(PEFI_DRIVER_ENTRY));
STATIC_ASSERT_EQ(sizeof(nt::PEXCEPTION_RECORD), sizeof(PEXCEPTION_RECORD));
// STATIC_ASSERT_EQ(sizeof(nt::PFILE_BASIC_INFORMATION), sizeof(PFILE_BASIC_INFORMATION));
// STATIC_ASSERT_EQ(sizeof(nt::PFILE_IO_COMPLETION_INFORMATION), sizeof(PFILE_IO_COMPLETION_INFORMATION));
// STATIC_ASSERT_EQ(sizeof(nt::PFILE_NETWORK_OPEN_INFORMATION), sizeof(PFILE_NETWORK_OPEN_INFORMATION));
// STATIC_ASSERT_EQ(sizeof(nt::PFILE_PATH), sizeof(PFILE_PATH));
STATIC_ASSERT_EQ(sizeof(nt::PFILE_SEGMENT_ELEMENT), sizeof(PFILE_SEGMENT_ELEMENT));
STATIC_ASSERT_EQ(sizeof(nt::PGENERIC_MAPPING), sizeof(PGENERIC_MAPPING));
STATIC_ASSERT_EQ(sizeof(nt::PGROUP_AFFINITY), sizeof(PGROUP_AFFINITY));
STATIC_ASSERT_EQ(sizeof(nt::PHANDLE), sizeof(PHANDLE));
// STATIC_ASSERT_EQ(sizeof(nt::PINITIAL_TEB), sizeof(PINITIAL_TEB));
STATIC_ASSERT_EQ(sizeof(nt::PIO_APC_ROUTINE), sizeof(PIO_APC_ROUTINE));
STATIC_ASSERT_EQ(sizeof(nt::PIO_STATUS_BLOCK), sizeof(PIO_STATUS_BLOCK));
STATIC_ASSERT_EQ(sizeof(nt::PJOB_SET_ARRAY), sizeof(PJOB_SET_ARRAY));
STATIC_ASSERT_EQ(sizeof(nt::PKEY_VALUE_ENTRY), sizeof(PKEY_VALUE_ENTRY));
STATIC_ASSERT_EQ(sizeof(nt::PKTMOBJECT_CURSOR), sizeof(PKTMOBJECT_CURSOR));
STATIC_ASSERT_EQ(sizeof(nt::PLARGE_INTEGER), sizeof(PLARGE_INTEGER));
STATIC_ASSERT_EQ(sizeof(nt::PLCID), sizeof(PLCID));
STATIC_ASSERT_EQ(sizeof(nt::PLONG), sizeof(PLONG));
// STATIC_ASSERT_EQ(sizeof(nt::PLUGPLAY_CONTROL_CLASS), sizeof(PLUGPLAY_CONTROL_CLASS));
STATIC_ASSERT_EQ(sizeof(nt::PLUID), sizeof(PLUID));
STATIC_ASSERT_EQ(sizeof(nt::PNTSTATUS), sizeof(PNTSTATUS));
STATIC_ASSERT_EQ(sizeof(nt::POBJECT_ATTRIBUTES), sizeof(POBJECT_ATTRIBUTES));
STATIC_ASSERT_EQ(sizeof(nt::POBJECT_TYPE_LIST), sizeof(POBJECT_TYPE_LIST));
// STATIC_ASSERT_EQ(sizeof(nt::PORT_INFORMATION_CLASS), sizeof(PORT_INFORMATION_CLASS));
STATIC_ASSERT_EQ(sizeof(nt::POWER_ACTION), sizeof(POWER_ACTION));
STATIC_ASSERT_EQ(sizeof(nt::POWER_INFORMATION_LEVEL), sizeof(POWER_INFORMATION_LEVEL));
// STATIC_ASSERT_EQ(sizeof(nt::PPLUGPLAY_EVENT_BLOCK), sizeof(PPLUGPLAY_EVENT_BLOCK));
// STATIC_ASSERT_EQ(sizeof(nt::PPORT_MESSAGE), sizeof(PPORT_MESSAGE));
// STATIC_ASSERT_EQ(sizeof(nt::PPORT_VIEW), sizeof(PPORT_VIEW));
STATIC_ASSERT_EQ(sizeof(nt::PPRIVILEGE_SET), sizeof(PPRIVILEGE_SET));
// STATIC_ASSERT_EQ(sizeof(nt::PPROCESS_ATTRIBUTE_LIST), sizeof(PPROCESS_ATTRIBUTE_LIST));
// STATIC_ASSERT_EQ(sizeof(nt::PPROCESS_CREATE_INFO), sizeof(PPROCESS_CREATE_INFO));
// STATIC_ASSERT_EQ(sizeof(nt::PPS_APC_ROUTINE), sizeof(PPS_APC_ROUTINE));
// STATIC_ASSERT_EQ(sizeof(nt::PPS_ATTRIBUTE_LIST), sizeof(PPS_ATTRIBUTE_LIST));
// STATIC_ASSERT_EQ(sizeof(nt::PREMOTE_PORT_VIEW), sizeof(PREMOTE_PORT_VIEW));
STATIC_ASSERT_EQ(sizeof(nt::PROCESSINFOCLASS), sizeof(PROCESSINFOCLASS));
// STATIC_ASSERT_EQ(sizeof(nt::PRTL_ATOM), sizeof(PRTL_ATOM));
STATIC_ASSERT_EQ(sizeof(nt::PRTL_USER_PROCESS_PARAMETERS), sizeof(PRTL_USER_PROCESS_PARAMETERS));
STATIC_ASSERT_EQ(sizeof(nt::PSECURITY_DESCRIPTOR), sizeof(PSECURITY_DESCRIPTOR));
STATIC_ASSERT_EQ(sizeof(nt::PSECURITY_QUALITY_OF_SERVICE), sizeof(PSECURITY_QUALITY_OF_SERVICE));
STATIC_ASSERT_EQ(sizeof(nt::PSID), sizeof(PSID));
STATIC_ASSERT_EQ(sizeof(nt::PSIZE_T), sizeof(PSIZE_T));
// STATIC_ASSERT_EQ(sizeof(nt::PTIMER_APC_ROUTINE), sizeof(PTIMER_APC_ROUTINE));
STATIC_ASSERT_EQ(sizeof(nt::PTOKEN_DEFAULT_DACL), sizeof(PTOKEN_DEFAULT_DACL));
STATIC_ASSERT_EQ(sizeof(nt::PTOKEN_GROUPS), sizeof(PTOKEN_GROUPS));
STATIC_ASSERT_EQ(sizeof(nt::PTOKEN_OWNER), sizeof(PTOKEN_OWNER));
STATIC_ASSERT_EQ(sizeof(nt::PTOKEN_PRIMARY_GROUP), sizeof(PTOKEN_PRIMARY_GROUP));
STATIC_ASSERT_EQ(sizeof(nt::PTOKEN_PRIVILEGES), sizeof(PTOKEN_PRIVILEGES));
STATIC_ASSERT_EQ(sizeof(nt::PTOKEN_SOURCE), sizeof(PTOKEN_SOURCE));
STATIC_ASSERT_EQ(sizeof(nt::PTOKEN_USER), sizeof(PTOKEN_USER));
STATIC_ASSERT_EQ(sizeof(nt::PTRANSACTION_NOTIFICATION), sizeof(PTRANSACTION_NOTIFICATION));
STATIC_ASSERT_EQ(sizeof(nt::PULARGE_INTEGER), sizeof(PULARGE_INTEGER));
STATIC_ASSERT_EQ(sizeof(nt::PULONG), sizeof(PULONG));
STATIC_ASSERT_EQ(sizeof(nt::PULONG_PTR), sizeof(PULONG_PTR));
STATIC_ASSERT_EQ(sizeof(nt::PUNICODE_STRING), sizeof(PUNICODE_STRING));
STATIC_ASSERT_EQ(sizeof(nt::PUSHORT), sizeof(PUSHORT));
STATIC_ASSERT_EQ(sizeof(nt::PVOID), sizeof(PVOID));
STATIC_ASSERT_EQ(sizeof(nt::PWSTR), sizeof(PWSTR));
STATIC_ASSERT_EQ(sizeof(nt::RESOURCEMANAGER_INFORMATION_CLASS), sizeof(RESOURCEMANAGER_INFORMATION_CLASS));
// STATIC_ASSERT_EQ(sizeof(nt::RTL_ATOM), sizeof(RTL_ATOM));
// STATIC_ASSERT_EQ(sizeof(nt::SECTION_INFORMATION_CLASS), sizeof(SECTION_INFORMATION_CLASS));
// STATIC_ASSERT_EQ(sizeof(nt::SECTION_INHERIT), sizeof(SECTION_INHERIT));
STATIC_ASSERT_EQ(sizeof(nt::SECURITY_INFORMATION), sizeof(SECURITY_INFORMATION));
// STATIC_ASSERT_EQ(sizeof(nt::SEMAPHORE_INFORMATION_CLASS), sizeof(SEMAPHORE_INFORMATION_CLASS));
// STATIC_ASSERT_EQ(sizeof(nt::SHUTDOWN_ACTION), sizeof(SHUTDOWN_ACTION));
STATIC_ASSERT_EQ(sizeof(nt::SIZE_T), sizeof(SIZE_T));
// STATIC_ASSERT_EQ(sizeof(nt::SYSDBG_COMMAND), sizeof(SYSDBG_COMMAND));
STATIC_ASSERT_EQ(sizeof(nt::SYSTEM_INFORMATION_CLASS), sizeof(SYSTEM_INFORMATION_CLASS));
STATIC_ASSERT_EQ(sizeof(nt::SYSTEM_POWER_STATE), sizeof(SYSTEM_POWER_STATE));
STATIC_ASSERT_EQ(sizeof(nt::THREADINFOCLASS), sizeof(THREADINFOCLASS));
// STATIC_ASSERT_EQ(sizeof(nt::TIMER_INFORMATION_CLASS), sizeof(TIMER_INFORMATION_CLASS));
// STATIC_ASSERT_EQ(sizeof(nt::TIMER_SET_INFORMATION_CLASS), sizeof(TIMER_SET_INFORMATION_CLASS));
// STATIC_ASSERT_EQ(sizeof(nt::TIMER_TYPE), sizeof(TIMER_TYPE));
STATIC_ASSERT_EQ(sizeof(nt::TOKEN_INFORMATION_CLASS), sizeof(TOKEN_INFORMATION_CLASS));
STATIC_ASSERT_EQ(sizeof(nt::TOKEN_TYPE), sizeof(TOKEN_TYPE));
STATIC_ASSERT_EQ(sizeof(nt::TRANSACTIONMANAGER_INFORMATION_CLASS), sizeof(TRANSACTIONMANAGER_INFORMATION_CLASS));
STATIC_ASSERT_EQ(sizeof(nt::TRANSACTION_INFORMATION_CLASS), sizeof(TRANSACTION_INFORMATION_CLASS));
STATIC_ASSERT_EQ(sizeof(nt::ULONG), sizeof(ULONG));
STATIC_ASSERT_EQ(sizeof(nt::ULONG_PTR), sizeof(ULONG_PTR));
STATIC_ASSERT_EQ(sizeof(nt::USHORT), sizeof(USHORT));
// STATIC_ASSERT_EQ(sizeof(nt::VDMSERVICECLASS), sizeof(VDMSERVICECLASS));
// STATIC_ASSERT_EQ(sizeof(nt::WAIT_TYPE), sizeof(WAIT_TYPE));
// STATIC_ASSERT_EQ(sizeof(nt::WIN32_PROTECTION_MASK), sizeof(WIN32_PROTECTION_MASK));
// STATIC_ASSERT_EQ(sizeof(nt::WORKERFACTORYINFOCLASS), sizeof(WORKERFACTORYINFOCLASS));
#endif
