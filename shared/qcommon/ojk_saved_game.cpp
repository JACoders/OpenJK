#include "ojk_saved_game.h"
#include "ojk_saved_game_exception.h"
#include "qcommon/qcommon.h"
#include "server/server.h"


namespace ojk {


SavedGame::SavedGame() :
        file_handle_(),
        io_buffer_(),
        io_buffer_offset_(),
        rle_buffer_(),
        is_testing_read_chunk_()
{
}

SavedGame::~SavedGame()
{
    close();
}

bool SavedGame::open(
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
        static_cast<void>(read_chunk<int32_t>(
            INT_ID('_', 'V', 'E', 'R'),
            sg_version));

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

bool SavedGame::create(
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

    static_cast<void>(write_chunk<int32_t>(
        INT_ID('_', 'V', 'E', 'R'),
        sg_version));

    return true;
}

void SavedGame::close()
{
    if (file_handle_ != 0) {
        ::FS_FCloseFile(file_handle_);
        file_handle_ = 0;
    }

    io_buffer_.clear();
    io_buffer_offset_ = 0;
}

bool SavedGame::read_chunk(
    const SavedGame::ChunkId chunk_id)
{
    throw SavedGameException(
        "Not implemented.");
}

bool SavedGame::write_chunk(
    const SavedGame::ChunkId chunk_id)
{
    throw SavedGameException(
        "Not implemented.");
}

void SavedGame::rename(
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

void SavedGame::remove(
    const std::string& base_file_name)
{
    auto path = generate_path(
        base_file_name);

    ::FS_DeleteUserGenFile(
        path.c_str());
}

SavedGame& SavedGame::get_instance()
{
    static SavedGame result;
    return result;
}

int SavedGame::compress()
{
    auto src_size = static_cast<int>(io_buffer_.size());

    rle_buffer_.resize(2 * src_size);

    int src_count = 0;
    int dst_index = 0;

    while (src_count < src_size) {
        auto src_index = src_count;
        auto b = io_buffer_[src_index++];

        while (src_index < src_size &&
            (src_index - src_count) < 127 &&
            io_buffer_[src_index] == b)
        {
            src_index += 1;
        }

        if ((src_index - src_count) == 1) {
            while (src_index < src_size &&
                (src_index - src_count) < 127 && (
                    io_buffer_[src_index] != io_buffer_[src_index - 1] || (
                        src_index > 1 &&
                        io_buffer_[src_index] != io_buffer_[src_index - 2])))
            {
                src_index += 1;
            }

            while (src_index < src_size &&
                io_buffer_[src_index] == io_buffer_[src_index - 1])
            {
                src_index -= 1;
            }

            rle_buffer_[dst_index++] =
                static_cast<uint8_t>(src_count - src_index);

            for (auto i = src_count; i < src_index; ++i) {
                rle_buffer_[dst_index++] = io_buffer_[i];
            }
        } else {
            rle_buffer_[dst_index++] =
                static_cast<uint8_t>(src_index - src_count);

            rle_buffer_[dst_index++] = b;
        }

        src_count = src_index;
    }

    rle_buffer_.resize(
        dst_index);

    return dst_index;
}

void SavedGame::decompress(
    int dst_size)
{
    rle_buffer_.resize(
        dst_size);

    int src_index = 0;
    int dst_index = 0;

    while (dst_size > 0) {
        auto count = static_cast<int8_t>(io_buffer_[src_index++]);

        if (count > 0) {
            std::uninitialized_fill_n(
                &rle_buffer_[dst_index],
                count,
                io_buffer_[src_index++]);
        } else {
            if (count < 0) {
                count = -count;

                std::uninitialized_copy_n(
                    &io_buffer_[src_index],
                    count,
                    &rle_buffer_[dst_index]);

                src_index += count;
            }
        }

        dst_index += count;
        dst_size -= count;
    }
}

std::string SavedGame::generate_path(
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

std::string SavedGame::get_failed_to_open_message(
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


} // ojk
