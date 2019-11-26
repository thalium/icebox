#include "symbols.hpp"

#define FDP_MODULE "pdb"
#include "endian.hpp"
#include "interfaces/if_symbols.hpp"
#include "log.hpp"
#include "reader.hpp"
#include "utils/hex.hpp"
#include "utils/pe.hpp"
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
    struct Sym
    {
        size_t name_idx;
        size_t offset;
    };

    struct Struc
    {
        size_t name_idx;
        size_t size;
    };

    struct Member
    {
        size_t name_idx; // struct name # member name
        size_t offset;
    };
}

namespace
{
    using StringData = std::vector<char>;
    using Strings    = std::vector<std::string_view>;
    using Symbols    = std::vector<Sym>;
    using Strucs     = std::vector<Struc>;
    using Members    = std::vector<Member>;

    struct Pdb
        : public symbols::Module
    {
        Pdb(fs::path filename, std::string guid);

        // methods
        bool setup();

        // IModule methods
        std::string_view        id          () override;
        opt<size_t>             symbol      (const std::string& symbol) override;
        opt<size_t>             struc_offset(const std::string& struc, const std::string& member) override;
        opt<size_t>             struc_size  (const std::string& struc) override;
        opt<symbols::Offset>    symbol      (size_t offset) override;
        bool                    sym_list    (symbols::on_symbol_fn on_sym) override;

        // members
        const fs::path    filename_;
        const std::string guid_;
        StringData        data_strings_;
        Strings           strings_;
        Symbols           symbols_;
        Symbols           offsets_to_symbols_;
        Strucs            strucs_;
        Members           members_;
    };
}

Pdb::Pdb(fs::path filename, std::string guid)
    : filename_(std::move(filename))
    , guid_(std::move(guid))
{
}

std::shared_ptr<symbols::Module> symbols::make_pdb(const std::string& module, const std::string& guid)
{
    const auto path = getenv("_NT_SYMBOL_PATH");
    if(!path)
        return nullptr;

    auto ptr      = std::make_unique<Pdb>(fs::path(path) / module / guid / module, guid);
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

    static std::string to_member_string(std::string_view struc, std::string_view member)
    {
        auto ret = std::string{};
        ret.reserve(struc.size() + 1 + member.size());
        for(auto c : struc)
            ret += static_cast<char>(tolower(c));
        ret += '#';
        for(auto c : member)
            ret += static_cast<char>(tolower(c));
        return ret;
    }

    static void insert_ordered(Symbols& vec, const Sym& item)
    {
        const auto predicate = [&](const auto& a, const auto& b)
        {
            return a.offset < b.offset;
        };
        vec.insert(std::upper_bound(std::begin(vec), std::end(vec), item, predicate), item);
    }

    static void emplace_string(Pdb& pdb, std::string_view item)
    {
        auto& dst       = pdb.data_strings_;
        const auto idx  = dst.size();
        const auto size = item.size();
        dst.resize(idx + size + 1);
        memcpy(&dst[idx], item.data(), size);
        dst[idx + size] = 0;
    }

    template <typename T>
    static void sort_by_name(std::vector<T>& vec, const Strings& strings)
    {
        vec.shrink_to_fit();
        std::sort(vec.begin(), vec.end(), [&](const auto& a, const auto& b)
        {
            return strings[a.name_idx] < strings[b.name_idx];
        });
    }
}

bool Pdb::setup()
{
    auto pdb       = pdb::PDBFile{};
    const auto err = pdb.load_pdb_file(filename_.generic_string().data());
    if(err != pdb::PDB_STATE_OK)
        return FAIL(false, "unable to open pdb %s: %s", filename_.generic_string().data(), to_string(err));

    pdb.initialize();
    const auto globals = pdb.get_global_variables();
    symbols_.reserve(globals->size());

    auto idx = size_t{0};
    for(const auto& it : *globals)
    {
        emplace_string(*this, it.second.name);
        const auto offset = static_cast<size_t>(it.second.address);
        const auto sym    = Sym{idx++, offset};
        symbols_.emplace_back(sym);
        insert_ordered(offsets_to_symbols_, sym);
    }

    for(const auto& it : pdb.get_types_container()->types_byname)
    {
        const auto& raw = *it.second;
        if(raw.type_class != pdb::PDBTYPE_STRUCT)
            continue;

        const auto& type = reinterpret_cast<const pdb::PDBTypeStruct&>(raw);
        emplace_string(*this, it.first);
        const auto size_bytes = static_cast<size_t>(type.size_bytes);
        strucs_.emplace_back(Struc{idx++, size_bytes});
        for(const auto& member : type.struct_members)
        {
            emplace_string(*this, to_member_string(it.first, member->name));
            const auto offset = static_cast<size_t>(member->offset);
            members_.emplace_back(Member{idx++, offset});
        }
    }

    data_strings_.shrink_to_fit();
    for(size_t i = 0; i < data_strings_.size(); i += strings_.back().size() + 1)
        strings_.emplace_back(std::string_view{&data_strings_[i]});
    strings_.shrink_to_fit();
    offsets_to_symbols_.shrink_to_fit();
    sort_by_name(symbols_, strings_);
    sort_by_name(strucs_, strings_);
    sort_by_name(members_, strings_);

    return true;
}

std::string_view Pdb::id()
{
    return guid_;
}

namespace
{
    template <typename T, typename U>
    static opt<T> binary_search(const Strings& strings, const std::vector<T>& vec, const U& item)
    {
        const auto it = std::lower_bound(std::begin(vec), std::end(vec), item, [&](const auto& a, const auto& b)
        {
            return strings[a.name_idx] < b;
        });
        if(it == std::end(vec))
            return {};

        const auto& str = strings[it->name_idx];
        if(str != item)
            return {};

        return *it;
    }
}

opt<size_t> Pdb::symbol(const std::string& symbol)
{
    const auto opt_sym = binary_search(strings_, symbols_, symbol);
    if(!opt_sym)
        return {};

    return opt_sym->offset;
}

bool Pdb::sym_list(symbols::on_symbol_fn on_sym)
{
    for(const auto& it : offsets_to_symbols_)
        if(on_sym(std::string{strings_[it.name_idx]}, it.offset) == walk_e::stop)
            break;

    return true;
}

opt<size_t> Pdb::struc_offset(const std::string& struc, const std::string& member)
{
    const auto target     = to_member_string(struc, member);
    const auto opt_member = binary_search(strings_, members_, target);
    if(!opt_member)
        return {};

    return opt_member->offset;
}

opt<size_t> Pdb::struc_size(const std::string& struc)
{
    const auto opt_struc = binary_search(strings_, strucs_, struc);
    if(!opt_struc)
        return {};

    return opt_struc->size;
}

namespace
{
    template <typename T>
    static opt<symbols::Offset> make_cursor(Pdb& p, const T& it, const T& end, size_t offset)
    {
        if(it == end)
            return {};

        return symbols::Offset{std::string{p.strings_[it->name_idx]}, offset - it->offset};
    }
}

opt<symbols::Offset> Pdb::symbol(size_t offset)
{
    // lower bound returns first item greater or equal
    auto it        = std::lower_bound(offsets_to_symbols_.begin(), offsets_to_symbols_.end(), offset, [](const auto& a, const auto& b)
    {
        return a.offset < b;
    });
    const auto end = offsets_to_symbols_.end();
    if(it == end)
        return make_cursor(*this, offsets_to_symbols_.rbegin(), offsets_to_symbols_.rend(), offset);

    // equal
    if(it->offset == offset)
        return make_cursor(*this, it, end, offset);

    // stricly greater, go to previous item
    return make_cursor(*this, --it, end, offset);
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
            const auto rsds = search(&src[0], &src[src_size], rsds_pattern);
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

std::shared_ptr<symbols::Module> symbols::make_pdb(span_t span, const reader::Reader& reader)
{
    // try to find pe debug section
    const auto debug     = pe::find_debug_codeview(reader, span);
    const auto span_read = debug ? *debug : span;

    auto buffer   = std::vector<uint8_t>(span_read.size);
    const auto ok = reader.read_all(&buffer[0], span_read.addr, span_read.size);
    if(!ok)
        return {};

    const auto pdb = read_pdb(&buffer[0], span_read.size);
    if(!pdb)
        return {};

    LOG(INFO, "%s %s", pdb->guid.data(), pdb->name.data());
    return make_pdb(pdb->name.data(), pdb->guid.data());
}
