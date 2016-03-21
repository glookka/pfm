#include "pch.h"

#include "LPanel/pbase.h"
#include "LCore/clog.h"
#include "LCore/cos.h"
#include "LSettings/sconfig.h"
#include "LSettings/scolors.h"
#include "LCrypt/cservices.h"
#include "LFile/fmisc.h"
#include "LPanel/presources.h"
#include "LPanel/pfile.h"
#include "LDialogs/dregister.h"
#include "FileManager/filemanager.h"

#include "aygshell.h"
#include "Resources/resource.h"

Array_T <Panel_c *> Panel_c::m_dPanels;
WNDPROC	Panel_c::m_pOldBtnProc = NULL;

extern HINSTANCE	g_hAppInstance;
extern HWND			g_hMainWindow;

static const wchar_t g_szPanelClass [] = L"PanelClass";
Str_c g_sNoDir = L"";

///////////////////////////////////////////////////////////////////////////////////////////
// common panel view
Panel_c::Panel_c  ()
	: m_bActive			( false )
	, m_hWnd			( NULL )
	, m_hBtnMax			( NULL )
	, m_hBtnChangeTo	( NULL )
	, m_bMaximized		( false )
	, m_bInMenu			( false )
	, m_pConfigMaximized( NULL )
	, m_hMemBitmap		( NULL )
	, m_hMemDC			( NULL )
{
	memset ( &m_BmpSize, 0, sizeof ( m_BmpSize ) );
	memset ( &m_tRect, 0, sizeof ( m_tRect ) );
	memset ( &m_tMaxMidBtnRect, 0, sizeof ( m_tMaxMidBtnRect ) );
	memset ( &m_tChangeToBtnRect, 0, sizeof ( m_tChangeToBtnRect ) );
}


Panel_c::~Panel_c ()
{
	DestroyWindow ( m_hWnd );
	DeleteObject ( m_hMemBitmap );
	DeleteDC ( m_hMemDC );
}


bool Panel_c::Init ()
{
	WNDCLASS tWC;
	tWC.style			= 0;
	tWC.lpfnWndProc		= (WNDPROC) Panel_c::WndProc;
	tWC.cbClsExtra		= 0;
	tWC.cbWndExtra		= 0;
	tWC.hInstance		= g_hAppInstance;
	tWC.hIcon			= NULL;
	tWC.hCursor			= NULL;
	tWC.hbrBackground	= (HBRUSH) GetStockObject ( DKGRAY_BRUSH );
	tWC.lpszMenuName	= NULL;
	tWC.lpszClassName	= g_szPanelClass;

	return !!RegisterClass ( &tWC );
}


void Panel_c::Shutdown ()
{
	UnregisterClass ( g_szPanelClass, g_hAppInstance );

	for ( int i = 0; i < m_dPanels.Length (); ++i )
		delete m_dPanels [i];
}


bool Panel_c::IsAnyPaneInMenu ()
{
	for ( int i = 0; i < m_dPanels.Length (); ++i )
		if ( m_dPanels [i]->IsInMenu () )
			return true;

	return false;
}

bool Panel_c::IsLayoutVertical ()
{
	if ( g_tConfig.pane_force_layout == -1 )
	{
		int iScrWidth, iScrHeight;
		GetScreenResolution ( iScrWidth, iScrHeight );
		return iScrWidth < iScrHeight;
	}
	else
		return g_tConfig.pane_force_layout == 0;
}


LRESULT CALLBACK Panel_c::WndProc ( HWND hWnd, UINT Msg, UINT wParam, LONG lParam )
{
	Panel_c * pPanel = NULL;

	for ( int i = 0; i < m_dPanels.Length () && ! pPanel; ++i )
		if ( m_dPanels [i]->m_hWnd == hWnd )
			pPanel = m_dPanels [i];

	if ( ! pPanel )
		return DefWindowProc ( hWnd, Msg, wParam, lParam ); 

	bool bCallRegDlg = false;
	static int actioncounter = 0;
	static bool bShown = false;

	switch ( Msg )
	{
	case WM_PAINT:
		{
			PAINTSTRUCT tPS;
			HDC hDC = ::BeginPaint ( hWnd, &tPS );

			if ( g_tConfig.double_buffer )
			{
				HDC hPaintDC = pPanel->BeginPaint ( hDC );
				pPanel->OnPaint ( hPaintDC );
				pPanel->EndPaint ( hDC, tPS );
			}
			else
				pPanel->OnPaint ( hDC );

			::EndPaint ( hWnd, &tPS );
		}
		break;

	case WM_ENTERMENULOOP:
		pPanel->EnterMenu ();
		break;

	case WM_EXITMENULOOP:
		pPanel->ExitMenu ();
		break;

	case WM_ERASEBKGND:
		++actioncounter;
		if ( actioncounter >= reg::interval && ! reg::registered && ! bShown )
		{
			bCallRegDlg = true;
			break;
		}
		return TRUE;

	case WM_LBUTTONDOWN:
		pPanel->OnLButtonDown ( LOWORD(lParam), HIWORD(lParam) );
		return 0;

	case WM_LBUTTONUP:
		pPanel->OnLButtonUp ( LOWORD(lParam), HIWORD(lParam) );
		return 0;

	case WM_MOUSEMOVE:
		pPanel->OnStyleMove ( LOWORD(lParam), HIWORD(lParam) );
		return 0;

	case WM_VSCROLL:
		pPanel->OnVScroll ( int ( HIWORD ( wParam ) ), int ( LOWORD ( wParam ) ), (HWND) lParam );
		return 0;

	case WM_MEASUREITEM:
		if ( pPanel->OnMeasureItem ( (int) wParam, ( MEASUREITEMSTRUCT*) lParam ) )
			return TRUE;
		break;

	case WM_DRAWITEM:
		if ( pPanel->OnDrawItem ( (int) wParam, (DRAWITEMSTRUCT*) lParam ) )
			return TRUE;
		break;

	case WM_COMMAND:
		if ( pPanel->OnCommand ( wParam, lParam ) )
			return 0;
		break;
	}

#if FM_NAG_SCREEN
	if ( bCallRegDlg )
	{
		ShowRegisterDialog ( g_hMainWindow );
		bShown = true;
	}
	return FALSE;
#endif
    
	return DefWindowProc ( hWnd, Msg, wParam, lParam );
}


LRESULT CALLBACK Panel_c::BtnProc ( HWND hWnd, UINT Msg, UINT wParam, LONG lParam )
{
	Assert ( m_pOldBtnProc );

	if ( Msg == WM_CAPTURECHANGED  )
		SetFocus ( g_hMainWindow );

	return CallWindowProc ( m_pOldBtnProc, hWnd, Msg, wParam, lParam );
}

Panel_c * Panel_c::CreatePanel ( PanelType_e eType, bool bTop )
{
	Panel_c * pResult = NULL;
	switch ( eType )
	{
	case PANEL_FILES:
		{
			FilePanel_c * pPanel = new FilePanel_c ();

			Str_c * pPath		= bTop ? &g_tConfig.file_pane_1_path : &g_tConfig.file_pane_2_path;
			int * pMode			= bTop ? &g_tConfig.file_pane_1_sort : &g_tConfig.file_pane_2_sort;
			bool * pReverse		= bTop ? &g_tConfig.file_pane_1_sort_reverse : &g_tConfig.file_pane_2_sort_reverse;
			int * pViewMode		= bTop ? &g_tConfig.file_pane_1_view : &g_tConfig.file_pane_2_view;
			bool * pMaximized	= bTop ? &g_tConfig.pane_1_maximized : &g_tConfig.pane_2_maximized;
			
			// send pointers so panel can write to config file
			pPanel->Config_Maximized ( pMaximized );

			pPanel->Config_Path ( pPath );
			pPanel->Config_SortMode ( pMode );
			pPanel->Config_SortReverse ( pReverse );
			pPanel->Config_ViewMode ( pViewMode );

			pPanel->UpdateAfterLoad ();
			pResult = pPanel;
		}
		break;

	default:
		Achtung ( L"Unknown panel type!" );
		break;
	}

	m_dPanels.Add ( pResult );

	return pResult;
}


void Panel_c::UpdateAfterLoad ()
{
	Maximize ( *m_pConfigMaximized );
}


void Panel_c::SetRect ( const RECT & tRect )
{
	Assert ( m_hWnd );

	bool bVisible = !!IsWindowVisible ( m_hWnd );
	SetWindowPos ( m_hWnd, g_hMainWindow, tRect.left, tRect.top, tRect.right - tRect.left, tRect.bottom - tRect.top, bVisible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW );
	GetClientRect ( m_hWnd, &m_tRect );

	int iHeight = GetHeaderViewHeight ();

	m_tMaxMidBtnRect.left	= m_tRect.right - g_tResources.m_tBmpMaxMidSize.cx;
	m_tMaxMidBtnRect.right	= m_tRect.right;
	m_tMaxMidBtnRect.top	= m_tRect.top;
	m_tMaxMidBtnRect.bottom	= m_tRect.top + iHeight;

	SetWindowPos ( m_hBtnMax, m_hWnd, m_tMaxMidBtnRect.left, m_tMaxMidBtnRect.top,
		m_tMaxMidBtnRect.right - m_tMaxMidBtnRect.left, iHeight, SWP_SHOWWINDOW );

	m_tChangeToBtnRect.right  = m_tMaxMidBtnRect.left - 1;
	m_tChangeToBtnRect.left	  = m_tChangeToBtnRect.right - g_tResources.m_tBmpChangeToSize.cx;
	m_tChangeToBtnRect.top	  = m_tRect.top;
	m_tChangeToBtnRect.bottom = m_tRect.top + iHeight;

	if ( m_bMaximized )
	{
		SetWindowPos ( m_hBtnChangeTo, m_hWnd, m_tChangeToBtnRect.left, m_tChangeToBtnRect.top,
			m_tChangeToBtnRect.right - m_tChangeToBtnRect.left, iHeight, SWP_SHOWWINDOW );
	}
	else
		ShowWindow ( m_hBtnChangeTo, SW_HIDE );
}


bool Panel_c::IsActive () const
{
	return m_bActive;
}


void Panel_c::Activate ( bool bActive )
{
	bool bOldActive = IsActive ();
	m_bActive = bActive;
	if ( bOldActive != IsActive () )
		InvalidateHeader ();
}


int Panel_c::GetMinHeight () const
{
	return 0;
}


int Panel_c::GetMinWidth () const
{
	return 0;
}


void Panel_c::Config_Maximized ( bool * pMaximized )
{
	m_pConfigMaximized = pMaximized;
}


void Panel_c::EnterMenu ()
{
	m_bInMenu = true;
}

void Panel_c::ExitMenu ()
{
	m_bInMenu = false;
}

bool Panel_c::IsInMenu () const
{
	return m_bInMenu;
}

bool Panel_c::IsClickAndHoldDetected ( int iX, int iY ) const
{
	SHRGINFO tGesture;
	tGesture.cbSize        = sizeof(SHRGINFO);
	tGesture.hwndClient    = m_hWnd;
	tGesture.ptDown.x      = iX;
	tGesture.ptDown.y      = iY;
	tGesture.dwFlags       = SHRG_RETURNCMD;

	return SHRecognizeGesture ( &tGesture ) == GN_CONTEXTMENU;
}


void Panel_c::Show ( bool bShow )
{
	Maximize ( false );

	ShowWindow ( m_hWnd, bShow ? SW_SHOWNORMAL : SW_HIDE );
	if ( ! bShow )
		ResetTemporaryStates ();
}

void Panel_c::Maximize ( bool bMaximize )
{
	if ( bMaximize == m_bMaximized )
		return;

	m_bMaximized = bMaximize;
	*m_pConfigMaximized = bMaximize;
	InvalidateRect ( m_hBtnMax, NULL, FALSE );

	if ( m_bMaximized )
		InvalidateRect ( m_hBtnChangeTo, NULL, FALSE );

	OnMaximize ();
}

bool Panel_c::IsMaximized () const
{
	return m_bMaximized;
}

const Str_c & Panel_c::GetDirectory () const
{
	return g_sNoDir;
}

void Panel_c::GetLeftmostBtnRect ( RECT & tRect )
{
	tRect = m_bMaximized ? m_tChangeToBtnRect : m_tMaxMidBtnRect;
}

void Panel_c::OnLButtonDown ( int iX, int iY )
{
	FMActivatePanel ( this );
}


void Panel_c::OnPaint ( HDC hDC )
{
	// setup the font
	SetBkMode ( hDC, TRANSPARENT );
	SetTextColor ( hDC, g_tColors.pane_font );

	HGDIOBJ hOldFont = SelectObject ( hDC, g_tPanelFont.GetHandle () );
	HGDIOBJ hOldPen = SelectObject ( hDC, g_tResources.m_hBorderPen );

	DrawPanelHeader ( hDC );

	// restore
	SelectObject ( hDC, hOldFont );
	SelectObject ( hDC, hOldPen );
}


bool Panel_c::OnCommand ( WPARAM wParam, LPARAM lParam )
{
	if ( (HWND) lParam == m_hBtnMax && HIWORD ( wParam ) == BN_CLICKED )
	{
		if ( m_bMaximized )
			FMNormalPanels ();
		else
			FMMaximizePanel ( this );

		return true;
	}

	if ( (HWND) lParam == m_hBtnChangeTo && HIWORD ( wParam ) == BN_CLICKED )
	{
		FMMaximizePanel ( FMGetPassivePanel () );
		return true;
	}

	return false;
}


bool Panel_c::OnDrawItem ( int iId, const DRAWITEMSTRUCT * pDrawItem )
{
	Assert ( pDrawItem );

    if ( pDrawItem->CtlType == ODT_BUTTON )
	{		
		if ( pDrawItem->hwndItem == m_hBtnMax )
		{
			if ( m_bMaximized )
				DrawButton ( pDrawItem, m_tMaxMidBtnRect, g_tResources.m_tBmpMaxMidSize, g_tResources.m_hBmpBtnMid, g_tResources.m_hBmpBtnMidPressed );
			else
				DrawButton ( pDrawItem, m_tMaxMidBtnRect, g_tResources.m_tBmpMaxMidSize, g_tResources.m_hBmpBtnMax, g_tResources.m_hBmpBtnMaxPressed );

			return true;
		}

		if ( pDrawItem->hwndItem == m_hBtnChangeTo && m_bMaximized )
		{
			if ( FMGetPanel1 () == this )
				DrawButton ( pDrawItem, m_tChangeToBtnRect, g_tResources.m_tBmpChangeToSize, g_tResources.m_hBmpChangeTo2, g_tResources.m_hBmpChangeTo2Pressed );
			else
				DrawButton ( pDrawItem, m_tChangeToBtnRect, g_tResources.m_tBmpChangeToSize, g_tResources.m_hBmpChangeTo1, g_tResources.m_hBmpChangeTo1Pressed );

			return true;
		}
	}

	return false;
}

HDC	Panel_c::BeginPaint ( HDC hDC )
{
	if ( g_tConfig.double_buffer )
	{
		RECT WinRect;
		GetWindowRect ( m_hWnd, &WinRect );

		bool bCreate = true;
		if ( m_hMemDC )
		{
			if ( ( m_BmpSize.cx == WinRect.right - WinRect.left ) && ( m_BmpSize.cy == WinRect.bottom - WinRect.top ) )
				bCreate = false;
		}

		if ( bCreate )
		{
			if ( m_hMemBitmap )
				DeleteObject ( m_hMemBitmap );

			if ( m_hMemDC )
				DeleteDC ( m_hMemDC );

			m_hMemDC = CreateCompatibleDC ( hDC );
			m_hMemBitmap = CreateCompatibleBitmap ( hDC, WinRect.right - WinRect.left, WinRect.bottom - WinRect.top );
			SelectObject ( m_hMemDC, m_hMemBitmap );
			m_BmpSize.cx = WinRect.right - WinRect.left;
			m_BmpSize.cy = WinRect.bottom - WinRect.top;
		}

		return m_hMemDC;
	}
	else
		return hDC;
}

void Panel_c::EndPaint ( HDC hDC, const PAINTSTRUCT & PS )
{
	if ( g_tConfig.double_buffer )
		BitBlt ( hDC, PS.rcPaint.left, PS.rcPaint.top, PS.rcPaint.right - PS.rcPaint.left, PS.rcPaint.bottom - PS.rcPaint.top, m_hMemDC, PS.rcPaint.left, PS.rcPaint.top, SRCCOPY );
}

HWND Panel_c::CreateButton ()
{
	HWND hButton = CreateWindow ( L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, NULL, g_hAppInstance, NULL );
	Assert ( hButton );
	if ( ! m_pOldBtnProc )
		m_pOldBtnProc = (WNDPROC) GetWindowLong  ( hButton, GWL_WNDPROC );

	SetWindowLong ( hButton, GWL_WNDPROC, (LONG) BtnProc );

	return hButton;
}

void Panel_c::DrawButton ( const DRAWITEMSTRUCT * pDrawItem, const RECT & tBtnRect, const SIZE & tBmpSize, HBITMAP hBmp, HBITMAP hBmpPressed ) const
{
	FillRect ( pDrawItem->hDC, &pDrawItem->rcItem, IsActive () ? g_tResources.m_hActiveHdrBrush : g_tResources.m_hPassiveHdrBrush );

	bool bPressed = pDrawItem->itemState & ODS_SELECTED;

	SelectObject ( g_tResources.m_hBitmapDC, bPressed ? hBmpPressed : hBmp );

	int iOffsetX = ( tBtnRect.right - tBtnRect.left - tBmpSize.cx ) / 2;
	int iOffsetY = ( tBtnRect.bottom - tBtnRect.top - tBmpSize.cy ) / 2;

	TransparentBlt ( pDrawItem->hDC, iOffsetX, iOffsetY, g_tResources.m_tBmpDirSize.cx, tBmpSize.cy,
			g_tResources.m_hBitmapDC, 0, 0, tBmpSize.cx, tBmpSize.cy, g_tColors.pane_button_transparent );
}


void Panel_c::CreateDrawObjects ()
{
	m_hWnd = CreateWindow ( g_szPanelClass, L"", WS_CHILD | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, g_hMainWindow, NULL, g_hAppInstance, NULL );
	Assert ( m_hWnd );

	// create buttons
	m_hBtnMax = CreateButton ();
	m_hBtnChangeTo = CreateButton ();
}


void Panel_c::DestroyDrawObjects ()
{
	DestroyWindow ( m_hBtnMax );
	DestroyWindow ( m_hBtnChangeTo );
}


void Panel_c::DrawPanelHeader ( HDC hDC )
{
	RECT tHeader;
	GetHeaderRect ( tHeader );

	// header background
	FillRect ( hDC, &tHeader, IsActive () ? g_tResources.m_hActiveHdrBrush : g_tResources.m_hPassiveHdrBrush );

	// title line
	MoveToEx ( hDC, m_tRect.left, tHeader.bottom, NULL );
	LineTo ( hDC, m_tRect.right, tHeader.bottom );
}


int	Panel_c::GetHeaderViewHeight () const
{
	return Max ( g_tPanelFont.GetHeight () + 2, g_tResources.m_tBmpMaxMidSize.cy, g_tResources.m_tBmpChangeToSize.cy );
}

void Panel_c::GetHeaderRect ( RECT & tRect ) const
{
	tRect = m_tRect;
	tRect.bottom = m_tRect.top + GetHeaderViewHeight ();
}

void Panel_c::GetHeaderViewRect ( RECT & tRect ) const
{
	tRect.left	 = m_tRect.left;
	tRect.right	 = m_tMaxMidBtnRect.left;
	tRect.top	 = m_tRect.top;
	tRect.bottom = m_tRect.top + GetHeaderViewHeight ();
}


void Panel_c::InvalidateHeader ()
{
	RECT tRect;
	GetHeaderRect ( tRect );
	InvalidateRect ( m_hWnd, & tRect, FALSE );
	InvalidateRect ( m_hBtnMax, NULL, FALSE );

	if ( m_bMaximized )
		InvalidateRect ( m_hBtnChangeTo, NULL, FALSE );
}

void Panel_c::RedrawWindow ()
{
	::RedrawWindow ( m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE );
}

// init the panels
void Init_Panels ()
{
	g_tPanelFont.Init ();
	Panel_c::Init ();
}

// shutdown the panels
void Shutdown_Panels ()
{
	g_tPanelFont.Shutdown ();
	g_tResources.Shutdown ();

	Panel_c::Shutdown ();
}