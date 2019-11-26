#include "symbols.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "sym"
#include "core.hpp"
#include "core_private.hpp"
#include "interfaces/if_symbols.hpp"
#include "log.hpp"
#include "reader.hpp"

#include "utils/hash.hpp"
#include "utils/hex.hpp"

#include <unordered_map>

#ifdef _MSC_VER
#    include <algorithm>
#    include <functional>
#    define search                          std::search
#    define boyer_moore_horspool_searcher   std::boyer_moore_horspool_searcher
#else
#    include <experimental/algorithm>
#    include <experimental/functional>
#    define search                          std::experimental::search
#    define boyer_moore_horspool_searcher   std::experimental::make_boyer_moore_horspool_searcher
#endif

namespace
{
    struct ModKey
    {
        std::string name;
        proc_t      proc;
    };

    static inline bool operator==(const ModKey& a, const ModKey& b)
    {
        return a.proc.id == b.proc.id && a.name == b.name;
    }
}

namespace std
{
    template <>
    struct hash<ModKey>
    {
        size_t operator()(const ModKey& arg) const
        {
            size_t seed = 0;
            ::hash::combine(seed, arg.name, arg.proc.id);
            return seed;
        }
    };
} // namespace std

namespace
{
    using ModulePtr = std::shared_ptr<symbols::Module>;
    struct Mod
    {
        ModulePtr module;
        span_t    span;
    };

    using Mods     = std::unordered_map<ModKey, Mod>;
    using ModByIds = std::unordered_map<std::string_view, ModulePtr>;
    using Data     = symbols::Modules::Data;
    using Buffer   = std::vector<uint8_t>;
}

struct symbols::Modules::Data
{
    Data(core::Core& core)
        : core(core)
    {
    }

    core::Core& core;
    Mods        mods;
    ModByIds    mod_by_ids;
    Buffer      buffer;
};

symbols::Modules::Modules(core::Core& core)
    : d_(std::make_unique<Data>(core))
{
}

symbols::Modules::~Modules() = default;

namespace
{
    static const struct
    {
        const char name[32];
        opt<symbols::Identity>  (*identify) (span_t, const reader::Reader&);
        ModulePtr               (*make)     (const std::string& name, const std::string& guid);
    } g_helpers[] =
    {
            {"pdb", &symbols::identify_pdb, &symbols::make_pdb},
    };

    bool is_kernel_proc(proc_t proc)
    {
        return proc.id == symbols::kernel.id && proc.dtb.val == symbols::kernel.dtb.val;
    }
}

symbols::Modules& symbols::Modules::modules(core::Core& core)
{
    return *core.symbols_;
}

namespace
{
    enum class insert_e
    {
        loaded,
        cached,
    };

    bool insert_module(Data& d, proc_t proc, const std::string& module, span_t span, const ModulePtr& sym, insert_e einsert)
    {
        LOG(INFO, "%s %s %s", einsert == insert_e::loaded ? "loaded" : "cached", sym->id().data(), module.data());
        const auto ret = d.mods.emplace(ModKey{module, proc}, Mod{sym, span});
        d.mod_by_ids.emplace(sym->id(), sym);
        return ret.second;
    }
}

bool symbols::Modules::insert(proc_t proc, const std::string& module, span_t span, const ModulePtr& symbols)
{
    return insert_module(*d_, proc, module, span, symbols, insert_e::loaded);
}

namespace
{
    bool has_module(Data& d, const std::string& name, const proc_t (&procs)[2], span_t module)
    {
        for(const auto& proc : procs)
        {
            const auto it = d.mods.find({name, proc});
            if(it == d.mods.end())
                continue;

            const auto span = it->second.span;
            if(span.addr == module.addr && span.size == module.size)
                return true;
        }
        return false;
    }
}

bool symbols::Modules::insert(proc_t proc, const std::string& name, span_t module)
{
    // do not reload known modules
    auto& d = *d_;
    if(has_module(d, name, {proc, symbols::kernel}, module))
        return true;

    const auto reader = is_kernel_proc(proc) ? reader::make(d.core) : reader::make(d.core, proc);
    for(const auto& h : g_helpers)
    {
        const auto opt_id = h.identify(module, reader);
        if(!opt_id)
            continue;

        const auto it        = d.mod_by_ids.find(opt_id->id);
        auto mod             = ModulePtr{};
        const auto is_cached = it != d.mod_by_ids.end();
        if(is_cached)
            mod = it->second;
        else
            mod = h.make(opt_id->name, opt_id->id);
        if(!mod)
            continue;

        return insert_module(d, proc, name, module, mod, is_cached ? insert_e::cached : insert_e::loaded);
    }
    return false;
}

namespace
{
    bool has_id(Data& d, std::string_view id)
    {
        for(const auto& it : d.mods)
            if(it.second.module->id() == id)
                return true;
        return false;
    }
}

bool symbols::Modules::remove(proc_t proc, const std::string& name)
{
    auto& d       = *d_;
    const auto it = d.mods.find({name, proc});
    if(it == d.mods.end())
        return false;

    const auto id = it->second.module->id();
    if(!has_id(d, id))
        d.mod_by_ids.erase(id);

    d.mods.erase(it);
    return true;
}

bool symbols::Modules::list(proc_t proc, const on_module_fn& on_module)
{
    for(const auto& m : d_->mods)
        if(m.first.proc.id == proc.id)
            if(on_module(m.second.span, *m.second.module) == walk_e::stop)
                break;
    return true;
}

namespace
{
    opt<Mod> find_module(Data& d, proc_t proc, const std::string& name)
    {
        const auto it = d.mods.find({name, proc});
        if(it != d.mods.end())
            return it->second;

        if(is_kernel_proc(proc))
            return {};

        const auto ju = d.mods.find({name, symbols::kernel});
        if(ju != d.mods.end())
            return ju->second;

        return {};
    }
}

symbols::Module* symbols::Modules::find(proc_t proc, const std::string& name)
{
    const auto it = find_module(*d_, proc, name);
    return it ? it->module.get() : nullptr;
}

opt<uint64_t> symbols::Modules::symbol(proc_t proc, const std::string& module, const std::string& symbol)
{
    const auto it = find_module(*d_, proc, module);
    if(!it)
        return {};

    const auto opt_offset = it->module->symbol(symbol);
    if(!opt_offset)
        return {};

    return it->span.addr + *opt_offset;
}

opt<uint64_t> symbols::Modules::struc_offset(proc_t proc, const std::string& module, const std::string& struc, const std::string& member)
{
    const auto mod = find(proc, module);
    if(!mod)
        return {};

    return mod->struc_offset(struc, member);
}

opt<size_t> symbols::Modules::struc_size(proc_t proc, const std::string& module, const std::string& struc)
{
    const auto mod = find(proc, module);
    if(!mod)
        return {};

    return mod->struc_size(struc);
}

namespace
{
    struct ModPair
    {
        std::string name;
        Mod         mod;
    };

    static opt<ModPair> find(Data& s, proc_t proc, uint64_t addr)
    {
        for(const auto& m : s.mods)
        {
            if(m.first.proc.id != proc.id)
                continue;

            const auto span = m.second.span;
            if(span.addr <= addr && addr < span.addr + span.size)
                return ModPair{m.first.name, m.second};
        }

        return {};
    }

    static auto find_mod(Data& s, proc_t proc, uint64_t addr)
    {
        const auto p = find(s, proc, addr);
        if(p)
            return p;

        return find(s, symbols::kernel, addr);
    }

    static symbols::Symbol read_name_from_proc(core::Core& core, proc_t proc, uint64_t addr)
    {
        const auto name = process::name(core, proc);
        if(!name)
            return {"", "", addr};

        const auto path = fs::path(*name).filename().replace_extension("").generic_string();
        return {path, "", addr};
    }

    static symbols::Symbol read_empty_symbol(core::Core& core, proc_t proc, uint64_t addr)
    {
        const auto mod = modules::find(core, proc, addr);
        if(!mod)
            return read_name_from_proc(core, proc, addr);

        const auto span = modules::span(core, proc, *mod);
        if(!span)
            return read_name_from_proc(core, proc, addr);

        const auto name = modules::name(core, proc, *mod);
        const auto path = name ? fs::path(*name).filename().replace_extension("").generic_string() : "";
        return {path, "", addr - span->addr};
    }
}

symbols::Symbol symbols::Modules::find(proc_t proc, uint64_t addr)
{
    auto& d      = *d_;
    const auto p = ::find_mod(d, proc, addr);
    if(!p)
        return read_empty_symbol(d.core, proc, addr);

    const auto cur = p->mod.module->symbol(addr - p->mod.span.addr);
    if(!cur)
        return {p->name.data(), "", addr};

    return {p->name.data(), cur->symbol, cur->offset};
}

namespace
{
    std::string to_offset(char prefix, uint64_t offset)
    {
        if(!offset)
            return {};

        char dst[2 + 8 * 2 + 1];
        const auto ptr  = hex::convert<hex::RemovePadding | hex::HexaPrefix>(dst, offset);
        const auto size = &dst[sizeof dst - 1] - ptr;
        auto ret        = std::string(!!prefix + size, prefix);
        memcpy(&ret[!!prefix], ptr, size);
        return ret;
    }
}

std::string symbols::to_string(const symbols::Symbol& symbol)
{
    if(!symbol.module.empty() && !symbol.symbol.empty())
        return symbol.module + '!' + symbol.symbol + to_offset('+', symbol.offset);

    if(!symbol.module.empty())
        return symbol.module + to_offset('+', symbol.offset);

    return to_offset(0, symbol.offset);
}

bool symbols::load_module_at(core::Core& core, proc_t proc, const std::string& name, span_t module)
{
    return core.symbols_->insert(proc, name, module);
}

namespace
{
    bool load_module_named_at(core::Core& core, proc_t proc, mod_t mod, const std::string& name)
    {
        const auto span = modules::span(core, proc, mod);
        if(!span)
            return false;

        const auto path = fs::path(name).filename().replace_extension("").generic_string();
        return symbols::load_module_at(core, proc, path, *span);
    }
}

bool symbols::load_module(core::Core& core, proc_t proc, mod_t mod)
{
    const auto name = modules::name(core, proc, mod);
    if(!name)
        return false;

    return load_module_named_at(core, proc, mod, *name);
}

bool symbols::load_modules(core::Core& core, proc_t proc)
{
    modules::list(core, proc, [&](mod_t mod)
    {
        load_module(core, proc, mod);
        return walk_e::next;
    });
    return true;
}

opt<symbols::bpid_t> symbols::autoload_modules(core::Core& core, proc_t proc)
{
    modules::list(core, proc, [&](mod_t mod)
    {
        load_module(core, proc, mod);
        return walk_e::next;
    });
    const auto ptr   = &core;
    const auto flags = process::flags(core, proc);
    return modules::listen_create(core, proc, flags, [=](mod_t mod)
    {
        load_module(*ptr, proc, mod);
    });
}

bool symbols::load_driver_at(core::Core& core, const std::string& name, span_t driver)
{
    return core.symbols_->insert(symbols::kernel, name, driver);
}

bool symbols::load_driver(core::Core& core, driver_t driver)
{
    const auto name = drivers::name(core, driver);
    if(!name)
        return false;

    const auto span = drivers::span(core, driver);
    if(!span)
        return false;

    const auto path = fs::path(*name).filename().replace_extension("").generic_string();
    return load_driver_at(core, path, *span);
}

bool symbols::load_drivers(core::Core& core)
{
    drivers::list(core, [&](driver_t driver)
    {
        load_driver(core, driver);
        return walk_e::next;
    });
    return true;
}

bool symbols::unload(core::Core& core, proc_t proc, const std::string& name)
{
    return core.symbols_->remove(proc, name);
}

opt<uint64_t> symbols::symbol(core::Core& core, proc_t proc, const std::string& module, const std::string& symbol)
{
    return core.symbols_->symbol(proc, module, symbol);
}

opt<uint64_t> symbols::struc_offset(core::Core& core, proc_t proc, const std::string& module, const std::string& struc, const std::string& member)
{
    return core.symbols_->struc_offset(proc, module, struc, member);
}

opt<size_t> symbols::struc_size(core::Core& core, proc_t proc, const std::string& module, const std::string& struc)
{
    return core.symbols_->struc_size(proc, module, struc);
}

symbols::Symbol symbols::find(core::Core& core, proc_t proc, uint64_t addr)
{
    return core.symbols_->find(proc, addr);
}
