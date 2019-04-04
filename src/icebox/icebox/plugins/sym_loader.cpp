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
    Data(core::Core& core, proc_t proc, sym::predicate_fn predicate);

    core::Core&          core;
    sym::Symbols         symbols;
    proc_t               proc;
    sym::predicate_fn    predicate;
    std::vector<uint8_t> buffer;
    reader::Reader       reader;
    opt<size_t>          bp_id;
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

    static bool load_module_named(Data& d, mod_t mod, const std::string& name)
    {
        LOG(INFO, "loading module {}", name);
        const auto span = d.core.os->mod_span(d.proc, mod);
        if(!span)
            return false;

        LOG(INFO, "{} loaded at {:x}:{:x}", name, span->addr, span->addr + span->size);
        const auto loaded = load_module_buffer(d, *span);
        if(!loaded)
            return false;

        const auto filename = path::filename(name).replace_extension();
        const auto inserted = d.symbols.insert(filename.generic_string(), *span, &d.buffer[0], d.buffer.size());
        return inserted;
    }

    static void load_module(Data& d, mod_t mod)
    {
        const auto name = d.core.os->mod_name(d.proc, mod);
        if(!name)
            return;

        if(!d.predicate(mod, *name))
            return;

        const auto loaded = load_module_named(d, mod, *name);
        if(!loaded)
            LOG(ERROR, "unable to load symbols from {}", *name);
    }
}

Data::Data(core::Core& core, proc_t proc, sym::predicate_fn predicate)
    : core(core)
    , proc(proc)
    , predicate(std::move(predicate))
    , reader(reader::make(core, proc))
{
}

sym::Loader::Loader(core::Core& core, proc_t proc, sym::predicate_fn predicate)
    : d_(std::make_unique<Data>(core, proc, predicate))
{
    auto& d = *d_;
    d.core.os->mod_list(d.proc, [&](mod_t mod)
    {
        load_module(d, mod);
        return WALK_NEXT;
    });
    d.bp_id = d.core.os->listen_mod_create([=](proc_t mod_proc, mod_t mod)
    {
        if(d_->proc.id != mod_proc.id)
            return;

        load_module(*d_, mod);
    });
}

sym::Loader::Loader(core::Core& core, proc_t proc)
    : Loader(core, proc, [=](mod_t, const std::string&) { return true; })
{
}

sym::Loader::~Loader()
{
    auto& d = *d_;
    if(d.bp_id)
        d.core.os->unlisten(*d.bp_id);
}

sym::Symbols& sym::Loader::symbols()
{
    return d_->symbols;
}
