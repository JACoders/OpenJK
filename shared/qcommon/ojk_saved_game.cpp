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
        is_preview_mode_(),
        is_write_failed_()
{
}

SavedGame::~SavedGame()
{
    close();
}

bool SavedGame::open(
    const std::string& base_file_name)
{
    close();


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
    close();


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

    is_write_failed_ = false;
}

bool SavedGame::read_chunk(
    const SavedGame::ChunkId chunk_id)
{
    auto chunk_id_string = get_chunk_id_string(
        chunk_id);

    ::Com_DPrintf(
        "Attempting read of chunk %s\n",
        chunk_id_string.c_str());

    uint32_t ulLoadedChid = 0;
    uint32_t uiLoadedLength = 0;

    auto uiLoaded = ::FS_Read(
        &ulLoadedChid,
        static_cast<int>(sizeof(ulLoadedChid)),
        file_handle_);

    uiLoaded += ::FS_Read(
        &uiLoadedLength,
        static_cast<int>(sizeof(uiLoadedLength)),
        file_handle_);

    auto bBlockIsCompressed = (static_cast<int32_t>(uiLoadedLength) < 0);

    if (bBlockIsCompressed) {
        uiLoadedLength = -static_cast<int32_t>(uiLoadedLength);
    }

    // Make sure we are loading the correct chunk...
    //
    if (ulLoadedChid != chunk_id) {
        auto loaded_chunk_id_string = get_chunk_id_string(ulLoadedChid);

        if (!is_preview_mode_) {
            ::Com_Error(
                ERR_DROP,
                "Loaded chunk ID (%s) does not match requested chunk ID (%s)",
                loaded_chunk_id_string.c_str(),
                chunk_id_string.c_str());
        }

        return false;
    }

    // Load in data and magic number...
    //
    uint32_t uiCompressedLength = 0;

    if (bBlockIsCompressed) {
        uiLoaded += ::FS_Read(
            &uiCompressedLength,
            static_cast<int>(uiCompressedLength),
            file_handle_);

        rle_buffer_.resize(
            uiCompressedLength);

        uiLoaded += ::FS_Read(
            rle_buffer_.data(),
            uiCompressedLength,
            file_handle_);

        decompress(
            rle_buffer_,
            io_buffer_);
    } else {
        io_buffer_.resize(
            uiLoadedLength);

        uiLoaded += ::FS_Read(
            io_buffer_.data(),
            uiLoadedLength,
            file_handle_);
    }

    // Get checksum...
    //
    uint32_t uiLoadedCksum = 0;

    uiLoaded += ::FS_Read(
        &uiLoadedCksum,
        static_cast<int>(sizeof(uiLoadedCksum)),
        file_handle_);

    // Make sure the checksums match...
    //
    auto uiCksum = ::Com_BlockChecksum(
        io_buffer_.data(),
        static_cast<int>(io_buffer_.size()));

    if (uiLoadedCksum != uiCksum) {
        if (!is_preview_mode_) {
            ::Com_Error(
                ERR_DROP,
                "Failed checksum check for chunk",
                chunk_id_string.c_str());
        }

        return false;
    }

    // Make sure we didn't encounter any read errors...
    if (uiLoaded !=
            sizeof(ulLoadedChid) +
            sizeof(uiLoadedLength) +
            sizeof(uiLoadedCksum) +
            (bBlockIsCompressed ? sizeof(uiCompressedLength) : 0) +
            (bBlockIsCompressed ? uiCompressedLength : io_buffer_.size()))
    {
        if (!is_preview_mode_) {
            ::Com_Error(
                ERR_DROP,
                "Error during loading chunk %s",
                chunk_id_string.c_str());
        }

        return false;
    }

    return true;
}

bool SavedGame::write_chunk(
    const SavedGame::ChunkId chunk_id)
{
    auto chunk_id_string = get_chunk_id_string(
        chunk_id);

    ::Com_DPrintf(
        "Attempting write of chunk %s\n",
        chunk_id_string.c_str());

    if (::sv_testsave->integer != 0) {
        return true;
    }

    auto src_size = static_cast<int>(io_buffer_.size());

    auto uiCksum = Com_BlockChecksum(
        io_buffer_.data(),
        src_size);

    uint32_t uiSaved = ::FS_Write(
        &chunk_id,
        static_cast<int>(sizeof(chunk_id)),
        file_handle_);

    int iCompressedLength = 0;

    if (::sv_compress_saved_games->integer == 0) {
        iCompressedLength = -1;
    } else {
        compress(
            io_buffer_,
            rle_buffer_);

        if (rle_buffer_.size() < io_buffer_.size()) {
            iCompressedLength = static_cast<int>(rle_buffer_.size());
        }
    }

    if (iCompressedLength >= 0) {
        auto iLength = -static_cast<int>(io_buffer_.size());

        uiSaved += ::FS_Write(
            &iLength,
            static_cast<int>(sizeof(iLength)),
            file_handle_);

        uiSaved += ::FS_Write(
            &iCompressedLength,
            static_cast<int>(sizeof(iCompressedLength)),
            file_handle_);

        uiSaved += ::FS_Write(
            rle_buffer_.data(),
            iCompressedLength,
            file_handle_);

        uiSaved += ::FS_Write(
            &uiCksum,
            static_cast<int>(sizeof(uiCksum)),
            file_handle_);

        if (uiSaved !=
            sizeof(chunk_id) +
            sizeof(iLength) +
            sizeof(uiCksum) +
            sizeof(iCompressedLength) +
            iCompressedLength)
        {
            is_write_failed_ = true;

            ::Com_Printf(
                S_COLOR_RED "Failed to write %s chunk\n",
                chunk_id_string.c_str());

            return false;
        }
    } else {
        auto iLength = static_cast<uint32_t>(io_buffer_.size());

        uiSaved += ::FS_Write(
            &iLength,
            static_cast<int>(sizeof(iLength)),
            file_handle_);

        uiSaved += ::FS_Write(
            io_buffer_.data(),
            iLength,
            file_handle_);

        uiSaved += ::FS_Write(
            &uiCksum,
            static_cast<int>(sizeof(uiCksum)),
            file_handle_);

        if (uiSaved !=
            sizeof(chunk_id) +
            sizeof(iLength) +
            sizeof(uiCksum) +
            iLength)
        {
            is_write_failed_ = true;

            ::Com_Printf(
                S_COLOR_RED "Failed to write %s chunk\n",
                chunk_id_string.c_str());

            return false;
        }
    }

    return true;
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

void SavedGame::compress(
    const Buffer& src_buffer,
    Buffer& dst_buffer)
{
    auto src_size = static_cast<int>(src_buffer.size());

    dst_buffer.resize(2 * src_size);

    int src_count = 0;
    int dst_index = 0;

    while (src_count < src_size) {
        auto src_index = src_count;
        auto b = src_buffer[src_index++];

        while (src_index < src_size &&
            (src_index - src_count) < 127 &&
            src_buffer[src_index] == b)
        {
            src_index += 1;
        }

        if ((src_index - src_count) == 1) {
            while (src_index < src_size &&
                (src_index - src_count) < 127 && (
                    src_buffer[src_index] != src_buffer[src_index - 1] || (
                        src_index > 1 &&
                        src_buffer[src_index] != src_buffer[src_index - 2])))
            {
                src_index += 1;
            }

            while (src_index < src_size &&
                src_buffer[src_index] == src_buffer[src_index - 1])
            {
                src_index -= 1;
            }

            dst_buffer[dst_index++] =
                static_cast<uint8_t>(src_count - src_index);

            for (auto i = src_count; i < src_index; ++i) {
                dst_buffer[dst_index++] = src_buffer[i];
            }
        } else {
            dst_buffer[dst_index++] =
                static_cast<uint8_t>(src_index - src_count);

            dst_buffer[dst_index++] = b;
        }

        src_count = src_index;
    }

    dst_buffer.resize(
        dst_index);
}

void SavedGame::decompress(
    const Buffer& src_buffer,
    Buffer& dst_buffer)
{
    int src_index = 0;
    int dst_index = 0;

    auto remain_size = static_cast<int>(dst_buffer.size());

    while (remain_size > 0) {
        auto count = static_cast<int8_t>(src_buffer[src_index++]);

        if (count > 0) {
            std::uninitialized_fill_n(
                &dst_buffer[dst_index],
                count,
                src_buffer[src_index++]);
        } else {
            if (count < 0) {
                count = -count;

                std::uninitialized_copy_n(
                    &src_buffer[src_index],
                    count,
                    &dst_buffer[dst_index]);

                src_index += count;
            }
        }

        dst_index += count;
        remain_size -= count;
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

std::string SavedGame::get_chunk_id_string(
    uint32_t chunk_id)
{
    std::string result(4, '\0');

    result[0] = static_cast<char>((chunk_id >> 24) & 0xFF);
    result[1] = static_cast<char>((chunk_id >> 16) & 0xFF);
    result[2] = static_cast<char>((chunk_id >> 8) & 0xFF);
    result[3] = static_cast<char>((chunk_id >> 0) & 0xFF);

    return result;
}


} // ojk
