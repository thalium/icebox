#pragma once

#include "icebox/types.hpp"

#include <functional>
#include <memory>

namespace core { struct Core; }
namespace sym { struct Symbols; }

namespace sym
{
    using predicate_fn = std::function<bool(mod_t, const std::string&)>;

    struct Loader
    {
         Loader(core::Core& core, proc_t proc);
        ~Loader();

        void            mod_listen  ();
        void            mod_listen  (predicate_fn predicate);
        bool            load        (mod_t mod);
        sym::Symbols&   symbols     ();

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace sym
