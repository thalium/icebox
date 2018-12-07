#include "sym.hpp"

#define FDP_MODULE "pdb"
#include "log.hpp"

namespace
{
    struct EmptyMod
        : public sym::IMod
    {
        EmptyMod(span_t span);

        // IModule methods
        span_t              span        () override;
        opt<uint64_t>       symbol      (const std::string& symbol) override;
        opt<uint64_t>       struc_offset(const std::string& struc, const std::string& member) override;
        opt<size_t>         struc_size  (const std::string& struc) override;
        opt<sym::ModCursor> symbol      (uint64_t addr) override;
        bool                sym_list    (const sym::on_sym_fn& on_sym) override;

        // members
        const span_t span_;
    };
}

EmptyMod::EmptyMod(span_t span)
    : span_(span)
{
}

std::unique_ptr<sym::IMod> sym::make_empty(span_t span, const void* /*data*/, const size_t /*data_size*/)
{
    return std::make_unique<EmptyMod>(span);
}

span_t EmptyMod::span()
{
    return span_;
}

opt<uint64_t> EmptyMod::symbol(const std::string& /*symbol*/)
{
    return {};
}

bool EmptyMod::sym_list(const sym::on_sym_fn& on_sym)
{
    on_sym("_", span_.addr);
    return false;
}

opt<uint64_t> EmptyMod::struc_offset(const std::string& /*struc*/, const std::string& /*member*/)
{
    return {};
}

opt<size_t> EmptyMod::struc_size(const std::string& /*struc*/)
{
    return {};
}

opt<sym::ModCursor> EmptyMod::symbol(uint64_t addr)
{
    if(span_.addr <= addr && addr < span_.addr + span_.size)
        return sym::ModCursor{"_", addr - span_.addr};

    return {};
}
