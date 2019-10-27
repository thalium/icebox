#include "symbols.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "symbols"
#include "core.hpp"
#include "core_private.hpp"
#include "interfaces/if_symbols.hpp"
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
    using Mod         = std::unique_ptr<symbols::Module>;
    using Mods        = std::unordered_map<ModKey, Mod>;
    using Data        = symbols::Modules::Data;
    using Buffer      = std::vector<uint8_t>;
    using Breakpoints = std::vector<os::bpid_t>;
}

struct symbols::Modules::Data
{
    Data(core::Core& core)
        : core(core)
    {
    }

    core::Core& core;
    Mods        mods;
    Breakpoints breakpoints;
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
        Mod (*make)(span_t, const reader::Reader&);
    } g_helpers[] =
    {
            {"pdb", &symbols::make_pdb},
            {"dwarf", &symbols::make_dwarf},
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

bool symbols::Modules::insert(proc_t proc, const std::string& module, Mod symbols)
{
    const auto ret = d_->mods.emplace(ModKey{module, proc}, std::move(symbols));
    return ret.second;
}

bool symbols::Modules::insert(proc_t proc, const std::string& name, span_t module)
{
    // do not reload known modules
    auto& d       = *d_;
    const auto it = d.mods.find({name, proc});
    if(it != d.mods.end())
    {
        const auto span = it->second->span();
        if(span.addr == module.addr && span.size == module.size)
            return true;
    }

    const auto reader = is_kernel_proc(proc) ? reader::make(d.core) : reader::make(d.core, proc);
    for(const auto& h : g_helpers)
    {
        auto mod = h.make(module, reader);
        if(!mod)
            continue;

        return insert(proc, name, std::move(mod));
    }
    return false;
}

bool symbols::Modules::remove(proc_t proc, const std::string& name)
{
    const auto ret = d_->mods.erase({name, proc});
    return !!ret;
}

bool symbols::Modules::list(proc_t proc, const on_module_fn& on_module)
{
    for(const auto& m : d_->mods)
        if(m.first.proc.id == proc.id)
            if(on_module(*m.second) == walk_e::stop)
                break;
    return true;
}

namespace
{
    symbols::Module* find_module(Data& d, proc_t proc, const std::string& name)
    {
        const auto it = d.mods.find({name, proc});
        if(it != d.mods.end())
            return it->second.get();

        if(is_kernel_proc(proc))
            return nullptr;

        const auto ju = d.mods.find({name, symbols::kernel});
        if(ju != d.mods.end())
            return ju->second.get();

        return nullptr;
    }
}

symbols::Module* symbols::Modules::find(proc_t proc, const std::string& name)
{
    return find_module(*d_, proc, name);
}

opt<uint64_t> symbols::Modules::symbol(proc_t proc, const std::string& module, const std::string& symbol)
{
    const auto mod = find(proc, module);
    if(!mod)
        return {};

    return mod->symbol(symbol);
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
        std::string      name;
        symbols::Module& mod;
    };

    static opt<ModPair> find(Data& s, proc_t proc, uint64_t addr)
    {
        for(const auto& m : s.mods)
        {
            if(m.first.proc.id != proc.id)
                continue;

            const auto span = m.second->span();
            if(span.addr <= addr && addr < span.addr + span.size)
                return ModPair{m.first.name, *m.second};
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

    const auto cur = p->mod.symbol(addr);
    if(!cur)
        return {p->name.data(), "", addr};

    return {p->name.data(), cur->symbol, cur->offset};
}

std::string symbols::to_string(const symbols::Symbol& symbol)
{
    char dst[2 + 8 * 2 + 1];
    const char* offset = hex::convert<hex::RemovePadding | hex::HexaPrefix>(dst, symbol.offset);
    if(!symbol.module.empty() && !symbol.symbol.empty())
        return symbol.module + '!' + symbol.symbol + '+' + offset;

    if(!symbol.module.empty())
        return symbol.module + '+' + offset;

    return offset;
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

namespace
{
    void load_module_if(core::Core& core, proc_t proc, mod_t mod, const symbols::module_predicate_fn& predicate)
    {
        const auto name = modules::name(core, proc, mod);
        if(!name)
            return;

        if(predicate && !predicate(mod, *name))
            return;

        load_module_named_at(core, proc, mod, *name);
    }
}

opt<symbols::bpid_t> symbols::listen_and_load(core::Core& core, proc_t proc, const module_predicate_fn& predicate)
{
    modules::list(core, proc, [&](mod_t mod)
    {
        load_module_if(core, proc, mod, predicate);
        return walk_e::next;
    });
    const auto ptr  = &core;
    const auto bpid = modules::listen_create(core, [=](proc_t mod_proc, mod_t mod)
    {
        if(proc.id != mod_proc.id)
            return;

        load_module_if(*ptr, proc, mod, predicate);
    });
    if(bpid)
        core.symbols_->d_->breakpoints.emplace_back(*bpid);
    return bpid;
}

void symbols::unlisten(core::Core& core, bpid_t bpid)
{
    auto& bps = core.symbols_->d_->breakpoints;
    bps.erase(std::remove(bps.begin(), bps.end(), bpid), bps.end());
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
