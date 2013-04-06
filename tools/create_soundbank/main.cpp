#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <direct.h>
#include <io.h>
#include <windows.h>

#include "zlib/zlib.h"

static int count = 0;
static int basesize	= 0;

//Changing this?  It needs to be synced with the snd_local_console.h.
enum SoundFilenameFlags
{
	SFF_WEAPONS_ATST,
	SFF_SAND_CREATURE,
	SFF_HOWLER,
	SFF_ALTCHARGE,
	SFF_FALLING,
	SFF_TIEEXPLODE
	//Can't have more than 8.
};
//Changing this?  It needs to be synced with the snd_local_console.h.

FILE*	out;
FILE*	out2;
FILE*   out3;

void processfile( const char* path, const char* filename)
{
	FILE*	in;
	char	name[256];

	// open the file and write it to the massive bank


	strcpy(name, path);
	strcat(name,"\\");
	strcat(name, filename);

	in	= fopen(name, "rb");
	
	if(!in)
	{
		printf("Error, could not open file, %s, for reading.\n", name);
		return;
	}

	fseek(in, 0, SEEK_END);
	int len	= ftell(in);
	fseek(in, 0, SEEK_SET);

	void*	crap	= malloc(len);

	fread(crap, len, 1, in);

	fclose(in);

	unsigned int offset	= ftell(out);
	//fwrite(&len, sizeof(len), 1, out);
	fwrite(crap, len, 1, out);

	free(crap);

	char osname[256];

	strcpy(osname, "d:\\base");
	strcat(osname, name + basesize);

	unsigned int filecode	= crc32(0, (const byte *)osname, strlen(osname));

	char qname[64];
	strcpy(qname, name + basesize + 1);

	fwrite(&filecode, sizeof(filecode), 1, out2);
	fwrite(&offset, sizeof(offset), 1, out2);
	fwrite(&len, sizeof(len), 1, out2);
	
	unsigned char flags = 0;
	if(strstr(qname, "weapons\\atst")) {
		flags |= (1 << SFF_WEAPONS_ATST);
	}
	if(strstr(qname, "sand_creature")) {
		flags |= (1 << SFF_SAND_CREATURE);
	}
	if(strstr(qname, "howler")) {
		flags |= (1 << SFF_HOWLER);
	}
	if(strstr(qname, "altcharge")) {
		flags |= (1 << SFF_ALTCHARGE);
	}
	if(strstr(qname, "falling")) {
		flags |= (1 << SFF_FALLING);
	}
	if(strstr(qname, "tieexplode")) {
		flags |= (1 << SFF_TIEEXPLODE);
	}
	fwrite(&flags, sizeof(flags), 1, out2);

	fprintf(out3, "%u\t\t%s\n", filecode, qname);
}

void process( const char* path)
{
	char spec[256];
	strcpy(spec, path);
	strcat(spec, "\\*.*");

	_finddata_t data;
	int h = _findfirst(spec, &data);
	while( h != -1)
	{	
		if(data.attrib & _A_SUBDIR && strcmp(data.name,".") && strcmp(data.name,"..") )
		{
			char sub[256];
			strcpy(sub,path);
			strcat(sub,"\\");
			strcat(sub,data.name);
			process(sub);
		}
		else if(strstr(data.name,".wxb"))
		{
			processfile(path, data.name);
			count++;
		}

		if (_findnext(h, &data)) break;
	}
	_findclose(h);
}

int main(int argc, const char** argv)
{
	// open a file to become the massive soundbank
	out	= fopen("sound.bnk", "wb");
	out2	= fopen("sound.tbl", "wb");
	out3 = fopen("crclookup.txt", "w");

	if(!out)
	{
		printf("Error, could not open sound.bnk for writing.\n");
		exit(0);
	}

	if(!out2)
	{
		printf("Error, could not open sound.tbl for writing.\n");
		exit(0);
	}

	if(!out3)
	{
		printf("Error, could not open crclookup.txt for writing.\n");
		exit(0);
	}


	basesize	= strlen(argv[1]);

	// find all WXP files in the path
	printf("Processing...\n");
	process(argv[1]);
	fclose(out);
	fclose(out2);
	fclose(out3);
	printf("%d files processed.\n", count);

	return 1;
}