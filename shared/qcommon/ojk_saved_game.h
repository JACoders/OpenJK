//
// Saved game.
// (forward declaration)
//


#ifndef OJK_SAVED_GAME_FWD_INCLUDED
#define OJK_SAVED_GAME_FWD_INCLUDED


#include <cstdint>
#include <string>
#include <vector>
#include "ojk_i_saved_game.h"


namespace ojk
{


class SavedGame :
    public ISavedGame
{
public:
    using ChunkId = uint32_t;


    SavedGame();

    SavedGame(
        const SavedGame& that) = delete;

    SavedGame& operator=(
        const SavedGame& that) = delete;

    virtual ~SavedGame();


    // Creates a new saved game file for writing.
    bool create(
        const std::string& base_file_name);

    // Opens an existing saved game file for reading.
    bool open(
        const std::string& base_file_name);

    // Closes the current saved game file.
    void close();


    // Returns true if the saved game opened for reading.
    bool is_readable() const override;

    // Returns true if the saved game opened for writing.
    bool is_writable() const override;


    // Reads a chunk from the file into the internal buffer.
    void read_chunk(
        const ChunkId chunk_id) override;

    using ISavedGame::read_chunk;


    // Returns true if all data read from the internal buffer.
    bool is_all_data_read() const override;

    // Throws an exception if all data not read.
    void ensure_all_data_read() const override;


    // Writes a chunk into the file from the internal buffer.
    void write_chunk(
        const ChunkId chunk_id) override;

    using ISavedGame::write_chunk;


    // Reads a raw data from the internal buffer.
    void raw_read(
        void* dst_data,
        int dst_size) override;

    using ISavedGame::read;


    // Writes a raw data into the internal buffer.
    void raw_write(
        const void* src_data,
        int src_size) override;

    using ISavedGame::write;


    bool is_write_failed() const;

    // Increments buffer's offset by the specified non-negative count.
    void skip(
        int count) override;


    // Stores current I/O buffer and it's position.
    void save_buffer() override;

    // Restores saved I/O buffer and it's position.
    void load_buffer() override;


    // Returns a pointer to data in the I/O buffer.
    const void* get_buffer_data() const override;

    // Returns a current size of the I/O buffer.
    int get_buffer_size() const override;


    // Clears buffer and resets it's offset to the beginning.
    void reset_buffer() override;

    // Resets buffer offset to the beginning.
    void reset_buffer_offset() override;


    // Renames a saved game file.
    static void rename(
        const std::string& old_base_file_name,
        const std::string& new_base_file_name);

    // Remove a saved game file.
    static void remove(
        const std::string& base_file_name);

    // Returns a default instance of the class.
    static SavedGame& get_instance();


protected:
    // If true won't throw an exception when buffer offset is beyond it's size.
    // Although, no data will be read beyond the buffer.
    void allow_read_overflow(
        bool value) override;


private:
    using Buffer = std::vector<uint8_t>;
    using BufferOffset = Buffer::size_type;
    using Paths = std::vector<std::string>;


    // A handle to a file.
    int32_t file_handle_;

    // I/O buffer.
    Buffer io_buffer_;

    // Saved copy of the I/O buffer.
    Buffer saved_io_buffer_;

    // A current offset inside the I/O buffer.
    BufferOffset io_buffer_offset_;

    // Saved I/O buffer offset.
    BufferOffset saved_io_buffer_offset_;

    // RLE codec buffer.
    Buffer rle_buffer_;

    // True if saved game opened for reading.
    bool is_readable_;

    // True if saved game opened for writing.
    bool is_writable_;

    // True if any previous write operation failed.
    bool is_write_failed_;

    // Controls exception throw on read overflow.
    bool is_read_overflow_allowed_;


    // Throws an exception.
    static void throw_error(
        const char* message);

    // Throws an exception.
    static void throw_error(
        const std::string& message);


    // Compresses data.
    static void compress(
        const Buffer& src_buffer,
        Buffer& dst_buffer);

    // Decompresses data.
    static void decompress(
        const Buffer& src_buffer,
        Buffer& dst_buffer);


    static std::string generate_path(
        const std::string& base_file_name);


    // Returns a string representation of a chunk id.
    static std::string get_chunk_id_string(
        uint32_t chunk_id);

    static constexpr uint32_t get_jo_magic_value();
}; // SavedGame


} // ojk


#endif // OJK_SAVED_GAME_FWD_INCLUDED
