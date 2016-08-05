//
// Saved game stream interface.
//


#ifndef OJK_I_SAVED_GAME_STREAM_INCLUDED
#define OJK_I_SAVED_GAME_STREAM_INCLUDED


#include <cstdint>


namespace ojk
{


class ISavedGameStream
{
public:
	ISavedGameStream()
	{
	}

	ISavedGameStream(
		const ISavedGameStream& that) = delete;

	ISavedGameStream& operator=(
		const ISavedGameStream& that) = delete;

	virtual ~ISavedGameStream()
	{
	}


	// Returns true if the saved game opened for reading.
	virtual bool is_readable() const = 0;

	// Returns true if the saved game opened for writing.
	virtual bool is_writable() const = 0;


	// Reads a chunk from the file into the internal buffer.
	virtual void read_chunk(
		const uint32_t chunk_id) = 0;

	// Returns true if all data read from the internal buffer.
	virtual bool is_all_data_read() const = 0;

	// Throws an exception if all data not read.
	virtual void ensure_all_data_read() const = 0;


	// Writes a chunk into the file from the internal buffer.
	virtual void write_chunk(
		const uint32_t chunk_id) = 0;


	// Reads a raw data from the internal buffer.
	virtual void read(
		void* dst_data,
		int dst_size) = 0;

	// Writes a raw data into the internal buffer.
	virtual void write(
		const void* src_data,
		int src_size) = 0;

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


	// If true won't throw an exception when buffer offset is beyond it's size.
	// Although, no data will be read beyond the buffer.
	virtual void allow_read_overflow(
		bool value) = 0;
}; // ISavedGameStream


} // ojk


#endif // OJK_I_SAVED_GAME_STREAM_INCLUDED
