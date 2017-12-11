/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

/*****************************************************************************
 * name:		be_ai_char.h
 *
 * desc:		bot characters
 *
 * $Archive: /source/code/botlib/be_ai_char.h $
 * $Author: osman $
 * $Revision: 1.4 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 2003/03/15 23:43:59 $
 *
 *****************************************************************************/

#pragma once

//loads a bot character from a file
int BotLoadCharacter(char *charfile, float skill);
//frees a bot character
void BotFreeCharacter(int character);
//returns a float characteristic
float Characteristic_Float(int character, int index);
//returns a bounded float characteristic
float Characteristic_BFloat(int character, int index, float min, float max);
//returns an integer characteristic
int Characteristic_Integer(int character, int index);
//returns a bounded integer characteristic
int Characteristic_BInteger(int character, int index, int min, int max);
//returns a string characteristic
void Characteristic_String(int character, int index, char *buf, int size);
//free cached bot characters
void BotShutdownCharacters(void);
