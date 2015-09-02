#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <iomanip>

using namespace std::string_literals;

int main( int argc, char** argv )
{
	if( argc != 2 || argv[ 1 ] == "--help"s || argv[ 1 ] == "-h"s )
	{
		std::cerr << "Determines the directory to place an executable in on a symbol server.\nUsage: "s << argv[ 0 ] << " <filename.exe>"s << std::endl;
		return EXIT_FAILURE;
	}
	const std::string filename = argv[ 1 ];
	GUID id;
	DWORD val1;
	DWORD val2;
	// Retrieves the indexes for the specified .pdb, .dbg, or image file that would be used to store the file. The combination of these values uniquely identifies the file in the symbol server.
	if( !SymSrvGetFileIndexes( filename.c_str(), &id, &val1, &val2, 0 ) )
	{
		std::cerr << "SymSrvGetFileIndexes() failed!" << std::endl;
		return EXIT_FAILURE;
	}
	if( false )
	{
		std::cout << std::uppercase << std::hex << id.Data1 << " - " << id.Data2 << " - " << id.Data3 << " -";
		for( unsigned char c : id.Data4 )
		{
			std::cout << " " << std::hex << static_cast< unsigned int >( c );
		}
		std::cout << "; " << val1 << "; " << val2 << std::endl;
	}
	DWORD timeDateStamp = id.Data1;
	DWORD sizeOfImage = val1;
	std::cout << std::uppercase << std::hex << timeDateStamp << sizeOfImage << std::endl;
	return EXIT_SUCCESS;
}
