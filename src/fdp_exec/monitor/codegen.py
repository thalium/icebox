#!/usr/bin/python3

import argparse
import json
import os

def generate_definitions(json_data):
    definitions = ""
    for target, (return_type, (args)) in json_data.items():
        # print prologue
        definitions += """
bool monitor::GenericMonitor::register_{target}(proc_t proc, const on_{target}_fn& on_func)
{{
    const auto ok = setup_func(proc, "{target}");
    if(!ok)
        FAIL(false, "unable to register {target}");

    d_->observers_{target}.push_back(on_func);
    return true;
}}

void monitor::GenericMonitor::on_{target}()
{{
    if(false)
        LOG(INFO, "break on {target}");
""".format(target=target)

        # print args
        pad = 0
        for name, typeof in args:
            pad = max(pad, len(name))
        idx = 0
        lines = []
        names = []
        for name, typeof in args:
            definitions += "\n    const auto %s = arg<nt::%s>(core_, %d);" % (name.ljust(pad), typeof, idx)
            idx += 1
            names.append(name)
        if idx > 0:
            definitions += "\n"

        # print epilogue
        definitions += """
    for(const auto& it : d_->observers_{target})
        it({args});
}}
""".format(target=target, args=", ".join(names))
    return definitions

def generate_callbacks(json_data, pad):
    data = """
#define DECLARE_SYSCALLS_CALLBACK_PROTOS\\
"""
    pad += len("on__fn")
    lines = []
    for target, (return_type, args) in json_data.items():
        types = [typeof for _, typeof in args]
        if len(types):
            types[0] = "nt::" + types[0]
        name = "on_%s_fn" % target
        lines.append("    using %s = std::function<nt::%s(%s)>;" % (name.ljust(pad), return_type, ", nt::".join(types)))
    return data + "\\\n".join(lines) + "\n"

def generate_prototypes(json_data, pad):
    data = """
#define DECLARE_SYSCALLS_FUNCTIONS_PROTOS\\
"""
    lines = []
    lines_ = []
    for target, _ in json_data.items():
        on = ("on_%s" % target).ljust(pad + len("on_"))
        reg = ("register_%s" % target).ljust(pad + len("register_"))
        lines.append("    void %s();" % on)
        lines_.append("    bool %s(proc_t proc, const on_%s_fn& on_func);" % (reg, target))
    return data + "\\\n".join(lines) + "\\\n" + "\\\n".join(lines_) + "\n"

def generate_observers(json_data, pad):
    data = """
#define DECLARE_SYSCALLS_OBSERVERS\\
"""
    lines = []
    for target, _ in json_data.items():
        on = ("on_%s_fn>" % target).ljust(pad + len("on__fn>"))
        lines.append("    std::vector<%s observers_%s;" % (on, target))
    return data + "\\\n".join(lines) + "\n"

def generate_handlers(json_data, pad):
    data = """
#define DECLARE_SYSCALLS_HANDLERS\\
"""
    lines = []
    for target, _ in json_data.items():
        name = ("%s\"," % target).ljust(pad + len("\","))
        lines.append("    {\"%s &monitor::GenericMonitor::on_%s}," % (name, target))
    return data + "\\\n".join(lines) + "\n"

def read_file(filename):
    with open(filename, "rb") as fh:
        return fh.read()

def write_file(filename, data):
    with open(filename, "wb") as fh:
        fh.write(data.encode("utf-8"))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='auto-generate tracer code')
    parser.add_argument('-i', '--input', type=os.path.abspath, help='input json', required=True)
    args = parser.parse_args()

    filename = os.path.basename(args.input)
    filedir = os.path.dirname(args.input)
    private = os.path.join(filedir, filename.replace(".json", "_private.gen.hpp"))
    public  = os.path.join(filedir, filename.replace(".json", "_public.gen.hpp"))

    json_data = json.loads(read_file(args.input))

    write_file(private,
        "#pragma once\n\n#include \"generic_mon.hpp\"\n\n"
        + generate_definitions(json_data))

    pad = 0
    for target, (rettype, args) in json_data.items():
        pad = max(pad, len(target))
    write_file(public,
        "#pragma once\n\n#include \"nt/nt.hpp\"\n\n"
        + generate_callbacks(json_data, pad)
        + generate_prototypes(json_data, pad)
        + generate_observers(json_data, pad)
        + generate_handlers(json_data, pad))
