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

// [M] strncpy_s is not present on linux and VS only function
#if !defined(_WIN32)
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

	static const char *shaderBlockNames[GPUSHADER_TYPE_COUNT] = {
		"Vertex", "Fragment", "Geometry"
	};

	GPUProgramDesc theProgram = {};
	const Block *parsedBlocks[GPUSHADER_TYPE_COUNT] = {};
	for ( const auto& shaderBlockName : shaderBlockNames )
	{
		Block *block = FindBlock(shaderBlockName, blocks, numBlocks);
		if ( block )
		{
			parsedBlocks[theProgram.numShaders++] = block;
		}
	}

	theProgram.shaders = ojkAllocArray<GPUShaderDesc>(allocator, theProgram.numShaders);

	int shaderIndex = 0;
	for ( int shaderType = 0;
			shaderType < theProgram.numShaders;
			++shaderType )
	{
		const Block *block = parsedBlocks[shaderType];
		if ( !block )
		{
			continue;
		}

		char *source = ojkAllocString(allocator, block->blockTextLength);

		strncpy_s(
			source,
			block->blockTextLength + 1,
			block->blockText,
			block->blockTextLength);

		GPUShaderDesc& shaderDesc = theProgram.shaders[shaderIndex];
		shaderDesc.type = static_cast<GPUShaderType>(shaderType);
		shaderDesc.source = source;
		shaderDesc.firstLineNumber = block->blockTextFirstLine;
		++shaderIndex;
	}

	return theProgram;
}
