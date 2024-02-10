//
// Saved game helper.
// (forward declaration)
//


#ifndef OJK_SAVED_GAME_HELPER_FWD_INCLUDED
#define OJK_SAVED_GAME_HELPER_FWD_INCLUDED


#include "ojk_i_saved_game.h"


namespace ojk
{


class SavedGameHelper
{
public:
	SavedGameHelper(
		ISavedGame* saved_game);


	// Reads a chunk from the file into the internal buffer.
	// Calls error method on failure.
	void read_chunk(
		const uint32_t chunk_id);

	// Writes a chunk into the file from the internal buffer.
	// Calls error method on failure.
	void write_chunk(
		const uint32_t chunk_id);

	// Increments buffer's offset by the specified non-negative count.
	// Calls error method on failure.
	void skip(
		int count);

	// Clears buffer and resets it's offset to the beginning.
	const void* get_buffer_data();

	// Returns a current size of the I/O buffer.
	int get_buffer_size() const;

	// Clears buffer and resets it's offset to the beginning.
	void reset_buffer();

	// Resets buffer offset to the beginning.
	void reset_buffer_offset();

	// Calls error method if not all data read.
	void ensure_all_data_read();

	// Error flag.
	// Returns true if read/write operation failed otherwise false.
	bool is_failed() const;


	// Tries to read a chunk's data into the internal buffer.
	// Return true on success or false otherwise.
	bool try_read_chunk(
		const uint32_t chunk_id);

	// Tries to read a value or an array of values from the file via
	// the internal buffer.
	// Return true on success or false otherwise.
	template<typename TSrc = void, typename TDst = void>
	bool try_read_chunk(
		const uint32_t chunk_id,
		TDst& dst_value);

	// Reads a value or an array of values from the file via
	// the internal buffer.
	// Calls error method on error.
	template<typename TSrc = void, typename TDst = void>
	void read_chunk(
		const uint32_t chunk_id,
		TDst& dst_value);

	// Reads an array of values with specified count from
	// the file via the internal buffer.
	// Return true on success or false otherwise.
	template<typename TSrc = void, typename TDst = void>
	bool try_read_chunk(
		const uint32_t chunk_id,
		TDst* dst_values,
		int dst_count);

	// Reads an array of values with specified count from
	// the file via the internal buffer.
	// Calls error method on error.
	template<typename TSrc = void, typename TDst = void>
	void read_chunk(
		const uint32_t chunk_id,
		TDst* dst_values,
		int dst_count);

	// Writes a data-chunk into the file from the internal buffer
	// prepended with a size-chunk that holds a size of the data-chunk.
	template<typename TSize>
	void write_chunk_and_size(
		const uint32_t size_chunk_id,
		const uint32_t data_chunk_id);

	// Writes a value or an array of values into the file via
	// the internal buffer.
	template<typename TDst = void, typename TSrc = void>
	void write_chunk(
		const uint32_t chunk_id,
		const TSrc& src_value);

	// Writes an array of values with specified count into
	// the file via the internal buffer.
	template<typename TDst = void, typename TSrc = void>
	void write_chunk(
		const uint32_t chunk_id,
		const TSrc* src_values,
		int src_count);


	// Tries to read a value or array of values from the internal buffer.
	// Returns true on success or false otherwise.
	template<typename TSrc = void, typename TDst = void>
	bool try_read(
		TDst& dst_value);

	// Reads a value or array of values from the internal buffer.
	// Calls error method on error.
	template<typename TSrc = void, typename TDst = void>
	void read(
		TDst& dst_value);

	// Tries to read an array of values with specificed count from the internal buffer.
	// Calls error method on error.
	// Return true on success or false otherwise.
	template<typename TSrc = void, typename TDst = void>
	bool try_read(
		TDst* dst_values,
		int dst_count);

	// Reads an array of values with specificed count from the internal buffer.
	// Calls error method on error.
	template<typename TSrc = void, typename TDst = void>
	void read(
		TDst* dst_values,
		int dst_count);


	// Writes a value or array of values into the internal buffer.
	// Returns true on success or false otherwise.
	template<typename TDst = void, typename TSrc = void>
	void write(
		const TSrc& src_value);

	// Writes an array of values with specificed count into the internal buffer.
	// Returns true on success or false otherwise.
	template<typename TDst = void, typename TSrc = void>
	void write(
		const TSrc* src_values,
		int src_count);


private:
	ISavedGame* saved_game_;


	// Tags for dispatching.
	class BooleanTag { public: };
	class NumericTag { public: };
	class PointerTag { public: };
	class ClassTag { public: };
	class Array1dTag { public: };
	class Array2dTag { public: };
	class InplaceTag { public: };
	class CastTag { public: };
	class InternalTag { public: };
	class ExternalTag { public: };


	template<typename TSrc, typename TDst>
	bool try_read(
		TDst& dst_value,
		BooleanTag);

	template<typename TSrc, typename TDst>
	bool try_read(
		TDst& dst_value,
		NumericTag);

	template<typename TSrc, typename TDst>
	bool try_read(
		TDst*& dst_value,
		PointerTag);

	template<typename TSrc, typename TDst>
	bool try_read(
		TDst& dst_value,
		ClassTag);

	template<typename TSrc, typename TDst>
	bool try_read(
		TDst& dst_value,
		ClassTag,
		InternalTag);

	template<typename TSrc, typename TDst>
	bool try_read(
		TDst& dst_value,
		ClassTag,
		ExternalTag);

	template<typename TSrc, typename TDst, int TCount>
	bool try_read(
		TDst(&dst_values)[TCount],
		Array1dTag);

	template<typename TSrc, typename TDst, int TCount1, int TCount2>
	bool try_read(
		TDst(&dst_values)[TCount1][TCount2],
		Array2dTag);


	template<typename TSrc, typename TDst>
	bool try_read(
		TDst* dst_values,
		int dst_count,
		InplaceTag);

	template<typename TSrc, typename TDst>
	bool try_read(
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

	template<typename TDst, typename TSrc>
	void write(
		const TSrc& src_value,
		ClassTag,
		InternalTag);

	template<typename TDst, typename TSrc>
	void write(
		const TSrc& src_value,
		ClassTag,
		ExternalTag);

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
}; // SavedGameHelper


template<typename T>
class SavedGameClassArchiver
{
public:
	enum { is_implemented = false };

	static void sg_export(
		SavedGameHelper& saved_game,
		const T& instance)
	{
		static_cast<void>(saved_game);
		static_cast<void>(instance);

		throw "Not implemented.";
	}

	static void sg_import(
		SavedGameHelper& saved_game,
		T& instance)
	{
		static_cast<void>(saved_game);
		static_cast<void>(instance);

		throw "Not implemented.";
	}
};


} // ojk


#endif // OJK_SAVED_GAME_HELPER_FWD_INCLUDED

