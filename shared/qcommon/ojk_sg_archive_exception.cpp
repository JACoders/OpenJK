//
// Saved game archive exception.
//


#include "ojk_sg_archive_exception.h"


namespace ojk {
namespace sg {


ArchiveException::ArchiveException(
    const char* message) :
        Exception(message)
{
}

ArchiveException::ArchiveException(
    const std::string& message) :
        Exception(message)
{
}

ArchiveException::~ArchiveException()
{
}


} // sg
} // ojk
