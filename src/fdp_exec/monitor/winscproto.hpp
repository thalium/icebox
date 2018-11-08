#pragma once

#include "memory.hpp"
#include "types.hpp"
#include "enums.hpp"

#include <vector>

namespace winscproto
{
    struct win_arg_t
    {
        const char* name;
        win_arg_direction_e dir;
        win_type_e type;
        const char* dir_opt;
    };

    struct win_syscall_t
    {
        const char* name;
        win_type_e ret;
        unsigned int num_args;
        std::vector<win_arg_t> args;
    };

    struct WinScProto
    {
         WinScProto();
        ~WinScProto();

        bool setup();
        opt<win_syscall_t> find(std::string name);

        struct Data;
        std::unique_ptr<Data> d_;
    };
}
