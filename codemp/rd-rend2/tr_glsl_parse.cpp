/*
===========================================================================
Copyright (C) 2013 - 2016, OpenJK contributors

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
#include "tr_allocator.h"
#if defined(GLSL_BUILDTOOL)
#include <iostream>
#endif

namespace
{

Block *FindBlock( const char *name, Block *blocks, size_t numBlocks )
{
	for ( size_t i = 0; i < numBlocks; ++i )
	{
		Block *block = blocks + i;
		if ( strncmp(block->blockHeaderTitle, name, block->blockHeaderTitleLength) == 0 )
		{
			return block;
		}
	}

	return nullptr;
}

#if defined(__clang__)
void strncpy_s( char *dest, size_t destSize, const char *src, size_t srcSize )
{
	// This isn't really a safe version, but I know the inputs to expect.
	size_t len = std::min(srcSize, destSize);
	memcpy(dest, src, len);
	if ( (destSize - len) > 0 )
		memset(dest + len, 0, destSize - len);
}
#endif

}

GPUProgramDesc ParseProgramSource( Allocator& allocator, const char *text )
{
	int numBlocks = 0;
	Block blocks[MAX_BLOCKS];
	Block *prevBlock = nullptr;

	int i = 0;
	int line = 1;
	while ( text[i] )
	{
		if ( strncmp(text + i, "/*[", 3) == 0 )
		{
			int startHeaderTitle = i + 3;
			int endHeaderTitle = -1;
			int endHeaderText = -1;
			int j = startHeaderTitle;
			while ( text[j] )
			{
				if ( text[j] == ']' )
				{
					endHeaderTitle = j;
				}
				else if ( strncmp(text + j, "*/\n", 3) == 0 )
				{
					endHeaderText = j;
					line++;
					break;
				}
				else if ( text[j] == '\n' )
				{
					line++;
				}

				++j;
			}

			if ( endHeaderTitle == -1 || endHeaderText == -1 )
			{
#if defined(GLSL_BUILDTOOL)
				std::cerr << "Unclosed block marker\n";
#else
				Com_Printf(S_COLOR_YELLOW "Unclosed block marker\n");
#endif
				break;
			}

			Block *block = blocks + numBlocks++;
			block->blockHeaderTitle = text + startHeaderTitle;
			block->blockHeaderTitleLength = endHeaderTitle - startHeaderTitle;
			block->blockHeaderText = text + endHeaderTitle + 1;
			block->blockHeaderTextLength = endHeaderText - endHeaderTitle - 1;
			block->blockText = text + endHeaderText + 3;
			block->blockTextLength = 0;
			block->blockTextFirstLine = line;

			if ( prevBlock )
			{
				prevBlock->blockTextLength = (text + i) - prevBlock->blockText;
			}
			prevBlock = block;

			i = endHeaderText + 3;
			continue;
		}
		else if ( text[i] == '\n' )
		{
			line++;
		}

		++i;
	}

	if ( prevBlock )
	{
		prevBlock->blockTextLength = (text + i) - prevBlock->blockText;
	}

	GPUProgramDesc theProgram = {};
	theProgram.numShaders = 2;

	Block *vertexBlock = FindBlock("Vertex", blocks, numBlocks);
	Block *fragmentBlock = FindBlock("Fragment", blocks, numBlocks);

	theProgram.shaders = ojkAllocArray<GPUShaderDesc>(allocator, theProgram.numShaders);

	char *vertexSource = ojkAllocString(allocator, vertexBlock->blockTextLength);
	char *fragmentSource = ojkAllocString(allocator, fragmentBlock->blockTextLength);

	strncpy_s(vertexSource, vertexBlock->blockTextLength + 1,
		vertexBlock->blockText, vertexBlock->blockTextLength);
	strncpy_s(fragmentSource, fragmentBlock->blockTextLength + 1,
		fragmentBlock->blockText, fragmentBlock->blockTextLength);

	theProgram.shaders[0].type      = GPUSHADER_VERTEX;
	theProgram.shaders[0].source    = vertexSource;
	theProgram.shaders[0].firstLine = vertexBlock->blockTextFirstLine;

	theProgram.shaders[1].type      = GPUSHADER_FRAGMENT;
	theProgram.shaders[1].source    = fragmentSource;
	theProgram.shaders[1].firstLine = fragmentBlock->blockTextFirstLine;

	Block *geometryBlock = FindBlock("Geometry", blocks, numBlocks);
	if ( geometryBlock )
	{
		theProgram.shaders[2].type      = GPUSHADER_FRAGMENT;
		theProgram.shaders[2].source    = fragmentSource;
		theProgram.shaders[2].firstLine = fragmentBlock->blockTextFirstLine;
	}

	return theProgram;
}
