#include "ojk_sg_archive.h"
#include "ojk_sg_archive_exception.h"
#include "qcommon/qcommon.h"
#include "server/server.h"


namespace ojk {
namespace sg {


Archive::Archive() :
        archive_mode_(),
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
    const std::string& base_file_name)
{
    validate_archive_mode(
        archive_mode);

    auto&& file_path = generate_path(
        base_file_name);

    auto is_succeed = true;

    if (is_succeed) {
        ::FS_FOpenFileRead(
            file_path.c_str(),
            &file_handle_,
            qtrue);

        if (file_handle_ == 0) {
            is_succeed = false;

            auto error_message = get_failed_to_open_message(
                archive_mode,
                file_path,
                true);

            ::Com_DPrintf(
                "%s\n",
                error_message.c_str());
        }
    }


    int sg_version = -1;

    if (is_succeed) {
        read_chunk<int32_t>(
            INT_ID('_', 'V', 'E', 'R'),
            sg_version);

        if (sg_version != iSAVEGAME_VERSION) {
            is_succeed = false;

            ::Com_Printf(
                S_COLOR_RED "File \"%s\" has version # %d (expecting %d)\n",
                base_file_name.c_str(),
                sg_version,
                iSAVEGAME_VERSION);
        }
    }

    if (is_succeed) {
        archive_mode_ = archive_mode;
    } else {
        close();
    }

    return is_succeed;
}

bool Archive::create(
    ArchiveMode archive_mode,
    const std::string& base_file_name)
{
    throw ArchiveException(
        "Not implemented.");
}

void Archive::close()
{
    archive_mode_ = ArchiveMode::none;

    if (file_handle_ != 0) {
        ::FS_FCloseFile(file_handle_);
        file_handle_ = 0;
    }

    io_buffer_.clear();
    io_buffer_offset_ = 0;
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

std::string Archive::generate_path(
    const std::string& base_file_name)
{
    auto normalized_file_name = base_file_name;

    std::replace(
        normalized_file_name.begin(),
        normalized_file_name.end(),
        '/',
        '_');

    auto path = "saves/" + normalized_file_name + ".sav";

    return path;
}

std::string Archive::get_failed_to_open_message(
    ArchiveMode archive_mode,
    const std::string& file_name,
    bool is_open)
{
    constexpr int max_length = 256;

    const char* message_id = nullptr;

    switch (archive_mode) {
    case ArchiveMode::jedi_outcast:
        if (is_open) {
            message_id = "MENUS3_FAILED_TO_OPEN_SAVEGAME";
        } else {
            message_id = "MENUS3_FAILED_TO_CREATE_SAVEGAME";
        }
        break;

    case ArchiveMode::jedi_academy:
        if (is_open) {
            message_id = "MENUS_FAILED_TO_OPEN_SAVEGAME";
        } else {
            message_id = "MENUS3_FAILED_TO_CREATE_SAVEGAME";
        }
        break;

    default:
        break;
    }

    std::string result(
        S_COLOR_RED);

    result += ::va(
        ::SE_GetString(message_id),
        file_name.c_str());

    if (result.length() > max_length) {
        result.resize(max_length);
    }

    return result;
}


} // sg
} // ojk
