#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <direct.h>
#include <io.h>
#include <windows.h>

#define VV_CONSOLE

/*********
Structures
*********/
typedef struct
{
	int			format;
	int			rate;
	int			width;
	int			channels;
	int			samples;
	int			dataofs;		// chunk starts this many bytes from file start
} wavinfo_t;


/**********
Prototypes
**********/
static void FindNextChunk(char *name);
static void FindChunk(char *name);
static wavinfo_t GetWavinfo (const char *name, byte *wav, int wavlength);
static byte* Process(const char* name);
static byte* ProcessData(byte* buffer, wavinfo_t* info);
static int GetLittleLong(void);
static short GetLittleShort(void);
static void PrintHeader(wavinfo_t *info);
static void StripExtension( const char *in, char *out );
static void WriteDataToFile(char *name);

/**********
Global Variables
**********/
static	byte	*data_p;
static	byte 	*iff_end;
static	byte 	*last_chunk;
static	byte 	*iff_data;
static	int 	iff_chunk_len;

static	float	cutoff1		= 0.5f;		// cutoffs
static	float	cutoff2		= 4.0f;
static	float	cutoff3		= 7.0f;
static	float	cutoff4		= 8.0f;

static	float	volRange	= 0.0f;		// volume range

static	int		lipcount	= 0;			// number of bytes in lipdata
static	int		ns			= 0;			// number of total samples

/**********
StripExtension
**********/
static void StripExtension( const char *in, char *out )
{
	while ( *in && *in != '.' )
	{
		*out++ = *in++;
	}
	*out = 0;
}

/**********
GetLittleShort
**********/
static short GetLittleShort(void)
{
	short val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	data_p += 2;
	return val;
}

/**********
GetLittleLong
**********/
static int GetLittleLong(void)
{
	int val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	val = val + (*(data_p+2)<<16);
	val = val + (*(data_p+3)<<24);
	data_p += 4;
	return val;
}

/*********
FindNextChunk
*********/
static void FindNextChunk(char *name)
{
	while (1)
	{
		data_p=last_chunk;

		if (data_p >= iff_end)
		{	// didn't find the chunk
			data_p = NULL;
			return;
		}
		
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if (iff_chunk_len < 0)
		{
			data_p = NULL;
			return;
		}
		data_p -= 8;
		last_chunk = data_p + 8 + ( (iff_chunk_len + 1) & ~1 );
		if (!strncmp((char *)data_p, name, 4))
			return;
	}
}

/*********
FindChunk
*********/
static void FindChunk(char *name)
{
	last_chunk = iff_data;
	FindNextChunk (name);
}

/*********
PrintHeader
*********/
static void PrintHeader(wavinfo_t *info)
{
	printf("***** Begin Header Information *****\n");
	printf("Format:   %d\n",info->format);
	printf("Rate:     %d\n",info->rate);
	printf("Width:    %d\n",info->width);
	printf("Channels: %d\n",info->channels);
	printf("Samples:  %d\n",info->samples);
	printf("Dataofs:  %d\n",info->dataofs);
	printf("***** End Header Information *****\n");
}

/*********
GetWavinfo
*********/
static wavinfo_t GetWavinfo (const char *name, byte *wav, int wavlength)
{
	wavinfo_t	info;

	memset (&info, 0, sizeof(info));

	if (!wav)
		return info;
		
	iff_data = wav;
	iff_end = wav + wavlength;

// find "RIFF" chunk
	FindChunk("RIFF");
	if (!(data_p && !strncmp((char *)data_p+8, "WAVE", 4)))
	{
		printf("Missing RIFF/WAVE chunks\n");
		return info;
	}

// get "fmt " chunk
	iff_data = data_p + 12;
// DumpChunks ();

	FindChunk("fmt ");
	if (!data_p)
	{
		printf("Missing fmt chunk\n");
		return info;
	}
	data_p += 8;
	info.format = GetLittleShort();
	info.channels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4+2;
	info.width = GetLittleShort() / 8;

	if (info.format != 1)
	{
		printf("Microsoft PCM format only\n");
		return info;
	}


// find data chunk
	FindChunk("data");
	if (!data_p)
	{
		printf("Missing data chunk\n");
		return info;
	}

	data_p += 4;
	info.samples = GetLittleLong () / info.width;
	info.dataofs = data_p - wav;

	return info;
}

/********
ProcessData
********/
static byte* ProcessData(byte* buffer, wavinfo_t* info)
{
	int		i,j,k;
	byte*	result;
	byte	lip;
	short*	ptr;
	int		sample;
	int		sampleTotal = 0;

	lipcount	= (info->samples / 500 / 2) + 1;
	ns			= info->samples;

	// allocate memory for the resulting data
	result	= (byte*)malloc(lipcount);

	// fill the resulting array
	memset(result,0,lipcount);

	// set up our data ptr
	ptr	= (short*)buffer;
	j = k = 0;

	// loop through all the samples and find our maxvalue;
	for(i = 0; i < info->samples; i++)
	{
		// get the data for this sample
		sample = *(ptr + i);

		if (sample < 0)
			sample = -sample;
		if (volRange < (sample >> 8) )
		{
			volRange =  sample >> 8;
		}
	}

	// loop through samples
	for(i = 0; i < info->samples; i += 100)
	{
		// get the data for this sample
		sample = *(ptr + i);

		sample = sample >> 8;

		sampleTotal += sample * sample;
		if (((i + 100) % 500) == 0)
		{
			sampleTotal /= 5;

			// check the sample against the threshold values

			if (sampleTotal < volRange *  cutoff1)
				lip = 0x00;
			else if (sampleTotal < volRange * cutoff2)
				lip = 0x01;
			else if (sampleTotal < volRange * cutoff3)
				lip = 0x02;
			else if (sampleTotal < volRange * cutoff4)
				lip = 0x03;
			else
				lip = 0x04;

			// if j is a mult of 2, write to the first 4 bits
			// of the result byte, otherwize write to the second
			// bits of the result byte and increment j
			if(k%2 == 0)
			{
				result[j] |= lip << 4;

			}
			else
			{
				result[j] |= lip;
				j++;
			}
			k++;
		}
	}

	return result;
}


/********
Process
********/
static byte* Process(const char* name)
{
	// open the wav
	FILE* in = fopen(name, "rb");
	
	// make sure it was opened ok
	if (!in)
	{
		fprintf(stderr, "Unable to open %s\n", name);
		exit(-1);
	}

	printf("Opened file %s\n",name);

	// local variables
	long		lSize;
	byte*		buffer;
	byte*		lipData;
	wavinfo_t	info;
	
	// obtain file size.
	fseek (in , 0 , SEEK_END);
	lSize = ftell (in);
	rewind (in);
	
	// allocate memory to contain the whole file.
	buffer = (byte*) malloc (lSize);
	if (buffer == NULL) exit (2);
	
	// copy the file into the buffer.
	fread (buffer, 1, lSize, in);

	// get the header information
	info = GetWavinfo (name, buffer, lSize);

	// print out the head information
	PrintHeader(&info);

	// process the data
	if( info.width == 2 &&
		info.channels == 1 )
	{
		lipData = ProcessData(buffer, &info);
	}
	else
	{
		printf("Unsupported wav file.\n");
		lipData = NULL;
	}

	// clear up the memory for the buffer
	free(buffer);

	// close the file
	fclose(in);

	printf("Closed file %s\n\n",name);

	return lipData;
}

/*********
WriteDataToFile
*********/
static void WriteDataToFile(byte* data, const char* infile)
{
	FILE*	out;
	char*	outfile;

	// create an output file based on the infile
	outfile = (char*)malloc(strlen(infile)+1);
	StripExtension(infile,outfile);
	strcat(outfile,".lip");

	// open the outfile
	out = fopen(outfile, "wb");
	
	// make sure it was opened ok
	if (!out)
	{
		fprintf(stderr, "Unable to open %s\n", outfile);
		exit(-1);
	}

	printf("Writing to file %s ....\n",outfile);

	// write out the number of samples
	fwrite(&ns,1,sizeof(int),out);

	// write out the data
	fwrite(data,1,lipcount,out);

	// close the file and free up data
	fclose(out);
	free(outfile);
	
}

int main(int argc, const char** argv)
{
	byte*	data;

	// check command line
	if (argc != 3)
	{
		fprintf(stderr, "USAGE: %s INPUT OUTPUT\n", argv[0]);
		return -1;
	}

	// process the file
	data = Process(argv[1]);

	// write out the data
	if(data)
	{
		WriteDataToFile(data, argv[2]);
		free(data);
	}

	return 0;
}