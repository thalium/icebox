#pragma once

#include "icebox/types.hpp"

namespace core { struct Core; }
namespace memory { struct Io; }

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

    opt<span_t> find_image_directory(const memory::Io& io, span_t span, image_directory_entry_e id);
    opt<span_t> find_debug_codeview (const memory::Io& io, span_t span);
    opt<bool>   is_pe64             (const memory::Io& io, const uint64_t image_file_header);
    opt<size_t> read_image_size     (const void* src, size_t size);
} // namespace pe
