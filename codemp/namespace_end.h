#ifdef _XBOX

#ifdef _GAME
}
using namespace game;
#elif defined _CGAME
}
using namespace cgame;
#elif defined _UI
}
using namespace ui;
#else
// See comment int namespace_begin.h
//#error Including namespace_end.h, but not in UI/GAME/CGAME
#endif

#endif _XBOX