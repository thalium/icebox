import json

# Strings for syscall_mon.gen.hpp
register_function = """
void syscall_mon::SyscallMonitor::register_{function_name}(const on_{function_name}& on_{function_name_lc})
{{
    d_->{function_name}_observers.push_back(on_{function_name_lc});
}}
"""

onevent_function = """
void syscall_mon::SyscallMonitor::On_{function_name}()
{{
    const auto nargs = {nbr_args};

    std::vector<arg_t> args;
    get_raw_args(nargs, [&](arg_t arg) {{ args.push_back(arg); return WALK_NEXT; }});

    {retreive_args}

    for(const auto it : d_->{function_name}_observers)
    {{
        it({args});
    }}
}}
"""
retreive_arg = """const auto {name: <16}= nt::cast_to{type: <25}(args[{idx}]);"""

#Strings for syscall_macros.gen.hpp
handlers_macro = """
#define DECLARE_HANDLERS\\
    {handlers}
"""
handler = "{{\"{function_name}\", &syscall_mon::SyscallMonitor::On_{function_name}}},"

observers_macro = """
#define DECLARE_OBSERVERS\\
    {observers}
"""
observer = "std::vector<on_{function_name}> {function_name}_observers;"

onevent_callback_macro = """
#define DECLARE_CALLBACK_PROTOS\\
    {onevent_callback_protos}
"""
onevent_callback_proto = "using on_{function_name: <16} = std::function<{return_type}({args_types})>;"


path_gen_files = "/home/user/work/WinbagFRP/src/fdp_exec/monitor/"
syscall_mon_gen_name = "syscall_mon.gen.hpp"
syscall_macros_gen_name = "syscall_macros.gen.hpp"

#Open output files
syscall_mon_gen = open(path_gen_files+syscall_mon_gen_name, "w")
syscall_mon_gen.write("#pragma once\n\n#include \"syscall_mon.hpp\"\n\n")

syscall_macros_gen = open(path_gen_files+syscall_macros_gen_name, "w")
syscall_macros_gen.write("#pragma once\n\n#include \"nt/nt.hpp\"\n\n")

#Read json that contains datas
f = open("syscalls.json")
sc_json = json.loads(f.read())
f.close()

onevent_callback_protos = ""
handlers = ""
observers = ""

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

        data["args"] += ","
        data["args_types"] += ","
        data["retreive_args"] += "\n    "

    #Write in syscall_mon.gen.hpp
    syscall_mon_gen.write(register_function.format(**data))
    syscall_mon_gen.write(onevent_function.format(**data))

    #Fill variables that will go in macros
    onevent_callback_protos += onevent_callback_proto.format(**data)
    handlers                += handler.format(**data)
    observers               += observer.format(**data)

    if j == l-1:
        break

    onevent_callback_protos += "\\\n    "
    handlers                += "\\\n    "
    observers               += "\\\n    "
    j+=1

#Write in syscall_macros.gen.hpp
syscall_macros_gen.write(onevent_callback_macro.format(onevent_callback_protos=onevent_callback_protos))
syscall_macros_gen.write(observers_macro.format(observers=observers))
syscall_macros_gen.write(handlers_macro.format(handlers=handlers))
