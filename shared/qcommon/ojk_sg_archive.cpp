#include "ojk_sg_archive.h"
#include "ojk_sg_archive_exception.h"
#include "qcommon/qcommon.h"


namespace ojk {
namespace sg {


Archive::Archive() :
        archive_mode_(),
        paths_(get_max_path_count()),
        path_index_(),
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
    ArchiveMode archive_mode,
    const std::string& file_path)
{
    throw ArchiveException(
        "Not implemented.");
}

bool Archive::create(
    ArchiveMode archive_mode,
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

int Archive::get_max_path_count()
{
    return 8;
}

void Archive::validate_archive_mode(
    ArchiveMode archive_mode)
{
    switch (archive_mode) {
    case ArchiveMode::jedi_academy:
    case ArchiveMode::jedi_outcast:
        break;

    default:
        throw ArchiveException(
            "Invalid mode.");
    }
}

const std::string& Archive::add_path(
    const std::string& path)
{
    auto next_path_index = (path_index_ + 1) % get_max_path_count();

    auto normalized_path = path;

    std::replace(
        normalized_path.begin(),
        normalized_path.end(),
        '/',
        '_');

    auto new_path = "saves/" + normalized_path + ".sav";

    paths_[path_index_] = new_path;

    path_index_ = next_path_index;

    return paths_[path_index_];
}


} // sg
} // ojk
