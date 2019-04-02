#define FDP_MODULE "nt_writefile"
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/nt/objects_nt.hpp>
#include <icebox/os.hpp>
#include <icebox/reader.hpp>
#include <icebox/tracer/syscalls.gen.hpp>
#include <icebox/utils/path.hpp>
#include <icebox/utils/pe.hpp>
#include <icebox/waiter.hpp>

namespace
{
    static std::vector<uint8_t> load_module_buffer(core::Core& core, proc_t proc, span_t span)
    {
        std::vector<uint8_t> invalid(1);
        const auto reader = reader::make(core, proc);
        const auto debug  = pe::find_debug_codeview(reader, span);
        if(!debug)
            return invalid;

        std::vector<uint8_t> buffer;
        buffer.resize(debug->size);
        auto ok = reader.read(&buffer[0], debug->addr, debug->size);
        if(!ok)
            return invalid;

        return buffer;
    }

    static opt<std::string> load_module(core::Core& core, proc_t proc, mod_t mod)
    {
        const auto name = core.os->mod_name(proc, mod);
        if(!name)
            return {};

        LOG(INFO, "loading module {}", *name);
        const auto span = core.os->mod_span(proc, mod);
        if(!span)
            return {};

        LOG(INFO, "{} loaded at {:x}:{:x}", *name, span->addr, span->addr + span->size);
        const auto buffer   = load_module_buffer(core, proc, *span);
        const auto filename = path::filename(*name).replace_extension();
        const auto inserted = core.sym.insert(filename.generic_string(), *span, &buffer[0], buffer.size());
        if(!inserted)
            return {};

        return *name;
    }

    static int listen_writefile(core::Core& core, std::string_view target)
    {
        LOG(INFO, "waiting for {}...", target);
        const auto try_proc = waiter::proc_wait(core, target, FLAGS_NONE);
        if(!try_proc)
            return FAIL(-1, "unable to wait for {}", target);

        LOG(INFO, "process {} active", target);
        const auto proc  = *try_proc;
        const auto ntdll = waiter::mod_wait(core, proc, "ntdll.dll", FLAGS_NONE);
        if(!ntdll)
            return FAIL(-1, "unable to load ntdll.dll");

        LOG(INFO, "loading ntdll module...");
        const auto loaded = load_module(core, proc, *ntdll);
        if(!loaded)
            return FAIL(-1, "unable to load ntdll.dll symbols");

        LOG(INFO, "ntdll module loaded");
        nt::ObjectNt objects{core};
        const auto ok = objects.setup();
        if(!ok)
            return FAIL(-1, "unable to load objects helper");

        int idx = -1;
        nt::syscalls tracer{core, "ntdll"};
        std::vector<uint8_t> buf;
        const auto reader = reader::make(core, proc);
        const auto bp     = tracer.register_NtWriteFile(proc, [&](nt::HANDLE FileHandle, nt::HANDLE /*Event*/, nt::PIO_APC_ROUTINE /*ApcRoutine*/,
                                                              nt::PVOID /*ApcContext*/, nt::PIO_STATUS_BLOCK /*IoStatusBlock*/, nt::PVOID Buffer,
                                                              nt::ULONG Length, nt::PLARGE_INTEGER /*ByteOffset*/, nt::PULONG /*Key*/)
        {
            ++idx;
            if(Length > 64 * 1024 * 1024)
            {
                LOG(ERROR, "buffer too big size:{}", Length);
                return;
            }

            const auto obj = objects.get_object_ref(proc, FileHandle);
            if(!obj)
            {
                LOG(ERROR, "unable to read object {:#x}", FileHandle);
                return;
            }

            const auto filename = objects.fileobj_filename(proc, *obj);
            if(!filename)
            {
                LOG(ERROR, "unable to read filename from object {:#x}", obj->id);
                return;
            }

            LOG(INFO, "{}: {} byte(s)", *filename, Length);
            buf.resize(Length);
            const auto ok = reader.read(&buf[0], Buffer, Length);
            if(!ok)
            {
                LOG(ERROR, "unable to read range:{:#x}-{:#x}", Buffer, Buffer + Length);
                return;
            }

            const auto ext = fs::path(*filename).extension().generic_string();
            const auto dst = path::filename(*filename).replace_extension(std::to_string(idx) + ext);
            const auto fh  = fopen(dst.generic_string().data(), "wb");
            LOG(INFO, "dumping {}: {} byte(s)", dst.generic_string(), Length);
            if(!fh)
                return;

            const auto n = fwrite(&buf[0], 1, buf.size(), fh);
            fclose(fh);
            if(n != buf.size())
                LOG(ERROR, "unable to write {}/{} byte(s)", n, buf.size());
        });

        LOG(INFO, "listening NtWriteFile events...");
        const auto now = std::chrono::high_resolution_clock::now();
        const auto end = now + std::chrono::minutes(5);
        while(std::chrono::high_resolution_clock::now() < end)
        {
            core.state.resume();
            core.state.wait();
        }
        tracer.unregister(*bp);

        return 0;
    }
}

int main(int argc, char** argv)
{
    logg::init(argc, argv);
    if(argc != 3)
        return FAIL(-1, "usage: nt_writefile <name> <process_name>");

    const auto name   = std::string{argv[1]};
    const auto target = std::string_view{argv[2]};
    LOG(INFO, "starting on {}", name.data());

    core::Core core;
    const auto ok = core.setup(name);
    if(!ok)
        return FAIL(-1, "unable to start core at {}", name.data());

    core.state.pause();
    const auto ret = listen_writefile(core, target);
    core.state.resume();
    return ret;
}
