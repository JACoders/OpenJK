//
// Saved game.
//


#ifndef OJK_SAVED_GAME_INCLUDED
#define OJK_SAVED_GAME_INCLUDED


#include <cstdint>
#include <algorithm>
#include <type_traits>
#include "ojk_saved_game_fwd.h"


namespace ojk {


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// I/O buffer manipulation.

template<typename T>
void SavedGame::check_io_buffer(
    int count)
{
    if (count <= 0) {
        throw SavedGameException(
            "Zero or negative count.");
    }

    const auto data_size = sizeof(T) * count;

    if ((io_buffer_offset_ + data_size) > io_buffer_.size()) {
        throw SavedGameException(
            "Not enough data.");
    }
}

template<typename T>
void SavedGame::accomodate_io_buffer(
    int count)
{
    if (count <= 0) {
        throw SavedGameException(
            "Zero or negative count.");
    }

    const auto data_size = sizeof(T) * count;

    const auto new_buffer_size = io_buffer_offset_ + data_size;

    io_buffer_.resize(
        new_buffer_size);
}

template<typename T>
T SavedGame::cast_io_buffer()
{
    return reinterpret_cast<T>(io_buffer_[io_buffer_offset_]);
}

template<typename T>
void SavedGame::advance_io_buffer(
    int count)
{
    if (count <= 0) {
        throw SavedGameException(
            "Zero or negative count.");
    }

    const auto data_size = sizeof(T) * count;
    io_buffer_offset_ += data_size;
}

// I/O buffer manipulation.
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// read_chunk

template<typename TSrc, typename TDst>
bool SavedGame::read_chunk(
    const ChunkId chunk_id,
    TDst& dst_value)
{
    auto result = read_chunk(
        chunk_id);

    read<TSrc>(
        dst_value);

    return result;
}

template<typename TSrc, typename TDst>
bool SavedGame::read_chunk(
    const ChunkId chunk_id,
    TDst* dst_values,
    int dst_count)
{
    auto result = read_chunk(
        chunk_id);

    read<TSrc>(
        dst_values,
        dst_count);

    return result;
}

// read_chunk
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// write_chunk

template<typename TDst, typename TSrc>
bool SavedGame::write_chunk(
    const ChunkId chunk_id,
    const TSrc& src_value)
{
    io_buffer_offset_ = 0;

    write<TDst>(
        src_value);

    return write_chunk(
        chunk_id);
}

template<typename TDst, typename TSrc>
bool SavedGame::write_chunk(
    const ChunkId chunk_id,
    const TSrc* src_values,
    int src_count)
{
    io_buffer_offset_ = 0;

    write<TDst>(
        src_values,
        src_count);

    return write_chunk(
        chunk_id);
}

// write_chunk
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// read

template<typename TSrc, typename TDst>
void SavedGame::read(
    TDst& dst_value)
{
    using Tag = typename std::conditional<
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
                        void
                    >::type
                >::type
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported type.");

    read<TSrc>(
        dst_value,
        Tag());
}

template<typename TSrc, typename TDst>
void SavedGame::read(
    TDst& dst_value,
    BooleanTag)
{
    constexpr auto src_size = sizeof(TSrc);

    check_io_buffer<TSrc>();

    dst_value = (cast_io_buffer<const TSrc&>() != 0);

    // FIXME Byte order
    //

    advance_io_buffer<TSrc>();
}

template<typename TSrc, typename TDst>
void SavedGame::read(
    TDst& dst_value,
    NumericTag)
{
    check_io_buffer<TSrc>();

    dst_value = static_cast<TDst>(cast_io_buffer<const TSrc&>());

    // FIXME Byte order
    //

    advance_io_buffer<TSrc>();
}

template<typename TSrc, typename TDst>
void SavedGame::read(
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

    auto dst_number = DstNumeric();

    read<TSrc>(
        dst_number,
        NumericTag());

    dst_value = reinterpret_cast<TDst*>(dst_number);
}

template<typename TSrc, typename TDst>
void SavedGame::read(
    TDst& dst_value,
    ClassTag)
{
    throw SavedGameException(
        "Not implemented.");
}

template<typename TSrc, typename TDst, int TCount>
void SavedGame::read(
    TDst (&dst_values)[TCount],
    Array1dTag)
{
    read<TSrc>(
        &dst_values[0],
        TCount);
}

// read
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// read (C-array)

template<typename TSrc, typename TDst>
void SavedGame::read(
    TDst* dst_values,
    int dst_count)
{
    static_assert(
        (std::is_arithmetic<TDst>::value &&
            !std::is_same<TDst, bool>::value &&
            !std::is_enum<TDst>::value) ||
            std::is_class<TDst>::value,
        "Unsupported types.");

    if (!dst_values) {
        throw SavedGameException(
            "Null pointer.");
    }

    if (dst_count < 0) {
        throw SavedGameException(
            "Negative count.");
    }

    if (dst_count == 0) {
        return;
    }

    using Src = typename std::conditional<
        std::is_same<TSrc, void>::value,
        TDst,
        TSrc>::type;

    constexpr auto is_src_pure_numeric =
        std::is_arithmetic<Src>::value &&
            (!std::is_same<Src, bool>::value) &&
            (!std::is_enum<Src>::value);

    constexpr auto is_dst_pure_numeric =
        std::is_arithmetic<TDst>::value &&
            (!std::is_same<TDst, bool>::value) &&
            (!std::is_enum<TDst>::value);

    constexpr auto has_same_size =
        (sizeof(Src) == sizeof(TDst));

    constexpr auto use_inplace =
        is_src_pure_numeric &&
        is_dst_pure_numeric &&
        has_same_size;

    using Tag = typename std::conditional<
        use_inplace,
        InplaceTag,
        CastTag
    >::type;

    read<TSrc>(
        dst_values,
        dst_count,
        Tag());
}

template<typename TSrc, typename TDst>
void SavedGame::read(
    TDst* dst_values,
    int dst_count,
    InplaceTag)
{
    check_io_buffer<TDst>(
        dst_count);

    std::uninitialized_copy_n(
        &cast_io_buffer<const TDst&>(),
        dst_count,
        dst_values);

    // FIXME Byte order
    //

    advance_io_buffer<TDst>(
        dst_count);
}

template<typename TSrc, typename TDst>
void SavedGame::read(
    TDst* dst_values,
    int dst_count,
    CastTag)
{
    using Tag = typename std::conditional<
        std::is_arithmetic<TDst>::value,
        NumericTag,
        typename std::conditional<
            std::is_class<TDst>::value,
            ClassTag,
            void
        >::type
    >::type;

    for (int i = 0; i < dst_count; ++i) {
        read<TSrc>(
            dst_values[i],
            Tag());
    }
}

// read (C-array)
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// write

template<typename TDst, typename TSrc>
void SavedGame::write(
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
                    void
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
void SavedGame::write(
    const TSrc& src_value,
    NumericTag)
{
    accomodate_io_buffer<TSrc>();

    cast_io_buffer<TDst&>() = static_cast<TDst>(src_value);

    // FIXME Byte order
    //

    advance_io_buffer<TSrc>();
}

template<typename TDst, typename TSrc>
void SavedGame::write(
    const TSrc*& src_value,
    PointerTag)
{
    using DstNumeric = typename std::conditional<
        std::is_signed<TSrc>::value,
        std::intptr_t,
        std::uintptr_t
    >::type;

    auto dst_number = reinterpret_cast<DstNumeric>(src_value);

    write<TDst>(
        dst_number,
        NumericTag());
}

template<typename TDst, typename TSrc>
void SavedGame::write(
    const TSrc& src_value,
    ClassTag)
{
    throw SavedGameException(
        "Not implemented.");
}

template<typename TDst, typename TSrc, int TCount>
void SavedGame::write(
    const TSrc (&src_values)[TCount],
    Array1dTag)
{
    write<TDst>(
        &src_values[0],
        TCount);
}

// write
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// write (C-array)

template<typename TDst, typename TSrc>
void SavedGame::write(
    const TSrc* src_values,
    int src_count)
{
    static_assert(
        (std::is_arithmetic<TSrc>::value &&
            !std::is_same<TSrc, bool>::value &&
            !std::is_enum<TSrc>::value) ||
            std::is_class<TSrc>::value,
        "Unsupported types.");

    if (!src_values) {
        throw SavedGameException(
            "Null pointer.");
    }

    if (src_count < 0) {
        throw SavedGameException(
            "Negative count.");
    }

    if (src_count == 0) {
        return;
    }

    using Dst = typename std::conditional<
        std::is_same<TDst, void>::value,
        TSrc,
        TDst>::type;

    constexpr auto is_src_pure_numeric =
        std::is_arithmetic<TSrc>::value &&
            (!std::is_same<TSrc, bool>::value) &&
            (!std::is_enum<TSrc>::value);

    constexpr auto is_dst_pure_numeric =
        std::is_arithmetic<Dst>::value &&
            (!std::is_same<Dst, bool>::value) &&
            (!std::is_enum<Dst>::value);

    constexpr auto has_same_size =
        (sizeof(TSrc) == sizeof(Dst));

    constexpr auto use_inplace =
        is_src_pure_numeric &&
        is_dst_pure_numeric &&
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
void SavedGame::write(
    const TSrc* src_values,
    int src_count,
    InplaceTag)
{
    accomodate_io_buffer<TSrc>(
        src_count);

    std::uninitialized_copy_n(
        src_values,
        src_count,
        &cast_io_buffer<TSrc&>());

    // FIXME Byte order
    //

    advance_io_buffer<TSrc>(
        src_count);
}

template<typename TDst, typename TSrc>
void SavedGame::write(
    const TSrc* src_values,
    int src_count,
    CastTag)
{
    using Tag = typename std::conditional<
        std::is_arithmetic<TSrc>::value,
        NumericTag,
        typename std::conditional<
            std::is_class<TSrc>::value,
            ClassTag,
            void
        >::type
    >::type;

    for (int i = 0; i < src_count; ++i) {
        write<TDst>(
            src_values[i],
            Tag());
    }
}

// write (C-array)
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


} // ojk


#endif // OJK_SAVED_GAME_INCLUDED
