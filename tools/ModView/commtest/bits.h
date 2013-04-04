// Filename:-	bits.h
//
// some leftover stuff to get this compiling, 
//	mainly from model.h in modview but I couldn't include that directly here


// NOTE:!!!!  It's ok for this to be ot of sync with the version in modview, there's no communication of the
//	actual values. This is important to realise, even if this is only a test app.



#ifndef BITS_H
#define BITS_H



typedef int ModelHandle_t;
typedef struct
{
	union
	{
		struct
		{
			unsigned int iItemType		: 8;	// allows 256 item types (see #defines below)
			unsigned int iModelHandle	: 8;	// allows 256 models
			unsigned int iItemNumber	: 16;	// allows 65536 surfaces, bones, sequences etc
		};
		//
		UINT32 uiData;
	};
} TreeItemData_t;


// max 256 of these...
//
typedef enum
{
	TREEITEMTYPE_NULL=0,			// nothing, ie usually a reasonable default for clicking on emptry tree space
	TREEITEMTYPE_MODELNAME,			// "modelname"
	TREEITEMTYPE_SURFACEHEADER,		// "surfaces"
	TREEITEMTYPE_BONEHEADER,		// "bones"
	TREEITEMTYPE_BONEALIASHEADER,	// "bone aliases"
	TREEITEMTYPE_SEQUENCEHEADER,	// "sequences"
	TREEITEMTYPE_BOLTONSHEADER,		// "BoltOns"
	//
	// Ones beyond here should have updated code in ModelTree_GetItemText() to handle pure enquiries if nec.
	//
	TREEITEMTYPE_GLM_SURFACE,		// a surface	(index in bottom bits, currently allows 65535 surfaces)
	TREEITEMTYPE_GLM_BONE,			// a bone		(index in bottom bits, currently allows 65535 bones)
	TREEITEMTYPE_GLM_BONEALIAS,		// a bone alias	(index in bottom bits, currently allows 65535 aliases)
	TREEITEMTYPE_SEQUENCE,			// a sequence	(index in bottom bits, currently allows 65535 bones)

} TreeTypes_e;


#define GetYesNo(psQuery)	(!!(AfxMessageBox(psQuery,						MB_YESNO|MB_ICONWARNING|MB_TASKMODAL)==IDYES))

#endif	// #ifndef BITS_H

///////////////////////// eof /////////////////////

