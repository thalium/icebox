#!/usr/bin/python3

import json
import argparse

# Strings for syscall_mon.gen.hpp
register_function = """
void monitor::GenericMonitor::register_{function_name}(const on_{function_name}_fn& on_{function_name_lc})
{{
    d_->observers_{function_name}.push_back(on_{function_name_lc});
}}
"""

onevent_function = """
void monitor::GenericMonitor::on_{function_name}()
{{
    LOG(INFO, "Break on {function_name}");
    const auto nargs = {nbr_args};

    std::vector<arg_t> args;
    if(nargs > 0)
        get_raw_args(nargs, [&](arg_t arg) {{ args.push_back(arg); return WALK_NEXT; }});

    {retreive_args}

    for(const auto& it : d_->observers_{function_name})
    {{
        it({args});
    }}
}}
"""
retreive_arg = """const auto {name: <16}= nt::cast_to{type: <25}(args[{idx}]);"""

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
onevent_callback_proto = "using on_{function_name}_fn = std::function<{return_type}({args_types})>;"

functions_protos_macro = """
#define DECLARE_SYSCALLS_FUNCTIONS_PROTOS\\
    {functions_protos}
"""
function_proto = "void on_{function_name: <38}();\\\n    void register_{function_name: <32}(const on_{function_name}_fn& on_{function_name_lc});"

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Trace alloc and free on heap.')
    parser.add_argument('-o', '--output-dir', help='Path to dir of output files', required=True)
    parser.add_argument('-i', '--input-dir', help='Path to dir of inupt files', required=True)
    script_args = parser.parse_args()

    #path_gen_files = "/home/user/work/WinbagFRP/src/fdp_exec/monitor/"
    input_json_name              = "syscalls.json"
    syscall_mon_private_gen_name = "syscall_mon_private.gen.hpp"
    syscall_mon_public_gen_name  = "syscall_mon_public.gen.hpp"

    #Open output files
    syscall_mon_private_gen = open(script_args.output_dir+'/'+syscall_mon_private_gen_name, "w")
    syscall_mon_private_gen.write("#pragma once\n\n#include \"generic_mon.hpp\"\n\n")

    syscall_mon_public_gen  = open(script_args.output_dir+'/'+syscall_mon_public_gen_name, "w")
    syscall_mon_public_gen.write("#pragma once\n\n#include \"nt/nt.hpp\"\n\n")

    #Read json that contains datas
    f = open(script_args.input_dir+'/'+input_json_name)
    sc_json = json.loads(f.read())
    f.close()

    onevent_callback_protos = ""
    handlers = ""
    observers = ""
    functions_protos = ""

    l = len(sc_json.items())
    j = 0
    for key, value in sc_json.items():
        #Fill data dict
        data = {}

        data["function_name"] = key
        data["function_name_lc"] = key.lower()
        data["return_type"] = "nt::" + value[0]

        args = value[1]
        data["nbr_args"] = len(args)

        data["args"]          = ""
        data["args_types"]    = ""
        data["retreive_args"] = ""
        for idx, arg in enumerate(args):
            data["args"] += arg[0]
            data["args_types"] += "nt::" + arg[1]
            data["retreive_args"] += retreive_arg.format(name=arg[0], type="<nt::"+arg[1]+">", idx=idx)

            if idx == len(args)-1:
                break

            data["args"] += ", "
            data["args_types"] += ", "
            data["retreive_args"] += "\n    "

        #Write in syscall_mon.gen.hpp
        syscall_mon_private_gen.write(register_function.format(**data))
        syscall_mon_private_gen.write(onevent_function.format(**data))

        #Fill variables that will go in macros
        onevent_callback_protos += onevent_callback_proto.format(**data)
        handlers                += handler.format(**data)
        observers               += observer.format(**data)
        functions_protos        += function_proto.format(**data)

        if j == l-1:
            break

        onevent_callback_protos += "\\\n    "
        handlers                += "\\\n    "
        observers               += "\\\n    "
        functions_protos        += "\\\n    "
        j+=1

    #Write in syscall_mon_public.gen.hpp
    syscall_mon_public_gen.write(onevent_callback_macro.format(onevent_callback_protos=onevent_callback_protos))
    syscall_mon_public_gen.write(functions_protos_macro.format(functions_protos=functions_protos))
    syscall_mon_public_gen.write(observers_macro.format(observers=observers))
    syscall_mon_public_gen.write(handlers_macro.format(handlers=handlers))
