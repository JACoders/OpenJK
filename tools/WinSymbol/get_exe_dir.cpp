#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#pragma warning(push)
#pragma warning(disable:4091)
#include <DbgHelp.h>
#pragma warning(pop)
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <iomanip>

int main( int argc, char** argv )
{
	if( argc != 2 || strcmp( argv[ 1 ], "--help" ) == 0 || strcmp( argv[ 1 ], "-h" ) == 0 )
	{
		std::cerr << "Determines the directory to place an executable in on a symbol server.\nUsage: " << argv[ 0 ] << " <filename.exe>" << std::endl;
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
