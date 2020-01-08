#pragma once

#include "types.hpp"

#include <functional>
#include <memory>
#include <unordered_set>

namespace core { struct Core; }

namespace state
{
    // auto-managed breakpoint object
    struct BreakpointPrivate;
    using Breakpoint = std::shared_ptr<BreakpointPrivate>;

    enum bp_cr3_e
    {
        BP_CR3_ON_WRITINGS,
        BP_CR3_NONE,
    };

    // generic functor object
    using Task = std::function<void(void)>;

    bool        pause                       (core::Core& core);
    bool        resume                      (core::Core& core);
    bool        single_step                 (core::Core& core);
    bool        wait                        (core::Core& core);
    bool        save                        (core::Core& core);
    bool        restore                     (core::Core& core);
    void        wait_for                    (core::Core& core, int timeout_ms);
    Breakpoint  break_on                    (core::Core& core, std::string_view name, uint64_t ptr, const Task& task);
    Breakpoint  break_on_process            (core::Core& core, std::string_view name, proc_t proc, uint64_t ptr, const Task& task);
    Breakpoint  break_on_thread             (core::Core& core, std::string_view name, thread_t thread, uint64_t ptr, const Task& task);
    Breakpoint  break_on_physical           (core::Core& core, std::string_view name, phy_t phy, const Task& task);
    Breakpoint  break_on_physical_process   (core::Core& core, std::string_view name, dtb_t dtb, phy_t phy, const Task& task);
    bool        run_to_cr_write             (core::Core& core, reg_e reg);
    void        run_to_current              (core::Core& core, std::string_view name);
    void        run_to                      (core::Core& core, std::string_view name, std::unordered_set<uint64_t> ptrs, bp_cr3_e bp_cr3, std::function<walk_e(proc_t, thread_t)> on_bp);
    bool        inject_interrupt            (core::Core& core, uint32_t code, uint32_t error, uint64_t cr2);
    bpid_t      save_breakpoint             (core::Core& core, const Breakpoint& bp);
    bpid_t      acquire_breakpoint_id       (core::Core& core);
    bpid_t      save_breakpoint_with        (core::Core& core, bpid_t bpid, const Breakpoint& bp);
    void        drop_breakpoint             (core::Core& core, bpid_t bpid);
} // namespace state
