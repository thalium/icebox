#include "symbols.hpp"

#define FDP_MODULE "dwarf"
#include "interfaces/if_symbols.hpp"
#include "log.hpp"
#include "utils/utils.hpp"

#include <dwarf.h>
#include <libdwarf.h>

namespace
{
    struct Dwarf
        : public symbols::Module
    {
        Dwarf(fs::path filename, std::string guid);
        ~Dwarf() override;

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
        Dwarf_Debug dbg = nullptr;
        Dwarf_Error err = nullptr;
        std::vector<Dwarf_Die> structures; // buffer of all references of structures
    };
}

namespace
{
    static bool open_file(Dwarf& p)
    {
        const auto ok = dwarf_init_path(
            p.filename_.generic_string().data(), // path
            nullptr,                             // true_path_out_buffer
            0,                                   // true_path_bufferlen
            DW_DLC_READ,                         // access
            DW_GROUPNUMBER_ANY,                  // groupnumber
            nullptr,                             // errhand
            nullptr,                             // errarg
            &p.dbg,                              // ret_dbg
            nullptr,                             // reserved1
            0,                                   // reserved2
            nullptr,                             // reserved3
            &p.err                               // error
        );

        if(ok == DW_DLV_ERROR)
            return FAIL(false, "libdwarf error %llu when initializing dwarf file : %s", dwarf_errno(p.err), dwarf_errmsg(p.err));

        if(ok == DW_DLV_NO_ENTRY)
            return FAIL(false, "unfound file or dwarf information in file '%s'", p.filename_.generic_string().data());

        return true;
    }

    static opt<std::vector<Dwarf_Die>> read_cu(Dwarf& p)
    {
        auto cu = std::vector<Dwarf_Die>();
        Dwarf_Unsigned cu_offset;
        int            ok;

        while(true)
        {
            ok = dwarf_next_cu_header_d(
                p.dbg,      // dbg
                true,       // is_info
                nullptr,    // cu_header_length
                nullptr,    // version_stamp
                nullptr,    // abbrev_offset
                nullptr,    // address_size
                nullptr,    // length_size
                nullptr,    // extension_size
                nullptr,    // type signature
                nullptr,    // typeoffset
                &cu_offset, // next_cu_header_offset
                nullptr,    // header_cu_type
                &p.err      // error
            );

            if(ok == DW_DLV_ERROR)
                return FAIL(ext::nullopt, "libdwarf error %llu when reading dwarf file : %s", dwarf_errno(p.err), dwarf_errmsg(p.err));

            if(ok == DW_DLV_NO_ENTRY)
                break;

            Dwarf_Die die = nullptr;
            ok            = dwarf_siblingof_b(p.dbg, nullptr, true, &die, &p.err);

            if(ok == DW_DLV_NO_ENTRY)
                continue;

            if(ok == DW_DLV_ERROR)
                return FAIL(ext::nullopt, "libdwarf error %llu when reading dwarf file : %s", dwarf_errno(p.err), dwarf_errmsg(p.err));

            cu.push_back(die);
        }

        if(cu.empty())
            return FAIL(ext::nullopt, "no compilation unit found in file %s", p.filename_.generic_string().data());

        return cu;
    }

    template <typename T>
    static bool read_children(Dwarf& p, const Dwarf_Die& parent, T on_child)
    {
        Dwarf_Die child = nullptr;
        auto ok         = dwarf_child(parent, &child, &p.err);

        while(ok != DW_DLV_NO_ENTRY)
        {
            if(ok == DW_DLV_ERROR)
                return FAIL(false, "libdwarf error %llu when reading dwarf file : %s", dwarf_errno(p.err), dwarf_errmsg(p.err));
            if(on_child(child) == walk_e::stop)
                break;

            ok = dwarf_siblingof_b(p.dbg, child, true, &child, &p.err);
        }

        return true;
    }

    static opt<Dwarf_Die> get_member(Dwarf& p, const std::string& name, const Dwarf_Die& structure)
    {
        opt<Dwarf_Die> result_member = {};
        read_children(p, structure, [&](const Dwarf_Die& member)
        {
            char* name_ptr        = nullptr;
            const auto ok_diename = dwarf_diename(member, &name_ptr, &p.err);

            if(ok_diename == DW_DLV_ERROR)
                LOG(ERROR, "libdwarf error %llu when reading name of a DIE : %s", dwarf_errno(p.err), dwarf_errmsg(p.err));

            if(ok_diename == DW_DLV_NO_ENTRY) // anonymous struct
            {
                Dwarf_Off type_offset = 0;
                auto ok               = dwarf_dietype_offset(member, &type_offset, &p.err);

                if(ok == DW_DLV_ERROR)
                    LOG(ERROR, "libdwarf error %llu when reading type offset of a DIE : %s", dwarf_errno(p.err), dwarf_errmsg(p.err));

                if(ok != DW_DLV_OK)
                    return walk_e::next;

                Dwarf_Die anonymous_struct = nullptr;
                ok                         = dwarf_offdie_b(p.dbg, type_offset, true, &anonymous_struct, &p.err);

                if(ok == DW_DLV_ERROR)
                    LOG(ERROR, "libdwarf error %llu when getting DIE : %s", dwarf_errno(p.err), dwarf_errmsg(p.err));

                if(ok != DW_DLV_OK)
                    return FAIL(walk_e::next, "unable to get DIE at offset 0x%llu", type_offset);

                const auto child = get_member(p, name, anonymous_struct);
                if(child)
                {
                    result_member = *child;
                    return walk_e::stop;
                }
            }

            if(ok_diename != DW_DLV_OK)
                return walk_e::next;

            const std::string structure_name(name_ptr);
            if(structure_name == name)
            {
                result_member = member;
                return walk_e::stop;
            }
            return walk_e::next;
        });
        return result_member;
    }

    template <typename T>
    static bool get_structure(Dwarf& p, const std::string& name, T on_structure)
    {
        if(name.empty())
            return false;

        for(const auto structure : p.structures)
        {
            char* name_ptr = nullptr;
            auto ok        = dwarf_diename(structure, &name_ptr, &p.err);

            if(ok == DW_DLV_ERROR)
                LOG(ERROR, "libdwarf error %llu when reading name of a DIE : %s", dwarf_errno(p.err), dwarf_errmsg(p.err));

            if(ok != DW_DLV_OK)
                continue;

            const std::string structure_name(name_ptr);
            if(structure_name == name)
            {
                Dwarf_Off type_offset = 0;
                ok                    = dwarf_dietype_offset(structure, &type_offset, &p.err); // typedef struct if there is an offset

                if(ok == DW_DLV_ERROR)
                    LOG(ERROR, "libdwarf error %llu when reading type offset of a DIE : %s", dwarf_errno(p.err), dwarf_errmsg(p.err));

                if(ok != DW_DLV_OK)
                {
                    if(on_structure(structure) == walk_e::stop)
                        return true;
                    continue;
                }

                Dwarf_Die typedef_struct = nullptr;
                ok                       = dwarf_offdie_b(p.dbg, type_offset, true, &typedef_struct, &p.err);

                if(ok == DW_DLV_ERROR)
                    LOG(ERROR, "libdwarf error %llu when getting DIE : %s", dwarf_errno(p.err), dwarf_errmsg(p.err));

                if(ok != DW_DLV_OK)
                    return FAIL(false, "unable to get DIE at offset 0x%llu, and so unable to find structure '%s'", type_offset, name.data());

                if(on_structure(typedef_struct) == walk_e::stop)
                    return true;
            }
        }

        return false;
    }

    static opt<uint64_t> get_attr_member_location(Dwarf& p, const Dwarf_Die& die)
    {
        Dwarf_Attribute attr = nullptr;
        const auto ok        = dwarf_attr(die, DW_AT_data_member_location, &attr, &p.err);

        if(ok == DW_DLV_ERROR)
            return FAIL(ext::nullopt, "libdwarf error %llu when reading attributes of a DIE : %s", dwarf_errno(p.err), dwarf_errmsg(p.err));

        if(ok == DW_DLV_NO_ENTRY)
            return FAIL(ext::nullopt, "die member has not DW_AT_data_member_location attribute");

        Dwarf_Half form;
        if(dwarf_whatform(attr, &form, &p.err) != DW_DLV_OK)
            return FAIL(ext::nullopt, "libdwarf error %llu when reading form of DW_AT_data_member_location attribute : %s", dwarf_errno(p.err), dwarf_errmsg(p.err));

        if(form == DW_FORM_data1 || form == DW_FORM_data2 || form == DW_FORM_data4 || form == DW_FORM_data8 || form == DW_FORM_udata)
        {
            Dwarf_Unsigned offset;
            if(dwarf_formudata(attr, &offset, &p.err) != DW_DLV_OK)
                return FAIL(ext::nullopt, "libdwarf error %llu when reading DW_AT_data_member_location attribute : %s", dwarf_errno(p.err), dwarf_errmsg(p.err));

            return (uint64_t) offset;
        }
        else if(form == DW_FORM_sdata)
        {
            Dwarf_Signed soffset;
            if(dwarf_formsdata(attr, &soffset, &p.err) != DW_DLV_OK)
                return FAIL(ext::nullopt, "libdwarf error %llu when reading DW_AT_data_member_location attribute : %s", dwarf_errno(p.err), dwarf_errmsg(p.err));

            if(soffset < 0)
                return FAIL(ext::nullopt, "unsupported negative offset for DW_AT_data_member_location attribute");

            return (uint64_t) soffset;
        }
        else
        {
            Dwarf_Locdesc** llbuf = nullptr;
            Dwarf_Signed listlen;
            if(dwarf_loclist_n(attr, &llbuf, &listlen, &p.err) != DW_DLV_OK)
            {
                dwarf_dealloc(p.dbg, &llbuf, DW_DLA_LOCDESC);
                return FAIL(ext::nullopt, "unsupported member offset in DW_AT_data_member_location attribute");
            }

            if(listlen != 1 || llbuf[0]->ld_cents != 1 || (llbuf[0]->ld_s[0]).lr_atom != DW_OP_plus_uconst)
            {
                dwarf_dealloc(p.dbg, &llbuf, DW_DLA_LOCDESC);
                return FAIL(ext::nullopt, "unsupported location expression in DW_AT_data_member_location attribute");
            }

            const auto offset = llbuf[0]->ld_s[0].lr_number;
            dwarf_dealloc(p.dbg, &llbuf, DW_DLA_LOCDESC);
            return (uint64_t) offset;
        }
    }

    static opt<size_t> struc_size_internal(Dwarf& p, const Dwarf_Die& struc)
    {
        Dwarf_Unsigned size;
        const auto ok = dwarf_bytesize(struc, &size, &p.err);

        if(ok == DW_DLV_ERROR)
            return FAIL(ext::nullopt, "libdwarf error %llu when reading size of a DIE : %s", dwarf_errno(p.err), dwarf_errmsg(p.err));

        if(ok == DW_DLV_NO_ENTRY)
            return {};

        return size;
    }
}

Dwarf::Dwarf(fs::path filename, std::string guid)
    : filename_(std::move(filename))
    , guid_(std::move(guid))
{
}

Dwarf::~Dwarf()
{
    dwarf_finish(dbg, &err);
}

std::shared_ptr<symbols::Module> symbols::make_dwarf(const std::string& module, const std::string& guid)
{
    const auto path = getenv("_LINUX_SYMBOL_PATH");
    if(!path)
        return nullptr;

    auto ptr = std::make_unique<Dwarf>(fs::path(path) / module / guid / "elf", guid);
    if(!ptr->setup())
        return nullptr;

    return ptr;
}

std::shared_ptr<symbols::Module> symbols::make_dwarf(span_t /*span*/, const reader::Reader& /*reader*/)
{
    return {};
}

bool Dwarf::setup()
{
    if(!open_file(*this))
        return false;

    const auto cu = read_cu(*this);
    if(!cu)
        return false;

    for(const auto die : *cu)
    {
        read_children(*this, die, [&](const Dwarf_Die& child)
        {
            structures.push_back(child);
            return walk_e::next;
        });
    }

    if(structures.empty())
        return FAIL(false, "no structures found in file %s", filename_.generic_string().data());

    return true;
}

std::string_view Dwarf::id()
{
    return guid_;
}

opt<size_t> Dwarf::symbol(const std::string& /*symbol*/)
{
    return {};
}

bool Dwarf::sym_list(symbols::on_symbol_fn /*on_sym*/)
{
    return false;
}

opt<size_t> Dwarf::struc_offset(const std::string& struc, const std::string& member)
{
    opt<Dwarf_Die> child = {};
    get_structure(*this, struc, [&](const Dwarf_Die& structure)
    {
        child = get_member(*this, member, structure);
        if(!child)
            return walk_e::next;

        return walk_e::stop;
    });
    if(!child)
        return {};

    const auto offset = get_attr_member_location(*this, *child);
    if(!offset)
        return {};

    return offset;
}

opt<size_t> Dwarf::struc_size(const std::string& struc)
{
    opt<size_t> size = {};
    get_structure(*this, struc, [&](const Dwarf_Die& structure)
    {
        size = struc_size_internal(*this, structure);
        if(!size)
            return walk_e::next;

        return walk_e::stop;
    });

    if(!size)
        LOG(ERROR, "unfound %s structure or die has not DW_AT_byte_size attribute", struc.data());

    return size;
}

opt<symbols::Offset> Dwarf::symbol(size_t /*offset*/)
{
    return {};
}
