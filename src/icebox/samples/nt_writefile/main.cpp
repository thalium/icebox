#define FDP_MODULE "nt_writefile"
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/nt/objects_nt.hpp>
#include <icebox/plugins/sym_loader.hpp>
#include <icebox/reader.hpp>
#include <icebox/tracer/syscalls.gen.hpp>
#include <icebox/utils/file.hpp>
#include <icebox/utils/path.hpp>
#include <icebox/waiter.hpp>

namespace
{
    static bool read_file(std::vector<uint8_t>& dst, const reader::Reader& reader, nt::PVOID Buffer, nt::ULONG Length)
    {
        if(Length > 64 * 1024 * 1024)
            return FAIL(false, "buffer too big size:{}", Length);

        dst.resize(Length);
        const auto ok = reader.read(&dst[0], Buffer, Length);
        if(!ok)
            return FAIL(false, "unable to read range:{:#x}-{:#x}", Buffer, Buffer + Length);

        return true;
    }

    static opt<std::string> read_filename(nt::ObjectNt& objects, nt::HANDLE FileHandle)
    {
        const auto file = objects.file_read(FileHandle);
        if(!file)
            return FAIL(ext::nullopt, "unable to read object {:#x}", FileHandle);

        const auto filename = objects.file_name(*file);
        if(!filename)
            return FAIL(ext::nullopt, "unable to read filename from object {:#x}", file->id);

        return *filename;
    }

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

        auto loader = sym::Loader{core};
        loader.mod_load(*proc, *ntdll);
        LOG(INFO, "ntdll module loaded");

        int idx           = -1;
        auto objects      = nt::ObjectNt{core, *proc};
        auto tracer       = nt::syscalls{core, loader.symbols(), "ntdll"};
        auto buffer       = std::vector<uint8_t>{};
        const auto reader = reader::make(core, *proc);
        const auto bp     = tracer.register_NtWriteFile(*proc, [&](nt::HANDLE FileHandle, nt::HANDLE /*Event*/, nt::PIO_APC_ROUTINE /*ApcRoutine*/,
                                                               nt::PVOID /*ApcContext*/, nt::PIO_STATUS_BLOCK /*IoStatusBlock*/, nt::PVOID Buffer,
                                                               nt::ULONG Length, nt::PLARGE_INTEGER /*ByteOffset*/, nt::PULONG /*Key*/)
        {
            ++idx;
            const auto filename = read_filename(objects, FileHandle);
            const auto read     = read_file(buffer, reader, Buffer, Length);
            if(!filename || !read)
                return;

            LOG(INFO, "{}: {} byte(s)", *filename, Length);
            const auto ext = fs::path(*filename).extension().generic_string();
            const auto dst = path::filename(*filename).replace_extension(std::to_string(idx) + ext);
            LOG(INFO, "dumping {}: {} byte(s)...", dst.generic_string(), buffer.size());
            file::write(dst, &buffer[0], buffer.size());
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
