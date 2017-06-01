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
		std::cerr << "Determines the directory to place an executable's debug database in on a symbol server.\nUsage: " << argv[ 0 ] << " <filename.exe|filename.pdb>" << std::endl;
		return EXIT_FAILURE;
	}
	const std::string filename = argv[ 1 ];
	SYMSRV_INDEX_INFO pdbInfo;
	pdbInfo.sizeofstruct = sizeof( pdbInfo );
	if( !SymSrvGetFileIndexInfo( filename.c_str(), &pdbInfo, 0 ) )
	{
		std::cerr << "SymSrvGetFileIndexInfo() failed!" << std::endl;
		return EXIT_FAILURE;
	}
	std::cout
		<< std::uppercase << std::hex << std::setfill( '0' ) << std::setw( 8 ) << pdbInfo.guid.Data1
		<< std::uppercase << std::hex << std::setfill( '0' ) << std::setw( 4 ) << pdbInfo.guid.Data2
		<< std::uppercase << std::hex << std::setfill( '0' ) << std::setw( 4 ) << pdbInfo.guid.Data3
		;
	for( int i = 0; i < 8; ++i )
	{
		std::cout << std::uppercase << std::hex << std::setfill( '0' ) << std::setw( 2 ) << static_cast< unsigned int >( pdbInfo.guid.Data4[ i ] );
	}
	std::cout << pdbInfo.age << std::endl;
	return EXIT_SUCCESS;
}
