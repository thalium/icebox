#include "os.hpp"

#define PRIVATE_CORE__
#define FDP_MODULE "function"
#include "core.hpp"
#include "core_private.hpp"
#include "os_private.hpp"

opt<arg_t> function::read_stack(core::Core& core, size_t index)
{
    return core.os_->read_stack(index);
}

opt<arg_t> function::read_arg(core::Core& core, size_t index)
{
    return core.os_->read_arg(index);
}

bool function::write_arg(core::Core& core, size_t index, arg_t arg)
{
    return core.os_->write_arg(index, arg);
}
