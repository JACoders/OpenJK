//
// Saved game wrappers.
//


#ifndef OJK_SG_WRAPPERS_INCLUDED
#define OJK_SG_WRAPPERS_INCLUDED


#ifdef __cplusplus


#include <algorithm>
#include <functional>
#include <type_traits>
#include "ojk_sg_wrappers_shared.h"


namespace detail {


template<typename T>
class SgTraits
{
private:
    using yes = std::true_type;
    using no = std::false_type;


    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    // ReadSaveData

    template<typename U>
    static auto test_read_save_data(int) -> decltype(
        std::declval<U>().ReadSaveData(
            static_cast<unsigned int>(0),
            static_cast<void*>(nullptr),
            static_cast<int>(0),
            static_cast<void**>(nullptr)
        ) == 0,

        yes()
    );

    template<typename>
    static no test_read_save_data(...);

    // ReadSaveData
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    // I_ReadSaveData

    template<typename U>
    static auto test_i_read_save_data(int) -> decltype(
        std::declval<U>().I_ReadSaveData(
            static_cast<unsigned int>(0),
            static_cast<void*>(nullptr),
            static_cast<int>(0),
            static_cast<void**>(nullptr)
        ) == 0,

        yes()
    );

    template<typename>
    static no test_i_read_save_data(...);

    // I_ReadSaveData
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    // ReadFromSaveGame

    template<typename U>
    static auto test_read_from_save_game(int) -> decltype(
        std::declval<U>().ReadFromSaveGame(
            static_cast<unsigned int>(0),
            static_cast<void*>(nullptr),
            static_cast<int>(0),
            static_cast<void**>(nullptr)
        ) == 0,

        yes()
    );

    template<typename>
    static no test_read_from_save_game(...);

    // ReadFromSaveGame
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    // WriteSaveData

    template<typename U>
    static auto test_write_save_data(int) -> decltype(
        std::declval<U>().WriteSaveData(
            static_cast<unsigned int>(0),
            static_cast<const void*>(nullptr),
            static_cast<int>(0)
        ) == 0,

        yes()
    );

    template<typename>
    static no test_write_save_data(...);

    // WriteSaveData
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    // I_WriteSaveData

    template<typename U>
    static auto test_i_write_save_data(int) -> decltype(
        std::declval<U>().I_WriteSaveData(
            static_cast<unsigned int>(0),
            static_cast<const void*>(nullptr),
            static_cast<int>(0)
        ) == 0,

        yes()
    );

    template<typename>
    static no test_i_write_save_data(...);

    // I_WriteSaveData
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    // AppendToSaveGame

    template<typename U>
    static auto test_append_to_save_game(int) -> decltype(
        std::declval<U>().AppendToSaveGame(
            static_cast<unsigned int>(0),
            static_cast<const void*>(nullptr),
            static_cast<int>(0)
        ) == 0,

        yes()
    );

    template<typename>
    static no test_append_to_save_game(...);

    // AppendToSaveGame
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    // SG_Append

    template<typename U>
    static auto test_sg_append(int) -> decltype(
        std::declval<U>().SG_Append(
            static_cast<unsigned int>(0),
            static_cast<const void*>(nullptr),
            static_cast<int>(0)
        ) == 0,

        yes()
    );

    template<typename>
    static no test_sg_append(...);

    // SG_Append
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    // has_sg_export

    template<typename U>
    static auto test_sg_export(int) -> decltype(
        std::declval<U>().sg_export(
            std::declval<typename U::SgType>()
        ),

        yes()
    );

    template<typename>
    static no test_sg_export(...);

    // has_sg_export
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    // has_sg_import

    template<typename U>
    static auto test_sg_import(int) -> decltype(
        std::declval<U>().sg_import(
            std::declval<typename U::SgType>()
        ),

        yes()
    );

    template<typename>
    static no test_sg_import(...);

    // has_sg_import
    // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


public:
    static constexpr bool is_numeric()
    {
        return std::is_arithmetic<T>::value || std::is_enum<T>::value;
    }

    static constexpr bool is_any_char()
    {
        return
            std::is_same<T, char>::value ||
            std::is_same<T, signed char>::value ||
            std::is_same<T, unsigned char>::value;
    }

    static constexpr bool is_array_1d()
    {
        return std::is_array<T>::value && std::rank<T>::value == 1;
    }

    static constexpr bool is_array_2d()
    {
        return std::is_array<T>::value && std::rank<T>::value == 2;
    }

    static constexpr bool has_read_save_data()
    {
        return std::is_same<decltype(test_read_save_data<T>(0)), yes>::value;
    }

    static constexpr bool has_i_read_save_data()
    {
        return std::is_same<decltype(test_i_read_save_data<T>(0)), yes>::value;
    }

    static constexpr bool has_read_from_save_game()
    {
        return std::is_same<decltype(test_read_from_save_game<T>(0)), yes>::value;
    }

    static constexpr bool has_write_save_data()
    {
        return std::is_same<decltype(test_write_save_data<T>(0)), yes>::value;
    }

    static constexpr bool has_i_write_save_data()
    {
        return std::is_same<decltype(test_i_write_save_data<T>(0)), yes>::value;
    }

    static constexpr bool has_append_to_save_game()
    {
        return std::is_same<decltype(test_append_to_save_game<T>(0)), yes>::value;
    }

    static constexpr bool has_sg_append()
    {
        return std::is_same<decltype(test_sg_append<T>(0)), yes>::value;
    }

    static constexpr bool has_sg_export()
    {
        return std::is_same<decltype(test_sg_export<T>(0)), yes>::value;
    }

    static constexpr bool has_sg_import()
    {
        return std::is_same<decltype(test_sg_import<T>(0)), yes>::value;
    }
}; // SgTraits


class SgVoidTag
{
public:
}; //SgVoidTag

class SgImportTag
{
public:
}; //SgImportTag

class SgExportTag
{
public:
}; //SgExportTag

class SgPointerTag
{
public:
}; // SgPointerTag

class SgArray1dTag
{
public:
}; // SgArray1dTag

class SgArray2dTag
{
public:
}; // SgArray2dTag

class SgClassTag
{
public:
}; // SgClassTag

class SgNumericTag
{
public:
}; // SgNumericTag

class SgBoolTag
{
public:
}; // SgBoolTag

class SgAnyCharTag
{
public:
}; // SgAnyCharTag

class SgFuncTag
{
public:
}; // SgFuncTag

class SgReadSaveDataFuncTag
{
public:
}; // SgReadSaveDataFuncTag

class SgIReadSaveDataFuncTag
{
public:
}; // SgIReadSaveDataFuncTag

class SgReadFromSaveGameFuncTag
{
public:
}; // SgReadFromSaveGameFuncTag

class SgSgAppendFuncTag
{
public:
}; // SgSgAppendFuncTag

class SgWriteSaveDataFuncTag
{
public:
}; // SgWriteSaveDataFuncTag

class SgIWriteSaveDataFuncTag
{
public:
}; // SgIWriteSaveDataFuncTag

class SgAppendToSaveGameFuncTag
{
public:
}; // SgAppendToSaveGameFuncTag


} // detail


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// sg_get_read_func
//
// Returns a read function wrapper for a function pointer,
// pointer to an object or a reference to an object.
//

namespace detail {


using SgReadFunc = std::function<int (
    unsigned int chunk_id,
    void* value_storage,
    int value_size,
    void** allocated_value)>;


template<typename TFunc>
inline SgReadFunc sg_get_read_func(
    TFunc& func,
    SgFuncTag)
{
    return func;
}

template<typename TInstance>
inline SgReadFunc sg_get_read_func(
    TInstance* instance,
    SgPointerTag,
    SgReadSaveDataFuncTag)
{
    return std::bind(
        &TInstance::ReadSaveData,
        instance,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3,
        std::placeholders::_4);
}

template<typename TInstance>
inline SgReadFunc sg_get_read_func(
    TInstance* instance,
    SgPointerTag,
    SgIReadSaveDataFuncTag)
{
    return instance->I_ReadSaveData;
}

template<typename TInstance>
inline SgReadFunc sg_get_read_func(
    TInstance* instance,
    SgPointerTag,
    SgReadFromSaveGameFuncTag)
{
    return instance->ReadFromSaveGame;
}

template<typename TInstance>
inline SgReadFunc sg_get_read_func(
    TInstance* instance,
    SgPointerTag)
{
    using Tag = typename std::conditional<
        SgTraits<TInstance>::has_read_save_data(),
        SgReadSaveDataFuncTag,
        typename std::conditional<
            SgTraits<TInstance>::has_i_read_save_data(),
            SgIReadSaveDataFuncTag,
            typename std::conditional<
                SgTraits<TInstance>::has_read_from_save_game(),
                SgReadFromSaveGameFuncTag,
                void
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported instance type.");

    return sg_get_read_func(
        instance,
        SgPointerTag(),
        Tag());
}

template<typename TInstance>
inline SgReadFunc sg_get_read_func(
    TInstance& instance,
    SgClassTag)
{
    return sg_get_read_func(
        &instance,
        SgPointerTag());
}

template<typename TInstance>
inline SgReadFunc sg_get_read_func(
    TInstance& instance)
{
    using Tag = typename std::conditional<
        std::is_function<TInstance>::value,
        SgFuncTag,
        typename std::conditional<
            std::is_class<TInstance>::value,
            SgClassTag,
            typename std::conditional<
                std::is_pointer<TInstance>::value,
                SgPointerTag,
                void
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported instance type.");

    return sg_get_read_func(
        instance,
        Tag());
}


} // detail

// sg_get_read_func
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// sg_get_write_func
//
// Returns a write function wrapper for a function pointer,
// pointer to an object or a reference to an object.
//

namespace detail {


using SgWriteFunc = std::function<int (
    unsigned int chunk_id,
    const void* value_storage,
    int value_size)>;


template<typename TFunc>
inline SgWriteFunc sg_get_write_func(
    TFunc& func,
    SgFuncTag)
{
    return func;
}

template<typename TInstance>
inline SgWriteFunc sg_get_write_func(
    TInstance* instance,
    SgPointerTag,
    SgSgAppendFuncTag)
{
    return instance->SG_Append;
}

template<typename TInstance>
inline SgWriteFunc sg_get_write_func(
    TInstance* instance,
    SgPointerTag,
    SgWriteSaveDataFuncTag)
{
    return std::bind(
        &TInstance::WriteSaveData,
        instance,
        std::placeholders::_1,
        std::placeholders::_2,
        std::placeholders::_3);
}

template<typename TInstance>
inline SgWriteFunc sg_get_write_func(
    TInstance* instance,
    SgPointerTag,
    SgIWriteSaveDataFuncTag)
{
    return instance->I_WriteSaveData;
}

template<typename TInstance>
inline SgWriteFunc sg_get_write_func(
    TInstance* instance,
    SgPointerTag,
    SgAppendToSaveGameFuncTag)
{
    return instance->AppendToSaveGame;
}

template<typename TInstance>
inline SgWriteFunc sg_get_write_func(
    TInstance* instance,
    SgPointerTag)
{
    using Tag = typename std::conditional<
        SgTraits<TInstance>::has_sg_append(),
        SgSgAppendFuncTag,
        typename std::conditional<
            SgTraits<TInstance>::has_write_save_data(),
            SgWriteSaveDataFuncTag,
            typename std::conditional<
                SgTraits<TInstance>::has_i_write_save_data(),
                SgIWriteSaveDataFuncTag,
                typename std::conditional<
                    SgTraits<TInstance>::has_append_to_save_game(),
                    SgAppendToSaveGameFuncTag,
                    void
                >::type
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported instance type.");

    return sg_get_write_func(
        instance,
        SgPointerTag(),
        Tag());
}

template<typename TInstance>
inline SgWriteFunc sg_get_write_func(
    TInstance& instance,
    SgClassTag)
{
    return sg_get_write_func(
        &instance,
        SgPointerTag());
}

template<typename TInstance>
inline SgWriteFunc sg_get_write_func(
    TInstance& instance)
{
    using Tag = typename std::conditional<
        std::is_function<TInstance>::value,
        SgFuncTag,
        typename std::conditional<
            std::is_class<TInstance>::value,
            SgClassTag,
            typename std::conditional<
                std::is_pointer<TInstance>::value,
                SgPointerTag,
                void
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported instance type.");

    return sg_get_write_func(
        instance,
        Tag());
}


} // detail

// sg_get_write_func
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// sg_export
//
// Exports a value or a fixed-array of values from a
// native type to a saved game one.
//

namespace detail {


template<typename TSrc, typename TDst>
inline void sg_export(
    const TSrc& src,
    TDst& dst,
    SgNumericTag)
{
    dst = static_cast<TDst>(src);
}

template<typename TSrc, typename TDst>
inline void sg_export(
    const TSrc& src,
    TDst& dst,
    SgPointerTag)
{
    dst = static_cast<TDst>(reinterpret_cast<std::intptr_t>(src));
}

template<typename TSrc, typename TDst>
inline void sg_export(
    const TSrc& src,
    TDst& dst,
    SgClassTag)
{
    src.sg_export(dst);
}

template<typename TSrc, typename TDst, std::size_t TCount>
inline void sg_export(
    const TSrc (&src)[TCount],
    SgArray<TDst, TCount>& dst,
    SgArray1dTag,
    SgNumericTag)
{
    std::uninitialized_copy_n(
        src,
        TCount,
        dst.begin());
}

template<typename TSrc, typename TDst, std::size_t TCount>
inline void sg_export(
    const TSrc (&src)[TCount],
    SgArray<TDst, TCount>& dst,
    SgArray1dTag,
    SgPointerTag)
{
    for (decltype(TCount) i = 0; i < TCount; ++i) {
        dst[i] = static_cast<TDst>(reinterpret_cast<std::intptr_t>(src[i]));
    }
}

template<typename TSrc, typename TDst, std::size_t TCount>
inline void sg_export(
    const TSrc (&src)[TCount],
    SgArray<TDst, TCount>& dst,
    SgArray1dTag,
    SgClassTag)
{
    for (decltype(TCount) i = 0; i < TCount; ++i) {
        sg_export(
            src[i],
            dst[i],
            SgClassTag());
    }
}

template<typename TSrc, typename TDst, std::size_t TCount>
inline void sg_export(
    const TSrc (&src)[TCount],
    SgArray<TDst, TCount>& dst,
    SgArray1dTag)
{
    using Tag = typename std::conditional<
        SgTraits<TSrc>::is_numeric() && SgTraits<TDst>::is_numeric(),
        SgNumericTag,
        typename std::conditional<
            std::is_pointer<TSrc>::value && SgTraits<TDst>::is_numeric(),
            SgPointerTag,
            typename std::conditional<
                SgTraits<TSrc>::has_sg_export() && std::is_class<TDst>::value,
                SgClassTag,
                void
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");

    sg_export(
        src,
        dst,
        SgArray1dTag(),
        Tag());
}

template<typename TSrc, typename TDst, std::size_t TCount1, std::size_t TCount2>
inline void sg_export(
    const TSrc (&src)[TCount1][TCount2],
    SgArray2d<TDst, TCount1, TCount2>& dst,
    SgArray2dTag)
{
    static_assert(
        detail::SgTraits<TSrc>::is_numeric() &&
            detail::SgTraits<TDst>::is_numeric(),
        "Unsupported types.");

    for (decltype(TCount1) i = 0; i < TCount1; ++i) {
        std::uninitialized_copy_n(
            src[i],
            TCount2,
            dst[i].begin());
    }
}


} // detail


template<typename TSrc, typename TDst>
inline void sg_export(
    const TSrc& src,
    TDst& dst)
{
    using Tag = typename std::conditional<
        detail::SgTraits<TSrc>::is_numeric() &&
            detail::SgTraits<TDst>::is_numeric(),
        detail::SgNumericTag,
        typename std::conditional<
            std::is_pointer<TSrc>::value &&
                detail::SgTraits<TDst>::is_numeric(),
            detail::SgPointerTag,
            typename std::conditional<
                detail::SgTraits<TSrc>::has_sg_export() &&
                    std::is_class<TDst>::value,
                detail::SgClassTag,
                typename std::conditional<
                    detail::SgTraits<TSrc>::is_array_1d(),
                    detail::SgArray1dTag,
                    typename std::conditional<
                        detail::SgTraits<TSrc>::is_array_2d(),
                        detail::SgArray2dTag,
                        void
                    >::type
                >::type
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");

    detail::sg_export(
        src,
        dst,
        Tag());
}

// sg_export
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// sg_import
//
// Imports a value or a fixed-szie array of values from a
// saved game type to a native one.
//

namespace detail {


template<typename TSrc, typename TDst>
inline void sg_import(
    const TSrc& src,
    TDst& dst,
    SgNumericTag,
    SgNumericTag)
{
    dst = static_cast<TDst>(src);
}

template<typename TSrc, typename TDst>
inline void sg_import(
    const TSrc& src,
    TDst& dst,
    SgNumericTag,
    SgBoolTag)
{
    dst = (src != 0);
}

template<typename TSrc, typename TDst>
inline void sg_import(
    const TSrc& src,
    TDst& dst,
    SgNumericTag)
{
    using Tag = typename std::conditional<
        std::is_same<TDst, bool>::value,
        SgBoolTag,
        SgNumericTag
    >::type;

    sg_import(
        src,
        dst,
        SgNumericTag(),
        Tag());
}

template<typename TSrc, typename TDst>
inline void sg_import(
    const TSrc& src,
    TDst& dst,
    SgPointerTag)
{
    dst = reinterpret_cast<TDst>(static_cast<std::intptr_t>(src));
}

template<typename TSrc, typename TDst>
inline void sg_import(
    const TSrc& src,
    TDst& dst,
    SgClassTag)
{
    dst.sg_import(src);
}

template<typename TSrc, typename TDst, std::size_t TCount>
inline void sg_import(
    const SgArray<TSrc, TCount>& src,
    TDst (&dst)[TCount],
    SgArray1dTag,
    SgNumericTag)
{
    std::uninitialized_copy(
        src.cbegin(),
        src.cend(),
        dst);
}

template<typename TSrc, typename TDst, std::size_t TCount>
inline void sg_import(
    const SgArray<TSrc, TCount>& src,
    TDst (&dst)[TCount],
    SgArray1dTag,
    SgPointerTag)
{
    for (decltype(TCount) i = 0; i < TCount; ++i) {
        dst[i] = reinterpret_cast<TDst>(static_cast<std::intptr_t>(src[i]));
    }
}

template<typename TSrc, typename TDst, std::size_t TCount>
inline void sg_import(
    const SgArray<TSrc, TCount>& src,
    TDst (&dst)[TCount],
    SgArray1dTag,
    SgClassTag)
{
    for (decltype(TCount) i = 0; i < TCount; ++i) {
        sg_import(
            src[i],
            dst[i],
            SgClassTag());
    }
}

template<typename TSrc, typename TDst, std::size_t TCount>
inline void sg_import(
    const SgArray<TSrc, TCount>& src,
    TDst (&dst)[TCount],
    SgArray1dTag)
{
    using Tag = typename std::conditional<
        SgTraits<TSrc>::is_numeric() && SgTraits<TDst>::is_numeric(),
        SgNumericTag,
        typename std::conditional<
            SgTraits<TSrc>::is_numeric() && std::is_pointer<TDst>::value,
            SgPointerTag,
            typename std::conditional<
                std::is_class<TSrc>::value && SgTraits<TDst>::has_sg_import(),
                SgClassTag,
                void
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");

    sg_import(
        src,
        dst,
        SgArray1dTag(),
        Tag());
}

template<typename TSrc, typename TDst, std::size_t TCount1, std::size_t TCount2>
inline void sg_import(
    const SgArray2d<TSrc, TCount1, TCount2>& src,
    TDst (&dst)[TCount1][TCount2],
    SgArray2dTag)
{
    static_assert(
        SgTraits<TSrc>::is_numeric() && SgTraits<TDst>::is_numeric(),
        "Unsupported types.");

    for (decltype(TCount1) i = 0; i < TCount1; ++i) {
        std::uninitialized_copy(
            src[i].cbegin(),
            src[i].cend(),
            dst[i]);
    }
}


} // detail


template<typename TSrc, typename TDst>
inline void sg_import(
    const TSrc& src,
    TDst& dst)
{
    using Tag = typename std::conditional<
        detail::SgTraits<TDst>::is_numeric(),
        detail::SgNumericTag,
        typename std::conditional<
            std::is_pointer<TDst>::value,
            detail::SgPointerTag,
            typename std::conditional<
                std::is_class<TSrc>::value &&
                    detail::SgTraits<TDst>::has_sg_import(),
                detail::SgClassTag,
                typename std::conditional<
                    detail::SgTraits<TDst>::is_array_1d(),
                    detail::SgArray1dTag,
                    typename std::conditional<
                        detail::SgTraits<TDst>::is_array_2d(),
                        detail::SgArray2dTag,
                        void
                    >::type
                >::type
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");

    detail::sg_import(
        src,
        dst,
        Tag());
}

// sg_import
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// sg_read_no_cast (fixed size)
//
// Reads a value or a fixed-size array of values from a
// saved game without conversion.
//

namespace detail {


template<typename TDst>
inline int sg_read_no_cast(
    SgReadFunc& read_func,
    unsigned int chunk_id,
    TDst& dst_value,
    SgNumericTag)
{
    constexpr auto dst_size = static_cast<int>(sizeof(TDst));

    return read_func(
        chunk_id,
        &dst_value,
        dst_size,
        nullptr);
}

template<typename TDst>
inline int sg_read_no_cast(
    SgReadFunc& read_func,
    unsigned int chunk_id,
    TDst& dst_value,
    SgClassTag)
{
    using SgType = typename TDst::SgType;

    constexpr auto src_size = static_cast<int>(sizeof(SgType));

    auto& buffer = sg_get_buffer(
        src_size);

    auto read_result = read_func(
        chunk_id,
        buffer.data(),
        src_size,
        nullptr);

    auto& src_value = *reinterpret_cast<const SgType*>(buffer.data());

    ::sg_import(src_value, dst_value);

    return read_result;
}

template<typename TDst, std::size_t TCount>
inline int sg_read_no_cast(
    SgReadFunc& read_func,
    unsigned int chunk_id,
    TDst (&dst_values)[TCount],
    SgArray1dTag,
    SgNumericTag)
{
    constexpr auto dst_size = static_cast<int>(TCount * sizeof(TDst));

    return read_func(
        chunk_id,
        &dst_values[0],
        dst_size,
        nullptr);
}

template<typename TDst, std::size_t TCount>
inline int sg_read_no_cast(
    SgReadFunc& read_func,
    unsigned int chunk_id,
    TDst (&dst_values)[TCount],
    SgArray1dTag,
    SgClassTag)
{
    using SgType = typename TDst::SgType;

    constexpr auto src_size = static_cast<int>(TCount * sizeof(SgType));

    auto& buffer = sg_get_buffer(
        src_size);

    auto read_result = read_func(
        chunk_id,
        buffer.data(),
        src_size,
        nullptr);

    auto src_values = reinterpret_cast<SgType*>(buffer.data());

    for (decltype(TCount) i = 0; i < TCount; ++i) {
        ::sg_import(src_values[i], dst_values[i]);
    }

    return read_result;
}

template<typename TDst, std::size_t TCount>
inline int sg_read_no_cast(
    SgReadFunc& read_func,
    unsigned int chunk_id,
    TDst (&dst_values)[TCount],
    SgArray1dTag)
{
    using Tag = typename std::conditional<
        std::is_same<TDst, bool>::value,
        void,
        typename std::conditional<
            detail::SgTraits<TDst>::is_numeric(),
            detail::SgNumericTag,
            typename std::conditional<
                detail::SgTraits<TDst>::has_sg_import(),
                detail::SgClassTag,
                void
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");

    return sg_read_no_cast(
        read_func,
        chunk_id,
        dst_values,
        SgArray1dTag(),
        Tag()
    );
}


} // detail


template<typename TDst, typename TInstance>
inline int sg_read_no_cast(
    TInstance&& instance,
    unsigned int chunk_id,
    TDst& dst_value)
{
    using Tag = typename std::conditional<
        std::is_same<TDst, bool>::value,
        void,
        typename std::conditional<
            detail::SgTraits<TDst>::is_numeric(),
            detail::SgNumericTag,
            typename std::conditional<
                detail::SgTraits<TDst>::has_sg_import(),
                detail::SgClassTag,
                typename std::conditional<
                    detail::SgTraits<TDst>::is_array_1d(),
                    detail::SgArray1dTag,
                    void
                >::type
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");

    auto read_func = detail::sg_get_read_func(
        instance);

    return detail::sg_read_no_cast(
        read_func,
        chunk_id,
        dst_value,
        Tag());
}

// sg_read_no_cast (fixed size)
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// sg_read_no_cast (dynamic size)
//
// Reads a specified number of values from a
// saved game without conversion.
//

namespace detail {


template<typename TDst>
inline int sg_read_no_cast(
    SgReadFunc& read_func,
    unsigned int chunk_id,
    TDst* dst_values,
    int dst_count,
    SgPointerTag,
    SgNumericTag)
{
    const auto dst_size = static_cast<int>(dst_count * sizeof(TDst));

    return read_func(
        chunk_id,
        dst_values,
        dst_size,
        nullptr);
}

inline int sg_read_no_cast(
    SgReadFunc& read_func,
    unsigned int chunk_id,
    void* dst_values,
    int dst_count,
    SgPointerTag,
    SgVoidTag)
{
    return read_func(
        chunk_id,
        dst_values,
        dst_count,
        nullptr);
}

template<typename TDst>
inline int sg_read_no_cast(
    SgReadFunc& read_func,
    unsigned int chunk_id,
    TDst* dst_values,
    int dst_count,
    SgPointerTag)
{
    using Tag = typename std::conditional<
        std::is_same<TDst, bool>::value,
        void,
        typename std::conditional<
            SgTraits<TDst>::is_numeric(),
            SgNumericTag,
            typename std::conditional<
                std::is_same<TDst, void>::value,
                SgVoidTag,
                void
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");

    return sg_read_no_cast(
        read_func,
        chunk_id,
        dst_values,
        dst_count,
        SgPointerTag(),
        Tag());
}

template<typename TDst, std::size_t TCount>
inline int sg_read_no_cast(
    SgReadFunc& read_func,
    unsigned int chunk_id,
    TDst (&dst_values)[TCount],
    int dst_count,
    SgArray1dTag)
{
    if (dst_count > TCount) {
        throw SgException("Read overflow.");
    }

    return sg_read_no_cast(
        read_func,
        chunk_id,
        &dst_values[0],
        dst_count,
        SgPointerTag());
}


} // detail


template<typename TDst, typename TInstance>
inline int sg_read_no_cast(
    TInstance&& instance,
    unsigned int chunk_id,
    TDst& dst_value,
    int dst_count)
{
    using Tag = typename std::conditional<
        std::is_same<TDst, bool>::value,
        void,
        typename std::conditional<
            std::is_pointer<TDst>::value,
            detail::SgPointerTag,
            typename std::conditional<
                detail::SgTraits<TDst>::is_array_1d(),
                detail::SgArray1dTag,
                void
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");


    auto read_func = detail::sg_get_read_func(
        instance);

    return detail::sg_read_no_cast(
        read_func,
        chunk_id,
        dst_value,
        dst_count,
        Tag());
}

// sg_read_no_cast (dynamic size)
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// sg_read (fixed size)
//
// Reads a value or a fixed-array of values from a
// saved game with conversion.
//

namespace detail {


template<typename TSrc, typename TDst>
inline int sg_read(
    SgReadFunc& read_func,
    unsigned int chunk_id,
    TDst& dst_value,
    SgBoolTag)
{
    TSrc src_value;
    constexpr auto src_size = static_cast<int>(sizeof(TSrc));

    auto read_result = read_func(
        chunk_id,
        &src_value,
        src_size,
        nullptr);

    dst_value = (src_value != 0);

    return read_result;
}

template<typename TSrc, typename TDst>
inline int sg_read(
    SgReadFunc& read_func,
    unsigned int chunk_id,
    TDst& dst_value,
    SgNumericTag)
{
    TSrc src_value;
    constexpr auto src_size = static_cast<int>(sizeof(TSrc));

    auto read_result = read_func(
        chunk_id,
        &src_value,
        src_size,
        nullptr);

    dst_value = static_cast<TDst>(src_value);

    return read_result;
}

template<typename TSrc, typename TDst>
inline int sg_read(
    SgReadFunc& read_func,
    unsigned int chunk_id,
    TDst& dst_value,
    SgPointerTag)
{
    TSrc src_value;
    constexpr auto src_size = static_cast<int>(sizeof(TSrc));

    auto read_result = read_func(
        chunk_id,
        &src_value,
        src_size,
        nullptr);

    dst_value = reinterpret_cast<TDst>(static_cast<std::intptr_t>(src_value));

    return read_result;
}

template<typename TSrc, typename TDst, std::size_t TCount>
inline int sg_read(
    SgReadFunc& read_func,
    unsigned int chunk_id,
    TDst (&dst_values)[TCount],
    SgArray1dTag,
    SgNumericTag)
{
    constexpr auto src_size = static_cast<int>(TCount * sizeof(TSrc));

    auto& src_buffer = sg_get_buffer(
        src_size);

    auto read_result = read_func(
        chunk_id,
        src_buffer.data(),
        src_size,
        nullptr);

    auto src_values = reinterpret_cast<const TSrc*>(src_buffer.data());

    std::uninitialized_copy_n(
        src_values,
        TCount,
        dst_values);

    return read_result;
}

template<typename TSrc, typename TDst, std::size_t TCount>
inline int sg_read(
    SgReadFunc& read_func,
    unsigned int chunk_id,
    TDst (&dst_values)[TCount],
    SgArray1dTag)
{
    using Tag = typename std::conditional<
        std::is_same<TSrc, bool>::value || std::is_same<TDst, bool>::value,
        void,
        typename std::conditional<
            SgTraits<TSrc>::is_numeric() && SgTraits<TDst>::is_numeric(),
            SgNumericTag,
            void
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");

    return sg_read<TSrc>(
        read_func,
        chunk_id,
        dst_values,
        SgArray1dTag(),
        Tag());
}


} // detail


template<typename TSrc, typename TDst, typename TInstance>
inline int sg_read(
    TInstance&& instance,
    unsigned int chunk_id,
    TDst& dst_value)
{
    using Tag = typename std::conditional<
        detail::SgTraits<TSrc>::is_numeric() &&
            std::is_same<TDst, bool>::value,
        detail::SgBoolTag,
        typename std::conditional<
            detail::SgTraits<TSrc>::is_numeric() &&
                detail::SgTraits<TDst>::is_numeric(),
            detail::SgNumericTag,
            typename std::conditional<
                detail::SgTraits<TSrc>::is_numeric() &&
                    std::is_pointer<TDst>::value,
                detail::SgPointerTag,
                typename std::conditional<
                    detail::SgTraits<TDst>::is_array_1d(),
                    detail::SgArray1dTag,
                    void
                >::type
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");

    auto read_func = detail::sg_get_read_func(
        instance);

    return detail::sg_read<TSrc>(
        read_func,
        chunk_id,
        dst_value,
        Tag());
}

// sg_read (fixed size)
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// sg_read (dynamic size)
//
// Reads a spesified number of values from a
// saved game with conversion.
//

namespace detail {


template<typename TSrc, typename TDst>
inline int sg_read(
    SgReadFunc& read_func,
    unsigned int chunk_id,
    TDst* dst_values,
    int dst_count,
    SgPointerTag)
{
    if (!dst_values) {
        throw SgException("Null pointer.");
    }

    const auto src_size = static_cast<int>(dst_count * sizeof(TSrc));

    auto& buffer = sg_get_buffer(
        src_size);

    auto read_result = read_func(
        chunk_id,
        buffer.data(),
        src_size,
        nullptr);

    auto src_values = reinterpret_cast<const TSrc*>(buffer.data());

    std::uninitialized_copy_n(
        src_values,
        dst_count,
        dst_values);

    return read_result;
}

template<typename TSrc, typename TDst, std::size_t TCount>
inline int sg_read(
    SgReadFunc& read_func,
    unsigned int chunk_id,
    TDst (&dst_values)[TCount],
    int dst_count,
    SgArray1dTag)
{
    return sg_read<TSrc>(
        read_func,
        chunk_id,
        &dst_values[0],
        dst_count,
        SgPointerTag());
}


} // detail


template<typename TSrc, typename TDst, typename TInstance>
inline int sg_read(
    TInstance&& instance,
    unsigned int chunk_id,
    TDst& dst_value,
    int dst_count)
{
    using Tag = typename std::conditional<
        detail::SgTraits<TSrc>::is_numeric() &&
            std::is_pointer<TDst>::value,
        detail::SgPointerTag,
        typename std::conditional<
            detail::SgTraits<TDst>::is_array_1d(),
            detail::SgArray1dTag,
            void
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");

    if (dst_count < 0) {
        throw SgException("Negative count.");
    }

    auto read_func = detail::sg_get_read_func(
        instance);

    return detail::sg_read<TSrc>(
        read_func,
        chunk_id,
        dst_value,
        dst_count,
        Tag());
}

// sg_read (dynamic size)
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// sg_read_skip
//
// Skips a specified number of bytes in a saved game.
//

template<typename TInstance>
inline int sg_read_skip(
    TInstance&& instance,
    unsigned int chunk_id,
    int count)
{
    if (count < 0) {
        throw SgException("Negative count.");
    }

    auto read_func = detail::sg_get_read_func(
        instance);

    return read_func(
        chunk_id,
        nullptr,
        count,
        nullptr);
}

template<typename TSkip, typename TInstance>
inline int sg_read_skip(
    TInstance&& instance,
    unsigned int chunk_id)
{
    constexpr auto skip_size = static_cast<int>(sizeof(TSkip));

    return sg_read_skip(
        instance,
        chunk_id,
        skip_size);
}

// sg_read_skip
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// Reads a data from a saved game into an automatically allocated block.
template<typename TInstance, typename TValue>
inline int sg_read_allocate(
    TInstance&& instance,
    unsigned int chunk_id,
    TValue*& value)
{
    auto read_func = detail::sg_get_read_func(
        instance);

    void* void_ptr = nullptr;

    auto read_result = read_func(
        chunk_id,
        nullptr,
        0,
        &void_ptr);

    value = static_cast<TValue*>(void_ptr);

    return read_result;
}


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// sg_write_no_cast (fixed size)
//
// Writes a value or a fixed-size array of values into a
// saved game without conversion.
//

namespace detail {


template<typename TSrc>
inline int sg_write_no_cast(
    SgWriteFunc& write_func,
    unsigned int chunk_id,
    const TSrc& src_value,
    SgNumericTag)
{
    constexpr auto src_size = static_cast<int>(sizeof(TSrc));

    return write_func(
        chunk_id,
        const_cast<TSrc*>(&src_value),
        src_size);
}

template<typename TSrc>
inline int sg_write_no_cast(
    SgWriteFunc& write_func,
    unsigned int chunk_id,
    const TSrc& src_value,
    SgClassTag)
{
    using SgType = typename TSrc::SgType;

    constexpr auto dst_size = static_cast<int>(sizeof(SgType));

    auto& dst_buffer = sg_get_buffer(
        dst_size);

    auto& dst_value = *reinterpret_cast<SgType*>(dst_buffer.data());

    ::sg_export(src_value, dst_value);

    return write_func(
        chunk_id,
        &dst_value,
        dst_size);
}

template<typename TSrc, std::size_t TCount>
inline int sg_write_no_cast(
    SgWriteFunc& write_func,
    unsigned int chunk_id,
    const TSrc (&src_values)[TCount],
    SgArray1dTag,
    SgNumericTag)
{
    constexpr auto src_size = static_cast<int>(TCount * sizeof(TSrc));

    return write_func(
        chunk_id,
        const_cast<TSrc*>(&src_values[0]),
        src_size);
}

template<typename TSrc, std::size_t TCount>
inline int sg_write_no_cast(
    SgWriteFunc& write_func,
    unsigned int chunk_id,
    const TSrc (&src_values)[TCount],
    SgArray1dTag,
    SgClassTag)
{
    using SgType = typename TSrc::SgType;

    constexpr auto dst_size = static_cast<int>(TCount * sizeof(SgType));

    auto& dst_buffer = sg_get_buffer(
        dst_size);

    auto dst_values = reinterpret_cast<SgType*>(dst_buffer.data());

    for (decltype(TCount) i = 0; i < TCount; ++i) {
        ::sg_export(src_values[i], dst_values[i]);
    }

    return write_func(
        chunk_id,
        dst_values,
        dst_size);
}

template<typename TSrc, std::size_t TCount>
inline int sg_write_no_cast(
    SgWriteFunc& write_func,
    unsigned int chunk_id,
    const TSrc (&src_values)[TCount],
    SgArray1dTag)
{
    using Tag = typename std::conditional<
        std::is_same<TSrc, bool>::value,
        void,
        typename std::conditional<
            detail::SgTraits<TSrc>::is_numeric(),
            detail::SgNumericTag,
            typename std::conditional<
                detail::SgTraits<TSrc>::has_sg_export(),
                detail::SgClassTag,
                void
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");

    return sg_write_no_cast(
        write_func,
        chunk_id,
        src_values,
        SgArray1dTag(),
        Tag()
    );
}


} // detail


template<typename TSrc, typename TInstance>
inline int sg_write_no_cast(
    TInstance&& instance,
    unsigned int chunk_id,
    const TSrc& src_value)
{
    using Tag = typename std::conditional<
        std::is_same<TSrc, bool>::value,
        void,
        typename std::conditional<
            detail::SgTraits<TSrc>::is_numeric(),
            detail::SgNumericTag,
            typename std::conditional<
                detail::SgTraits<TSrc>::has_sg_export(),
                detail::SgClassTag,
                typename std::conditional<
                    detail::SgTraits<TSrc>::is_array_1d(),
                    detail::SgArray1dTag,
                    void
                >::type
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");

    auto write_func = detail::sg_get_write_func(
        instance);

    return detail::sg_write_no_cast(
        write_func,
        chunk_id,
        src_value,
        Tag());
}

// sg_write_no_cast (fixed size)
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// sg_write_no_cast (dynamic size)
//
// Writes a specified number of values into a
// saved game without conversion.
//

namespace detail {


template<typename TSrc>
inline int sg_write_no_cast(
    SgWriteFunc& write_func,
    unsigned int chunk_id,
    const TSrc* src_values,
    int src_count,
    SgPointerTag,
    SgVoidTag)
{
    return write_func(
        chunk_id,
        const_cast<TSrc*>(src_values),
        src_count);
}

template<typename TSrc>
inline int sg_write_no_cast(
    SgWriteFunc& write_func,
    unsigned int chunk_id,
    const TSrc* src_values,
    int src_count,
    SgPointerTag,
    SgNumericTag)
{
    const auto src_size = static_cast<int>(src_count * sizeof(TSrc));

    return write_func(
        chunk_id,
        const_cast<TSrc*>(src_values),
        src_size);
}

template<typename TSrc>
inline int sg_write_no_cast(
    SgWriteFunc& write_func,
    unsigned int chunk_id,
    const TSrc* src_values,
    int src_count,
    SgPointerTag)
{
    using Tag = typename std::conditional<
        std::is_same<TSrc, bool>::value,
        void,
        typename std::conditional<
            SgTraits<TSrc>::is_numeric(),
            SgNumericTag,
            typename std::conditional<
                std::is_same<TSrc, void>::value,
                SgVoidTag,
                void
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");

    if (!src_values) {
        throw SgException("Null pointer.");
    }

    return sg_write_no_cast(
        write_func,
        chunk_id,
        src_values,
        src_count,
        SgPointerTag(),
        Tag());
}

template<typename TSrc, std::size_t TCount>
inline int sg_write_no_cast(
    SgWriteFunc& write_func,
    unsigned int chunk_id,
    const TSrc (&src_values)[TCount],
    int src_count,
    SgArray1dTag)
{
    if (src_count > TCount) {
        throw SgException("Write overflow.");
    }

    return sg_write_no_cast(
        write_func,
        chunk_id,
        &src_values[0],
        src_count,
        SgPointerTag());
}


} // detail


template<typename TSrc, typename TInstance>
inline int sg_write_no_cast(
    TInstance&& instance,
    unsigned int chunk_id,
    TSrc&& src_value,
    int src_count)
{
    using TSrcTag = typename std::remove_reference<TSrc>::type;

    using Tag = typename std::conditional<
        std::is_same<TSrcTag, bool>::value,
        void,
        typename std::conditional<
            std::is_pointer<TSrcTag>::value,
            detail::SgPointerTag,
            typename std::conditional<
                detail::SgTraits<TSrcTag>::is_array_1d(),
                detail::SgArray1dTag,
                void
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");


    auto write_func = detail::sg_get_write_func(
        instance);

    return detail::sg_write_no_cast(
        write_func,
        chunk_id,
        src_value,
        src_count,
        Tag());
}

// sg_write_no_cast (dynamic size)
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// sg_write (fixed size)
//
// Writes a value or a fixed-array of values into a
// saved game with conversion.
//

namespace detail {


template<typename TDst, typename TSrc>
inline int sg_write(
    SgWriteFunc& write_func,
    unsigned int chunk_id,
    const TSrc& src_value,
    SgNumericTag)
{
    auto dst_value = static_cast<TDst>(src_value);
    constexpr auto dst_size = static_cast<int>(sizeof(TDst));

    return write_func(
        chunk_id,
        &dst_value,
        dst_size);
}

template<typename TDst, typename TSrc>
inline int sg_write(
    SgWriteFunc& write_func,
    unsigned int chunk_id,
    const TSrc& src_value,
    SgPointerTag)
{
    auto dst_value = static_cast<TDst>(reinterpret_cast<std::intptr_t>(src_value));

    constexpr auto dst_size = static_cast<int>(sizeof(TDst));

    return write_func(
        chunk_id,
        &dst_value,
        dst_size);
}

template<typename TDst, typename TSrc, std::size_t TCount>
inline int sg_write(
    SgWriteFunc& write_func,
    unsigned int chunk_id,
    const TSrc (&src_values)[TCount],
    SgArray1dTag,
    SgNumericTag)
{
    constexpr auto dst_size = static_cast<int>(TCount * sizeof(TDst));

    auto& dst_buffer = sg_get_buffer(
        dst_size);

    auto dst_values = reinterpret_cast<TDst*>(dst_buffer.data());

    std::uninitialized_copy_n(
        src_values,
        TCount,
        dst_values);

    return write_func(
        chunk_id,
        dst_values,
        dst_size);
}

template<typename TDst, typename TSrc, std::size_t TCount>
inline int sg_write(
    SgWriteFunc& write_func,
    unsigned int chunk_id,
    const TSrc (&src_values)[TCount],
    SgArray1dTag)
{
    using Tag = typename std::conditional<
        SgTraits<TSrc>::is_numeric() && SgTraits<TDst>::is_numeric(),
        SgNumericTag,
        void
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");

    return sg_write<TDst>(
        write_func,
        chunk_id,
        src_values,
        SgArray1dTag(),
        Tag());
}


} // detail


template<typename TDst, typename TSrc, typename TInstance>
inline int sg_write(
    TInstance&& instance,
    unsigned int chunk_id,
    const TSrc& src_value)
{
    using Tag = typename std::conditional<
        detail::SgTraits<TSrc>::is_numeric() &&
            detail::SgTraits<TDst>::is_numeric(),
        detail::SgNumericTag,
        typename std::conditional<
            std::is_pointer<TSrc>::value &&
                detail::SgTraits<TDst>::is_numeric(),
            detail::SgPointerTag,
            typename std::conditional<
                detail::SgTraits<TSrc>::is_array_1d(),
                detail::SgArray1dTag,
                void
            >::type
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");

    auto write_func = detail::sg_get_write_func(
        instance);

    return detail::sg_write<TDst>(
        write_func,
        chunk_id,
        src_value,
        Tag());
}

// sg_write (fixed size)
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
// sg_write (dynamic size)
//
// Reads a spesified number of values from a
// saved game with conversion.
//

namespace detail {


template<typename TDst, typename TSrc>
inline int sg_write(
    SgWriteFunc& write_func,
    unsigned int chunk_id,
    const TSrc* src_values,
    int src_count,
    SgPointerTag)
{
    if (!src_values) {
        throw SgException("Null pointer.");
    }

    const auto dst_size = static_cast<int>(src_count * sizeof(TDst));

    auto& dst_buffer = sg_get_buffer(
        dst_size);

    auto dst_values = reinterpret_cast<TDst*>(dst_buffer.data());

    std::uninitialized_copy_n(
        src_values,
        src_count,
        dst_values);

    return write_func(
        chunk_id,
        dst_values,
        dst_size);
}

template<typename TDst, typename TSrc, std::size_t TCount>
inline int sg_write(
    SgWriteFunc& write_func,
    unsigned int chunk_id,
    const TSrc (&src_values)[TCount],
    int src_count,
    SgArray1dTag)
{
    return sg_write<TDst>(
        write_func,
        chunk_id,
        &src_values[0],
        src_count,
        SgPointerTag());
}


} // detail


template<typename TDst, typename TSrc, typename TInstance>
inline int sg_write(
    TInstance&& instance,
    unsigned int chunk_id,
    const TSrc& src_value,
    int src_count)
{
    using Tag = typename std::conditional<
        std::is_pointer<TSrc>::value &&
            detail::SgTraits<TDst>::is_numeric(),
        detail::SgPointerTag,
        typename std::conditional<
            detail::SgTraits<TSrc>::is_array_1d(),
            detail::SgArray1dTag,
            void
        >::type
    >::type;

    static_assert(
        !std::is_same<Tag, void>::value,
        "Unsupported types.");

    if (src_count < 0) {
        throw SgException("Negative count.");
    }

    auto write_func = detail::sg_get_write_func(
        instance);

    return detail::sg_write<TDst>(
        write_func,
        chunk_id,
        src_value,
        src_count,
        Tag());
}

// sg_write (dynamic size)
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


#endif // __cplusplus


#endif // OJK_SG_WRAPPERS_INCLUDED
