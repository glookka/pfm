#include "pch.h"
#include "progress.h"

#include "resources.h"
#include "config.h"

#include "Dlls/Resource/resource.h"

WindowProgress_c::WindowProgress_c ()
  : m_hTotalProgressBar	( NULL )
  , m_hFileProgressBar	( NULL )
  , m_hSourceText		( NULL )
  , m_hDestText			( NULL )
  , m_hTotalProgressText( NULL )
  , m_hFileProgressText	( NULL )
  , m_hTotalText		( NULL )
  , m_hFileText			( NULL )
  , m_fLastTime			( 0.0 )
{
}

void WindowProgress_c::Init ( HWND hDlg, const wchar_t * szHeader, FileProgress_c * pProgress  )
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
	if ( !pProgress )
		return false;
		
	const double UPDATE_SEC = 0.2;

	double fTimer = g_Timer.GetTimeSec ();
	if ( fTimer - m_fLastTime < UPDATE_SEC )
		return false;
	else
		m_fLastTime = fTimer;

	wchar_t szOldProgressBuff [16], szProgressBuff [16];

	int iCurFilePos = (int)SendMessage ( m_hFileProgressBar, PBM_GETPOS, 0, 0 );
	if ( iCurFilePos != pProgress->GetFileProgress () )
		SendMessage ( m_hFileProgressBar, PBM_SETPOS, pProgress->GetFileProgress (), 0 );

	float fProgress = float ( pProgress->GetFileProgress () ) / float ( FileProgress_c::NUM_PROGRESS_POINTS ) * 100.0f;
	wsprintf ( szProgressBuff, L"%d%%", int ( fProgress ) );
	GetWindowText ( m_hFileProgressText, szOldProgressBuff, 16 );

	if ( wcscmp ( szProgressBuff, szOldProgressBuff ) )
		SetWindowText ( m_hFileProgressText, szProgressBuff );

	int iCurTotalPos = (int)SendMessage ( m_hTotalProgressBar, PBM_GETPOS, 0, 0 );
	if ( iCurTotalPos != pProgress->GetTotalProgress () )
		SendMessage ( m_hTotalProgressBar, PBM_SETPOS, pProgress->GetTotalProgress (), 0 );

	fProgress = float ( pProgress->GetTotalProgress () ) / float ( FileProgress_c::NUM_PROGRESS_POINTS ) * 100.0f;
	wsprintf ( szProgressBuff, L"%d%%", int ( fProgress ) );
	GetWindowText ( m_hFileProgressText, szOldProgressBuff, 16 );

	if ( wcscmp ( szProgressBuff, szOldProgressBuff ) )
		SetWindowText ( m_hTotalProgressText, szProgressBuff );

	wchar_t szSize [FILESIZE_BUFFER_SIZE];
	wchar_t szSize1 [FILESIZE_BUFFER_SIZE];
	wchar_t szRes [FILESIZE_BUFFER_SIZE*2];
	wchar_t szOldRes [FILESIZE_BUFFER_SIZE*2];

	FileSizeToStringUL ( pProgress->GetTotalProcessedSize (), szSize, true  );
	wsprintf ( szRes, Txt ( T_DLG_PROGRESS_SIZE ), szSize, m_szTotalSize );

	GetWindowText ( m_hTotalText, szOldRes, FILESIZE_BUFFER_SIZE*2 );
	if ( wcscmp ( szRes, szOldRes ) )
		SetWindowText ( m_hTotalText, szRes );

	FileSizeToStringUL ( pProgress->GetFileProcessedSize (), szSize, true  );
	FileSizeToStringUL ( pProgress->GetFileTotalSize (), szSize1, true  );
	wsprintf ( szRes, Txt ( T_DLG_PROGRESS_SIZE ), szSize, szSize1 );

	GetWindowText ( m_hFileText, szOldRes, FILESIZE_BUFFER_SIZE*2 );
	if ( wcscmp ( szRes, szOldRes ) )
		SetWindowText ( m_hFileText, szRes );

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
		FileSizeToStringUL ( pProgress->GetTotalSize (), m_szTotalSize, true );
}