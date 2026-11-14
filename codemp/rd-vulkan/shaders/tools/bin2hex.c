#include <stdio.h>
#include <stdlib.h>

int main( int argc, const char* argv[] ) {
	const int line_length = 16;
	int size = 0;
	char buf[256];
	FILE *f_in;
	long in_len;
	FILE *f_out;
	int c, n;

	if ( argc != 4 ) {
		printf("Usage: %s <input_file> [+]<output_file> <output_array_name>\n", argv[0] );
		return - 1;
	}

	if ( strlen( argv[3] ) > sizeof( buf ) - 64 ) {
		printf( "Array name is too long: %s\n", argv[3] );
		return -1;
	}

	f_in = fopen( argv[1], "rb" );

	if ( !f_in ) {
		printf( "Could not open file for reading: %s\n", argv[1] );
		return -1;
	}

	if ( fseek( f_in, 0, SEEK_END ) ) {
		printf( "Seek error for: %s\n", argv[1] );
		fclose( f_in );
		return -1;
	}

	in_len = ftell( f_in );

#if 0
	if ( in_len & 3 ) {
		printf( "Incorrect size %li: %s\n", in_len, argv[1] );
		fclose( f_in );
		return -1;
	}
#endif

	if ( fseek( f_in, 0, SEEK_SET ) ) {
		printf( "Seek error for: %s\n", argv[1] );
		fclose( f_in );
		return -1;
	}

	if ( argv[2][0] == '+' ) // append mode
		f_out = fopen( argv[2]+1, "ab" );
	else
		f_out = fopen( argv[2], "wb" );

	if ( !f_out ) {
		printf( "Could not open file for writing: %s\n", argv[2] );
		fclose( f_in );
		return -1;
	}

	n = sprintf( buf, "const unsigned char %s[%li] = {\n\t", argv[3], in_len );

	fwrite( buf, n, 1, f_out );

	c = fgetc( f_in );

	while ( c != EOF ) {

		n = sprintf( buf, "0x%.2X", c );
		fwrite( buf, n, 1, f_out );

		c = fgetc( f_in );

		size++;

		if ( c != EOF ) {
			if ( size % line_length ) 
				fputs( ", ", f_out );
			else
				fputs( ",\n\t", f_out );
		} else {
			break;
		}
	}

	fputs( "\n};\n", f_out );

#if 0
	n = sprintf( buf, "const int %s_size = %i;\n", argv[3], size );
	fwrite( buf, n, 1, f_out );
#endif

	if ( !feof( f_in ) ) {
		printf( "Could not read entire file: %s", argv[1] );
	}

	fflush( f_out );
	fclose( f_out );
	fclose( f_in );

	return 0;
}

