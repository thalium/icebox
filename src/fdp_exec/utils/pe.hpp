#pragma once

#include <map>

#include "types.hpp"
#include "core.hpp"

namespace pe
{
    struct UnwindCode
    {
        uint8_t    code_offset;
        uint8_t    unwind_op_and_info;
        int        stack_size_used;
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

    struct Pe
    {
         Pe();
        ~Pe();

        bool setup(core::Core& core);

        opt<span_t>          get_directory_entry  (core::Core& core, const span_t span, const pe_directory_entries_e directory_entry_id);
        opt<FunctionTable>   parse_exception_dir  (core::Core& core, void* src, const uint64_t mod_base_addr, const span_t exception_dir);
        opt<span_t>          parse_debug_dir      (void* src, const uint64_t mod_base_addr, const span_t debug_dir);
        const FunctionEntry* lookup_function_entry(const uint64_t addr, FunctionTable function_table);

        struct Data;
        std::unique_ptr<Data> d_;
    };

    opt<size_t>          read_image_size      (const void* src, size_t size);
}
