#include "symbols.hpp"

#define FDP_MODULE "pdb"
#include "endian.hpp"
#include "indexer.hpp"
#include "log.hpp"
#include "memory.hpp"
#include "utils/hex.hpp"
#include "utils/pe.hpp"

#include <algorithm>
#include <cctype>
#include <functional>

#include "pdbparser.hpp"
namespace pdb = retdec::pdbparser;

namespace
{
    const char* to_string(pdb::PDBFileState x)
    {
        switch(x)
        {
            case pdb::PDB_STATE_OK:                     return "ok";
            case pdb::PDB_STATE_ALREADY_LOADED:         return "already_loaded";
            case pdb::PDB_STATE_ERR_FILE_OPEN:          return "err_file_open";
            case pdb::PDB_STATE_INVALID_FILE:           return "invalid_file";
            case pdb::PDB_STATE_UNSUPPORTED_VERSION:    return "unsupported_version";
        }
        return "<invalid>";
    }

    bool setup_pdb(symbols::Indexer& indexer, const fs::path& filename)
    {
        auto pdb       = pdb::PDBFile{};
        const auto err = pdb.load_pdb_file(filename.generic_string().data());
        if(err != pdb::PDB_STATE_OK)
            return FAIL(false, "unable to open pdb %s: %s", filename.generic_string().data(), to_string(err));

        pdb.initialize();

        const auto globals = pdb.get_global_variables();
        for(const auto& it : *globals)
            indexer.add_symbol(it.second.name, static_cast<size_t>(it.second.address));

        for(const auto& it : pdb.get_types_container()->types_byname)
        {
            const auto& raw = *it.second;
            if(raw.type_class != pdb::PDBTYPE_STRUCT)
                continue;

            const auto& type = reinterpret_cast<const pdb::PDBTypeStruct&>(raw);
            const auto size  = static_cast<size_t>(type.size_bytes);
            auto& struc      = indexer.add_struc(it.first, size);
            for(const auto& member : type.struct_members)
                indexer.add_member(struc, member->name, static_cast<size_t>(member->offset));
        }

        indexer.finalize();
        return true;
    }

    struct PdbCtx
    {
        std::string guid;
        std::string name;
    };

    opt<std::string> read_pdb_name(const uint8_t* ptr, const uint8_t* end)
    {
        for(auto it = ptr; it != end; ++it)
            if(!std::isprint(*it))
                return {};

        return std::string{ptr, end};
    }

    constexpr uint8_t rsds_magic[] = {'R', 'S', 'D', 'S'};
    const auto rsds_pattern        = std::boyer_moore_horspool_searcher(std::begin(rsds_magic), std::end(rsds_magic));

    opt<PdbCtx> read_pdb(const void* vsrc, size_t src_size)
    {
        auto src       = reinterpret_cast<const uint8_t*>(vsrc);
        const auto end = &src[src_size];
        while(true)
        {
            const auto rsds = std::search(&src[0], &src[src_size], rsds_pattern);
            if(rsds == &src[src_size])
                return FAIL(std::nullopt, "unable to find RSDS pattern into kernel module");

            const auto size = std::distance(rsds, &src[src_size]);
            if(size < 4 /*magic*/ + 16 /*guid*/ + 4 /*age*/ + 2 /*name*/)
                return FAIL(std::nullopt, "kernel module is too small for pdb header");

            const auto name_end = reinterpret_cast<const uint8_t*>(memchr(&rsds[4 + 16 + 4], 0x00, size));
            if(!name_end)
                return FAIL(std::nullopt, "missing null-terminating byte on PDB header module name");

            uint8_t guid[16];
            write_be32(&guid[0], read_le32(&rsds[4 + 0])); // Data1
            write_be16(&guid[4], read_le16(&rsds[4 + 4])); // Data2
            write_be16(&guid[6], read_le16(&rsds[4 + 6])); // Data3
            memcpy(&guid[8], &rsds[4 + 8], 8);             // Data4

            char strguid[sizeof guid * 2];
            hex::convert(strguid, hex::chars_upper, guid, sizeof guid);

            uint32_t age    = read_le32(&rsds[4 + 16]);
            const auto name = read_pdb_name(&rsds[4 + 16 + 4], name_end);
            if(name)
                return PdbCtx{std::string{strguid, sizeof strguid} + std::to_string(age), *name};

            src      = rsds + 1;
            src_size = end - src;
        }
    }
}

opt<symbols::Identity> symbols::identify_pdb(span_t span, const memory::Io& io)
{
    // try to find pe debug section
    const auto debug     = pe::find_debug_codeview(io, span);
    const auto span_read = debug ? *debug : span;

    auto buffer   = std::vector<uint8_t>(span_read.size);
    const auto ok = io.read_all(&buffer[0], span_read.addr, span_read.size);
    if(!ok)
        return {};

    const auto pdb = read_pdb(&buffer[0], span_read.size);
    if(!pdb)
        return {};

    return symbols::Identity{pdb->name, pdb->guid};
}

std::shared_ptr<symbols::Module> symbols::make_pdb(const std::string& module, const std::string& guid)
{
    const auto path = getenv("_NT_SYMBOL_PATH");
    if(!path)
        return FAIL(nullptr, "missing _NT_SYMBOL_PATH environment variable");

    auto indexer = symbols::make_indexer(guid);
    if(!indexer)
        return nullptr;

    const auto ok = setup_pdb(*indexer, fs::path(path) / module / guid / module);
    if(!ok)
        return nullptr;

    return indexer;
}
