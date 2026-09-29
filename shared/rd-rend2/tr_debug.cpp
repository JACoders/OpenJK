/*
===========================================================================
Copyright (C) 2016, OpenJK contributors

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
#include "tr_local.h"

void R_PushDebugGroup(annotationLayer_t layer, const char* name)
{
	static GLuint currentLayer = (GLuint)AL_NONE;
	assert(layer <= currentLayer + 1);
	while (layer <= currentLayer)
	{
		if (currentLayer == AL_NONE)
			break;
		qglPopDebugGroupKHR();
		currentLayer--;
	}
	if (layer == AL_NONE)
		return;
	currentLayer = (GLuint)layer;
	qglPushDebugGroupKHR(GL_DEBUG_SOURCE_APPLICATION, currentLayer, -1, name);
}