#include "sym.hpp"

#define FDP_MODULE "dwarf"
#include "endian.hpp"
#include "log.hpp"
#include "utils/fnview.hpp"
#include "utils/hex.hpp"
#include "utils/utils.hpp"

#include <libdwarf.h>

/*#include <fcntl.h>
#include <sys/stat.h>*/
#include <sys/types.h>

namespace
{
    struct Dwarf
        : public sym::IMod
    {
         Dwarf(fs::path filename, span_t span);
        ~Dwarf();

        // methods
        bool setup();

        // IModule methods
        span_t              span        () override;
        opt<uint64_t>       symbol      (const std::string& symbol) override;
        opt<uint64_t>       struc_offset(const std::string& struc, const std::string& member) override;
        opt<size_t>         struc_size  (const std::string& struc) override;
        opt<sym::ModCursor> symbol      (uint64_t addr) override;
        bool                sym_list    (sym::on_sym_fn on_sym) override;

        // members
        const fs::path filename_;
        const span_t   span_;
        Dwarf_Unsigned dwarf_;
        Dwarf_Debug dbg = nullptr;
        Dwarf_Error err = nullptr;
    };
}

Dwarf::Dwarf(fs::path filename, span_t span)
    : filename_(std::move(filename))
    , span_(span)
{
}

Dwarf::~Dwarf()
{
    if(dwarf_finish(dbg, &err) != DW_DLV_OK)
    {
        LOG(ERROR, "unable to free dwarf ressources ({}) : {}", dwarf_errno(err), dwarf_errmsg(err));
    }
}

std::unique_ptr<sym::IMod> sym::make_dwarf(span_t span, const void* /*data*/, const size_t /*data_size*/)
{
    const auto path = getenv("_LINUX_SYMBOL_PATH");
    if(!path)
        return nullptr;

    auto ptr      = std::make_unique<Dwarf>(fs::path(path) / "kernel.ko", span);
    const auto ok = ptr->setup();
    if(!ok)
        return nullptr;

    return ptr;
}

bool Dwarf::setup()
{
    const auto ok = dwarf_init_path(
        filename_.generic_string().data(), // path
        nullptr,                           // true_path_out_buffer
        NULL,                              // true_path_bufferlen
        DW_DLC_READ,                       // access
        DW_GROUPNUMBER_ANY,                // groupnumber
        NULL,                              // errhand
        nullptr,                           // errarg
        &dbg,                              // ret_dbg
        nullptr,                           // reserved1
        NULL,                              // reserved2
        nullptr,                           // reserved3
        &err                               // error
    );

    if(ok != DW_DLV_OK)
    {
        if(ok == DW_DLV_NO_ENTRY)
            LOG(ERROR, "unfound file or dwarf information in file '{}'", filename_.generic_string());
        else
            LOG(ERROR, "libdwarf error {} when initializing dwarf file : {}", dwarf_errno(err), dwarf_errmsg(err));
        return false;
    }

    return true;
}

span_t Dwarf::span()
{
    return span_;
}

opt<uint64_t> Dwarf::symbol(const std::string& /*symbol*/)
{
    return {};
}

bool Dwarf::sym_list(sym::on_sym_fn /*on_sym*/)
{
    return false;
}

opt<uint64_t> Dwarf::struc_offset(const std::string& /*struc*/, const std::string& /*member*/)
{
    return {};
}

opt<size_t> Dwarf::struc_size(const std::string& /*struc*/)
{
    return {};
}

opt<sym::ModCursor> Dwarf::symbol(uint64_t /*addr*/)
{
    return {};
}
