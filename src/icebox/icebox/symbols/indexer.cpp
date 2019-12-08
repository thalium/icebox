#include "indexer.hpp"

#include <cstring>

#ifdef _MSC_VER
#    define strnicmp _strnicmp
#else
#    include <strings.h>
#    define strnicmp strncasecmp
#endif

struct symbols::Struc
{
    uint32_t name_idx;
    uint32_t size;
    uint32_t member_idx;
    uint32_t member_end;
};

namespace
{
    struct Sym
    {
        uint32_t name_idx;
        uint32_t offset;
    };

    struct Member
    {
        uint32_t name_idx;
        uint32_t offset;
    };

    using StringData = std::vector<char>;
    using Strings    = std::vector<std::string_view>;
    using Symbols    = std::vector<Sym>;
    using Strucs     = std::vector<symbols::Struc>;
    using Members    = std::vector<Member>;

    struct Data
        : public symbols::Indexer
    {
        Data(std::string_view id);

        // symbols::Indexer methods
        void            add_symbol  (std::string_view name, size_t offset) override;
        symbols::Struc& add_struc   (std::string_view name, size_t size) override;
        void            add_member  (symbols::Struc& struc, std::string_view name, size_t offset) override;
        void            finalize    () override;

        // symbols::Module methods
        std::string_view        id              () override;
        opt<size_t>             symbol_offset   (const std::string& symbol) override;
        void                    struc_names     (const symbols::on_name_fn& on_struc) override;
        opt<size_t>             struc_size      (const std::string& struc) override;
        void                    struc_members   (const std::string& struc, const symbols::on_name_fn& on_member) override;
        opt<size_t>             member_offset   (const std::string& struc, const std::string& member) override;
        opt<symbols::Offset>    find_symbol     (size_t offset) override;
        bool                    list_symbols    (symbols::on_symbol_fn on_symbol) override;

        const std::string guid;
        uint32_t          last_name_idx;
        StringData        data;
        Strings           strings;
        Symbols           symbols;
        Symbols           offsets;
        Strucs            strucs;
        Members           members;
    };

    void save_string_data(StringData& data, std::string_view item)
    {
        const auto idx  = data.size();
        const auto size = item.size();
        data.resize(idx + size + 1);
        memcpy(&data[idx], item.data(), size);
        data[idx + size] = 0;
    }

    template <typename T>
    void remap_and_shrink(T& items, const std::vector<uint32_t>& reverse)
    {
        for(auto& item : items)
            item.name_idx = reverse[item.name_idx];
        items.shrink_to_fit();
    }
}

Data::Data(std::string_view id)
    : guid(id)
    , last_name_idx(0)
{
}

void Data::add_symbol(std::string_view name, size_t offset)
{
    const auto name_idx = last_name_idx++;
    save_string_data(data, name);
    const auto sym = Sym{name_idx, static_cast<uint32_t>(offset)};
    symbols.emplace_back(sym);
    offsets.emplace_back(sym);
}

symbols::Struc& Data::add_struc(std::string_view name, size_t size)
{
    const auto name_idx = last_name_idx++;
    save_string_data(data, name);
    const auto usize      = static_cast<uint32_t>(size);
    const auto member_idx = static_cast<uint32_t>(members.size());
    strucs.emplace_back(symbols::Struc{name_idx, usize, member_idx, 0});
    return strucs.back();
}

void Data::add_member(symbols::Struc& struc, std::string_view name, size_t offset)
{
    const auto name_idx = last_name_idx++;
    save_string_data(data, name);
    members.emplace_back(Member{name_idx, static_cast<uint32_t>(offset)});
    struc.member_end = static_cast<uint32_t>(members.size());
}

namespace
{
    void fill_strings(Strings& strings, const StringData& data)
    {
        for(size_t i = 0; i < data.size(); i += strings.back().size() + 1)
            strings.emplace_back(std::string_view{&data[i]});
    }
}

void Data::finalize()
{
    // sort & map strings
    fill_strings(strings, data);
    auto sorted = std::vector<uint32_t>{};
    sorted.resize(strings.size());
    for(uint32_t i = 0; i < strings.size(); ++i)
        sorted[i] = i;
    std::sort(sorted.begin(), sorted.end(), [&](const auto& a, const auto& b)
    {
        return strings[a] < strings[b];
    });

    // reverse indexes & rebuild string buffers
    auto reverse = std::vector<uint32_t>{};
    reverse.resize(strings.size());
    auto new_data = StringData{};
    new_data.reserve(data.size());
    for(uint32_t i = 0; i < strings.size(); ++i)
    {
        const auto idx = sorted[i];
        reverse[idx]   = i;
        save_string_data(new_data, strings[idx]);
    }
    data.swap(new_data);
    strings.resize(0);
    fill_strings(strings, data);
    strings.shrink_to_fit();

    // index all remaining buffers
    const auto by_name = [](const auto& a, const auto& b)
    {
        return a.name_idx < b.name_idx;
    };
    const auto by_offset = [](const auto& a, const auto& b)
    {
        return a.offset < b.offset;
    };
    remap_and_shrink(symbols, reverse);
    std::sort(symbols.begin(), symbols.end(), by_name);
    remap_and_shrink(offsets, reverse);
    std::sort(offsets.begin(), offsets.end(), by_offset);
    remap_and_shrink(strucs, reverse);
    std::sort(strucs.begin(), strucs.end(), by_name);
    remap_and_shrink(members, reverse);
}

namespace
{
    template <typename T, typename U>
    opt<T> binary_search(const Strings& strings, const std::vector<T>& vec, const U& item)
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

std::string_view Data::id()
{
    return this->guid;
}

opt<size_t> Data::symbol_offset(const std::string& symbol)
{
    const auto opt_sym = binary_search(strings, symbols, symbol);
    if(!opt_sym)
        return {};

    return opt_sym->offset;
}

void Data::struc_names(const symbols::on_name_fn& on_struc)
{
    for(const auto& struc : strucs)
        on_struc(strings[struc.name_idx]);
}

opt<size_t> Data::struc_size(const std::string& struc)
{
    const auto opt_struc = binary_search(strings, strucs, struc);
    if(!opt_struc)
        return {};

    return opt_struc->size;
}

void Data::struc_members(const std::string& struc, const symbols::on_name_fn& on_member)
{
    const auto opt_struc = binary_search(strings, strucs, struc);
    if(!opt_struc)
        return;

    for(auto idx = opt_struc->member_idx; idx < opt_struc->member_end; ++idx)
    {
        const auto& m = members[idx];
        on_member(strings[m.name_idx]);
    }
}

namespace
{
    bool is_lowercase_equal(const std::string_view& a, const std::string_view& b)
    {
        if(a.size() != b.size())
            return false;

        return !strnicmp(a.data(), b.data(), a.size());
    }
}

opt<size_t> Data::member_offset(const std::string& struc, const std::string& member)
{
    const auto opt_struc = binary_search(strings, strucs, struc);
    if(!opt_struc)
        return {};

    for(auto idx = opt_struc->member_idx; idx != opt_struc->member_end; ++idx)
    {
        const auto& m = members[idx];
        if(is_lowercase_equal(member, strings[m.name_idx]))
            return m.offset;
    }
    return {};
}

namespace
{
    template <typename T>
    opt<symbols::Offset> make_cursor(Data& d, const T& it, const T& end, size_t offset)
    {
        if(it == end)
            return {};

        return symbols::Offset{std::string{d.strings[it->name_idx]}, offset - it->offset};
    }
}

opt<symbols::Offset> Data::find_symbol(size_t offset)
{
    // lower bound returns first item greater or equal
    auto it        = std::lower_bound(offsets.begin(), offsets.end(), offset, [](const auto& a, const auto& b)
    {
        return a.offset < b;
    });
    const auto end = offsets.end();
    if(it == end)
        return make_cursor(*this, offsets.rbegin(), offsets.rend(), offset);

    // equal
    if(it->offset == offset)
        return make_cursor(*this, it, end, offset);

    if(it == offsets.begin())
        return {};

    // strictly greater, go to previous item
    return make_cursor(*this, --it, end, offset);
}

bool Data::list_symbols(symbols::on_symbol_fn on_sym)
{
    for(const auto& it : offsets)
        if(on_sym(std::string{strings[it.name_idx]}, it.offset) == walk_e::stop)
            break;

    return true;
}

std::shared_ptr<symbols::Indexer> symbols::make_indexer(std::string_view id)
{
    return std::make_shared<Data>(id);
}
