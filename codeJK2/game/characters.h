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

typedef enum //# characters_e
{
	//HazTeam Alpha
	//CHARACTER_MUNRO = 0,
	CHARACTER_FOSTER = 0,
	CHARACTER_TELSIA,
	CHARACTER_BIESSMAN,
	CHARACTER_CHANG,
	CHARACTER_CHELL,
	CHARACTER_JUROT,
	//HazTeam Beta
	CHARACTER_LAIRD,
	CHARACTER_KENN,
	CHARACTER_OVIEDO,
	CHARACTER_ODELL,
	CHARACTER_NELSON,
	CHARACTER_JAWORSKI,
	CHARACTER_CSATLOS,
	//Senior Crew
	CHARACTER_JANEWAY,
	CHARACTER_CHAKOTAY,
	CHARACTER_TUVOK,
	CHARACTER_TUVOKHAZ,
	CHARACTER_TORRES,
	CHARACTER_PARIS,
	CHARACTER_KIM,
	CHARACTER_DOCTOR,
	CHARACTER_SEVEN,
	CHARACTER_SEVENHAZ,
	CHARACTER_NEELIX,
	//Other Crew
	CHARACTER_PELLETIER,
	//Generic Crew
	CHARACTER_CREWMAN,
	//CHARACTER_ENSIGN,
	CHARACTER_LT,
	CHARACTER_COMM,
	CHARACTER_CAPT,
	CHARACTER_GENERIC1,
	CHARACTER_GENERIC2,
	CHARACTER_GENERIC3,
	CHARACTER_GENERIC4,
	//# #eol
	CHARACTER_NUM_CHARS
} characters_t;

typedef struct
{
	char	*name;
	char	*sound;
	sfxHandle_t		soundIndex;
} characterName_t;
