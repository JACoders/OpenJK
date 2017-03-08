//
// Saved game helper.
//


#ifndef OJK_SAVED_GAME_HELPER_INCLUDED
#define OJK_SAVED_GAME_HELPER_INCLUDED


#include <cstdint>
#include <type_traits>
#include "ojk_saved_game_helper_fwd.h"
#include "ojk_scope_guard.h"
#include "ojk_saved_game_class_archivers.h"


namespace ojk
{


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// Class stuff

inline SavedGameHelper::SavedGameHelper(
	ISavedGame* saved_game) :
		saved_game_(saved_game)
{
}

// Class stuff
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// Public methods

inline void SavedGameHelper::read_chunk(
	const uint32_t chunk_id)
{
	if (!saved_game_->read_chunk(
		chunk_id))
	{
		saved_game_->throw_error();
	}
}

inline void SavedGameHelper::write_chunk(
	const uint32_t chunk_id)
{
	if (!saved_game_->write_chunk(
		chunk_id))
	{
		saved_game_->throw_error();
	}
}

inline void SavedGameHelper::skip(
	int count)
{
	if (!saved_game_->skip(
		count))
	{
		saved_game_->throw_error();
	}
}

inline const void* SavedGameHelper::get_buffer_data()
{
	return saved_game_->get_buffer_data();
}

inline int SavedGameHelper::get_buffer_size() const
{
	return saved_game_->get_buffer_size();
}

inline void SavedGameHelper::reset_buffer()
{
	saved_game_->reset_buffer();
}

inline void SavedGameHelper::reset_buffer_offset()
{
	saved_game_->reset_buffer_offset();
}

inline void SavedGameHelper::ensure_all_data_read()
{
	saved_game_->ensure_all_data_read();
}

inline bool SavedGameHelper::is_failed() const
{
	return saved_game_->is_failed();
}

// Public methods
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// try_read_chunk

inline bool SavedGameHelper::try_read_chunk(
	const uint32_t chunk_id)
{
	return saved_game_->read_chunk(
		chunk_id);
}

template<typename TSrc, typename TDst>
bool SavedGameHelper::try_read_chunk(
	const uint32_t chunk_id,
	TDst& dst_value)
{
	if (!saved_game_->read_chunk(
		chunk_id))
	{
		return false;
	}

	if (!try_read<TSrc>(
		dst_value))
	{
		return false;
	}

	return saved_game_->is_all_data_read();
}

template<typename TSrc, typename TDst>
bool SavedGameHelper::try_read_chunk(
	const uint32_t chunk_id,
	TDst* dst_values,
	int dst_count)
{
	if (!saved_game_->read_chunk(
		chunk_id))
	{
		return false;
	}

	if (!try_read<TSrc>(
		dst_values,
		dst_count))
	{
		return false;
	}

	return saved_game_->is_all_data_read();
}

// try_read_chunk
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// read_chunk

template<typename TSrc, typename TDst>
void SavedGameHelper::read_chunk(
	const uint32_t chunk_id,
	TDst& dst_value)
{
	if (!try_read_chunk<TSrc>(
		chunk_id,
		dst_value))
	{
		saved_game_->throw_error();
	}
}

template<typename TSrc, typename TDst>
void SavedGameHelper::read_chunk(
	const uint32_t chunk_id,
	TDst* dst_values,
	int dst_count)
{
	if (!try_read_chunk<TSrc>(
		chunk_id,
		dst_values,
		dst_count))
	{
		saved_game_->throw_error();
	}
}

// read_chunk
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// write_chunk

template<typename TSize>
void SavedGameHelper::write_chunk_and_size(
	const uint32_t size_chunk_id,
	const uint32_t data_chunk_id)
{
	saved_game_->save_buffer();

	const int data_size = saved_game_->get_buffer_size();

	saved_game_->reset_buffer();

	write_chunk<TSize>(
		size_chunk_id,
		data_size);

	saved_game_->load_buffer();

	saved_game_->write_chunk(
		data_chunk_id);
}

template<typename TDst, typename TSrc>
void SavedGameHelper::write_chunk(
	const uint32_t chunk_id,
	const TSrc& src_value)
{
	saved_game_->reset_buffer();

	write<TDst>(
		src_value);

	saved_game_->write_chunk(
		chunk_id);
}

template<typename TDst, typename TSrc>
void SavedGameHelper::write_chunk(
	const uint32_t chunk_id,
	const TSrc* src_values,
	int src_count)
{
	saved_game_->reset_buffer();

	write<TDst>(
		src_values,
		src_count);

	saved_game_->write_chunk(
		chunk_id);
}

// write_chunk
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// try_read

template<typename TSrc, typename TDst>
bool SavedGameHelper::try_read(
	TDst& dst_value)
{
	using Tag = typename std::conditional <
		std::is_same<TDst, bool>::value,
		BooleanTag,
		typename std::conditional<
			std::is_arithmetic<TDst>::value || std::is_enum<TDst>::value,
			NumericTag,
			typename std::conditional<
				std::is_pointer<TDst>::value,
				PointerTag,
				typename std::conditional<
					std::is_class<TDst>::value,
					ClassTag,
					typename std::conditional<
						std::rank<TDst>::value == 1,
						Array1dTag,
						typename std::conditional<
							std::rank<TDst>::value == 2,
							Array2dTag,
							void
						>::type
					>::type
				>::type
			>::type
		>::type
	> ::type;

	static_assert(
		!std::is_same<Tag, void>::value,
		"Unsupported type.");

	return try_read<TSrc>(
		dst_value,
		Tag());
}

// try_read
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// read

template<typename TSrc, typename TDst>
void SavedGameHelper::read(
	TDst& dst_value)
{
	if (!try_read<TSrc>(
		dst_value))
	{
		saved_game_->throw_error();
	}
}

template<typename TSrc, typename TDst>
bool SavedGameHelper::try_read(
	TDst& dst_value,
	BooleanTag)
{
	TSrc src_value;

	if (!saved_game_->read(
		&src_value,
		static_cast<int>(sizeof(TSrc))))
	{
		return false;
	}

	// FIXME Byte order
	//

	dst_value = (src_value != 0);

	return true;
}

template<typename TSrc, typename TDst>
bool SavedGameHelper::try_read(
	TDst& dst_value,
	NumericTag)
{
	const int src_size = static_cast<int>(sizeof(TSrc));

	TSrc src_value;

	if (!saved_game_->read(
		&src_value,
		src_size))
	{
		return false;
	}

	// FIXME Byte order
	//

	dst_value = static_cast<TDst>(src_value);

	return true;
}

template<typename TSrc, typename TDst>
bool SavedGameHelper::try_read(
	TDst*& dst_value,
	PointerTag)
{
	static_assert(
		std::is_arithmetic<TSrc>::value &&
			!std::is_same<TSrc, bool>::value,
		"Unsupported types.");

	using DstNumeric = typename std::conditional<
		std::is_signed<TSrc>::value,
		std::intptr_t,
		std::uintptr_t
	>::type;

	DstNumeric dst_number;

	if (!try_read<TSrc>(
		dst_number,
		NumericTag()))
	{
		return false;
	}

	dst_value = reinterpret_cast<TDst*>(dst_number);

	return true;
}

template<typename TSrc, typename TDst>
bool SavedGameHelper::try_read(
	TDst& dst_value,
	ClassTag)
{
	static_assert(
		std::is_same<TSrc, void>::value,
		"Unsupported types.");

	using Tag = typename std::conditional<
		SavedGameClassArchiver<TDst>::is_implemented,
		ExternalTag,
		InternalTag
	>::type;

	return try_read<TSrc>(
		dst_value,
		ClassTag(),
		Tag());
}

template<typename TSrc, typename TDst>
bool SavedGameHelper::try_read(
	TDst& dst_value,
	ClassTag,
	InternalTag)
{
	dst_value.sg_import(
		*this);

	return !saved_game_->is_failed();
}

template<typename TSrc, typename TDst>
bool SavedGameHelper::try_read(
	TDst& dst_value,
	ClassTag,
	ExternalTag)
{
	SavedGameClassArchiver<TDst>::sg_import(
		*this,
		dst_value);

	return !saved_game_->is_failed();
}

template<typename TSrc, typename TDst, int TCount>
bool SavedGameHelper::try_read(
	TDst(&dst_values)[TCount],
	Array1dTag)
{
	return try_read<TSrc>(
		&dst_values[0],
		TCount);
}

template<typename TSrc, typename TDst, int TCount1, int TCount2>
bool SavedGameHelper::try_read(
	TDst(&dst_values)[TCount1][TCount2],
	Array2dTag)
{
	return try_read<TSrc>(
		&dst_values[0][0],
		TCount1 * TCount2);
}

// read
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// try_read (C-array)

template<typename TSrc, typename TDst>
bool SavedGameHelper::try_read(
	TDst* dst_values,
	int dst_count)
{
	static_assert(
		std::is_arithmetic<TDst>::value ||
			std::is_enum<TDst>::value ||
			std::is_pointer<TDst>::value ||
			std::is_class<TDst>::value,
		"Unsupported types.");

	using Src = typename std::conditional<
		std::is_same<TSrc, void>::value,
		TDst,
		TSrc
	>::type;

	const bool is_src_pure_numeric =
		std::is_arithmetic<Src>::value &&
		(!std::is_same<Src, bool>::value) &&
		(!std::is_enum<Src>::value);

	const bool is_dst_pure_numeric =
		std::is_arithmetic<TDst>::value &&
		(!std::is_same<TDst, bool>::value) &&
		(!std::is_enum<TDst>::value);

	const bool is_src_float_point =
		std::is_floating_point<Src>::value;

	const bool is_dst_float_point =
		std::is_floating_point<TDst>::value;

	const bool has_same_size =
		(sizeof(Src) == sizeof(TDst));

	const bool use_inplace =
		is_src_pure_numeric &&
		is_dst_pure_numeric &&
		((!is_src_float_point && !is_dst_float_point) ||
			(is_src_float_point && is_dst_float_point)) &&
		has_same_size;

	using Tag = typename std::conditional<
		use_inplace,
		InplaceTag,
		CastTag
	>::type;

	return try_read<TSrc>(
		dst_values,
		dst_count,
		Tag());
}

// try_read (C-array)
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// read (C-array)

template<typename TSrc, typename TDst>
void SavedGameHelper::read(
	TDst* dst_values,
	int dst_count)
{
	if (!try_read<TSrc>(
		dst_values,
		dst_count))
	{
		saved_game_->throw_error();
	}
}

template<typename TSrc, typename TDst>
bool SavedGameHelper::try_read(
	TDst* dst_values,
	int dst_count,
	InplaceTag)
{
	const int dst_size = dst_count * static_cast<int>(sizeof(TDst));

	if (!saved_game_->read(
		dst_values,
		dst_size))
	{
		return false;
	}

	// FIXME Byte order
	//

	return true;
}

template<typename TSrc, typename TDst>
bool SavedGameHelper::try_read(
	TDst* dst_values,
	int dst_count,
	CastTag)
{
	using Tag = typename std::conditional<
		std::is_arithmetic<TDst>::value ||
			std::is_enum<TDst>::value,
		NumericTag,
		typename std::conditional<
			std::is_pointer<TDst>::value,
			PointerTag,
			typename std::conditional<
				std::is_class<TDst>::value,
				ClassTag,
				void
			>::type
		>::type
	>::type;

	for (int i = 0; i < dst_count; ++i)
	{
		if (!try_read<TSrc>(
			dst_values[i],
			Tag()))
		{
			return false;
		}
	}

	return true;
}

// read (C-array)
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// write

template<typename TDst, typename TSrc>
void SavedGameHelper::write(
	const TSrc& src_value)
{
	using Tag = typename std::conditional<
		std::is_arithmetic<TSrc>::value || std::is_enum<TSrc>::value,
		NumericTag,
		typename std::conditional<
			std::is_pointer<TSrc>::value,
			PointerTag,
			typename std::conditional<
				std::is_class<TSrc>::value,
				ClassTag,
				typename std::conditional<
					std::rank<TSrc>::value == 1,
					Array1dTag,
					typename std::conditional<
						std::rank<TSrc>::value == 2,
						Array2dTag,
						void
					>::type
				>::type
			>::type
		>::type
	>::type;

	static_assert(
		!std::is_same<Tag, void>::value,
		"Unsupported type.");

	write<TDst>(
		src_value,
		Tag());
}

template<typename TDst, typename TSrc>
void SavedGameHelper::write(
	const TSrc& src_value,
	NumericTag)
{
	const int dst_size = static_cast<int>(sizeof(TDst));

	const TDst dst_value = static_cast<TDst>(src_value);

	// FIXME Byte order
	//

	saved_game_->write(
		&dst_value,
		dst_size);
}

template<typename TDst, typename TSrc>
void SavedGameHelper::write(
	const TSrc* src_value,
	PointerTag)
{
	using DstNumeric = typename std::conditional<
		std::is_signed<TSrc>::value,
		std::intptr_t,
		std::uintptr_t
	>::type;

	const DstNumeric dst_number = reinterpret_cast<DstNumeric>(src_value);

	write<TDst>(
		dst_number,
		NumericTag());
}

template<typename TDst, typename TSrc>
void SavedGameHelper::write(
	const TSrc& src_value,
	ClassTag)
{
	static_assert(
		std::is_same<TDst, void>::value,
		"Unsupported types.");

	using Tag = typename std::conditional<
		SavedGameClassArchiver<TSrc>::is_implemented,
		ExternalTag,
		InternalTag
	>::type;

	write<TDst>(
		src_value,
		ClassTag(),
		Tag());
}

template<typename TDst, typename TSrc>
void SavedGameHelper::write(
	const TSrc& src_value,
	ClassTag,
	InternalTag)
{
	src_value.sg_export(
		*this);
}

template<typename TDst, typename TSrc>
void SavedGameHelper::write(
	const TSrc& src_value,
	ClassTag,
	ExternalTag)
{
	SavedGameClassArchiver<TSrc>::sg_export(
		*this,
		src_value);
}

template<typename TDst, typename TSrc, int TCount>
void SavedGameHelper::write(
	const TSrc(&src_values)[TCount],
	Array1dTag)
{
	write<TDst>(
		&src_values[0],
		TCount);
}

template<typename TDst, typename TSrc, int TCount1, int TCount2>
void SavedGameHelper::write(
	const TSrc(&src_values)[TCount1][TCount2],
	Array2dTag)
{
	write<TDst>(
		&src_values[0][0],
		TCount1 * TCount2);
}

// write
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// write (C-array)

template<typename TDst, typename TSrc>
void SavedGameHelper::write(
	const TSrc* src_values,
	int src_count)
{
	static_assert(
		std::is_arithmetic<TSrc>::value ||
			std::is_enum<TSrc>::value ||
			std::is_pointer<TSrc>::value ||
			std::is_class<TSrc>::value,
		"Unsupported types.");

	using Dst = typename std::conditional<
		std::is_same<TDst, void>::value,
		TSrc,
		TDst>::type;

	const bool is_src_pure_numeric =
		std::is_arithmetic<TSrc>::value &&
		(!std::is_same<TSrc, bool>::value) &&
		(!std::is_enum<TSrc>::value);

	const bool is_dst_pure_numeric =
		std::is_arithmetic<Dst>::value &&
		(!std::is_same<Dst, bool>::value) &&
		(!std::is_enum<Dst>::value);

	const bool is_src_float_point =
		std::is_floating_point<TSrc>::value;

	const bool is_dst_float_point =
		std::is_floating_point<Dst>::value;

	const bool has_same_size =
		(sizeof(TSrc) == sizeof(Dst));

	const bool use_inplace =
		is_src_pure_numeric &&
		is_dst_pure_numeric &&
		((!is_src_float_point && !is_dst_float_point) ||
		(is_src_float_point && is_dst_float_point)) &&
		has_same_size;

	using Tag = typename std::conditional<
		use_inplace,
		InplaceTag,
		CastTag
	>::type;

	write<TDst>(
		src_values,
		src_count,
		Tag());
}

template<typename TDst, typename TSrc>
void SavedGameHelper::write(
	const TSrc* src_values,
	int src_count,
	InplaceTag)
{
	const int src_size = src_count * static_cast<int>(sizeof(TSrc));

	saved_game_->write(
		src_values,
		src_size);

	// FIXME Byte order
	//
}

template<typename TDst, typename TSrc>
void SavedGameHelper::write(
	const TSrc* src_values,
	int src_count,
	CastTag)
{
	using Tag = typename std::conditional<
		std::is_arithmetic<TSrc>::value ||
			std::is_enum<TSrc>::value,
		NumericTag,
		typename std::conditional<
			std::is_pointer<TSrc>::value,
			PointerTag,
			typename std::conditional<
				std::is_class<TSrc>::value,
				ClassTag,
				void
			>::type
		>::type
	>::type;

	for (int i = 0; i < src_count; ++i)
	{
		write<TDst>(
			src_values[i],
			Tag());
	}
}

// write (C-array)
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


} // ojk


#endif // OJK_SAVED_GAME_HELPER_INCLUDED
