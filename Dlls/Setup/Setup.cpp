#include "windows.h"
#include "ce_setup.h"

#include "PFM\crypt\shark.h"

void EncryptSymmetric ( unsigned char * dData, int iDataSize, unsigned char * dKey, int iKeySize )
{
	shark * pShark = new shark ( dKey, iKeySize );

	ddword tRes;
	int nBlocks = iDataSize / 8;

	for( int i = 0; i < nBlocks; i++)
	{
		ddword tData;
		tData.w [0] = (( long * ) dData )[i*2];
		tData.w [1] = (( long * ) dData )[i*2 + 1];

		tRes = pShark->encryption ( tData );
		(( long * ) dData )[i*2]	= tRes.w [0];
		(( long * ) dData )[i*2 + 1]= tRes.w [1];
	}

	delete pShark;
}

void WriteDate ()
{
	const DWORD DATE_SIG = 0xDADE;
	const int KEY_LEN_WITH_SIG = 16;

	const int KEY_STORE_LEN	= 8;
	unsigned char KEY_STORE_KEY [KEY_STORE_LEN] = { 0x4E, 0x0C, 0xE3, 0x38, 0x57, 0xC4, 0x7E, 0x06 };

	HKEY hKey;
	DWORD dwDisposition;

	if ( RegCreateKeyEx ( HKEY_CURRENT_USER, L"\\Software\\PFM\\Cache", 0, NULL, 0, 0, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS )
		return;

	unsigned char dDate [256];
	ZeroMemory ( dDate, sizeof ( dDate ) );

	SYSTEMTIME tCurTime;
	GetSystemTime ( &tCurTime );
	*(DWORD *)dDate = DATE_SIG;
	*(SYSTEMTIME *)( ( (DWORD *)dDate ) + 1 ) = tCurTime;

	EncryptSymmetric ( dDate, KEY_LEN_WITH_SIG, KEY_STORE_KEY, KEY_STORE_LEN );

	DWORD dwType = REG_BINARY;
	DWORD dwSize = 256;
	unsigned char dDateCrypted [256];

	if ( RegQueryValueEx ( hKey, L"Icons", 0, &dwType, (PBYTE)dDateCrypted, &dwSize ) == ERROR_SUCCESS )
		return;

	if ( RegSetValueEx ( hKey, L"Icons", 0, REG_BINARY, dDate, KEY_LEN_WITH_SIG ) != ERROR_SUCCESS )
		return;
}

//////////////////////////////////////////////////////////////////////////
codeINSTALL_INIT Install_Init( HWND hwndParent, BOOL fFirstCall, BOOL fPreviouslyInstalled, LPCTSTR pszInstallDir )
{
	return codeINSTALL_INIT_CONTINUE;
}

codeINSTALL_EXIT Install_Exit ( HWND hwndParent, LPCTSTR pszInstallDir, WORD cFailedDirs, WORD cFailedFiles, WORD cFailedRegKeys, WORD cFailedRegVals, WORD cFailedShortcuts )
{
	WriteDate ();

	OSVERSIONINFO tOSInfo;
	tOSInfo.dwOSVersionInfoSize = sizeof ( OSVERSIONINFO );
	GetVersionEx ( &tOSInfo );

	wchar_t szDest [512];
	wcscpy ( szDest, pszInstallDir );

	bool b2003 = tOSInfo.dwMajorVersion == 4;
	if ( b2003 )
	{
		wcscat ( szDest, L"\\WidcommBT5.dll" );
		DeleteFile ( szDest );
	}
	else
	{
		wcscat ( szDest, L"\\WidcommBT.dll" );
		DeleteFile ( szDest );
	}

	return codeINSTALL_EXIT_DONE;
}

codeUNINSTALL_INIT Uninstall_Init( HWND hwndParent, LPCTSTR pszInstallDir )
{
	if ( MessageBox ( hwndParent, L"Uninstall will delete all Pocket File Manager settings. Do you wish to keep your settings?", L"Delete settings?", MB_YESNO ) != IDYES )
	{
		wchar_t szDest [512];

		wcscpy ( szDest, pszInstallDir );
		wcscat ( szDest, L"\\buttons.ini" );
		DeleteFile ( szDest );

		wcscpy ( szDest, pszInstallDir );
		wcscat ( szDest, L"\\config.ini" );
		DeleteFile ( szDest );

		wcscpy ( szDest, pszInstallDir );
		wcscat ( szDest, L"\\recent.txt" );
		DeleteFile ( szDest );

		wcscpy ( szDest, pszInstallDir );
		wcscat ( szDest, L"\\bookmarks.txt" );
		DeleteFile ( szDest );

		HKEY hKey;
		if ( RegOpenKeyEx ( HKEY_CURRENT_USER, L"\\Software", 0, KEY_READ, &hKey ) == ERROR_SUCCESS )
		{
			RegDeleteKey ( hKey, L"PFM" );
			RegCloseKey ( hKey );
		}
	}

	return codeUNINSTALL_INIT_CONTINUE;
}

codeUNINSTALL_EXIT Uninstall_Exit( HWND hwndParent )
{
	return codeUNINSTALL_EXIT_DONE;
}