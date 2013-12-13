#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

typedef std::vector<std::string> string_list;

bool should_be_escaped ( char c )
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

std::string& escape_string ( std::string& s )
{
	int escapable_characters = std::count_if (s.begin(), s.end(), should_be_escaped);
	if ( escapable_characters == 0 )
	{
		return s;
	}

	if ( s.capacity() < s.length() + escapable_characters )
	{
		// Grow if necessary.
		s.resize (s.length() + escapable_characters);
	}

	std::string::iterator it = s.begin();
	while ( it != s.end() )
	{
		char c = *it;
		if ( should_be_escaped (c) )
		{
			std::cout << "escaped something\n";
			it = s.insert (it, '\\');
			it += 2;
		}
		else
		{
			++it;
		}
	}

	return s;
}

bool ends_with ( const std::string& s, const std::string& suffix )
{
	return s.compare (s.length() - suffix.length(), suffix.length(), suffix) == 0;
}

int main ( int argc, char *argv[] )
{
	string_list args (argv, argv + argc);

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
	string_list glslFiles (args.begin() + 2, args.end());

	std::cout << "Outputting to " << outFile << '\n';

	std::string output = "#include \"tr_local.h\"\n\n";

	std::string line;
	for ( string_list::const_iterator it = glslFiles.begin();
			it != glslFiles.end(); ++it )
	{
		// Get shader name from file name
		if ( !ends_with (*it, ".glsl") )
		{
			std::cerr << *it << " doesn't end with .glsl extension.\n";
			continue;
		}

		std::string::size_type lastSlash = it->find_last_of ("\\/");
		std::string shaderName (it->begin() + lastSlash + 1, it->end() - 5);

		// Write, one line at a time to the output
		std::ifstream fs (it->c_str(), std::ios::in);
		if ( !fs )
		{
			std::cerr << *it << " could not be opened.\n";
			continue;
		}

		output += "const char *fallbackShader_" + shaderName + " = \"";
		while ( std::getline (fs, line) )
		{
			if ( line.empty() )
			{
				continue;
			}

			output += escape_string (line) + "\\n\\\n";
		}
		output += "\";\n\n";
	}

	std::ofstream ofs (outFile, std::ios::out);
	if ( !ofs )
	{
		std::cerr << "Could not create file " << outFile << '\n';
	}

	ofs << output;
}