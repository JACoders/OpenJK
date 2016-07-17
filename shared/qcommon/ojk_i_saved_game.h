//
// Saved game interface.
//


#ifndef OJK_I_SAVED_GAME_INCLUDED
#define OJK_I_SAVED_GAME_INCLUDED


#include <cstdint>
#include <type_traits>
#include "ojk_i_saved_game_fwd.h"


namespace ojk {


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// Class stuff

inline ISavedGame::ISavedGame()
{
}

inline ISavedGame::~ISavedGame()
{
}

// Class stuff
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// read_chunk

template<typename TSrc, typename TDst>
bool ISavedGame::read_chunk(
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
bool ISavedGame::read_chunk(
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
bool ISavedGame::write_chunk(
    const ChunkId chunk_id,
    const TSrc& src_value)
{
    reset_buffer();

    write<TDst>(
        src_value);

    return write_chunk(
        chunk_id);
}

template<typename TDst, typename TSrc>
bool ISavedGame::write_chunk(
    const ChunkId chunk_id,
    const TSrc* src_values,
    int src_count)
{
    reset_buffer();

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
void ISavedGame::read(
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
                        typename std::conditional<
                            std::rank<TDst>::value == 2,
                            Array2dTag,
                            void
                        >::type
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
void ISavedGame::read(
    TDst& dst_value,
    BooleanTag)
{
    constexpr auto src_size = static_cast<int>(sizeof(TSrc));

    TSrc src_value;

    raw_read(
        &src_value,
        static_cast<int>(sizeof(TSrc)));

    // FIXME Byte order
    //

    dst_value = (src_value != 0);
}

template<typename TSrc, typename TDst>
void ISavedGame::read(
    TDst& dst_value,
    NumericTag)
{
    constexpr auto src_size = static_cast<int>(sizeof(TSrc));

    TSrc src_value;

    raw_read(
        &src_value,
        src_size);

    // FIXME Byte order
    //

    dst_value = static_cast<TDst>(src_value);
}

template<typename TSrc, typename TDst>
void ISavedGame::read(
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
void ISavedGame::read(
    TDst& dst_value,
    ClassTag)
{
    static_assert(
        std::is_same<TSrc, void>::value,
        "Unsupported types.");

    dst_value.sg_import(
        this);
}

template<typename TSrc, typename TDst, int TCount>
void ISavedGame::read(
    TDst (&dst_values)[TCount],
    Array1dTag)
{
    read<TSrc>(
        &dst_values[0],
        TCount);
}

template<typename TSrc, typename TDst, int TCount1, int TCount2>
void ISavedGame::read(
    TDst(&dst_values)[TCount1][TCount2],
    Array2dTag)
{
    read<TSrc>(
        &dst_values[0][0],
        TCount1 * TCount2);
}

// read
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// read (C-array)

template<typename TSrc, typename TDst>
void ISavedGame::read(
    TDst* dst_values,
    int dst_count)
{
    static_assert(
        (std::is_arithmetic<TDst>::value &&
            !std::is_same<TDst, bool>::value &&
            !std::is_enum<TDst>::value) ||
            std::is_pointer<TDst>::value ||
            std::is_class<TDst>::value,
        "Unsupported types.");

    using Src = typename std::conditional<
        std::is_same<TSrc, void>::value,
        TDst,
        TSrc
    >::type;

    constexpr auto is_src_pure_numeric =
        std::is_arithmetic<Src>::value &&
            (!std::is_same<Src, bool>::value) &&
            (!std::is_enum<Src>::value);

    constexpr auto is_dst_pure_numeric =
        std::is_arithmetic<TDst>::value &&
            (!std::is_same<TDst, bool>::value) &&
            (!std::is_enum<TDst>::value);

    constexpr auto is_src_float_point =
        std::is_floating_point<Src>::value;

    constexpr auto is_dst_float_point =
        std::is_floating_point<TDst>::value;

    constexpr auto has_same_size =
        (sizeof(Src) == sizeof(TDst));

    constexpr auto use_inplace =
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

    read<TSrc>(
        dst_values,
        dst_count,
        Tag());
}

template<typename TSrc, typename TDst>
void ISavedGame::read(
    TDst* dst_values,
    int dst_count,
    InplaceTag)
{
    const auto dst_size = dst_count * static_cast<int>(sizeof(TDst));

    raw_read(
        dst_values,
        dst_size);

    // FIXME Byte order
    //
}

template<typename TSrc, typename TDst>
void ISavedGame::read(
    TDst* dst_values,
    int dst_count,
    CastTag)
{
    using Tag = typename std::conditional<
        std::is_arithmetic<TDst>::value,
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
void ISavedGame::write(
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
void ISavedGame::write(
    const TSrc& src_value,
    NumericTag)
{
    constexpr auto dst_size = static_cast<int>(sizeof(TDst));

    auto dst_value = static_cast<TDst>(src_value);

    // FIXME Byte order
    //

    raw_write(
        &dst_value,
        dst_size);
}

template<typename TDst, typename TSrc>
void ISavedGame::write(
    const TSrc* src_value,
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
void ISavedGame::write(
    const TSrc& src_value,
    ClassTag)
{
    static_assert(
        std::is_same<TDst, void>::value,
        "Unsupported types.");

    src_value.sg_export(
        this);
}

template<typename TDst, typename TSrc, int TCount>
void ISavedGame::write(
    const TSrc (&src_values)[TCount],
    Array1dTag)
{
    write<TDst>(
        &src_values[0],
        TCount);
}

template<typename TDst, typename TSrc, int TCount1, int TCount2>
void ISavedGame::write(
    const TSrc (&src_values)[TCount1][TCount2],
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
void ISavedGame::write(
    const TSrc* src_values,
    int src_count)
{
    static_assert(
        (std::is_arithmetic<TSrc>::value &&
            !std::is_same<TSrc, bool>::value &&
            !std::is_enum<TSrc>::value) ||
            std::is_pointer<TSrc>::value ||
            std::is_class<TSrc>::value,
        "Unsupported types.");

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

    constexpr auto is_src_float_point =
        std::is_floating_point<TSrc>::value;

    constexpr auto is_dst_float_point =
        std::is_floating_point<Dst>::value;

    constexpr auto has_same_size =
        (sizeof(TSrc) == sizeof(Dst));

    constexpr auto use_inplace =
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
void ISavedGame::write(
    const TSrc* src_values,
    int src_count,
    InplaceTag)
{
    const auto src_size = src_count * static_cast<int>(sizeof(TSrc));

    raw_write(
        src_values,
        src_size);

    // FIXME Byte order
    //
}

template<typename TDst, typename TSrc>
void ISavedGame::write(
    const TSrc* src_values,
    int src_count,
    CastTag)
{
    using Tag = typename std::conditional<
        std::is_arithmetic<TSrc>::value,
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


#endif // OJK_I_SAVED_GAME_INCLUDED
