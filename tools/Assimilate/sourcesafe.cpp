// Filename:- sourcesafe.cpp
//
// (read the sourcesafe.h notes for extra info)
//
#include "stdafx.h"
#include "Includes.h"

// Microsoft's GUID stuff never seems to work properly, so...
//
#define DEFINE_GUID_THAT_WORKS(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#include <ocidl.h>
//#include "oddbits.h"
#include "ssauto.h"
#include "ssauterr.h"

#include "sourcesafe.h"

// SourceSafe can sometimes take a LONG time to return from calls to it, so in order to let users know that your app
//	hasn't crashed elsewhere it's a good idea to fill in these macros with something appropriate to your own code...
//
int iSourceSafeReferenceCount = 0;
void SourceSafe_Enter(LPCSTR psString)
{
//	if (!iSourceSafeReferenceCount++)
	{			
		CString str = psString;
				str.Insert(0,"SourceSafe: ");
		StatusText(va("%s\n",str));		
	}
}

void SourceSafe_Leave(void)
{
//	if (!--iSourceSafeReferenceCount)
	{
		//((CMainFrame*)AfxGetMainWnd())->StatusMessage
		StatusText("Ready\n");	
	}
}

#define _SS_ENTER(string)	SourceSafe_Enter(string)	//((CMainFrame*)AfxGetMainWnd())->StatusMessage("(Waiting for SourceSafe...)");	
#define _SS_LEAVE()			SourceSafe_Leave()			//((CMainFrame*)AfxGetMainWnd())->StatusMessage("Ready");	

// Coding note, generally the functions beginning "SS_" are called externally, and the ones beginning "_" are internal.
//
// Incidentally, the docs say:
//
// -----
// Be sure to link with the following libraries:
// user32.lib uuid.lib oleaut32.lib ole32.lib
// -----
//
//... but this appears not to be needed. I suspect stdafx.h may have something to do with that, so I'll leave 
//	that comment there in case this is ever used with standard windows code only
//



// replace these 3 with hacks to fit this module to BehavEd...
//
CString g_cstrSourceSafeINI;//		= sEF1_SS_INI;
CString g_cstrSourceSafeProject;//	= sEF1_SS_PROJECT;
bool	g_bUseSourceSafe = false;	//true;


void SS_SetString_Ini(LPCSTR psArg)
{
	if (g_bUseSourceSafe && g_cstrSourceSafeINI.CompareNoCase(psArg))
	{
		SS_Shutdown_OnceOnly();
	}

	g_cstrSourceSafeINI = psArg;
	g_bUseSourceSafe = (!g_cstrSourceSafeINI.IsEmpty() && !g_cstrSourceSafeProject.IsEmpty());
}
			
void SS_SetString_Project(LPCSTR psArg)
{
	if (g_bUseSourceSafe && g_cstrSourceSafeProject.CompareNoCase(psArg))
	{
		SS_Shutdown_OnceOnly();
	}

	g_cstrSourceSafeProject = psArg;
	g_bUseSourceSafe = (!g_cstrSourceSafeINI.IsEmpty() && !g_cstrSourceSafeProject.IsEmpty());
}

// remember to do a SysFreeString(<BSTR>) on the returned value when you've finished with it!
//
BSTR StringToBSTR(LPCSTR string)
{
	OLECHAR* svalue		= NULL;
	BSTR	 bstrValue  = NULL;
	int		 iLen;
	bool	 bFree_svalue = false;

	if( (iLen = MultiByteToWideChar(CP_ACP, 0, string, -1, svalue, 0 )) != 1)
	{
		svalue = new OLECHAR[iLen];
		bFree_svalue = true;
		if( MultiByteToWideChar(CP_ACP, 0, string, -1, svalue, iLen ) == 0 )
		{
			ErrorBox( va("StringToBSTR: Error converting \"%s\" to BSTR", string));
		}
	}
	else
	{
		svalue = L"";
	}
	
	bstrValue = SysAllocString(svalue);

	if (svalue && bFree_svalue)
		delete svalue;

	return bstrValue;
}


// thanks for making us jump through yet more hoops, Microsoft...
//
// (actually there is a "legal" way to do this, in a similar way to above, but I can be arsed looking it up)
//
LPCSTR BSTRToString(BSTR pStrOLE_MyName)
{
	static CString str;
	str.Empty();

	WORD *pw = (WORD *) pStrOLE_MyName;

	char c;
	do
	{
		c = LOBYTE(*pw++);
		str+=va("%c",c);	// hahahaha!
	}
	while (c);

	return (LPCSTR) str;
}


IClassFactory *g_pClf = NULL;
IVSSDatabase  *g_pVdb = NULL;
BSTR bstrPath   = NULL;
BSTR bstrUName	= NULL;
BSTR bstrUPass	= NULL;
BSTR bstrProject= NULL;
//
bool gbDatabaseHeldOpen = false;

bool _OpenDatabase(bool bQuietErrors = false);	// spot the retrofit <g>
bool _OpenDatabase(bool bQuietErrors)
{
	if (gbDatabaseHeldOpen)
		return true;

	static bool bFirstTime = false;
	if (!bFirstTime)
	{
		bFirstTime = true;
		SS_Startup_OnceOnly();
	}

	_SS_ENTER("Opening...");
	
	bool bReturn = false;
	CLSID clsid;	
	
	bstrPath	= StringToBSTR	( (LPCSTR) g_cstrSourceSafeINI );  // eg "\\\\RAVEND\\VSS\\SRCSAFE.INI"
	bstrUName	= SysAllocString(L"");	// user name (eg) "scork" or "guest", but best left blank for auto.
	bstrUPass	= SysAllocString(L"");	// user password, again leave blank
	bstrProject	= StringToBSTR	( (LPCSTR) g_cstrSourceSafeProject );	// eg. "$/StarTrek/BaseQ3/Maps"

	CoInitialize(0);
	if(S_OK == CLSIDFromProgID(L"SourceSafe", &clsid ))
	{
		if(S_OK == CoGetClassObject( clsid, CLSCTX_ALL, NULL, IID_IClassFactory, (void**)&g_pClf ))
		{
			if(S_OK == g_pClf->CreateInstance( NULL, IID_IVSSDatabase, (void **) &g_pVdb ))
			{
				if(S_OK == g_pVdb->Open(bstrPath, bstrUName, bstrUPass))
				{
					if (S_OK == g_pVdb->put_CurrentProject(bstrProject))
					{
						gbDatabaseHeldOpen = true;
						bReturn = true;
					}
					else
					{
						if (!bQuietErrors)
							ErrorBox( va("SourceSafe: Failed to set current project to \"%s\"",g_cstrSourceSafeProject));
					}
				}
				else
				{
					if (!bQuietErrors)
						ErrorBox( va("SourceSafe: Failed during open of INI file \"%s\"",g_cstrSourceSafeINI));
				}
			}
			else
			{
				if (!bQuietErrors)
					ErrorBox( "SourceSafe: Failed to create a database instance");
			}
		}
		else
		{
			if (!bQuietErrors)
				ErrorBox( "SourceSafe: Failed to get ClassFactory interface");
		}
	}
	else
	{
		if (!bQuietErrors)
			ErrorBox( "SourceSafe: Failed to get CLSID (GUID)");
	}

	_SS_LEAVE();

	return bReturn;
}

// this should be called regardless of whether or not the _OpenDatabase function succeeded because it could
//	have got part way through and only error'd later, so this frees up what it needs to
//
void _CloseDatabase()
{
	if (gbDatabaseHeldOpen)
		return;

	_SS_ENTER("Closeing...");

	if (g_pVdb)
	{
		g_pVdb->Release();
		g_pVdb = NULL;
	}

	if (g_pClf)
	{
		g_pClf->Release();
		g_pClf = NULL;
	}

	CoUninitialize();
	
	// no need to check for NULL ptr for this function...
	//
	SysFreeString(bstrPath);	bstrPath	= NULL;
	SysFreeString(bstrUName);	bstrUName	= NULL;
	SysFreeString(bstrUPass);	bstrUPass	= NULL;
	SysFreeString(bstrProject);	bstrProject	= NULL;

	_SS_LEAVE();
}



// there isn't really any totally legal way to do this because Radiant doesn't have projects with files added to
//	sourcesafe through menus etc in the same way as (eg) VC, so I'll just do a bit of a kludge for now. This should
//	be updated for any other projects/files you want to handle yourself...
//
// currently I'm expecting files like:
//
//		"Q:\\quake\\baseq3\\scripts\\borg.shader"
//
// .. which I convert by matching the "baseq3" part to produce:
//
//		"$/StarTrek/BaseQ3/Shaders/borg3.shader"
//
// returns NULL on fail.
//

// BEHAVED NOTE:
//
// scripts pathnames are in usually in the form	:	"Q:\\quake\\baseq3\\real_scripts\\oz\\myborg.txt"
// sourcesafe pathnames  are in the form		:	"$/StarTrek/real_scripts" downwards
//
// ... so I just need to search for the common part of the path ("real_scripts")...
//
LPCSTR FilenameToProjectName(LPCSTR path, bool bQuiet = false);
LPCSTR FilenameToProjectName(LPCSTR path, bool bQuiet)
{
	static CString  strPath;
					strPath = path;	// do NOT join to above line (or it only happens first time)

	strPath.Replace("\\","/");

	int loc = strPath.Find("/models/");
	if (loc>=0)
	{
		strPath = strPath.Mid(loc+1);	// reduce to "models/players/blah/blah.car"
	}
	else
	{
		if (!bQuiet)
		{
			ErrorBox( va("SourceSafe: FilenameToProjectName(): Failed to convert \"%s\" to SS path",path));
		}
		return NULL;
	}

	strPath.Insert(0,va("%s",(LPCSTR) g_cstrSourceSafeProject));

	return (LPCSTR) strPath;
}

// return is how many versions were listed, AFAIK it should always be at least 1 (ie "created") for any valid SS item
//
int _ListVersions( IVSSDatabase* db, LPCSTR psPathSS, CString &strOutput )
{	
	_SS_ENTER("ListVersions...");

#define MAX_VERSIONS_TO_LIST 40	// otherwise these things get fucking *huge*!
	int	 iVersions = 0;
	BSTR bstrval;
	char lpbuf[200];
	char lpbuf2[200];
	
	IVSSItem     *vssi;
	IVSSVersion  *vers;
	IVSSVersions *vx;
	LPUNKNOWN    lpunk;
	IEnumVARIANT *ppvobj;
	VARIANT      st;
	BSTR         bstrValue;
	int          x;
	ULONG        fetched;
	long         lvnum;	
	
	HRESULT	hr;

	strOutput = "";	

	bstrValue = StringToBSTR(psPathSS);		
	
	if( S_OK == db->get_VSSItem(bstrValue, FALSE, &vssi) )
	{
		if( S_OK == vssi->get_Versions( 0l, &vx ) )
		{
			if( S_OK == vx->_NewEnum(&lpunk) )
			{
				if(!FAILED(lpunk->QueryInterface(IID_IEnumVARIANT, (void**)&ppvobj)))
				{
					vssi->get_Spec( &bstrval );
					x = WideCharToMultiByte( CP_ACP, 0, (LPCWSTR)bstrval, -1,
						lpbuf, sizeof(lpbuf), NULL, NULL );

//					strOutput = va("History of: %s\n", lpbuf );
//					OutputDebugString(va("History of: %s\n", lpbuf ));
//					strOutput+= "ACTION   USER NAME   VERSION NUMBER\n";
//					OutputDebugString("ACTION   USER NAME   VERSION NUMBER\n");
					
					do
					{
						ppvobj->Next( 1UL, &st, &fetched );
						if( fetched != 0 )
						{
							if(!FAILED(hr = st.punkVal->QueryInterface(IID_IVSSVersion,(void**)&vers)))
							{
								vers->get_Action( &bstrval );
								WideCharToMultiByte( CP_ACP, 0, (LPCWSTR)bstrval, -1,
									lpbuf, sizeof(lpbuf), NULL, NULL );
								vers->get_Username( &bstrval );
								WideCharToMultiByte( CP_ACP, 0, (LPCWSTR)bstrval, -1,
									lpbuf2, sizeof( lpbuf2 ), NULL, NULL );
								vers->get_VersionNumber( &lvnum );
//								OutputDebugString(va("%s  %s  %ld\n", lpbuf, lpbuf2, lvnum ));

								// version numbers can go a bit weird, in that global labels have different version
								//	numbers, but since it's all presented in a backwards order I'd have to do a whole
								//	bunch of messy stuff putting them into arrays then scanning them from last to first
								//	to check version numbers were sequential, then zap any that weren't, so in the end
								//	I'm just going to to ignore version numbers for labels...
								//
								if (!strnicmp(lpbuf,"labeled",7))
								{
									strOutput += va("%s by user %s\n", lpbuf, lpbuf2, lvnum );
								}
								else
								{
									strOutput += va("%s by user %s  (Ver. %ld)\n", lpbuf, lpbuf2, lvnum );
								}

								iVersions++;
								
								vers->Release();
							}
							else
							{
								ErrorBox( va("SourceSafe: _ListVersions(): Failed in QueryInterface(IID_IVSSVersion) for file \"$s\"",psPathSS ));					
							}
							st.punkVal->Release();
						}
					} while( fetched != 0 
							#ifdef MAX_VERSIONS_TO_LIST
							&& iVersions < MAX_VERSIONS_TO_LIST
							#endif
							);
					ppvobj->Release();
				}
				else
				{
					ErrorBox( va("SourceSafe: _ListVersions(): Failed in QueryInterface(IID_IEnumVARIANT) for file \"$s\"",psPathSS ));					
				}
				lpunk->Release();
			}
			else
			{
				ErrorBox( va("SourceSafe: _ListVersions(): Failed in _NewEnum() for file \"$s\"",psPathSS ));
			}
			vx->Release();
		}
		else
		{
			ErrorBox( va("SourceSafe: _ListVersions(): Failed in get_Versions() for file \"$s\"",psPathSS ));
		}
		vssi->Release();
	}
	else
	{
		ErrorBox( va("SourceSafe: _ListVersions(): Failed in get_VSSItem() for file \"$s\"",psPathSS ));
	}
	SysFreeString(bstrValue);

	#ifdef MAX_VERSIONS_TO_LIST
	if (iVersions >= MAX_VERSIONS_TO_LIST)
	{
		strOutput += va("\n\n(History list capped to %d entries)",MAX_VERSIONS_TO_LIST);
	}
	#endif

	_SS_LEAVE();

	return iVersions;
}


// int return is number of users who have this file checked out, unless param 'psNameToCheckForMatch' is NZ, 
//	in which case return is a bool as to whether or not the name in question (usually our name) has the file checked out...
//
// ( output list is CR-delineated )
//
// return is -1 for error, else checkout count, else 1 for true if doing a me-match via last param
//
int _ListCheckOuts( IVSSDatabase* db, LPCSTR psPathSS, CString &strOutput, LPCSTR psNameToCheckForMatch )
{
	_SS_ENTER("ListCheckOuts...");

	int iCheckOuts = -1;
	BSTR bstrval;
	char lpbuf[200];
	char lpbuf2[200];
	
	IVSSItem     *vssi;
	IVSSVersion  *vers;
	IVSSCheckouts *vx;
	LPUNKNOWN    lpunk;
	IEnumVARIANT *ppvobj;
	VARIANT      st;
	BSTR         bstrValue;
	int          x;
	ULONG        fetched;
	
	HRESULT	hr;

	strOutput = "";

	bstrValue = StringToBSTR( psPathSS );		
	
	if( S_OK == db->get_VSSItem( bstrValue, FALSE, &vssi ))
	{
		if( S_OK == vssi->get_Checkouts( &vx ) )
		{
			if( S_OK == vx->_NewEnum(&lpunk) )
			{
				if(!FAILED(lpunk->QueryInterface(IID_IEnumVARIANT, (void**)&ppvobj)))
				{
					vssi->get_Spec( &bstrval );
					x = WideCharToMultiByte( CP_ACP, 0, (LPCWSTR)bstrval, -1, lpbuf, sizeof(lpbuf), NULL, NULL );

//					OutputDebugString(va("Checkout status of: %s\n", lpbuf ));
//					strOutput = va("Checkout status of: %s\n", lpbuf ));

					do
					{
						ppvobj->Next( 1UL, &st, &fetched );
						if( fetched != 0 )
						{
							if(!FAILED(hr = st.punkVal->QueryInterface(IID_IVSSCheckout,(void**)&vers)))
							{
//								vers->get_Action( &bstrval );
//								WideCharToMultiByte( CP_ACP, 0, (LPCWSTR)bstrval, -1,
//									lpbuf, sizeof(lpbuf), NULL, NULL );
								vers->get_Username( &bstrval );
								WideCharToMultiByte( CP_ACP, 0, (LPCWSTR)bstrval, -1,
									lpbuf2, sizeof( lpbuf2 ), NULL, NULL );
//								vers->get_VersionNumber( &lvnum );

//								OutputDebugString(va("%s  %s  %ld\n", lpbuf, lpbuf2, lvnum ));
								//OutputDebugString(va("%s\n", lpbuf2));
								strOutput += va("%s\n", lpbuf2);

								if ( psNameToCheckForMatch )
								{
									if (!stricmp(lpbuf2,psNameToCheckForMatch) )	// is stricmp too paranoid?
									{
										iCheckOuts = 1;	// hardwire: checked out by me = true
									}
								}
								else
								{
									if (iCheckOuts == -1)	// first time - change error-flag to valid count?
									{
										iCheckOuts = 1;
									}
									else
									{
										iCheckOuts++;
									}
								}
								
								vers->Release();
							}
							else
							{
								ErrorBox( va("SourceSafe: _ListCheckOuts(): Failed in QueryInterface(IID_IVSSCheckout) for file \"$s\"",psPathSS ));
							}
							st.punkVal->Release();
						}
					} while( fetched != 0 );
					ppvobj->Release();
				}
				else
				{
					ErrorBox( va("SourceSafe: _ListCheckOuts(): Failed in QueryInterface(IID_IEnumVARIANT) for file \"$s\"",psPathSS ));					
				}
				lpunk->Release();
			}
			else
			{
				ErrorBox( va("SourceSafe: _ListCheckOuts(): Failed in _NewEnum() for file \"$s\"",psPathSS ));
			}
			vx->Release();
		}
		else
		{
			ErrorBox( va("SourceSafe: _ListCheckOuts(): Failed in get_Checkouts() for file \"$s\"",psPathSS ));
		}
		vssi->Release();
	}
	else
	{
		ErrorBox( va("SourceSafe: _ListCheckOuts(): Failed in get_VSSItem() for file \"$s\"",psPathSS ));
	}
	SysFreeString(bstrValue);

	_SS_LEAVE();

	return iCheckOuts;
}


bool _IsCheckedOut( IVSSDatabase* db, LPCSTR psPathSS )
{
	_SS_ENTER("Is checked out?...");

	IVSSItem*	vssi;
	bool		bReturn		= false;
	BSTR		bstrValue	= StringToBSTR( psPathSS );

	if( S_OK == db->get_VSSItem( bstrValue, FALSE, &vssi ) )
	{		
		long lStatus;
		if( S_OK == vssi->get_IsCheckedOut( &lStatus ) )
		{
			bReturn = !!lStatus;
		}
		else
		{
			ErrorBox( va("SourceSafe: Error during _IsCheckedOut() for file \"%s\"",psPathSS ));
		}
		vssi->Release();
	}
	else
	{
		ErrorBox( va( "SourceSafe: _IsCheckedOut(): Error during get_VSSItem() for file \"%s\"",psPathSS ));
	}

	SysFreeString( bstrValue );
	
	_SS_LEAVE();

	return bReturn;	
}

bool _CheckOut( IVSSDatabase* db, LPCSTR psPathSS, LPCSTR psPathDisk )
{
	_SS_ENTER("Check Out...");

	IVSSItem*	vssi;
	bool		bReturn		= false;
	BSTR		bstrValue	= StringToBSTR	( psPathSS );
	BSTR		bsComment	= SysAllocString( L"" );

	if( S_OK == db->get_VSSItem( bstrValue, FALSE, &vssi ) )
	{
		BSTR bsLocal = StringToBSTR(psPathDisk);		

		if( S_OK == vssi->Checkout( bsComment, bsLocal, 0 ))
		{
			bReturn = true;
		}
		else
		{
			ErrorBox( va( "SourceSafe: Error during _CheckOut() for file \"%s\"",psPathSS ));
		}
		
		SysFreeString( bsLocal );

		vssi->Release();		
	}
	else
	{
		ErrorBox( va( "SourceSafe: _CheckOut(): Error during get_VSSItem() for file \"%s\"",psPathSS ));
	}
	
	SysFreeString( bsComment );
	SysFreeString( bstrValue );

	_SS_LEAVE();

	return bReturn;	
}

bool _Add( IVSSDatabase* db, LPCSTR psPathSS, LPCSTR psPathDisk )
{
	_SS_ENTER("Add...");

	IVSSItem	*vssi,*vssi2;
	bool		bReturn		= false;
	BSTR		bstrValue	= StringToBSTR	( psPathSS );
	BSTR		bsComment	= SysAllocString( L"" );

	CString		strPathRelativeToProject = psPathSS;
				strPathRelativeToProject.Replace("/","\\");				
				Filename_RemoveFilename(strPathRelativeToProject);
				strPathRelativeToProject.Replace("\\","/");
				
				// ( eg: "$/StarTrek/BaseQ3/real_scripts/ste" )

	BSTR		bstrProject = StringToBSTR(strPathRelativeToProject);

	// ensure SS path exists...
	//
	CString strBuiltUpProject;
	CString strSourceProject(strPathRelativeToProject);
	while (!strSourceProject.IsEmpty())
	{
		CString strProjectPathSoFar(strBuiltUpProject);
		CString strNewSubProj;
		int iLoc = strSourceProject.Find("/");
		if (iLoc >=0)
		{				
			strNewSubProj = strSourceProject.Left(iLoc);
		}
		else
		{
			strNewSubProj = strSourceProject;
							strSourceProject = "";
		}
		strBuiltUpProject += strBuiltUpProject.IsEmpty() ? "" : "/";
		strBuiltUpProject += strNewSubProj;
		strSourceProject = strSourceProject.Mid(iLoc+1);

		if (strBuiltUpProject != "$")	// get_VSSItem() doesn't work on root, and it'll always be there anyway
		{	
			BSTR bstrTempProjectPath = StringToBSTR(strBuiltUpProject);
			{
				if( S_OK != db->get_VSSItem( bstrTempProjectPath, FALSE, &vssi ) )
				{
					// need to build this new part of the path...
					//
					BSTR bstrProjectPathSoFar = StringToBSTR(strProjectPathSoFar);
					{
						if( S_OK == db->get_VSSItem( bstrProjectPathSoFar, FALSE, &vssi ) )
						{
							BSTR bstrNewSubProj = StringToBSTR(strNewSubProj);
							{					
								if (S_OK == vssi->NewSubproject(bstrNewSubProj, bsComment, &vssi2))
								{
									// yay!
								}
								else
								{
									ErrorBox(va("SS_Add(): Error creating subproject \"%s\" in \"%s\"!",(LPCSTR) strNewSubProj, (LPCSTR) strProjectPathSoFar));
									break;
								}
							}
							SysFreeString( bstrNewSubProj );
						}
					}
					SysFreeString( bstrProjectPathSoFar );
				}
			}
			SysFreeString( bstrTempProjectPath );
		}
	}
			
	if( S_OK == db->get_VSSItem( bstrProject, FALSE, &vssi ) )
	{
		BSTR bsLocal = StringToBSTR( psPathDisk );
			
	//	if( S_OK == vssi->Checkin ( bsComment, bsLocal, 0 ))
		if( S_OK == vssi->Add	  ( bsLocal, bsComment, 0, &vssi2))
		{
			vssi2->Release();
			bReturn = true;
		}
		else
		{
			ErrorBox( va( "SourceSafe: Error during _Add() for file \"%s\"",psPathSS ));
		}		
			
		SysFreeString( bsLocal );

		vssi->Release();
	}
	else
	{
		ErrorBox( va( "SourceSafe: _CheckIn(): Error during get_VSSItem() for file \"%s\"",psPathSS ));
	}

	SysFreeString( bsComment );
	SysFreeString( bstrValue );

	_SS_LEAVE();

	return bReturn;	
}


bool _CheckIn( IVSSDatabase* db, LPCSTR psPathSS, LPCSTR psPathDisk )
{
	_SS_ENTER("Check In...");

	IVSSItem*	vssi;
	bool		bReturn		= false;
	BSTR		bstrValue	= StringToBSTR	( psPathSS );		
	BSTR		bsComment	= SysAllocString( L"" );

	if( S_OK == db->get_VSSItem( bstrValue, FALSE, &vssi ) )
	{
		BSTR bsLocal = StringToBSTR( psPathDisk );
		
		if( S_OK == vssi->Checkin ( bsComment, bsLocal, 0 ))
		{
			bReturn = true;
		}
		else
		{
			ErrorBox( va( "SourceSafe: Error during _CheckIn() for file \"%s\"",psPathSS ));
		}		
		
		SysFreeString( bsLocal );

		vssi->Release();
	}
	else
	{
		ErrorBox( va( "SourceSafe: _CheckIn(): Error during get_VSSItem() for file \"%s\"",psPathSS ));
	}

	SysFreeString( bsComment );
	SysFreeString( bstrValue );

	_SS_LEAVE();

	return bReturn;	
}



bool _UndoCheckOut( IVSSDatabase* db, LPCSTR psPathSS )
{
	_SS_ENTER("Undo Check out...");

	IVSSItem*	vssi;
	bool		bReturn		= false;
	BSTR		bstrValue	= StringToBSTR	( psPathSS );		
	BSTR		bsComment	= SysAllocString( L"" );

	if( S_OK == db->get_VSSItem(bstrValue, FALSE, &vssi ) )
	{
		BSTR bsLocal = SysAllocString( L"" );	// this doesn't seem to need a path to the disk file, presumably it works it out

		if( S_OK == vssi->UndoCheckout( bsLocal, 0 ))		
		{
			bReturn = true;
		}
		else
		{
			ErrorBox( va("SourceSafe: Error during _UndoCheckOut() for file \"%s\"",psPathSS));
		}
		
		SysFreeString(bsLocal);

		vssi->Release();
	}
	else
	{
		ErrorBox( va( "SourceSafe: _UndoCheckOut(): Error during get_VSSItem() for file \"%s\"",psPathSS ));
	}

	SysFreeString( bsComment );
	SysFreeString( bstrValue );

	_SS_LEAVE();

	return bReturn;
}



// these routines called externally...
//


bool SS_IsUnderSourceControl( LPCSTR psPathedFilename )
{
	CWaitCursor waitcursor;
	bool bReturn = false;

	if ( SS_FunctionsAvailable() )
	{
		LPCSTR psSSName = FilenameToProjectName(psPathedFilename, true);	// bQuiet

		if (psSSName )
		{
			if (_OpenDatabase() )
			{
				_SS_ENTER("Checking if item is under control");	

				IVSSItem*	vssi;
				BSTR		bstrValue	= StringToBSTR	( psSSName );		

				if( S_OK == g_pVdb->get_VSSItem( bstrValue, FALSE, &vssi ) )
				{
					vssi->Release();
					bReturn = true;
				}
				else
				{
					// item doesn't appear to be under SS control
				}
				
				SysFreeString( bstrValue );				

				_SS_LEAVE();	
			}
			_CloseDatabase();
		}
	}

	return bReturn;
}

bool SS_Add(LPCSTR psPathedFilename)
{	
	CWaitCursor waitcursor;
	
	bool bReturn = false;
	
	if ( SS_FunctionsAvailable() )
	{
		LPCSTR psSSName = FilenameToProjectName(psPathedFilename);
		
		if ( psSSName )
		{
			if ( _OpenDatabase() )
			{
				if (_Add( g_pVdb, psSSName, psPathedFilename ) )				
				{
					bReturn = true;
				}
			}
			_CloseDatabase();
		}
	}
	
	return bReturn;
}

bool SS_CheckIn(LPCSTR psPathedFilename)
{
	CWaitCursor waitcursor;

	bool bReturn = false;

	if ( SS_FunctionsAvailable() )
	{
		LPCSTR psSSName = FilenameToProjectName(psPathedFilename);

		if ( psSSName )
		{
			if ( _OpenDatabase() )
			{
				if ( _CheckIn( g_pVdb, psSSName, psPathedFilename ) )
				{
					bReturn = true;
				}				
			}
			_CloseDatabase();
		}
	}

	return bReturn;
}

bool SS_CheckOut( LPCSTR psPathedFilename )
{
	CWaitCursor waitcursor;
	bool bReturn = false;

	if ( SS_FunctionsAvailable() )
	{
		LPCSTR psSSName = FilenameToProjectName(psPathedFilename);

		if ( psSSName )
		{
			if ( _OpenDatabase() )
			{
				if ( _CheckOut( g_pVdb, psSSName, psPathedFilename ) )
				{
					bReturn = true;
				}				
			}
			_CloseDatabase();
		}
	}

	return bReturn;
}

bool SS_UndoCheckOut( LPCSTR psPathedFilename )
{
	CWaitCursor waitcursor;
	bool bReturn = false;

	if ( SS_FunctionsAvailable() )
	{
		LPCSTR psSSName = FilenameToProjectName(psPathedFilename);

		if ( psSSName )
		{
			if ( _OpenDatabase() )
			{
				if ( _UndoCheckOut( g_pVdb, psSSName ) )
				{
					bReturn = true;
				}				
			}
			_CloseDatabase();
		}
	}

	return bReturn;
}

bool SS_IsCheckedOut( LPCSTR psPathedFilename )
{
	CWaitCursor waitcursor;
	bool bReturn = false;

	ASSERT( SS_FunctionsAvailable() );	// you shouldn't really be calling this otherwise

	if ( SS_FunctionsAvailable() )
	{
		LPCSTR psSSName = FilenameToProjectName(psPathedFilename);

		if ( psSSName )
		{
			if ( _OpenDatabase() )
			{
				if ( _IsCheckedOut( g_pVdb, psSSName ) )
				{
					bReturn = true;
				}				
			}
			_CloseDatabase();
		}
	}

	return bReturn;
}

// similar to above, but we're only interested if it's checked out by us...
//
// (actually after I wrote this I found a more legal way to do it, but this works...)
//
bool SS_IsCheckedOutByMe( LPCSTR psPathedFilename )
{
	CWaitCursor waitcursor;
	bool bReturn = false;

	ASSERT( SS_FunctionsAvailable() );	// you shouldn't really be calling this otherwise

	if ( SS_FunctionsAvailable() )
	{
		LPCSTR psSSName = FilenameToProjectName( psPathedFilename );

		if ( psSSName )
		{
			if ( _OpenDatabase() )
			{
				_SS_ENTER("Is checked out by me?");

				CString junk;

				BSTR pStrOLE_MyName;
				if (S_OK == g_pVdb->get_UserName(&pStrOLE_MyName))
				{					
					int iCount = _ListCheckOuts( g_pVdb, psSSName, junk, BSTRToString( pStrOLE_MyName) );

					if ( iCount == 1)
					{
						bReturn = true;
					}
				}
				else
				{
					ErrorBox( "SourceSafe: SS_IsCheckedOutByMe(): Failed in get_UserName()" );
				}				

				_SS_LEAVE();
			}
			_CloseDatabase();
		}
	}
	
	return bReturn;
}

bool SS_ListVersions( LPCSTR psPathedFilename, CString &strOutput )
{
	CWaitCursor waitcursor;
	bool bReturn = false;

	if ( SS_FunctionsAvailable() )
	{
		LPCSTR psSSName = FilenameToProjectName( psPathedFilename );

		if ( psSSName )
		{
			if ( _OpenDatabase() )
			{				
				if ( _ListVersions( g_pVdb, psSSName, strOutput ))
				{
					bReturn = true;
				}				
			}
			_CloseDatabase();
		}
	}

	return bReturn;
}

// if false (error) return, don't rely on a valid iCount!
//
bool SS_ListCheckOuts( LPCSTR psPathedFilename, CString &strOutput, int &iCount )
{
	CWaitCursor waitcursor;
	bool bReturn = false;

	ASSERT( SS_FunctionsAvailable() );	// you shouldn't really be calling this otherwise

	if ( SS_FunctionsAvailable() )
	{
		LPCSTR psSSName = FilenameToProjectName( psPathedFilename );

		if ( psSSName )
		{
			if ( _OpenDatabase() )
			{
				iCount = _ListCheckOuts( g_pVdb, psSSName, strOutput, NULL );

				if (iCount != -1)
				{
					bReturn = true;	// strictly speaking this may not be true, but if       (later: hmmmm, what was i going to write here?   <g>)
				}				
			}
			_CloseDatabase();
		}
	}

	return bReturn;
}


bool SS_FunctionsAvailable( void )
{
	bool bReturn = false;

	if ( g_bUseSourceSafe )
	{	
		if (_OpenDatabase(true))	// bool bQuietErrors
		{
			bReturn = true;			
		}
		_CloseDatabase();
	}

	return bReturn;
}

// this is a pretty tacky, but for now...  :-)
//
bool SS_SetupOk( void )
{
	CWaitCursor waitcursor;
	bool g_bUseSourceSafe_PREV = g_bUseSourceSafe;
								 g_bUseSourceSafe = true;

	bool bReturn = SS_FunctionsAvailable();

	g_bUseSourceSafe = g_bUseSourceSafe_PREV;

	return bReturn;
}




void SS_Startup_OnceOnly(void)
{
	// nothing at the moment...
}

// this is now also called when switching databases
void SS_Shutdown_OnceOnly(void)
{
	gbDatabaseHeldOpen = false;
	_CloseDatabase();
}


/////////////////// eof /////////////////

