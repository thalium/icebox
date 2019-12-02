#include "callstacks.hpp"

#define FDP_MODULE "unwind"
#include "core.hpp"
#include "endian.hpp"
#include "interfaces/if_callstacks.hpp"
#include "log.hpp"
#include "reader.hpp"
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

static inline bool operator==(const proc_t& a, const proc_t& b)
{
    return a.id == b.id;
}

namespace
{
    struct unwind_code_t
    {
        uint32_t stack_size_used;
        uint8_t  code_offset;
        uint8_t  unwind_op_and_info;
    };

    struct function_entry_t
    {
        uint32_t start_address;
        uint32_t end_address;
        uint32_t stack_frame_size;
        uint32_t prev_frame_reg;
        uint32_t mother_start_addr;
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

    struct NtOffset
    {
        offsets_e  e_id;
        const char struc[32];
        const char member[32];
    };
    // clang-format off
    const NtOffset g_nt_offsets[] =
    {
        {TEB_NtTib,         "_TEB",     "NtTib"},
        {NT_TIB_StackBase,  "_NT_TIB",  "StackBase"},
        {NT_TIB_StackLimit, "_NT_TIB",  "StackLimit"},
    };
    // clang-format on
    STATIC_ASSERT_EQ(COUNT_OF(g_nt_offsets), OFFSET_COUNT);

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
    using Offsets       = std::array<uint64_t, OFFSET_COUNT>;
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
        core::Core&   core_;
        Drivers       all_drivers_;
        AllModules    all_modules_;
        ExceptionDirs exception_dirs_;
        opt<Offsets>  offsets64_;
        opt<Offsets>  offsets32_;
        Buffer        buffer_;
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
    static bool compare_function_entries(const function_entry_t& a, const function_entry_t& b)
    {
        return a.start_address < b.start_address;
    }

    static opt<uint64_t> get_stack_frame_size(uint64_t off_in_mod, const FunctionTable& function_table, const function_entry_t& function_entry)
    {
        const auto off_in_prolog = off_in_mod - function_entry.start_address;
        if(off_in_prolog == 0)
            return 0;

        if(off_in_prolog > function_entry.prolog_size)
            return function_entry.stack_frame_size;

        const auto idx = function_entry.unwind_codes_idx;
        const auto nb  = function_entry.unwind_codes_nb;
        if(idx + nb >= function_table.unwinds.size())
            return {};

        for(auto it = &function_table.unwinds[idx]; it < &function_table.unwinds[idx + nb]; ++it)
            if(off_in_prolog > it->code_offset)
                return function_entry.stack_frame_size - it->stack_size_used;

        return {};
    }

    template <typename T>
    static opt<T> find_prev(const uint64_t addr, const std::map<uint64_t, T>& mods)
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

    static Modules& get_modules(AllModules& all, proc_t proc)
    {
        auto it = all.find(proc);
        if(it == all.end())
            it = all.emplace(proc, Modules{}).first;
        return it->second;
    }

    static opt<mod_t> find_mod(NtCallstacks& c, proc_t proc, uint64_t addr)
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

    static opt<driver_t> find_drv(NtCallstacks& c, uint64_t addr)
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

    static void get_unwind_codes(Unwinds& unwind_codes, function_entry_t& function_entry, const uint8_t* buffer, size_t unwind_codes_size, size_t chained_info_size)
    {
        static const auto reg_size = 0x08; // TODO Defined this somewhere else
        size_t idx                 = 0;
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

                default:
                case UWOP_PUSH_MACHFRAME:
                    break;
            }

            unwind_codes.push_back({function_entry.stack_frame_size, buffer[idx], buffer[idx + 1]});
            idx += 2;
        }
    }

    static opt<function_entry_t> lookup_mother_function_entry(uint32_t mother_start_addr, const std::vector<function_entry_t>& function_entries)
    {
        for(const auto& fe : function_entries)
            if(mother_start_addr == fe.start_address)
                return fe;

        return {};
    }

    static opt<FunctionTable> parse_exception_dir(NtCallstacks& c, proc_t proc, uint64_t mod_base_addr, span_t exception_dir)
    {
        if(!exception_dir.size)
            return {};

        auto& buf = c.buffer_;
        buf.resize(exception_dir.size);
        const auto reader = reader::make(c.core_, proc);
        auto ok           = reader.read_all(&buf[0], exception_dir.addr, exception_dir.size);
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
            ok                 = reader.read_all(&unwind_info[0], to_read, sizeof unwind_info);
            if(!ok)
                return FAIL(ext::nullopt, "unable to read unwind info");

            const bool chained_flag        = unwind_info[0] & UNWIND_CHAINED_FLAG_MASK;
            function_entry.prolog_size     = unwind_info[1];
            function_entry.unwind_codes_nb = unwind_info[2];

            // Deal with frame register
            const auto frame_register       = unwind_info[3] & 0x0F;      // register used as frame pointer
            function_entry.frame_reg_offset = 16 * (unwind_info[3] >> 4); // offset of frame register
            if(function_entry.frame_reg_offset != 0 && frame_register != UWINFO_RBP)
                LOG(ERROR, "WARNING : the used framed register is not rbp (code %d), this case is never used and not implemented", frame_register);

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
            ok          = reader.read_all(&buffer[0], mod_base_addr + unwind_info_ptr + sizeof unwind_info, unwind_codes_size);
            if(!ok)
                return FAIL(ext::nullopt, "unable to read unwind codes");

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

                function_entry.stack_frame_size += mother_function_entry->stack_frame_size;
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

            orphan_fe.stack_frame_size += mother_function_entry->stack_frame_size;
            function_table.function_entries.push_back(orphan_fe);
        }

        std::sort(function_table.function_entries.begin(), function_table.function_entries.end(), &compare_function_entries);
        return function_table;
    }

    static opt<FunctionTable> parse_module_unwind(NtCallstacks& c, proc_t proc, const std::string& name, const span_t span)
    {
        LOG(INFO, "loading %s", name.data());
        const auto reader        = reader::make(c.core_, proc);
        const auto exception_dir = pe::find_image_directory(reader, span, pe::IMAGE_DIRECTORY_ENTRY_EXCEPTION);
        if(!exception_dir)
            return FAIL(ext::nullopt, "unable to get span of exception_dir");

        const auto function_table = parse_exception_dir(c, proc, span.addr, *exception_dir);
        if(!function_table)
            return FAIL(ext::nullopt, "unable to parse exception dir from %s", name.data());

        const auto ret = c.exception_dirs_.emplace(name, *function_table);
        if(!ret.second)
            return {};

        return function_table;
    }

    static opt<FunctionTable> get_module_unwind(NtCallstacks& c, proc_t proc, const std::string& name, const span_t span)
    {
        const auto it = c.exception_dirs_.find(name);
        if(it != c.exception_dirs_.end())
            return it->second;

        return parse_module_unwind(c, proc, name, span);
    }

    static bool load_ntdll(core::Core& core, proc_t proc, const char* want_name, bool is_32bit)
    {
        const auto flags   = is_32bit ? flags::x86 : flags::x64;
        const auto opt_mod = modules::find_name(core, proc, "ntdll.dll", flags);
        if(!opt_mod)
            return false;

        const auto opt_span = modules::span(core, proc, *opt_mod);
        if(!opt_span)
            return false;

        return symbols::load_module_at(core, proc, want_name, *opt_span);
    }

    static bool read_offsets(NtCallstacks& c, proc_t proc, bool is_32bit)
    {
        auto& opt_offsets = is_32bit ? c.offsets32_ : c.offsets64_;
        const auto name   = is_32bit ? "wntdll" : "ntdll";
        if(opt_offsets)
            return true;

        const auto ok = !is_32bit || load_ntdll(c.core_, proc, name, is_32bit);
        if(!ok)
            return FAIL(false, "unable to load ntdll");

        bool fail    = false;
        auto offsets = Offsets{};
        for(size_t i = 0; i < OFFSET_COUNT; ++i)
        {
            fail |= g_nt_offsets[i].e_id != i;
            const auto offset = symbols::struc_offset(c.core_, proc, name, g_nt_offsets[i].struc, g_nt_offsets[i].member);
            if(!offset)
            {
                LOG(ERROR, "unable to read %s!%s.%s member offset", name, g_nt_offsets[i].struc, g_nt_offsets[i].member);
                continue;
            }
            offsets[i] = *offset;
        }
        if(fail)
            return false;

        opt_offsets = offsets;
        return true;
    }

    static uint64_t offset(const NtCallstacks& c, bool is_32bit, offsets_e off)
    {
        const auto& offsets = is_32bit ? *c.offsets32_ : *c.offsets64_;
        return offsets[off];
    }

    static opt<span_t> get_user_stack(NtCallstacks& c, proc_t proc, bool is_32bit)
    {
        const auto reader = reader::make(c.core_, proc);
        if(!read_offsets(c, proc, is_32bit))
            return FAIL(ext::nullopt, "unable to read ntdll offsets");

        const auto teb    = registers::read_msr(c.core_, is_32bit ? msr_e::fs_base : msr_e::gs_base);
        const auto nt_tib = teb + offset(c, is_32bit, TEB_NtTib);
        auto base         = reader.read(nt_tib + offset(c, is_32bit, NT_TIB_StackBase));
        auto limit        = reader.read(nt_tib + offset(c, is_32bit, NT_TIB_StackLimit));
        if(!base || !limit)
            return FAIL(ext::nullopt, "unable to find stack boundaries");

        if(is_32bit)
        {
            *base  = static_cast<uint32_t>(*base);
            *limit = static_cast<uint32_t>(*limit);
        }
        return span_t{*limit, *base - *limit};
    }

    static opt<span_t> get_kernel_stack(NtCallstacks& /*c*/)
    {
        // TODO: get kernel stack boundaries
        return span_t{(size_t) 0, (size_t) -1};
    }

    static opt<span_t> get_stack(NtCallstacks& c, proc_t proc, const context_t& ctxt, bool is_32bits)
    {
        if(os::is_kernel_address(c.core_, ctxt.ip))
            return get_kernel_stack(c);

        return get_user_stack(c, proc, is_32bits);
    }

    static opt<std::tuple<std::string, span_t>> get_name_span(NtCallstacks& c, proc_t proc, const context_t& ctx)
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
    static const function_entry_t* check_previous_exist(const T& it, const T& end, const uint32_t offset_in_mod)
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

    static bool get_next_context_x64(NtCallstacks& c, proc_t proc, const reader::Reader& reader, const span_t& stack, context_t& ctx)
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
            return FAIL(false, "Can't calculate stack frame size");

        if(function_entry->frame_reg_offset != 0)
            ctx.sp = ctx.bp - function_entry->frame_reg_offset;

        if(function_entry->prev_frame_reg != 0)
            if(const auto bp = reader.read(ctx.sp + function_entry->prev_frame_reg))
                ctx.bp = *bp;

        const auto caller_addr_on_stack = ctx.sp + *stack_frame_size;

        // Check if caller's address on stack is consistent, if not we suppose that the end of the callstack has been reached
        if(stack.addr > caller_addr_on_stack || stack.addr + stack.size < caller_addr_on_stack)
            return false;

        const auto return_addr = reader.read(caller_addr_on_stack);
        if(!return_addr)
            return FAIL(false, "unable to read return address at 0x%" PRIx64, caller_addr_on_stack);

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

    static bool get_next_context_x86(NtCallstacks& /*c*/, proc_t /*proc*/, const reader::Reader& reader, const span_t& stack, context_t& ctx)
    {
        constexpr auto reg_size = 4;
        if(stack.addr > ctx.bp || stack.addr + stack.size < ctx.bp)
            return FAIL(false, "ebp out of stack bounds, ebp: 0x%" PRIx64 " stack bounds: 0x%" PRIx64 "-0x%" PRIx64, ctx.bp, stack.addr, stack.addr + stack.size);

        const auto caller_addr_on_stack = reader.le32(ctx.bp);
        if(!caller_addr_on_stack)
            return FAIL(false, "unable to read caller address on stack at 0x%" PRIx64, ctx.bp);

        const auto return_addr = reader.le32(ctx.bp + reg_size);
        if(!return_addr)
            return FAIL(false, "unable to read return address at 0x%" PRIx64, ctx.bp + reg_size);

        ctx.ip = *return_addr;
        ctx.bp = *caller_addr_on_stack;
        return true;
    }

    static size_t read_callers(NtCallstacks& c, caller_t* callers, size_t num_callers, proc_t proc, const context_t& first)
    {
        const auto reader = reader::make(c.core_, proc);
        const auto stack  = get_stack(c, proc, first, true);
        if(!stack)
            return 0;

        auto ctx                = first;
        callers[0].addr         = ctx.ip;
        const auto next_context = first.flags.is_x86 ? &get_next_context_x86 : &get_next_context_x64;
        for(size_t i = 1; i < num_callers; ++i)
        {
            const auto ok = next_context(c, proc, reader, *stack, ctx);
            if(!ok)
                return i;

            callers[i].addr = ctx.ip;
        }
        return num_callers;
    }
}

size_t NtCallstacks::read_from(caller_t* callers, size_t num_callers, proc_t proc, const context_t& first)
{
    memset(callers, 0, num_callers * sizeof *callers);
    return read_callers(*this, callers, num_callers, proc, first);
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
