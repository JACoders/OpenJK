//
// Saved game interface.
// (forward declaration)
//


#ifndef OJK_I_SAVED_GAME_FWD_INCLUDED
#define OJK_I_SAVED_GAME_FWD_INCLUDED


#include <cstdint>
#include <string>
#include <vector>


namespace ojk
{


class ISavedGame
{
public:
    using ChunkId = uint32_t;
    using Buffer = std::vector<uint8_t>;


    ISavedGame();

    ISavedGame(
        const ISavedGame& that) = delete;

    ISavedGame& operator=(
        const ISavedGame& that) = delete;

    virtual ~ISavedGame();


    // Returns true if the saved game opened for reading.
    virtual bool is_readable() const = 0;

    // Returns true if the saved game opened for writing.
    virtual bool is_writable() const = 0;


    // Reads a chunk from the file into the internal buffer.
    virtual bool read_chunk(
        const ChunkId chunk_id) = 0;

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

    // Returns true if all data read from the internal buffer.
    virtual bool is_all_data_read() const = 0;


    // Writes a chunk into the file from the internal buffer.
    virtual bool write_chunk(
        const ChunkId chunk_id) = 0;

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


    // Reads a raw data from the internal buffer.
    virtual void raw_read(
        void* dst_data,
        int dst_size) = 0;

    // Reads a value or array of values from the internal buffer.
    template<typename TSrc = void, typename TDst = void>
    void read(
        TDst& dst_value);

    // Reads an array of values with specificed count from the internal buffer.
    template<typename TSrc = void, typename TDst = void>
    void read(
        TDst* dst_values,
        int dst_count);


    // Writes a raw data into the internal buffer.
    virtual void raw_write(
        const void* src_data,
        int src_size) = 0;

    // Writes a value or array of values into the internal buffer.
    template<typename TDst = void, typename TSrc = void>
    void write(
        const TSrc& src_value);

    // Writes an array of values with specificed count into the internal buffer.
    template<typename TDst = void, typename TSrc = void>
    void write(
        const TSrc* src_values,
        int src_count);


    // Increments buffer's offset by the specified non-negative count.
    virtual void skip(
        int count) = 0;


    // Returns an I/O buffer.
    virtual const Buffer& get_buffer() const = 0;


    // Clears buffer and resets it's offset to the beginning.
    virtual void reset_buffer() = 0;


protected:
    // Tags for dispatching.
    class BooleanTag { public: };
    class NumericTag { public: };
    class PointerTag { public: };
    class ClassTag { public: };
    class Array1dTag { public: };
    class Array2dTag { public: };
    class InplaceTag { public: };
    class CastTag { public: };


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
        TDst (&dst_values)[TCount1][TCount2],
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
        const TSrc* src_value,
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
        const TSrc (&src_values)[TCount1][TCount2],
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
}; // ISavedGame


} // ojk


#endif // OJK_I_SAVED_GAME_FWD_INCLUDED
