//
// Saved game.
// (forward declaration)
//


#ifndef OJK_SAVED_GAME_FWD_INCLUDED
#define OJK_SAVED_GAME_FWD_INCLUDED


#include <cstdint>
#include <string>
#include <vector>


namespace ojk {


class SavedGame
{
public:
    using ChunkId = uint32_t;
    using Buffer = std::vector<uint8_t>;


    SavedGame();

    SavedGame(
        const SavedGame& that) = delete;

    SavedGame& operator=(
        const SavedGame& that) = delete;

    ~SavedGame();


    // Creates a new saved game file for writing.
    bool create(
        const std::string& base_file_name);

    // Opens an existing saved game file for reading.
    bool open(
        const std::string& base_file_name);

    // Closes the current saved game file.
    void close();


    // Reads a chunk from the file into the internal buffer.
    bool read_chunk(
        const ChunkId chunk_id);

    // Reads a value or an array of values from the file via
    // the internal buffer.
    template<typename TSrc = void, typename TDst = void>
    bool read_chunk(
        const ChunkId chunk_id,
        TDst& dst_value);

    // Reads an array of values with specified count from
    // the file via the internal buffer.
    template<typename TSrc = void, typename TDst = void>
    bool read_chunk(
        const ChunkId chunk_id,
        TDst* dst_values,
        int dst_count);


    // Writes a chunk into the file from the internal buffer.
    bool write_chunk(
        const ChunkId chunk_id);

    // Writes a value or an array of values into the file via
    // the internal buffer.
    template<typename TDst = void, typename TSrc = void>
    bool write_chunk(
        const ChunkId chunk_id,
        const TSrc& src_value);

    // Writes an array of values with specified count into
    // the file via the internal buffer.
    template<typename TDst = void, typename TSrc = void>
    bool write_chunk(
        const ChunkId chunk_id,
        const TSrc* src_values,
        int src_count);


    // Reads a value or array of values from the internal buffer.
    template<typename TSrc = void, typename TDst = void>
    void read(
        TDst& dst_value);

    // Reads an array of values with specificed count from the internal buffer.
    template<typename TSrc = void, typename TDst = void>
    void read(
        TDst* dst_values,
        int dst_count);


    // Writes a value or array of values into the internal buffer.
    template<typename TDst = void, typename TSrc = void>
    void write(
        const TSrc& src_value);

    // Writes an array of values with specificed count into the internal buffer.
    template<typename TDst = void, typename TSrc = void>
    void write(
        const TSrc* src_values,
        int src_count);


    // Returns an I/O buffer.
    Buffer& get_buffer();

    // Returns an I/O buffer.
    const Buffer& get_buffer() const;

    // Returns an I/O buffer offset;
    int get_buffer_offset() const;

    uint8_t* get_current_data();

    const uint8_t* get_current_data() const;

    // Casts referenced data at current position into a specified type.
    template<typename T>
    T cast_buffer();


    // Renames a saved game file.
    static void rename(
        const std::string& old_base_file_name,
        const std::string& new_base_file_name);

    // Remove a saved game file.
    static void remove(
        const std::string& base_file_name);

    // Returns a default instance of the class.
    static SavedGame& get_instance();


private:
    using BufferOffset = Buffer::size_type;
    using Paths = std::vector<std::string>;


    // Tags for dispatching.
    class BooleanTag { public: };
    class NumericTag { public: };
    class PointerTag { public: };
    class ClassTag { public: };
    class Array1dTag { public: };
    class Array2dTag { public: };
    class InplaceTag { public: };
    class CastTag { public: };


    // A handle to a file.
    int32_t file_handle_;

    // I/O buffer.
    Buffer io_buffer_;

    // A current offset inside the I/O buffer.
    BufferOffset io_buffer_offset_;

    // RLE codec buffer.
    Buffer rle_buffer_;

    // Does not throws an exception on chunk reading if true.
    bool is_preview_mode_;

    bool is_write_failed_;


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

    static std::string get_failed_to_open_message(
        const std::string& file_name,
        bool is_open);


    // Returns a string representation of a chunk id.
    static std::string get_chunk_id_string(
        uint32_t chunk_id);


    // Checks if there is enough data for reading in the I/O buffer.
    void check_io_buffer(
        int item_size,
        int count = 1);

    // Resizes the I/O buffer according to desire size of data to write.
    void accomodate_io_buffer(
        int item_size,
        int count = 1);

    // Advances the current I/O buffer offset.
    void advance_io_buffer(
        int item_size,
        int count = 1);

    // Resets I/O buffer offset to zero.
    void reset_io_buffer_offset();


    template<typename TSrc, typename TDst>
    void read(
        TDst& dst_value,
        BooleanTag);

    template<typename TSrc, typename TDst>
    void read(
        TDst& dst_value,
        NumericTag);

    template<typename TSrc, typename TDst>
    void read(
        TDst*& dst_value,
        PointerTag);

    template<typename TSrc, typename TDst>
    void read(
        TDst& dst_value,
        ClassTag);

    template<typename TSrc, typename TDst, int TCount>
    void read(
        TDst (&dst_values)[TCount],
        Array1dTag);

    template<typename TSrc, typename TDst, int TCount1, int TCount2>
    void read(
        TDst(&dst_values)[TCount1][TCount2],
        Array2dTag);


    template<typename TSrc, typename TDst>
    void read(
        TDst* dst_values,
        int dst_count,
        InplaceTag);

    template<typename TSrc, typename TDst>
    void read(
        TDst* dst_values,
        int dst_count,
        CastTag);


    template<typename TDst, typename TSrc>
    void write(
        const TSrc& src_value,
        NumericTag);

    template<typename TDst, typename TSrc>
    void write(
        const TSrc*& src_value,
        PointerTag);

    template<typename TDst, typename TSrc>
    void write(
        const TSrc& src_value,
        ClassTag);

    template<typename TDst, typename TSrc, int TCount>
    void write(
        const TSrc (&src_values)[TCount],
        Array1dTag);

    template<typename TDst, typename TSrc, int TCount1, int TCount2>
    void write(
        const TSrc(&src_values)[TCount1][TCount2],
        Array2dTag);


    template<typename TDst, typename TSrc>
    void write(
        const TSrc* src_values,
        int src_count,
        InplaceTag);

    template<typename TDst, typename TSrc>
    void write(
        const TSrc* src_values,
        int src_count,
        CastTag);
}; // SavedGame


} // ojk


#endif // OJK_SAVED_GAME_FWD_INCLUDED
