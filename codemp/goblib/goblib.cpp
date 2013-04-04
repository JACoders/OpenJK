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

#include "goblib.h"
#include "../zlib/zlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#if (VV_PLATFORM == VV_PLATFORM_WIN) || (VV_PLATFORM == VV_PLATFORM_XBOX)
#	define CDECL __cdecl
#else
#	define CDECL 
#endif

// Profiling data
static GOBProfileReadFunc ProfileReadCallback = NULL;
static GOBBool ProfileEnabled = GOB_FALSE;

// Indicates whether or not the library has been initialized
static GOBBool LibraryInit = GOB_FALSE;

// Callbacks for handling low-level compression/decompression
static struct GOBCodecFuncSet CodecFuncs;

// Callbacks for handling low-level memory alloc and free
static struct GOBMemoryFuncSet MemFuncs;

// Callbacks for handling low-level file access
static struct GOBFileSysFuncSet FSFuncs;

// Callbacks for handling block caching (ie Xbox temp space)
static struct GOBCacheFileFuncSet CacheFileFuncs;
static GOBBool CacheFileActive = GOB_FALSE;

// Name of the GFC file
static GOBChar ControlFileName[GOB_MAX_FILE_NAME_LEN];

// Handle to the GOB archive
static GOBFSHandle ArchiveHandle = (GOBFSHandle*)0xFFFFFFFF;

// Size of the active GOB archive
static GOBUInt32 ArchiveSize = 0;
static GOBUInt32 ArchiveNumBlocks = 0;
static GOBUInt32 ArchiveNumFiles = 0;

// Cached blocks
struct GOBBlockCache
{
	GOBChar* data;
	GOBUInt32 block;
	GOBUInt32 time;
	GOBUInt32 size;
};
static struct GOBBlockCache* CacheBlocks = NULL;
static GOBUInt32 NumCacheBlocks = 0;
static GOBUInt32 CacheBlockCounter = 0;

// Read ahead buffer
struct GOBReadBuffer
{
	GOBChar* data;
	GOBChar* dataStart;
	GOBUInt32 pos;
	GOBUInt32 size;
};
static struct GOBReadBuffer ReadBuffer;

// Decompression buffer
static GOBChar* DecompBuffer = NULL;

// Stats gathering
static struct GOBReadStats ReadStats;
static GOBUInt32 CurrentArchivePos = 0;

// File tables (from the GFC)
static struct GOBFileTableBasicEntry* FileTableBasic = NULL;
static struct GOBFileTableExtEntry* FileTableExt = NULL;

// Block tables (from the GFC)
static struct GOBBlockTableEntry* BlockTable = NULL;
static GOBUInt32* BlockCRC = NULL;
static GOBUInt32* CacheFileTable = NULL;

// Do the tables need to be written?
static GOBBool FileTableDirty = GOB_FALSE;

// Information about open files
struct OpenFileInfo
{
	GOBBool valid;
	GOBUInt32 startBlock;
	GOBUInt32 block;
	GOBUInt32 offset;

	GOBUInt32 pos;
	GOBUInt32 size;
};

// Open file table -- indices in this array are passed
// back to the caller as pseudo file handles.
static struct OpenFileInfo OpenFiles[GOB_MAX_OPEN_FILES];

// Converting text to lower case -- this isn't very
// clean.  A common buffer is used to store lower case
// text.  So its not thread safe... among other things. ;)
static GOBChar LowerCaseBuffer[GOB_MAX_FILE_NAME_LEN];
static GOBChar* LowerCase(const GOBChar* name)
{
	GOBInt32 i;
	for (i = 0; name[i]; ++i) {
		LowerCaseBuffer[i] = (GOBChar)tolower(name[i]);
	}
	LowerCaseBuffer[i] = 0;

	return LowerCaseBuffer;
}

// Checks if a file handle is invalid
static GOBBool InvalidHandle(GOBFSHandle h)
{
	return (GOBUInt32)h == 0xFFFFFFFF ? GOB_TRUE : GOB_FALSE;
}

// Endian conversion
#if VV_ENDIAN == VV_ENDIAN_LITTLE
static GOBUInt32 SwapBytes(GOBUInt32 x)
{
	return
		(x >> 24) |
		((x >> 8) & 0xFF00) |
		((x << 8) & 0xFF0000) |
		(x << 24);

}
#else
static GOBUInt32 SwapBytes(GOBUInt32 x)
{
	return x;
}
#endif


// Given a file name, get its index in the FileTable
static GOBInt32 GetFileTableEntry(const GOBChar* file)
{
	GOBUInt32 entry;
	GOBUInt32 hash;
	
	// hash the file name
	hash = crc32(0L, Z_NULL, 0);
	hash = crc32(hash, (const unsigned char*)file, strlen(file));

	// linear search for matching a matching hash
	for (entry = 0; entry < ArchiveNumFiles; ++entry) {
		if (FileTableBasic[entry].block != GOB_INVALID_BLOCK &&
			FileTableBasic[entry].hash == hash) 
		{
			return entry;
		}
	}

	return -1;
}

// Mark the contents of cache and read buffer invalid
static GOBVoid InvalidateCache(GOBVoid)
{
	GOBUInt32 i;
	for (i = 0; i < NumCacheBlocks; ++i) {
		CacheBlocks[i].block = 0xFFFFFFFF;
	}
	ReadBuffer.pos = 0xFFFFFFFF;
}

// Deallocate memory used by cache and read buffer
static GOBVoid FreeCache(GOBVoid)
{
	GOBUInt32 i;

	if (CacheBlocks) {
		for (i = 0; i < NumCacheBlocks; ++i) {
			if (CacheBlocks[i].data) MemFuncs.free(CacheBlocks[i].data);
			CacheBlocks[i].data = NULL;
		}

		MemFuncs.free(CacheBlocks);
		NumCacheBlocks = 0;
		CacheBlocks = NULL;
	}
}

// Write the file table to disk if the form of a GFC
static GOBError CommitFileTable(GOBVoid)
{
	GOBUInt32 num;
	struct GOBFileTableBasicEntry basic;
	struct GOBFileTableExtEntry ext;
	struct GOBBlockTableEntry block;
	
	// open the GFC
	GOBFSHandle handle = FSFuncs.open(ControlFileName, GOBACCESS_WRITE);
	if (InvalidHandle(handle)) return GOBERR_FILE_WRITE;

	// write the magic identifier
	num = SwapBytes(GOB_MAGIC_IDENTIFIER);
	if (!FSFuncs.write(handle, &num, sizeof(num))) return GOBERR_FILE_WRITE;

	// write the size of the GOB
	num = SwapBytes(ArchiveSize);
	if (!FSFuncs.write(handle, &num, sizeof(num))) return GOBERR_FILE_WRITE;

	// write number of blocks in archive
	num = SwapBytes(ArchiveNumBlocks);
	if (!FSFuncs.write(handle, &num, sizeof(num))) return GOBERR_FILE_WRITE;

	// write number of file in archive
	num = SwapBytes(ArchiveNumFiles);
	if (!FSFuncs.write(handle, &num, sizeof(num))) return GOBERR_FILE_WRITE;

	// write block table -- with endian conversion
	for (num = 0; num < ArchiveNumBlocks; ++num) {
		block.next = SwapBytes(BlockTable[num].next);
		block.offset = SwapBytes(BlockTable[num].offset);
		block.size = SwapBytes(BlockTable[num].size);

		if (!FSFuncs.write(handle, &block, sizeof(block))) return GOBERR_FILE_WRITE;
	}
	
	// write block CRCs -- with endian conversion
	for (num = 0; num < ArchiveNumBlocks; ++num) {
		BlockCRC[num] = SwapBytes(BlockCRC[num]);
		if (!FSFuncs.write(handle, &BlockCRC[num], sizeof(BlockCRC[num]))) {
			return GOBERR_FILE_WRITE;
		}
	}
	
	// write each basic table entry -- with endian conversion
	for (num = 0; num < ArchiveNumFiles; ++num) {
		basic.hash = SwapBytes(FileTableBasic[num].hash);
		basic.block = SwapBytes(FileTableBasic[num].block);
		basic.size = SwapBytes(FileTableBasic[num].size);

		if (!FSFuncs.write(handle, &basic, sizeof(basic))) return GOBERR_FILE_WRITE;
	}
	
	// write each extended table entry -- with endian conversion
	for (num = 0; num < ArchiveNumFiles; ++num) {
		strcpy(ext.name, FileTableExt[num].name);
		ext.crc = SwapBytes(FileTableExt[num].crc);
		ext.time = SwapBytes(FileTableExt[num].time);

		if (!FSFuncs.write(handle, &ext, sizeof(ext))) return GOBERR_FILE_WRITE;
	}

	// all done
	FSFuncs.close(&handle);
	FileTableDirty = GOB_FALSE;

	return GOBERR_OK;
}


static GOBVoid DeallocTables(GOBVoid)
{
	if (BlockTable) {
		// free the block table
		MemFuncs.free(BlockTable);
		BlockTable = NULL;
	}

	if (BlockCRC) {
		// free the block crc table
		MemFuncs.free(BlockCRC);
		BlockCRC = NULL;
	}

	if (CacheFileTable)
	{
		// free the block cache table
		MemFuncs.free(CacheFileTable);
		CacheFileTable = NULL;
	}

	if (FileTableBasic) {
		// free the basic file table
		MemFuncs.free(FileTableBasic);
		FileTableBasic = NULL;
	}

	if (FileTableExt) {
		// free the extended file table
		MemFuncs.free(FileTableExt);
		FileTableExt = NULL;
	}
}

static GOBError AllocTables(GOBUInt32 num_blocks, GOBUInt32 num_files, 
	GOBBool extended, GOBBool safe)
{
	GOBUInt32 num;

	// dump any old tables
	DeallocTables();
	
	// allocate the block table
	BlockTable = (struct GOBBlockTableEntry*)
		MemFuncs.alloc(num_blocks * sizeof(struct GOBBlockTableEntry));
	if (!BlockTable) return GOBERR_NO_MEMORY;

	if (safe) {
		// allocate the block crc table for verifying data validity
		BlockCRC = (GOBUInt32*)MemFuncs.alloc(num_blocks * sizeof(GOBUInt32));
		if (!BlockCRC) return GOBERR_NO_MEMORY;
	}
	else {
		BlockCRC = NULL;
	}

	if (CacheFileActive)
	{
		// allocate the block cache bitfield
		CacheFileTable = (GOBUInt32*)
			MemFuncs.alloc((num_blocks / 32 + 1) * 4);
		if (!CacheFileTable) return GOBERR_NO_MEMORY;
	}

	// allocate the basic file table
	FileTableBasic = (struct GOBFileTableBasicEntry*)
		MemFuncs.alloc(num_files * sizeof(struct GOBFileTableBasicEntry));
	if (!FileTableBasic) return GOBERR_NO_MEMORY;

	if (extended) {
		// allocate the extended file table
		FileTableExt = (struct GOBFileTableExtEntry*)
			MemFuncs.alloc(num_files * sizeof(struct GOBFileTableExtEntry));
		if (!FileTableExt) return GOBERR_NO_MEMORY;
	}
	else {
		FileTableExt = NULL;
	}

	// clear the tables
	for (num = 0; num < num_files; ++num) {
		FileTableBasic[num].block = GOB_INVALID_BLOCK;
		if (FileTableExt) FileTableExt[num].name[0] = 0;
	}

	for (num = 0; num < num_blocks; ++num) {
		BlockTable[num].next = GOB_INVALID_BLOCK;
		BlockTable[num].size = GOB_INVALID_SIZE;
	}

	return GOBERR_OK;
}


// GOBInit
// Public function.  Initialize the library.
GOBError GOBInit(struct GOBMemoryFuncSet* mem, 
	struct GOBFileSysFuncSet* file, 
	struct GOBCodecFuncSet* codec,
	struct GOBCacheFileFuncSet* cache)
{
	GOBInt32 i;
	GOBError err;
	
	if (LibraryInit) return GOBERR_ALREADY_INIT;
	
	// setup the callbacks
	MemFuncs = *mem;
	FSFuncs = *file;
	CodecFuncs = *codec;
	if (cache) {
		CacheFileFuncs = *cache;
		CacheFileActive = GOB_TRUE;
	} else {
		CacheFileActive = GOB_FALSE;
	}

	// allocate decompression buffer
	DecompBuffer = (GOBChar*)MemFuncs.alloc(GOB_BLOCK_SIZE + GOB_COMPRESS_OVERHEAD);
	if (!DecompBuffer) return GOBERR_NO_MEMORY;

	// clear open table
	for (i = 0; i < GOB_MAX_OPEN_FILES; ++i) {
		OpenFiles[i].valid = GOB_FALSE;
	}

	LibraryInit = GOB_TRUE;

	err = GOBSetCacheSize(1);
	if (err != GOBERR_OK) {
		LibraryInit = GOB_FALSE;
		return err;
	}

	ReadBuffer.data = NULL;
	err = GOBSetReadBufferSize(128*1024);
	if (err != GOBERR_OK) {
		LibraryInit = GOB_FALSE;
		return err;
	}

	return GOBERR_OK;
}

// GOBShutdown
// Public function.  Close the library.
GOBError GOBShutdown(GOBVoid)
{
	if (!LibraryInit) return GOBERR_NOT_INIT;
	
	// if we have an open archive, close it
	if (!InvalidHandle(ArchiveHandle)) GOBArchiveClose();
	
	FreeCache();

	// free read ahead buffer
	if (ReadBuffer.data) {
		MemFuncs.free(ReadBuffer.data);
		ReadBuffer.data = NULL;
	}

	// free decompression buffer
	MemFuncs.free(DecompBuffer);

	// free the file and block tables
	DeallocTables();

	LibraryInit = GOB_FALSE;
	return GOBERR_OK;
}


// GOBArchiveCreate
// Public function.  Create an empty GFC and GOB.
GOBError GOBArchiveCreate(const GOBChar* file)
{
	GOBChar fname[GOB_MAX_FILE_NAME_LEN];
	GOBFSHandle handle;
	GOBError error;

	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (!InvalidHandle(ArchiveHandle)) return GOBERR_ALREADY_OPEN;

	// Allocate the max space for tables
	error = AllocTables(GOB_MAX_BLOCKS, GOB_MAX_FILES, GOB_TRUE, GOB_TRUE);
	if (GOBERR_OK != error) {
		return error;
	}
	
	// create an empty GFC
	_snprintf(ControlFileName, GOB_MAX_FILE_NAME_LEN, "%s.gfc", file);

	ArchiveSize = 0;
	ArchiveNumBlocks = 0;
	ArchiveNumFiles = 0;
	CacheFileActive = GOB_FALSE;
	
	CommitFileTable();

	// create an empty GOB
	_snprintf(fname, GOB_MAX_FILE_NAME_LEN, "%s.gob", file);
	handle = FSFuncs.open(fname, GOBACCESS_WRITE);
	if (InvalidHandle(handle)) return GOBERR_CANNOT_CREATE;
	
	FSFuncs.close(&handle);

	return GOBERR_OK;	
}

// GOBArchiveOpen
// Public function.  Open a GOB file and cache file tables.
GOBError GOBArchiveOpen(const GOBChar* file, GOBAccessType atype, 
	GOBBool extended, GOBBool safe)
{
	GOBChar fname[GOB_MAX_FILE_NAME_LEN];
	GOBFSHandle handle;
	GOBUInt32 magic;
	GOBUInt32 i;
	GOBError error;

	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (!InvalidHandle(ArchiveHandle)) return GOBERR_ALREADY_OPEN;

	// open the GFC
	_snprintf(ControlFileName, GOB_MAX_FILE_NAME_LEN, "%s.gfc", file);
	handle = FSFuncs.open(ControlFileName, atype);
	if (InvalidHandle(handle)) return GOBERR_FILE_NOT_FOUND;

	// read and check the magic
	if (!FSFuncs.read(handle, &magic, sizeof(magic))) return GOBERR_FILE_READ;
	if (SwapBytes(magic) != GOB_MAGIC_IDENTIFIER) return GOBERR_NOT_GOB_FILE;

	// read the GOB archive size
	if (!FSFuncs.read(handle, &ArchiveSize, sizeof(ArchiveSize))) return GOBERR_FILE_READ;
	ArchiveSize = SwapBytes(ArchiveSize);

	// read the number of blocks
	if (!FSFuncs.read(handle, &ArchiveNumBlocks, sizeof(ArchiveNumBlocks))) return GOBERR_FILE_READ;
	ArchiveNumBlocks = SwapBytes(ArchiveNumBlocks);

	// read the number of files
	if (!FSFuncs.read(handle, &ArchiveNumFiles, sizeof(ArchiveNumFiles))) return GOBERR_FILE_READ;
	ArchiveNumFiles = SwapBytes(ArchiveNumFiles);
	
	// Allocate the space for tables
	if (atype == GOBACCESS_READ) {
		error = AllocTables(ArchiveNumBlocks, ArchiveNumFiles, extended, safe);
	}
	else {
		error = AllocTables(GOB_MAX_BLOCKS, GOB_MAX_FILES, extended, safe);
	}
	if (GOBERR_OK != error) {
		return error;
	}

	// read the block table
	if (ArchiveNumBlocks && 
		!FSFuncs.read(handle, BlockTable, 
		sizeof(struct GOBBlockTableEntry) * ArchiveNumBlocks))
	{
		return GOBERR_FILE_READ;
	}

	if (BlockCRC) {
		// read the block CRCs
		if (ArchiveNumBlocks && 
			!FSFuncs.read(handle, BlockCRC, 
			sizeof(GOBUInt32) * ArchiveNumBlocks))
		{
			return GOBERR_FILE_READ;
		}
	}
	else {
		// skip block CRCs
		FSFuncs.seek(handle, sizeof(GOBUInt32) * ArchiveNumBlocks, 
			GOBSEEK_CURRENT);
	}

	if (CacheFileActive)
	{
		// clear the block cache table
		for (i = 0; i < ArchiveNumBlocks / 32; ++i) {
			CacheFileTable[i] = 0;
		}
	}

	// open the cache file
	if (CacheFileActive && !CacheFileFuncs.open(ArchiveSize)) {
		CacheFileActive = GOB_FALSE;
	}

	// endian convert the table
	for (i = 0; i < ArchiveNumBlocks; ++i) {
		BlockTable[i].next = SwapBytes(BlockTable[i].next);
		BlockTable[i].offset = SwapBytes(BlockTable[i].offset);
		BlockTable[i].size = SwapBytes(BlockTable[i].size);

		if (BlockCRC) {
			BlockCRC[i] = SwapBytes(BlockCRC[i]);
		}
	}

	// read the basic file table
	if (ArchiveNumFiles &&
		!FSFuncs.read(handle, FileTableBasic, 
		sizeof(struct GOBFileTableBasicEntry) * ArchiveNumFiles))
	{
		return GOBERR_FILE_READ;
	}

	// endian convert the table
	for (i = 0; i < ArchiveNumFiles; ++i) {
		FileTableBasic[i].hash = SwapBytes(FileTableBasic[i].hash);
		FileTableBasic[i].block = SwapBytes(FileTableBasic[i].block);
		FileTableBasic[i].size = SwapBytes(FileTableBasic[i].size);
	}

	// if we have memory for the extended file table
	if (FileTableExt) {
		// read the table
		if (ArchiveNumFiles &&
			!FSFuncs.read(handle, FileTableExt, 
			sizeof(struct GOBFileTableExtEntry) * ArchiveNumFiles))
		{
			return GOBERR_FILE_READ;
		}

		// endian convert the table
		for (i = 0; i < ArchiveNumFiles; ++i) {
			FileTableExt[i].crc = SwapBytes(FileTableExt[i].crc);
			FileTableExt[i].time = SwapBytes(FileTableExt[i].time);
		}
	}
	
	FSFuncs.close(&handle);

	// open the GOB
	_snprintf(fname, GOB_MAX_FILE_NAME_LEN, "%s.gob", file);
	ArchiveHandle = FSFuncs.open(fname, atype);
	if (InvalidHandle(ArchiveHandle)) return GOBERR_FILE_NOT_FOUND;

	// initialize stats gathering
	CurrentArchivePos = 0;
	ReadStats.bufferUsed = 0;
	ReadStats.bytesRead = 0;
	ReadStats.cacheBytesRead = 0;
	ReadStats.cacheBytesWrite = 0;
	ReadStats.totalSeeks = 0;
	ReadStats.farSeeks = 0;
	ReadStats.filesOpened = 0;

	return GOBERR_OK;
}

// GOBArchiveClose
// Public function.  Close an open GOB archive.
GOBError GOBArchiveClose(GOBVoid)
{
	GOBInt32 i;

	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (InvalidHandle(ArchiveHandle)) return GOBERR_NOT_OPEN;

	// close any open files
	for (i = 0; i < GOB_MAX_OPEN_FILES; ++i) {
		GOBClose(i);
	}

	// close the GOB
	FSFuncs.close(&ArchiveHandle);
	ArchiveHandle = (GOBFSHandle*)0xFFFFFFFF;

	// commit the file table if we're updated it
	if (FileTableDirty) {
		CommitFileTable();
	}
	
	// close the cache file
	if (CacheFileActive) {
		CacheFileFuncs.close();
		CacheFileActive = GOB_FALSE;
	}

	return GOBERR_OK;
}

static int CDECL SortBlockDescsCallback(const void* elem1, const void* elem2)
{
	return (int)((struct GOBBlockTableEntry *)elem1)->offset - 
		(int)((struct GOBBlockTableEntry *)elem2)->offset;
}

// GOBArchiveCheckMarkers
// Public function.  Check start/end markers to check approximate validity of GOB file
GOBError GOBArchiveCheckMarkers(GOBVoid)
{
	GOBUInt32 i;
	GOBUInt32 valid_blocks;
	struct GOBBlockTableEntry *blocks;
	GOBUInt32 block;
	GOBUInt32 start_marker;
	GOBUInt32 end_marker;
	GOBBool ok;

	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (InvalidHandle(ArchiveHandle)) return GOBERR_NOT_OPEN;

	// count valid blocks
	valid_blocks = 0;
	for (i = 0; i < ArchiveNumBlocks; i++)
	{
		if (BlockTable[i].size != GOB_INVALID_SIZE && 
			BlockTable[i].next != GOB_INVALID_BLOCK)
		{
			valid_blocks++;
		}
	}

	// arcvive is empty
	if (valid_blocks == 0)
	{
		return GOBERR_OK;
	}

	// alloc mem for valid block list
	blocks = (GOBBlockTableEntry *) MemFuncs.alloc(sizeof(*blocks) * valid_blocks);
	if (blocks == NULL)
	{
		return GOBERR_NO_MEMORY;
	}

	// copy valid blocks descriptions
	block = 0;
	for (i = 0; i < ArchiveNumBlocks; ++i)
	{
		if (BlockTable[i].size != GOB_INVALID_SIZE && 
			BlockTable[i].next != GOB_INVALID_BLOCK)
		{
			blocks[block++] = BlockTable[i];
		}
	}
	assert(block == valid_blocks);

	// and sort 'em
	qsort(blocks, valid_blocks, sizeof(*blocks), SortBlockDescsCallback);

	// suppress some warnings
	start_marker = 0;
	end_marker = 0;

	// now scan entire archive for start-of-block and end-of-block markers
	for (i = 0; i < valid_blocks; i++)
	{
		ok = GOB_TRUE;
		ok = ok && !FSFuncs.seek(ArchiveHandle, blocks[i].offset, GOBSEEK_START);
		ok = ok && FSFuncs.read(ArchiveHandle, &start_marker, sizeof(GOBUInt32)) == sizeof(GOBUInt32);
		ok = ok && !FSFuncs.seek(ArchiveHandle, blocks[i].offset + blocks[i].size - sizeof(GOBUInt32), GOBSEEK_START);
		ok = ok && FSFuncs.read(ArchiveHandle, &end_marker, sizeof(GOBUInt32)) == sizeof(GOBUInt32);
		if (!ok || 
			SwapBytes(start_marker) != GOBMARKER_STARTBLOCK || 
			SwapBytes(end_marker) != GOBMARKER_ENDBLOCK)
		{
			MemFuncs.free(blocks);
			
			return GOBERR_NOT_GOB_FILE;
		}
	}
	
	MemFuncs.free(blocks);
	
	return GOBERR_OK;
}

// GOBArchiveCreate
// Public function.  Create an empty GFC and GOB.
GOBUInt32 GOBGetSlack(GOBUInt32 x)
{
	GOBUInt32 align = x % GOB_BLOCK_ALIGNMENT;
	if (align) return GOB_BLOCK_ALIGNMENT - align;
	return 0;
}

// GOBOpen
// Public function.  Open a file inside a GOB.
GOBError GOBOpen(GOBChar* file, GOBHandle* handle)
{
	GOBInt32 entry;
	GOBChar* lfile;
	
	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (InvalidHandle(ArchiveHandle)) return GOBERR_NOT_OPEN;

	// find a free handle
	for (*handle = 0; *handle < GOB_MAX_OPEN_FILES; (*handle) += 1) {
		if (!OpenFiles[*handle].valid) break;
	}

	if (*handle >= GOB_MAX_OPEN_FILES) return GOBERR_TOO_MANY_OPEN;

	// find the file in the table
	lfile = LowerCase(file);

	entry = GetFileTableEntry(lfile);

	if (entry == -1) return GOBERR_FILE_NOT_FOUND;

	// setup the open file
	OpenFiles[*handle].startBlock = OpenFiles[*handle].block = 
		FileTableBasic[entry].block;
	OpenFiles[*handle].size = FileTableBasic[entry].size;
	OpenFiles[*handle].offset = 0;
	OpenFiles[*handle].pos = 0;

	OpenFiles[*handle].valid = GOB_TRUE;

	++ReadStats.filesOpened;
	
	return GOBERR_OK;
}

// GOBOpenCode
// Public function.  Open file with a code inside a GOB.
GOBError GOBOpenCode(GOBInt32 code, GOBHandle* handle)
{	
	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (InvalidHandle(ArchiveHandle)) return GOBERR_NOT_OPEN;

	// find a free handle
	for (*handle = 0; *handle < GOB_MAX_OPEN_FILES; (*handle) += 1) {
		if (!OpenFiles[*handle].valid) break;
	}

	if (*handle >= GOB_MAX_OPEN_FILES) return GOBERR_TOO_MANY_OPEN;

	// setup the open file
	OpenFiles[*handle].startBlock = OpenFiles[*handle].block = 
		FileTableBasic[code].block;
	OpenFiles[*handle].size = FileTableBasic[code].size;
	OpenFiles[*handle].offset = 0;
	OpenFiles[*handle].pos = 0;

	OpenFiles[*handle].valid = GOB_TRUE;

	++ReadStats.filesOpened;
	
	return GOBERR_OK;
}

// GOBClose
// Public function.  Close a file.
GOBError GOBClose(GOBHandle handle)
{
	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (InvalidHandle(ArchiveHandle)) return GOBERR_NOT_OPEN;
	if (!OpenFiles[handle].valid) return GOBERR_NOT_OPEN;
	
	// close the file by simply invalidating the open
	// file table entry
	OpenFiles[handle].valid = GOB_FALSE;
	
	return GOBERR_OK;
}

static GOBUInt32 RawRead(GOBVoid* buffer, GOBUInt32 size, GOBUInt32 pos)
{
	GOBUInt32 bytes;

	// Reads _must_ be aligned otherwise things get very slow
	if (pos % GOB_BLOCK_ALIGNMENT) {
		return 0;
	}
	if ((GOBUInt32)buffer % GOB_MEM_ALIGNMENT) {
		return 0;
	}
	
	// seek
	if (FSFuncs.seek(ArchiveHandle, pos, GOBSEEK_START)) return 0;
	
	if (CurrentArchivePos != pos) ++ReadStats.totalSeeks;
	if (pos > CurrentArchivePos + GOB_BLOCK_ALIGNMENT ||
		CurrentArchivePos > pos + GOB_BLOCK_ALIGNMENT)
	{
		++ReadStats.farSeeks;
	}

	// read
	bytes = FSFuncs.read(ArchiveHandle, buffer, size);
	
	ReadStats.bytesRead += bytes;
	CurrentArchivePos = pos + bytes;
	
	return bytes;
}

static GOBUInt32 CacheRawRead(GOBVoid* buffer, GOBUInt32 size, GOBUInt32 pos)
{
	GOBUInt32 bytes;

	// Reads _must_ be aligned otherwise things get very slow
	if (pos % GOB_BLOCK_ALIGNMENT) {
		return 0;
	}
	if ((GOBUInt32)buffer % GOB_MEM_ALIGNMENT) {
		return 0;
	}
	
	// seek
	if (CacheFileFuncs.seek(pos)) return 0;
	
	// read
	bytes = CacheFileFuncs.read(buffer, size);
	ReadStats.cacheBytesRead += bytes;

	return bytes;
}

static GOBUInt32 CacheRawWrite(GOBVoid* buffer, GOBUInt32 size, GOBUInt32 pos)
{
	GOBUInt32 bytes;

	// Writes _must_ be aligned otherwise things get very slow
	if (pos % GOB_BLOCK_ALIGNMENT) {
		return 0;
	}
	if ((GOBUInt32)buffer % GOB_MEM_ALIGNMENT) {
		return 0;
	}
	
	// seek
	if (CacheFileFuncs.seek(pos)) return 0;
	
	// write
	bytes = CacheFileFuncs.write(buffer, size);	
	ReadStats.cacheBytesWrite += bytes;
	
	return bytes;
}

static GOBInt32 BlockReadLow(GOBUInt32 block)
{
	GOBUInt32 pos;
	GOBUInt32 bytes;
	GOBBool cache_read;
	GOBBool cache_write;
	GOBBool cache_fail;

	pos = 0;
	cache_read = GOB_FALSE;
	cache_write = GOB_FALSE;
	cache_fail = GOB_FALSE;
	
	for (;;) {	
		// is the block in the read ahead buffer?
		if (ReadBuffer.pos <= BlockTable[block].offset + pos &&
			ReadBuffer.pos + ReadBuffer.size > BlockTable[block].offset + pos)
		{
			GOBUInt32 buffer_offset;
			GOBUInt32 buffer_size;
			
			// use data in the read buffer
			buffer_offset = BlockTable[block].offset + pos - ReadBuffer.pos;
			buffer_size = ReadBuffer.size - buffer_offset;

			// clamp size
			if (buffer_size > BlockTable[block].size - pos) {
				buffer_size = BlockTable[block].size - pos;
			}
			
			memcpy(&DecompBuffer[pos], &ReadBuffer.dataStart[buffer_offset], buffer_size);

			pos += buffer_size;
		}

		// got enough data
		if (pos == BlockTable[block].size) break;

		// refill read buffer
		ReadBuffer.pos = BlockTable[block].offset + pos;
		ReadBuffer.pos -= ReadBuffer.pos % GOB_BLOCK_ALIGNMENT;

		// check if block is in the external cache system
		if (CacheFileActive &&
			CacheFileTable[block / 32] & (1 << (block % 32)))
		{
			if (CacheRawRead(ReadBuffer.dataStart, 
				ReadBuffer.size, ReadBuffer.pos))
			{
				cache_read = GOB_TRUE;
				continue;
			}
		}

		// read block from archive
		bytes = RawRead(ReadBuffer.dataStart, ReadBuffer.size, ReadBuffer.pos);
		if (bytes != ReadBuffer.size && 
			bytes != ArchiveSize - ReadBuffer.pos) 
		{
			return -1; // Main read fail error code
		}
		
		// write block to cache file
		if (CacheFileActive)
		{
			if (CacheRawWrite(ReadBuffer.dataStart, bytes, 
				ReadBuffer.pos) == bytes)
			{
				cache_write = GOB_TRUE;
			}
			else
			{
				cache_fail = GOB_TRUE;
			}
		}
	}

	if (cache_write) {
		if (!cache_fail) return 2;
		return 0;
	}

	if (cache_read) return 1;
	return 0;
}

static GOBBool BlockReadWithCache(GOBUInt32 block)
{
	GOBInt32 i;
	
	for (i = 0; i < GOB_READ_RETRYS; ++i) {
		GOBInt32 result;

		// read the data
		result = BlockReadLow(block);
		if (result >= 0)
		{
			if (BlockCRC) {
				// crc check
				GOBUInt32 crc;

				crc = adler32(0L, Z_NULL, 0);
				crc = adler32(crc, (const unsigned char*)DecompBuffer, 
					BlockTable[block].size);
				
				if (BlockCRC[block] != crc) {
					// crc mismatch, we must have got bad data --
					// try invalidating the cache and retrying...
					if (CacheFileActive) {
						CacheFileTable[block / 32] &= ~(1 << (block % 32));
					}
					ReadBuffer.pos = 0xFFFFFFFF;
					continue;
				}
			}

			// if cache write occurred -- mark block as cached
			if (result == 2) {
				CacheFileTable[block / 32] |= (1 << (block % 32));
			}
			
			// read success, crc success (or no check performed)
			return GOB_TRUE;
		}
	}

	// multiple read/crc failures
	return GOB_FALSE;
}

static GOBUInt32 BlockRead(GOBVoid* buffer, GOBUInt32 block)
{
	GOBUInt32 size;
	GOBInt32 codec_index;
	GOBChar *compressed_data;
	
	// read block from cache or archive
	if (!BlockReadWithCache(block))
	{
		return GOB_INVALID_SIZE;
	}
	
	// decompress
	codec_index = 0;
	size = 0; // Initialize to satisfy compiler
	compressed_data = DecompBuffer + sizeof(GOBUInt32); // skip start-of-block marker
	while (codec_index < CodecFuncs.codecs) {
		// Check if codec matches
		if (*compressed_data == CodecFuncs.codec[codec_index].tag) {
			size = GOB_BLOCK_SIZE;
			if (CodecFuncs.codec[codec_index].decompress(compressed_data + 1, 
				BlockTable[block].size - 1 - sizeof(GOBUInt32) * 2, buffer, &size)) {
				return GOB_INVALID_SIZE;
			}
			break;
		}
		codec_index++;
	}

	// If no suitable codecs were found, we're screwed
	if (codec_index == CodecFuncs.codecs) {
		return GOB_INVALID_SIZE;
	}

	if (ProfileReadCallback && ProfileEnabled) {
		// register current read command
		ProfileReadCallback(block);
	}

	return size;
}

static GOBVoid FillCacheBlock(GOBUInt32 block, GOBUInt32 index)
{
	CacheBlocks[index].time = CacheBlockCounter++;
	CacheBlocks[index].block = block;
	CacheBlocks[index].size = BlockRead(CacheBlocks[index].data, block);
}

static GOBInt32 FindBestCacheBlock(GOBUInt32 block)
{
	GOBInt32 i;
	GOBUInt32 oldest_time;
	GOBInt32 oldest_index;

	oldest_time = 0xFFFFFFFF;
	oldest_index = -1;

	for (i = 0; i < (signed)NumCacheBlocks; ++i) {
		if (CacheBlocks[i].block == block) {
			// if block is in this read buffer, use it
			return i;
		}
		
		// find the buffer that hasn't been accessed
		// for the longest time
		if (CacheBlocks[i].time < oldest_time) {
			oldest_time = CacheBlocks[i].time;
			oldest_index = i;
		}
	}

	// use the buffer that hasn't been accessed
	// in the longest time
	return oldest_index;
}

// GOBRead
// Public function.  Read from an open file using
// a funky read-ahead buffer system.
GOBUInt32 GOBRead(GOBVoid* buffer, GOBUInt32 size, GOBHandle handle)
{
	GOBUInt32 pos;
	GOBInt32 cache_id;
	
	if (!LibraryInit) return 0;
	if (InvalidHandle(ArchiveHandle)) return 0;
	if (!OpenFiles[handle].valid) return 0;

	// make sure we're reading within the file
	if (OpenFiles[handle].pos + size > OpenFiles[handle].size) {
		size = OpenFiles[handle].size - OpenFiles[handle].pos;
		if (!size) return 0;
	}
	
	cache_id = FindBestCacheBlock(OpenFiles[handle].block);
	if (cache_id < 0) return GOB_INVALID_SIZE;

	pos = OpenFiles[handle].pos;

	for (;;) {
		// are looking for data inside the read buffer?
		if (CacheBlocks[cache_id].block == OpenFiles[handle].block) {
			// move any relevant data from the read buffer to the target buffer
			GOBUInt32 buffer_size;
			
			// calc size of data we want from current buffer
			buffer_size = CacheBlocks[cache_id].size - OpenFiles[handle].offset;
			if (buffer_size > size) buffer_size = size;
			
			// move from read buffer into output buffer
			memcpy(&((char*)buffer)[OpenFiles[handle].pos - pos], 
				&CacheBlocks[cache_id].data[OpenFiles[handle].offset], 
				buffer_size);

			// update file position
			OpenFiles[handle].pos += buffer_size;
			OpenFiles[handle].offset += buffer_size;
			
			// if we've completed this block -- move to next
			if (OpenFiles[handle].offset == CacheBlocks[cache_id].size) {
				OpenFiles[handle].block = BlockTable[OpenFiles[handle].block].next;
				OpenFiles[handle].offset = 0;
			}
			
			CacheBlocks[cache_id].time = CacheBlockCounter++;
		
			ReadStats.bufferUsed += buffer_size;
			size -= buffer_size;
			if (size == 0) break;
		}

		// refill the buffer
		FillCacheBlock(OpenFiles[handle].block, cache_id);
		if (CacheBlocks[cache_id].size == GOB_INVALID_SIZE) {
			CacheBlocks[cache_id].block = GOB_INVALID_BLOCK;
			return GOB_INVALID_SIZE;
		}

		// reading off the end of the archive
		if (CacheBlocks[cache_id].block != OpenFiles[handle].block) break;
	}
	
	return OpenFiles[handle].pos - pos;
}

// GOBSeek
// Public function.  Seek to a position in an open file.
GOBError GOBSeek(GOBHandle handle, GOBUInt32 offset, GOBSeekType type, GOBUInt32* pos)
{
	GOBUInt32 blocks;
	
	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (InvalidHandle(ArchiveHandle)) return GOBERR_NOT_OPEN;
	if (!OpenFiles[handle].valid) return GOBERR_NOT_OPEN;
	
	// find a new position based on the seek type
	switch (type) {
	case GOBSEEK_START:
		*pos = offset;
		break;

	case GOBSEEK_CURRENT:
		*pos = OpenFiles[handle].pos + offset;
		break;

	case GOBSEEK_END:
		*pos = OpenFiles[handle].size + offset;
		break;

	default:
		return GOBERR_INVALID_SEEK;
	}

	// check to make sure we're still in the file
	if (*pos > OpenFiles[handle].size) {
		return GOBERR_INVALID_SEEK;
	}

	// update the file position
	OpenFiles[handle].pos = *pos;

	// update block
	blocks = *pos / GOB_BLOCK_SIZE;
	OpenFiles[handle].block = OpenFiles[handle].startBlock;
	while (blocks--) {
		OpenFiles[handle].block = BlockTable[OpenFiles[handle].block].next;
	}

	// update position inside block
	OpenFiles[handle].offset = *pos % GOB_BLOCK_SIZE;

	return GOBERR_OK;
}


static GOBUInt32 FindFreeBlock(GOBVoid)
{
	GOBInt32 i;
	for (i = 0; i < GOB_MAX_BLOCKS; ++i) {
		if (BlockTable[i].next == GOB_INVALID_BLOCK) return i;
	}
	return GOB_MAX_BLOCKS;
}

// GOBWrite
// Public function.  Write an entire file.  The file should not be open!
GOBError GOBWrite(GOBVoid* buffer, GOBUInt32 size, GOBUInt32 mtime, const GOBChar* file, GOBUInt32 codec_mask)
{
	GOBHandle handle;
	GOBInt32 slack;
	GOBChar* lfile;
	GOBUInt32 hash;
	GOBUInt32 crc;
	GOBInt32 i;
	GOBChar* out;
	GOBUInt32 pos;
	GOBUInt32 last_block;
	GOBInt32 codec_index;
	GOBInt32 compression_ratio;
	GOBChar* out_data;
	GOBUInt32 compressed_size;
	
	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (InvalidHandle(ArchiveHandle)) return GOBERR_NOT_OPEN;
	if (!FileTableExt) return GOBERR_NO_EXTENDED;
	if (!BlockCRC) return GOBERR_NO_EXTENDED;

	InvalidateCache();

	// delete the file if it exists
	GOBDelete(file);

	// find a free entry in the file table
	for (handle = 0; handle < GOB_MAX_FILES; ++handle) {
		if (FileTableBasic[handle].block == GOB_INVALID_BLOCK) break;
	}

	if (handle >= GOB_MAX_FILES) return GOBERR_TOO_MANY_FILES;
	if (handle >= (GOBInt32)ArchiveNumFiles) ArchiveNumFiles = handle + 1;

	// move to the end of the GOB
	if (FSFuncs.seek(ArchiveHandle, 0, GOBSEEK_END)) {
		return GOBERR_FILE_WRITE;
	}
	
	// alloc compression buffer
	out = (GOBChar*)MemFuncs.alloc(GOB_BLOCK_SIZE + GOB_COMPRESS_OVERHEAD);

	last_block = GOB_MAX_BLOCKS - 1;

	for (pos = 0; pos < size; pos += GOB_BLOCK_SIZE) {
		GOBUInt32 block;
		GOBUInt32 in_size;

		// get a free block
		block = FindFreeBlock();
		if (block >= GOB_MAX_BLOCKS) return GOBERR_TOO_MANY_BLOCKS;
		if (block >= ArchiveNumBlocks) ArchiveNumBlocks = block + 1;

		// if this is not the first block, mark next block for the last block
		// else assign the first block in file table
		if (pos != 0) BlockTable[last_block].next = block;
		else FileTableBasic[handle].block = block;
		
		// invalidate the next block
		BlockTable[block].next = GOB_MAX_BLOCKS;

		// compute the decompressed block size
		in_size = size - pos;
		if (in_size > GOB_BLOCK_SIZE) in_size = GOB_BLOCK_SIZE;

		// compress block
		
		for (
			codec_index = 0;
			codec_index < CodecFuncs.codecs;
			codec_index++)
		{
			if ( ! (GOB_CODEC_MASK(codec_index) & codec_mask) )
			{
				// skip if this codec is not listed as one of the allowed ones
				continue;
			}
			BlockTable[block].size = GOB_BLOCK_SIZE + GOB_COMPRESS_OVERHEAD;
			out_data = out;
			*(GOBUInt32*)out_data = SwapBytes(GOBMARKER_STARTBLOCK);
			out_data += sizeof(GOBUInt32);
			*out_data = CodecFuncs.codec[codec_index].tag;
			out_data++;
			if (CodecFuncs.codec[codec_index].compress(&((GOBChar*)buffer)[pos],
				in_size, out_data, &BlockTable[block].size)) 
			{
				return GOBERR_COMPRESS_FAIL;
			}
			out_data += BlockTable[block].size;
			*(GOBUInt32*)out_data = SwapBytes(GOBMARKER_ENDBLOCK);
			out_data += sizeof(GOBUInt32);

			// Adjust for the prefixed start-of-block marker and codec tag and trailing end-of-block marker
			compressed_size = BlockTable[block].size;
			BlockTable[block].size += 1 + sizeof(GOBUInt32) * 2;

			// Check compression result
			compression_ratio = compressed_size * 100 / in_size;
			if (compression_ratio <= CodecFuncs.codec[codec_index].max_ratio)
			{
				// Compressed result is under par.  Let's go with it
				break;
			}

			// Otherwise, try the next compressor
		}

		// If no suitable codecs were found, take our ball and go home
		if (codec_index == CodecFuncs.codecs) return GOBERR_NO_SUITABLE_CODEC;

		// compute and store the CRC
		BlockCRC[block] = adler32(0L, Z_NULL, 0);
		BlockCRC[block] = adler32(BlockCRC[block], (const unsigned char*)out, 
					BlockTable[block].size);

		// write block
		if (FSFuncs.write(ArchiveHandle, out, BlockTable[block].size) != 
			(signed)BlockTable[block].size) 
		{
			return GOBERR_FILE_WRITE;
		}

		// compute the slack (to keep alignment)
		slack = GOBGetSlack(BlockTable[block].size);
	
		// write the slack space
		memset(out, 0, slack);
		if (FSFuncs.write(ArchiveHandle, out, slack) != slack) {
			return GOBERR_FILE_WRITE;
		}
	
		BlockTable[block].offset = ArchiveSize;
		ArchiveSize += BlockTable[block].size + slack;

		last_block = block;
	}

	MemFuncs.free(out);

	lfile = LowerCase(file);

	// calculate file name hash
	hash = crc32(0L, Z_NULL, 0);
	hash = crc32(hash, (const unsigned char*)lfile, strlen(lfile));
	
	// make sure hash is unique
	for (i = 0; i < GOB_MAX_FILES; ++i) {
		if (i != handle &&
			FileTableBasic[i].block != GOB_INVALID_BLOCK && 
			FileTableBasic[i].hash == hash) 
		{
			return GOBERR_DUP_HASH;
		}
	}

	// update the file tables
	FileTableBasic[handle].hash = hash;
	FileTableBasic[handle].size = size;

	strcpy(FileTableExt[handle].name, lfile);
	
	crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, (const unsigned char*)buffer, size);
	FileTableExt[handle].crc = crc;
	
	FileTableExt[handle].time = mtime;

	FileTableDirty = GOB_TRUE;
	return GOBERR_OK;
}

// GOBDelete
// Public function.  Delete a file from a GOB.  The file should not be open!
GOBError GOBDelete(const GOBChar* file)
{
	GOBInt32 entry;
	GOBChar* lfile;
	GOBUInt32 block;

	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (InvalidHandle(ArchiveHandle)) return GOBERR_NOT_OPEN;
	if (!FileTableExt) return GOBERR_NO_EXTENDED;
	if (!BlockCRC) return GOBERR_NO_EXTENDED;

	// find the file in the table
	lfile = LowerCase(file);
	
	entry = GetFileTableEntry(lfile);
	if (entry == -1) return GOBERR_FILE_NOT_FOUND;

	// invalidate blocks
	block = FileTableBasic[entry].block;
	do {
		GOBUInt32 next;
		next = BlockTable[block].next;
		BlockTable[block].next = GOB_INVALID_BLOCK;
		block = next;
	 } while(block != GOB_MAX_BLOCKS);

	// invalidate the file
	FileTableBasic[entry].block = GOB_INVALID_BLOCK;

	FileTableDirty = GOB_TRUE;

	return GOBERR_OK;
}

// GOBRearrange
// Public function.  Sorts the blocks in an archive.
GOBError GOBRearrange(const GOBChar* file, const GOBUInt32* xlat, GOBFileSysRenameFunc _rename)
{
	GOBError err;
	GOBVoid* buffer;
	GOBInt32 slack;
	GOBVoid* slack_buf;
	GOBUInt32 i;
	GOBUInt32 size;
	GOBFSHandle temp_handle;
	GOBChar full_name[GOB_MAX_FILE_NAME_LEN];
	
	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (!InvalidHandle(ArchiveHandle)) return GOBERR_ALREADY_OPEN;
	if (!FileTableExt) return GOBERR_NO_EXTENDED;
	if (!BlockCRC) return GOBERR_NO_EXTENDED;

	// start things up
	err = GOBArchiveOpen(file, GOBACCESS_READ, GOB_TRUE, GOB_TRUE);
	if (err != GOBERR_OK) return err;

	// create temporary file
	temp_handle = FSFuncs.open("~temp.tmp", GOBACCESS_WRITE);
	if (InvalidHandle(temp_handle)) return GOBERR_FILE_WRITE;

	size = 0;

	// create an empty buffer for slack
	slack_buf = MemFuncs.alloc(GOB_BLOCK_ALIGNMENT);
	if (!slack_buf) return GOBERR_NO_MEMORY;
	memset(slack_buf, 0, GOB_BLOCK_ALIGNMENT);

	// get memory for block
	buffer = MemFuncs.alloc(GOB_BLOCK_SIZE + GOB_COMPRESS_OVERHEAD);
	if (!buffer) return GOBERR_NO_MEMORY;

	// copy files in new order to end of archive
	for (i = 0; i < ArchiveNumBlocks; ++i) {
		if (BlockTable[xlat[i]].next != GOB_INVALID_BLOCK) {
			// seek to the block
			if (FSFuncs.seek(ArchiveHandle, 
				BlockTable[xlat[i]].offset, GOBSEEK_START)) 
			{
				return GOBERR_FILE_READ;
			}
			
			// read the block
			if (FSFuncs.read(ArchiveHandle, buffer, BlockTable[xlat[i]].size) !=
				(signed)BlockTable[xlat[i]].size) 
			{
				return GOBERR_FILE_READ;
			}

			// write block
			if (FSFuncs.write(temp_handle, buffer, BlockTable[xlat[i]].size) !=
				(signed)BlockTable[xlat[i]].size) 
			{
				return GOBERR_FILE_WRITE;
			}
			
			// write the slack
			slack = GOBGetSlack(BlockTable[xlat[i]].size);
			if (FSFuncs.write(temp_handle, slack_buf, slack) != slack) {
				return GOBERR_FILE_WRITE;
			}
			
			// update block pos
			BlockTable[xlat[i]].offset = size;
			size += BlockTable[xlat[i]].size + slack;
		}
	}

	MemFuncs.free(buffer);
	MemFuncs.free(slack_buf);
	
	// close the archive
	err = GOBArchiveClose();
	if (err != GOBERR_OK) return err;

	// close temp file
	FSFuncs.close(&temp_handle);

	// overrwrite archive with temp file
	_snprintf(full_name, GOB_MAX_FILE_NAME_LEN, "%s.gob", file);
	if (_rename("~temp.tmp", full_name)) return GOBERR_FILE_RENAME;

	ArchiveSize = size;

	CommitFileTable();
			
	return GOBERR_OK;
}


// GOBVerify
// Public function.  Verifies the integrity of a file.
GOBError GOBVerify(const GOBChar* file, GOBBool* status)
{
	GOBHandle handle;
	GOBError err;
	GOBVoid* buffer;
	GOBUInt32 size, junk;
	GOBUInt32 crc;
	GOBInt32 entry;
	GOBChar* lfile;

	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (InvalidHandle(ArchiveHandle)) return GOBERR_NOT_OPEN;
	if (!FileTableExt) return GOBERR_NO_EXTENDED;
	if (!BlockCRC) return GOBERR_NO_EXTENDED;

	// get the file size
	size = 0; // assign to avoid compiler warning
	err = GOBGetSize(file, &size, &junk, &junk);
	if (err != GOBERR_OK) return err;
	
	// open the file
	err = GOBOpen((GOBChar*)file, &handle);
	if (err != GOBERR_OK) return err;

	lfile = LowerCase(file);
	entry = GetFileTableEntry(lfile);

	// alloc space for the file 
	buffer = MemFuncs.alloc(size);
	if (!buffer) return GOBERR_NO_MEMORY;

	// read it into the buffer
	crc = GOBRead(buffer, size, handle);
	if (crc != size) return GOBERR_FILE_READ;
	
	// calc the crc
	crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, (const unsigned char*)buffer, size);
	
	MemFuncs.free(buffer);

	// verify the crc matches
	if (crc != FileTableExt[entry].crc) *status = GOB_FALSE;
	else *status = GOB_TRUE;
	
	err = GOBClose(handle);
	if (err != GOBERR_OK) return err;
	
	return GOBERR_OK;
}

// GOBGetSize
// Public function.  Get a file compressed, decompressed, slack sizes.
GOBError GOBGetSize(const GOBChar* file, 
	GOBUInt32* decomp, GOBUInt32* comp, GOBUInt32* slack)
{
	GOBInt32 entry;
	GOBChar* lfile;
	GOBUInt32 block;

	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (InvalidHandle(ArchiveHandle)) return GOBERR_NOT_OPEN;

	// get file table entry
	lfile = LowerCase(file);
	entry = GetFileTableEntry(lfile);
	if (entry == -1) return GOBERR_FILE_NOT_FOUND;

	// decompressed size from file table
	*decomp = FileTableBasic[entry].size;
	
	// compressed size is sum of block sizes
	*comp = 0;
	*slack = 0;
	block = FileTableBasic[entry].block;
	while (block != GOB_MAX_BLOCKS) {
		*comp += BlockTable[block].size;
		*slack += GOBGetSlack(BlockTable[block].size);
		block = BlockTable[block].next;
	}

	return GOBERR_OK;
}

// GOBGetTime
// Public function.  Get a file modification time.
GOBError GOBGetTime(const GOBChar* file, GOBUInt32* time)
{
	GOBInt32 entry;
	GOBChar* lfile;

	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (InvalidHandle(ArchiveHandle)) return GOBERR_NOT_OPEN;
	if (!FileTableExt) return GOBERR_NO_EXTENDED;
	if (!BlockCRC) return GOBERR_NO_EXTENDED;

	lfile = LowerCase(file);
	entry = GetFileTableEntry(lfile);
	if (entry == -1) return GOBERR_FILE_NOT_FOUND;

	*time = FileTableExt[entry].time;
	return GOBERR_OK;
}

// GOBGetCRC
// Public function.  Get a file CRC.
GOBError GOBGetCRC(const GOBChar* file, GOBUInt32* crc)
{
	GOBInt32 entry;
	GOBChar* lfile;

	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (InvalidHandle(ArchiveHandle)) return GOBERR_NOT_OPEN;
	if (!FileTableExt) return GOBERR_NO_EXTENDED;
	if (!BlockCRC) return GOBERR_NO_EXTENDED;

	lfile = LowerCase(file);
	entry = GetFileTableEntry(lfile);
	if (entry == -1) return GOBERR_FILE_NOT_FOUND;

	*crc = FileTableExt[entry].crc;
	return GOBERR_OK;
}

// GOBAccess
// Public function.  Determine if a file exists in the archive.
GOBError GOBAccess(const GOBChar* file, GOBBool* status)
{
	GOBInt32 entry;
	GOBChar* lfile;

	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (InvalidHandle(ArchiveHandle)) return GOBERR_NOT_OPEN;

	lfile = LowerCase(file);
	entry = GetFileTableEntry(lfile);
	if (entry == -1) *status = GOB_FALSE;
	else *status = GOB_TRUE;

	return GOBERR_OK;
}

// GOBGetFileCode
// Public function.  Find the index into the file table of a file.
GOBInt32 GOBGetFileCode(const GOBChar* file)
{
	GOBInt32 entry;
	GOBChar* lfile;

	if (!LibraryInit) return -1;
	if (InvalidHandle(ArchiveHandle)) return -1;

	lfile = LowerCase(file);
	entry = GetFileTableEntry(lfile);

	return entry;
}

// GOBGetFileTables
// Public function.  Return the active file tables.
GOBError GOBGetFileTables(struct GOBFileTableBasicEntry** basic, 
	struct GOBFileTableExtEntry** ext)
{
	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (InvalidHandle(ArchiveHandle)) return GOBERR_NOT_OPEN;
	*basic = FileTableBasic;
	*ext = FileTableExt;
	return GOBERR_OK;
}

// GOBGetBlockTable
// Public function.  Return the active block table.
GOBError GOBGetBlockTable(struct GOBBlockTableEntry** table, GOBUInt32* num)
{
	if (!LibraryInit) return GOBERR_NOT_INIT;
	if (InvalidHandle(ArchiveHandle)) return GOBERR_NOT_OPEN;
	*table = BlockTable;
	*num = ArchiveNumBlocks;
	return GOBERR_OK;
}

// GOBSetCacheSize
// Public function.  Allocates buffers to cache blocks.
GOBError GOBSetCacheSize(GOBUInt32 num)
{
	GOBUInt32 i;

	if (!LibraryInit) return GOBERR_NOT_INIT;

	// only continue if we actually need to resize
	if (num == NumCacheBlocks) return GOBERR_OK;

	// free old cache buffers
	FreeCache();

	NumCacheBlocks = 0;
	
	CacheBlocks = (struct GOBBlockCache*)MemFuncs.alloc(
			sizeof(struct GOBBlockCache) * num);
	if (!CacheBlocks) return GOBERR_NO_MEMORY;

	// allocate cache blocks and initialize
	for (i = 0; i < num; ++i) {
		CacheBlocks[i].data = (GOBChar*)MemFuncs.alloc(GOB_BLOCK_SIZE);
		if (!CacheBlocks[i].data) return GOBERR_NO_MEMORY;
		
		CacheBlocks[i].size = 0;
		CacheBlocks[i].time = 0;
		CacheBlocks[i].block = 0xFFFFFFFF;
		
		++NumCacheBlocks;
	}
	
	return GOBERR_OK;
}

// GOBSetReadBufferSize
// Public function.  Allocate a read ahead buffer.
GOBError GOBSetReadBufferSize(GOBUInt32 size)
{
	if (!LibraryInit) return GOBERR_NOT_INIT;

	// only continue if we actually need to resize
	if (size == ReadBuffer.size) return GOBERR_OK;

	// remove old buffer
	if (ReadBuffer.data) MemFuncs.free(ReadBuffer.data);

	// allocate new buffer
	ReadBuffer.data = (GOBChar*)MemFuncs.alloc(size + GOB_MEM_ALIGNMENT);
	if (!ReadBuffer.data) return GOB_INVALID_SIZE;
	
	// set aligned pointer
	ReadBuffer.dataStart = 
		&ReadBuffer.data[GOB_MEM_ALIGNMENT - 
		((GOBUInt32)(ReadBuffer.data) % GOB_MEM_ALIGNMENT)];

	ReadBuffer.pos = 0xFFFFFFFF;
	ReadBuffer.size = size;
	
	return GOBERR_OK;
}

// GOBGetReadStats
// Public function.  Get file read statistics (seeks, sizes).
struct GOBReadStats GOBGetReadStats(GOBVoid)
{
	return ReadStats;
}


GOBVoid GOBSetProfileFuncs(struct GOBProfileFuncSet* fset)
{
	ProfileReadCallback = fset->read;
}

GOBError GOBStartProfile(GOBVoid)
{
	if (ProfileEnabled) return GOBERR_PROFILE_ON;
	ProfileEnabled = GOB_TRUE;
	return GOBERR_OK;
}

GOBError GOBStopProfile(GOBVoid)
{
	if (!ProfileEnabled) return GOBERR_PROFILE_OFF;
	ProfileEnabled = GOB_FALSE;
	return GOBERR_OK;
}
