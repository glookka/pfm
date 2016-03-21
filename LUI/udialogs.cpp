#include "pch.h"

#include "LUI/udialogs.h"
#include "LCore/cos.h"
#include "LPanel/presources.h"
#include "LSettings/slocal.h"
#include "LSettings/sconfig.h"
#include "Filemanager/filemanager.h"

#include "aygshell.h"
#include "Resources/resource.h"

extern HWND g_hMainWindow;
static HWND g_hFullscreenDlg = NULL;

Dialog_c::WndProcs_t Dialog_c::m_WndProcs;
Dialog_c * Dialog_c::m_pLastCreatedDialog = NULL;


///////////////////////////////////////////////////////////////////////////////////////////
Dialog_c::Dialog_c ( const wchar_t * szHelp )
	: m_sHelp	( szHelp )
{
}

Dialog_c::~Dialog_c ()
{
	m_WndProcs.Delete ( m_hWnd );
}

int Dialog_c::Run ( int iResource, HWND hParent )
{
	m_pLastCreatedDialog = this;
	int iRes = DialogBox ( ResourceInstance (), MAKEINTRESOURCE (iResource), hParent, Dialog_c::DialogProc );
	m_pLastCreatedDialog = NULL;
	return iRes;
}

void Dialog_c::Close ( int iItem )
{
	OnClose ();
	EndDialog ( m_hWnd, iItem );
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
	InitFullscreenDlg ( m_hWnd, m_iToolbar, m_uBarFlags, m_bHideDone );
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
	int iMul = IsVGAScreen () ? 2 : 1;
	m_iSpacing = 2 * iMul;
	m_iHorSpacing = 1 * iMul;
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
	if ( ! m_hResizer )
		return;

	RECT tDlgRect;
	GetClientRect ( m_hWnd, &tDlgRect );

	int iMinTop = 0;
	for ( int i = 0; i < m_dStayers.Length (); ++i )
	{
		HWND hWnd = m_dStayers [i];
		RECT tRect;
		GetWindowRect ( hWnd, &tRect );

		POINT tPt = { tRect.left, tRect.top };
		ScreenToClient ( m_hWnd, &tPt );

		int iHeight = tRect.bottom - tRect.top;
		int iTop = tDlgRect.bottom - iHeight - m_iSpacing;

		if ( !i )
			iMinTop = iTop;
		else
			if ( iTop < iMinTop )
				iMinTop = iTop;
	}

	if ( ! iMinTop )
		iMinTop = tDlgRect.bottom - m_iSpacing;

	RECT tOldResizerRect;
	GetWindowRect ( m_hResizer, &tOldResizerRect );
	POINT tPt = { tOldResizerRect.left, tOldResizerRect.top };
	ScreenToClient ( m_hWnd, &tPt );

	SetWindowPos ( m_hResizer, HWND_TOP, m_iHorSpacing, tPt.y, tDlgRect.right - tDlgRect.left - m_iHorSpacing * 2,
		iMinTop - tPt.y - m_iSpacing, SWP_SHOWWINDOW | SWP_NOMOVE );

	RECT tResizerRect;
	GetWindowRect ( m_hResizer, &tResizerRect );

	for ( int i = 0; i < m_dStayers.Length (); ++i )
	{
		HWND hWnd = m_dStayers [i];
		RECT tRect;
		GetWindowRect ( hWnd, &tRect );

		POINT tPt = { tRect.left, tRect.top };
		ScreenToClient ( m_hWnd, &tPt );

		int iTop = tPt.y - tOldResizerRect.bottom + tResizerRect.bottom;
		SetWindowPos ( hWnd, HWND_TOP, tPt.x, iTop, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE );
	}
}

//////////////////////////////////////////////////////////////////////////
// moving dialog
Dialog_Moving_c::Dialog_Moving_c ( const wchar_t * szHelp, int iToolbar, bool bDefaultNames )
	: Dialog_c		( szHelp )
	, m_iToolbar	( iToolbar )
	, m_bDefaultNames( bDefaultNames )
{
	memset ( &m_DlgRect, 0, sizeof ( m_DlgRect ) );
	memset ( &m_MouseInDlg, 0, sizeof ( m_MouseInDlg ) );
}

void Dialog_Moving_c::OnInit ()
{
	Dialog_c::OnInit ();
	CreateToolbar ( m_hWnd, m_iToolbar, 0, m_bDefaultNames );
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
void Help ( const wchar_t * szHelp )
{
	const Str_c sHelp = Str_c ( L"file:PFM.htm#" ) + szHelp;
	CreateProcess ( L"peghelp.exe", sHelp, NULL, NULL, FALSE, 0, NULL, NULL, NULL, NULL ); 
}

HWND CreateToolbar ( HWND hParent, int iId, DWORD uFlags, bool bDefaultNames )
{
	SHMENUBARINFO tMenuBar;
	memset ( &tMenuBar, 0, sizeof(SHMENUBARINFO) );
	tMenuBar.cbSize 	= sizeof(SHMENUBARINFO);
	tMenuBar.hwndParent = hParent;
	tMenuBar.nToolBarId = iId;
	tMenuBar.dwFlags	= uFlags;
	tMenuBar.hInstRes 	= ResourceInstance ();

	if ( !SHCreateMenuBar ( &tMenuBar ) )
		return NULL;

	if ( bDefaultNames )
	{
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
	g_hFullscreenDlg = hDlg;

	HWND hToolbar = CreateToolbar ( hDlg, iToolbar, uBarFlags );

	SHINITDLGINFO tInitDlg;
	tInitDlg.dwMask	= SHIDIM_FLAGS;
	tInitDlg.dwFlags= SHIDIF_DONEBUTTON | SHIDIF_SIZEDLGFULLSCREEN;
	tInitDlg.hDlg 	= hDlg;
	SHInitDialog ( &tInitDlg );

	if ( g_tConfig.fullscreen )
		SHFullScreen ( g_hMainWindow, SHFS_SHOWTASKBAR );

	SipShowIM ( SIPF_OFF );

	if ( bHideDone )
	{
		SHDoneButton ( hDlg, SHDB_HIDE );

		DWORD dwStyle = GetWindowLong ( hDlg, GWL_STYLE );
		SetWindowLong ( hDlg, GWL_STYLE, dwStyle | WS_NONAVDONEBUTTON );
		RedrawWindow ( hDlg, NULL, NULL, RDW_ERASE );
	}

	return hToolbar;
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
