#pragma once

#include "types.hpp"
#include "utils/utils.hpp"

// nt::         native types
// wow64::      wow64 types
// nt_types::   common types
namespace nt_types
{
    // ACCESS_MASK
    enum class ACCESS_MASK : uint32_t
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

    const char*                 access_mask_str (ACCESS_MASK arg);
    std::vector<const char*>    access_mask_all (uint32_t arg);

    // IOCTL_METHOD
    enum class IOCTL_METHOD
    {
        METHOD_BUFFERED   = 0,
        METHOD_IN_DIRECT  = 1,
        METHOD_OUT_DIRECT = 2,
        METHOD_NEITHER    = 3,
    };

    // DEVICE_TYPE
    enum class DEVICE_TYPE
    {
        FILE_DEVICE_NETWORK = 0x12,
    };

#define AFD_CONTROL_CODE(OPERATION, METHOD) ((static_cast<uint32_t>(DEVICE_TYPE::FILE_DEVICE_NETWORK) << 12) \
                                             | ((OPERATION) << 2)                                            \
                                             | static_cast<uint32_t>(METHOD))

    // IOCTL_CODE
    enum class IOCTL_CODE : uint32_t
    {
        AFD_BIND                        = AFD_CONTROL_CODE(0, IOCTL_METHOD::METHOD_NEITHER),
        AFD_CONNECT                     = AFD_CONTROL_CODE(1, IOCTL_METHOD::METHOD_NEITHER),
        AFD_START_LISTEN                = AFD_CONTROL_CODE(2, IOCTL_METHOD::METHOD_NEITHER),
        AFD_WAIT_FOR_LISTEN             = AFD_CONTROL_CODE(3, IOCTL_METHOD::METHOD_BUFFERED),
        AFD_ACCEPT                      = AFD_CONTROL_CODE(4, IOCTL_METHOD::METHOD_BUFFERED),
        AFD_RECV                        = AFD_CONTROL_CODE(5, IOCTL_METHOD::METHOD_NEITHER),
        AFD_RECV_DATAGRAM               = AFD_CONTROL_CODE(6, IOCTL_METHOD::METHOD_NEITHER),
        AFD_SEND                        = AFD_CONTROL_CODE(7, IOCTL_METHOD::METHOD_NEITHER),
        AFD_SEND_DATAGRAM               = AFD_CONTROL_CODE(8, IOCTL_METHOD::METHOD_NEITHER),
        AFD_SELECT                      = AFD_CONTROL_CODE(9, IOCTL_METHOD::METHOD_BUFFERED),
        AFD_DISCONNECT                  = AFD_CONTROL_CODE(10, IOCTL_METHOD::METHOD_NEITHER),
        AFD_GET_SOCK_NAME               = AFD_CONTROL_CODE(11, IOCTL_METHOD::METHOD_NEITHER),
        AFD_GET_PEER_NAME               = AFD_CONTROL_CODE(12, IOCTL_METHOD::METHOD_NEITHER),
        AFD_GET_TDI_HANDLES             = AFD_CONTROL_CODE(13, IOCTL_METHOD::METHOD_NEITHER),
        AFD_SET_INFO                    = AFD_CONTROL_CODE(14, IOCTL_METHOD::METHOD_NEITHER),
        AFD_GET_CONTEXT_SIZE            = AFD_CONTROL_CODE(15, IOCTL_METHOD::METHOD_NEITHER),
        AFD_GET_CONTEXT                 = AFD_CONTROL_CODE(16, IOCTL_METHOD::METHOD_NEITHER),
        AFD_SET_CONTEXT                 = AFD_CONTROL_CODE(17, IOCTL_METHOD::METHOD_NEITHER),
        AFD_SET_CONNECT_DATA            = AFD_CONTROL_CODE(18, IOCTL_METHOD::METHOD_NEITHER),
        AFD_SET_CONNECT_OPTIONS         = AFD_CONTROL_CODE(19, IOCTL_METHOD::METHOD_NEITHER),
        AFD_SET_DISCONNECT_DATA         = AFD_CONTROL_CODE(20, IOCTL_METHOD::METHOD_NEITHER),
        AFD_SET_DISCONNECT_OPTIONS      = AFD_CONTROL_CODE(21, IOCTL_METHOD::METHOD_NEITHER),
        AFD_GET_CONNECT_DATA            = AFD_CONTROL_CODE(22, IOCTL_METHOD::METHOD_NEITHER),
        AFD_GET_CONNECT_OPTIONS         = AFD_CONTROL_CODE(23, IOCTL_METHOD::METHOD_NEITHER),
        AFD_GET_DISCONNECT_DATA         = AFD_CONTROL_CODE(24, IOCTL_METHOD::METHOD_NEITHER),
        AFD_GET_DISCONNECT_OPTIONS      = AFD_CONTROL_CODE(25, IOCTL_METHOD::METHOD_NEITHER),
        AFD_SET_CONNECT_DATA_SIZE       = AFD_CONTROL_CODE(26, IOCTL_METHOD::METHOD_NEITHER),
        AFD_SET_CONNECT_OPTIONS_SIZE    = AFD_CONTROL_CODE(27, IOCTL_METHOD::METHOD_NEITHER),
        AFD_SET_DISCONNECT_DATA_SIZE    = AFD_CONTROL_CODE(28, IOCTL_METHOD::METHOD_NEITHER),
        AFD_SET_DISCONNECT_OPTIONS_SIZE = AFD_CONTROL_CODE(29, IOCTL_METHOD::METHOD_NEITHER),
        AFD_GET_INFO                    = AFD_CONTROL_CODE(30, IOCTL_METHOD::METHOD_NEITHER),
        AFD_EVENT_SELECT                = AFD_CONTROL_CODE(33, IOCTL_METHOD::METHOD_NEITHER),
        AFD_ENUM_NETWORK_EVENTS         = AFD_CONTROL_CODE(34, IOCTL_METHOD::METHOD_NEITHER),
        AFD_DEFER_ACCEPT                = AFD_CONTROL_CODE(35, IOCTL_METHOD::METHOD_NEITHER),
        AFD_GET_PENDING_CONNECT_DATA    = AFD_CONTROL_CODE(41, IOCTL_METHOD::METHOD_NEITHER),
        AFD_VALIDATE_GROUP              = AFD_CONTROL_CODE(42, IOCTL_METHOD::METHOD_NEITHER),
    };

    const char* ioctl_code_str  (IOCTL_CODE arg);
    std::string ioctl_code_dump (IOCTL_CODE arg);

    enum class PAGE_ACCESS : uint32_t
    {
        PAGE_NOACCESS            = 0x1,
        PAGE_READONLY            = 0x2,
        PAGE_READWRITE           = 0x4,
        PAGE_WRITECOPY           = 0x8,
        PAGE_EXECUTE             = 0x10,
        PAGE_EXECUTE_READ        = 0x20,
        PAGE_EXECUTE_READWRITE   = 0x40,
        PAGE_EXECUTE_WRITECOPY   = 0x80,
        PAGE_GUARD               = 0x100,
        PAGE_NOCACHE             = 0x200,
        PAGE_WRITECOMBINE        = 0x400,
        PAGE_REVERT_TO_FILE_MAP  = 0x80000000,
        PAGE_TARGETS_INVALID     = 0x40000000,
        PAGE_ENCLAVE_UNVALIDATED = 0x20000000,
        PAGE_ENCLAVE_DECOMMIT    = 0x10000000,
    };
    const char*                 page_access_str (PAGE_ACCESS arg);
    std::vector<const char*>    page_access_all (uint32_t arg);
} // namespace nt_types
