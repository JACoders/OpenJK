//
// Shared staff of saved game wrappers.
//


#ifndef OJK_SG_WRAPPERS_SHARED_INCLUDED
#define OJK_SG_WRAPPERS_SHARED_INCLUDED


#ifdef __cplusplus


#include <cstddef>
#include <array>
#include <stdexcept>
#include <vector>


template<typename T, std::size_t TCount>
using SgArray = std::array<T, TCount>;

template<typename T, std::size_t TCount1, std::size_t TCount2>
using SgArray2d = SgArray<SgArray<T, TCount2>, TCount1>;

using SgVec3 = SgArray<float, 3>;
using SgVec4 = SgArray<float, 4>;

using SgBuffer = std::vector<uint8_t>;


class SgException :
    public std::runtime_error
{
public:
    explicit SgException(
        const std::string& what_arg) :
            std::runtime_error(what_arg)
    {
    }

    explicit SgException(
        const char* what_arg) :
            std::runtime_error(what_arg)
    {
    }

    virtual ~SgException()
    {
    }
}; // SgException


inline SgBuffer& sg_get_buffer(
    int new_size)
{
    static SgBuffer buffer;
    buffer.resize(new_size);
    return buffer;
}


#endif // __cplusplus


#endif // OJK_SG_WRAPPERS_SHARED_INCLUDED
