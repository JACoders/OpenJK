#include <stdio.h>
#include <stdlib.h>

int main( int argc, const char* argv[] ) {
	char buf[256];
	FILE *f_out;
	int n;

	if ( argc != 3 ) {
		printf("Usage: %s %d <output_file> <chars>\n", argv[0], argc );
		return - 1;
	}

	//printf("args %s %s %s\n", argv[0], argv[1], argv[2] );

	if ( argv[1][0] == '+' ) // append mode
		f_out = fopen( argv[1]+1, "ab" );
	else
		f_out = fopen( argv[1], "wb" );

	if ( !f_out ) {
		printf( "Could not open file for writing: %s\n", argv[1] );
		return -1;
	}

	n = sprintf( buf, "%s\n", argv[2] );
	fwrite( buf, n, 1, f_out );


	fflush( f_out );
	fclose( f_out );

	return 0;
}

