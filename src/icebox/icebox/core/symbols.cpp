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

    inline bool operator==(const ModKey& a, const ModKey& b)
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
    const struct
    {
        const char* name;
        opt<symbols::Identity>  (*identify) (span_t, const reader::Reader&);
        ModulePtr               (*make)     (const std::string& name, const std::string& guid);
    } g_helpers[] =
    {
            {"pdb", &symbols::identify_pdb, &symbols::make_pdb},
    };

    bool is_kernel_proc(proc_t proc)
    {
        return proc.id == symbols::kernel.id;
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
    std::string fix_module_name(const std::string& name)
    {
        auto is_lower       = false;
        auto is_upper       = false;
        const auto stripped = fs::path(name).filename().replace_extension().generic_string();
        auto ret            = stripped;
        for(auto& c : ret)
        {
            const auto alpha = isalpha(c);
            is_lower |= alpha && tolower(c) == c;
            is_upper |= alpha && toupper(c) == c;
            if(is_lower && is_upper)
                return stripped;

            c = static_cast<char>(tolower(c));
        }
        return ret;
    }
}

bool symbols::Modules::insert(proc_t proc, span_t span)
{
    // do not reload known modules
    auto& d           = *d_;
    const auto reader = is_kernel_proc(proc) ? reader::make(d.core) : reader::make(d.core, proc);
    for(const auto& h : g_helpers)
    {
        const auto opt_id = h.identify(span, reader);
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

        const auto name = fix_module_name(opt_id->name);
        return insert_module(d, proc, name, span, mod, is_cached ? insert_e::cached : insert_e::loaded);
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

bool symbols::Modules::remove(proc_t proc, const std::string& module)
{
    auto& d       = *d_;
    const auto it = d.mods.find({module, proc});
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
    enum find_e
    {
        all,
        proc_only,
    };

    opt<Mod> find_module(Data& d, proc_t proc, const std::string& name, find_e efind)
    {
        const auto it = d.mods.find({name, proc});
        if(it != d.mods.end())
            return it->second;

        if(efind == find_e::proc_only)
            return {};

        if(is_kernel_proc(proc))
            return {};

        const auto ju = d.mods.find({name, symbols::kernel});
        if(ju != d.mods.end())
            return ju->second;

        return {};
    }
}

symbols::Module* symbols::Modules::find(proc_t proc, const std::string& module)
{
    const auto it = find_module(*d_, proc, module, find_e::all);
    return it ? it->module.get() : nullptr;
}

opt<uint64_t> symbols::Modules::address(proc_t proc, const std::string& module, const std::string& symbol)
{
    const auto it = find_module(*d_, proc, module, find_e::all);
    if(!it)
        return {};

    const auto opt_offset = it->module->symbol_offset(symbol);
    if(!opt_offset)
        return {};

    return it->span.addr + *opt_offset;
}

void symbols::Modules::struc_names(proc_t proc, const std::string& module, const symbols::on_name_fn& on_struc)
{
    const auto mod = find(proc, module);
    if(mod)
        mod->struc_names(on_struc);
}

opt<size_t> symbols::Modules::struc_size(proc_t proc, const std::string& module, const std::string& struc)
{
    const auto mod = find(proc, module);
    if(!mod)
        return {};

    return mod->struc_size(struc);
}

void symbols::Modules::struc_members(proc_t proc, const std::string& module, const std::string& struc, const on_name_fn& on_member)
{
    const auto mod = find(proc, module);
    if(mod)
        mod->struc_members(struc, on_member);
}

opt<uint64_t> symbols::Modules::member_offset(proc_t proc, const std::string& module, const std::string& struc, const std::string& member)
{
    const auto mod = find(proc, module);
    if(!mod)
        return {};

    return mod->member_offset(struc, member);
}

namespace
{
    struct ModPair
    {
        std::string name;
        Mod         mod;
    };

    opt<ModPair> find(Data& s, proc_t proc, uint64_t addr)
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

    auto find_mod(Data& s, proc_t proc, uint64_t addr)
    {
        const auto p = find(s, proc, addr);
        if(p)
            return p;

        return find(s, symbols::kernel, addr);
    }

    symbols::Symbol read_name_from_proc(core::Core& core, proc_t proc, uint64_t addr)
    {
        const auto name = process::name(core, proc);
        if(!name)
            return {"", "", addr};

        return {fix_module_name(*name), "", addr};
    }

    symbols::Symbol read_empty_symbol(core::Core& core, proc_t proc, uint64_t addr)
    {
        const auto mod = modules::find(core, proc, addr);
        if(!mod)
            return read_name_from_proc(core, proc, addr);

        const auto span = modules::span(core, proc, *mod);
        if(!span)
            return read_name_from_proc(core, proc, addr);

        const auto name = modules::name(core, proc, *mod);
        const auto path = name ? fix_module_name(*name) : "";
        return {path, "", addr - span->addr};
    }
}

symbols::Symbol symbols::Modules::find(proc_t proc, uint64_t addr)
{
    auto& d      = *d_;
    const auto p = ::find_mod(d, proc, addr);
    if(!p)
        return read_empty_symbol(d.core, proc, addr);

    const auto cur = p->mod.module->find_symbol(addr - p->mod.span.addr);
    if(!cur)
        return {p->name, "", addr};

    return {p->name, cur->symbol, cur->offset};
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

bool symbols::load_module_memory(core::Core& core, proc_t proc, span_t span)
{
    return core.symbols_->insert(proc, span);
}

namespace
{
    bool load_module_from(core::Core& core, proc_t proc, mod_t mod)
    {
        const auto opt_span = modules::span(core, proc, mod);
        if(!opt_span)
            return false;

        return symbols::load_module_memory(core, proc, *opt_span);
    }

    bool copy_module(Data& d, proc_t proc, const std::string& module)
    {
        const auto opt_mod = find_module(d, symbols::kernel, module, find_e::proc_only);
        if(!opt_mod)
            return false;

        return insert_module(d, proc, module, opt_mod->span, opt_mod->module, insert_e::cached);
    }

    bool is_target_module(core::Core& core, proc_t proc, mod_t mod, const reader::Reader& reader, const std::string& name)
    {
        const auto opt_span = modules::span(core, proc, mod);
        if(!opt_span)
            return false;

        for(const auto& h : g_helpers)
        {
            const auto opt_id = h.identify(*opt_span, reader);
            if(!opt_id)
                continue;

            const auto fix_name = fix_module_name(opt_id->name);
            if(fix_name == name)
                return true;
        }
        return false;
    }

    opt<mod_t> wait_for_module(core::Core& core, proc_t proc, const std::string& name)
    {
        auto found = opt<mod_t>{};
        process::join(core, proc, mode_e::user);
        const auto reader    = reader::make(core, proc);
        const auto check_mod = [&](mod_t mod)
        {
            if(is_target_module(core, proc, mod, reader, name))
                found = mod;
            return found ? walk_e::stop : walk_e::next;
        };
        modules::list(core, proc, check_mod);
        if(found)
            return found;

        // module not found yet, wait for it...
        const auto bpx64 = modules::listen_create(core, proc, flags::x64, check_mod);
        if(!bpx64)
            return {};

        const auto bpx86 = modules::listen_create(core, proc, flags::x86, check_mod);
        if(!bpx86)
        {
            os::unlisten(core, *bpx64);
            return {};
        }

        while(!found)
        {
            state::resume(core);
            state::wait(core);
        }
        os::unlisten(core, *bpx64);
        os::unlisten(core, *bpx86);
        return found;
    }
}

bool symbols::load_module(core::Core& core, proc_t proc, const std::string& name)
{
    auto& d = *core.symbols_->d_;
    if(find_module(d, proc, name, find_e::proc_only))
        return true;

    const auto ok = copy_module(d, proc, name);
    if(ok)
        return true;

    const auto opt_mod = wait_for_module(core, proc, name);
    if(!opt_mod)
        return false;

    return load_module_from(core, proc, *opt_mod);
}

bool symbols::load_modules(core::Core& core, proc_t proc)
{
    modules::list(core, proc, [&](mod_t mod)
    {
        load_module_from(core, proc, mod);
        return walk_e::next;
    });
    return true;
}

opt<symbols::bpid_t> symbols::autoload_modules(core::Core& core, proc_t proc)
{
    load_modules(core, proc);
    const auto ptr   = &core;
    const auto flags = process::flags(core, proc);
    return modules::listen_create(core, proc, flags, [=](mod_t mod)
    {
        load_module_from(*ptr, proc, mod);
    });
}

bool symbols::load_driver_memory(core::Core& core, span_t span)
{
    return core.symbols_->insert(symbols::kernel, span);
}

bool symbols::load_driver(core::Core& core, driver_t driver)
{
    const auto span = drivers::span(core, driver);
    if(!span)
        return false;

    return load_driver_memory(core, *span);
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

bool symbols::unload(core::Core& core, proc_t proc, const std::string& module)
{
    return core.symbols_->remove(proc, module);
}

opt<uint64_t> symbols::address(core::Core& core, proc_t proc, const std::string& module, const std::string& symbol)
{
    return core.symbols_->address(proc, module, symbol);
}

void symbols::struc_names(core::Core& core, proc_t proc, const std::string& module, const on_name_fn& on_struc)
{
    core.symbols_->struc_names(proc, module, on_struc);
}

opt<size_t> symbols::struc_size(core::Core& core, proc_t proc, const std::string& module, const std::string& struc)
{
    return core.symbols_->struc_size(proc, module, struc);
}

void symbols::struc_members(core::Core& core, proc_t proc, const std::string& module, const std::string& struc, const on_name_fn& on_member)
{
    core.symbols_->struc_members(proc, module, struc, on_member);
}

opt<uint64_t> symbols::member_offset(core::Core& core, proc_t proc, const std::string& module, const std::string& struc, const std::string& member)
{
    return core.symbols_->member_offset(proc, module, struc, member);
}

symbols::Symbol symbols::find(core::Core& core, proc_t proc, uint64_t addr)
{
    return core.symbols_->find(proc, addr);
}
