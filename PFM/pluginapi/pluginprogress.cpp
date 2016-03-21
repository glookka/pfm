#include "pluginprogress.h"

extern PluginStartupInfo_t g_PSI;

WindowProgress_c::WindowProgress_c ( const wchar_t * szSizeText )
  : m_hHeader			( NULL )
  , m_hTotalProgressBar	( NULL )
  , m_hFileProgressBar	( NULL )
  , m_hSourceText		( NULL )
  , m_hDestText			( NULL )
  , m_hTotalProgressText( NULL )
  , m_hFileProgressText	( NULL )
  , m_hTotalText		( NULL )
  , m_hFileText			( NULL )
  , m_fLastTime			( 0.0 )
  , m_szSizeText		( szSizeText )
{
}

void WindowProgress_c::SetItems ( HWND hHeader, HWND hTotal, HWND hFile, HWND hSourceTxt, HWND hDestTxt, HWND hTotalPerc, HWND hFilePerc, HWND hTotalSize, HWND hFileSize )
{
	m_hHeader			= hHeader;
	m_hTotalProgressBar	= hTotal;
	m_hFileProgressBar	= hFile;
	m_hSourceText		= hSourceTxt;
	m_hDestText			= hDestTxt;
	m_hTotalProgressText= hTotalPerc;
	m_hFileProgressText	= hFilePerc;
	m_hTotalText		= hTotalSize;
	m_hFileText			= hFileSize;

}

void WindowProgress_c::Init ( HWND hDlg, const wchar_t * szHeader, const wchar_t * szProgressText, FileProgress_c * pProgress  )
{
	SetWindowText ( m_hHeader, szHeader );
	SendMessage ( m_hHeader, WM_SETFONT, (WPARAM)g_PSI.m_hBoldFont, TRUE );

	Setup ( pProgress );
	if ( pProgress )
		UpdateProgress ( pProgress );

	wcsncpy ( m_szProgressText, szProgressText, PROGRESS_BUFFER_SIZE );
}
		
bool WindowProgress_c::UpdateProgress ( FileProgress_c * pProgress )
{
	if ( !pProgress )
		return false;
		
	const float UPDATE_SEC = 0.2;

	float fTimer = g_PSI.m_fnTimeSec ();
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

	g_PSI.m_fnSizeToStr ( pProgress->GetTotalProcessedSize (), szSize, true  );
	wsprintf ( szRes, m_szSizeText, szSize, m_szTotalSize );

	GetWindowText ( m_hTotalText, szOldRes, FILESIZE_BUFFER_SIZE*2 );
	if ( wcscmp ( szRes, szOldRes ) )
		SetWindowText ( m_hTotalText, szRes );

	g_PSI.m_fnSizeToStr  ( pProgress->GetFileProcessedSize (), szSize, true  );
	g_PSI.m_fnSizeToStr  ( pProgress->GetFileTotalSize (), szSize1, true  );
	wsprintf ( szRes, m_szSizeText, szSize, szSize1 );

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
		g_PSI.m_fnSizeToStr ( pProgress->GetTotalSize (), m_szTotalSize, true );
}