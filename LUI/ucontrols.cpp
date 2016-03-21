#include "pch.h"

#include "LUI/ucontrols.h"
#include "LSettings/sconfig.h"
#include "LSettings/scolors.h"
#include "LPanel/presources.h"
#include "aygshell.h"

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
	bool bRet = true;
	SHELLEXECUTEINFO shExeInfo = { sizeof ( SHELLEXECUTEINFO ), 0, 0, L"open", m_sURL, 0, 0, SW_SHOWNORMAL, 0, 0, 0, 0, 0, 0, 0 };
	ShellExecuteEx ( &shExeInfo );

	DWORD_PTR dwRet = (DWORD_PTR) shExeInfo.hInstApp;

	return dwRet > 32;
}