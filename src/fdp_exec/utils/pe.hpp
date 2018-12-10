#pragma once

#include "types.hpp"

namespace core { struct Core; }
namespace reader { struct Reader; }

namespace pe
{
    enum image_directory_entry_e
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

        opt<span_t> find_image_directory(const reader::Reader& core, span_t module, image_directory_entry_e id);
        opt<span_t> find_debug_codeview (const reader::Reader& core, span_t module);

        struct Data;
        std::unique_ptr<Data> d_;
    };

    opt<size_t> read_image_size(const void* src, size_t size);
} // namespace pe
