//
// Saved game interface.
//


#ifndef OJK_I_SAVED_GAME_INCLUDED
#define OJK_I_SAVED_GAME_INCLUDED


#include <cstdint>


namespace ojk
{


class ISavedGame
{
public:
	ISavedGame()
	{
	}

	ISavedGame(
		const ISavedGame& that) = delete;

	ISavedGame& operator=(
		const ISavedGame& that) = delete;

	virtual ~ISavedGame()
	{
	}


	// Reads a chunk from the file into the internal buffer.
	// Returns true on success or false otherwise.
	virtual bool read_chunk(
		const uint32_t chunk_id) = 0;

	// Returns true if all data read from the internal buffer.
	virtual bool is_all_data_read() const = 0;

	// Calls error method if all data not read from the internal buffer.
	virtual void ensure_all_data_read() = 0;


	// Writes a chunk into the file from the internal buffer.
	// Returns true on success or false otherwise.
	virtual bool write_chunk(
		const uint32_t chunk_id) = 0;


	// Reads data from the internal buffer.
	// Returns true on success or false otherwise.
	virtual bool read(
		void* dst_data,
		int dst_size) = 0;

	// Writes data into the internal buffer.
	// Returns true on success or false otherwise.
	virtual bool write(
		const void* src_data,
		int src_size) = 0;

	// Increments buffer's offset by the specified non-negative count.
	// Returns true on success or false otherwise.
	virtual bool skip(
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


	// Error flag.
	// Returns true if read/write operation failed otherwise false.
	virtual bool is_failed() const = 0;

	// Clears error flag and message.
	virtual void clear_error() = 0;

	// Calls Com_Error with last error message or with a generic one.
	virtual void throw_error() = 0;
}; // ISavedGame


} // ojk


#endif // OJK_I_SAVED_GAME_INCLUDED
