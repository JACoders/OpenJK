// ICARUS: IBIze Interpreter
//
//	-- jweier

#ifdef _WIN32
#include <conio.h>	//For getch()
#include <io.h>		//For _findXXX
#else
#include "ibize_platform.h"
#endif

#include "Tokenizer.h"
#include "BlockStream.h"
#include "Interpreter.h"

#define IBIZE_VERSION		1.6f
#define SCRIPT_EXTENSION	".ICR"

CInterpreter	Interpreter;
CTokenizer		Tokenizer;
CBlockStream	BlockStream;

/*
-------------------------
NULL_Error
-------------------------
*/

void NULL_Error( LPCSTR *error_msg )
{
}

/*
-------------------------
InterpretFile
-------------------------
*/

// note different return type now, rather than true/false bool it returns CBlock# +1 of the bad command (if any),
//	else 0 for all ok. Also returns -ve block numbers to indicate last block that was ok if an error occured between
//	blocks (eg unexpected float at command identifier position)
//
int InterpretFile( char *filename )
{
	printf("Parsing '%s'...\n\n", filename);

	//Create a block stream
	if ( BlockStream.Create( filename ) == false )
	{
		printf("ERROR: Unable to create file \"%s\"!\n", filename );
		return 1;
	}
	
	//Create the Interpreted Block Instruction file
	//
	// (if error, return)
	//
	int iErrorBlock = Interpreter.Interpret( &Tokenizer, &BlockStream, filename );
	if (iErrorBlock!=0)
	{
		return iErrorBlock;
	}
			
	BlockStream.Free();

	return iErrorBlock;	//true;
}

/*
-------------------------
main
-------------------------
*/

int main(int argc, char* argv[])
{
#ifdef _WIN32
	struct _finddata_t fileinfo;
	bool	error_pause = false;
#endif
	char	*filename, error_msg[MAX_STRING_LENGTH], newfilename[MAX_FILENAME_LENGTH];
	int		handle;

	//Init the tokenizer and pass the symbols we require
	Tokenizer.Create( 0 );
	Tokenizer.SetSymbols( (keywordArray_t *) Interpreter.GetSymbols() );
	Tokenizer.SetErrorProc( (void (__cdecl *)(const char *)) NULL_Error );

	//Init the block streaming class
	BlockStream.Init();

	//No script arguments, print the banner
	if (argc < 2)
	{
		printf("\n\nIBIze v%1.2f -- jweier\n", IBIZE_VERSION );
		printf("ICARUS v%1.2f\n", ICARUS_VERSION );
		printf("Copyright (c) 1999, Raven Software\n");
		printf("------------------------------\n");
		printf("\nIBIze [script1.txt] [script2.txt] [script3.txt] ...\n\n");
		
		return 0;
	}

	int iErrorBlock = 0;

	//Interpret all files passed on the command line
	for (int i=1; i<argc; i++)
	{
		filename = (char *) argv[i];

		//FIXME: There could be better ways to do this...
		if ( filename[0] == '-' )
		{
#ifdef _WIN32
			if ( tolower(filename[1]) == 'e' )
				error_pause = true;
 //Can just use a wildcard in the commandline on non-windows
			if ( tolower(filename[1]) == 'a' )
			{
				handle = _findfirst ( "*.txt", &fileinfo);

				while ( handle != -1 )
				{
					if (Tokenizer.AddParseFile( (char*) &fileinfo.name ))
					{
						//Interpret the file
						if ( (iErrorBlock=InterpretFile( (char *) &fileinfo.name )) !=0 )
						{
							// failed
							//
							BlockStream.Free();

							if (error_pause)
								getch();
						}
					}

					if ( _findnext( handle, &fileinfo ) == -1 ) 
						break;
				}

				_findclose (handle);
			}
#endif
			
			continue;
		}

		//Tokenize the file
		if (Tokenizer.AddParseFile( filename ))
		{
			//Interpret the file
			if ( (iErrorBlock=InterpretFile( filename )) !=0 )
			{
				// failed
				//
				BlockStream.Free();

#ifdef _WIN32
				if (error_pause)
					getch();
#endif

				return iErrorBlock;
			}
		}
		else
		{
			//Try adding on the SCR extension if it was left off
			strcpy((char *) &newfilename, filename);
			strcat((char *) &newfilename, SCRIPT_EXTENSION);

			if (Tokenizer.AddParseFile( (char*) &newfilename ))
			{
				//Interpret the file
				if ( (iErrorBlock=InterpretFile( (char *) &newfilename )) !=0 )
				{
					// failed
					//
					BlockStream.Free();

#ifdef _WIN32
                    if (error_pause)
                        getch();
#endif
					return iErrorBlock;
				}
			}
			else
			{
				//File wasn't found
				sprintf(error_msg, "ERROR: File '%s' not found!\n", filename);
				printf(error_msg);
				
#ifdef _WIN32
				if (error_pause)
					getch();
#endif

				return 1;	// this will technically mean that there was a problem with cblock 1, but wtf?
			}
		}
	}

	printf("Done\n\n");

	return 0;
}

