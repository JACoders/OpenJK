//
// Saved game archive.
// (forward declaration)
//


#ifndef OJK_SG_ARCHIVE_FWD_INCLUDED
#define OJK_SG_ARCHIVE_FWD_INCLUDED


#include <cstdint>
#include <string>
#include <vector>
#include "ojk_sg_archive_exception.h"


using fileHandle_t = int32_t;


namespace ojk {
namespace sg {


enum class ArchiveMode {
    jedi_outcast,
    jedi_academy,
}; // ArchiveMode


class Archive
{
public:
    using ChunkId = uint32_t;


    Archive();

    Archive(
        const Archive& that) = delete;

    Archive& operator=(
        const Archive& that) = delete;

    ~Archive();


    // Creates a new saved game file for writing.
    bool create(
        const std::string& file_path);

    // Opens an existing saved game file for reading.
    bool open(
        const std::string& file_path);

    // Closes the current saved game file.
    void close();


    // Reads a chunk from the file into the internal buffer.
    void read_chunk(
        const ChunkId chunk_id);

    // Reads a value or an array of values from the file via
    // the internal buffer.
    template<typename TSrc = void, typename TDst = void>
    void read_chunk(
        const ChunkId chunk_id,
        TDst& dst_value);

    // Reads an array of values with specified count from
    // the file via the internal buffer.
    template<typename TSrc = void, typename TDst = void>
    void read_chunk(
        const ChunkId chunk_id,
        TDst* dst_values,
        int dst_count);


    // Writes a chunk into the file from the internal buffer.
    void write_chunk(
        const ChunkId chunk_id);

    // Writes a value or an array of values into the file via
    // the internal buffer.
    template<typename TDst = void, typename TSrc = void>
    void write_chunk(
        const ChunkId chunk_id,
        const TSrc& src_value);

    // Writes an array of values with specified count into
    // the file via the internal buffer.
    template<typename TDst = void, typename TSrc = void>
    void write_chunk(
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


    // Renames a saved game file.
    static void rename(
        const std::string& old_file_path,
        const std::string& new_file_path);

    // Remove a saved game file.
    static void remove(
        const std::string& file_path);

    // Returns a default instance of the class.
    static Archive& get_instance();


private:
    using Buffer = std::vector<uint8_t>;
    using BufferOffset = Buffer::size_type;

    // Tags for dispatching.
    class BooleanTag { public: };
    class NumericTag { public: };
    class PointerTag { public: };
    class ClassTag { public: };
    class Array1dTag { public: };
    class InplaceTag { public: };
    class CastTag { public: };

    // A handle to a file.
    fileHandle_t file_handle_;

    // I/O buffer.
    Buffer io_buffer_;

    // A current offset inside the I/O buffer.
    BufferOffset io_buffer_offset_;


    // Checks if there is enough data for reading in the I/O buffer.
    template<typename T>
    void check_io_buffer(
        int count = 1);

    // Resizes the I/O buffer according to desire size of data to write.
    template<typename T>
    void accomodate_io_buffer(
        int count = 1);

    // Casts I/O buffer data at the current offset.
    template<typename T>
    T cast_io_buffer();

    // Advances the current I/O buffer offset.
    template<typename T>
    void advance_io_buffer(
        int count = 1);


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
}; // Archive


} // sg
} // ojk


#endif // OJK_SG_ARCHIVE_FWD_INCLUDED
