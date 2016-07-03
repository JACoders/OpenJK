//
// Base exception.
//


#include "ojk_exception.h"


namespace ojk {


Exception::Exception(
    const char* message) :
        std::runtime_error(message)
{
}

Exception::Exception(
    const std::string& message) :
        std::runtime_error(message)
{
}

Exception::~Exception()
{
}


} // ojk
