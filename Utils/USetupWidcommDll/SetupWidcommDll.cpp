#include "windows.h"
#include "ce_setup.h"

//////////////////////////////////////////////////////////////////////////
codeINSTALL_INIT Install_Init( HWND hwndParent, BOOL fFirstCall, BOOL fPreviouslyInstalled, LPCTSTR pszInstallDir )
{
	HKEY hKey = NULL;
	bool bWidcommPresent = false;

	if ( RegOpenKeyEx ( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Widcomm\\BtConfig\\General", 0, KEY_READ, &hKey ) == ERROR_SUCCESS )
	{
		if ( hKey != NULL )
			bWidcommPresent = true;

		RegCloseKey ( hKey );
	}

	return bWidcommPresent ? codeINSTALL_INIT_CONTINUE : codeINSTALL_INIT_CANCEL;
}

codeINSTALL_EXIT Install_Exit ( HWND hwndParent, LPCTSTR pszInstallDir, WORD cFailedDirs, WORD cFailedFiles, WORD cFailedRegKeys, WORD cFailedRegVals, WORD cFailedShortcuts )
{
	return codeINSTALL_EXIT_DONE;
}

codeUNINSTALL_INIT Uninstall_Init( HWND hwndParent, LPCTSTR pszInstallDir )
{
	return codeUNINSTALL_INIT_CONTINUE;
}

codeUNINSTALL_EXIT Uninstall_Exit( HWND hwndParent )
{
	return codeUNINSTALL_EXIT_DONE;
}