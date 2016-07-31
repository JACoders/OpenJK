#include "ojk_saved_game.h"
#include <algorithm>
#include "ojk_saved_game_exception.h"
#include "qcommon/qcommon.h"
#include "server/server.h"


namespace ojk
{


SavedGame::SavedGame() :
        file_handle_(),
        io_buffer_(),
        saved_io_buffer_(),
        io_buffer_offset_(),
        saved_io_buffer_offset_(),
        rle_buffer_(),
        is_readable_(),
        is_writable_(),
        is_write_failed_(),
        is_read_overflow_allowed_()
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


    const auto&& file_path = generate_path(
        base_file_name);

    auto is_succeed = true;

    if (is_succeed)
    {
        ::FS_FOpenFileRead(
            file_path.c_str(),
            &file_handle_,
            qtrue);

        if (file_handle_ == 0)
        {
            is_succeed = false;

            const auto&& error_message =
                S_COLOR_RED "Failed to open a saved game file: \"" +
                    file_path + "\".";

            ::Com_DPrintf(
                "%s\n",
                error_message.c_str());
        }
    }

    if (is_succeed)
    {
        is_readable_ = true;
    }

    auto sg_version = -1;

    if (is_succeed)
    {
        static_cast<void>(read_chunk<int32_t>(
            INT_ID('_', 'V', 'E', 'R'),
            sg_version));

        if (sg_version != iSAVEGAME_VERSION)
        {
            is_succeed = false;

            ::Com_Printf(
                S_COLOR_RED "File \"%s\" has version # %d (expecting %d)\n",
                base_file_name.c_str(),
                sg_version,
                iSAVEGAME_VERSION);
        }
    }

    if (!is_succeed)
    {
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

    const auto&& file_path = generate_path(
        base_file_name);

    file_handle_ = ::FS_FOpenFileWrite(
        file_path.c_str());

    if (file_handle_ == 0)
    {
        const auto&& error_message =
            S_COLOR_RED "Failed to create a saved game file: \"" +
                file_path + "\".";

        ::Com_Printf(
            "%s\n",
            error_message.c_str());

        return false;
    }


    is_writable_ = true;

    const auto sg_version = iSAVEGAME_VERSION;

    write_chunk<int32_t>(
        INT_ID('_', 'V', 'E', 'R'),
        sg_version);

    return true;
}

void SavedGame::close()
{
    if (file_handle_ != 0)
    {
        ::FS_FCloseFile(file_handle_);
        file_handle_ = 0;
    }

    io_buffer_.clear();
    io_buffer_offset_ = 0;

    is_readable_ = false;
    is_writable_ = false;
    is_write_failed_ = false;
}

bool SavedGame::is_readable() const
{
    return is_readable_;
}

bool SavedGame::is_writable() const
{
    return is_writable_;
}

void SavedGame::read_chunk(
    const SavedGame::ChunkId chunk_id)
{
    io_buffer_offset_ = 0;

    const auto&& chunk_id_string = get_chunk_id_string(
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

    const auto bBlockIsCompressed = (static_cast<int32_t>(uiLoadedLength) < 0);

    if (bBlockIsCompressed)
    {
        uiLoadedLength = -static_cast<int32_t>(uiLoadedLength);
    }

    // Make sure we are loading the correct chunk...
    //
    if (ulLoadedChid != chunk_id)
    {
        const auto&& loaded_chunk_id_string = get_chunk_id_string(
            ulLoadedChid);

        const auto&& error_message =
            "Loaded chunk ID (" +
            loaded_chunk_id_string +
            ") does not match requested chunk ID (" +
            chunk_id_string +
            ").";

        throw_error(
            error_message);
    }

    uint32_t uiLoadedCksum = 0;

#ifdef JK2_MODE
    // Get checksum...
    //
    uiLoaded += ::FS_Read(
        &uiLoadedCksum,
        static_cast<int>(sizeof(uiLoadedCksum)),
        file_handle_);
#endif // JK2_MODE

    // Load in data and magic number...
    //
    uint32_t uiCompressedLength = 0;

    if (bBlockIsCompressed)
    {
        uiLoaded += ::FS_Read(
            &uiCompressedLength,
            static_cast<int>(sizeof(uiCompressedLength)),
            file_handle_);

        rle_buffer_.resize(
            uiCompressedLength);

        uiLoaded += ::FS_Read(
            rle_buffer_.data(),
            uiCompressedLength,
            file_handle_);

        io_buffer_.resize(
            uiLoadedLength);

        decompress(
            rle_buffer_,
            io_buffer_);
    }
    else
    {
        io_buffer_.resize(
            uiLoadedLength);

        uiLoaded += ::FS_Read(
            io_buffer_.data(),
            uiLoadedLength,
            file_handle_);
    }

#ifdef JK2_MODE
    uint32_t uiLoadedMagic = 0;

    uiLoaded += ::FS_Read(
        &uiLoadedMagic,
        static_cast<int>(sizeof(uiLoadedMagic)),
        file_handle_);

    if (uiLoadedMagic != get_jo_magic_value())
    {
        const auto&& error_message =
            "Bad saved game magic for chunk " + chunk_id_string + ".";

        throw_error(
            error_message);
    }
#endif // JK2_MODE

#ifndef JK2_MODE
    // Get checksum...
    //
    uiLoaded += ::FS_Read(
        &uiLoadedCksum,
        static_cast<int>(sizeof(uiLoadedCksum)),
        file_handle_);
#endif // !JK2_MODE

    // Make sure the checksums match...
    //
    const auto uiCksum = ::Com_BlockChecksum(
        io_buffer_.data(),
        static_cast<int>(io_buffer_.size()));

    if (uiLoadedCksum != uiCksum)
    {
        const auto&& error_message =
            "Failed checksum check for chunk " + chunk_id_string + ".";

        throw_error(
            error_message);
    }

    // Make sure we didn't encounter any read errors...
    if (uiLoaded !=
        sizeof(ulLoadedChid) +
        sizeof(uiLoadedLength) +
        sizeof(uiLoadedCksum) +
        (bBlockIsCompressed ? sizeof(uiCompressedLength) : 0) +
        (bBlockIsCompressed ? uiCompressedLength : io_buffer_.size()) +
#ifdef JK2_MODE
        sizeof(uiLoadedMagic) +
#endif
        0)
    {
        const auto&& error_message =
            "Error during loading chunk " + chunk_id_string + ".";

        throw_error(
            error_message);
    }
}

bool SavedGame::is_all_data_read() const
{
    return io_buffer_.size() == io_buffer_offset_;
}

void SavedGame::ensure_all_data_read() const
{
    if (!is_all_data_read())
    {
        throw_error(
            "Not all expected data read.");
    }
}

void SavedGame::write_chunk(
    const SavedGame::ChunkId chunk_id)
{
    const auto&& chunk_id_string = get_chunk_id_string(
        chunk_id);

    ::Com_DPrintf(
        "Attempting write of chunk %s\n",
        chunk_id_string.c_str());

    if (::sv_testsave->integer != 0)
    {
        return;
    }

    const auto src_size = static_cast<int>(io_buffer_.size());

    const auto uiCksum = Com_BlockChecksum(
        io_buffer_.data(),
        src_size);

    uint32_t uiSaved = ::FS_Write(
        &chunk_id,
        static_cast<int>(sizeof(chunk_id)),
        file_handle_);

#ifdef JK2_MODE
    uiSaved += ::FS_Write(
        &uiCksum,
        static_cast<int>(sizeof(uiCksum)),
        file_handle_);
#endif // JK2_MODE

    auto iCompressedLength = -1;

    if (::sv_compress_saved_games->integer != 0)
    {
        compress(
            io_buffer_,
            rle_buffer_);

        if (rle_buffer_.size() < io_buffer_.size())
        {
            iCompressedLength = static_cast<int>(rle_buffer_.size());
        }
    }

#ifdef JK2_MODE
    const auto uiMagic = get_jo_magic_value();
#endif // JK2_MODE

    if (iCompressedLength > 0)
    {
        const auto iLength = -static_cast<int>(io_buffer_.size());

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

#ifdef JK2_MODE
        uiSaved += ::FS_Write(
            &uiMagic,
            static_cast<int>(sizeof(uiMagic)),
            file_handle_);
#endif // JK2_MODE

#ifndef JK2_MODE
        uiSaved += ::FS_Write(
            &uiCksum,
            static_cast<int>(sizeof(uiCksum)),
            file_handle_);
#endif // !JK2_MODE

        if (uiSaved !=
            sizeof(chunk_id) +
            sizeof(iLength) +
            sizeof(uiCksum) +
            sizeof(iCompressedLength) +
            iCompressedLength +
#ifdef JK2_MODE
            sizeof(uiMagic) +
#endif // JK2_MODE
            0)
        {
            is_write_failed_ = true;

            ::Com_Printf(
                S_COLOR_RED "Failed to write %s chunk\n",
                chunk_id_string.c_str());

            return;
        }
    }
    else
    {
        const auto iLength = static_cast<uint32_t>(io_buffer_.size());

        uiSaved += ::FS_Write(
            &iLength,
            static_cast<int>(sizeof(iLength)),
            file_handle_);

        uiSaved += ::FS_Write(
            io_buffer_.data(),
            iLength,
            file_handle_);

#ifdef JK2_MODE
        uiSaved += ::FS_Write(
            &uiMagic,
            static_cast<int>(sizeof(uiMagic)),
            file_handle_);
#endif // JK2_MODE

#ifdef JK2_MODE
        uiSaved += ::FS_Write(
            &uiCksum,
            static_cast<int>(sizeof(uiCksum)),
            file_handle_);
#endif // JK2_MODE

        if (uiSaved !=
            sizeof(chunk_id) +
            sizeof(iLength) +
            sizeof(uiCksum) +
            iLength +
#ifdef JK2_MODE
            sizeof(uiMagic) +
#endif // JK2_MODE
            0)
        {
            is_write_failed_ = true;

            ::Com_Printf(
                S_COLOR_RED "Failed to write %s chunk\n",
                chunk_id_string.c_str());

            return;
        }
    }
}

void SavedGame::raw_read(
    void* dst_data,
    int dst_size)
{
    if (!dst_data)
    {
        throw_error(
            "Null pointer.");
    }

    if (dst_size < 0)
    {
        throw_error(
            "Negative size.");
    }

    if (!is_readable_)
    {
        throw_error(
            "Not readable.");
    }

    if (dst_size == 0)
    {
        return;
    }

    auto is_overflowed = ((io_buffer_offset_ + dst_size) > io_buffer_.size());

    if (is_overflowed)
    {
        if (!is_read_overflow_allowed_)
        {
            throw_error(
                "Not enough data.");
        }
    }

    if (!is_overflowed)
    {
        std::uninitialized_copy_n(
            &io_buffer_[io_buffer_offset_],
            dst_size,
            static_cast<uint8_t*>(dst_data));
    }

    io_buffer_offset_ += dst_size;
}

void SavedGame::raw_write(
    const void* src_data,
    int src_size)
{
    if (!src_data)
    {
        throw_error(
            "Null pointer.");
    }

    if (src_size < 0)
    {
        throw_error(
            "Negative size.");
    }

    if (!is_writable_)
    {
        throw_error(
            "Not writable.");
    }

    if (src_size == 0)
    {
        return;
    }

    const auto new_buffer_size = io_buffer_offset_ + src_size;

    io_buffer_.resize(
        new_buffer_size);

    std::uninitialized_copy_n(
        static_cast<const uint8_t*>(src_data),
        src_size,
        &io_buffer_[io_buffer_offset_]);

    io_buffer_offset_ = new_buffer_size;
}

bool SavedGame::is_write_failed() const
{
    return is_write_failed_;
}

void SavedGame::skip(
    int count)
{
    if (!is_readable_ && !is_writable_)
    {
        throw_error(
            "Not open or created.");
    }

    if (count < 0)
    {
        throw_error(
            "Negative count.");
    }

    if (count == 0)
    {
        return;
    }

    const auto new_offset = io_buffer_offset_ + count;
    const auto buffer_size = io_buffer_.size();

    if (new_offset > buffer_size)
    {
        if (is_readable_)
        {
            throw_error(
                "Not enough data.");
        }
        else if (is_writable_)
        {
            if (new_offset > buffer_size)
            {
                io_buffer_.resize(
                    new_offset);
            }
        }
    }

    io_buffer_offset_ = new_offset;
}

void SavedGame::save_buffer()
{
    saved_io_buffer_ = io_buffer_;
    saved_io_buffer_offset_ = io_buffer_offset_;
}

void SavedGame::load_buffer()
{
    io_buffer_ = saved_io_buffer_;
    io_buffer_offset_ = saved_io_buffer_offset_;
}

const void* SavedGame::get_buffer_data() const
{
    return io_buffer_.data();
}

int SavedGame::get_buffer_size() const
{
    return static_cast<int>(io_buffer_.size());
}

void SavedGame::rename(
    const std::string& old_base_file_name,
    const std::string& new_base_file_name)
{
    const auto&& old_path = generate_path(
        old_base_file_name);

    const auto&& new_path = generate_path(
        new_base_file_name);

    const auto rename_result = ::FS_MoveUserGenFile(
        old_path.c_str(),
        new_path.c_str());

    if (rename_result == 0)
    {
        ::Com_Printf(
            S_COLOR_RED "Error during savegame-rename."
                " Check \"%s\" for write-protect or disk full!\n",
            new_path.c_str());
    }
}

void SavedGame::remove(
    const std::string& base_file_name)
{
    const auto&& path = generate_path(
        base_file_name);

    ::FS_DeleteUserGenFile(
        path.c_str());
}

SavedGame& SavedGame::get_instance()
{
    static SavedGame result;
    return result;
}

void SavedGame::allow_read_overflow(
    bool value)
{
    is_read_overflow_allowed_ = value;
}

void SavedGame::throw_error(
    const char* message)
{
    throw SavedGameException(
        message);
}

void SavedGame::throw_error(
    const std::string& message)
{
    throw SavedGameException(
        message);
}

void SavedGame::compress(
    const Buffer& src_buffer,
    Buffer& dst_buffer)
{
    const auto src_size = static_cast<int>(src_buffer.size());

    dst_buffer.resize(2 * src_size);

    auto src_count = 0;
    auto dst_index = 0;

    while (src_count < src_size)
    {
        auto src_index = src_count;
        auto b = src_buffer[src_index++];

        while (src_index < src_size &&
            (src_index - src_count) < 127 &&
            src_buffer[src_index] == b)
        {
            src_index += 1;
        }

        if ((src_index - src_count) == 1)
        {
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

            for (auto i = src_count; i < src_index; ++i)
            {
                dst_buffer[dst_index++] = src_buffer[i];
            }
        }
        else
        {
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
    auto src_index = 0;
    auto dst_index = 0;

    auto remain_size = static_cast<int>(dst_buffer.size());

    while (remain_size > 0)
    {
        auto count = static_cast<int8_t>(src_buffer[src_index++]);

        if (count > 0)
        {
            std::uninitialized_fill_n(
                &dst_buffer[dst_index],
                count,
                src_buffer[src_index++]);
        }
        else
        {
            if (count < 0)
            {
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

    auto&& path = "saves/" + normalized_file_name + ".sav";

    return path;
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

void SavedGame::reset_buffer()
{
    io_buffer_.clear();
    reset_buffer_offset();
}

void SavedGame::reset_buffer_offset()
{
    io_buffer_offset_ = 0;
}

constexpr uint32_t SavedGame::get_jo_magic_value()
{
    return 0x1234ABCD;
}


} // ojk
