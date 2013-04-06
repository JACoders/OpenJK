// Filename:	statindex.h
//
// accessed from both server and game modules

#ifndef STATINDEX_H
#define STATINDEX_H


// player_state->stats[] indexes
typedef enum {
	STAT_HEALTH,
	STAT_ITEMS,
	STAT_WEAPONS,					// 16 bit fields
	STAT_ARMOR,				
	STAT_DEAD_YAW,					// look this direction when dead (FIXME: get rid of?)
	STAT_CLIENTS_READY,				// bit mask of clients wishing to exit the intermission (FIXME: configstring?)
	STAT_MAX_HEALTH					// health / armor limit, changable by handicap
} statIndex_t;	   



#endif	// #ifndef STATINDEX_H


/////////////////////// eof /////////////////////

