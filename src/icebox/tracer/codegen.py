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
        lines.append("    using %s = std::function<%s(%s)>;" % (name.ljust(pad), return_type, args_list.join(types)))
    return data + "\n".join(lines)

def generate_registers(json_data, pad):
    data = ""
    lines = []
    for target, _ in json_data.items():
        name = ("register_%s" % target).ljust(pad + len("register_"))
        lines.append("        bool %s(proc_t proc, const on_%s_fn& on_func);" % (name, target))
    return data + "\n".join(lines)

def generate_header(json_data, filename, namespace, pad, wow64):
    return """#pragma once

#include "core.hpp"
#include "types.hpp"

#include "nt/nt.hpp"
#include "nt/{namespace}.hpp"

#include <functional>

namespace {namespace}
{{
{usings}

    struct argcfg_t
    {{
        char	 type[64];
        char	 name[64];
    }};

    struct callcfg_t
    {{
        char        name[64];
        size_t      argc;
        argcfg_t    args[32];
    }};

    struct {filename}
    {{
         {filename}(core::Core& core, std::string_view module);
        ~{filename}();

        // register generic callback with process filtering
        using on_call_fn = std::function<void(const callcfg_t& callcfg)>;
        bool register_all(proc_t proc, const on_call_fn& on_call);

{registers}

        struct Data;
        std::unique_ptr<Data> d_;
    }};
}} // namespace {namespace}
""".format(filename=filename, namespace=namespace,
        usings=generate_usings(json_data, pad),
        registers=generate_registers(json_data, pad),
		ptr_t = "x86_t" if wow64 else "x64_t")

def generate_enumerates(json_data):
    data = ""
    lines = []
    for target, _ in json_data.items():
        lines.append("        %s," % target)
    return data + "\n".join(lines)

def generate_methods(json_data):
    data = ""
    lines = []
    for target, _ in json_data.items():
        lines.append("    void on_%s();" % target)
    return data + "\n".join(lines)

def generate_observers(json_data, pad):
    data = ""
    lines = []
    for target, _ in json_data.items():
        on = ("on_%s_fn>" % target).ljust(pad + len("on__fn>"))
        lines.append("    std::vector<%s observers_%s;" % (on, target))
    return data + "\n".join(lines)

def generate_dispatchers(json_data, filename, namespace):
    dispatchers = ""
    for target, (return_type, (args)) in json_data.items():
        # print prologue
        dispatchers += """
    static void on_{target}({namespace}::{filename}::Data& d)
    {{""".format(filename=filename, namespace=namespace, target=target)

        # print args
        pad = 0
        for name, typeof in args:
            pad = max(pad, len(name))
        idx = 0
        lines = []
        names = []
        formats = []
        for name, typeof in args:
            dispatchers += "\n        const auto %s = arg<%s::%s>(d.core, %d);" % (name.ljust(pad), namespace, typeof, idx)
            idx += 1
            names.append(name)
            formats.append("%s:{:#x}" % name)
        if idx > 0:
            dispatchers += "\n"

        # print epilogue
        logargs = ", " if len(names) else ""
        dispatchers += """
        if constexpr(g_debug)
            logg::print(logg::level_t::info, fmt::format("{target}({fmtargs})"{logargs}));

        for(const auto& it : d.observers_{target})
            it({args});
    }}
""".format(target=target, args=", ".join(names), fmtargs=", ".join(formats), logargs=logargs + ", ".join(names))
    return dispatchers

def generate_definitions(json_data, filename, namespace, wow64):
    definitions = ""
    for target, (return_type, (args)) in json_data.items():
        symbol_name = target if not wow64 else "_{target}@{size}".format(target=target, size=len(args)*4)
        # print prologue
        definitions += """
bool {namespace}::{filename}::register_{target}(proc_t proc, const on_{target}_fn& on_func)
{{
    if(d_->observers_{target}.empty())
        if(!register_callback_with(*d_, proc, "{symbol_name}", &on_{target}))
            return false;

    d_->observers_{target}.push_back(on_func);
    return true;
}}
""".format(filename=filename, namespace=namespace, target=target, symbol_name=symbol_name)
    return definitions

def generate_callers(json_data, wow64):
    data = ""
    lines = []
    for target, (_, (args)) in json_data.items():
        name = target if not wow64 else "_{target}@{size}".format(target=target, size=len(args) * 4)
        args = ['{{"{type}", "{name}"}}'.format(type=typeof, name=name) for name, typeof in args]
        elem = '        {{"{name}", {argc}, {{{keys}}}}},'.format(name=name, argc=len(args), keys=", ".join(args))
        lines.append(elem)
    return data + "\n".join(lines)

def generate_impl(json_data, filename, namespace, pad, wow64):
    return """#include "{filename}.gen.hpp"

#define FDP_MODULE "{filename}"
#include "log.hpp"
#include "os.hpp"

namespace
{{
	constexpr bool g_debug = false;

	static const {namespace}::callcfg_t g_callcfgs[] =
	{{
{callers}
	}};
}}

struct {namespace}::{filename}::Data
{{
    Data(core::Core& core, std::string_view module);

    using Breakpoints = std::vector<core::Breakpoint>;
    core::Core& core;
    std::string module;
    Breakpoints breakpoints;

{observers}
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

namespace
{{
    using Data = {namespace}::{filename}::Data;

    static core::Breakpoint register_callback(Data& d, proc_t proc, const char* name, const core::Task& on_call)
    {{
        const auto addr = d.core.sym.symbol(d.module, name);
        if(!addr)
            return FAIL(nullptr, "unable to find symbole {{}}!{{}}", d.module.data(), name);

        return d.core.state.set_breakpoint(*addr, proc, on_call);
    }}

    static bool register_callback_with(Data& d, proc_t proc, const char* name, void (*callback)(Data&))
    {{
        const auto dptr = &d;
        const auto bp = register_callback(d, proc, name, [=]
        {{
            callback(*dptr);
        }});
        if(!bp)
            return false;

        d.breakpoints.emplace_back(bp);
        return true;
    }}

    template <typename T>
    static T arg(core::Core& core, size_t i)
    {{
        const auto arg = core.os->read_arg(i);
        if(!arg)
            return {{}};

        return {namespace}::cast_to<T>(*arg);
    }}
{dispatchers}
}}

{definitions}

bool {namespace}::{filename}::register_all(proc_t proc, const {namespace}::{filename}::on_call_fn& on_call)
{{
    Data::Breakpoints breakpoints;
    for(const auto cfg : g_callcfgs)
        if(const auto bp = register_callback(*d_, proc, cfg.name, [=]{{ on_call(cfg); }}))
            breakpoints.emplace_back(bp);

    d_->breakpoints.insert(d_->breakpoints.end(), breakpoints.begin(), breakpoints.end());
    return true;
}}
""".format(filename=filename, namespace=namespace,
        enumerates=generate_enumerates(json_data),
        observers=generate_observers(json_data, pad),
        dispatchers=generate_dispatchers(json_data, filename, namespace),
        definitions=generate_definitions(json_data, filename, namespace, wow64),
        callers=generate_callers(json_data, wow64))

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
