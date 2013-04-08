/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

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
