#include "pch.h"

#include "LComm/ccomm.h"
#include "LCore/clog.h"
#include "LComm/cbluetooth.h"

#include "bthutil.h"

static bool g_bSocketsInited = false;
static bool g_bWidcommInited = false;
static bool g_bWidcommPresent = false;
static bool g_bMSPresent = false;

static DWORD g_dwBTMode = BTH_POWER_OFF;

//////////////////////////////////////////////////////////////////////////
WindowNotifier_c::WindowNotifier_c ()
	: m_hWnd 	( NULL )
{
}

void WindowNotifier_c::SetWindow ( HWND hWnd )
{
	m_hWnd = hWnd;
}

void CheckBTStacks ()
{
	// assume none
	g_bWidcommPresent = false;
	g_bMSPresent = false;

	// widcomm first
	HKEY hKey = NULL;

	if ( RegOpenKeyEx ( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Widcomm\\BtConfig\\General", 0, KEY_READ, &hKey ) == ERROR_SUCCESS )
	{
    	if ( hKey != NULL )
			g_bWidcommPresent = true;

		RegCloseKey ( hKey );
	}

	if ( ! g_bWidcommPresent )
		g_bMSPresent = true;
}

bool IsWidcommBTPresent ()
{
	return g_bWidcommPresent;
}

bool IsMSBTPresent ()
{
	return g_bMSPresent;
}

bool Init_WidcommBT ()
{
	if ( ! g_bWidcommInited )
	{
		if ( ! Init_WidcommLibrary () )
			return false;

		g_bWidcommInited = true;
	}

	return true;
}

void Shutdown_WidcommBT ()
{
	Shutdown_WidcommLibrary ();
	g_bWidcommInited = false;
}

bool Init_MsBT ()
{
	if ( ! Init_Sockets () )
		return false;

	if ( BthGetMode ( &g_dwBTMode ) != ERROR_SUCCESS )
		return false;

	if ( g_dwBTMode != BTH_DISCOVERABLE )
	{
		if ( BthSetMode ( BTH_DISCOVERABLE ) != ERROR_SUCCESS )
			return false;

		DWORD dwMode = 0;
		for ( int i = 0; i < 10; ++i )
		{
			if ( BthGetMode ( &dwMode ) != ERROR_SUCCESS )
				Sleep ( 200 );
			else
				if ( dwMode != BTH_DISCOVERABLE )
					Sleep ( 200 );
				else
					break;
		}
	}
	
	return true;
}


void Shutdown_MsBT ()
{
	DWORD dwBTMode = BTH_POWER_OFF;

	if ( BthGetMode ( &dwBTMode ) == ERROR_SUCCESS )
	{	
		if ( dwBTMode != g_dwBTMode )
			BthSetMode ( g_dwBTMode );
	}
}

bool Init_Sockets ()
{
	if ( g_bSocketsInited )
		return true;

	WORD wVersionRequested = MAKEWORD( 2, 2 );
	WSADATA wsaData;
	if ( WSAStartup( wVersionRequested, &wsaData ) != 0 )
		return false;

	if ( LOBYTE( wsaData.wVersion ) != 2 ||
		HIBYTE( wsaData.wVersion ) != 2 )
	{
		WSACleanup ();
		return false;
	}

	g_bSocketsInited = true;
	return true; 
}


void Shutdown_Sockets ()
{
	if ( g_bSocketsInited )
	{
		WSACleanup ();
		g_bSocketsInited = false;
	}
}