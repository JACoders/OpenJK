//
// Saved game exception.
//


#include "ojk_saved_game_exception.h"


namespace ojk
{


SavedGameException::SavedGameException(
    const char* message) :
        Exception(message)
{
}

SavedGameException::SavedGameException(
    const std::string& message) :
        Exception(message)
{
}

SavedGameException::~SavedGameException()
{
}


} // ojk
