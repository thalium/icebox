#include "core.hpp"
#include "os.hpp"

#define FDP_MODULE "main"
#include "log.hpp"

#include <thread>
#include <chrono>
#include <filesystem>

namespace fs = std::experimental::filesystem;

int main(int argc, char* argv[])
{
    loguru::g_preamble_uptime = false;
    loguru::g_preamble_date = false;
    loguru::g_preamble_thread = false;
    loguru::g_preamble_file = false;
    loguru::init(argc, argv);
    if(argc != 2)
        FAIL(-1, "usage: fdp_exec <name>");

    const auto name = std::string{argv[1]};
    LOG(INFO, "starting on %s", name.data());

    core::Core core;
    const auto ok = core::setup(core, name);
    if(!ok)
        FAIL(-1, "unable to start core at %s", name.data());

    core.state.resume();
    core.state.pause();

    LOG(INFO, "drivers:");
    core.os->driver_list([&](driver_t drv)
    {
        const auto name = core.os->driver_name(drv);
        const auto span = core.os->driver_span(drv);
        LOG(INFO, "    driver: %llx %s 0x%llx 0x%llx", drv.id, name ? name->data() : "<noname>", span ? span->addr : 0, span ? span->size : 0);
        return WALK_NEXT;
    });

    const auto pc = core.os->proc_current();
    LOG(INFO, "current process: %llx dtb: %llx %s", pc->id, pc->dtb, core.os->proc_name(*pc)->data());

    const auto tc = core.os->thread_current();
    LOG(INFO, "current thread: %llx", tc->id);

    LOG(INFO, "processes:");
    core.os->proc_list([&](proc_t proc)
    {
        const auto procname = core.os->proc_name(proc);
        LOG(INFO, "proc: %llx %s", proc.id, procname ? procname->data() : "<noname>");
        return WALK_NEXT;
    });

    LOG(INFO, "searching notepad.exe");
    const auto notepad = core.os->proc_find("notepad.exe");
    LOG(INFO, "notepad.exe: %" PRIx64 " dtb: %" PRIx64 " %s", notepad->id, notepad->dtb, core.os->proc_name(*notepad)->data());

    const auto WriteFile = core.sym.symbol("nt", "NtWriteFile");
    LOG(INFO, "WriteFile = 0x%llx", WriteFile ? *WriteFile : 0);

    // load all modules
    {
        const auto bp = core.state.set_breakpoint(*WriteFile, *notepad, core::FILTER_CR3);
        core.state.resume();
        core.state.wait();
    }
    std::vector<uint8_t> buffer;
    core.os->mod_list(*notepad, [&](mod_t mod)
    {
        const auto name = core.os->mod_name(*notepad, mod);
        const auto span = core.os->mod_span(*notepad, mod);
        if(!name || !span)
            return WALK_NEXT;

        LOG(INFO, "    %s 0x%" PRIx64 " 0x%zx", name->data(), span->addr, span->size);
        buffer.resize(span->size);
        auto ok = core.mem.virtual_read(&buffer[0], span->addr, span->size);
        if(!ok)
            return WALK_NEXT;

        const auto fname = fs::path(*name).filename();
        ok = core.sym.insert(fname.generic_string().data(), *span, &buffer[0]);
        if(!ok)
            return WALK_NEXT;

        return WALK_NEXT;
    });

    core.state.resume();
    return 0;
}
