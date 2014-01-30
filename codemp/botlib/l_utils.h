#pragma once

/*****************************************************************************
 * name:		l_util.h
 *
 * desc:		utils
 *
 * $Archive: /source/code/botlib/l_util.h $
 * $Author: Mrelusive $
 * $Revision: 2 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 10/05/99 3:42p $
 *
 *****************************************************************************/

#define Vector2Angles(v,a)		vectoangles(v,a)
#ifdef MAX_PATH
	#undef MAX_PATH
#endif
#define MAX_PATH				MAX_QPATH
#define Maximum(x,y)			(x > y ? x : y)
#define Minimum(x,y)			(x < y ? x : y)
