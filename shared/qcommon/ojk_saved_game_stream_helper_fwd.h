//
// Saved game stream helper.
// (forward declaration)
//


#ifndef OJK_SAVED_GAME_STREAM_HELPER_FWD_INCLUDED
#define OJK_SAVED_GAME_STREAM_HELPER_FWD_INCLUDED


#include "ojk_i_saved_game_stream.h"


namespace ojk
{


class SavedGameStreamHelper
{
public:
	SavedGameStreamHelper(
		ISavedGameStream* saved_game_stream);


	// Reads a value or an array of values from the file via
	// the internal buffer.
	template<typename TSrc = void, typename TDst = void>
	void read_chunk(
		const uint32_t chunk_id,
		TDst& dst_value);

	// Reads an array of values with specified count from
	// the file via the internal buffer.
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


	// Writes a value or array of values into the internal buffer.
	template<typename TDst = void, typename TSrc = void>
	void write(
		const TSrc& src_value);

	// Writes an array of values with specificed count into the internal buffer.
	template<typename TDst = void, typename TSrc = void>
	void write(
		const TSrc* src_values,
		int src_count);


private:
	ISavedGameStream* saved_game_stream_;


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
}; // SavedGameStreamHelper


}


#endif // OJK_SAVED_GAME_STREAM_HELPER_FWD_INCLUDED

