#pragma once

#include "icebox/types.hpp"

#include <functional>
#include <memory>

namespace core { struct Core; }
namespace sym { struct Symbols; }

namespace sym
{
    using mod_predicate_fn = std::function<bool(mod_t, const std::string&)>;
    using drv_predicate_fn = std::function<bool(driver_t, const std::string&)>;

    struct Loader
    {
        Loader(core::Core& core, proc_t proc);

        // Loader initizialied without proc will load the drivers pdb
         Loader(core::Core& core);
        ~Loader();

        void            mod_listen  ();
        void            mod_listen  (mod_predicate_fn predicate);
        void            drv_listen  ();
        void            drv_listen  (drv_predicate_fn predicate);
        bool            load        (mod_t mod);
        sym::Symbols&   symbols     ();

        struct Data;
        std::unique_ptr<Data> d_;
    };
} // namespace sym
