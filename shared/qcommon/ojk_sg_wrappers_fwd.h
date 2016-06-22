//
// Forward declarartions of saved game wrappers.
//


#ifndef OJK_SG_WRAPPERS_FWD_INCLUDED
#define OJK_SG_WRAPPERS_FWD_INCLUDED


#include "ojk_sg_wrappers_shared.h"


#ifdef __cplusplus


// Exports a value or a fixed-array of values from a
// native type to a saved game one.
template<typename TSrc, typename TDst>
void sg_export(
    const TSrc& src,
    TDst& dst);

// Imports a value or a fixed-szie array of values from a
// saved game type to a native one.
template<typename TSrc, typename TDst>
void sg_import(
    const TSrc& src,
    TDst& dst);

// Reads a value or a fixed-size array of values from a
// saved game without conversion.
template<typename TDst, typename TInstance>
int sg_read_no_cast(
    TInstance&& instance,
    unsigned int chunk_id,
    TDst& dst_value);

// Reads a specified number of values from a
// saved game without conversion.
template<typename TDst, typename TInstance>
int sg_read_no_cast(
    TInstance&& instance,
    unsigned int chunk_id,
    TDst& dst_value,
    int dst_count);

// Reads a value or a fixed-array of values from a
// saved game with conversion.
template<typename TSrc, typename TDst, typename TInstance>
int sg_read(
    TInstance&& instance,
    unsigned int chunk_id,
    TDst& dst_value);

// Reads a spesified number of values from a
// saved game with conversion.
template<typename TSrc, typename TDst, typename TInstance>
int sg_read(
    TInstance&& instance,
    unsigned int chunk_id,
    TDst& dst_value,
    int dst_count);

// Skips a specified number of bytes in a saved game.
template<typename TInstance>
int sg_read_skip(
    TInstance&& instance,
    unsigned int chunk_id,
    int count);

// Skips a specified number (by a type size) of bytes in a saved game.
template<typename TSkip, typename TInstance>
int sg_read_skip(
    TInstance&& instance,
    unsigned int chunk_id);

// Reads a data from a saved game into an automatically allocated block.
template<typename TInstance, typename TValue>
int sg_read_allocate(
    TInstance&& instance,
    unsigned int chunk_id,
    TValue*& value);

// Writes a value or a fixed-size array of values into a
// saved game without conversion.
template<typename TSrc, typename TInstance>
int sg_write_no_cast(
    TInstance&& instance,
    unsigned int chunk_id,
    const TSrc& src_value);

// Writes a specified number of values into a
// saved game without conversion.
template<typename TSrc, typename TInstance>
int sg_write_no_cast(
    TInstance&& instance,
    unsigned int chunk_id,
    TSrc&& src_value,
    int src_count);

// Writes a value or a fixed-array of values into a
// saved game with conversion.
template<typename TDst, typename TSrc, typename TInstance>
int sg_write(
    TInstance&& instance,
    unsigned int chunk_id,
    const TSrc& src_value);

// Reads a spesified number of values from a
// saved game with conversion.
template<typename TDst, typename TSrc, typename TInstance>
int sg_write(
    TInstance&& instance,
    unsigned int chunk_id,
    const TSrc& src_value,
    int src_count);


#endif // __cplusplus


#endif // OJK_SG_WRAPPERS_FWD_INCLUDED
