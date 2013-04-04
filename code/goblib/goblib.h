/*****************************************
 *
 * GOB File System
 *
 * Here's what Merriam-Webster says about "gob":  --Chuck
 *		Entry:     gob
 *		Function:  noun
 *		Etymology: Middle English gobbe, from Middle French gobe large piece of food,
 *		           back-formation from gobet
 *		Date:      14th century
 *		1 : LUMP
 *		2 : a large amount -- usually used in plural <gobs of money>
 *
 * Purpose: Provide fast, efficient disk access on a variety of platforms.
 *
 * Implementation:
 *		The GOB system maintains two files -- GOB and GFC.  The GOB file is actually
 *		an archive of many files split into variable size, compressed blocks.  The GFC,
 *		GOB File Control, contains 3 tables -- a block table, basic file table, and
 *		extended file table.  The block table is analogous to a DOS FAT.  The basic
 *		file table contains a minimal set of file information to handle basic reading
 *		tasks.  The extended file table is optionally loaded and contains additional
 *		file information.  File names are case insensitive.
 *		  
 *		Files can be read in a normal manner.  Open, read, seek and close
 *		operations are all provided.  Files can only be written in a single
 *		contiguous chunk of blocks at the end of an archive.  Reads are processed
 *		through a configurable number of read ahead buffers to in an effort to
 *		minimize both reads and seeks.  Other operations including delete, verify, 
 *		access, and get size are also supported on files inside an archive.
 *
 *		The system supports read profiling.  By supplying a file read callback 
 *		function, the library will output the block number of each read.  This can 
 *		be used rearrange block in the archive to minimize seek times.  The 
 *		GOBRearrange sorts files in an archive.
 *
 *		Supports block based caching.  Primarily aimed at caching files off a DVD/CD
 *		to a faster hard disk.
 *
 * Future Work: 
 *
 * Dependencies: vvInt, snprintf, zlib
 * Owner: Chris McEvoy
 * History:
 *     09/23/2001 Original version
 *     10/28/2002 Merged into vvtech
 *
 * Copyright (C) 2002, Vicarious Visions, Inc.  All Rights Reserved.
 *
 * UNPUBLISHED -- Rights  reserved  under  the  copyright  laws  of the 
 * United States.  Use  of a copyright notice is precautionary only and 
 * does not imply publication or disclosure.                            
 *                                                                      
 * THIS DOCUMENTATION CONTAINS CONFIDENTIAL AND PROPRIETARY INFORMATION 
 * OF    VICARIOUS   VISIONS,  INC.    ANY  DUPLICATION,  MODIFICATION, 
 * DISTRIBUTION, OR DISCLOSURE IS STRICTLY PROHIBITED WITHOUT THE PRIOR 
 * EXPRESS WRITTEN PERMISSION OF VICARIOUS VISIONS, INC.
 *
 *****************************************/

/*
	This is an unofficial branch of GOB, for Jedi Academy
	Maintainer: Brian Osman
*/

#ifndef GOBLIB_H__
#define GOBLIB_H__

#ifdef __cplusplus
extern "C" {
#endif

#define GOB_MAGIC_IDENTIFIER		0x8008
#define GOB_MAX_FILE_NAME_LEN		96
#define GOB_MAX_OPEN_FILES			16
#define GOB_MAX_CODECS				2
#define GOB_INFINITE_RATIO			1000
#define GOB_READ_RETRYS				3

#define GOB_MAX_FILES				(16*1024)
#define GOB_MAX_BLOCKS				32767

#define GOB_BLOCK_SIZE				(64*1024)
#define GOB_BLOCK_ALIGNMENT			2048
#define GOB_MEM_ALIGNMENT			64
#define GOB_COMPRESS_OVERHEAD		1024

#define GOB_INVALID_SIZE			0xFFFFFFFF
#define GOB_INVALID_BLOCK			0xFFFFFFFF

#define GOB_TRUE					1
#define GOB_FALSE					0

#define GOBERR_OK					0
#define GOBERR_NOT_INIT				1
#define GOBERR_FILE_NOT_FOUND		2
#define GOBERR_FILE_READ			3
#define GOBERR_FILE_WRITE			4
#define GOBERR_NO_MEMORY			5
#define GOBERR_ALREADY_INIT			6
#define GOBERR_ALREADY_OPEN			7
#define GOBERR_INVALID_ACCESS		8
#define GOBERR_NOT_GOB_FILE			9
#define GOBERR_NOT_OPEN				10
#define GOBERR_CANNOT_CREATE		11
#define GOBERR_TOO_MANY_OPEN		12
#define GOBERR_INVALID_SEEK			13
#define GOBERR_TOO_MANY_FILES		14
#define GOBERR_FILE_RENAME			15
#define GOBERR_PROFILE_OFF			16
#define GOBERR_PROFILE_ON			17
#define GOBERR_NO_EXTENDED			18
#define GOBERR_DUP_HASH				19
#define GOBERR_TOO_MANY_BLOCKS		20
#define GOBERR_COMPRESS_FAIL		21
#define GOBERR_NO_SUITABLE_CODEC	22

#define GOBACCESS_READ				0
#define GOBACCESS_WRITE				1
#define GOBACCESS_RW				2

#define GOBSEEK_START				0
#define GOBSEEK_CURRENT				1
#define GOBSEEK_END					2

#define GOB_CODEC_MASK(n)			((GOBUInt32)(1u<<(n)))
#define GOB_CODEC_MASK_ANY			((GOBUInt32)(-1))

#define GOBMARKER_STARTBLOCK		('L' | 'B' << 8 | 'T' << 16 | 'S' << 24)
#define GOBMARKER_ENDBLOCK			('L' | 'B' << 8 | 'N' << 16 | 'E' << 24)

typedef int						int32;
typedef unsigned int			uint32;
//#define bool					int
//#define false					0
//#define true					1
typedef unsigned long			ulong;
typedef unsigned char			byte;

typedef int32					GOBInt32;
typedef uint32					GOBUInt32;
typedef char					GOBChar;
typedef bool					GOBBool;
typedef int32					GOBError;
typedef int32					GOBSeekType;
typedef int32					GOBHandle;
typedef int32					GOBAccessType;
typedef void*					GOBFSHandle;
typedef void					GOBVoid;

typedef GOBFSHandle (*GOBFileSysOpenFunc)(GOBChar*, GOBAccessType);
typedef GOBBool (*GOBFileSysCloseFunc)(GOBFSHandle*);
typedef GOBInt32 (*GOBFileSysReadFunc)(GOBFSHandle, GOBVoid*, GOBInt32);
typedef GOBInt32 (*GOBFileSysWriteFunc)(GOBFSHandle, GOBVoid*, GOBInt32);
typedef GOBInt32 (*GOBFileSysSeekFunc)(GOBFSHandle, GOBInt32, GOBSeekType);
typedef GOBInt32 (*GOBFileSysRenameFunc)(GOBChar*, GOBChar*);

typedef GOBVoid* (*GOBMemAllocFunc)(GOBUInt32);
typedef GOBVoid (*GOBMemFreeFunc)(GOBVoid*);

typedef GOBInt32 (*GOBCompressFunc)(GOBVoid*, GOBUInt32, GOBVoid*, GOBUInt32*);
typedef GOBInt32 (*GOBDecompressFunc)(GOBVoid*, GOBUInt32, GOBVoid*, GOBUInt32*);

typedef GOBBool (*GOBCacheFileOpenFunc)(GOBUInt32);
typedef GOBBool (*GOBCacheFileCloseFunc)(GOBVoid);
typedef GOBInt32 (*GOBCacheFileReadFunc)(GOBVoid*, GOBInt32);
typedef GOBInt32 (*GOBCacheFileWriteFunc)(GOBVoid*, GOBInt32);
typedef GOBInt32 (*GOBCacheFileSeekFunc)(GOBInt32);

struct GOBBlockTableEntry
{
	GOBUInt32 size; // compressed size
	GOBUInt32 offset;
	GOBUInt32 next;
};

struct GOBFileTableBasicEntry
{
	GOBUInt32	hash;
	GOBUInt32	size; // decompressed size
	GOBUInt32	block;
};

struct GOBFileTableExtEntry
{
	GOBChar		name[GOB_MAX_FILE_NAME_LEN];
	GOBUInt32	crc;
	GOBUInt32	time;
};

struct GOBMemoryFuncSet
{
	GOBMemAllocFunc		alloc;
	GOBMemFreeFunc		free;
};

struct GOBSingleCodecDesc
{
	GOBChar				tag;
	GOBInt32			max_ratio;
	GOBCompressFunc		compress;
	GOBDecompressFunc	decompress;
};

struct GOBCodecFuncSet
{
	GOBInt32 codecs;
	struct GOBSingleCodecDesc codec[GOB_MAX_CODECS];
};

struct GOBFileSysFuncSet
{
	GOBFileSysOpenFunc		open;
	GOBFileSysCloseFunc		close;
	GOBFileSysReadFunc		read;
	GOBFileSysWriteFunc		write;
	GOBFileSysSeekFunc		seek;
};

struct GOBCacheFileFuncSet
{
	GOBCacheFileOpenFunc		open;
	GOBCacheFileCloseFunc		close;
	GOBCacheFileReadFunc		read;
	GOBCacheFileWriteFunc		write;
	GOBCacheFileSeekFunc		seek;
};

struct GOBReadStats
{
	GOBUInt32 bufferUsed;
	GOBUInt32 bytesRead;
	GOBUInt32 cacheBytesRead;
	GOBUInt32 cacheBytesWrite;
	GOBUInt32 totalSeeks;
	GOBUInt32 farSeeks;
	GOBUInt32 filesOpened;
};

extern GOBError GOBInit(struct GOBMemoryFuncSet* mem, 
	struct GOBFileSysFuncSet* file, 
	struct GOBCodecFuncSet* codec,
	struct GOBCacheFileFuncSet* cache);
extern GOBError GOBShutdown(GOBVoid);

extern GOBError GOBArchiveCreate(const GOBChar* file);
extern GOBError GOBArchiveOpen(const GOBChar* file, GOBAccessType atype,
	GOBBool extended, GOBBool safe);
extern GOBError GOBArchiveClose(GOBVoid);
extern GOBError GOBArchiveCheckMarkers(GOBVoid);

extern GOBError GOBOpen(GOBChar* file, GOBHandle* handle);
extern GOBError GOBOpenCode(GOBInt32 code, GOBHandle* handle);
extern GOBError GOBClose(GOBHandle handle);

extern GOBUInt32 GOBRead(GOBVoid* buffer, GOBUInt32 size, GOBHandle handle);
extern GOBError GOBSeek(GOBHandle handle, GOBUInt32 offset, GOBSeekType type, GOBUInt32* pos);

extern GOBError GOBWrite(GOBVoid* buffer, GOBUInt32 size, GOBUInt32 mtime, const GOBChar* file, GOBUInt32 codec_mask);
extern GOBError GOBDelete(const GOBChar* file);

extern GOBError GOBRearrange(const GOBChar* file, const GOBUInt32* xlat, GOBFileSysRenameFunc _rename);

extern GOBError GOBVerify(const GOBChar* file, GOBBool* status);

extern GOBError GOBGetSize(const GOBChar* file, GOBUInt32* decomp, GOBUInt32* comp, GOBUInt32* slack);
extern GOBError GOBGetTime(const GOBChar* file, GOBUInt32* time);
extern GOBError GOBGetCRC(const GOBChar* file, GOBUInt32* crc);

extern GOBError GOBAccess(const GOBChar* file, GOBBool* status);
extern GOBInt32 GOBGetFileCode(const GOBChar* file);

extern GOBError GOBGetFileTables(struct GOBFileTableBasicEntry** basic, struct GOBFileTableExtEntry** ext);
extern GOBError GOBGetBlockTable(struct GOBBlockTableEntry** table, GOBUInt32* num);
extern GOBUInt32 GOBGetSlack(GOBUInt32 x);

extern GOBError GOBSetCacheSize(GOBUInt32 num);
extern GOBError GOBSetReadBufferSize(GOBUInt32 size);

extern struct GOBReadStats GOBGetReadStats(GOBVoid);


typedef GOBVoid (*GOBProfileReadFunc)(GOBUInt32);
struct GOBProfileFuncSet
{
	GOBProfileReadFunc read;
};
extern GOBVoid GOBSetProfileFuncs(struct GOBProfileFuncSet* fset);

extern GOBError GOBStartProfile(GOBVoid);
extern GOBError GOBStopProfile(GOBVoid);


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* GOBLIB_H__ */
