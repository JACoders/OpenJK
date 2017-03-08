//
// Saved game interface implementation.
//


#ifndef OJK_SAVED_GAME_INCLUDED
#define OJK_SAVED_GAME_INCLUDED


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


	// Reads a chunk from the file into the internal buffer.
	bool read_chunk(
		const uint32_t chunk_id) override;


	// Returns true if all data read from the internal buffer.
	bool is_all_data_read() const override;

	// Calls error method if all data not read.
	void ensure_all_data_read() override;


	// Writes a chunk into the file from the internal buffer.
	// Returns true on success or false otherwise.
	bool write_chunk(
		const uint32_t chunk_id) override;


	// Reads a raw data from the internal buffer.
	// Returns true on success or false otherwise.
	bool read(
		void* dst_data,
		int dst_size) override;


	// Writes a raw data into the internal buffer.
	// Returns true on success or false otherwise.
	bool write(
		const void* src_data,
		int src_size) override;


	// Increments buffer's offset by the specified non-negative count.
	// Returns true on success or false otherwise.
	bool skip(
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


	// Returns true if read/write operation failed otherwise false.
	// Any chunk related try-method clears at the beginning this flag.
	bool is_failed() const override;

	// Clears error flag and message.
	void clear_error() override;

	// Calls Com_Error with last error message or with a generic one.
	void throw_error() override;


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
	using Buffer = std::vector<uint8_t>;
	using BufferOffset = Buffer::size_type;
	using Paths = std::vector<std::string>;


	// Last error message.
	std::string error_message_;

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

	// Error flag.
	bool is_failed_;


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

	static const uint32_t get_jo_magic_value();
}; // SavedGame


} // ojk


#endif // OJK_SAVED_GAME_INCLUDED
