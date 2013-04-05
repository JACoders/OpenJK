#ifndef __SAY_H__
#define __SAY_H__

typedef enum //# saying_e
{
	//Acknowledge command
	SAY_ACKCOMM1,
	SAY_ACKCOMM2,
	SAY_ACKCOMM3,
	SAY_ACKCOMM4,
	//Refuse command
	SAY_REFCOMM1,
	SAY_REFCOMM2,
	SAY_REFCOMM3,
	SAY_REFCOMM4,
	//Bad command
	SAY_BADCOMM1,
	SAY_BADCOMM2,
	SAY_BADCOMM3,
	SAY_BADCOMM4,
	//Unfinished hail
	SAY_BADHAIL1,
	SAY_BADHAIL2,
	SAY_BADHAIL3,
	SAY_BADHAIL4,
	//# #eol
	NUM_SAYINGS
} saying_t;

#endif //#ifndef __SAY_H__
