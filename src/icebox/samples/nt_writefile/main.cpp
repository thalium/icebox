#define FDP_MODULE "nt_writefile"
#include <icebox/core.hpp>
#include <icebox/log.hpp>
#include <icebox/nt/nt_objects.hpp>
#include <icebox/tracer/syscalls.gen.hpp>
#include <icebox/utils/file.hpp>
#include <icebox/utils/path.hpp>

#include <string>

namespace
{
    bool read_file(std::vector<uint8_t>& dst, const memory::Io& io, nt::PVOID Buffer, nt::ULONG Length)
    {
        if(Length > 64 * 1024 * 1024)
            return FAIL(false, "buffer too big size:%d", Length);

        dst.resize(Length);
        const auto ok = io.read_all(&dst[0], Buffer, Length);
        if(!ok)
            return FAIL(false, "unable to read range:0x%" PRIx64 "-0x%" PRIx64, Buffer, Buffer + Length);

        return true;
    }

    opt<std::string> read_filename(objects::Data& d, nt::HANDLE FileHandle)
    {
        const auto file = objects::file_read(d, FileHandle);
        if(!file)
            return FAIL(std::nullopt, "unable to read object 0x%" PRIx64, FileHandle);

        const auto filename = objects::file_name(d, *file);
        if(!filename)
            return FAIL(std::nullopt, "unable to read filename from object 0x%" PRIx64, file->id);

        return *filename;
    }

    int listen_writefile(core::Core& core, const std::string& target)
    {
        LOG(INFO, "waiting for %s...", target.data());
        const auto proc = process::wait(core, target, flags::x64);
        if(!proc)
            return FAIL(-1, "unable to wait for %s", target.data());

        LOG(INFO, "process %s active", target.data());
        const auto ok = symbols::load_module(core, *proc, "ntdll");
        if(!ok)
            return FAIL(-1, "unable to load ntdll symbols");

        int idx       = -1;
        auto objects  = objects::make(core, *proc);
        auto tracer   = nt::syscalls{core, "ntdll"};
        auto buffer   = std::vector<uint8_t>{};
        const auto io = memory::make_io(core, *proc);
        const auto bp = tracer.register_NtWriteFile(*proc, [&](nt::HANDLE FileHandle, nt::HANDLE /*Event*/, nt::PIO_APC_ROUTINE /*ApcRoutine*/,
                                                               nt::PVOID /*ApcContext*/, nt::PIO_STATUS_BLOCK /*IoStatusBlock*/, nt::PVOID Buffer,
                                                               nt::ULONG Length, nt::PLARGE_INTEGER /*ByteOffset*/, nt::PULONG /*Key*/)
        {
            ++idx;
            const auto filename = read_filename(*objects, FileHandle);
            const auto read     = read_file(buffer, io, Buffer, Length);
            if(!filename || !read)
                return;

            LOG(INFO, "%s: %d byte(s)", filename->data(), Length);
            const auto ext = fs::path(*filename).extension().generic_string();
            const auto dst = path::filename(*filename).replace_extension(std::to_string(idx) + ext);
            LOG(INFO, "dumping %s: %" PRId64 " byte(s)...", dst.generic_string().data(), buffer.size());
            file::write(dst, &buffer[0], buffer.size());
        });

        LOG(INFO, "listening NtWriteFile events...");
        const auto now = std::chrono::high_resolution_clock::now();
        const auto end = now + std::chrono::minutes(5);
        while(std::chrono::high_resolution_clock::now() < end)
            state::exec(core);

        state::drop_breakpoint(core, *bp);
        return 0;
    }
}

int main(int argc, char** argv)
{
    logg::init(argc, argv);
    if(argc != 3)
        return FAIL(-1, "usage: nt_writefile <name> <process_name>");

    const auto name   = std::string{argv[1]};
    const auto target = std::string{argv[2]};
    LOG(INFO, "starting on %s", name.data());

    const auto core = core::attach(name);
    if(!core)
        return FAIL(-1, "unable to start core at %s", name.data());

    state::pause(*core);
    const auto ret = listen_writefile(*core, target);
    state::resume(*core);
    return ret;
}
