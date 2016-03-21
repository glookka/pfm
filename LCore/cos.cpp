#include "pch.h"

#include "LCore/cos.h"
#include "LCore/clog.h"

static HINSTANCE g_hCoreDll;
UnregisterFunc1Proc UndergisterFunc = NULL;

Str_c g_sWinDir;
/*
const int MAX_TABLE_STRLEN = 256;
wchar_t g_szBuf [MAX_TABLE_STRLEN]; 
*/

WinCeVersion_e GetOSVersion ()
{
	OSVERSIONINFO tOSInfo;
	tOSInfo.dwOSVersionInfoSize = sizeof ( OSVERSIONINFO );
	GetVersionEx ( &tOSInfo );

	if ( tOSInfo.dwPlatformId != VER_PLATFORM_WIN32_CE )
		return WINCE_UNKNOWN;
	
	const DWORD & dwMaj = tOSInfo.dwMajorVersion;
	const DWORD & dwMin = tOSInfo.dwMinorVersion;
	
	if ( dwMaj == 4 && dwMin == 20 )
		return WINCE_2003;
	
	if ( dwMaj == 4 && dwMin == 21 )
		return WINCE_2003SE;

	// in case its some weird version
	if ( dwMaj == 4 )
		return WINCE_2003;

	if ( dwMaj == 5 )
		return WINCE_50;

	return WINCE_UNKNOWN;
}

bool IsVGAScreen ()
{
	int iScrWidth, iScrHeight;
	GetScreenResolution ( iScrWidth, iScrHeight );
	return Max ( iScrWidth, iScrHeight ) >= 480;
}

int	GetVGAScale ()
{
	return IsVGAScreen () ? 2 : 1;
}

void GetScreenResolution ( int & iWidth, int & iHeight )
{
	HDC hDesk = GetDC ( NULL );

	iWidth = GetDeviceCaps ( hDesk, HORZRES );
	iHeight = GetDeviceCaps ( hDesk, VERTRES );
}

void LoadExtraOSFuncs ()
{
	g_hCoreDll = LoadLibrary ( L"coredll.dll" );
	Assert ( g_hCoreDll );
	UndergisterFunc = (UnregisterFunc1Proc) GetProcAddress ( g_hCoreDll, L"UnregisterFunc1" );
}

void UnloadExtraOSFuncs ()
{
	FreeLibrary ( g_hCoreDll );
}

bool CanChangeOrientation ()
{
	DEVMODE tDevMode;
	memset ( &tDevMode, 0, sizeof ( DEVMODE ) );
	tDevMode.dmSize = sizeof(DEVMODE);
	tDevMode.dmFields = DM_DISPLAYQUERYORIENTATION;

	ChangeDisplaySettingsEx ( NULL, &tDevMode, NULL, CDS_TEST, NULL );
	return tDevMode.dmDisplayOrientation != DMDO_0;
}

int GetDisplayOrientation ()
{
	DEVMODE tDevMode;
	memset ( &tDevMode, 0, sizeof ( DEVMODE ) );
	tDevMode.dmSize = sizeof(DEVMODE);
	tDevMode.dmFields = DM_DISPLAYORIENTATION;

	ChangeDisplaySettingsEx ( NULL, &tDevMode, NULL, CDS_TEST, NULL );

	return tDevMode.dmDisplayOrientation == 4 ? 3 : tDevMode.dmDisplayOrientation;
}

void ChangeDisplayOrientation ( int iAngle )
{
	if ( iAngle < 0 || iAngle > 3 )
		return;

	DEVMODE tDevMode;
	memset ( &tDevMode, 0, sizeof ( DEVMODE ) );
	tDevMode.dmSize = sizeof(DEVMODE);
	tDevMode.dmFields = DM_DISPLAYORIENTATION;
	tDevMode.dmDisplayOrientation = iAngle == 3 ? 4 : iAngle;

	ChangeDisplaySettingsEx ( NULL, &tDevMode, NULL, 0, NULL );
}

Str_c GetWinDir ( HWND hWnd )
{
	if ( g_sWinDir.Empty () )
	{
		wchar_t szPath [MAX_PATH];
		if ( SHGetSpecialFolderPath ( hWnd, szPath, CSIDL_WINDOWS, FALSE ) )
			g_sWinDir = szPath;
	}

	return g_sWinDir;
}

Str_c GetWinErrorText ( int iErrorCode )
{
	Str_c sError;

	LPVOID lpMsgBuf;
	FormatMessage ( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, iErrorCode, 0, (LPTSTR) &lpMsgBuf, 0, NULL );

	wchar_t * szMsg = (wchar_t *)lpMsgBuf;
	int iLen = wcslen ( szMsg );
	if ( szMsg [iLen-1] == L'\n' )
		szMsg [iLen-1] = L'\0';

	sError = szMsg;

	LocalFree( lpMsgBuf );

	return sError;
}