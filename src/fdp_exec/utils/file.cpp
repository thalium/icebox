#include "file.hpp"

bool file::write(const fs::path& output, const void* data, size_t size)
{
    const auto fd = fopen(output.generic_string().data(), "wb");
    if(!fd)
        return false;

    const auto n = fwrite(data, size, 1, fd);
    fclose(fd);
    if(n != 1)
        return false;

    return true;
}
