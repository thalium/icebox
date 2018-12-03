#!/usr/bin/python3

import argparse
import json
import os

# Strings for syscall_mon.gen.hpp
register_function = """
bool monitor::GenericMonitor::register_{function_name}(proc_t proc, const on_{function_name}_fn& on_{function_name_lc})
{{
    const auto ok = setup_func(proc, "{function_name}");
    if (!ok)
        FAIL(false, "Unable to setup bp");

    d_->observers_{function_name}.push_back(on_{function_name_lc});
    return true;
}}
"""

onevent_function = """
void monitor::GenericMonitor::on_{function_name}()
{{
    //LOG(INFO, "Break on {function_name}");
    constexpr int nargs = {nb_args};

    std::vector<arg_t> args;
    if constexpr(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) {{ args.push_back(arg); return WALK_NEXT; }});

    {retrieve_args}

    for(const auto& it : d_->observers_{function_name})
        it({args});
}}
"""
retrieve_arg = """const auto {name: <16}= nt::cast_to{type: <25}(args[{idx}]);"""

#Strings for syscall_macros.gen.hpp
handlers_macro = """
#define DECLARE_SYSCALLS_HANDLERS\\
    {handlers}
"""
handler = "{{\"{function_name}\", &monitor::GenericMonitor::on_{function_name}}},"

observers_macro = """
#define DECLARE_SYSCALLS_OBSERVERS\\
    {observers}
"""
observer = "std::vector<on_{function_name}_fn> observers_{function_name};"

onevent_callback_macro = """
#define DECLARE_SYSCALLS_CALLBACK_PROTOS\\
    {onevent_callback_protos}
"""
onevent_callback_proto = "using on_{function_name}_fn = std::function<{return_type}({arg_types})>;"

functions_protos_macro = """
#define DECLARE_SYSCALLS_FUNCTIONS_PROTOS\\
    {functions_protos}
"""
function_proto = "void on_{function_name: <38}();\\\n    bool register_{function_name: <32}(proc_t proc, const on_{function_name}_fn& on_{function_name_lc});"

def generate_model(json_data):
    callbacks  = ""
    handlers   = ""
    observers  = ""
    prototypes = ""
    privates   = ""

    first = True
    for key, value in json_data.items():
        args = value[1]
        data = {}
        data["function_name"]    = key
        data["function_name_lc"] = key.lower()
        data["return_type"]      = "nt::" + value[0]
        data["nb_args"]          = len(args)
        data["args"]             = ""
        data["arg_types"]        = ""
        data["retrieve_args"]    = ""
        for idx, arg in enumerate(args):
            if idx > 0:
                data["args"]          += ", "
                data["arg_types"]     += ", "
                data["retrieve_args"] += "\n    "
            data["args"]          += arg[0]
            data["arg_types"]     += "nt::" + arg[1]
            data["retrieve_args"] += retrieve_arg.format(name=arg[0], type="<nt::%s>" % arg[1], idx=idx)

        # write in <name>_private.gen.hpp
        privates += register_function.format(**data)
        privates += onevent_function.format(**data)

        if not first:
            callbacks  += "\\\n    "
            handlers   += "\\\n    "
            observers  += "\\\n    "
            prototypes += "\\\n    "
        first = False

        # fill variables that will go in macros
        callbacks  += onevent_callback_proto.format(**data)
        handlers   += handler.format(**data)
        observers  += observer.format(**data)
        prototypes += function_proto.format(**data)

    return callbacks, handlers, observers, prototypes, privates

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
    callbacks, handlers, observers, prototypes, privates = generate_model(json_data)

    write_file(private,
        "#pragma once\n\n#include \"generic_mon.hpp\"\n\n"
        + privates)
    write_file(public,
        "#pragma once\n\n#include \"nt/nt.hpp\"\n\n"
        + onevent_callback_macro.format(onevent_callback_protos=callbacks)
        + functions_protos_macro.format(functions_protos=prototypes)
        + observers_macro.format(observers=observers)
        + handlers_macro.format(handlers=handlers))
