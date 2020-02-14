#include "symbols.hpp"

#define FDP_MODULE "dwarf"
#include "indexer.hpp"
#include "interfaces/if_symbols.hpp"
#include "log.hpp"
#include "utils/utils.hpp"

#include <dwarf.h>
#include <libdwarf.h>

namespace
{
    using Handler = std::shared_ptr<Dwarf_Debug_s>;

    void CloseHandler(Dwarf_Debug dbg)
    {
        if(!dbg)
            return;

        auto err = Dwarf_Error{};
        dwarf_finish(dbg, &err);
    }

    Handler open_file(const fs::path& path)
    {
        auto dbg       = Dwarf_Debug{};
        auto error     = Dwarf_Error{};
        const auto err = dwarf_init_path(
            path.generic_string().data(), // path
            nullptr,                      // true_path_out_buffer
            0,                            // true_path_bufferlen
            DW_DLC_READ,                  // access
            DW_GROUPNUMBER_ANY,           // groupnumber
            nullptr,                      // errhand
            nullptr,                      // errarg
            &dbg,                         // ret_dbg
            nullptr,                      // reserved1
            0,                            // reserved2
            nullptr,                      // reserved3
            &error                        // error
        );
        if(err == DW_DLV_ERROR)
            return FAIL(nullptr, "libdwarf error %llu when initializing dwarf file : %s", dwarf_errno(error), dwarf_errmsg(error));

        if(err == DW_DLV_NO_ENTRY)
            return FAIL(nullptr, "unfound file or dwarf information in file '%s'", path.generic_string().data());

        return Handler(dbg, &CloseHandler);
    }

    template <typename T>
    bool read_cu(Dwarf_Debug_s& dbg, T on_die)
    {
        auto error = Dwarf_Error{};
        while(true)
        {
            auto offset = Dwarf_Unsigned{};
            auto err    = dwarf_next_cu_header_d(
                &dbg,    // dbg
                true,    // is_info
                nullptr, // cu_header_length
                nullptr, // version_stamp
                nullptr, // abbrev_offset
                nullptr, // address_size
                nullptr, // length_size
                nullptr, // extension_size
                nullptr, // type signature
                nullptr, // typeoffset
                &offset, // next_cu_header_offset
                nullptr, // header_cu_type
                &error   // error
            );

            if(err == DW_DLV_ERROR)
                return FAIL(false, "libdwarf error %llu when reading dwarf file : %s", dwarf_errno(error), dwarf_errmsg(error));

            if(err == DW_DLV_NO_ENTRY)
                return true;

            auto die = Dwarf_Die{};
            err      = dwarf_siblingof_b(&dbg, nullptr, true, &die, &error);
            if(err == DW_DLV_NO_ENTRY)
                continue;

            if(err == DW_DLV_ERROR)
                return FAIL(false, "libdwarf error %llu when reading dwarf file : %s", dwarf_errno(error), dwarf_errmsg(error));

            on_die(die);
        }
    }

    template <typename T>
    bool read_children(Dwarf_Debug_s& dbg, const Dwarf_Die& parent, T on_child)
    {
        auto error = Dwarf_Error{};
        auto child = Dwarf_Die{};
        auto err   = dwarf_child(parent, &child, &error);
        while(err != DW_DLV_NO_ENTRY)
        {
            if(err == DW_DLV_ERROR)
                return FAIL(false, "libdwarf error %llu when reading dwarf file : %s", dwarf_errno(error), dwarf_errmsg(error));

            if(on_child(child) == walk_e::stop)
                break;

            err = dwarf_siblingof_b(&dbg, child, true, &child, &error);
        }
        return true;
    }

    const char* read_die_name(Dwarf_Die die)
    {
        auto error    = Dwarf_Error{};
        auto name_ptr = static_cast<char*>(nullptr);
        auto err      = dwarf_diename(die, &name_ptr, &error);
        if(err == DW_DLV_ERROR)
            return FAIL(nullptr, "error dwarf_diename: %llu: %s", dwarf_errno(error), dwarf_errmsg(error));

        if(err != DW_DLV_OK)
            return nullptr;

        return name_ptr;
    }

    Dwarf_Die read_die_child(Dwarf_Debug_s& dbg, Dwarf_Die die, const char* name)
    {
        auto error  = Dwarf_Error{};
        auto offset = Dwarf_Off{};
        auto err    = dwarf_dietype_offset(die, &offset, &error);
        if(err != DW_DLV_OK)
            return die;

        err = dwarf_offdie_b(&dbg, offset, true, &die, &error);
        if(err != DW_DLV_OK)
            return FAIL(nullptr, "error dwarf_offdie_b: %llu: %s: name:%s offset:0%llu", dwarf_errno(error), dwarf_errmsg(error), name, offset);

        return die;
    }

    opt<uint64_t> read_member_offset(Dwarf_Debug_s& dbg, Dwarf_Die die)
    {
        auto error = Dwarf_Error{};
        auto attr  = Dwarf_Attribute{};
        auto err   = dwarf_attr(die, DW_AT_data_member_location, &attr, &error);
        if(err == DW_DLV_ERROR)
            return FAIL(std::nullopt, "libdwarf error %llu when reading attributes of a DIE : %s", dwarf_errno(error), dwarf_errmsg(error));

        if(err == DW_DLV_NO_ENTRY)
            return std::nullopt;

        auto form = Dwarf_Half{};
        err       = dwarf_whatform(attr, &form, &error);
        if(err != DW_DLV_OK)
            return FAIL(std::nullopt, "libdwarf error %llu when reading form of DW_AT_data_member_location attribute : %s", dwarf_errno(error), dwarf_errmsg(error));

        if(form == DW_FORM_data1 || form == DW_FORM_data2 || form == DW_FORM_data4 || form == DW_FORM_data8 || form == DW_FORM_udata)
        {
            auto offset = Dwarf_Unsigned{};
            err         = dwarf_formudata(attr, &offset, &error);
            if(err != DW_DLV_OK)
                return FAIL(std::nullopt, "libdwarf error %llu when reading DW_AT_data_member_location attribute : %s", dwarf_errno(error), dwarf_errmsg(error));

            return offset;
        }

        if(form == DW_FORM_sdata)
        {
            auto offset = Dwarf_Signed{};
            err         = dwarf_formsdata(attr, &offset, &error);
            if(err != DW_DLV_OK)
                return FAIL(std::nullopt, "libdwarf error %llu when reading DW_AT_data_member_location attribute : %s", dwarf_errno(error), dwarf_errmsg(error));

            if(offset < 0)
                return FAIL(std::nullopt, "unsupported negative offset for DW_AT_data_member_location attribute");

            return offset;
        }

        auto llbuf   = static_cast<Dwarf_Locdesc**>(nullptr);
        auto listlen = Dwarf_Signed{};
        err          = dwarf_loclist_n(attr, &llbuf, &listlen, &error);
        if(err != DW_DLV_OK)
        {
            dwarf_dealloc(&dbg, &llbuf, DW_DLA_LOCDESC);
            return FAIL(std::nullopt, "unsupported member offset in DW_AT_data_member_location attribute");
        }

        if(listlen != 1 || llbuf[0]->ld_cents != 1 || llbuf[0]->ld_s->lr_atom != DW_OP_plus_uconst)
        {
            dwarf_dealloc(&dbg, &llbuf, DW_DLA_LOCDESC);
            return FAIL(std::nullopt, "unsupported location expression in DW_AT_data_member_location attribute");
        }

        const auto offset = llbuf[0]->ld_s->lr_number;
        dwarf_dealloc(&dbg, &llbuf, DW_DLA_LOCDESC);
        return offset;
    }

    opt<size_t> read_struc_size(Dwarf_Die struc)
    {
        auto error     = Dwarf_Error{};
        auto size      = Dwarf_Unsigned{};
        const auto err = dwarf_bytesize(struc, &size, &error);
        if(err == DW_DLV_ERROR)
            return FAIL(std::nullopt, "libdwarf error %llu when reading size of a DIE : %s", dwarf_errno(error), dwarf_errmsg(error));

        if(err == DW_DLV_NO_ENTRY)
            return {};

        return size;
    }

    template <typename T>
    void all_strucs(Dwarf_Debug_s& dbg, const T& on_struc)
    {
        read_cu(dbg, [&](Dwarf_Die cu)
        {
            read_children(dbg, cu, [&](Dwarf_Die child)
            {
                const auto name = read_die_name(child);
                if(!name)
                    return walk_e::next;

                const auto ok = on_struc(child, name);
                return ok ? walk_e::next : walk_e::stop;
            });
        });
    }

    template <typename T>
    void all_members(Dwarf_Debug_s& dbg, Dwarf_Die struc, const T& on_member)
    {
        read_children(dbg, struc, [&](Dwarf_Die child)
        {
            auto error     = Dwarf_Error{};
            auto name_ptr  = static_cast<char*>(nullptr);
            const auto err = dwarf_diename(child, &name_ptr, &error);
            if(err == DW_DLV_ERROR)
                return FAIL(walk_e::next, "error dwarf_diename: %llu: %s", dwarf_errno(error), dwarf_errmsg(error));

            if(err == DW_DLV_OK)
                return on_member(child);

            if(const auto sub = read_die_child(dbg, child, ""))
                if(sub != child)
                    all_members(dbg, sub, on_member);

            return walk_e::next;
        });
    }

    bool setup(symbols::Indexer& indexer, const fs::path& path)
    {
        const auto dbg = open_file(path);
        if(!dbg)
            return false;

        const auto on_member = [&](auto& struc, Dwarf_Die member, std::string_view name)
        {
            const auto opt_offset = read_member_offset(*dbg, member);
            const auto offset     = opt_offset ? *opt_offset : (uint32_t) -1;
            indexer.add_member(struc, name, offset);
        };
        all_strucs(*dbg, [&](Dwarf_Die struc, std::string_view name)
        {
            struc               = read_die_child(*dbg, struc, name.data());
            const auto opt_size = read_struc_size(struc);
            const auto size     = opt_size ? *opt_size : -1;
            auto has_members    = false;
            // skip strucs without members
            all_members(*dbg, struc, [&](Dwarf_Die member)
            {
                has_members = !!read_die_name(member);
                return has_members ? walk_e::stop : walk_e::next;
            });
            if(!has_members)
                return true;

            auto& idx = indexer.add_struc(name, size);
            all_members(*dbg, struc, [&](Dwarf_Die member)
            {
                auto mname = read_die_name(member);
                if(mname)
                    on_member(idx, member, mname);
                return walk_e::next;
            });
            return true;
        });
        return true;
    }
}

std::shared_ptr<symbols::Module> symbols::make_dwarf(const std::string& module, const std::string& guid)
{
    const auto path = getenv("_LINUX_SYMBOL_PATH");
    if(!path)
        return nullptr;

    auto indexer = symbols::make_indexer(guid);
    if(!indexer)
        return nullptr;

    const auto ok = setup(*indexer, fs::path(path) / module / guid / "elf");
    if(!ok)
        return nullptr;

    indexer->finalize();
    return indexer;
}
