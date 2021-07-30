#include "nt_os.hpp"

#define FDP_MODULE "nt::vma"
#include "log.hpp"
#include "nt.hpp"

opt<uint64_t> nt::read_vad_root_addr(nt::Os& os, const memory::Io& io, proc_t proc, uint64_t vad_root_offset)
{
    if(!os.NtMajorVersion_)
        LOG(ERROR, "missing nt major version");

    if(os.NtMajorVersion_ > 6)
        return io.read(proc.id + vad_root_offset);

    return proc.id + vad_root_offset;
}

namespace
{
    struct vad_t
    {
        nt::ptr_t Left;
        nt::ptr_t Right;
        uint64_t  StartingVpn;
        uint64_t  EndingVpn;
    };

#if 0
    constexpr int mm_protect_to_vma_access[] =
    {
            VMA_ACCESS_NONE,                                      // PAGE_NOACCESS
            VMA_ACCESS_READ,                                      // PAGE_READONLY
            VMA_ACCESS_EXEC,                                      // PAGE_EXECUTE
            VMA_ACCESS_EXEC | VMA_ACCESS_READ,                    // PAGE_EXECUTE_READ
            VMA_ACCESS_READ | VMA_ACCESS_WRITE,                   // PAGE_READWRITE
            VMA_ACCESS_COPY_ON_WRITE,                             // PAGE_WRITECOPY
            VMA_ACCESS_EXEC | VMA_ACCESS_READ | VMA_ACCESS_WRITE, // PAGE_EXECUTE_READWRITE
            VMA_ACCESS_EXEC | VMA_ACCESS_COPY_ON_WRITE,           // PAGE_EXECUTE_WRITECOPY
    };
#endif

    nt::_LARGE_INTEGER vad_starting(const nt::win10::_MMVAD_SHORT& vad)
    {
        auto ret       = nt::_LARGE_INTEGER{};
        ret.u.LowPart  = vad.StartingVpn;
        ret.u.HighPart = vad.StartingVpnHigh;
        return ret;
    }

    nt::_LARGE_INTEGER vad_ending(const nt::win10::_MMVAD_SHORT& vad)
    {
        auto ret       = nt::_LARGE_INTEGER{};
        ret.u.LowPart  = vad.EndingVpn;
        ret.u.HighPart = vad.EndingVpnHigh;
        return ret;
    }

    bool read_vad(nt::Os& os, vad_t& vad, const memory::Io& io, uint64_t current_vad)
    {
        if(!os.NtMajorVersion_)
            LOG(ERROR, "missing nt major version");
        if(os.NtMajorVersion_ > 6)
        {
            auto temp_vad = nt::win10::_MMVAD_SHORT{};
            auto ok       = io.read_all(&temp_vad, current_vad, sizeof temp_vad);
            if(!ok)
                return FAIL(false, "unable to read _MMVAD_SHORT_WIN10");

            vad.Left        = temp_vad.VadNode.Left;
            vad.Right       = temp_vad.VadNode.Right;
            vad.StartingVpn = vad_starting(temp_vad).QuadPart;
            vad.EndingVpn   = vad_ending(temp_vad).QuadPart;
            return true;
        }

        auto       temp_vad = nt::win7::_MMVAD_SHORT{};
        const auto ok       = io.read_all(&temp_vad, current_vad, sizeof temp_vad);
        if(!ok)
            return FAIL(false, "unable to read _MMVAD_SHORT_WIN7");

        vad.Left        = temp_vad.LeftChild;
        vad.Right       = temp_vad.RightChild;
        vad.StartingVpn = temp_vad.StartingVpn;
        vad.EndingVpn   = temp_vad.EndingVpn;
        return true;
    }

    uint64_t get_mmvad(nt::Os& os, const memory::Io& io, uint64_t vad_ptr, uint64_t addr)
    {
        auto vad = vad_t{};
        while(vad_ptr)
        {
            const auto ok = read_vad(os, vad, io, vad_ptr);
            if(!ok)
                return 0;

            if(vad.StartingVpn <= addr && addr <= vad.EndingVpn)
                return vad_ptr;

            vad_ptr = addr <= vad.StartingVpn ? vad.Left : vad.Right;
        }
        return 0;
    }

    opt<span_t> get_vad_span(nt::Os& os, const memory::Io& io, uint64_t current_vad)
    {
        auto       vad = vad_t{};
        const auto ok  = read_vad(os, vad, io, current_vad);
        if(!ok)
            return {};

        return span_t{vad.StartingVpn << 12, ((vad.EndingVpn - vad.StartingVpn) + 1) << 12};
    }

    bool rec_walk_vad_tree(nt::Os& os, const memory::Io& io, proc_t proc, uint64_t current_vad, uint32_t level, const vm_area::on_vm_area_fn& on_vm_area)
    {
        auto       vad = vad_t{};
        const auto ok  = read_vad(os, vad, io, current_vad);
        if(!ok)
            return false;

        if(vad.Left)
            if(!rec_walk_vad_tree(os, io, proc, vad.Left, level + 1, on_vm_area))
                return false;

        const auto walk = on_vm_area(vm_area_t{current_vad});
        if(walk == walk_e::stop)
            return false;

        if(vad.Right)
            if(!rec_walk_vad_tree(os, io, proc, vad.Right, level + 1, on_vm_area))
                return false;

        return true;
    }
}

bool nt::Os::vm_area_list(proc_t proc, vm_area::on_vm_area_fn on_vm_area)
{
    const auto io       = memory::make_io(core_, proc);
    const auto vad_root = read_vad_root_addr(*this, io, proc, offsets_[EPROCESS_VadRoot]);
    if(!vad_root)
        return false;

    return rec_walk_vad_tree(*this, io, proc, *vad_root, 0, on_vm_area);
}

opt<vm_area_t> nt::Os::vm_area_find(proc_t proc, uint64_t addr)
{
    const auto io       = memory::make_io(core_, proc);
    const auto vad_root = read_vad_root_addr(*this, io, proc, offsets_[EPROCESS_VadRoot]);
    if(!vad_root)
        return {};

    const auto vad = get_mmvad(*this, io, *vad_root, addr >> 12);
    if(!vad)
        return {};

    return vm_area_t{vad};
}

opt<span_t> nt::Os::vm_area_span(proc_t proc, vm_area_t vm_area)
{
    const auto io = memory::make_io(core_, proc);
    return get_vad_span(*this, io, vm_area.id);
}

vma_access_e nt::Os::vm_area_access(proc_t /*proc*/, vm_area_t /*vm_area*/)
{
    return VMA_ACCESS_NONE; // todo with struct bitfields
}

vma_type_e nt::Os::vm_area_type(proc_t /*proc*/, vm_area_t /*vm_area*/)
{
    return vma_type_e::none;
}

opt<std::string> nt::Os::vm_area_name(proc_t proc, vm_area_t vm_area)
{
    // we need pdb bitfields to check if we can read subsection
    // _MMVAD is too different between win7 & win10
    if(true)
        return "";

    const auto io              = memory::make_io(core_, proc);
    auto       vad_subsection  = offsets_[MMVAD_SubSection];
    const auto subsection_addr = io.read(vm_area.id + vad_subsection);
    if(!subsection_addr || !*subsection_addr)
        return "";

    const auto control_area_addr = io.read(*subsection_addr + offsets_[SUBSECTION_ControlArea]);
    if(!control_area_addr || !*control_area_addr)
        return "";

    auto file_pointer_addr = io.read(*control_area_addr + offsets_[CONTROL_AREA_FilePointer]);
    if(!file_pointer_addr || !*file_pointer_addr) // if no file pointer cannot get the image name
        return "";

    // the file_pointer_addr is _EX_FAST_REF
    *file_pointer_addr &= ~0xF;
    const auto name = nt::read_unicode_string(io, *file_pointer_addr + offsets_[FILE_OBJECT_FileName]);
    return name ? name : "";
}
