#include "file.hpp"

status_t file::write(const fs::path& output, const void* data, size_t size)
{
    const auto fd = fopen(output.generic_string().data(), "wb");
    if(!fd)
        return err::make(err_e::cannot_open);

    const auto n = fwrite(data, size, 1, fd);
    fclose(fd);
    if(n != 1)
        return err::make(err_e::cannot_write);

    return err::ok;
}
