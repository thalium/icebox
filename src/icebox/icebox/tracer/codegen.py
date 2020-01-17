#!/usr/bin/python3

import argparse
import json
import os

def generate_usings(json_data, pad):
    pad += len("on__fn")
    lines = []
    for target, (_, args) in json_data.items():
        types = [typeof for _, typeof in args]
        if len(types):
            types[0] = types[0]
        name = "on_%s_fn" % target
        args_list = ", "
        lines.append("    using %s = std::function<void(%s)>;" % (name.ljust(pad), args_list.join(types)))
    lines.sort()
    return "\n".join(lines)

def generate_registers(json_data, pad):
    lines = []
    for target, _ in json_data.items():
        name = ("register_%s" % target).ljust(pad + len("register_"))
        lines.append("        opt<bpid_t> %s(proc_t proc, const on_%s_fn& on_func);" % (name, target))
    lines.sort()
    return "\n".join(lines)

def generate_header(json_data, filename, namespace, pad, wow64):
    return """#pragma once

#include "icebox/nt/{namespace}.hpp"
#include "icebox/types.hpp"
#include "tracer.hpp"

#include <array>
#include <functional>

namespace core {{ struct Core; }}

namespace {namespace}
{{
{usings}

    struct {filename}
    {{
         {filename}(core::Core& core, std::string_view module);
        ~{filename}();

        using on_call_fn = std::function<void(const tracer::callcfg_t& callcfg)>;
        using callcfgs_t = std::array<tracer::callcfg_t, {size}>;

        opt<bpid_t>                 register_all(proc_t proc, const on_call_fn& on_call);
        static const callcfgs_t&    callcfgs    ();

{listens}

        struct Data;
        std::unique_ptr<Data> d_;
    }};
}} // namespace {namespace}
""".format(filename=filename, namespace=namespace,
        size=len(json_data.items()),
        usings=generate_usings(json_data, pad),
        listens=generate_registers(json_data, pad))

def generate_definitions(json_data, filename, namespace, wow64):
    definitions = ""
    callcfg_idx = -1
    items = [x for x in json_data.items()]
    items.sort(key=lambda x: x[0])
    for target, (_, (args)) in items:
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
    return register_callback(*d_, proc, "{symbol_name}", [=]
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
    lines = []
    for target, (_, (args)) in json_data.items():
        fn_name = target if not wow64 else "_{target}@{size}".format(target=target, size=len(args) * 4)
        args = ['{{"{type}", "{name}", sizeof({namespace}::{type})}}'.format(type=typeof, name=name, namespace=namespace) for name, typeof in args]
        elem = '        {{"{name}", {argc}, {{{keys}}}}},'.format(name=fn_name, argc=len(args), keys=", ".join(args))
        lines.append(elem)
    lines.sort()
    return "\n".join(lines)

def generate_impl(json_data, filename, namespace, pad, wow64):
    return """#include "{filename}.gen.hpp"

#define FDP_MODULE "{filename}"
#include "log.hpp"
#include "core.hpp"

#include <cstring>

namespace
{{
    constexpr bool g_debug = false;

    constexpr {namespace}::{filename}::callcfgs_t g_callcfgs =
    {{{{
{callers}
    }}}};
}}

struct {namespace}::{filename}::Data
{{
    Data(core::Core& core, std::string_view module);

    core::Core&   core;
    std::string   module;
}};

{namespace}::{filename}::Data::Data(core::Core& core, std::string_view module)
    : core(core)
    , module(module)
{{
}}

{namespace}::{filename}::{filename}(core::Core& core, std::string_view module)
    : d_(std::make_unique<Data>(core, module))
{{
}}

{namespace}::{filename}::~{filename}() = default;

const {namespace}::{filename}::callcfgs_t& {namespace}::{filename}::callcfgs()
{{
    return g_callcfgs;
}}

namespace
{{
    opt<bpid_t> register_callback_with({namespace}::{filename}::Data& d, bpid_t bpid, proc_t proc, const char* name, const state::Task& on_call)
    {{
        const auto addr = symbols::address(d.core, proc, d.module, name);
        if(!addr)
            return FAIL(ext::nullopt, "unable to find symbole %s!%s", d.module.data(), name);

        const auto bp = state::break_on_process(d.core, name, proc, *addr, on_call);
        if(!bp)
            return FAIL(ext::nullopt, "unable to set breakpoint");

        return state::save_breakpoint_with(d.core, bpid, bp);
    }}

    opt<bpid_t> register_callback({namespace}::{filename}::Data& d, proc_t proc, const char* name, const state::Task& on_call)
    {{
        const auto bpid = state::acquire_breakpoint_id(d.core);
        return register_callback_with(d, bpid, proc, name, on_call);
    }}

    template <typename T>
    T arg(core::Core& core, size_t i)
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
    auto& d         = *d_;
    const auto bpid = state::acquire_breakpoint_id(d.core);
    for(const auto cfg : g_callcfgs)
        register_callback_with(d, bpid, proc, cfg.name, [=]{{ on_call(cfg); }});
    return bpid;
}}
""".format(filename=filename, namespace=namespace,
        definitions=generate_definitions(json_data, filename, namespace, wow64),
        callers=generate_callers(json_data, namespace, wow64))

def read_file(filename):
    try:
        with open(filename, "rb") as fh:
            return fh.read().decode()
    except:
        return ""

def write_file(filename, data):
    current = read_file(filename)
    if current != data:
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
