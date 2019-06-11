#include "sym_loader.hpp"

#define FDP_MODULE "loader"
#include "core.hpp"
#include "log.hpp"
#include "os.hpp"
#include "reader.hpp"
#include "sym.hpp"
#include "utils/fnview.hpp"
#include "utils/path.hpp"
#include "utils/pe.hpp"

namespace
{
    using Breakpoints = std::vector<size_t>;
}

struct sym::Loader::Data
{
    Data(core::Core& core);

    core::Core&          core;
    sym::Symbols         symbols;
    std::vector<uint8_t> buffer;
    Breakpoints          breakpoints;
};

namespace
{
    using Data = sym::Loader::Data;

    static bool load_module_buffer(Data& d, const reader::Reader& reader, span_t span)
    {
        const auto debug = pe::find_debug_codeview(reader, span);
        if(!debug)
            return false;

        d.buffer.resize(debug->size);
        const auto read = reader.read(&d.buffer[0], debug->addr, debug->size);
        return read;
    }

    static bool load_span_named(Data& d, const reader::Reader& reader, const span_t& span, const std::string& name)
    {
        LOG(INFO, "{} loaded at {:x}:{:x}", name, span.addr, span.addr + span.size);
        const auto loaded = load_module_buffer(d, reader, span);
        if(!loaded)
            return false;

        const auto filename = path::filename(name).replace_extension();
        const auto inserted = d.symbols.insert(filename.generic_string(), span, &d.buffer[0], d.buffer.size());
        return inserted;
    }

    static bool load_module_named(Data& d, const reader::Reader& reader, proc_t proc, mod_t mod, const std::string& name)
    {
        LOG(INFO, "loading module {}", name);
        const auto span = d.core.os->mod_span(proc, mod);
        if(!span)
            return false;

        return load_span_named(d, reader, *span, name);
    }

    static bool load_driver_named(Data& d, const reader::Reader& reader, driver_t drv, const std::string& name)
    {
        LOG(INFO, "loading driver {}", name);
        const auto span = d.core.os->driver_span(drv);
        if(!span)
            return false;

        return load_span_named(d, reader, *span, name);
    }

    static bool load_module(Data& d, const reader::Reader& reader, proc_t proc, mod_t mod, const sym::mod_predicate_fn& predicate)
    {
        const auto name = d.core.os->mod_name(proc, mod);
        if(!name)
            return false;

        if(predicate && !predicate(mod, *name))
            return false;

        const auto loaded = load_module_named(d, reader, proc, mod, *name);
        if(!loaded)
            return FAIL(false, "unable to load symbols from {}", *name);

        return true;
    }

    static void load_driver(Data& d, const reader::Reader& reader, driver_t drv, const sym::drv_predicate_fn& predicate)
    {
        const auto name = d.core.os->driver_name(drv);
        if(!name)
            return;

        if(predicate && !predicate(drv, *name))
            return;

        const auto loaded = load_driver_named(d, reader, drv, *name);
        if(!loaded)
            LOG(ERROR, "unable to load symbols from {}", *name);
    }
}

Data::Data(core::Core& core)
    : core(core)
{
}

sym::Loader::Loader(core::Core& core)
    : d_(std::make_unique<Data>(core))
{
}

void sym::Loader::mod_listen(proc_t proc, sym::mod_predicate_fn predicate)
{
    auto& d           = *d_;
    const auto reader = reader::make(d.core, proc);
    d.core.os->mod_list(proc, [&](mod_t mod)
    {
        load_module(d, reader, proc, mod, predicate);
        return WALK_NEXT;
    });
    const auto bpid = d.core.os->listen_mod_create([=](proc_t mod_proc, mod_t mod)
    {
        if(proc.id != mod_proc.id)
            return;

        load_module(*d_, reader, proc, mod, predicate);
    });
    if(bpid)
        d.breakpoints.emplace_back(*bpid);
}

void sym::Loader::drv_listen(sym::drv_predicate_fn predicate)
{
    auto& d           = *d_;
    const auto reader = reader::make(d.core);
    d.core.os->driver_list([&](driver_t drv)
    {
        load_driver(d, reader, drv, predicate);
        return WALK_NEXT;
    });
    const auto bpid = d.core.os->listen_drv_create([=](driver_t drv, bool load)
    {
        if(load)
            load_driver(*d_, reader, drv, predicate);
    });
    if(bpid)
        d.breakpoints.emplace_back(*bpid);
}

sym::Loader::~Loader()
{
    auto& d = *d_;
    for(const auto bpid : d.breakpoints)
        d.core.os->unlisten(bpid);
}

bool sym::Loader::mod_load(proc_t proc, mod_t mod)
{
    auto& d     = *d_;
    auto reader = reader::make(d.core, proc);
    return load_module(d, reader, proc, mod, {});
}

sym::Symbols& sym::Loader::symbols()
{
    return d_->symbols;
}
