// Filename:-	script.h
//

#ifndef SCRIPT_H
#define SCRIPT_H


///////////////////////////////////////////////////////////////////
//
// I've externalised these #defines so that the NPC->ModView script generator can use the same keywords 
//	without worrying about keeping 2 version in sync...
//
#define sANY_NONBLANK_STRING "<any non-blank string>"	// for when I just need to write out a flag keyword,
														//	but GenericParser needs a string arg during reading in.
#define sSCRIPTKEYWORD_LOADMODEL	"loadmodel"
#define sSCRIPTKEYWORD_NAME			"name"
#define sSCRIPTKEYWORD_BASEDIR		"basedir"	// special hack use for writing temp MVS files during NPC->MVS conversion
#define sSCRIPTKEYWORD_MODELFILE	"modelfile"
#define sSCRIPTKEYWORD_BOLTMODEL	"boltmodel"
#define sSCRIPTKEYWORD_PARENT		"parent"
#define sSCRIPTKEYWORD_BOLTTOBONE	"bolttobone"
#define sSCRIPTKEYWORD_BOLTTOSURFACE "bolttosurface"
#define sSCRIPTKEYWORD_LOCKSEQUENCE	"locksequence"
#define sSCRIPTKEYWORD_LOCKSEQUENCE_SECONDARY "locksequence_secondary"
#define sSCRIPTKEYWORD_BONENAME_SECONDARYSTART "bonename_secondarystart"
#define sSCRIPTKEYWORD_SURFACENAME_ROOTOVERRIDE "surfacename_rootoverride"
#define sSCRIPTKEYWORD_SKINFILE		"skinfile"
#define sSCRIPTKEYWORD_OLDSKINFILE	"oldskinfile"
#define sSCRIPTKEYWORD_ETHNIC		"ethnic"
#define sSCRIPTKEYWORD_SURFACES_ON				"surfaces_on"
#define sSCRIPTKEYWORD_SURFACES_OFF				"surfaces_off"
#define sSCRIPTKEYWORD_SURFACES_OFFNOCHILDREN	"surfaces_offnochildren"
#define sSCRIPTKEYWORD_MULTISEQ_PRIMARYLOCK		"multiseq_primary_lock"
#define sSCRIPTKEYWORD_MULTISEQ_SECONDARYLOCK	"multiseq_secondary_lock"
#define sSCRIPTKEYWORD_MULTISEQ_PRIMARYLIST		"multiseq_primarylist"
#define sSCRIPTKEYWORD_MULTISEQ_SECONDARYLIST	"multiseq_secondarylist"
//
#define sSCRIPT_EXTENSION			".mvs"
//
///////////////////////////////////////////////////////////////////



LPCSTR	Script_GetExtension(void);
LPCSTR	Script_GetFilter(bool bStandAlone = true );
bool	Script_Write(LPCSTR psFullPathedFilename);
bool	Script_Read	(LPCSTR psFullPathedFilename);


#endif	// #ifndef SCRIPT_H

//////////////////// eof //////////////////////


