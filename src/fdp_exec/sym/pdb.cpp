#include "sym.hpp"

#define FDP_MODULE "pdb"
#include "log.hpp"
#include "utils.hpp"

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include "pdbparser.hpp"
namespace pdb = retdec::pdbparser;

namespace
{
    static const int BASE_ADDRESS = 0x80000000;

    struct Pdb
        : public sym::IModule
    {
         Pdb(const fs::path& filename, span_t span);

        // methods
        bool setup();

        // IModule methods
        span_t                  get_span        () override;
        std::optional<uint64_t> get_symbol      (const std::string& symbol) override;
        std::optional<uint64_t> get_struc_offset(const std::string& struc, const std::string& member) override;

        // members
        const fs::path  filename_;
        const span_t    span_;
        pdb::PDBFile    pdb_;
    };
}

Pdb::Pdb(const fs::path& filename, span_t span)
    : filename_(filename)
    , span_(span)
{
}

std::unique_ptr<sym::IModule> sym::make_pdb(span_t span, const std::string& module, const std::string& guid)
{
    auto ptr = std::make_unique<Pdb>(fs::path(getenv("_NT_SYMBOL_PATH")) / module / guid / module, span);
    const auto ok = ptr->setup();
    if(!ok)
        return std::nullptr_t();

    return ptr;
}

namespace
{
    char* to_string(pdb::PDBFileState x)
    {
        switch(x)
        {
            case pdb::PDB_STATE_OK:                  return "ok";
            case pdb::PDB_STATE_ALREADY_LOADED:      return "already_loaded";
            case pdb::PDB_STATE_ERR_FILE_OPEN:       return "err_file_open";
            case pdb::PDB_STATE_INVALID_FILE:        return "invalid_file";
            case pdb::PDB_STATE_UNSUPPORTED_VERSION: return "unsupported_version";
        }
        return "<invalid>";
    }
}

bool Pdb::setup()
{
    const auto err = pdb_.load_pdb_file(filename_.generic_string().data());
    if(err != pdb::PDB_STATE_OK)
        FAIL(false, "unable to open pdb %s: %s", filename_.generic_string().data(), to_string(err));

    pdb_.initialize(BASE_ADDRESS);
    return true;
}

span_t Pdb::get_span()
{
    return span_;
}

std::optional<uint64_t> Pdb::get_symbol(const std::string& symbol)
{
    const auto globals = pdb_.get_global_variables();
    for(const auto& it : *globals)
        if(symbol == it.second.name)
            return span_.addr + it.second.address - BASE_ADDRESS;

    return std::nullopt;
}

std::optional<uint64_t> Pdb::get_struc_offset(const std::string& struc, const std::string& member)
{
    const auto& types = pdb_.get_types_container();
    const auto type = types->get_type_by_name(const_cast<char*>(struc.data()));
    if(!type)
        return std::nullopt;

    if(type->type_class != pdb::PDBTYPE_STRUCT)
        return std::nullopt;

    auto stype = reinterpret_cast<pdb::PDBTypeStruct*>(type);

    for(const auto& m : stype->struct_members)
        if(member == m->name)
            return m->offset;

    return std::nullopt;
}
