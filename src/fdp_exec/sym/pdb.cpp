#include "sym.hpp"

#define FDP_MODULE "pdb"
#include "log.hpp"
#include "utils.hpp"

#define NOMINMAX
#include <windows.h>
#include <dbghelp.h>

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

namespace
{
    static const uint64_t BASE_ADDRESS = 0x80000000;

    struct Pdb
        : public sym::IModule
    {
         Pdb(const fs::path& filename);
        ~Pdb();

        // methods
        bool setup();

        // IModule methods
        std::optional<uint64_t> get_offset(const char* symbol) override;
        std::optional<uint64_t> get_struc_member_offset(const char* struc, const char* member) override;

        // members
        const fs::path  filename_;
        HANDLE          hproc_;
    };
}

Pdb::Pdb(const fs::path& filename)
    : filename_(filename)
    , hproc_(INVALID_HANDLE_VALUE)
{
}

Pdb::~Pdb()
{
    SymCleanup(hproc_);
}

std::unique_ptr<sym::IModule> sym::make_pdb(const char* module, const char* guid)
{
    auto ptr = std::make_unique<Pdb>(fs::path(getenv("_NT_SYMBOL_PATH")) / module / guid / module);
    const auto ok = ptr->setup();
    if(!ok)
        return std::nullptr_t();

    return ptr;
}

bool Pdb::setup()
{
    const auto hproc = GetCurrentProcess();
    const auto ok = SymInitialize(hproc, nullptr, false);
    if(!ok)
        FAIL(false, "unable to initialize library");

    hproc_ = hproc;
    std::error_code ec;
    const auto size = fs::file_size(filename_, ec); // FIXME need ?
    if(ec)
        FAIL(false, "unable to read size from %s: %s", filename_.generic_string().data(), ec.message().data());

    const auto addr = SymLoadModuleEx(hproc_, nullptr, filename_.generic_string().data(), nullptr, BASE_ADDRESS, static_cast<DWORD>(size), nullptr, 0);
    if(!addr)
        FAIL(false, "unable to load module %s", filename_.generic_string().data());

    return true;
}

namespace
{
    SYMBOL_INFO_PACKAGE make_symbol_info_package()
    {
        SYMBOL_INFO_PACKAGE info;
        memset(&info, 0, sizeof info);
        info.si.SizeOfStruct = sizeof info.si;
        info.si.MaxNameLen = sizeof info.name - 1;
        return info;
    }
}

std::optional<uint64_t> Pdb::get_offset(const char* symbol)
{
    auto info = make_symbol_info_package();
    const auto ok = SymFromName(hproc_, symbol, &info.si);
    if(!ok)
        return std::nullopt;

    return info.si.Address - BASE_ADDRESS;
}

namespace
{
    struct TI_FINDCHILDREN_PARAMS_PACKAGE
    {
        TI_FINDCHILDREN_PARAMS fp;
        ULONG                  child[256];
    };
}

std::optional<uint64_t> Pdb::get_struc_member_offset(const char* struc, const char* member)
{
    auto info = make_symbol_info_package();
    auto ok = SymGetTypeFromName(hproc_, BASE_ADDRESS, struc, &info.si);
    if(!ok)
        return std::nullopt;

    DWORD count = 0;
    ok = SymGetTypeInfo(hproc_, BASE_ADDRESS, info.si.TypeIndex, TI_GET_CHILDRENCOUNT, &count);
    if(!ok)
        return std::nullopt;

    TI_FINDCHILDREN_PARAMS_PACKAGE params;
    memset(&params, 0, sizeof params);
    params.fp.Count = count;
    ok = SymGetTypeInfo(hproc_, BASE_ADDRESS, info.si.TypeIndex, TI_FINDCHILDREN, &params.fp);
    if(!ok)
        return std::nullopt;

    const auto wmember = std::wstring(member, &member[strlen(member)]); // ASCII conversion
    for(auto i = params.fp.Start; i < params.fp.Count; ++i)
    {
        wchar_t* name = nullptr;
        ok = SymGetTypeInfo(hproc_, BASE_ADDRESS, params.fp.ChildId[i], TI_GET_SYMNAME, &name);
        if(!ok)
            continue;

        const auto is_same = wmember == name;
        LocalFree(name);
        if(!is_same)
            continue;

        DWORD offset = 0;
        ok = SymGetTypeInfo(hproc_, BASE_ADDRESS, params.fp.ChildId[i], TI_GET_OFFSET, &offset);
        if(!ok)
            return std::nullopt;

        return offset;
    }
    return std::nullopt;
}
