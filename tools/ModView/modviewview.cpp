// ModViewView.cpp : implementation of the CModViewView class
//

#include "stdafx.h"
#include "includes.h"
#include <mmsystem.h>  // for timeGettime()

#include "ModView.h"
#include "ModViewDoc.h"
#include "ModViewTreeView.h"
#include "TEXT.H"
#include "drag.h"
#include "wintalk.h"
#include "textures.h"
#include "sof2npcviewer.h"	// for gallery stuff
#include "clipboard.h"		// for clipboard
//
#include "ModViewView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool DraggingMouse();

int g_iScreenWidth = 0;
int g_iScreenHeight = 0;
int g_iViewAreaMouseX = 0;
int g_iViewAreaMouseY = 0;

/////////////////////////////////////////////////////////////////////////////
// CModViewView

IMPLEMENT_DYNCREATE(CModViewView, CView)

BEGIN_MESSAGE_MAP(CModViewView, CView)
	//{{AFX_MSG_MAP(CModViewView)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModViewView construction/destruction

void testcode(void);
CModViewView::CModViewView()
{
	testcode();
}

CModViewView::~CModViewView()
{
}

BOOL CModViewView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}


/////////////////////////////////////////////////////////////////////////////
// CModViewView drawing

void CModViewView::OnDraw(CDC* pDC)
{
	CModViewDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_hRC && m_hDC)
	{
		if (wglMakeCurrent(m_hDC,m_hRC))
		{	
			ModelList_Render(m_iWindowWidth, m_iWindowDepth);

			SwapBuffers(pDC->GetSafeHdc());
//			VERIFY(wglMakeCurrent(m_hDC,NULL));
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// CModViewView printing

BOOL CModViewView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CModViewView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CModViewView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CModViewView diagnostics

#ifdef _DEBUG
void CModViewView::AssertValid() const
{
	CView::AssertValid();
}

void CModViewView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CModViewDoc* CModViewView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CModViewDoc)));
	return (CModViewDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CModViewView message handlers


BOOL CModViewView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	m_hRC = 0;	// rendering context
	m_hDC = 0;	// device context
	m_iWindowWidth = m_iWindowDepth = 0;
	m_TimerHandle_Update100FPS = 0;
	
	return CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

int CModViewView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	AppVars.hWnd = m_hWnd;
	
	m_hDC = ::GetDC(m_hWnd);

	m_hRC = GL_GenerateRC(m_hDC, true );	// bool bDoubleBuffer

	m_TimerHandle_Update100FPS = SetTimer(	th_100FPS,	// UINT nIDEvent, 
											10,				// UINT nElapse, (in milliseconds)
											NULL				// void (CALLBACK EXPORT* lpfnTimer)(HWND, UINT, UINT, DWORD) 
										);

	if (!m_TimerHandle_Update100FPS)
	{
		ErrorBox("Warning: no Timer available for CModViewView update!");
	}

	OnceOnly_GLVarsInit();
	
	return 0;
}

// app shutdown
void CModViewView::OnDestroy() 
{
	AppVars.bFinished = true;	// may be useful

	Media_Delete();

	if (m_hRC)		// if we had an opengl rendering context for this window
	{
		wglDeleteContext(m_hRC);
		m_hRC = NULL;
	}

	if (m_hDC)
	{
		::ReleaseDC(m_hWnd,m_hDC);
		m_hDC = NULL;
	}

	if (m_TimerHandle_Update100FPS)
	{
		bool bOk = !!KillTimer(m_TimerHandle_Update100FPS);	// NZ return if good
		if (bOk)
		{
			m_TimerHandle_Update100FPS = NULL;
		}
		else
		{
			ErrorBox("Error killing timer!");	// should never happen
		}
	}

	CView::OnDestroy();
}

void CModViewView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	m_iWindowWidth = cx;
	m_iWindowDepth = cy;	

	g_iScreenWidth = cx;
	g_iScreenHeight= cy;
}


BOOL CModViewView::OnEraseBkgnd(CDC* pDC) 
{
	// actually this looks nicer to not do it at all, so...
	//
	/*
//	return CView::OnEraseBkgnd(pDC);

	CBrush backBrush(RGB(AppVars._R, AppVars._G, AppVars._B));      // Save old brush
	CBrush* pOldBrush = pDC->SelectObject(&backBrush);

	CRect rect;
	pDC->GetClipBox(&rect);     // Erase the area needed
	pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATCOPY); 
	
	pDC->SelectObject(pOldBrush); 
	  */
	return TRUE;   	
}



int Sys_Milliseconds (void)
{
	static bool bInitialised = false;
	static int sys_timeBase;

	int sys_curtime;

	if (!bInitialised)
	{
		sys_timeBase = timeGetTime();
		bInitialised = true;
	}
	sys_curtime = timeGetTime() - sys_timeBase;

	return sys_curtime;
}

float GetFloatTime(void)
{
	float fTime  = (float)Sys_Milliseconds() / 1000.0f;	// reduce to game-type seconds

	return fTime;
}


// should be called roughly at 100 times per second... (which is enough for our needs when lerping is factored in)
//
int giGalleryItemsRemaining = 0;	// declared here so StatusMessage can reference it (which is pretty tacky.... again).
CString strGalleryErrors;
CString strGalleryWarnings;
CString strGalleryInfo;

void Gallery_AddError(LPCSTR psText)
{
	strGalleryErrors += psText;
}
void Gallery_AddInfo(LPCSTR psText)
{
	strGalleryInfo += psText;
}
void Gallery_AddWarning(LPCSTR psText)
{
	strGalleryWarnings += psText;
}

//
void CModViewView::OnTimer(UINT nIDEvent) 
{
	if (nIDEvent != th_100FPS)
	{
		CView::OnTimer(nIDEvent);
		return;
	}
	

	// otherwise, it's one of our timer events, so...


	{ // new bit, poll the remote control stuff 10 times per second (it's also done in OnIdle(), but that's not always fast enough when animating)
		static	float fTime = 0.0f;
				float fTimeNow = GetFloatTime();

		#define UPDATE_FRAMES_PER_SECOND 10.0f

		if (fTimeNow - fTime > 1.0f/UPDATE_FRAMES_PER_SECOND)
		{
			fTime = fTimeNow;
//			OutputDebugString(va("Time = %f seconds\n",GetFloatTime()));

			if (WinTalk_HandleMessages())
			{
				// app exit requested
				AppVars.bAnimate = qfalse;	// groan... stop the animation so the app doesn't spend all it's time
											//				in the render loop. This allows the App::OnIdle() version
											//				of the Wintalk_HandleMessages() handler to get a look in,
											//				and therefore spot that an exit is being requested.
			}
		}
	}

//	if (!DraggingMouse()) 
	{
		if (ModelList_Animation())
		{
			// one or more models have updated frames (or lerping)...
			//
			Invalidate(false);
		}
	}


	if (Gallery_Active())
	{
		extern bool gbInRenderer;
		if (!gbInRenderer)
		{
			static bool bAlreadyHere = false;
			if (!bAlreadyHere)	// jic.
			{
				bAlreadyHere = true;

				extern int giRenderCount;
				static CString strCaption;
				static CString strScript;

				static bool bSnapshotTakingPlace = false;
				if (!bSnapshotTakingPlace)
				{				
					int iRemainingPlusOne = GalleryRead_ExtractEntry(strCaption, strScript);
					if (iRemainingPlusOne)	// because 0 would be fail/empty
					{
						giGalleryItemsRemaining = iRemainingPlusOne-1;
						StatusMessage( va("( Gallery: %d remaining )", giGalleryItemsRemaining) );
						OutputDebugString(va("\"%s\" (script len %d)\n",(LPCSTR)strCaption,strScript.GetLength()));

						strScript += "\n";			

						string strOutputFileName( va("%s\\%s",scGetTempPath(),"temp.mvs") );
							
						int iReturn = SaveFile(strOutputFileName.c_str(),(LPCSTR)strScript, strScript.GetLength());
						if (iReturn != -1)
						{
							extern bool Document_ModelLoadPrimary(LPCSTR psFilename);
							if (Document_ModelLoadPrimary(strOutputFileName.c_str()))
							{
								if (Model_Loaded())
								{
									ModelHandle_t hModel = AppVars.Container.hModel;

									Model_Sequence_Lock( hModel, Gallery_GetSeqToLock(), true, false);
								}

								giRenderCount = 0;
								bSnapshotTakingPlace = true;
							}
						}
					}
					else
					{
						// all done...
						//
						gbTextInhibit = false;
						Gallery_Done();
						StatusMessage( NULL );
						//
						// report...
						//
						CString strReport;

						if (!strGalleryErrors.IsEmpty())
						{
							strReport += "====================== Errors: ===================\n\n";
							strReport += strGalleryErrors;
							strReport += "\n\n";
						}

						if (!strGalleryWarnings.IsEmpty())
						{
							strReport += "====================== Warnings: ===================\n\n";
							strReport += strGalleryWarnings;
							strReport += "\n\n";
						}

						if (!strGalleryInfo.IsEmpty())
						{
							strReport += "====================== Info: ===================\n\n";
							strReport += strGalleryInfo;
							strReport += "\n\n";
						}

						if (!strReport.IsEmpty())
						{
							strReport.Insert(0,"The following messages appeared during gallery-snapshots....\n\n");
						}
						else
						{
							strReport = va("All gallery-snapshots done\n\nOutput dir was: \"%s\\n",Gallery_GetOutputDir());
						}

						SendStringToNotepad(strReport,"gallery_report.txt");
					}
				}
				else
				{
					if (giRenderCount == 2)	// ... so it's rendered to back buffer for snapshot, and front for user
					{	
						//
						// generate a filename...
						//				
						char sOutputFileName[MAX_PATH];
						CString strBaseName(strCaption);
						while (strBaseName.Replace("\t"," "));
						while (strBaseName.Replace("  "," "));
						sprintf(sOutputFileName, "%s\\%s.bmp",Gallery_GetOutputDir(),strBaseName);
						ScreenShot(sOutputFileName,/*strCaption*/strBaseName);
						BMP_Free();			

						bSnapshotTakingPlace = false;	// trigger next snapshot
					}
					else
					{
						Invalidate(false);	// cause another screen update until render count satisfied
					}
				}

				bAlreadyHere = false;
			}
		}
	}
}




bool sys_rbuttondown = false;
bool sys_lbuttondown = false;
bool sys_mbuttondown = false;
bool DraggingMouse()
{
	return !!(sys_rbuttondown || sys_lbuttondown || sys_mbuttondown);
}
POINT DragStartPoint;
void CModViewView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	GetCursorPos(&DragStartPoint);

	start_drag( (mkey_enum)nFlags, DragStartPoint.x, DragStartPoint.y );
	SetCapture();
	ShowCursor(false); 
	sys_lbuttondown = true;

//	CView::OnLButtonDown(nFlags, point);
}


void CModViewView::OnLButtonUp(UINT nFlags, CPoint point) 
{
//	CView::OnLButtonUp(nFlags, point);

	if (sys_lbuttondown) 
	{
		ReleaseCapture();
		ShowCursor( true ); 	
		end_drag( (mkey_enum)nFlags, point.x, point.y );
		sys_lbuttondown = false;
	}
}

void CModViewView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	GetCursorPos(&DragStartPoint);

	start_drag( (mkey_enum)nFlags, DragStartPoint.x, DragStartPoint.y );
	SetCapture();
	ShowCursor(false); 
	sys_rbuttondown = true;

//	CView::OnRButtonDown(nFlags, point);
}

void CModViewView::OnRButtonUp(UINT nFlags, CPoint point) 
{
//	CView::OnRButtonUp(nFlags, point);

	if (sys_rbuttondown) 
	{
		ReleaseCapture(); 
		ShowCursor( true ); 
		end_drag( (mkey_enum)nFlags, point.x, point.y );
		sys_rbuttondown = false;
	}
}

void CModViewView::OnMouseMove(UINT nFlags, CPoint point) 
{
	CView::OnMouseMove(nFlags, point);

	GetCursorPos(&point);

	if (!DraggingMouse()) 
	{
		ScreenToClient(&point);
		g_iViewAreaMouseX = point.x;
		g_iViewAreaMouseY = point.y;
		return;
	}


	static int _i=0;
//	OutputDebugString(va("(%d):   x:(%d) y:(%d)   (dragstart: %d %d)\n",_i++,point.x,point.y, DragStartPoint.x,DragStartPoint.y));

	if (drag( (mkey_enum)nFlags, point.x, point.y ))
	{
		SetCursorPos(DragStartPoint.x,DragStartPoint.y);		
		//OutputDebugString("drag-painting\n");
		Invalidate(false);
		//UpdateWindow();		
	}
}


BOOL CModViewView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	// wheel down = -ve, wheel up = +ve zDelta (test vals were +/-120, but just check sign)
	//
	OutputDebugString(va("zDelta = %d\n",zDelta));
	
	return CView::OnMouseWheel(nFlags, zDelta, pt);
}


typedef unsigned int MBChar_t;
MBChar_t *UTF8AsciiToMBC(LPCSTR psSource)
{		
	static vector <MBChar_t>	Out;
								Out.clear();
	typedef struct
	{
		byte bRangeStart, bRangeStop;		
		byte b1stByteAND;
		byte bBytesLong;
		byte b2ndRangeStart,b2ndRangeStop;
	} UTF8DecodeTable_t;

	static const UTF8DecodeTable_t UTF8DecodeTable[] =
	{
		{	0x00,	0x7F,	0x7F,	1,	0x00,0x00 },
		{	0xC0,	0xDF,	0x1F,	2,	0x80,0xBF },
		{	0xE0,	0xEF,	0x0F,	3,	0x80,0xBF },
		{	0xF0,	0xF7,	0x07,	4,	0x80,0xBF },
		{	0xF8,	0xFB,	0x03,	5,	0x80,0xBF },
		{	0xFC,	0xFD,	0x01,	6,	0x80,0xBF }
	};

	while (*psSource)
	{			
		byte b = *psSource++;

		MBChar_t m=0;

		for (int i=0; i< (sizeof(UTF8DecodeTable) / sizeof(UTF8DecodeTable[0])); i++)
		{
			if (b >= UTF8DecodeTable[i].bRangeStart && b <= UTF8DecodeTable[i].bRangeStop)
			{
				m = (b & UTF8DecodeTable[i].b1stByteAND);
				m <<= (UTF8DecodeTable[i].bBytesLong - 1)*6;

				// supplementary chars...
				//
				for (int j=0; j<UTF8DecodeTable[i].bBytesLong-1; j++)
				{
					b = *psSource++;
					if (b)
					{					
						const int iShift = (UTF8DecodeTable[i].bBytesLong-2)-j;
						m |= (b&63) << (iShift*6);
					}
					else
					{
						assert(0);	// bad UTF-8 stream, bail
						break;
					}
				}
				Out.push_back(m);
				break;
			}
		}
	}

	Out.push_back(0);

	return &Out[0];
}


void testcode(void)
{
	MBChar_t*	p = UTF8AsciiToMBC(va("%c",0x4D));
				p = UTF8AsciiToMBC(va("%c",0x79));
				p = UTF8AsciiToMBC(va("%c",0x20));
				p = UTF8AsciiToMBC(va("%c%c%c",0xE6,0xB2,0xB3));
				p = UTF8AsciiToMBC(va("%c%c%c",0xE8,0xB1,0x9A));

	int z=1;

}

