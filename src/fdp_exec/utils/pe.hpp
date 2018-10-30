#pragma once

#include <map>

#include "types.hpp"
#include "core.hpp"

namespace pe
{
    const int UNWIND_VERSION_MASK       = 0b0111;
    const int UNWIND_CHAINED_FLAG_MASK  = 0b00100000;

    struct UnwindCode
    {
        uint8_t    code_offset;
        uint8_t    unwind_op_and_info;
        int        stack_size_used;
    };

    typedef uint8_t UnwindInfo[4];

    struct RuntimeFunction
    {
        uint32_t    start_address;
        uint32_t    end_address;
        uint32_t    unwind_info;
    };

    struct FunctionEntry
    {
        uint32_t    start_address;
        uint32_t    end_address;
        uint8_t     prolog_size;
        int         stack_frame_size;
        int         prev_frame_reg;
        uint8_t     frame_reg_offset;
        uint32_t    mother_start_addr;
        std::vector<UnwindCode> unwind_codes;
    };

    using FunctionTable = std::map<uint32_t, FunctionEntry>;

    enum pe_directory_entries_e
    {
        IMAGE_DIRECTORY_ENTRY_EXPORT,             // Export Directory
        IMAGE_DIRECTORY_ENTRY_IMPORT,             // Import Directory
        IMAGE_DIRECTORY_ENTRY_RESOURCE,           // Resource Directory
        IMAGE_DIRECTORY_ENTRY_EXCEPTION,          // Exception Directory
        IMAGE_DIRECTORY_ENTRY_SECURITY,           // Security Directory
        IMAGE_DIRECTORY_ENTRY_BASERELOC,          // Base Relocation Table
        IMAGE_DIRECTORY_ENTRY_DEBUG,              // Debug Directory
        IMAGE_DIRECTORY_ENTRY_ARCHITECTURE,       // Architecture Specific Data
        IMAGE_DIRECTORY_ENTRY_GLOBALPTR,          // RVA of GP
        IMAGE_DIRECTORY_ENTRY_TLS,                // TLS Directory
        IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,        // Load Configuration Directory
        IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT,       // Bound Import Directory in headers
        IMAGE_DIRECTORY_ENTRY_IAT,                // Import Address Table
        IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT,       // Delay Load Import Descriptors
        IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,     // COM Runtime descriptor
    };

    enum unwind_op_codes_e
    {
        UWOP_PUSH_NONVOL,                         // info : register number
        UWOP_ALLOC_LARGE,                         // no info, alloc size in next 2 slots
        UWOP_ALLOC_SMALL,                         // info : size of allocation / 8 - 1
        UWOP_SET_FPREG,                           // no info, FP = RSP + UNWIND_INFO.FPRegOffset*16
        UWOP_SAVE_NONVOL,                         // info : register number, offset in next slot
        UWOP_SAVE_NONVOL_FAR,                     // info : register number, offset in next 2 slots
        UWOP_SAVE_XMM128,                         // info : XMM reg number, offset in next slot
        UWOP_SAVE_XMM128_FAR,                     // info : XMM reg number, offset in next 2 slots
        UWOP_PUSH_MACHFRAME,                      // info : 0: no error-code, 1: error-code
    };

    enum register_numbers_e
    {
        UWINFO_RAX,
        UWINFO_RCX,
        UWINFO_RDX,
        UWINFO_RBX,
        UWINFO_RSP,
        UWINFO_RBP,
        UWINFO_RSI,
        UWINFO_RDI,
    };

    // TODO Create a setup function

    opt<size_t> read_image_size(const void* src, size_t size);
    opt<span_t> get_directory_entry(core::Core& core, const span_t span, const pe_directory_entries_e directory_entry_id);
    opt<FunctionTable> parse_exception_dir(core::Core& core, void* vsrc, const uint64_t mod_base_addr, const span_t exception_dir);
    const FunctionEntry* lookup_function_entry(const uint64_t addr, FunctionTable function_table);

}
