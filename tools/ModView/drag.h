// Filename:-	drag.h
//

#ifndef DRAG_H
#define DRAG_H



enum mkey_enum {
	KEY_LBUTTON = MK_LBUTTON,
	KEY_RBUTTON = MK_RBUTTON,
	KEY_MBUTTON = MK_MBUTTON	
};


void start_drag( mkey_enum keyFlags, int x, int y );
bool drag(  mkey_enum keyFlags, int x, int y );
void end_drag(  mkey_enum keyFlags, int x, int y );



#endif	// #ifndef DRAG_H

///////////// eof ///////////

