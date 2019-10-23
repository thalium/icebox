#!/usr/bin/python3

import argparse
import json
import os

def generate_usings(json_data, pad):
    data = ""
    pad += len("on__fn")
    lines = []
    for target, (return_type, args) in json_data.items():
        types = [typeof for _, typeof in args]
        if len(types):
            types[0] = types[0]
        name = "on_%s_fn" % target
        args_list = ", "
        lines.append("    using %s = std::function<void(%s)>;" % (name.ljust(pad), args_list.join(types)))
    return data + "\n".join(lines)

def generate_registers(json_data, pad):
    data = ""
    lines = []
    for target, _ in json_data.items():
        name = ("register_%s" % target).ljust(pad + len("register_"))
        lines.append("        opt<bpid_t> %s(proc_t proc, const on_%s_fn& on_func);" % (name, target))
    return data + "\n".join(lines)

def generate_header(json_data, filename, namespace, pad, wow64):
    return """#pragma once

#include "icebox/nt/{namespace}.hpp"
#include "icebox/types.hpp"
#include "tracer.hpp"

#include <functional>

namespace core {{ struct Core; }}
namespace sym {{ struct Symbols; }}

namespace {namespace}
{{
{usings}

    struct {filename}
    {{
         {filename}(core::Core& core, sym::Symbols& syms, std::string_view module);
        ~{filename}();

        using on_call_fn = std::function<void(const tracer::callcfg_t& callcfg)>;
        using bpid_t     = uint64_t;

        opt<bpid_t> register_all(proc_t proc, const on_call_fn& on_call);
        bool        unregister  (bpid_t id);

{listens}

        struct Data;
        std::unique_ptr<Data> d_;
    }};
}} // namespace {namespace}
""".format(filename=filename, namespace=namespace,
        usings=generate_usings(json_data, pad),
        listens=generate_registers(json_data, pad))

def generate_definitions(json_data, filename, namespace, wow64):
    definitions = ""
    callcfg_idx = -1
    for target, (return_type, (args)) in json_data.items():
        callcfg_idx += 1
        symbol_name = target if not wow64 else "_{target}@{size}".format(target=target, size=len(args)*4)

        # print args
        pad = 0
        for name, typeof in args:
            pad = max(pad, len(name))
        idx = 0
        read_args = ""
        names = []
        for name, typeof in args:
            read_args += "\n        const auto %s = arg<%s::%s>(core, %d);" % (name.ljust(pad), namespace, typeof, idx)
            idx += 1
            names.append(name)
        if idx > 0:
            read_args += "\n"

        definitions += """
opt<bpid_t> {namespace}::{filename}::register_{target}(proc_t proc, const on_{target}_fn& on_func)
{{
    return register_callback(*d_, ++d_->last_id, proc, "{symbol_name}", [=]
    {{
        auto& core = d_->core;
        {read_args}
        if constexpr(g_debug)
            tracer::log_call(core, g_callcfgs[{callcfg_idx}]);

        on_func({names});
    }});
}}
""".format(filename=filename, namespace=namespace, target=target, symbol_name=symbol_name, read_args=read_args, callcfg_idx=callcfg_idx, names=", ".join(names))
    return definitions

def generate_callers(json_data, namespace, wow64):
    data = ""
    lines = []
    for target, (_, (args)) in json_data.items():
        name = target if not wow64 else "_{target}@{size}".format(target=target, size=len(args) * 4)
        args = ['{{"{type}", "{name}", sizeof({namespace}::{type})}}'.format(type=typeof, name=name, namespace=namespace) for name, typeof in args]
        elem = '        {{"{name}", {argc}, {{{keys}}}}},'.format(name=name, argc=len(args), keys=", ".join(args))
        lines.append(elem)
    return data + "\n".join(lines)

def generate_impl(json_data, filename, namespace, pad, wow64):
    return """#include "{filename}.gen.hpp"

#define FDP_MODULE "{filename}"
#include "log.hpp"
#include "core.hpp"

#include <cstring>
#include <map>

namespace
{{
	constexpr bool g_debug = false;

	static const tracer::callcfg_t g_callcfgs[] =
	{{
{callers}
	}};

    using bpid_t    = {namespace}::{filename}::bpid_t;
    using Listeners = std::multimap<bpid_t, state::Breakpoint>;
}}

struct {namespace}::{filename}::Data
{{
    Data(core::Core& core, sym::Symbols& syms, std::string_view module);

    core::Core&   core;
    sym::Symbols& syms;
    std::string   module;
    Listeners     listeners;
    bpid_t        last_id;
}};

{namespace}::{filename}::Data::Data(core::Core& core, sym::Symbols& syms, std::string_view module)
    : core(core)
    , syms(syms)
    , module(module)
    , last_id(0)
{{
}}

{namespace}::{filename}::{filename}(core::Core& core, sym::Symbols& syms, std::string_view module)
    : d_(std::make_unique<Data>(core, syms, module))
{{
}}

{namespace}::{filename}::~{filename}() = default;

namespace
{{
    static opt<bpid_t> register_callback({namespace}::{filename}::Data& d, bpid_t id, proc_t proc, const char* name, const state::Task& on_call)
    {{
        const auto addr = d.syms.symbol(d.module, name);
        if(!addr)
            return FAIL(ext::nullopt, "unable to find symbole %s!%s", d.module.data(), name);

        const auto bp = state::set_breakpoint(d.core, name, *addr, proc, on_call);
        if(!bp)
            return FAIL(ext::nullopt, "unable to set breakpoint");

        d.listeners.emplace(id, bp);
        return id;
    }}

    template <typename T>
    static T arg(core::Core& core, size_t i)
    {{
        const auto arg = functions::read_arg(core, i);
        if(!arg)
            return {{}};

        T value = {{}};
        static_assert(sizeof value <= sizeof arg->val, "invalid size");
        memcpy(&value, &arg->val, sizeof value);
        return value;
    }}
}}
{definitions}
opt<bpid_t> {namespace}::{filename}::register_all(proc_t proc, const {namespace}::{filename}::on_call_fn& on_call)
{{
    const auto id   = ++d_->last_id;
    const auto size = d_->listeners.size();
    for(const auto cfg : g_callcfgs)
        register_callback(*d_, id, proc, cfg.name, [=]{{ on_call(cfg); }});

    if(size == d_->listeners.size())
        return {{}};

    return id;
}}

bool {namespace}::{filename}::unregister(bpid_t id)
{{
    return d_->listeners.erase(id) > 0;
}}
""".format(filename=filename, namespace=namespace,
        definitions=generate_definitions(json_data, filename, namespace, wow64),
        callers=generate_callers(json_data, namespace, wow64))

def read_file(filename):
    with open(filename, "rb") as fh:
        return fh.read()

def write_file(filename, data):
    with open(filename, "wb") as fh:
        fh.write(data.encode("utf-8"))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='auto-generate tracer code')
    parser.add_argument('-i', '--input', type=os.path.abspath, help='input json', required=True)
    parser.add_argument('-n', '--namespace', type=str, help='input namespace', required=True)
    parser.add_argument('-w', '--wow64', action='store_true', help='generate wow64 verion')
    opts = parser.parse_args()

    filename = os.path.basename(opts.input)
    filedir = os.path.dirname(opts.input)

    json_data = json.loads(read_file(opts.input))
    pad = 0
    for target, (rettype, args) in json_data.items():
        pad = max(pad, len(target))

    filename = filename.replace(".json", "")

    header = os.path.join(filedir, filename + ".gen.hpp")
    write_file(header, generate_header(json_data, filename, opts.namespace, pad, opts.wow64))
    implementation = os.path.join(filedir, filename + ".gen.cpp")
    write_file(implementation, generate_impl(json_data, filename, opts.namespace, pad, opts.wow64))
