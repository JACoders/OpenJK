#include "ojk_sg_archive.h"
#include <cstdio>
#include <unordered_map>
#include "ojk_sg_archive_exception.h"


namespace ojk {
namespace sg {


Archive::Archive() :
        file_handle_(),
        io_buffer_(),
        io_buffer_offset_()
{
}

Archive::~Archive()
{
    close();
}

bool Archive::open(
    const std::string& file_path)
{
    throw ArchiveException(
        "Not implemented.");
}

bool Archive::create(
    const std::string& file_path)
{
    throw ArchiveException(
        "Not implemented.");
}

void Archive::close()
{
    throw ArchiveException(
        "Not implemented.");
}

void Archive::read_chunk(
    const Archive::ChunkId chunk_id)
{
    throw ArchiveException(
        "Not implemented.");
}

void Archive::write_chunk(
    const Archive::ChunkId chunk_id)
{
    throw ArchiveException(
        "Not implemented.");
}

void Archive::rename(
        const std::string& old_file_path,
        const std::string& new_file_path)
{
    throw ArchiveException(
        "Not implemented.");
}

void Archive::remove(
    const std::string& file_path)
{
    throw ArchiveException(
        "Not implemented.");
}

Archive& Archive::get_instance()
{
    static Archive result;
    return result;
}


} // sg
} // ojk
