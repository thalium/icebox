#include "sym.hpp"

#define FDP_MODULE "dwarf"
#include "log.hpp"
#include "utils/fnview.hpp"

#include <dwarf.h>
#include <libdwarf.h>

namespace
{
    struct Dwarf
        : public sym::IMod
    {
         Dwarf(fs::path filename);
        ~Dwarf();

        // methods
        bool                        setup                   ();
        bool                        open_file               ();
        opt<std::vector<Dwarf_Die>> read_cu                 ();
        bool                        read_children           (const Dwarf_Die& parent, std::vector<Dwarf_Die>& children);
        opt<Dwarf_Die>              get_structure           (const std::string& name);
        opt<Dwarf_Die>              get_structure           (const std::string& name, const std::vector<Dwarf_Die>& collection_of_dies);
        opt<size_t>                 struc_size              (const Dwarf_Die& struc);
        opt<uint64_t>               get_attr_member_location(const Dwarf_Die& die);

        // IModule methods
        span_t              span        () override;
        opt<uint64_t>       symbol      (const std::string& symbol) override;
        opt<uint64_t>       struc_offset(const std::string& struc, const std::string& member) override;
        opt<size_t>         struc_size  (const std::string& struc) override;
        opt<sym::ModCursor> symbol      (uint64_t addr) override;
        bool                sym_list    (sym::on_sym_fn on_sym) override;

        // members
        const fs::path filename_;
        Dwarf_Debug dbg = nullptr;
        Dwarf_Error err = nullptr;
        std::vector<Dwarf_Die> structures; // buffer of all references of structures
    };
}

Dwarf::Dwarf(fs::path filename)
    : filename_(std::move(filename))
{
}

Dwarf::~Dwarf()
{
    if(dwarf_finish(dbg, &err) != DW_DLV_OK)
    {
        LOG(ERROR, "unable to free dwarf ressources ({}) : {}", dwarf_errno(err), dwarf_errmsg(err));
    }
}

std::unique_ptr<sym::IMod> sym::make_dwarf(span_t /*span*/, const std::string& module, const std::string& guid)
{
    const auto path = getenv("_LINUX_SYMBOL_PATH");
    if(!path)
        return nullptr;

    auto ptr = std::make_unique<Dwarf>(fs::path(path) / module / guid / "headers.ko");
    if(!ptr->setup())
        return nullptr;

    return ptr;
}

std::unique_ptr<sym::IMod> sym::make_dwarf(span_t /*span*/, const void* /*data*/, const size_t /*data_size*/)
{
    LOG(ERROR, "make_dwarf(span, data, data_size) not implemented");
    return nullptr;
}

bool Dwarf::setup()
{
    if(!open_file())
        return false;

    const auto cu = read_cu();
    if(!cu)
        return false;

    std::vector<Dwarf_Die> children;
    for(const auto die : *cu)
    {
        if(read_children(die, children))
            structures.insert(structures.end(), children.begin(), children.end());
    }

    if(structures.empty())
        return FAIL(false, "no structures found in file {}", filename_.generic_string());

    return true;
}

bool Dwarf::open_file()
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

    if(ok == DW_DLV_ERROR)
        return FAIL(false, "libdwarf error {} when initializing dwarf file : {}", dwarf_errno(err), dwarf_errmsg(err));

    if(ok == DW_DLV_NO_ENTRY)
        return FAIL(false, "unfound file or dwarf information in file '{}'", filename_.generic_string());

    return true;
}

opt<std::vector<Dwarf_Die>> Dwarf::read_cu()
{
    const auto cu = std::make_unique<std::vector<Dwarf_Die>>();
    Dwarf_Unsigned cu_offset;
    int            ok;

    while(true)
    {
        ok = dwarf_next_cu_header_d(
            dbg,        // dbg
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
            &err        // error
        );

        if(ok == DW_DLV_ERROR)
            return FAIL(ext::nullopt, "libdwarf error {} when reading dwarf file : {}", dwarf_errno(err), dwarf_errmsg(err));

        if(ok == DW_DLV_NO_ENTRY)
            break;

        const auto die = std::make_unique<Dwarf_Die>();
        ok             = dwarf_siblingof_b(dbg, NULL, true, die.get(), &err);

        if(ok == DW_DLV_NO_ENTRY)
            continue;

        if(ok == DW_DLV_ERROR)
            return FAIL(ext::nullopt, "libdwarf error {} when reading dwarf file : {}", dwarf_errno(err), dwarf_errmsg(err));

        cu.get()->push_back(*die.get());
    }

    if(cu.get()->empty())
        return FAIL(ext::nullopt, "no compilation unit found in file {}", filename_.generic_string());

    return *cu.get();
}

bool Dwarf::read_children(const Dwarf_Die& parent, std::vector<Dwarf_Die>& children)
{
    children.clear();

    const auto child = std::make_unique<Dwarf_Die>();
    auto ok          = dwarf_child(parent, child.get(), &err);

    while(ok != DW_DLV_NO_ENTRY)
    {
        if(ok == DW_DLV_ERROR)
        {
            children.clear();
            return FAIL(false, "libdwarf error {} when reading dwarf file : {}", dwarf_errno(err), dwarf_errmsg(err));
        }
        children.push_back(*child.get());

        ok = dwarf_siblingof_b(dbg, *child.get(), true, child.get(), &err);
    }

    return true;
}

span_t Dwarf::span()
{
    return {};
}

opt<uint64_t> Dwarf::symbol(const std::string& /*symbol*/)
{
    return {};
}

bool Dwarf::sym_list(sym::on_sym_fn /*on_sym*/)
{
    return false;
}

opt<Dwarf_Die> Dwarf::get_structure(const std::string& name)
{
    return get_structure(name, structures);
}

opt<Dwarf_Die> Dwarf::get_structure(const std::string& name, const std::vector<Dwarf_Die>& collection_of_dies)
{
    for(const auto structure : collection_of_dies)
    {
        const auto name_ptr = std::make_unique<char*>();
        auto ok             = dwarf_diename(structure, name_ptr.get(), &err);

        if(ok == DW_DLV_ERROR)
            LOG(ERROR, "libdwarf error {} when reading name of a DIE : {}", dwarf_errno(err), dwarf_errmsg(err));

        if(ok != DW_DLV_OK)
            continue;

        const std::string structure_name(*name_ptr.get());
        if(structure_name == name)
            return structure;
    }

    LOG(ERROR, "unable to find structure '{}'", name);
    return {};
}

opt<uint64_t> Dwarf::get_attr_member_location(const Dwarf_Die& die)
{
    const auto attr = std::make_unique<Dwarf_Attribute>();
    const auto ok   = dwarf_attr(die, DW_AT_data_member_location, attr.get(), &err);

    if(ok == DW_DLV_ERROR)
        return FAIL(ext::nullopt, "libdwarf error {} when reading attributes of a DIE : {}", dwarf_errno(err), dwarf_errmsg(err));

    if(ok == DW_DLV_NO_ENTRY)
        return FAIL(ext::nullopt, "die member has not DW_AT_data_member_location attribute");

    Dwarf_Half form;
    if(dwarf_whatform(*attr.get(), &form, &err) != DW_DLV_OK)
        return FAIL(ext::nullopt, "libdwarf error {} when reading form of DW_AT_data_member_location attribute : {}", dwarf_errno(err), dwarf_errmsg(err));

    if(form == DW_FORM_data1 || form == DW_FORM_data2 || form == DW_FORM_data4 || form == DW_FORM_data8 || form == DW_FORM_udata)
    {
        Dwarf_Unsigned offset;
        if(dwarf_formudata(*attr.get(), &offset, &err) != DW_DLV_OK)
            return FAIL(ext::nullopt, "libdwarf error {} when reading DW_AT_data_member_location attribute : {}", dwarf_errno(err), dwarf_errmsg(err));

        return (uint64_t) offset;
    }
    else if(form == DW_FORM_sdata)
    {
        Dwarf_Signed soffset;
        if(dwarf_formsdata(*attr.get(), &soffset, &err) != DW_DLV_OK)
            return FAIL(ext::nullopt, "libdwarf error {} when reading DW_AT_data_member_location attribute : {}", dwarf_errno(err), dwarf_errmsg(err));

        if(soffset < 0)
            return FAIL(ext::nullopt, "unsupported negative offset for DW_AT_data_member_location attribute");

        return (uint64_t) soffset;
    }
    else
    {
        Dwarf_Locdesc** llbuf;
        Dwarf_Signed    listlen;
        if(dwarf_loclist_n(*attr.get(), &llbuf, &listlen, &err) != DW_DLV_OK)
        {
            dwarf_dealloc(dbg, &llbuf, DW_DLA_LOCDESC);
            return FAIL(ext::nullopt, "unsupported member offset in DW_AT_data_member_location attribute");
        }

        if(listlen != 1 || llbuf[0]->ld_cents != 1 || (llbuf[0]->ld_s[0]).lr_atom != DW_OP_plus_uconst)
        {
            dwarf_dealloc(dbg, &llbuf, DW_DLA_LOCDESC);
            return FAIL(ext::nullopt, "unsupported location expression in DW_AT_data_member_location attribute");
        }

        const auto offset = llbuf[0]->ld_s[0].lr_number;
        dwarf_dealloc(dbg, &llbuf, DW_DLA_LOCDESC);
        return (uint64_t) offset;
    }
}

opt<uint64_t> Dwarf::struc_offset(const std::string& struc, const std::string& member)
{
    const auto structure = get_structure(struc);
    if(!structure)
        return {};

    std::vector<Dwarf_Die> children;
    if(!read_children(*structure, children))
        return {};

    const auto child = get_structure(member, children);
    if(!child)
        return {};

    const auto offset = get_attr_member_location(*child);
    if(!offset)
        return {};

    return offset;
}

opt<size_t> Dwarf::struc_size(const std::string& struc)
{
    const auto structure = get_structure(struc);
    if(!structure)
        return {};

    return struc_size(*structure);
}

opt<size_t> Dwarf::struc_size(const Dwarf_Die& struc)
{
    Dwarf_Unsigned size;
    const auto ok = dwarf_bytesize(struc, &size, &err);

    if(ok == DW_DLV_ERROR)
        return FAIL(ext::nullopt, "libdwarf error {} when reading size of a DIE : {}", dwarf_errno(err), dwarf_errmsg(err));

    if(ok == DW_DLV_NO_ENTRY)
        return FAIL(ext::nullopt, "die has not DW_AT_byte_size attribute");

    return size;
}

opt<sym::ModCursor> Dwarf::symbol(uint64_t /*addr*/)
{
    return {};
}
