#pragma once

#include "types.hpp"

namespace core { struct Core; }
namespace reader { struct Reader; }

namespace pe
{
    enum pe_directory_entries_e
    {
        IMAGE_DIRECTORY_ENTRY_EXPORT,         // Export Directory
        IMAGE_DIRECTORY_ENTRY_IMPORT,         // Import Directory
        IMAGE_DIRECTORY_ENTRY_RESOURCE,       // Resource Directory
        IMAGE_DIRECTORY_ENTRY_EXCEPTION,      // Exception Directory
        IMAGE_DIRECTORY_ENTRY_SECURITY,       // Security Directory
        IMAGE_DIRECTORY_ENTRY_BASERELOC,      // Base Relocation Table
        IMAGE_DIRECTORY_ENTRY_DEBUG,          // Debug Directory
        IMAGE_DIRECTORY_ENTRY_ARCHITECTURE,   // Architecture Specific Data
        IMAGE_DIRECTORY_ENTRY_GLOBALPTR,      // RVA of GP
        IMAGE_DIRECTORY_ENTRY_TLS,            // TLS Directory
        IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,    // Load Configuration Directory
        IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT,   // Bound Import Directory in headers
        IMAGE_DIRECTORY_ENTRY_IAT,            // Import Address Table
        IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT,   // Delay Load Import Descriptors
        IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR, // COM Runtime descriptor
    };

    struct Pe
    {
         Pe();
        ~Pe();

        bool setup(core::Core& core);

        opt<span_t> get_directory_entry (const reader::Reader& core, const span_t span, const pe_directory_entries_e directory_entry_id);
        opt<span_t> parse_debug_dir     (const void* src, uint64_t mod_base_addr, span_t debug_dir);

        struct Data;
        std::unique_ptr<Data> d_;
    };

    opt<size_t> read_image_size(const void* src, size_t size);
} // namespace pe
