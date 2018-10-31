#include "callstack.hpp"

#define FDP_MODULE "callstack_nt"
#include "utils/pe.hpp"
#include "core/helpers.hpp"
#include "log.hpp"
#include "os.hpp"

#include <algorithm>
#include <experimental/filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::experimental::filesystem;

namespace
{
    struct CallstackNt
        : public callstack::ICallstack
    {
        CallstackNt(core::Core& core);

        // methods
        bool setup();
        opt<pe::FunctionTable> get_mod_functiontable(const std::string& name, span_t module);
        opt<pe::FunctionTable> insert(const std::string& name, span_t module);
        opt<mod_t>             find_mod(proc_t proc, uint64_t addr);

        // os::IModule
        bool    get_callstack        (proc_t proc, uint64_t rip, uint64_t rsp, uint64_t rbp,
                                      const on_callstep_fn& on_callstep) override;

        // members
        core::Core&     core_;

        // private data
        struct Data;
        std::unique_ptr<Data> d_;
    };
}

namespace
{
    using ModsByProc    = std::unordered_map<uint64_t, std::map<uint64_t, mod_t>>;
    using ExceptionDirs = std::unordered_map<std::string, pe::FunctionTable>;
}

struct CallstackNt::Data
{
    ModsByProc    modules_by_proc_;
    ExceptionDirs exception_dirs_;
};

CallstackNt::CallstackNt(core::Core& core)
    : core_(core), d_(std::make_unique<Data>())
{
}

std::unique_ptr<callstack::ICallstack> callstack::make_callstack_nt(core::Core& core)
{
    auto cs_nt = std::make_unique<CallstackNt>(core);
    if(!cs_nt)
        return nullptr;

    const auto ok = cs_nt->setup();
    if(!ok)
        return nullptr;

    return cs_nt;
}

bool CallstackNt::setup(){
    return true;
}

namespace {
    struct CurrentContext
    {
        uint64_t rip;
        uint64_t rsp;
        uint64_t rbp;
    };

    opt<uint64_t> get_stack_frame_size(const uint64_t off_in_mod, const pe::FunctionEntry function_entry)
    {
        const auto off_in_prolog = off_in_mod - function_entry.start_address;
        if (off_in_prolog == 0)
            return 0;

        if (off_in_prolog > function_entry.prolog_size)
            return function_entry.stack_frame_size;

        for (auto it : function_entry.unwind_codes){
            if (off_in_prolog > it.code_offset){
                return function_entry.stack_frame_size - it.stack_size_used;
            }
        }

        return exp::nullopt;
    }

}

namespace{
    opt<mod_t> find_prev(const uint64_t addr, std::map<uint64_t, mod_t> mod_map)
    {
        if (mod_map.size() == 0)
            return exp::nullopt;

        // lower bound returns first item greater or equal
        auto it = mod_map.lower_bound(addr);
        const auto end = mod_map.end();
        if(it == end)
            return mod_map.rbegin()->second;

        // equal
        if(it->first == addr)
            return it->second;

        // stricly greater, go to previous item
        if(it == mod_map.begin())
            return exp::nullopt;

        return (--it)->second;
    }
}


opt<mod_t> CallstackNt::find_mod(proc_t proc, uint64_t addr)
{
    std::map<uint64_t, mod_t> mod_map;

    auto it = d_->modules_by_proc_.find(proc.id);

    if(it == d_->modules_by_proc_.end())
        it = d_->modules_by_proc_.emplace(proc.id, mod_map).first;

    mod_map = it->second;

    auto mod = find_prev(addr, mod_map);
    if (mod){
        const auto span = core_.os->mod_span(proc, *mod);
        if (addr <= (span->addr + span->size))
            return mod;
    }

    //Insert mod
    mod = core_.os->mod_find(proc, addr);
    if(!mod)
        return exp::nullopt;

    const auto span = core_.os->mod_span(proc, *mod);
    it->second.emplace(span->addr, *mod);

    return mod;
}

opt<pe::FunctionTable> CallstackNt::insert(const std::string& name, span_t span)
{
    const auto exception_dir = pe::get_directory_entry(core_, span, pe::pe_directory_entries_e::IMAGE_DIRECTORY_ENTRY_EXCEPTION);

    std::vector<uint8_t> buffer_excep;
    buffer_excep.resize(exception_dir->size);
    auto ok = core_.mem.virtual_read(&buffer_excep[0], exception_dir->addr, exception_dir->size);
    if (!ok)
        FAIL(exp::nullopt, "unable to read exception dir of %s", name.c_str());

    const auto function_table = pe::parse_exception_dir(core_, &buffer_excep[0], span.addr, *exception_dir);

    const auto ret = d_->exception_dirs_.emplace(name, *function_table);
    if(!ret.second)
        return exp::nullopt;

    return function_table;
}

opt<pe::FunctionTable> CallstackNt::get_mod_functiontable(const std::string& name, span_t span)
{
    const auto it = d_->exception_dirs_.find(name);
    if(it != d_->exception_dirs_.end())
        return it->second;

    return insert(name, span);
}

bool CallstackNt::get_callstack (proc_t proc, uint64_t rip, uint64_t rsp, uint64_t rbp, const on_callstep_fn& on_callstep)
{
    const auto proc_ctx = core_.mem.switch_process(proc);

    CurrentContext ctx = {rip, rsp, rbp};

    int i = 0;
    std::vector<uint8_t> buffer_mod;
    int max_cs_depth = 50;
    while(i<max_cs_depth){
        //Get module from address
        const auto mc = find_mod(proc, ctx.rip);
        if(!mc)
            FAIL(false, "Unable to find module");
        auto modname  = core_.os->mod_name(proc, *mc);
        const auto span     = core_.os->mod_span(proc, *mc);

        //Load PDB
        if (false){
            const auto debug_dir = pe::get_directory_entry(core_, *span, pe::pe_directory_entries_e::IMAGE_DIRECTORY_ENTRY_DEBUG);
            buffer_mod.resize(debug_dir->size);
            auto ok = core_.mem.virtual_read(&buffer_mod[0], debug_dir->addr, debug_dir->size);
            if(!ok)
                return WALK_NEXT;

            const auto codeview = pe::parse_debug_dir(&buffer_mod[0], span->addr, *debug_dir);
            buffer_mod.resize(codeview->size);
            ok = core_.mem.virtual_read(&buffer_mod[0], codeview->addr, codeview->size);
            if (!ok)
                FAIL(WALK_NEXT, "Unable to read IMAGE_CODEVIEW (RSDS)");

            std::replace( modname->begin(), modname->end(), '\\', '/');
            const auto fname = fs::path(modname->substr(3)).filename().replace_extension("");
            ok = core_.sym.insert(fname.generic_string().data(), *span, &buffer_mod[0], buffer_mod.size());
        }

        // Get function table of the module
        const auto exception_dir = pe::get_directory_entry(core_, *span, pe::pe_directory_entries_e::IMAGE_DIRECTORY_ENTRY_EXCEPTION);

        std::vector<uint8_t> buffer_excep;
        buffer_excep.resize(exception_dir->size);
        auto ok = core_.mem.virtual_read(&buffer_excep[0], exception_dir->addr, exception_dir->size);
        if (!ok)
            FAIL(false, "unable to read exception dir of %s", modname ? modname->data() : "<noname>");

        const auto function_table = get_mod_functiontable(*modname, *span);
        if (!function_table)
            FAIL(false, "unable to get function table of %s", modname->c_str());

        const auto off_in_mod = ctx.rip-span->addr;
        const auto function_entry = pe::lookup_function_entry(off_in_mod, *function_table);
        if (!function_entry)
            FAIL(false, "No matching function entry");

        const auto stack_frame_size = get_stack_frame_size(off_in_mod, *function_entry);
        if (!stack_frame_size)
            FAIL(false, "Can't calculate stack frame size");

        if (function_entry->frame_reg_offset != 0)
            ctx.rsp = ctx.rbp - function_entry->frame_reg_offset;

        if (function_entry->prev_frame_reg != 0)
            ctx.rbp = *(core::read_ptr(core_, ctx.rsp-function_entry->prev_frame_reg));

        const auto caller_addr_on_stack = ctx.rsp + *stack_frame_size;

        // print stack
        if(false){
            const auto print_d = 25;
            for (int k = 0; k < print_d*8; k += 8){
                LOG(INFO, "%" PRIx64 " - %" PRIx64, ctx.rsp+k,*core::read_ptr(core_, ctx.rsp+k));
            }
        }

        const auto return_addr = core::read_ptr(core_, caller_addr_on_stack); // +8?

        if(false){
            LOG(INFO, "Chosen chosen %" PRIx64 " start address %" PRIx32 " end %" PRIx32, off_in_mod, function_entry->start_address, function_entry->end_address);
            LOG(INFO, "Offset of current func %" PRIx64 ", Caller address on stack %" PRIx64 " so %" PRIx64, off_in_mod, caller_addr_on_stack, *return_addr);
        }

        auto mycursor = core_.sym.find(ctx.rip);
        if(!mycursor)
            mycursor = sym::Cursor{modname ? modname->data() : "<noname>", "<nosymbol>", off_in_mod};

        ctx.rip = *return_addr;
        ctx.rsp = caller_addr_on_stack + 8;

        if(on_callstep(*mycursor) == WALK_STOP)
            return true;

        i++;
    }

    return true;
}
