/**********************************************************************
	Copyright (c) 1999 - 2000 Immersion Corporation

	Permission to use, copy, modify, distribute, and sell this
	software and its documentation may be granted without fee;
	interested parties are encouraged to request permission from
		Immersion Corporation
		801 Fox Lane
		San Jose, CA 95131
		408-467-1900

	IMMERSION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
	INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
	IN NO EVENT SHALL IMMERSION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
	CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
	LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
	NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
	CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

  FILE:		FEELitIFR.h

  PURPOSE:	Input/Output for IFR Files, FEELit version

  STARTED:	

  NOTES/REVISIONS:

**********************************************************************/

#if !defined( _IMMIFR_H_)
#define _IMMIFR_H_

#ifndef __FEELITAPI_INCLUDED__
	#error include 'dinput.h' before including this file for structures.
#endif /* !__DINPUT_INCLUDED__ */

#define IFRAPI __stdcall

#if !defined(_IFCDLL_)
#define DLLAPI __declspec(dllimport)
#else
#define DLLAPI __declspec(dllexport)
#endif

#if defined __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
**  CONSTANTS
*/

/*
**  RT_IMMERSION - Resource type for IFR projects stored as resources.
**   This is the resource type looked for by IFLoadProjectResource().
*/
#define	RT_IMMERSION	((LPCSTR)"IMMERSION")


/*
**  TYPES/STRUCTURES
*/

/*
**  HIFRPROJECT - used to identify a loaded project as a whole.
**   individual objects within a project are uniquely referenced by name.
**   Created by the IFLoadProject*() functions and released by IFReleaseProject().
*/
typedef	LPVOID HIFRPROJECT;

/*
**  IFREffect - contains the information needed to create a DI effect
**   using IDirectInputEffect::CreateEffect(). An array of pointers to these
**	 structures is allocated and returned by IFCreateEffectStructs().
*/
typedef struct {
	GUID		guid;
	DWORD		dwIterations;
	char		*effectName;
	LPIMM_EFFECT	lpDIEffect;
} IFREffect;


/*
**  FUNCTION DECLARATIONS
*/

/*
**  IFLoadProjectResource() - Load a project from a resource.
**   hRsrcModule - handle of the module containing the project definition resource.
**   pRsrcName - name or MAKEINTRESOURCE(id) identifier of resource to load.
**   pDevice - device for which the project is being loaded. If NULL,
**     effects will be created generically, and IFCreateEffects() will fail.
**   Returns an identifier for the loaded project, or NULL if unsuccessful.
*/
DLLAPI
HIFRPROJECT
IFRAPI
IFRLoadProjectResource(
	HMODULE hRsrcModule,
	LPCSTR pRsrcName,
	LPIIMM_DEVICE pDevice );

/*
**  IFLoadProjectPointer() - Load a project from a pointer.
**   pProject - points to a project definition.
**   pDevice - device for which the project is being loaded. If NULL,
**     effects will be created generically, and IFCreateEffects() will fail.
**   Returns an identifier for the loaded project, or NULL if unsuccessful.
*/
DLLAPI
HIFRPROJECT
IFRAPI
IFRLoadProjectPointer(
	LPVOID pProject,
	LPIIMM_DEVICE pDevice );

/*
**  IFLoadProjectFile() - Load a project from a file.
**    pProjectFileName - points to a project file name.
**    pDevice - device for which the project is being loaded. If NULL,
**       effects will be created generically, and IFCreateEffects() will fail.
**    Returns an identifier for the loaded project, or NULL if unsuccessful.
*/
DLLAPI
HIFRPROJECT
IFRAPI
IFRLoadProjectFile(
	LPCSTR pProjectFileName,
	LPIIMM_DEVICE pDevice );

/*
**  IFRLoadProjectFromMemory() - Load a project from memory.
**
**    In cases where a file or resource is readily accessible, it may 
**	  be necessary to pass IFR formated information through memory. 
**
**    pProjectDef - memory addres that contains information from an IFR file.
**    pDevice - device for which the project is being loaded. If NULL,
**       effects will be created generically, and IFRCreateEffects() will fail.
**    Returns an identifier for the loaded project, or NULL if unsuccessful.
*/
DLLAPI 
HIFRPROJECT 
IFRAPI
IFRLoadProjectFromMemory( 
	LPVOID pProjectDef, 
	LPIIMM_DEVICE pDevice );

/*
**  IFLoadProjectObjectPointer() - Load a project from a pointer to a single
**     object definition (usually used only by the editor).
**   pObject - points to an object definition.
**   pDevice - device for which the project is being loaded. If NULL,
**     effects will be created generically, and IFCreateEffects() will fail.
**   Returns an identifier for the loaded project, or NULL if unsuccessful.
*/
DLLAPI
HIFRPROJECT
IFRAPI
IFRLoadProjectObjectPointer(
	LPVOID pObject,
	LPIIMM_DEVICE pDevice );

/*
**  IFReleaseProject() - Release a loaded project.
**   hProject - identifies the project to be released.
**   Returns TRUE if the project is released, FALSE if it is an invalid project.
*/
DLLAPI
BOOL
IFRAPI
IFRReleaseProject(
	HIFRPROJECT hProject );

/*
**  IFCreateEffectStructs() - Create IFREffects for a named effect.
**   hProject - identifies the project containing the object.
**   pObjectName - name of the object for which to create structures.
**   pNumEffects - if not NULL will be set to a count of the IFREffect
**     structures in the array (not including the terminating NULL pointer.)
**   Returns a pointer to the allocated array of pointers to IFREffect
**     structures. The array is terminated with a NULL pointer. If the
**     function fails, a NULL pointer is returned.
*/
DLLAPI
IFREffect **
IFRAPI
IFRCreateEffectStructs(
	HIFRPROJECT hProject,
	LPCSTR pObjectName,
	int *pNumEffects );

DLLAPI
IFREffect **
IFRAPI
IFRCreateEffectStructsByIndex(
	HIFRPROJECT hProject,
	int nObjectIndex,
	int *pNumEffects );

DLLAPI 
int 
IFRAPI
IFRGetNumEffects( 
	HIFRPROJECT hProject 
	);

DLLAPI
LPCSTR
IFRAPI
IFRGetObjectNameByIndex(
	HIFRPROJECT hProject,
	int nObjectIndex );

DLLAPI
LPCSTR
IFRAPI
IFRGetObjectSoundPath(
	HIFRPROJECT hProject,
	LPCSTR pObjectName );

DLLAPI
DWORD
IFRAPI
IFRGetObjectType(
	HIFRPROJECT hProject,
	LPCSTR pObjectName );

DLLAPI
DWORD
IFRAPI
IFRGetObjectTypeByIndex(
	HIFRPROJECT hProject,
	int nObjectIndex );

DLLAPI 
LPCSTR 
IFRAPI
IFRGetObjectNameByGUID(
	HIFRPROJECT hProject, 
	GUID *pGUID );

DLLAPI 
GUID 
IFRAPI
IFRGetObjectID( 
	HIFRPROJECT hProject, 
	LPCSTR pObjectName);

DLLAPI 
GUID* 
IFRAPI
IFRGetContainedObjIDs( 
	HIFRPROJECT hProject, 
	LPCSTR pCompoundObjName);


/*
**  IFReleaseEffectStructs() - Release an array of IFREffects.
**   hProject - identifies the project for which the effects were created.
**   pEffects - points to the array of IFREffect pointers to be released.
**   Returns TRUE if the array is released, FALSE if it is an invalid array.
*/
DLLAPI
BOOL
IFRAPI
IFRReleaseEffectStructs(
	HIFRPROJECT hProject,
	IFREffect **pEffects );

/*
**  IFCreateEffects() - Creates the DirectInput effects using
**     IDirectInput::CreateEffect().
**   hProject - identifies the project containing the object.
**   pObjectName - name of the object for which to create effects.
**   pNumEffects - if not NULL will be set to a count of the IDirectInputEffect
**     pointers in the array (not including the terminating NULL pointer.)
**   Returns a pointer to the allocated array of pointers to IDirectInputEffects.
**     The array is terminated with a NULL pointer. If the function fails,
**     a NULL pointer is returned.
*/
DLLAPI
LPIIMM_EFFECT *
IFRAPI
IFRCreateEffects(
	HIFRPROJECT hProject,
	LPCSTR pObjectName,
	int *pNumEffects );

/*
**  IFReleaseEffects() - Releases an array of IDirectInputEffect structures.
**   hProject - identifies the project for which the effects were created.
**   pEffects - points to the array if IDirectInputEffect pointers to be released.
**   Returns TRUE if the array is released, FALSE if it is an invalid array.
*/
DLLAPI
BOOL
IFRAPI
IFRReleaseEffects(
	HIFRPROJECT hProject,
	LPIIMM_EFFECT *pEffects );

#if defined __cplusplus
}
#endif /* __cplusplus */

#endif /* !IMMIFR_h */
