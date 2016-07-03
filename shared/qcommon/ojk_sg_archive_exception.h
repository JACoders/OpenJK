//
// Saved game archive exception.
//


#ifndef OJK_SG_ARCHIVE_EXCEPTION_INCLUDED
#define OJK_SG_ARCHIVE_EXCEPTION_INCLUDED


#include "ojk_exception.h"


namespace ojk {
namespace sg {


class ArchiveException :
    public Exception
{
public:
    explicit ArchiveException(
        const char* message);

    explicit ArchiveException(
        const std::string& message);

    virtual ~ArchiveException();
}; // ArchiveException


} // sg
} // ojk


#endif // OJK_SG_ARCHIVE_EXCEPTION_INCLUDED

