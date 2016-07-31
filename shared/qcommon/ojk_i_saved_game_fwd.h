//
// Saved game interface.
// (forward declaration)
//


#ifndef OJK_I_SAVED_GAME_FWD_INCLUDED
#define OJK_I_SAVED_GAME_FWD_INCLUDED


#include <cstdint>
#include <string>


namespace ojk
{


class ISavedGame
{
public:
	using ChunkId = uint32_t;


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
	virtual void read_chunk(
		const ChunkId chunk_id) = 0;

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

	// Returns true if all data read from the internal buffer.
	virtual bool is_all_data_read() const = 0;

	// Throws an exception if all data not read.
	virtual void ensure_all_data_read() const = 0;


	// Writes a chunk into the file from the internal buffer.
	virtual void write_chunk(
		const ChunkId chunk_id) = 0;

	// Writes a data-chunk into the file from the internal buffer
	// prepended with a size-chunk that holds a size of the data-chunk.
	template<typename TSize>
	void write_chunk_and_size(
		const ChunkId size_chunk_id,
		const ChunkId data_chunk_id);

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


	// Tries to read a value or array of values from the internal buffer.
	// Returns true on success or false otherwise.
	template<typename TSrc = void, typename TDst = void>
	bool try_read(
		TDst& dst_value);


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


	// Stores current I/O buffer and it's position.
	virtual void save_buffer() = 0;

	// Restores saved I/O buffer and it's position.
	virtual void load_buffer() = 0;

	// Returns a pointer to data in the I/O buffer.
	virtual const void* get_buffer_data() const = 0;

	// Returns a current size of the I/O buffer.
	virtual int get_buffer_size() const = 0;


	// Clears buffer and resets it's offset to the beginning.
	virtual void reset_buffer() = 0;

	// Resets buffer offset to the beginning.
	virtual void reset_buffer_offset() = 0;


protected:
	// Tags for dispatching.
	class BooleanTag
	{
	public:
	};
	class NumericTag
	{
	public:
	};
	class PointerTag
	{
	public:
	};
	class ClassTag
	{
	public:
	};
	class Array1dTag
	{
	public:
	};
	class Array2dTag
	{
	public:
	};
	class InplaceTag
	{
	public:
	};
	class CastTag
	{
	public:
	};


	// If true won't throw an exception when buffer offset is beyond it's size.
	// Although, no data will be read beyond the buffer.
	virtual void allow_read_overflow(
		bool value) = 0;


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
		TDst(&dst_values)[TCount],
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
		const TSrc* src_value,
		PointerTag);

	template<typename TDst, typename TSrc>
	void write(
		const TSrc& src_value,
		ClassTag);

	template<typename TDst, typename TSrc, int TCount>
	void write(
		const TSrc(&src_values)[TCount],
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
}; // ISavedGame


} // ojk


#endif // OJK_I_SAVED_GAME_FWD_INCLUDED
