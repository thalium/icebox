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

struct sym::Loader::Data
{
    Data(core::Core& core, proc_t proc);
    Data(core::Core& core);

    core::Core&          core;
    sym::Symbols         symbols;
    proc_t               proc;
    std::vector<uint8_t> buffer;
    reader::Reader       reader;
    opt<size_t>          mod_listen;
    opt<size_t>          drv_listen;
};

namespace
{
    using Data = sym::Loader::Data;

    static bool load_module_buffer(Data& d, span_t span)
    {
        const auto debug = pe::find_debug_codeview(d.reader, span);
        if(!debug)
            return false;

        d.buffer.resize(debug->size);
        const auto read = d.reader.read(&d.buffer[0], debug->addr, debug->size);
        return read;
    }

    static bool load_span_named(Data& d, const span_t& span, const std::string& name)
    {
        LOG(INFO, "{} loaded at {:x}:{:x}", name, span.addr, span.addr + span.size);
        const auto loaded = load_module_buffer(d, span);
        if(!loaded)
            return false;

        const auto filename = path::filename(name).replace_extension();
        const auto inserted = d.symbols.insert(filename.generic_string(), span, &d.buffer[0], d.buffer.size());
        return inserted;
    }

    static bool load_module_named(Data& d, mod_t mod, const std::string& name)
    {
        LOG(INFO, "loading module {}", name);
        const auto span = d.core.os->mod_span(d.proc, mod);
        if(!span)
            return false;

        return load_span_named(d, *span, name);
    }

    static bool load_driver_named(Data& d, driver_t drv, const std::string& name)
    {
        LOG(INFO, "loading driver {}", name);
        const auto span = d.core.os->driver_span(drv);
        if(!span)
            return false;

        return load_span_named(d, *span, name);
    }

    static bool load_module(Data& d, mod_t mod, const sym::mod_predicate_fn& predicate)
    {
        const auto name = d.core.os->mod_name(d.proc, mod);
        if(!name)
            return false;

        if(!predicate(mod, *name))
            return false;

        const auto loaded = load_module_named(d, mod, *name);
        if(!loaded)
            return FAIL(false, "unable to load symbols from {}", *name);

        return true;
    }

    static void load_driver(Data& d, driver_t drv, const sym::drv_predicate_fn& predicate)
    {
        const auto name = d.core.os->driver_name(drv);
        if(!name)
            return;

        if(!predicate(drv, *name))
            return;

        const auto loaded = load_driver_named(d, drv, *name);
        if(!loaded)
            LOG(ERROR, "unable to load symbols from {}", *name);
    }
}

Data::Data(core::Core& core, proc_t proc)
    : core(core)
    , proc(proc)
    , reader(reader::make(core, proc))
{
}

Data::Data(core::Core& core)
    : core(core)
    , proc({})
    , reader(reader::make(core))
{
}

sym::Loader::Loader(core::Core& core, proc_t proc)
    : d_(std::make_unique<Data>(core, proc))
{
}

sym::Loader::Loader(core::Core& core)
    : d_(std::make_unique<Data>(core))
{
}

void sym::Loader::mod_listen(sym::mod_predicate_fn predicate)
{
    auto& d = *d_;
    d.core.os->mod_list(d.proc, [&](mod_t mod)
    {
        load_module(d, mod, predicate);
        return WALK_NEXT;
    });
    d.mod_listen = d.core.os->listen_mod_create([=](proc_t mod_proc, mod_t mod)
    {
        if(d_->proc.id != mod_proc.id)
            return;

        load_module(*d_, mod, predicate);
    });
}

void sym::Loader::mod_listen()
{
    mod_listen([](mod_t, const std::string&) { return true; });
}

void sym::Loader::drv_listen(sym::drv_predicate_fn predicate)
{
    auto& d = *d_;
    d.core.os->driver_list([&](driver_t drv)
    {
        load_driver(d, drv, predicate);
        return WALK_NEXT;
    });
    d.drv_listen = d.core.os->listen_drv_create([=](driver_t drv, bool load)
    {
        if(load)
            load_driver(*d_, drv, predicate);
        else
        {
            // driver unloading todo...
        }
    });
}

void sym::Loader::drv_listen()
{
    drv_listen([](driver_t, const std::string&) { return true; });
}

sym::Loader::~Loader()
{
    auto& d = *d_;
    if(d.mod_listen)
        d.core.os->unlisten(*d.mod_listen);

    if(d.drv_listen)
        d.core.os->unlisten(*d.drv_listen);
}

bool sym::Loader::load(mod_t mod)
{
    return load_module(*d_, mod, [](mod_t, const std::string&) { return true; });
}

sym::Symbols& sym::Loader::symbols()
{
    return d_->symbols;
}
