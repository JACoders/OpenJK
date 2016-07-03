#include "ojk_sg_archive.h"
#include "ojk_sg_archive_exception.h"
#include "qcommon/qcommon.h"
#include "server/server.h"


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
    const std::string& base_file_name)
{
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

    if (!is_succeed) {
        close();
    }

    return is_succeed;
}

bool Archive::create(
    const std::string& base_file_name)
{
    remove(
        base_file_name);

    auto path = generate_path(
        base_file_name);

    file_handle_ = ::FS_FOpenFileWrite(
        path.c_str());

    if (file_handle_ == 0) {
        auto error_message = get_failed_to_open_message(
            path,
            false);

        ::Com_Printf(
            "%s\n",
            error_message.c_str());

        return false;
    }

    int sg_version = iSAVEGAME_VERSION;

    write_chunk<int32_t>(
        INT_ID('_', 'V', 'E', 'R'),
        sg_version);

    return true;
}

void Archive::close()
{
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
    const std::string& old_base_file_name,
    const std::string& new_base_file_name)
{
    auto old_path = generate_path(
        old_base_file_name);

    auto new_path = generate_path(
        new_base_file_name);

    auto rename_result = ::FS_MoveUserGenFile(
        old_path.c_str(),
        new_path.c_str());

    if (rename_result != 0) {
        ::Com_Printf(
            S_COLOR_RED "Error during savegame-rename. Check \"%s\" for write-protect or disk full!\n",
            new_path.c_str());
    }
}

void Archive::remove(
    const std::string& base_file_name)
{
    auto path = generate_path(
        base_file_name);

    ::FS_DeleteUserGenFile(
        path.c_str());
}

Archive& Archive::get_instance()
{
    static Archive result;
    return result;
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
    const std::string& file_name,
    bool is_open)
{
    constexpr int max_length = 256;

    auto message_id =
        is_open ?
#if JK2_MODE
            "MENUS3_FAILED_TO_OPEN_SAVEGAME" :
            "MENUS3_FAILED_TO_CREATE_SAVEGAME"
#else
            "MENUS_FAILED_TO_OPEN_SAVEGAME" :
            "MENUS3_FAILED_TO_CREATE_SAVEGAME"
#endif
    ;

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
