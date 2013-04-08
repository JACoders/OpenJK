/*
This file is part of Jedi Knight 2.

    Jedi Knight 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Knight 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Knight 2.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

#pragma once
#if !defined(CG_LIGHTS_H_INC)
#define CG_LIGHTS_H_INC

typedef struct
{
	int				length;
	color4ub_t		value;
	color4ub_t		map[MAX_QPATH];
} clightstyle_t;

void	CG_ClearLightStyles (void);
void	CG_RunLightStyles (void);
void	CG_SetLightstyle (int i);

#endif // CG_LIGHTS_H_INC