#define FDP_MODULE "nt_writefile"
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/nt/objects_nt.hpp>
#include <icebox/plugins/sym_loader.hpp>
#include <icebox/reader.hpp>
#include <icebox/tracer/syscalls.gen.hpp>
#include <icebox/utils/path.hpp>
#include <icebox/waiter.hpp>

namespace
{
    static int listen_writefile(core::Core& core, std::string_view target)
    {
        LOG(INFO, "waiting for {}...", target);
        const auto proc = waiter::proc_wait(core, target, FLAGS_NONE);
        if(!proc)
            return FAIL(-1, "unable to wait for {}", target);

        LOG(INFO, "process {} active", target);
        const auto ntdll = waiter::mod_wait(core, *proc, "ntdll.dll", FLAGS_NONE);
        if(!ntdll)
            return FAIL(-1, "unable to load ntdll.dll");

        LOG(INFO, "ntdll module loaded");
        int idx           = -1;
        auto loader       = sym::Loader{core, *proc};
        auto objects      = nt::ObjectNt{core, *proc};
        auto tracer       = nt::syscalls{core, loader.symbols(), "ntdll"};
        auto buffer       = std::vector<uint8_t>{};
        const auto reader = reader::make(core, *proc);
        const auto bp     = tracer.register_NtWriteFile(*proc, [&](nt::HANDLE FileHandle, nt::HANDLE /*Event*/, nt::PIO_APC_ROUTINE /*ApcRoutine*/,
                                                               nt::PVOID /*ApcContext*/, nt::PIO_STATUS_BLOCK /*IoStatusBlock*/, nt::PVOID Buffer,
                                                               nt::ULONG Length, nt::PLARGE_INTEGER /*ByteOffset*/, nt::PULONG /*Key*/)
        {
            ++idx;
            if(Length > 64 * 1024 * 1024)
            {
                LOG(ERROR, "buffer too big size:{}", Length);
                return;
            }

            const auto file = objects.file_read(FileHandle);
            if(!file)
            {
                LOG(ERROR, "unable to read object {:#x}", FileHandle);
                return;
            }

            const auto filename = objects.file_name(*file);
            if(!filename)
            {
                LOG(ERROR, "unable to read filename from object {:#x}", file->id);
                return;
            }

            LOG(INFO, "{}: {} byte(s)", *filename, Length);
            buffer.resize(Length);
            const auto ok = reader.read(&buffer[0], Buffer, Length);
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

            const auto n = fwrite(&buffer[0], 1, buffer.size(), fh);
            fclose(fh);
            if(n != buffer.size())
                LOG(ERROR, "unable to write {}/{} byte(s)", n, buffer.size());
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
