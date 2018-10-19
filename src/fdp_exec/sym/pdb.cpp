#include "sym.hpp"

#define FDP_MODULE "pdb"
#include "log.hpp"
#include "utils/utils.hpp"
#include "endian.hpp"

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

#include "pdbparser.hpp"
namespace pdb = retdec::pdbparser;

namespace
{
    static const int BASE_ADDRESS = 0x80000000;

    using Symbols = std::unordered_map<std::string, pdb::PDBGlobalVariable>;

    struct Pdb
        : public sym::IMod
    {
         Pdb(const fs::path& filename, span_t span);

        // methods
        bool setup();

        // IModule methods
        span_t          span        () override;
        opt<uint64_t>   symbol      (const std::string& symbol) override;
        opt<uint64_t>   struc_offset(const std::string& struc, const std::string& member) override;

        // members
        const fs::path  filename_;
        const span_t    span_;
        pdb::PDBFile    pdb_;
        Symbols         symbols_;
    };
}

Pdb::Pdb(const fs::path& filename, span_t span)
    : filename_(filename)
    , span_(span)
{
}

std::unique_ptr<sym::IMod> sym::make_pdb(span_t span, const std::string& module, const std::string& guid)
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
    const auto globals = pdb_.get_global_variables();
    symbols_.reserve(globals->size());
    for(const auto& it : *globals)
        symbols_.emplace(it.second.name, it.second);
    return true;
}

span_t Pdb::span()
{
    return span_;
}

opt<uint64_t> Pdb::symbol(const std::string& symbol)
{
    const auto it = symbols_.find(symbol);
    if(it == symbols_.end())
        return std::nullopt;

    return span_.addr + it->second.address - BASE_ADDRESS;
}

opt<uint64_t> Pdb::struc_offset(const std::string& struc, const std::string& member)
{
    const auto type = pdb_.get_types_container()->get_type_by_name(const_cast<char*>(struc.data()));
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

namespace
{
    struct PdbCtx
    {
        std::string guid;
        std::string name;
    };

    void binhex(char* dst, const void* vsrc, size_t size)
    {
        static const char hexchars_upper[] = "0123456789ABCDEF";
        const uint8_t* src = static_cast<const uint8_t*>(vsrc);
        for(size_t i = 0; i < size; ++i)
        {
            dst[i * 2 + 0] = hexchars_upper[src[i] >> 4];
            dst[i * 2 + 1] = hexchars_upper[src[i] & 0x0F];
        }
    }

    opt<PdbCtx> read_pdb(const void* vsrc, size_t src_size)
    {
        const auto src = reinterpret_cast<const uint8_t*>(vsrc);
        std::vector<uint8_t> magic = {'R', 'S', 'D', 'S'};
        const auto rsds = std::search(&src[0], &src[src_size], std::boyer_moore_horspool_searcher(magic.begin(), magic.end()));
        if(!rsds)
            FAIL(std::nullopt, "unable to find RSDS pattern into kernel module");

        const auto size = std::distance(rsds, &src[src_size]);
        if(size < 4 /*magic*/ + 16 /*guid*/ + 4 /*age*/ + 2 /*name*/)
            FAIL(std::nullopt, "kernel module is too small for pdb header");

        const auto end = reinterpret_cast<const uint8_t*>(memchr(&rsds[4 + 16 + 4], 0x00, size));
        if(!end)
            FAIL(std::nullopt, "missing null-terminating byte on PDB header module name");

        uint8_t guid[16];
        write_be32(&guid[0], read_le32(&rsds[4 + 0])); // Data1
        write_be16(&guid[4], read_le16(&rsds[4 + 4])); // Data2
        write_be16(&guid[6], read_le16(&rsds[4 + 6])); // Data3
        memcpy(&guid[8], &rsds[4 + 8], 8);             // Data4

        char strguid[sizeof guid * 2];
        binhex(strguid, &guid, sizeof guid);

        uint32_t age = read_le32(&rsds[4 + 16]);
        const auto name = &rsds[4 + 16 + 4];
        const auto strname = std::string{reinterpret_cast<const char*>(name), reinterpret_cast<const char*>(end)};
        return PdbCtx{std::string{strguid, sizeof strguid} +std::to_string(age), strname};
    }
}

std::unique_ptr<sym::IMod> sym::make_pdb(span_t span, const void* data)
{
    const auto pdb = read_pdb(data, span.size);
    if(!pdb)
        return nullptr;

    LOG(INFO, "%s %s", pdb->name.data(), pdb->guid.data());
    return make_pdb(span, pdb->name.data(), pdb->guid.data());
}
