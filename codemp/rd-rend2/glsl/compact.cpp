#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "tr_local.h"
#include "tr_allocator.h"

namespace
{

using StringList = std::vector<std::string>;

bool ShouldEscape( char c )
{
	switch ( c )
	{
	case '\\':
	case '"':
		return true;
	default:
		return false;
	}
}

std::string& Escape( std::string& s )
{
	std::string::difference_type escapableCharacters = std::count_if( s.begin(), s.end(), ShouldEscape );
	if ( escapableCharacters == 0 )
	{
		return s;
	}

	if ( s.capacity() < (s.length() + escapableCharacters) )
	{
		// Grow if necessary.
		s.resize(s.length() + escapableCharacters);
	}

	std::string::iterator it = s.begin();
	while ( it != s.end() )
	{
		char c = *it;
		if ( ShouldEscape(c) )
		{
			it = s.insert(it, '\\');
			it += 2;
		}
		else
		{
			++it;
		}
	}

	return s;
}

bool EndsWith( const std::string& s, const std::string& suffix )
{
	return s.compare(s.length() - suffix.length(), suffix.length(), suffix) == 0;
}

const char *GetShaderSuffix( GPUShaderType type )
{
	switch ( type )
	{
		case GPUSHADER_VERTEX:   return "_vp";
		case GPUSHADER_FRAGMENT: return "_fp";
		case GPUSHADER_GEOMETRY: return "_gp";
		default: assert(!"Invalid shader type");
	}
	return nullptr;
}

const char *ToString( GPUShaderType type )
{
	switch ( type )
	{
		case GPUSHADER_VERTEX:   return "GPUSHADER_VERTEX";
		case GPUSHADER_FRAGMENT: return "GPUSHADER_FRAGMENT";
		case GPUSHADER_GEOMETRY: return "GPUSHADER_GEOMETRY";
		default: assert(!"Invalid shader type");
	}
	return nullptr;
}

} // anonymous namespace

int main( int argc, char *argv[] )
{
	StringList args(argv, argv + argc);

	if ( args.empty() )
	{
		std::cerr << "No GLSL files were given.\n";
		return EXIT_FAILURE;
	}

	if ( args.size() < 3 )
	{
		// 0 = exe, 1 = outfile, 2+ = glsl files
		return EXIT_FAILURE;
	}

	std::string& outFile = args[1];
	StringList glslFiles(args.begin() + 2, args.end());

	std::cout << "Outputting to " << outFile << '\n';

	Allocator allocator(512 * 1024);

	std::ostringstream ss;
	std::string line;

	ss << "#include \"tr_local.h\"\n\n";
	for ( StringList::const_iterator it = glslFiles.begin();
			it != glslFiles.end(); ++it )
	{

		// Get shader name from file name
		if ( !EndsWith(*it, ".glsl") )
		{
			std::cerr << *it << " doesn't end with .glsl extension.\n";
			continue;
		}

		std::string::size_type lastSlash = it->find_last_of("\\/");
		std::string shaderName(it->begin() + lastSlash + 1, it->end() - 5);

		// Write, one line at a time to the output
		std::ifstream fs(it->c_str());
		if ( !fs )
		{
			std::cerr << *it << " could not be opened.\n";
			continue;
		}

		std::streampos fileSize;
		fs.seekg(0, std::ios::end);
		fileSize = fs.tellg();
		fs.seekg(0, std::ios::beg);

		allocator.Reset();

		char *programText = ojkAllocString(allocator, fileSize);
		memset(programText, 0, (size_t)fileSize + 1);
		fs.read(programText, fileSize);

		GPUProgramDesc programDesc = ParseProgramSource(allocator,  programText);
		for ( size_t i = 0, numShaders = programDesc.numShaders; i < numShaders; ++i )
		{
			GPUShaderDesc& shaderDesc = programDesc.shaders[i];
			const char *suffix = GetShaderSuffix(shaderDesc.type);

			ss << "const char *fallback_" + shaderName + suffix + " = \"";

			const char *lineStart = shaderDesc.source;
			const char *lineEnd = strchr(lineStart, '\n');
			while ( lineEnd )
			{
				line.assign(lineStart, lineEnd - lineStart);
				ss << Escape(line);
				ss << "\\n\"\n\"";

				lineStart = lineEnd + 1;
				lineEnd = strchr(lineStart, '\n');
			}

			line.assign(lineStart);
			ss << Escape(line) << "\";\n";
		}

		ss << "GPUShaderDesc fallback_" << shaderName << "Shaders[] = {\n";
		for ( size_t i = 0, numShaders = programDesc.numShaders; i < numShaders; ++i )
		{
			GPUShaderDesc& shaderDesc = programDesc.shaders[i];
			const char *suffix = GetShaderSuffix(shaderDesc.type);

			ss << "  { " << ToString(shaderDesc.type) << ", "
						"fallback_" << shaderName << suffix << ", "
						<< shaderDesc.firstLineNumber << " },\n";
		}
		ss << "};\n";

		ss << "extern const GPUProgramDesc fallback_" << shaderName << "Program = { "
			<< programDesc.numShaders << ", fallback_" << shaderName << "Shaders };\n\n";
	}

	std::ofstream ofs(outFile.c_str());
	if ( !ofs )
	{
		std::cerr << "Could not create file " << outFile << '\n';
	}

	ofs << ss.str();
}
