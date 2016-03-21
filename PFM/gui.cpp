#include "pch.h"
#include "gui.h"

#include "config.h"
#include "resources.h"
#include "pfm.h"
#include "dlg_enums.inc"

#include "aygshell.h"
#include "Dlls/Resource/resource.h"

extern HINSTANCE g_hAppInstance;

CustomControl_c::WndProcs_t CustomControl_c::m_WndProcs;

static const wchar_t g_szCustomCtlClass [] = L"CustomCtlClass";

//////////////////////////////////////////////////////////////////////////
CustomControl_c::CustomControl_c ()
	: m_hWnd		( NULL )
	, m_hParent		( NULL )
	, m_fnWndProc	( NULL )
{
}

CustomControl_c::~CustomControl_c ()
{
	m_WndProcs.Delete ( m_hWnd );
	if ( ! m_fnWndProc )
		DestroyWindow ( m_hWnd );
}

LRESULT CustomControl_c::WndProc ( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam )
{
	CustomControl_c ** ppCtl = m_WndProcs.Find ( hWnd );
	if ( ! ppCtl )
		return DefWindowProc ( hWnd, Msg, wParam, lParam );

	CustomControl_c * pCtl = *ppCtl;

	switch ( Msg )
	{
	case WM_PAINT:
		{
			PAINTSTRUCT tPS;
			HDC hDC = BeginPaint ( hWnd, &tPS );
			pCtl->OnPaint ( hDC, tPS.rcPaint );
			EndPaint ( hWnd, &tPS );
		}
		return TRUE;

	case WM_ERASEBKGND:
		if ( pCtl->OnEraseBkgnd () )
			return true;
		break;

	case WM_SETCURSOR:
		pCtl->OnSetCursor ();
		break;

	case WM_LBUTTONDOWN:
		pCtl->OnLButtonDown ( (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam) );
		return 0;

	case WM_LBUTTONUP:
		pCtl->OnLButtonUp ( (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam) );
		return 0;

	case WM_MOUSEMOVE:
		pCtl->OnMouseMove ( (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam) );
		return 0;
	}

	return pCtl->m_fnWndProc ? CallWindowProc ( pCtl->m_fnWndProc, hWnd, Msg, wParam, lParam ) : DefWindowProc ( hWnd, Msg, wParam, lParam );
}

void CustomControl_c::Create ( HWND hParent )
{
	m_hParent = hParent;

	Assert ( !m_hWnd );
	m_hWnd = CreateWindow ( g_szCustomCtlClass, L"", WS_CHILD | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hParent, NULL, g_hAppInstance, NULL );
	Assert ( m_hWnd );

	Verify ( m_WndProcs.Add ( m_hWnd, this ) );
	OnCreate ();
}

void CustomControl_c::Subclass ( HWND hControl )
{
	m_hWnd = hControl;
	m_fnWndProc = (WNDPROC) SetWindowLong ( hControl, GWL_WNDPROC, LONG ( WndProc ) );
	Verify ( m_WndProcs.Add ( hControl, this ) );
	OnCreate ();
}

void CustomControl_c::Show ( bool bShow )
{
	ShowWindow ( m_hWnd, bShow ? SW_SHOWNORMAL : SW_HIDE );
}

HWND CustomControl_c::Handle () const
{
	return m_hWnd;
}

bool CustomControl_c::Init ()
{
	WNDCLASS tWC;
	tWC.style			= 0;
	tWC.lpfnWndProc		= (WNDPROC) CustomControl_c::WndProc;
	tWC.cbClsExtra		= 0;
	tWC.cbWndExtra		= 0;
	tWC.hInstance		= g_hAppInstance;
	tWC.hIcon			= NULL;
	tWC.hCursor			= NULL;
	tWC.hbrBackground	= NULL;
	tWC.lpszMenuName	= NULL;
	tWC.lpszClassName	= g_szCustomCtlClass;

	return !!RegisterClass ( &tWC );
}

void CustomControl_c::Shutdown ()
{
	UnregisterClass ( g_szCustomCtlClass, g_hAppInstance );
}


//////////////////////////////////////////////////////////////////////////

//
// the splitter bar
//
SplitterBar_c::SplitterBar_c ()
	: m_bVertical		( false )
	, m_iOffset			( 0 )
	, m_iMin			( 0 )
	, m_iMax			( INT_MAX )
	, m_uBorderColor	( GetSysColor ( COLOR_WINDOWFRAME ) )
	, m_uFillColor		( GetSysColor ( COLOR_BTNFACE) )
	, m_uPressedColor	( GetSysColor ( COLOR_WINDOWFRAME ) )
	, m_hBorderPen		( NULL )
	, m_hPressedBrush	( NULL )
	, m_hFillBrush		( NULL )
{
}

SplitterBar_c::~SplitterBar_c ()
{
	DestroyDrawObjects ();
}

void SplitterBar_c::SetMinMax ( int iMin, int iMax )
{
	m_iMin = iMin;
	m_iMax = iMax;
}

void SplitterBar_c::SetRect ( const RECT & tRect )
{
	Assert ( m_hWnd );

	bool bVisible = !!IsWindowVisible ( m_hWnd );

	SetWindowPos ( m_hWnd, m_hParent, tRect.left, tRect.top, tRect.right - tRect.left, tRect.bottom - tRect.top, bVisible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW );
	m_tRect = tRect;
	GetClientRect ( m_hWnd, &m_tClientRect );
	m_bVertical = tRect.bottom - tRect.top > tRect.right - tRect.left;
}

void SplitterBar_c::GetRect ( RECT & tRect ) const
{
	tRect = m_tRect;
}

void SplitterBar_c::OnCreate ()
{
	CreateDrawObjects ();
}

bool SplitterBar_c::OnEraseBkgnd ()
{
	return true;
}

void SplitterBar_c::OnPaint ( HDC hDC, const RECT & Rect )
{
	HGDIOBJ hObj1 = SelectObject ( hDC, m_hBorderPen );
	HGDIOBJ hObj2 = SelectObject ( hDC, GetCapture () == m_hWnd ?  m_hPressedBrush : m_hFillBrush );
	
	Rectangle ( hDC, m_tClientRect.left, m_tClientRect.top, m_tClientRect.right, m_tClientRect.bottom );

	SelectObject ( hDC, hObj1 );
	SelectObject ( hDC, hObj2 );
}

void SplitterBar_c::OnLButtonDown ( int iX, int iY )
{
	if ( GetCapture () != m_hWnd )
	{
		SetCapture ( m_hWnd );
		m_iOffset = m_bVertical ? m_tClientRect.left - iX : m_tClientRect.top - iY;
		InvalidateRect ( m_hWnd, NULL, FALSE );
	}
}

void SplitterBar_c::OnLButtonUp ( int iX, int iY )
{
	if ( GetCapture () == m_hWnd )
	{
		ReleaseCapture ();
		InvalidateRect ( m_hWnd, NULL, FALSE );
	}
}

void SplitterBar_c::OnMouseMove ( int iX, int iY )
{
	if ( GetCapture () == m_hWnd )
	{
		RECT tRect = m_tRect;
		if ( m_bVertical )
		{
			int iWidth = tRect.right - tRect.left;
			tRect.left  += m_iOffset + iX;
			tRect.right = tRect.left + iWidth;
		}
		else
		{
			int iHeight = tRect.bottom - tRect.top;
			tRect.top  += m_iOffset + iY;
			tRect.bottom = tRect.top + iHeight;
		}

		if ( SetRectAndFixup ( tRect ) )
			Event_SplitterMoved ();
	}
}


bool SplitterBar_c::SetRectAndFixup ( const RECT & tRect )
{
	m_bVertical = tRect.bottom - tRect.top > tRect.right - tRect.left;

	RECT tOldRect = m_tRect;

	m_tRect = tRect;
	FixupRect ();

	bool bChanged =tOldRect.left != m_tRect.left
				|| tOldRect.top	!= m_tRect.top
				|| tOldRect.bottom != m_tRect.bottom
				|| tOldRect.right != m_tRect.right;

	if ( bChanged )
		SetRect ( m_tRect );

	return bChanged;
}


void SplitterBar_c::FixupRect ()
{
	if ( m_bVertical )
	{
		if ( m_tRect.left < m_iMin )
		{
			m_tRect.right = m_iMin + m_tRect.right - m_tRect.left;
			m_tRect.left = m_iMin;
		}

		if ( m_tRect.right > m_iMax )
		{
			m_tRect.left = m_iMax - m_tRect.right + m_tRect.left;
			m_tRect.right = m_iMax;
		}
	}
	else
	{
		if ( m_tRect.top < m_iMin )
		{
			m_tRect.bottom = m_iMin + m_tRect.bottom - m_tRect.top;
			m_tRect.top = m_iMin;
		}

		if ( m_tRect.bottom > m_iMax )
		{
			m_tRect.top = m_iMax - m_tRect.bottom + m_tRect.top;
			m_tRect.bottom = m_iMax;
		}
	}
}

void SplitterBar_c::SetColors ( DWORD uBorder, DWORD uFill, DWORD uPressed )
{
	m_uBorderColor = uBorder;
	m_uFillColor = uFill;
	m_uPressedColor = uPressed;
}

void SplitterBar_c::CreateDrawObjects ()
{
	m_hBorderPen	= CreatePen ( PS_SOLID, 1, m_uBorderColor );
	m_hFillBrush	= CreateSolidBrush ( m_uFillColor );
	m_hPressedBrush = CreateSolidBrush ( m_uPressedColor );

	Assert ( m_hBorderPen && m_hFillBrush && m_hPressedBrush );
}

void SplitterBar_c::DestroyDrawObjects ()
{
	DeleteObject ( m_hBorderPen );
	DeleteObject ( m_hFillBrush );
	DeleteObject ( m_hPressedBrush );
}

//////////////////////////////////////////////////////////////////////////

HCURSOR	Hyperlink_c::m_hLinkCursor = NULL;
COLORREF Hyperlink_c::m_tColor = RGB ( 0, 0, 255 );

Hyperlink_c::Hyperlink_c ( const Str_c & sText, const Str_c & sURL )
	: m_sURL	( sURL )
	, m_sText	( sText )
{
}

void Hyperlink_c::OnCreate ()
{
	if ( ! m_hLinkCursor )
		m_hLinkCursor = LoadCursor ( NULL, IDC_HAND );

	SetWindowText ( m_hWnd, m_sText );
}

void Hyperlink_c::OnSetCursor ()
{
	SetCursor ( m_hLinkCursor );
}

void Hyperlink_c::OnLButtonDown ( int iX, int iY )
{
	POINT pt = { iX, iY };
	RECT LinkRect;
	GetClientRect ( m_hWnd, &LinkRect );
	if ( ::PtInRect ( &LinkRect, pt ) )
	{
		SetFocus ( m_hWnd );
		SetCapture ( m_hWnd );
	}
}

void Hyperlink_c::OnLButtonUp ( int iX, int iY )
{
	if ( GetCapture () == m_hWnd )
	{
		ReleaseCapture();
		POINT pt = { iX, iY };
		RECT LinkRect;
		GetClientRect ( m_hWnd, &LinkRect );
		if ( ::PtInRect ( &LinkRect, pt ) )
			Navigate ();
	}
}

void Hyperlink_c::OnPaint ( HDC hDC, const RECT & Rect )
{
	HBRUSH hBrush = CreateSolidBrush ( GetSysColor ( COLOR_WINDOW ) );
	FillRect ( hDC, &Rect, hBrush );
	DeleteObject ( hBrush );

	SetBkMode ( hDC, TRANSPARENT );
	HFONT hFontOld = (HFONT)SelectObject ( hDC, g_tResources.m_hSystemFont );
	SetTextColor ( hDC, m_tColor );

	DrawText ( hDC, m_sText, -1, (RECT *)&Rect, DT_NOCLIP | DT_SINGLELINE | DT_LEFT | DT_TOP | DT_NOPREFIX );
	SelectObject ( hDC, hFontOld );
}

bool Hyperlink_c::Navigate ()
{
	SHELLEXECUTEINFO shExeInfo = { sizeof ( SHELLEXECUTEINFO ), 0, 0, L"open", m_sURL, 0, 0, SW_SHOWNORMAL, 0, 0, 0, 0, 0, 0, 0 };
	ShellExecuteEx ( &shExeInfo );

	DWORD_PTR dwRet = (DWORD_PTR) shExeInfo.hInstApp;

	return dwRet > 32;
}


///////////////////////////////////////////////////////////////////////////////////////////
// dialog-related stuff

extern HWND g_hMainWindow;
static HWND g_hFullscreenDlg = NULL;

Dialog_c::WndProcs_t Dialog_c::m_WndProcs;
Dialog_c * Dialog_c::m_pLastCreatedDialog = NULL;


///////////////////////////////////////////////////////////////////////////////////////////
Dialog_c::Dialog_c ( const wchar_t * szHelp )
	: m_sHelp		( szHelp )
	, m_iDialogId	( 0 )
	, m_hToolbar	( NULL )
{
}

Dialog_c::~Dialog_c ()
{
	m_WndProcs.Delete ( m_hWnd );
}

int Dialog_c::Run ( int iResource, HWND hParent )
{
	m_pLastCreatedDialog = this;
	m_iDialogId = iResource;
	int iRes = DialogBox ( ResourceInstance (), MAKEINTRESOURCE (iResource), hParent, Dialog_c::DialogProc );
	m_pLastCreatedDialog = NULL;
	return iRes;
}

void Dialog_c::Close ( int iItem )
{
	OnClose ();
	EndDialog ( m_hWnd, iItem );
}

void Dialog_c::OnInit ()
{
	// auto-localize Ok/Cancel buttons
	Loc ( IDOK, T_TBAR_OK );
	Loc ( IDCANCEL, T_TBAR_CANCEL );
}

void Dialog_c::OnHelp ()
{
	Help ( m_sHelp );
}

HWND Dialog_c::Item ( int iItemId )
{
	return GetDlgItem ( m_hWnd, iItemId );
}

void Dialog_c::ItemTxt ( int iItemId, const wchar_t * szText )
{
	SetDlgItemText ( m_hWnd, iItemId, szText );
}

Str_c Dialog_c::GetItemTxt ( int iItemId )
{
	wchar_t szBuf [256];
	GetDlgItemText ( m_hWnd, iItemId, szBuf, 256 );
	return szBuf;
}

void Dialog_c::CheckBtn ( int iItemId, bool bCheck )
{
	CheckDlgButton ( m_hWnd, iItemId, bCheck ? BST_CHECKED : BST_UNCHECKED );
}

bool Dialog_c::IsChecked ( int iItemId ) const
{
	return IsDlgButtonChecked ( m_hWnd, iItemId ) == BST_CHECKED;
}

void Dialog_c::Loc ( int iItemId, int iString )
{
	DlgTxt ( m_hWnd, iItemId, iString );
}

void Dialog_c::Bold ( int iItemId )
{
	SendMessage ( GetDlgItem ( m_hWnd, iItemId ), WM_SETFONT, (WPARAM)g_tResources.m_hBoldSystemFont, TRUE );
}

int	Dialog_c::GetDlgId () const
{
	return m_iDialogId;
}

HWND Dialog_c::GetToolbar () const
{
	return m_hToolbar;
}

BOOL CALLBACK Dialog_c::DialogProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{
	Dialog_c * pDialog = NULL;

	if ( Msg == WM_INITDIALOG )
	{
		Assert ( m_pLastCreatedDialog );
		m_pLastCreatedDialog->m_hWnd = hDlg;
		Verify ( m_WndProcs.Add ( hDlg, m_pLastCreatedDialog ) );
		pDialog = m_pLastCreatedDialog;
	}
	else
	{	
		Dialog_c ** ppDialog = m_WndProcs.Find ( hDlg );
		if ( ! ppDialog )
			return FALSE;

		pDialog = *ppDialog;
	}

	if ( ! pDialog )
		return FALSE;

	switch ( Msg )
	{
	case WM_INITDIALOG:
		pDialog->OnInit ();
		pDialog->PostInit ();
		break;

	case WM_ACTIVATE:
		if ( LOWORD ( wParam ) != WA_INACTIVE )
			SipShowIM ( SIPF_OFF );
		break;

	case WM_COMMAND:
		pDialog->OnCommand ( LOWORD ( wParam ), HIWORD ( wParam ) );
		break;

	case WM_HELP:
		pDialog->OnHelp ();
		return TRUE;

	case WM_CLOSE:
		pDialog->OnClose ();
		break;

	case WM_LBUTTONDOWN:
		if ( pDialog->OnLButtonDown ( GET_X_LPARAM ( lParam ), GET_Y_LPARAM ( lParam ) ) )
			return TRUE;
		break;

	case WM_MOUSEMOVE:
		if ( pDialog->OnMouseMove ( GET_X_LPARAM ( lParam ), GET_Y_LPARAM ( lParam ) ) )
			return TRUE;
		break;

	case WM_LBUTTONUP:
		if ( pDialog->OnLButtonUp () )
			return TRUE;
		break;

	case WM_SETTINGCHANGE:
		pDialog->OnSettingsChange ();
		break;

	case WM_NOTIFY:
		if ( pDialog->OnNotify ( (int) wParam, (NMHDR *) lParam ) )
			return TRUE;
		break;

	case WM_CONTEXTMENU:
		pDialog->OnContextMenu ( LOWORD ( lParam ), HIWORD ( lParam ) );
		break;

	case WM_TIMER:
		pDialog->OnTimer ( wParam );
		break;
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
// fullscreen dialog
Dialog_Fullscreen_c::Dialog_Fullscreen_c ( const wchar_t * szHelp, int iToolbar, DWORD uBarFlags, bool bHideDone )
	: Dialog_c ( szHelp )
	, m_iToolbar ( iToolbar )
	, m_uBarFlags ( uBarFlags )
	, m_bHideDone ( bHideDone )
{
}

void Dialog_Fullscreen_c::OnInit ()
{
	Dialog_c::OnInit ();
	m_hToolbar = InitFullscreenDlg ( m_hWnd, m_iToolbar, m_uBarFlags, m_bHideDone );
}

void Dialog_Fullscreen_c::Close ( int iItem )
{
	CloseFullscreenDlg ();
	Dialog_c::Close ( iItem );
}

//////////////////////////////////////////////////////////////////////////
// resizer dialog
Dialog_Resizer_c::Dialog_Resizer_c ( const wchar_t * szHelp, int iToolbar, DWORD uBarFlags, bool bHideDone )
	: Dialog_Fullscreen_c ( szHelp, iToolbar, uBarFlags, bHideDone )
	, m_hResizer		( NULL )
	, m_bForcedResize	( false )
{
}

void Dialog_Resizer_c::PostInit ()
{
	Dialog_Fullscreen_c::PostInit ();
	m_bForcedResize = true;
	OnSettingsChange ();
	m_bForcedResize = false;
}

void Dialog_Resizer_c::OnSettingsChange ()
{
	Dialog_Fullscreen_c::OnSettingsChange ();

	RECT tWndRect;
	Verify ( FMCalcWindowRect ( tWndRect, true ) );
	SetWindowPos ( m_hWnd, HWND_TOP, tWndRect.left, tWndRect.top,
				tWndRect.right - tWndRect.left,
				tWndRect.bottom - tWndRect.top,
				SWP_SHOWWINDOW | ( m_bForcedResize ? 0 : SWP_NOACTIVATE ) );

	DWORD dwStyle = GetWindowLong ( m_hWnd, GWL_STYLE );
	SetWindowLong ( m_hWnd, GWL_STYLE, dwStyle & ~WS_VSCROLL );

	Resize ();
}

void Dialog_Resizer_c::SetResizer ( HWND hWnd )
{
	m_hResizer = hWnd;
}

void Dialog_Resizer_c::AddStayer ( HWND hWnd )
{
	if ( hWnd )
		m_dStayers.Add ( hWnd );
}

void Dialog_Resizer_c::Resize ()
{
	DoResize ( m_hWnd, m_hResizer, m_dStayers.Length () ? &(m_dStayers [0]) : NULL, m_dStayers.Length () );
}

//////////////////////////////////////////////////////////////////////////
// moving dialog
Dialog_Moving_c::Dialog_Moving_c ( const wchar_t * szHelp, int iToolbar )
	: Dialog_c		( szHelp )
	, m_iToolbar	( iToolbar )
{
	memset ( &m_DlgRect, 0, sizeof ( m_DlgRect ) );
	memset ( &m_MouseInDlg, 0, sizeof ( m_MouseInDlg ) );
}

void Dialog_Moving_c::OnInit ()
{
	Dialog_c::OnInit ();
	m_hToolbar = CreateToolbar ( m_hWnd, m_iToolbar, 0 );
}

bool Dialog_Moving_c::OnLButtonDown ( int iX, int iY )
{
	Dialog_c::OnLButtonDown ( iX, iY );

	POINT Point = { iX, iY };
	SetCapture ( m_hWnd );
	GetWindowRect ( m_hWnd, &m_DlgRect );
	ClientToScreen ( m_hWnd, &Point);
	m_MouseInDlg.x = Point.x - m_DlgRect.left;
	m_MouseInDlg.y = Point.y - m_DlgRect.top;

	return true;
}

bool Dialog_Moving_c::OnMouseMove ( int iX, int iY )
{
	Dialog_c::OnMouseMove ( iX, iY );

	if ( GetCapture () == m_hWnd )
	{
		POINT Point = { iX, iY };
		ClientToScreen( m_hWnd, &Point );

		SetWindowPos ( m_hWnd, HWND_TOP, Point.x - m_MouseInDlg.x, Point.y - m_MouseInDlg.y,
			m_DlgRect.right - m_DlgRect.left, m_DlgRect.bottom - m_DlgRect.top, SWP_SHOWWINDOW );

		UpdateWindow ( m_hWnd );
		UpdateWindow ( g_hMainWindow );
	}

	return true;
}

bool Dialog_Moving_c::OnLButtonUp ()
{
	Dialog_c::OnLButtonUp ();
	ReleaseCapture ();
	return true;
}

//////////////////////////////////////////////////////////////////////////
static POINT g_tMouseInDlg = { 0, 0 };
static RECT g_tDlgRect = { 0, 0, 0, 0 };


BOOL MovingWindowProc ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch ( uMsg )
	{
	case WM_LBUTTONDOWN:
		{
			POINT tPoint = { GET_X_LPARAM ( lParam ), GET_Y_LPARAM ( lParam ) };
			SetCapture ( hDlg );
			GetWindowRect ( hDlg, &g_tDlgRect );
			ClientToScreen ( hDlg, &tPoint);
			g_tMouseInDlg.x = tPoint.x - g_tDlgRect.left;
			g_tMouseInDlg.y = tPoint.y - g_tDlgRect.top;
		}
		return TRUE;

	case WM_MOUSEMOVE:
		if ( GetCapture () == hDlg )
		{
			POINT tPoint = { GET_X_LPARAM ( lParam ), GET_Y_LPARAM ( lParam ) };
			ClientToScreen( hDlg, &tPoint );

			SetWindowPos ( hDlg, HWND_TOP, tPoint.x - g_tMouseInDlg.x, tPoint.y - g_tMouseInDlg.y,
				g_tDlgRect.right - g_tDlgRect.left, g_tDlgRect.bottom - g_tDlgRect.top, SWP_SHOWWINDOW );

			UpdateWindow ( hDlg );
			UpdateWindow ( g_hMainWindow );
		}
		return TRUE;

	case WM_LBUTTONUP:
		ReleaseCapture ();
		return TRUE;
	}

	return FALSE;
}

void Help ( const wchar_t * szHelp )
{
	const Str_c sHelp = Str_c ( L"file:PFM.htm#" ) + szHelp;
	CreateProcess ( L"peghelp.exe", sHelp, NULL, NULL, FALSE, 0, NULL, NULL, NULL, NULL ); 
}

static int IdToToolbar ( int iId )
{
	switch ( iId )
	{
		case TOOLBAR_OK:		return IDM_OK;
		case TOOLBAR_CANCEL:	return IDM_CANCEL;
		case TOOLBAR_OK_CANCEL:	return IDM_OK_CANCEL;
	}

	return iId;
}

HWND CreateToolbar ( HWND hParent, int iId, DWORD uFlags )
{
	return CreateToolbar ( hParent, ResourceInstance (), IdToToolbar ( iId ), uFlags );
}

HWND CreateToolbar ( HWND hParent, HINSTANCE hInstance, int iId, DWORD uFlags )
{
	SHMENUBARINFO tMenuBar;
	memset ( &tMenuBar, 0, sizeof(SHMENUBARINFO) );
	tMenuBar.cbSize 	= sizeof(SHMENUBARINFO);
	tMenuBar.hwndParent = hParent;
	tMenuBar.nToolBarId = iId;
	tMenuBar.dwFlags	= uFlags;
	tMenuBar.hInstRes 	= hInstance ? hInstance : ResourceInstance ();

	if ( !SHCreateMenuBar ( &tMenuBar ) )
		return NULL;

	switch ( iId )
	{
	case IDM_OK:
		SetToolbarText ( tMenuBar.hwndMB, IDOK, Txt ( T_TBAR_OK ) );
		break;
	case IDM_CANCEL:
		SetToolbarText ( tMenuBar.hwndMB, IDCANCEL, Txt ( T_TBAR_CANCEL ) );
		break;
	case IDM_OK_CANCEL:
		SetToolbarText ( tMenuBar.hwndMB, IDOK, Txt ( T_TBAR_OK ) );
		SetToolbarText ( tMenuBar.hwndMB, IDCANCEL, Txt ( T_TBAR_CANCEL ) );
		break;
	}

	return tMenuBar.hwndMB;
}

void SetToolbarText ( HWND hToolbar, int iId, const wchar_t * szOkText )
{
	if ( ! hToolbar )
		return;

	TBBUTTONINFO tBtnInfo;
	tBtnInfo.cbSize		= sizeof ( tBtnInfo );
	tBtnInfo.dwMask		= TBIF_TEXT;
	tBtnInfo.pszText	= (wchar_t *)szOkText;
	SendMessage ( hToolbar, TB_SETBUTTONINFO, iId, (LPARAM) &tBtnInfo );
}

void EnableToolbarButton ( HWND hToolbar, int iId, bool bEnable )
{
	SendMessage ( hToolbar, TB_ENABLEBUTTON, iId, MAKELONG ( bEnable ? TRUE : FALSE, 0 ) );
}

HWND InitFullscreenDlg ( HWND hDlg, int iToolbar, DWORD uBarFlags, bool bHideDone )
{
	if ( !InitFullscreenDlg ( hDlg, bHideDone ) )
		return NULL;

	return CreateToolbar ( hDlg, ResourceInstance (), iToolbar, uBarFlags );
}

bool InitFullscreenDlg ( HWND hDlg, bool bHideDone )
{
	g_hFullscreenDlg = hDlg;

	SHINITDLGINFO tInitDlg;
	tInitDlg.dwMask	= SHIDIM_FLAGS;
	tInitDlg.dwFlags= SHIDIF_DONEBUTTON | SHIDIF_SIZEDLGFULLSCREEN;
	tInitDlg.hDlg 	= hDlg;
	SHInitDialog ( &tInitDlg );

	if ( cfg->fullscreen )
		SHFullScreen ( g_hMainWindow, SHFS_SHOWTASKBAR );

	SipShowIM ( SIPF_OFF );

	if ( bHideDone )
	{
		SHDoneButton ( hDlg, SHDB_HIDE );

		DWORD dwStyle = GetWindowLong ( hDlg, GWL_STYLE );
		SetWindowLong ( hDlg, GWL_STYLE, dwStyle | WS_NONAVDONEBUTTON );
		RedrawWindow ( hDlg, NULL, NULL, RDW_ERASE );
	}

	return true;
}


void CloseFullscreenDlg ()
{
	SipShowIM ( SIPF_OFF );
	g_hFullscreenDlg = NULL;
}

bool IsInDialog ()
{
	return !!g_hFullscreenDlg;
}

void ActivateFullscreenDlg ()
{
	Assert ( g_hFullscreenDlg );
	SetForegroundWindow ( g_hFullscreenDlg );
	SipShowIM ( SIPF_OFF );
}


///////////////////////////////////////////////////////////////////////////////////////////
bool HandleSizeChange ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, bool bForced )
{
	if ( bForced || uMsg == WM_SETTINGCHANGE )
	{
		if ( hDlg )
		{
			RECT tWndRect;
			Verify ( FMCalcWindowRect ( tWndRect, true ) );
			SetWindowPos ( hDlg, HWND_TOP, tWndRect.left, tWndRect.top,
				tWndRect.right - tWndRect.left,
				tWndRect.bottom - tWndRect.top,
				SWP_SHOWWINDOW | ( bForced ? 0 : SWP_NOACTIVATE ) );

			DWORD dwStyle = GetWindowLong ( hDlg, GWL_STYLE );
			SetWindowLong ( hDlg, GWL_STYLE, dwStyle & ~WS_VSCROLL );
		}

		return true;
	}

	return false;
}

bool HandleTabbedSizeChange ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, bool bForced )
{
	if ( bForced || ( uMsg == WM_SETTINGCHANGE && wParam == SPI_SETSIPINFO ) )
	{
		if ( hDlg )
		{
			DWORD dwStyle = GetWindowLong ( hDlg, GWL_STYLE );
			SetWindowLong ( hDlg, GWL_STYLE, dwStyle & ~WS_VSCROLL );
		}
		return true;
	}

	return false;

}

void DoResize ( HWND hDlg, HWND hResizer, HWND * dStayers, int nStayers )
{
	if ( !hResizer )
		return;

	const int iMul		= IsVGAScreen () ? 2 : 1;
	const int SPACING_V	= 2*iMul;
	const int SPACING_H	= 1*iMul;

	RECT tDlgRect;
	GetClientRect ( hDlg, &tDlgRect );

	int iMinTop = 0;
	for ( int i = 0; i < nStayers; ++i )
	{
		HWND hWnd = dStayers [i];
		RECT tRect;
		GetWindowRect ( hWnd, &tRect );

		POINT tPt = { tRect.left, tRect.top };
		ScreenToClient ( hDlg, &tPt );

		int iHeight = tRect.bottom - tRect.top;
		int iTop = tDlgRect.bottom - iHeight - SPACING_V;

		if ( !i )
			iMinTop = iTop;
		else
			if ( iTop < iMinTop )
				iMinTop = iTop;
	}

	if ( ! iMinTop )
		iMinTop = tDlgRect.bottom - SPACING_V;

	RECT tOldResizerRect;
	GetWindowRect ( hResizer, &tOldResizerRect );
	POINT tPt = { tOldResizerRect.left, tOldResizerRect.top };
	ScreenToClient ( hDlg, &tPt );

	SetWindowPos ( hResizer, HWND_TOP, SPACING_H, tPt.y, tDlgRect.right - tDlgRect.left - SPACING_H * 2,
		iMinTop - tPt.y - SPACING_V, SWP_SHOWWINDOW | SWP_NOMOVE );

	RECT tResizerRect;
	GetWindowRect ( hResizer, &tResizerRect );

	for ( int i = 0; i < nStayers; ++i )
	{
		HWND hWnd = dStayers [i];
		RECT tRect;
		GetWindowRect ( hWnd, &tRect );

		POINT tPt = { tRect.left, tRect.top };
		ScreenToClient ( hDlg, &tPt );

		int iTop = tPt.y - tOldResizerRect.bottom + tResizerRect.bottom;
		SetWindowPos ( hWnd, HWND_TOP, tPt.x, iTop, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE );
	}
}

int GetColumnWidthRelative ( float fWidth )
{
	int iMul = IsVGAScreen () ? 2 : 1;
	return Round <float,int> ( float ( g_tResources.m_iScreenWidth - 20 * iMul ) * fWidth );
}

void SetComboTextFocused ( HWND hCombo, const wchar_t * szText )
{
	SetWindowText ( hCombo, szText );
	SetFocus ( hCombo);
	SendMessage ( hCombo, CB_SETEDITSEL, 0, MAKELPARAM ( -1, wcslen ( szText ) ) );
}

void SetEditTextFocused ( HWND hCombo, const wchar_t * szText )
{
	SetWindowText ( hCombo, szText );
	SetFocus ( hCombo);
	SendMessage ( hCombo, EM_SETSEL, wcslen ( szText ), wcslen ( szText ) );
}


void HandleActivate ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if ( uMsg == WM_ACTIVATE && LOWORD ( wParam ) != WA_INACTIVE )
		SipShowIM ( SIPF_OFF );
}

//////////////////////////////////////////////////////////////////////////
static BOOL CALLBACK WaitDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
		SetDlgItemText ( hDlg, IDC_TEXT, Txt ( T_DLG_PREPARING_FILES ) );
		break;
	}

	return FALSE;
}

static HWND g_hPrepareWnd = NULL;

void PrepareCallback ()
{
	Assert ( !g_hPrepareWnd );
	g_hPrepareWnd = CreateDialog ( ResourceInstance (), MAKEINTRESOURCE ( IDD_WAITBOX ), g_hMainWindow, WaitDlgProc );
	if ( g_hPrepareWnd )
		ShowWindow ( g_hPrepareWnd, SW_SHOW );
}

void DestroyPrepareDialog ()
{
	if ( g_hPrepareWnd )
	{
		DestroyWindow ( g_hPrepareWnd );
		g_hPrepareWnd = NULL;
	}
}