#include "callstack.hpp"

#define FDP_MODULE "callstack_nt"
#include "core.hpp"
#include "endian.hpp"
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

    using UnwindInfo = uint8_t[4];

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

    struct CallstackNt
        : public callstack::ICallstack
    {
        CallstackNt(core::Core& core);

        // callstack::ICallstack
        bool    get_callstack               (proc_t proc, callstack::on_callstep_fn on_callstep) override;
        bool    get_callstack_from_context  (proc_t proc, const callstack::context_t& first, flags_e flag, callstack::on_callstep_fn on_callstep) override;

        // methods
        opt<FunctionTable>      get_mod_functiontable   (proc_t proc, const std::string& name, const span_t module);
        opt<FunctionTable>      insert                  (proc_t proc, const std::string& name, const span_t module);
        opt<mod_t>              find_mod                (proc_t proc, uint64_t addr);
        opt<driver_t>           find_drv                (uint64_t addr);
        opt<FunctionTable>      parse_exception_dir     (proc_t proc, const void* src, uint64_t mod_base_addr, span_t exception_dir);
        const function_entry_t* lookup_function_entry   (uint32_t offset_in_mod, const FunctionEntries& function_entries);

        // members
        core::Core&   core_;
        Drivers       all_drivers_;
        AllModules    all_modules_;
        ExceptionDirs exception_dirs_;
        opt<Offsets>  offsets64_;
        opt<Offsets>  offsets32_;
    };
}

CallstackNt::CallstackNt(core::Core& core)
    : core_(core)
{
}

std::shared_ptr<callstack::ICallstack> callstack::make_callstack_nt(core::Core& core)
{
    return std::make_shared<CallstackNt>(core);
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

        // stricly greater, go to previous item
        if(it == mods.begin())
            return {};

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
}

opt<mod_t> CallstackNt::find_mod(proc_t proc, uint64_t addr)
{
    auto& modules = get_modules(all_modules_, proc);
    auto mod      = find_prev(addr, modules);
    if(mod)
    {
        const auto span = modules::span(core_, proc, *mod);
        if(addr <= span->addr + span->size)
            return mod;
    }

    mod = modules::find(core_, proc, addr);
    if(!mod)
        return {};

    const auto span = modules::span(core_, proc, *mod);
    modules.emplace(span->addr, *mod);
    return mod;
}

opt<driver_t> CallstackNt::find_drv(uint64_t addr)
{
    auto drv = find_prev(addr, all_drivers_);
    if(drv)
    {
        const auto span = drivers::span(core_, *drv);
        if(addr <= span->addr + span->size)
            return drv;
    }

    drv = drivers::find(core_, addr);
    if(!drv)
        return {};

    const auto span = drivers::span(core_, *drv);
    all_drivers_.emplace(span->addr, *drv);
    return drv;
}

opt<FunctionTable> CallstackNt::insert(proc_t proc, const std::string& name, const span_t span)
{
    const auto reader        = reader::make(core_, proc);
    const auto exception_dir = pe::find_image_directory(reader, span, pe::IMAGE_DIRECTORY_ENTRY_EXCEPTION);
    if(!exception_dir)
        return FAIL(ext::nullopt, "unable to get span of exception_dir");

    std::vector<uint8_t> buffer(exception_dir->size);
    auto ok = reader.read(&buffer[0], exception_dir->addr, exception_dir->size);
    if(!ok)
        return FAIL(ext::nullopt, "unable to read exception dir of %s", name.data());

    const auto function_table = parse_exception_dir(proc, &buffer[0], span.addr, *exception_dir);
    const auto ret            = exception_dirs_.emplace(name, *function_table);
    if(!ret.second)
        return {};

    return function_table;
}

opt<FunctionTable> CallstackNt::get_mod_functiontable(proc_t proc, const std::string& name, const span_t span)
{
    const auto it = exception_dirs_.find(name);
    if(it != exception_dirs_.end())
        return it->second;

    return insert(proc, name, span);
}

namespace
{
    static std::unique_ptr<sym::Symbols> load_ntdll(core::Core& core, proc_t proc, bool is_32bit)
    {
        std::unique_ptr<sym::Symbols> sym;
        const auto reader = reader::make(core, proc);
        modules::list(core, proc, [&](mod_t mod)
        {
            const auto name = modules::name(core, proc, mod);
            if(!name)
                return WALK_NEXT;

            const auto filename = path::filename(*name);
            if(filename != "ntdll.dll")
                return WALK_NEXT;

            const auto is_wow64 = !!(mod.flags & FLAGS_32BIT);
            if(is_32bit ^ is_wow64)
                return WALK_NEXT;

            const auto span = modules::span(core, proc, mod);
            if(!span)
                return WALK_NEXT;

            const auto debug = pe::find_debug_codeview(reader, *span);
            if(!debug)
                return WALK_NEXT;

            std::vector<uint8_t> buffer(debug->size);
            const auto ok = reader.read(&buffer[0], debug->addr, debug->size);
            if(!ok)
                return FAIL(WALK_NEXT, "unable to read IMAGE_CODEVIEW (RSDS)");

            auto tmp            = std::make_unique<sym::Symbols>();
            const auto inserted = tmp->insert("ntdll", *span, &buffer[0], buffer.size());
            if(!inserted)
                return WALK_NEXT;

            sym = std::move(tmp);
            return WALK_STOP;
        });
        return sym;
    }

    static bool read_offsets(CallstackNt& c, proc_t proc, bool is_32bit)
    {
        auto& opt_offsets = is_32bit ? c.offsets32_ : c.offsets64_;
        if(opt_offsets)
            return true;

        const auto sym = load_ntdll(c.core_, proc, is_32bit);
        if(!sym)
            return FAIL(false, "unable to load ntdll");

        bool fail = false;
        Offsets offsets;
        memset(&offsets[0], 0, sizeof offsets);
        for(size_t i = 0; i < OFFSET_COUNT; ++i)
        {
            fail |= g_nt_offsets[i].e_id != i;
            const auto offset = sym->struc_offset("ntdll", g_nt_offsets[i].struc, g_nt_offsets[i].member);
            if(!offset)
            {
                LOG(ERROR, "unable to read ntdll!%s.%s member offset", g_nt_offsets[i].struc, g_nt_offsets[i].member);
                continue;
            }
            offsets[i] = *offset;
        }
        if(fail)
            return false;

        opt_offsets = offsets;
        return true;
    }

    static uint64_t offset(const CallstackNt& c, bool is_32bit, offsets_e off)
    {
        const auto& offsets = is_32bit ? *c.offsets32_ : *c.offsets64_;
        return offsets[off];
    }

    static opt<span_t> get_user_stack(CallstackNt& c, proc_t proc, bool is_32bit)
    {
        const auto reader = reader::make(c.core_, proc);
        if(!read_offsets(c, proc, is_32bit))
            return FAIL(ext::nullopt, "unable to read ntdll offsets");

        const auto teb    = registers::read_msr(c.core_, is_32bit ? MSR_FS_BASE : MSR_GS_BASE);
        const auto nt_tib = teb + offset(c, is_32bit, TEB_NtTib);
        const auto base   = reader.read(nt_tib + offset(c, is_32bit, NT_TIB_StackBase));
        const auto limit  = reader.read(nt_tib + offset(c, is_32bit, NT_TIB_StackLimit));
        if(!base || !limit)
            return FAIL(ext::nullopt, "unable to find stack boundaries");

        return span_t{*limit, *base - *limit};
    }

    static opt<span_t> get_kernel_stack(CallstackNt& /*c*/)
    {
        // TODO: get kernel stack boundaries
        return span_t{(size_t) 0, (size_t) -1};
    }

    static opt<span_t> get_stack(CallstackNt& c, proc_t proc, const callstack::context_t& ctxt, bool is_32bits)
    {
        if(os::is_kernel_address(c.core_, ctxt.ip))
            return get_kernel_stack(c);

        return get_user_stack(c, proc, is_32bits);
    }

    static opt<std::tuple<std::string, span_t>> get_name_span(CallstackNt& c, proc_t proc, const callstack::context_t& ctx)
    {
        if(os::is_kernel_address(c.core_, ctx.ip))
        {
            const auto drv = c.find_drv(ctx.ip);
            if(!drv)
                return {};

            auto name = drivers::name(c.core_, *drv);
            auto span = drivers::span(c.core_, *drv);
            if(!name || !span)
                return {};

            return std::make_tuple(*name, *span);
        }

        const auto mod = c.find_mod(proc, ctx.ip);
        if(!mod)
            return {};

        auto name = modules::name(c.core_, proc, *mod);
        auto span = modules::span(c.core_, proc, *mod);
        if(!name || !span)
            return {};

        return std::make_tuple(*name, *span);
    }

    static bool get_callstack64(CallstackNt& c, proc_t proc, const callstack::context_t& first, const callstack::on_callstep_fn& on_callstep)
    {
        std::vector<uint8_t> buffer;
        constexpr auto reg_size = 8;
        const auto max_cs_depth = size_t(150);
        const auto reader       = reader::make(c.core_, proc);
        const auto stack        = get_stack(c, proc, first, false);
        if(!stack)
            return false;

        auto ctx = first;
        for(size_t i = 0; i < max_cs_depth; ++i)
        {
            // Get module from address
            const auto tuple = get_name_span(c, proc, ctx);
            if(!tuple)
                return false;

            auto [name, span] = *tuple;
            // Get function table of the module
            const auto function_table = c.get_mod_functiontable(proc, name, span);
            if(!function_table)
            {
                if(on_callstep(callstack::callstep_t{ctx.ip}) == WALK_STOP)
                    return true;

                return false;
            }
            const auto off_in_mod     = static_cast<uint32_t>(ctx.ip - span.addr);
            const auto function_entry = c.lookup_function_entry(off_in_mod, function_table->function_entries);
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
            if(stack->addr > caller_addr_on_stack || stack->addr + stack->size < caller_addr_on_stack)
                return true;

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

            if(on_callstep(callstack::callstep_t{ctx.ip}) == WALK_STOP)
                return true;

            ctx.ip = *return_addr;
            ctx.sp = caller_addr_on_stack + reg_size;
        }

        return true;
    }

    static bool get_callstack32(CallstackNt& c, proc_t proc, const callstack::context_t& first, const callstack::on_callstep_fn& on_callstep)
    {
        std::vector<uint8_t> buffer;
        constexpr auto reg_size = 4;
        const auto max_cs_depth = size_t(150);
        const auto reader       = reader::make(c.core_, proc);
        const auto stack        = get_stack(c, proc, first, true);
        if(!stack)
            return false;

        auto ctx = first;
        for(size_t i = 0; i < max_cs_depth; ++i)
        {
            if(stack->addr > ctx.bp || stack->addr + stack->size < ctx.bp)
                return true;

            const auto caller_addr_on_stack = reader.le32(ctx.bp);
            if(!caller_addr_on_stack)
                return FAIL(false, "unable to read caller address on stack at 0x%" PRIx64, ctx.bp);

            const auto return_addr = reader.le32(ctx.bp + reg_size);
            if(!return_addr)
                return FAIL(false, "unable to read return address at 0x%" PRIx64, ctx.bp + reg_size);

            if(on_callstep(callstack::callstep_t{ctx.ip}) == WALK_STOP)
                return true;

            ctx.ip = *return_addr;
            ctx.bp = *caller_addr_on_stack;
        }
        return true;
    }
}

bool CallstackNt::get_callstack(proc_t proc, callstack::on_callstep_fn on_callstep)
{
    const auto ip         = registers::read(core_, reg_e::rip);
    const auto sp         = registers::read(core_, reg_e::rsp);
    const auto bp         = registers::read(core_, reg_e::rbp);
    const auto cs         = registers::read(core_, reg_e::cs);
    const auto ctx        = callstack::context_t{ip, sp, bp, cs};
    constexpr auto x86_cs = 0x23;
    if(cs == x86_cs)
        return ::get_callstack32(*this, proc, ctx, on_callstep);

    return get_callstack64(*this, proc, ctx, on_callstep);
}

bool CallstackNt::get_callstack_from_context(proc_t proc, const callstack::context_t& first, flags_e flag, callstack::on_callstep_fn on_callstep)
{
    if(flag & FLAGS_32BIT)
        return get_callstack32(*this, proc, first, on_callstep);

    return get_callstack64(*this, proc, first, on_callstep);
}

namespace
{
    static void get_unwind_codes(Unwinds& unwind_codes, function_entry_t& function_entry, const std::vector<uint8_t>& buffer, size_t unwind_codes_size, size_t chained_info_size)
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
}

opt<FunctionTable> CallstackNt::parse_exception_dir(proc_t proc, const void* vsrc, uint64_t mod_base_addr, span_t exception_dir)
{
    const auto src = reinterpret_cast<const uint8_t*>(vsrc);

    FunctionTable   function_table;
    FunctionEntries orphan_function_entries;

    const auto reader = reader::make(core_, proc);
    for(size_t i = 0; i < exception_dir.size; i = i + sizeof(RuntimeFunction))
    {

        function_entry_t function_entry;
        memset(&function_entry, 0, sizeof function_entry);
        function_entry.start_address = read_le32(&src[i + offsetof(RuntimeFunction, start_address)]);
        function_entry.end_address   = read_le32(&src[i + offsetof(RuntimeFunction, end_address)]);
        const auto unwind_info_ptr   = read_le32(&src[i + offsetof(RuntimeFunction, unwind_info)]);

        UnwindInfo unwind_info;
        const auto to_read = mod_base_addr + unwind_info_ptr;
        auto ok            = reader.read(unwind_info, to_read, sizeof unwind_info);
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

        const auto SIZE_UC           = 2;
        const auto chained_info_size = chained_flag ? sizeof(RuntimeFunction) : 0;
        const auto unwind_codes_size = function_entry.unwind_codes_nb * SIZE_UC + chained_info_size;
        if(!unwind_codes_size)
        {
            function_entry.stack_frame_size = 0;
            function_table.function_entries.push_back(function_entry);
            continue;
        }

        std::vector<uint8_t> buffer(unwind_codes_size);
        reader.read(&buffer[0], mod_base_addr + unwind_info_ptr + sizeof unwind_info, unwind_codes_size);
        if(!ok)
            return FAIL(ext::nullopt, "unable to read unwind codes");

        function_entry.unwind_codes_idx = function_table.unwinds.size();
        get_unwind_codes(function_table.unwinds, function_entry, buffer, unwind_codes_size, chained_info_size);

        // Deal with the runtime func at the end
        uint32_t mother_start_addr = 0;
        if(chained_flag)
        {
            mother_start_addr = read_le32(&buffer[unwind_codes_size - chained_info_size + offsetof(RuntimeFunction, start_address)]);

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

    std::sort(function_table.function_entries.begin(), function_table.function_entries.end(), compare_function_entries);
    return function_table;
}

namespace
{
    template <typename T>
    static const function_entry_t* check_previous_exist(const T& it, const T& end, const uint32_t offset_in_mod)
    {
        if(it == end)
            return nullptr;

        if(offset_in_mod > it->end_address)
            return nullptr;

        return &*it;
    }
}

const function_entry_t* CallstackNt::lookup_function_entry(uint32_t offset_in_mod, const FunctionEntries& function_entries)
{
    // lower bound returns first item greater or equal
    function_entry_t entry;
    memset(&entry, 0, sizeof entry);
    entry.start_address = offset_in_mod;
    auto it             = lower_bound(function_entries.begin(), function_entries.end(), entry, compare_function_entries);
    const auto end      = function_entries.end();
    if(it == end)
        return check_previous_exist(function_entries.rbegin(), function_entries.rend(), offset_in_mod);

    // equal
    if(it->start_address == offset_in_mod)
        return check_previous_exist(it, end, offset_in_mod);

    // stricly greater, go to previous item
    return check_previous_exist(--it, end, offset_in_mod);
}
