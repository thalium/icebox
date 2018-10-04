#include "os.hpp"

#define FDP_MODULE "os_nt"
#include "log.hpp"
#include "core.hpp"
#include "endian.hpp"
#include "utils.hpp"

#include <algorithm>

namespace
{
    struct OsNt
        : public os::IHelper
    {
        OsNt(ICore& core);

        // methods
        bool setup();

        // os::IHelper
        std::string_view name() const override;
        bool             list_procs(const on_process_fn& on_process) override;
        os::proc_t       get_current_proc() override;
        os::proc_t       get_proc(const std::string& name) override;
        std::string_view get_name(os::proc_t proc) override;

        // members
        ICore& core_;
    };
}

OsNt::OsNt(ICore& core)
    : core_(core)
{
}

namespace
{
    struct span_t
    {
        uint64_t    addr;
        size_t      size;
    };

    std::optional<size_t> validate_pe_header(const uint8_t (&buf)[PAGE_SIZE])
    {
        static const auto e_lfanew_offset = 0x3C;
        int idx = e_lfanew_offset;
        if(idx + 4 > sizeof buf)
            return std::nullopt;

        const auto e_lfanew = read_le32(&buf[idx]);
        idx = e_lfanew;

        static const uint32_t image_nt_signature = 'PE' << 16;
        if(idx + sizeof image_nt_signature > sizeof buf)
            return std::nullopt;

        const auto signature = read_be32(&buf[idx]);
        if(signature != image_nt_signature)
            return std::nullopt;

        static const uint16_t image_file_machine_amd64 = 0x8664;
        idx += sizeof signature;
        if(idx + sizeof image_file_machine_amd64 > sizeof buf)
            return std::nullopt;

        const auto machine = read_le16(&buf[idx]);
        if(machine != image_file_machine_amd64)
            return std::nullopt;

        static const int image_file_header_size = 20;
        static const uint16_t image_nt_optional_hdr64_magic = 0x20B;
        idx += image_file_header_size;
        if(idx + sizeof image_nt_optional_hdr64_magic > sizeof buf)
            return std::nullopt;

        const auto magic = read_le16(&buf[idx]);
        if(magic != image_nt_optional_hdr64_magic)
            return std::nullopt;

        static const auto size_of_image_offset = 14 * 4;
        idx += size_of_image_offset;
        if(idx + 4 > sizeof buf)
            return std::nullopt;

        return read_le32(&buf[idx]);
    }

    std::optional<span_t> find_kernel(ICore& core)
    {
        const auto lstar = core.read_msr(MSR_LSTAR);
        if(!lstar)
            return std::nullopt;

        uint8_t buf[PAGE_SIZE];
        for(auto ptr = align<PAGE_SIZE>(*lstar); ptr < *lstar; ptr -= PAGE_SIZE)
        {
            auto ok = core.read_mem(buf, ptr, sizeof buf);
            if(!ok)
                return std::nullopt;

            const auto e_magic = read_be16(buf);
            static const auto image_dos_signature = 'MZ';
            if(e_magic != image_dos_signature)
                continue;

            const auto size = validate_pe_header(buf);
            if(!size)
            {
                LOG(ERROR, "invalid PE header at 0x%llx", ptr);
                continue;
            }

            return span_t{ptr, *size};
        }

        return std::nullopt;
    }

    std::vector<uint8_t> read_buffer(ICore& core, span_t span)
    {
        uint8_t page[PAGE_SIZE];

        std::vector<uint8_t> buffer;
        buffer.reserve(span.size);
        for(size_t i = 0; i < span.size; i += sizeof page)
        {
            const auto chunk = std::min<size_t>(PAGE_SIZE, std::max<int64_t>(0, span.size - i));
            const auto ok = core.read_mem(page, span.addr + i, chunk);
            if(!ok)
            {
                LOG(ERROR, "unable to read memory 0x%llx - 0x%llx", span.addr + i, span.addr + i + chunk);
                buffer.clear();
                return buffer;
            }

            const auto old = buffer.size();
            buffer.resize(old + sizeof page);
            memcpy(&buffer[old], page, sizeof page);
        }

        return buffer;
    }

    struct PdbCtx
    {
        std::string guid;
        uint32_t    age;
        std::string name;
    };

    void binhex(char* dst, const void* vsrc, size_t size)
    {
        static const char hexchars_upper[] = "0123456789ABCDEF";
        const uint8_t* src = static_cast<const uint8_t*>(vsrc);
        for (size_t i = 0; i < size; ++i)
        {
            dst[i * 2 + 0] = hexchars_upper[src[i] >> 4];
            dst[i * 2 + 1] = hexchars_upper[src[i] & 0x0F];
        }
    }

    std::optional<PdbCtx> read_pdb(ICore& core, span_t kernel)
    {
        const auto buf = read_buffer(core, kernel);

        std::vector<uint8_t> magic = { 'R', 'S', 'D', 'S' };
        const auto it = std::search(buf.begin(), buf.end(), std::boyer_moore_horspool_searcher(magic.begin(), magic.end()));
        if(it == buf.end())
            return std::nullopt;

        const auto rsds = &*it;
        const auto size = std::distance(it, buf.end());
        if(size < 4 /*magic*/ + 16 /*guid*/ + 4 /*age*/ + 2 /*name*/)
            return std::nullopt;

        const auto end = reinterpret_cast<const uint8_t*>(memchr(&rsds[4 + 16 + 4], 0x00, size));
        if(!end)
            return std::nullopt;

        uint8_t guid[16];
        write_be32(&guid[0], read_le32(&rsds[4 + 0]));  // Data1
        write_be16(&guid[4], read_le16(&rsds[4 + 4]));  // Data2
        write_be16(&guid[6], read_le16(&rsds[4 + 6 ])); // Data3
        memcpy(&guid[8], &rsds[4 + 8], 8);              // Data4

        char strguid[sizeof guid * 2];
        binhex(strguid, &guid, sizeof guid);

        uint32_t age = read_le32(&rsds[4 + 16]);
        const auto name = &rsds[4 + 16 + 4];
        return PdbCtx{{strguid, sizeof strguid}, age, {name, end}};
    }
}

bool OsNt::setup()
{
    const auto kernel = find_kernel(core_);
    if(!kernel)
        FAIL(false, "unable to find kernel");

    LOG(INFO, "kernel: 0x%016llx - 0x%016llx (%lld 0x%llx)", kernel->addr, kernel->addr + kernel->size, kernel->size, kernel->size);
    const auto pdb = read_pdb(core_, *kernel);
    if(!pdb)
        FAIL(false, "unable to read pdb in kernel module");

    LOG(INFO, "kernel: pdb: %s %s age:%d", pdb->guid.data(), pdb->name.data(), pdb->age);
    return true;
}

std::unique_ptr<os::IHelper> os::make_nt(ICore& core)
{
    auto nt = std::make_unique<OsNt>(core);
    if(!nt)
        return std::nullptr_t();

    const auto ok = nt->setup();
    if(!ok)
        return std::nullptr_t();

    return nt;
}

std::string_view OsNt::name() const
{
    return "nt";
}

bool OsNt::list_procs(const on_process_fn& /*on_process*/)
{
    return false;
}

os::proc_t OsNt::get_current_proc()
{
    return 0;
}

os::proc_t OsNt::get_proc(const std::string& /*name*/)
{
    return 0;
}

std::string_view OsNt::get_name(os::proc_t /*proc*/)
{
    return {};
}
