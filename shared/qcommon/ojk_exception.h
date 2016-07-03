//
// Base exception.
//


#ifndef OJK_EXCEPTION_INCLUDED
#define OJK_EXCEPTION_INCLUDED


#include <stdexcept>
#include <string>


namespace ojk {


class Exception :
    public std::runtime_error
{
public:
    explicit Exception(
        const char* message);

    explicit Exception(
        const std::string& message);

    virtual ~Exception();
}; // Exception


} // ojk


#endif // OJK_EXCEPTION_INCLUDED

