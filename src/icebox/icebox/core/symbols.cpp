#include "symbols.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "sym"
#include "core.hpp"
#include "core_private.hpp"
#include "interfaces/if_symbols.hpp"
#include "log.hpp"

#include "utils/hash.hpp"
#include "utils/hex.hpp"
#include "utils/path.hpp"

#include <unordered_map>

#ifdef _MSC_VER
#    define strnicmp _strnicmp
#else
#    include <strings.h>
#    define strnicmp strncasecmp
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
        opt<symbols::Identity>  (*identify) (span_t, const memory::Io&);
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

bool symbols::Modules::insert(proc_t proc, const std::string& module, span_t span, const std::shared_ptr<Module>& symbols)
{
    return insert_module(*d_, proc, module, span, symbols, insert_e::loaded);
}

namespace
{
    std::string fix_module_name(const std::string& name)
    {
        auto is_lower       = false;
        auto is_upper       = false;
        const auto stripped = path::filename(name).replace_extension().generic_string();
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

bool symbols::Modules::insert(proc_t proc, const memory::Io& io, span_t span)
{
    // do not reload known modules
    auto& d = *d_;
    for(const auto& h : g_helpers)
    {
        const auto opt_id = h.identify(span, io);
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
    enum class find_e
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

void symbols::Modules::list_strucs(proc_t proc, const std::string& module, const symbols::on_name_fn& on_struc)
{
    const auto mod = find(proc, module);
    if(mod)
        mod->list_strucs(on_struc);
}

opt<symbols::Struc> symbols::Modules::read_struc(proc_t proc, const std::string& module, const std::string& struc)
{
    const auto mod = find(proc, module);
    if(!mod)
        return {};

    return mod->read_struc(struc);
}

namespace
{
    bool is_lowercase_equal(const std::string_view& a, const std::string_view& b)
    {
        if(a.size() != b.size())
            return false;

        return !strnicmp(a.data(), b.data(), a.size());
    }
}

void symbols::list_strucs(core::Core& core, proc_t proc, const std::string& module, const on_name_fn& on_struc)
{
    return core.symbols_->list_strucs(proc, module, on_struc);
}

opt<symbols::Struc> symbols::read_struc(core::Core& core, proc_t proc, const std::string& module, const std::string& struc)
{
    return core.symbols_->read_struc(proc, module, struc);
}

opt<symbols::Member> symbols::find_member(const Struc& struc, const std::string& member)
{
    for(const auto& m : struc.members)
        if(is_lowercase_equal(m.name, member))
            return m;

    return {};
}

opt<symbols::Member> symbols::read_member(core::Core& core, proc_t proc, const std::string& module, const std::string& struc, const std::string& member)
{
    const auto opt_struc = read_struc(core, proc, module, struc);
    if(!opt_struc)
        return {};

    return find_member(*opt_struc, member);
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

    opt<std::string> read_name_from_module(core::Core& core, proc_t proc, uint64_t addr)
    {
        const auto opt_mod = modules::find(core, proc, addr);
        if(!opt_mod)
            return {};

        const auto span = modules::span(core, proc, *opt_mod);
        if(!span)
            return {};

        const auto name = modules::name(core, proc, *opt_mod);
        if(!name)
            return {};

        return fix_module_name(*name) + to_offset('+', addr - span->addr);
    }

    opt<std::string> read_name_from_driver(core::Core& core, uint64_t addr)
    {
        const auto opt_drv = drivers::find(core, addr);
        if(!opt_drv)
            return {};

        const auto span = drivers::span(core, *opt_drv);
        if(!span)
            return {};

        const auto name = drivers::name(core, *opt_drv);
        if(!name)
            return {};

        return fix_module_name(*name) + to_offset('+', addr - span->addr);
    }

    std::string read_empty_symbol(core::Core& core, proc_t proc, uint64_t addr)
    {
        if(!is_kernel_proc(proc))
        {
            if(!process::is_valid(core, proc))
                return to_offset(0, addr);

            const auto opt_mod = read_name_from_module(core, proc, addr);
            if(opt_mod)
                return *opt_mod;
        }

        const auto opt_drv = read_name_from_driver(core, addr);
        if(opt_drv)
            return *opt_drv;

        return to_offset(0, addr);
    }
}

std::string symbols::Modules::string(proc_t proc, uint64_t addr)
{
    auto& d      = *d_;
    const auto p = ::find_mod(d, proc, addr);
    if(!p)
        return read_empty_symbol(d.core, proc, addr);

    const auto cur = p->mod.module->find_symbol(addr - p->mod.span.addr);
    if(!cur)
        return p->name + to_offset('+', addr - p->mod.span.addr);

    return p->name + '!' + cur->symbol + to_offset('+', cur->offset);
}

bool symbols::load_module_memory(core::Core& core, proc_t proc, const memory::Io& io, span_t span)
{
    return core.symbols_->insert(proc, io, span);
}

namespace
{
    bool load_module_from(core::Core& core, proc_t proc, mod_t mod)
    {
        const auto opt_span = modules::span(core, proc, mod);
        if(!opt_span)
            return false;

        const auto io = memory::make_io(core, proc);
        return symbols::load_module_memory(core, proc, io, *opt_span);
    }

    bool copy_module(Data& d, proc_t proc, const std::string& module)
    {
        const auto opt_mod = find_module(d, symbols::kernel, module, find_e::proc_only);
        if(!opt_mod)
            return false;

        return insert_module(d, proc, module, opt_mod->span, opt_mod->module, insert_e::cached);
    }

    bool is_target(span_t span, const memory::Io& io, const std::string& name)
    {
        for(const auto& h : g_helpers)
        {
            const auto opt_id = h.identify(span, io);
            if(!opt_id)
                continue;

            const auto fix_name = fix_module_name(opt_id->name);
            if(fix_name == name)
                return true;
        }
        return false;
    }

    bool is_target_module(core::Core& core, proc_t proc, mod_t mod, const memory::Io& io, const std::string& name)
    {
        const auto opt_span = modules::span(core, proc, mod);
        if(!opt_span)
            return false;

        return is_target(*opt_span, io, name);
    }

    opt<mod_t> wait_for_module(core::Core& core, proc_t proc, const std::string& name)
    {
        process::join(core, proc, mode_e::user);
        auto found           = opt<mod_t>{};
        const auto io        = memory::make_io(core, proc);
        const auto check_mod = [&](mod_t mod)
        {
            if(is_target_module(core, proc, mod, io, name))
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
            state::drop_breakpoint(core, *bpx64);
            return {};
        }

        while(!found)
            state::exec(core);

        state::drop_breakpoint(core, *bpx64);
        state::drop_breakpoint(core, *bpx86);
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

opt<bpid_t> symbols::autoload_modules(core::Core& core, proc_t proc)
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
    const auto io = memory::make_io_kernel(core);
    return core.symbols_->insert(symbols::kernel, io, span);
}

namespace
{
    opt<driver_t> wait_for_driver(core::Core& core, const std::string& name)
    {
        auto found           = opt<driver_t>{};
        const auto io        = memory::make_io_kernel(core);
        const auto check_drv = [&](driver_t drv)
        {
            const auto opt_span = drivers::span(core, drv);
            if(!opt_span)
                return walk_e::next;

            if(!is_target(*opt_span, io, name))
                return walk_e::next;

            found = drv;
            return walk_e::stop;
        };
        drivers::list(core, check_drv);
        if(found)
            return found;

        // driver not found yet, wait for it...
        const auto bp = drivers::listen_create(core, [=](driver_t drv, bool load)
        {
            if(!load)
                return walk_e::next;

            return check_drv(drv);
        });
        if(!bp)
            return {};

        while(!found)
            state::exec(core);

        state::drop_breakpoint(core, *bp);
        return found;
    }
}

bool symbols::load_driver(core::Core& core, const std::string& name)
{
    auto& d = *core.symbols_->d_;
    if(find_module(d, symbols::kernel, name, find_e::proc_only))
        return true;

    const auto opt_drv = wait_for_driver(core, name);
    if(!opt_drv)
        return false;

    const auto span = drivers::span(core, *opt_drv);
    if(!span)
        return false;

    return load_driver_memory(core, *span);
}

bool symbols::load_drivers(core::Core& core)
{
    drivers::list(core, [&](driver_t driver)
    {
        const auto opt_span = drivers::span(core, driver);
        if(opt_span)
            load_driver_memory(core, *opt_span);

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

std::string symbols::string(core::Core& core, proc_t proc, uint64_t addr)
{
    return core.symbols_->string(proc, addr);
}
