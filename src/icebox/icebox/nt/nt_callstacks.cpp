#include "callstacks.hpp"

#define FDP_MODULE "unwind"
#define PRIVATE_CORE__
#include "core.hpp"
#include "core/core_private.hpp"
#include "endian.hpp"
#include "interfaces/if_callstacks.hpp"
#include "log.hpp"
#include "nt_os.hpp"
#include "nt_types.hpp"
#include "utils/path.hpp"
#include "utils/pe.hpp"
#include "utils/utils.hpp"

#include <array>
#include <map>
#include <tuple>

namespace std
{
    template <>
    struct hash<proc_t>
    {
        size_t operator()(const proc_t& arg) const
        {
            return hash<uint64_t>()(arg.id);
        }
    };
} // namespace std

inline bool operator==(const proc_t& a, const proc_t& b)
{
    return a.id == b.id;
}

namespace
{

    enum class land_e
    {
        unknown,
        user,
        kernel,
        switched_k2u,
    };

    struct unwind_code_t
    {
        uint32_t stack_size_to_use;
        uint8_t  code_offset;
        uint8_t  unwind_op_and_info;
    };

    struct function_entry_t
    {
        uint32_t start_address;
        uint32_t end_address;
        uint32_t stack_frame_size;
        uint32_t mother_stack_frame_size;
        uint32_t prev_frame_reg;
        uint32_t mother_start_addr;
        uint32_t machframe_rip_off;
        uint8_t  prolog_size;
        uint8_t  frame_reg_offset;
        size_t   unwind_codes_idx;
        int      unwind_codes_nb;
    };

    using FunctionEntries = std::vector<function_entry_t>;
    using Unwinds         = std::vector<unwind_code_t>;

    struct FunctionTable
    {
        FunctionEntries function_entries;
        Unwinds         unwinds;
    };

    enum unwind_op_codes_e
    {
        UWOP_PUSH_NONVOL,     // info : register number
        UWOP_ALLOC_LARGE,     // no info, alloc size in next 2 slots
        UWOP_ALLOC_SMALL,     // info : size of allocation / 8 - 1
        UWOP_SET_FPREG,       // no info, FP = RSP + UNWIND_INFO.FPRegOffset*16
        UWOP_SAVE_NONVOL,     // info : register number, offset in next slot
        UWOP_SAVE_NONVOL_FAR, // info : register number, offset in next 2 slots
        RESERVED1,
        RESERVED2,
        UWOP_SAVE_XMM128,     // info : XMM reg number, offset in next slot
        UWOP_SAVE_XMM128_FAR, // info : XMM reg number, offset in next 2 slots
        UWOP_PUSH_MACHFRAME,  // info : 0: no error-code, 1: error-code
    };

    enum register_numbers_e
    {
        UWINFO_RAX,
        UWINFO_RCX,
        UWINFO_RDX,
        UWINFO_RBX,
        UWINFO_RSP,
        UWINFO_RBP,
        UWINFO_RSI,
        UWINFO_RDI,
    };

    enum offsets_e
    {
        TEB_NtTib,
        NT_TIB_StackBase,
        NT_TIB_StackLimit,
        OFFSET_COUNT,
    };

    struct UserOffset
    {
        offsets_e   e_id;
        const char* struc;
        const char* member;
    };
    // clang-format off
    const UserOffset g_user_offsets[] =
    {
        {TEB_NtTib,         "_TEB",     "NtTib"},
        {NT_TIB_StackBase,  "_NT_TIB",  "StackBase"},
        {NT_TIB_StackLimit, "_NT_TIB",  "StackLimit"},
    };
    // clang-format on
    STATIC_ASSERT_EQ(COUNT_OF(g_user_offsets), OFFSET_COUNT);

    const auto UNWIND_CHAINED_FLAG_MASK = 0b00100000;

    using UnwindInfo = std::array<uint8_t, 4>;

    struct RuntimeFunction
    {
        uint32_t start_address;
        uint32_t end_address;
        uint32_t unwind_info;
    };

    using Modules       = std::map<uint64_t, mod_t>;
    using Drivers       = std::map<uint64_t, driver_t>;
    using AllModules    = std::unordered_map<proc_t, Modules>;
    using ExceptionDirs = std::unordered_map<std::string, FunctionTable>;
    using UserOffsets   = std::array<uint64_t, OFFSET_COUNT>;
    using Buffer        = std::vector<uint8_t>;
    using caller_t      = callstacks::caller_t;
    using context_t     = callstacks::context_t;

    struct NtCallstacks
        : public callstacks::Module
    {
        NtCallstacks(core::Core& core);

        // callstacks::Module
        size_t  read        (caller_t* callers, size_t num_callers, proc_t proc) override;
        size_t  read_from   (caller_t* callers, size_t num_callers, proc_t proc, const context_t& where) override;
        bool    preload     (proc_t proc, const std::string& name, span_t span) override;

        // members
        core::Core&      core_;
        Drivers          all_drivers_;
        AllModules       all_modules_;
        ExceptionDirs    exception_dirs_;
        opt<UserOffsets> offsets_x64_;
        opt<UserOffsets> offsets_x86_;
        Buffer           buffer_;
    };
}

NtCallstacks::NtCallstacks(core::Core& core)
    : core_(core)
{
}

std::unique_ptr<callstacks::Module> callstacks::make_nt(core::Core& core)
{
    return std::make_unique<NtCallstacks>(core);
}

namespace
{
    bool compare_function_entries(const function_entry_t& a, const function_entry_t& b)
    {
        return a.start_address < b.start_address;
    }

    opt<int> get_unwind_nb_max(uint64_t off_in_mod, const FunctionTable& function_table, const function_entry_t& function_entry)
    {
        const auto off_in_prolog = off_in_mod - function_entry.start_address;
        if(off_in_prolog == 0)
            return 0;

        if(off_in_prolog > function_entry.prolog_size)
            return function_entry.unwind_codes_nb;

        const auto idx = function_entry.unwind_codes_idx;
        const auto nb  = function_entry.unwind_codes_nb;
        if(idx + nb >= function_table.unwinds.size())
            return {};

        for(auto i = 0; i < nb; ++i)
        {
            const auto uwd = function_table.unwinds[idx + i];
            if(off_in_prolog > uwd.code_offset)
                return i;
        }

        return {};
    }

    opt<uint64_t> get_stack_frame_size(uint64_t off_in_mod, const FunctionTable& function_table, const function_entry_t& function_entry)
    {
        const auto base_size = function_entry.mother_stack_frame_size;
        const auto opt_nb    = get_unwind_nb_max(off_in_mod, function_table, function_entry);
        if(!opt_nb)
            return {};

        const auto nb = *opt_nb;
        if(nb == 0)
            return base_size;

        if(nb == function_entry.unwind_codes_nb)
            return base_size + function_entry.stack_frame_size;

        const auto idx = function_entry.unwind_codes_idx;
        return base_size + function_entry.stack_frame_size - function_table.unwinds[idx + nb].stack_size_to_use;
    }

    opt<uint64_t> get_prev_frame_reg(uint64_t off_in_mod, const FunctionTable& function_table, const function_entry_t& function_entry)
    {
        const auto opt_nb = get_unwind_nb_max(off_in_mod, function_table, function_entry);
        if(!opt_nb)
            return {};

        const auto nb = *opt_nb;
        if(nb == 0)
            return 0;

        if(!function_entry.prev_frame_reg)
            return 0;

        if(nb == function_entry.unwind_codes_nb)
            return function_entry.prev_frame_reg;

        const auto idx = function_entry.unwind_codes_idx;
        return function_entry.prev_frame_reg - function_table.unwinds[idx + nb].stack_size_to_use;
    }

    template <typename T>
    opt<T> find_prev(const uint64_t addr, const std::map<uint64_t, T>& mods)
    {
        if(mods.empty())
            return {};

        // lower bound returns first item greater or equal
        auto it        = mods.lower_bound(addr);
        const auto end = mods.end();
        if(it == end)
            return mods.rbegin()->second;

        // equal
        if(it->first == addr)
            return it->second;

        if(it == mods.begin())
            return {};

        // strictly greater, go to previous item
        --it;
        return it->second;
    }

    Modules& get_modules(AllModules& all, proc_t proc)
    {
        auto it = all.find(proc);
        if(it == all.end())
            it = all.emplace(proc, Modules{}).first;
        return it->second;
    }

    opt<mod_t> find_mod(NtCallstacks& c, proc_t proc, uint64_t addr)
    {
        auto& modules = get_modules(c.all_modules_, proc);
        auto mod      = find_prev(addr, modules);
        if(mod)
        {
            const auto span = modules::span(c.core_, proc, *mod);
            if(addr <= span->addr + span->size)
                return mod;
        }

        mod = modules::find(c.core_, proc, addr);
        if(!mod)
            return {};

        const auto span = modules::span(c.core_, proc, *mod);
        modules.emplace(span->addr, *mod);
        return mod;
    }

    opt<driver_t> find_drv(NtCallstacks& c, uint64_t addr)
    {
        auto drv = find_prev(addr, c.all_drivers_);
        if(drv)
        {
            const auto span = drivers::span(c.core_, *drv);
            if(addr <= span->addr + span->size)
                return drv;
        }

        drv = drivers::find(c.core_, addr);
        if(!drv)
            return {};

        const auto span = drivers::span(c.core_, *drv);
        c.all_drivers_.emplace(span->addr, *drv);
        return drv;
    }

    void get_unwind_codes(Unwinds& unwind_codes, function_entry_t& function_entry, const uint8_t* buffer, size_t unwind_codes_size, size_t chained_info_size)
    {
        constexpr auto reg_size = 0x08; // TODO Defined this somewhere else
        size_t idx              = 0;
        while(idx < unwind_codes_size - chained_info_size)
        {
            const auto unwind_operation = buffer[idx + 1] & 0xF;
            const auto unwind_code_info = buffer[idx + 1] >> 4;
            switch(unwind_operation)
            {
                case UWOP_PUSH_NONVOL:
                    if(unwind_code_info == UWINFO_RBP)
                        function_entry.prev_frame_reg = function_entry.stack_frame_size;
                    function_entry.stack_frame_size += reg_size;
                    break;

                case UWOP_ALLOC_LARGE:
                    if(unwind_code_info == 0)
                    {
                        const auto extra_info = read_le16(&buffer[idx + 2]);
                        function_entry.stack_frame_size += extra_info * 8;
                        idx += 2;
                    }
                    else if(unwind_code_info == 1)
                    {
                        const auto extra_info = read_le32(&buffer[idx + 2]);
                        function_entry.stack_frame_size += extra_info;
                        idx += 4;
                    }
                    break;

                case UWOP_ALLOC_SMALL:
                    function_entry.stack_frame_size += unwind_code_info * 8 + 8;
                    break;

                case UWOP_SET_FPREG:
                    break;

                case UWOP_SAVE_NONVOL:
                    if(unwind_code_info == UWINFO_RBP)
                        function_entry.prev_frame_reg = 8 * read_le16(&buffer[idx + 2]);
                    idx += 2;
                    break;

                case UWOP_SAVE_NONVOL_FAR:
                    if(unwind_code_info == UWINFO_RBP)
                        function_entry.prev_frame_reg = read_le32(&buffer[idx + 2]);
                    idx += 4;
                    break;

                case UWOP_SAVE_XMM128:
                    idx += 2;
                    break;

                case UWOP_SAVE_XMM128_FAR:
                    idx += 4;
                    break;

                case UWOP_PUSH_MACHFRAME:
                    // should never be 2 UWOP_PUSH_MACHFRAME in a same prolog
                    // rip is pushed in 6th position (cf microsoft documentation)
                    function_entry.stack_frame_size += unwind_code_info ? 0x30 : 0x28;
                    function_entry.machframe_rip_off = 0x28;
                    break;

                default:
                    break;
            }

            unwind_codes.push_back({function_entry.stack_frame_size, buffer[idx], buffer[idx + 1]});
            idx += 2;
        }
    }

    opt<function_entry_t> lookup_mother_function_entry(uint32_t mother_start_addr, const std::vector<function_entry_t>& function_entries)
    {
        for(const auto& fe : function_entries)
            if(mother_start_addr == fe.start_address)
                return fe;

        return {};
    }

    opt<FunctionTable> parse_exception_dir(NtCallstacks& c, proc_t proc, uint64_t mod_base_addr, span_t exception_dir, const std::string& name)
    {
        if(!exception_dir.size)
            return {};

        auto& buf = c.buffer_;
        buf.resize(exception_dir.size);
        const auto io = memory::make_io(c.core_, proc);
        auto ok       = io.read_all(&buf[0], exception_dir.addr, exception_dir.size);
        if(!ok)
            return {};

        auto function_table          = FunctionTable{};
        auto orphan_function_entries = FunctionEntries{};
        const auto buf_offset        = buf.size();
        for(size_t i = 0; i < exception_dir.size; i = i + sizeof(RuntimeFunction))
        {
            auto function_entry          = function_entry_t{};
            function_entry.start_address = read_le32(&buf[i + offsetof(RuntimeFunction, start_address)]);
            function_entry.end_address   = read_le32(&buf[i + offsetof(RuntimeFunction, end_address)]);
            const auto unwind_info_ptr   = read_le32(&buf[i + offsetof(RuntimeFunction, unwind_info)]);

            auto unwind_info   = UnwindInfo{};
            const auto to_read = mod_base_addr + unwind_info_ptr;
            ok                 = io.read_all(&unwind_info[0], to_read, sizeof unwind_info);
            if(!ok)
                return FAIL(std::nullopt, "unable to read unwind info");

            const bool chained_flag        = unwind_info[0] & UNWIND_CHAINED_FLAG_MASK;
            function_entry.prolog_size     = unwind_info[1];
            function_entry.unwind_codes_nb = unwind_info[2];

            // Deal with frame register
            const auto frame_register       = unwind_info[3] & 0x0F;      // register used as frame pointer
            function_entry.frame_reg_offset = 16 * (unwind_info[3] >> 4); // offset of frame register
            if(function_entry.frame_reg_offset != 0 && frame_register != UWINFO_RBP)
                LOG(ERROR, "WARNING : in function %s+%x the used framed register is not rbp (code %d)", name.data(), function_entry.start_address, frame_register);

            const auto SIZE_UC           = size_t{2};
            const auto chained_info_size = chained_flag ? sizeof(RuntimeFunction) : 0;
            const auto unwind_codes_size = function_entry.unwind_codes_nb * SIZE_UC + chained_info_size;
            if(!unwind_codes_size)
            {
                function_entry.stack_frame_size = 0;
                function_table.function_entries.push_back(function_entry);
                continue;
            }

            buf.resize(buf_offset + unwind_codes_size);
            auto buffer = &buf[buf_offset];
            ok          = io.read_all(&buffer[0], mod_base_addr + unwind_info_ptr + sizeof unwind_info, unwind_codes_size);
            if(!ok)
                return FAIL(std::nullopt, "unable to read unwind codes");

            function_entry.unwind_codes_idx = function_table.unwinds.size();
            get_unwind_codes(function_table.unwinds, function_entry, &buffer[0], unwind_codes_size, chained_info_size);

            // Deal with the runtime func at the end
            uint32_t mother_start_addr = 0;
            if(chained_flag)
            {
                mother_start_addr                = read_le32(&buffer[unwind_codes_size - chained_info_size + offsetof(RuntimeFunction, start_address)]);
                const auto mother_function_entry = lookup_mother_function_entry(mother_start_addr, function_table.function_entries);
                if(!mother_function_entry)
                {
                    function_entry.mother_start_addr = mother_start_addr;
                    orphan_function_entries.push_back(function_entry);
                    continue;
                }

                function_entry.mother_stack_frame_size += mother_function_entry->stack_frame_size + mother_function_entry->mother_stack_frame_size;
                function_entry.prev_frame_reg = mother_function_entry->prev_frame_reg; // child_function_entry should not change frame register ?
            }

            function_table.function_entries.push_back(function_entry);

#ifdef USE_DEBUG_PRINT
            LOG(INFO, "Function entry : start 0x%" PRIx64 " end 0x%" PRIx64 " prolog size 0x%" PRIx64 " number of codes 0x%" PRIx64 " unwind info pointer 0x%" PRIx64 " stack frame size 0x%" PRIx64 ", previous frame reg 0x%" PRIx64, function_entry.start_address, function_entry.end_address, function_entry.prolog_size, function_entry.unwind_codes_nb,
                unwind_info_ptr, function_entry.stack_frame_size, function_entry.prev_frame_reg);
#endif
        }

        for(auto& orphan_fe : orphan_function_entries)
        {
            const auto mother_function_entry = lookup_mother_function_entry(orphan_fe.mother_start_addr, function_table.function_entries);
            if(!mother_function_entry)
                continue; // Should never happend

            orphan_fe.mother_stack_frame_size += mother_function_entry->stack_frame_size + mother_function_entry->mother_stack_frame_size;
            orphan_fe.prev_frame_reg = mother_function_entry->prev_frame_reg; // child_function_entry should not change frame register ?
            function_table.function_entries.push_back(orphan_fe);
        }

        std::sort(function_table.function_entries.begin(), function_table.function_entries.end(), &compare_function_entries);
        return function_table;
    }

    opt<FunctionTable> parse_module_unwind(NtCallstacks& c, proc_t proc, const std::string& name, const span_t span)
    {
        LOG(INFO, "loading %s", name.data());
        const auto io            = memory::make_io(c.core_, proc);
        const auto exception_dir = pe::find_image_directory(io, span, pe::IMAGE_DIRECTORY_ENTRY_EXCEPTION);
        if(!exception_dir)
            return FAIL(std::nullopt, "unable to get span of exception_dir");

        const auto function_table = parse_exception_dir(c, proc, span.addr, *exception_dir, name);
        if(!function_table)
            return FAIL(std::nullopt, "unable to parse exception dir from %s", name.data());

        const auto ret = c.exception_dirs_.emplace(name, *function_table);
        if(!ret.second)
            return {};

        return function_table;
    }

    opt<FunctionTable> get_module_unwind(NtCallstacks& c, proc_t proc, const std::string& name, const span_t span)
    {
        const auto it = c.exception_dirs_.find(name);
        if(it != c.exception_dirs_.end())
            return it->second;

        return parse_module_unwind(c, proc, name, span);
    }

    bool read_user_offsets(NtCallstacks& c, flags_t flags)
    {
        auto& opt_offsets = flags.is_x86 ? c.offsets_x86_ : c.offsets_x64_;
        if(opt_offsets)
            return true;

        const auto name = flags.is_x86 ? "wntdll" : "ntdll";
        bool fail       = false;
        auto offsets    = UserOffsets{};
        for(size_t i = 0; i < OFFSET_COUNT; ++i)
        {
            fail |= g_user_offsets[i].e_id != i;
            const auto opt_mb = symbols::read_member(c.core_, symbols::kernel, name, g_user_offsets[i].struc, g_user_offsets[i].member);
            if(!opt_mb)
            {
                LOG(ERROR, "unable to read %s!%s.%s member offset", name, g_user_offsets[i].struc, g_user_offsets[i].member);
                continue;
            }
            offsets[i] = opt_mb->offset;
        }
        if(fail)
            return false;

        opt_offsets = offsets;
        return true;
    }

    uint64_t user_offset(const NtCallstacks& c, flags_t flags, offsets_e off)
    {
        const auto& offsets = flags.is_x86 ? *c.offsets_x86_ : *c.offsets_x64_;
        return offsets[off];
    }

    opt<span_t> get_user_stack(NtCallstacks& c, const memory::Io& io, const context_t& ctx)
    {
        if(!read_user_offsets(c, ctx.flags))
            return FAIL(std::nullopt, "unable to read ntdll offsets");

        const auto is_kernel_ctx = !(ctx.cs & 0x03);
        const auto msr_read      = ctx.flags.is_x86 ? msr_e::fs_base : (is_kernel_ctx ? msr_e::kernel_gs_base : msr_e::gs_base);
        const auto teb           = registers::read_msr(c.core_, msr_read);
        const auto nt_tib        = teb + user_offset(c, ctx.flags, TEB_NtTib);
        auto base                = io.read(nt_tib + user_offset(c, ctx.flags, NT_TIB_StackBase));
        auto limit               = io.read(nt_tib + user_offset(c, ctx.flags, NT_TIB_StackLimit));
        if(!base || !limit)
            return FAIL(std::nullopt, "unable to find stack boundaries");

        if(ctx.flags.is_x86)
        {
            *base  = static_cast<uint32_t>(*base);
            *limit = static_cast<uint32_t>(*limit);
        }
        return span_t{*base, *base - *limit};
    }

    opt<span_t> get_kernel_stack(NtCallstacks& c, const memory::Io& io, const context_t& ctx)
    {
        const auto is_kernel_ctx  = !(ctx.cs & 0x03);
        const auto msr_read       = is_kernel_ctx ? msr_e::gs_base : msr_e::kernel_gs_base;
        const auto kernel_gs_base = registers::read_msr(c.core_, msr_read);
        const auto& koffsets      = c.core_.nt_->offsets_;
        const auto rsp_base       = kernel_gs_base + koffsets[KPCR_Prcb] + koffsets[KPRCB_RspBase];
        auto base                 = io.read(rsp_base);
        if(!base)
            return {};

        return span_t{*base, *base - 0xFFFF800000000000};
    }

    opt<span_t> get_stack(NtCallstacks& c, const memory::Io& io, const context_t& ctx)
    {
        if(os::is_kernel_address(c.core_, ctx.ip))
            return get_kernel_stack(c, io, ctx);

        return get_user_stack(c, io, ctx);
    }

    opt<std::tuple<std::string, span_t>> get_name_span(NtCallstacks& c, proc_t proc, const context_t& ctx)
    {
        if(os::is_kernel_address(c.core_, ctx.ip))
        {
            const auto drv = find_drv(c, ctx.ip);
            if(!drv)
                return {};

            auto name = drivers::name(c.core_, *drv);
            auto span = drivers::span(c.core_, *drv);
            if(!name || !span)
                return {};

            return std::make_tuple(*name, *span);
        }

        const auto mod = find_mod(c, proc, ctx.ip);
        if(!mod)
            return {};

        auto name = modules::name(c.core_, proc, *mod);
        auto span = modules::span(c.core_, proc, *mod);
        if(!name || !span)
            return {};

        return std::make_tuple(*name, *span);
    }

    template <typename T>
    const function_entry_t* check_previous_exist(const T& it, const T& end, const uint32_t offset_in_mod)
    {
        if(it == end)
            return nullptr;

        if(offset_in_mod > it->end_address)
            return nullptr;

        return &*it;
    }

    const function_entry_t* lookup_function_entry(uint32_t offset_in_mod, const FunctionEntries& function_entries)
    {
        // lower bound returns first item greater or equal
        auto entry          = function_entry_t{};
        entry.start_address = offset_in_mod;
        auto it             = std::lower_bound(function_entries.begin(), function_entries.end(), entry, compare_function_entries);
        const auto end      = function_entries.end();
        if(it == end)
            return check_previous_exist(function_entries.rbegin(), function_entries.rend(), offset_in_mod);

        // equal
        if(it->start_address == offset_in_mod)
            return check_previous_exist(it, end, offset_in_mod);

        if(it == function_entries.begin())
            return nullptr;

        // strictly greater, go to previous item
        return check_previous_exist(--it, end, offset_in_mod);
    }

    bool get_next_context_x64(NtCallstacks& c, proc_t proc, const memory::Io& io, const span_t& stack, context_t& ctx)
    {
        constexpr auto reg_size = 8;

        // Get module from address
        const auto tuple = get_name_span(c, proc, ctx);
        if(!tuple)
            return false;

        auto [name, span] = *tuple;
        // Get function table of the module
        const auto function_table = get_module_unwind(c, proc, name, span);
        if(!function_table)
            return false;

        const auto off_in_mod     = static_cast<uint32_t>(ctx.ip - span.addr);
        const auto function_entry = lookup_function_entry(off_in_mod, function_table->function_entries);
        if(!function_entry)
            return FAIL(false, "No matching function entry");

        const auto stack_frame_size = get_stack_frame_size(off_in_mod, *function_table, *function_entry);
        if(!stack_frame_size)
            return FAIL(false, "cannot calculate stack frame size");

        if(function_entry->frame_reg_offset != 0)
            ctx.sp = ctx.bp - function_entry->frame_reg_offset;

        const auto prev_frame_reg = get_prev_frame_reg(off_in_mod, *function_table, *function_entry);
        if(!prev_frame_reg)
            return FAIL(false, "cannot calculate previous frame register offset");

        if(*prev_frame_reg != 0)
            if(const auto bp = io.read(ctx.sp + *prev_frame_reg))
                ctx.bp = *bp;

        const auto caller_addr_on_stack = ctx.sp + *stack_frame_size - function_entry->machframe_rip_off;

        // Check if caller's address on stack is consistent, if not we suppose that the end of the callstack has been reached
        if(!(stack.addr > caller_addr_on_stack && caller_addr_on_stack > (stack.addr - stack.size)))
            return false;

        const auto return_addr = io.read(caller_addr_on_stack);
        if(!return_addr)
            return FAIL(false, "unable to read return address at 0x%" PRIx64, caller_addr_on_stack);

        // end of callstack
        if(!*return_addr)
            return false;

#ifdef USE_DEBUG_PRINT
        // print stack
        const auto print_d = 25;
        for(int k = -3 * reg_size; k < print_d * reg_size; k += reg_size)
        {
            LOG(INFO, "0x%" PRIx64 " - 0x%" PRIx64, ctx.rsp + *stack_frame_size + k, *core::read_ptr(core_, ctx.rsp + *stack_frame_size + k));
        }
        LOG(INFO, "Chosen chosen 0x%" PRIx64 " start address 0x%" PRIx64 " end 0x%" PRIx64, off_in_mod, function_entry->start_address, function_entry->end_address);
        LOG(INFO, "Offset of current func 0x%" PRIx64 ", Caller address on stack 0x%" PRIx64 " so 0x%" PRIx64, off_in_mod, caller_addr_on_stack, *return_addr);
#endif

        ctx.ip = *return_addr;
        ctx.sp = caller_addr_on_stack + reg_size;
        return true;
    }

    bool get_next_context_x86(NtCallstacks& /*c*/, proc_t /*proc*/, const memory::Io& io, const span_t& stack, context_t& ctx)
    {
        constexpr auto reg_size = 4;
        if(!ctx.bp)
            return false;

        if(!(stack.addr < ctx.bp && ctx.bp < (stack.addr - stack.size)))
            return FAIL(false, "ebp out of stack bounds, ebp: 0x%" PRIx64 " stack bounds: 0x%" PRIx64 "-0x%" PRIx64, ctx.bp, stack.addr, stack.addr + stack.size);

        const auto caller_addr_on_stack = io.le32(ctx.bp);
        if(!caller_addr_on_stack)
            return FAIL(false, "unable to read caller address on stack at 0x%" PRIx64, ctx.bp);

        const auto return_addr = io.le32(ctx.bp + reg_size);
        if(!return_addr)
            return FAIL(false, "unable to read return address at 0x%" PRIx64, ctx.bp + reg_size);

        ctx.ip = *return_addr;
        ctx.bp = *caller_addr_on_stack;
        return true;
    }

    bool get_state(NtCallstacks& c, const context_t& ctx, land_e& land)
    {
        switch(land)
        {
            case land_e::unknown:
                land = os::is_kernel_address(c.core_, ctx.ip) ? land_e::kernel : land_e::user;
                return true;
            case land_e::kernel:
                land = os::is_kernel_address(c.core_, ctx.ip) ? land_e::kernel : land_e::switched_k2u;
                return true;
            case land_e::switched_k2u:
                if(os::is_kernel_address(c.core_, ctx.ip))
                    return false;
                land = land_e::user;
                return true;
            case land_e::user:
                return !os::is_kernel_address(c.core_, ctx.ip);
        }
        return false;
    }

    bool switch_ctx_x86(NtCallstacks& c, const memory::Io& io, span_t& stack, context_t& ctx)
    {
        // disable this ctx switch until it is tested
        if(true)
            return false;

        const auto opt_stack = get_stack(c, io, ctx);
        if(!opt_stack)
            return false;

        stack = *opt_stack;
        // wow64 context is stored by wow64cpu.dll
        const auto teb        = registers::read_msr(c.core_, msr_e::gs_base); // always in kernel ctx here
        const auto opt_member = symbols::read_member(c.core_, symbols::kernel, "nt", "_TEB", "TlsSlots");
        if(!opt_member)
            return false;

        const auto TlsSlot       = teb + opt_member->offset + 8;
        const auto WOW64_CONTEXT = io.read(TlsSlot);
        if(!WOW64_CONTEXT)
            return false;

        if(false)
            LOG(INFO, "TEB is: 0x%" PRIx64 ", r12 is: 0x%" PRIx64 ", r13 is: 0x%" PRIx64, teb, TlsSlot, *WOW64_CONTEXT);

        auto offset = 4; // FIXME: why 4 on 10240 ?
        nt_types::_WOW64_CONTEXT wow64ctx;
        const auto ok = io.read_all(&wow64ctx, *WOW64_CONTEXT + offset, sizeof wow64ctx);
        if(!ok)
            return false;

        const auto ebp   = static_cast<uint64_t>(wow64ctx.Ebp);
        const auto eip   = static_cast<uint64_t>(wow64ctx.Eip);
        const auto esp   = static_cast<uint64_t>(wow64ctx.Esp);
        const auto segcs = static_cast<uint64_t>(wow64ctx.SegCs);
        ctx              = context_t{eip, esp, ebp, segcs, flags::x86};
        return true;
    }

    opt<uint64_t> switch_bp_x64(NtCallstacks& c, proc_t proc, const memory::Io& io)
    {
        const auto kernel_gs_base = registers::read_msr(c.core_, msr_e::gs_base);
        const auto& koffsets      = c.core_.nt_->offsets_;
        const auto& symbols       = c.core_.nt_->symbols_;
        const auto shadow         = symbols[KiKvaShadow] ? io.read(*symbols[KiKvaShadow]) : 0;
        const auto rsp_base_off   = shadow ? koffsets[KPRCB_RspBaseShadow] : koffsets[KPRCB_RspBase];
        const auto rsp_base       = kernel_gs_base + koffsets[KPCR_Prcb] + rsp_base_off;
        auto base                 = io.read(rsp_base);
        if(!base)
            return {};

        auto fake_ctx    = context_t{};
        fake_ctx.ip      = shadow ? *symbols[KiSystemCall64Shadow] : *symbols[KiSystemCall64];
        const auto tuple = get_name_span(c, proc, fake_ctx);
        if(!tuple)
            return {};

        auto [name, span]         = *tuple;
        const auto function_table = get_module_unwind(c, proc, name, span);
        if(!function_table)
            return {};

        const auto off_in_mod     = static_cast<uint32_t>(fake_ctx.ip - span.addr);
        const auto function_entry = lookup_function_entry(off_in_mod, function_table->function_entries);
        if(!function_entry)
            return FAIL(std::nullopt, "No matching function entry");

        const auto bp = io.read(*base - function_entry->stack_frame_size + function_entry->prev_frame_reg);
        if(!bp)
            return {};

        return bp;
    }

    opt<uint64_t> switch_sp_x64(NtCallstacks& c, const memory::Io& io)
    {
        const auto& symbols = c.core_.nt_->symbols_;
        const auto shadow   = symbols[KiKvaShadow] ? io.read(*symbols[KiKvaShadow]) : 0;
        if(shadow)
        {
            const auto kernel_gs_base = registers::read_msr(c.core_, msr_e::gs_base);
            const auto& koffsets      = c.core_.nt_->offsets_;
            const auto user_rsp       = kernel_gs_base + koffsets[KPCR_Prcb] + koffsets[KPRCB_UserRspShadow];
            return io.read(user_rsp);
        }

        const auto teb    = registers::read_msr(c.core_, msr_e::gs_base);
        const auto nt_tib = teb + user_offset(c, flags::x64, TEB_NtTib);
        return io.read(nt_tib + user_offset(c, flags::x64, NT_TIB_StackLimit));
    }

    bool switch_ctx_x64(NtCallstacks& c, proc_t proc, const memory::Io& io, span_t& stack, context_t& ctx)
    {
        const auto opt_stack = get_stack(c, io, ctx);
        if(!opt_stack)
            return false;

        stack = *opt_stack;

        const auto sp = switch_sp_x64(c, io);
        if(!sp)
            return false;

        ctx.sp        = *sp;
        const auto bp = switch_bp_x64(c, proc, io);
        if(!bp)
            return false;

        ctx.bp = *bp;
        return true;
    }

    bool switch_ctx(NtCallstacks& c, proc_t proc, const memory::Io& io, span_t& stack, context_t& ctx)
    {
        const auto flags = process::flags(c.core_, proc);
        if(flags.is_x86)
            return switch_ctx_x86(c, io, stack, ctx);

        return switch_ctx_x64(c, proc, io, stack, ctx);
    }

    size_t read_callers(NtCallstacks& c, caller_t* callers, size_t num_callers, proc_t proc, const context_t& first)
    {
        const auto io        = memory::make_io(c.core_, proc);
        const auto opt_stack = get_stack(c, io, first);
        if(!opt_stack)
            return 0;

        auto stack      = *opt_stack;
        auto ctx        = first;
        callers[0].addr = ctx.ip;

        auto land = land_e::unknown;
        get_state(c, ctx, land);

        const auto next_context = first.flags.is_x86 ? &get_next_context_x86 : &get_next_context_x64;
        for(size_t i = 1; i < num_callers; ++i)
        {
            auto ok = next_context(c, proc, io, stack, ctx);
            if(!ok)
                return i;

            callers[i].addr = ctx.ip;

            ok = get_state(c, ctx, land);
            if(!ok)
                return i;

            if(land != land_e::switched_k2u)
                continue;

            ok = switch_ctx(c, proc, io, stack, ctx);
            if(!ok)
                return i;
        }
        return num_callers;
    }
}

size_t NtCallstacks::read_from(caller_t* callers, size_t num_callers, proc_t proc, const context_t& where)
{
    memset(callers, 0, num_callers * sizeof *callers);
    return read_callers(*this, callers, num_callers, proc, where);
}

size_t NtCallstacks::read(caller_t* callers, size_t num_callers, proc_t proc)
{
    const auto ip         = registers::read(core_, reg_e::rip);
    const auto sp         = registers::read(core_, reg_e::rsp);
    const auto bp         = registers::read(core_, reg_e::rbp);
    const auto cs         = registers::read(core_, reg_e::cs);
    constexpr auto x86_cs = 0x23;
    const auto flags      = cs == x86_cs ? flags::x86 : flags::x64;
    const auto ctx        = context_t{ip, sp, bp, cs, flags};
    return read_from(callers, num_callers, proc, ctx);
}

bool NtCallstacks::preload(proc_t proc, const std::string& name, span_t span)
{
    const auto opt_entries = get_module_unwind(*this, proc, name, span);
    return !!opt_entries;
}
