#pragma once

#include "types.hpp"

#include <memory>

namespace core
{
    // auto-managed process object
    struct ProcessContextPrivate;
    using ProcessContext = std::shared_ptr<ProcessContextPrivate>;

    // private break state
    struct BreakState;

    struct Memory
    {
         Memory();
        ~Memory();

        opt<uint64_t>   virtual_to_physical (uint64_t ptr, dtb_t dtb);
        bool            read_virtual        (void* dst, uint64_t src, size_t size);
        bool            read_virtual        (void* dst, dtb_t dtb, uint64_t src, size_t size);
        bool            read_physical       (void* dst, uint64_t src, size_t size);

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace core
