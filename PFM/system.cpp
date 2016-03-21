#include "pch.h"
#include "system.h"

#include "winuserm.h"

static HINSTANCE g_hCoreDll;
UnregisterFunc1Proc UndergisterFunc = NULL;

Str_c g_sWinDir;


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

Str_c GetExecutablePath ()
{
	wchar_t szFileName [MAX_PATH];

	GetModuleFileName ( NULL, szFileName, MAX_PATH );
	return GetPath ( szFileName );
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


//////////////////////////////////////////////////////////////////////////////////////////
// buttons
namespace btns
{
	struct Button_t
	{
		float	m_fLastEventTime;
		bool	m_bPressed;
	};

	const float LONG_PRESS_TIME = 0.4f;
	const int NUM_BUTTONS = 256;

	Button_t g_dButtons [NUM_BUTTONS];
	double g_fStartTime = 0.0;
	int g_iLastKeyDown = -1;
	
	Event_e	Event_Hotkey ( int iKey, DWORD uModifiers, DWORD uVirtKey )
	{
		return ( uModifiers & MOD_KEYUP ) ? Event_Keyup ( iKey ) : Event_Keydown ( iKey );
	}

	float GetEventTime ()
	{
		if ( g_fStartTime == 0.0 )
			g_fStartTime = g_Timer.GetTimeSec ();
		
		return (float) ( g_Timer.GetTimeSec () - g_fStartTime );
	}

	Event_e	Event_Keydown ( int iKey )
	{
		if ( g_iLastKeyDown == VK_ACTION && iKey == VK_RETURN )
			return EVENT_NONE;

		if ( iKey < 0 || iKey >= NUM_BUTTONS )
			return EVENT_NONE;

		if ( iKey == 91 || iKey == 132 )
			return EVENT_NONE;

		Button_t & tButton = g_dButtons [iKey];

		g_iLastKeyDown = iKey;

		bool bWasDown = tButton.m_bPressed;

		tButton.m_bPressed = true;
		tButton.m_fLastEventTime = GetEventTime ();

		if ( bWasDown )
			return EVENT_PRESS;

		return EVENT_NONE;
	}

	Event_e	Event_Keyup ( int iKey )
	{
		g_iLastKeyDown = -1;

		if ( iKey < 0 || iKey >= NUM_BUTTONS )
			return EVENT_NONE;

		if ( iKey == 91 || iKey == 132 )
			return EVENT_NONE;

		Button_t & tButton = g_dButtons [iKey];

		if ( tButton.m_bPressed )
		{
			tButton.m_bPressed = false;
			if ( GetEventTime () - tButton.m_fLastEventTime >= LONG_PRESS_TIME )
				return EVENT_LONGPRESS;
			else
				return EVENT_PRESS;
		}
		else
		{
			// TODO: this may be not the right way
			if ( iKey == VK_TSOFT1 || iKey == VK_TSOFT2 )
				return EVENT_PRESS;
			else
				return EVENT_NONE;
		}
	}

	Event_e Event_Timer (  int & iKey )
	{
		for ( int i = 0; i < NUM_BUTTONS; ++i )
		{
			Button_t & tButton = g_dButtons [i];
			if ( GetEventTime () - tButton.m_fLastEventTime >= LONG_PRESS_TIME && tButton.m_bPressed )
			{
				iKey = i;
				tButton.m_bPressed = false;
				return EVENT_LONGPRESS;
			}
		}

		return EVENT_NONE;
	}

	void Reset ()
	{
		g_iLastKeyDown = -1;
		memset ( g_dButtons, 0, sizeof ( g_dButtons ) );
	}
}