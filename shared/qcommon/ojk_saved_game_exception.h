//
// Saved game exception.
//


#ifndef OJK_SAVED_GAME_EXCEPTION_INCLUDED
#define OJK_SAVED_GAME_EXCEPTION_INCLUDED


#include "ojk_exception.h"


namespace ojk
{


class SavedGameException :
    public Exception
{
public:
    explicit SavedGameException(
        const char* message);

    explicit SavedGameException(
        const std::string& message);

    virtual ~SavedGameException();
}; // SavedGameException


} // ojk


#endif // OJK_SAVED_GAME_EXCEPTION_INCLUDED

