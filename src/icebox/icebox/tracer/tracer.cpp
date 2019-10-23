#include "tracer.hpp"

#define FDP_MODULE "tracer"
#include "core.hpp"
#include "log.hpp"
#include "os.hpp"
#include "types.hpp"

#include <fmt/format.h>

#include <vector>

namespace
{
    static uint64_t read_arg(core::Core& core, size_t i, size_t size)
    {
        const auto arg = os::read_arg(core, i);
        if(!arg)
            return {};

        return arg->val & (~uint64_t(0) >> (64 - size * 8));
    }

    static std::string join(const std::vector<std::string>& args, const std::string& sep)
    {
        std::string reply;
        if(args.empty())
            return reply;

        for(const auto& arg : args)
            reply += arg + sep;
        reply.resize(reply.size() - sep.size());

        return reply;
    }
}

void tracer::log_call(core::Core& core, const tracer::callcfg_t& call)
{
    std::vector<std::string> args;
    args.reserve(call.argc);
    for(size_t i = 0; i < call.argc; ++i)
        args.emplace_back(fmt::format("{}:{:#x}", call.args[i].name, read_arg(core, i, call.args[i].size)));
    LOG(INFO, "%s(%s)", call.name, join(args, ", ").data());
}
