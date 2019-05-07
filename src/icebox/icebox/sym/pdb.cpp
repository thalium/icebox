#include "sym.hpp"

#define FDP_MODULE "pdb"
#include "endian.hpp"
#include "log.hpp"
#include "utils/fnview.hpp"
#include "utils/hex.hpp"
#include "utils/utils.hpp"

#include <cctype>

#ifdef _MSC_VER
#    include <algorithm>
#    include <functional>
#    define search                          std::search
#    define boyer_moore_horspool_searcher   std::boyer_moore_horspool_searcher
#else
#    include <experimental/algorithm>
#    include <experimental/functional>
#    define search                          std::experimental::search
#    define boyer_moore_horspool_searcher   std::experimental::make_boyer_moore_horspool_searcher
#endif

#include "pdbparser.hpp"
namespace pdb = retdec::pdbparser;

#ifdef _MSC_VER
#    define stricmp _stricmp
#else
#    define stricmp strcasecmp
#endif

namespace
{
    static const int BASE_ADDRESS = 0x80000000;

    using Symbols         = std::unordered_map<std::string, pdb::PDBGlobalVariable>;
    using SymbolsByOffset = std::map<uint64_t, pdb::PDBGlobalVariable>;

    struct Pdb
        : public sym::IMod
    {
        Pdb(fs::path filename, span_t span);

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
        const fs::path  filename_;
        const span_t    span_;
        pdb::PDBFile    pdb_;
        Symbols         symbols_;
        SymbolsByOffset symbols_by_offset;
    };
}

Pdb::Pdb(fs::path filename, span_t span)
    : filename_(std::move(filename))
    , span_(span)
{
}

std::unique_ptr<sym::IMod> sym::make_pdb(span_t span, const std::string& module, const std::string& guid)
{
    const auto path = getenv("_NT_SYMBOL_PATH");
    if(!path)
        return nullptr;

    auto ptr      = std::make_unique<Pdb>(fs::path(path) / module / guid / module, span);
    const auto ok = ptr->setup();
    if(!ok)
        return nullptr;

    return ptr;
}

namespace
{
    static const char* to_string(pdb::PDBFileState x)
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

    static uint64_t get_offset(Pdb& pdb, const pdb::PDBGlobalVariable& var)
    {
        return pdb.span_.addr + var.address - BASE_ADDRESS;
    }
}

bool Pdb::setup()
{
    const auto err = pdb_.load_pdb_file(filename_.generic_string().data());
    if(err != pdb::PDB_STATE_OK)
        return FAIL(false, "unable to open pdb {}: {}", filename_.generic_string().data(), to_string(err));

    pdb_.initialize(BASE_ADDRESS);
    const auto globals = pdb_.get_global_variables();
    symbols_.reserve(globals->size());
    for(const auto& it : *globals)
    {
        symbols_.emplace(it.second.name, it.second);
        symbols_by_offset.emplace(get_offset(*this, it.second), it.second);
    }
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
        return {};

    return get_offset(*this, it->second);
}

bool Pdb::sym_list(sym::on_sym_fn on_sym)
{
    for(const auto& symbol : symbols_)
        if(on_sym(symbol.first, get_offset(*this, symbol.second)) == WALK_STOP)
            break;

    return true;
}

namespace
{
    static const pdb::PDBTypeStruct* get_struc(Pdb& p, const std::string& struc)
    {
        const auto type = p.pdb_.get_types_container()->get_type_by_name(const_cast<char*>(struc.data()));
        if(!type)
            return nullptr;

        if(type->type_class != pdb::PDBTYPE_STRUCT)
            return nullptr;

        return reinterpret_cast<const pdb::PDBTypeStruct*>(type);
    }
}

opt<uint64_t> Pdb::struc_offset(const std::string& struc, const std::string& member)
{
    const auto stype = get_struc(*this, struc);
    if(!stype)
        return {};

    for(const auto& m : stype->struct_members)
        if(!stricmp(member.data(), m->name))
            return m->offset;

    return {};
}

opt<size_t> Pdb::struc_size(const std::string& struc)
{
    const auto stype = get_struc(*this, struc);
    if(!stype)
        return {};

    return stype->size_bytes;
}

namespace
{
    template <typename T>
    static opt<sym::ModCursor> make_cursor(Pdb& p, const T& it, const T& end, uint64_t addr)
    {
        if(it == end)
            return {};

        return sym::ModCursor{it->second.name, addr - get_offset(p, it->second)};
    }
}

opt<sym::ModCursor> Pdb::symbol(uint64_t addr)
{
    // lower bound returns first item greater or equal
    auto it        = symbols_by_offset.lower_bound(addr);
    const auto end = symbols_by_offset.end();
    if(it == end)
        return make_cursor(*this, symbols_by_offset.rbegin(), symbols_by_offset.rend(), addr);

    // equal
    if(it->first == addr)
        return make_cursor(*this, it, end, addr);

    // stricly greater, go to previous item
    return make_cursor(*this, --it, end, addr);
}

namespace
{
    struct PdbCtx
    {
        std::string guid;
        std::string name;
    };

    static opt<std::string> read_pdb_name(const uint8_t* ptr, const uint8_t* end)
    {
        for(auto it = ptr; it != end; ++it)
            if(!std::isprint(*it))
                return {};

        return std::string{ptr, end};
    }

    static const uint8_t rsds_magic[] = {'R', 'S', 'D', 'S'};
    static const auto rsds_pattern    = boyer_moore_horspool_searcher(std::begin(rsds_magic), std::end(rsds_magic));

    static opt<PdbCtx> read_pdb(const void* vsrc, size_t src_size)
    {
        auto src       = reinterpret_cast<const uint8_t*>(vsrc);
        const auto end = &src[src_size];
        while(true)
        {
            const auto rsds = search(&src[0], &src[src_size - 1], rsds_pattern);
            if(rsds == &src[src_size])
                return FAIL(ext::nullopt, "unable to find RSDS pattern into kernel module");

            const auto size = std::distance(rsds, &src[src_size]);
            if(size < 4 /*magic*/ + 16 /*guid*/ + 4 /*age*/ + 2 /*name*/)
                return FAIL(ext::nullopt, "kernel module is too small for pdb header");

            const auto name_end = reinterpret_cast<const uint8_t*>(memchr(&rsds[4 + 16 + 4], 0x00, size));
            if(!name_end)
                return FAIL(ext::nullopt, "missing null-terminating byte on PDB header module name");

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

std::unique_ptr<sym::IMod> sym::make_pdb(span_t span, const void* data, const size_t data_size)
{
    const auto pdb = read_pdb(data, data_size);
    if(!pdb)
        return nullptr;

    LOG(INFO, "{} {}", pdb->name.data(), pdb->guid.data());
    return make_pdb(span, pdb->name.data(), pdb->guid.data());
}
