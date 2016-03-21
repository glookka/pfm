#include "pch.h"

#include "LDialogs/dcommon.h"
#include "LCore/clog.h"
#include "LCore/cos.h"
#include "LCore/ctimer.h"
#include "LPanel/presources.h"
#include "LSettings/sconfig.h"
#include "LSettings/slocal.h"
#include "LUI/udialogs.h"
#include "Filemanager/filemanager.h"

#include "aygshell.h"
#include "Resources/resource.h"

extern HWND g_hMainWindow;
static HWND g_hPrepareWnd = NULL;


WindowResizer_c::WindowResizer_c ()
	: m_hDlg		( NULL )
	, m_hResizer	( NULL )
{
	int iMul = IsVGAScreen () ? 2 : 1;
	m_iSpacing = 2 * iMul;
	m_iHorSpacing = 1 * iMul;
}

void WindowResizer_c::SetDlg ( HWND hDlg )
{
	m_hDlg = hDlg;
}

void WindowResizer_c::SetResizer ( HWND hWnd )
{
	m_hResizer = hWnd;
}

void WindowResizer_c::AddStayer ( HWND hWnd )
{
	if ( hWnd )
		m_dStayers.Add ( hWnd );
}

void WindowResizer_c::Resize ()
{
	RECT tDlgRect;
	GetClientRect ( m_hDlg, &tDlgRect );

	int iMinTop = 0;
	for ( int i = 0; i < m_dStayers.Length (); ++i )
	{
		HWND hWnd = m_dStayers [i];
		RECT tRect;
		GetWindowRect ( hWnd, &tRect );

		POINT tPt = { tRect.left, tRect.top };
		ScreenToClient ( m_hDlg, &tPt );

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
	ScreenToClient ( m_hDlg, &tPt );

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
		ScreenToClient ( m_hDlg, &tPt );

		int iTop = tPt.y - tOldResizerRect.bottom + tResizerRect.bottom;
		SetWindowPos ( hWnd, HWND_TOP, tPt.x, iTop, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
WindowProgress_c::WindowProgress_c ()
	: m_hTotalProgressBar	( NULL )
	, m_hFileProgressBar	( NULL )
	, m_hSourceText			( NULL )
	, m_hDestText			( NULL )
	, m_hTotalProgressText	( NULL )
	, m_hFileProgressText	( NULL )
	, m_hTotalText			( NULL )
	, m_hFileText			( NULL )
	, m_fLastTime			( 0.0 )
{
}

void WindowProgress_c::Init ( HWND hDlg, const wchar_t * szHeader, FileProgress_c * pProgress )
{
	m_hTotalProgressBar	= GetDlgItem ( hDlg, IDC_TOTAL_PROGRESS );
	m_hFileProgressBar	= GetDlgItem ( hDlg, IDC_FILE_PROGRESS );
	m_hSourceText		= GetDlgItem ( hDlg, IDC_SOURCE_TEXT );
	m_hDestText			= GetDlgItem ( hDlg, IDC_DEST_TEXT );
	m_hTotalProgressText= GetDlgItem ( hDlg, IDC_TOTAL_PERCENT );
	m_hFileProgressText	= GetDlgItem ( hDlg, IDC_FILE_PERCENT );
	m_hTotalText		= GetDlgItem ( hDlg, IDC_TOTAL_SIZE );
	m_hFileText			= GetDlgItem ( hDlg, IDC_FILE_SIZE );

	HWND hHeader = GetDlgItem ( hDlg, IDC_HEADER );
	SetWindowText ( hHeader, szHeader );
	SendMessage ( hHeader, WM_SETFONT, (WPARAM)g_tResources.m_hBoldSystemFont, TRUE );

	Setup ( pProgress );
	if ( pProgress )
		UpdateProgress ( pProgress );
}

bool WindowProgress_c::UpdateProgress ( FileProgress_c * pProgress )
{
	Assert ( pProgress );

	const double UPDATE_SEC = 0.2;

	double fTimer = g_Timer.GetTimeSec ();
	if ( fTimer - m_fLastTime < UPDATE_SEC )
		return false;
	else
		m_fLastTime = fTimer;

	wchar_t szProgressBuff [16];
	if ( pProgress->GetFileProgressFlag () )
	{
		SendMessage ( m_hFileProgressBar, PBM_SETPOS, pProgress->GetFileProgress (), 0 );
		float fProgress = float ( pProgress->GetFileProgress () ) / float ( FileProgress_c::NUM_PROGRESS_POINTS ) * 100.0f;
		wsprintf ( szProgressBuff, L"%d%%", int ( fProgress ) );
		SetWindowText ( m_hFileProgressText, szProgressBuff );
	}

	if ( pProgress->GetTotalProgressFlag () )
	{
		SendMessage ( m_hTotalProgressBar, PBM_SETPOS, pProgress->GetTotalProgress (), 0 );
		float fProgress = float ( pProgress->GetTotalProgress () ) / float ( FileProgress_c::NUM_PROGRESS_POINTS ) * 100.0f;
		wsprintf ( szProgressBuff, L"%d%%", int ( fProgress ) );
		SetWindowText ( m_hTotalProgressText, szProgressBuff );
	}

	wchar_t szSize [FILESIZE_BUFFER_SIZE];
	wchar_t szSize1 [FILESIZE_BUFFER_SIZE];
	wchar_t szRes [FILESIZE_BUFFER_SIZE*2];
	FileSizeToString ( pProgress->GetTotalProcessedSize (), szSize, true  );
	wsprintf ( szRes, Txt ( T_DLG_PROGRESS_SIZE ), szSize, m_szTotalSize );
	SetWindowText ( m_hTotalText, szRes );

	FileSizeToString ( pProgress->GetFileProcessedSize (), szSize, true  );
	FileSizeToString ( pProgress->GetFileTotalSize (), szSize1, true  );
	wsprintf ( szRes, Txt ( T_DLG_PROGRESS_SIZE ), szSize, szSize1 );
	SetWindowText ( m_hFileText, szRes );

	return true;
}

void WindowProgress_c::Setup ( FileProgress_c * pProgress )
{
	SendMessage ( m_hTotalProgressBar, PBM_SETRANGE, 0, MAKELPARAM ( 0, FileProgress_c::NUM_PROGRESS_POINTS ) );
	SendMessage ( m_hTotalProgressBar, PBM_SETPOS, 0, 0 );

	SendMessage ( m_hFileProgressBar, PBM_SETRANGE, 0, MAKELPARAM ( 0, FileProgress_c::NUM_PROGRESS_POINTS ) );
	SendMessage ( m_hFileProgressBar, PBM_SETPOS, 0, 0 );

	if ( pProgress )
		FileSizeToString ( pProgress->GetTotalSize (), m_szTotalSize, true );
}


///////////////////////////////////////////////////////////////////////////////////////////

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


DWORD HandleDlgColor ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	/*
	if ( uMsg == WM_CTLCOLORDLG || uMsg == WM_CTLCOLORBTN || uMsg == WM_CTLCOLORMSGBOX || uMsg == WM_CTLCOLOREDIT
		|| uMsg == WM_CTLCOLORLISTBOX || uMsg == WM_CTLCOLORSCROLLBAR || uMsg == WM_CTLCOLORSTATIC )
	{
		if ( uMsg == WM_CTLCOLORSTATIC )
			SetBkMode ( (HDC) wParam, TRANSPARENT );

		return (DWORD)g_tResources.m_hDlgBackBrush;
	}*/

	return NULL;
}

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
