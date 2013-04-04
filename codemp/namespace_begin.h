#ifdef _XBOX

#ifdef _GAME
namespace game {
#elif defined _CGAME
namespace cgame {
#elif defined _UI
namespace ui {
#else
// Hmmm. Some of the module headers end up included in EXE code.
// Let's hope that the apocalpyse can be held at bay.
// #error Including namespace_begin.h, but not in UI/GAME/CGAME
#endif

#endif // _XBOX