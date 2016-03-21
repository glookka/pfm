#include "pch.h"

#include "LFile/fapps.h"
#include "LCore/clog.h"
#include "LCore/cos.h"
#include "LCore/cfile.h"
#include "LFile/fiterator.h"
#include "LFile/ffilter.h"
#include "LSettings/srecent.h"

#include "shlobj.h"

extern HWND g_hMainWindow;

namespace apps
{
Array_T < AppInfo_t > g_dApps;
bool GetCommandFor ( const Str_c & sFileName, Str_c & sCommand );

/////////////////////////////////////////////////////////
void AppInfo_t::LoadIcon ()
{
	SHFILEINFO tShFileInfo;
	tShFileInfo.iIcon = -1;

	Str_c sAppPath = m_bBuiltIn ? GetWinDir ( g_hMainWindow ) + L"\\" + m_sFileName : m_sFileName;

	SHGetFileInfo ( sAppPath, 0, &tShFileInfo, sizeof (SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON );
	m_iIcon = tShFileInfo.iIcon;
}
/////////////////////////////////////////////////////////

int AppCompare ( const AppInfo_t & tApp1, const AppInfo_t & tApp2 )
{
	return wcscmp ( tApp1.m_sName, tApp2.m_sName );
}

Str_c FilenameFromCommand ( const wchar_t * szCommand )
{
	const wchar_t * szPtr = szCommand;
	
	while ( *szPtr && *szPtr == L'\"' )
		++szPtr;

	Str_c sCut = szPtr;

	int iFindRes = sCut.Find ( L".exe " );
	if ( iFindRes == -1 )
		iFindRes = sCut.Find ( L".exe\"" );

	if ( iFindRes != -1 )
		sCut = sCut.SubStr ( 0, iFindRes + 4 );

	return sCut;
}

static bool ReadAppFromRegistry ( const Str_c & sFileName, AppInfo_t & tAppInfo )
{
	if ( ! sFileName.Begins ( L":" ) )
		return false;

	HKEY hKey = NULL;
	if ( RegOpenKeyEx ( HKEY_LOCAL_MACHINE, Str_c ( L"Software\\Microsoft\\Shell\\Rai\\" ) + sFileName, 0, 0, &hKey ) != ERROR_SUCCESS )
		return false;

	if ( hKey == NULL )
		return false;

	DWORD dwType = REG_SZ;
	DWORD dwSize = 256;
	wchar_t szString [256];

	szString [0] = L'\0';
	if ( RegQueryValueEx ( hKey, L"1", 0, &dwType, (PBYTE)szString, &dwSize ) != ERROR_SUCCESS )
	{
		RegCloseKey ( hKey );
		return false;
	}

	szString [dwSize] = L'\0';

	RegCloseKey ( hKey );

	if ( ! Str_c ( szString ).Ends ( L".exe" ) )
		return false;
	
	tAppInfo.m_sFileName = szString;

	return true;
}

bool EnumApps ()
{
	g_dApps.Clear ();

	wchar_t szPath [MAX_PATH];
	if ( ! SHGetSpecialFolderPath ( g_hMainWindow, szPath, CSIDL_PROGRAMS, FALSE ) )
		return false;

	Str_c sPath = szPath;
	int iPos = sPath.RFind ( L'\\' );
	if ( iPos != -1 && iPos != 0 )
		wcscpy ( szPath, sPath.Chop ( sPath.Length () - iPos ) );

	Str_c sProgramsPath = szPath;

	FileIteratorTree_c tIterator;
	tIterator.IterateStart ( sProgramsPath );

	ExtFilter_c tFilter;
	tFilter.Set ( L"lnk" );

	Str_c sFileName, sParams, sDir, sName, sExt;

	AppInfo_t tAppInfo;


	while ( tIterator.IterateNext () )
	{
		const WIN32_FIND_DATA * pData = tIterator.GetData ();
		if ( ! pData || tIterator.Is2ndPassDir () || ( pData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
			continue;
		
		if ( ! tFilter.Fits ( pData->cFileName ) )
			continue;

		bool bDir;

		sFileName = sProgramsPath + L"\\" + tIterator.GetFileName ();
		if ( ! DecomposeLnk ( sFileName, sParams, bDir ) )
			continue;

		if ( bDir )
			continue;

		SplitPath ( tIterator.GetFileName (), sDir, sName, sExt );

		tAppInfo.m_sName = sName;

		if ( sFileName.Ends ( L".exe" ) )
			tAppInfo.m_sFileName = sFileName;
		else
			if ( ! ReadAppFromRegistry ( sFileName, tAppInfo ) )
				continue;
		
		tAppInfo.m_bBuiltIn = tAppInfo.m_sFileName.Find ( L'\\' ) == -1;
		tAppInfo.LoadIcon ();
		tAppInfo.m_bRecommended = false;
		g_dApps.Add ( tAppInfo );
	}

	Sort ( g_dApps, AppCompare );

	return true;
}

void AddRecommendedApp ( const Str_c & sFilename, const Str_c & sName )
{
	int iOldApp = FindAppByFilename ( sFilename );
	if ( iOldApp == -1 )
	{
		if ( sFilename.Find ( L'%' ) == -1 )
		{
			AppInfo_t tAppInfo;
			tAppInfo.m_sFileName	= sFilename;
			tAppInfo.m_sName		= sName;
			tAppInfo.m_bBuiltIn		= sFilename.Find ( L'\\' ) == -1;
			tAppInfo.m_bRecommended	= true;
			tAppInfo.LoadIcon ();

			g_dApps.Add ( tAppInfo );
		}
	}
	else
		g_dApps [iOldApp].m_bRecommended = true;
}

bool EnumAppsFor ( const Str_c & sFileName )
{
	if ( ! EnumApps () )
		return false;

	Str_c sCommand;
	if ( GetCommandFor ( sFileName, sCommand ) )
	{
		Str_c sExeFile = FilenameFromCommand ( sCommand );
		AddRecommendedApp ( sExeFile, GetName ( sExeFile ) );
	}

	const OpenWithArray_t * pArray = g_tRecent.GetOpenWithFor ( GetExt ( sFileName ) );

	if ( pArray )
	{
		for ( int i = pArray->Length () - 1; i >= 0 ; --i )
		{
			const RecentOpenWith_t & OpenWith = (*pArray) [i];
			AddRecommendedApp ( OpenWith.m_sFilename, OpenWith.m_sName );
		}
	}

	return true;
}

int GetNumApps ()
{
	return g_dApps.Length ();
}

const AppInfo_t & GetApp ( int iApp )
{
	return g_dApps [iApp];
}

bool GetCommandFor ( const Str_c & sFileName, Str_c & sCommand )
{
	Str_c sDir, sName, sExt;

	SplitPath ( sFileName, sDir, sName, sExt );

	HKEY hKey = NULL;
	if ( RegOpenKeyEx ( HKEY_CLASSES_ROOT, sExt, 0, 0, &hKey ) != ERROR_SUCCESS )
		return false;

	if ( hKey == NULL )
		return false;

	DWORD dwType = REG_SZ;
	DWORD dwSize = 512;
	wchar_t szString [512];

	szString [0] = L'\0';
	LONG iRes = RegQueryValueEx ( hKey, L"", 0, &dwType, (PBYTE)szString, &dwSize );
	RegCloseKey ( hKey );

	if ( iRes != ERROR_SUCCESS )
		return false;

	szString [dwSize] = L'\0';

	Str_c sType = szString;

	hKey = NULL;
	if ( RegOpenKeyEx ( HKEY_CLASSES_ROOT, sType + L"\\Shell\\Open\\Command", 0, 0, &hKey ) != ERROR_SUCCESS )
		return false;

	if ( hKey == NULL )
		return false;

	dwType = REG_SZ;
	dwSize = 512;
	szString [0] = L'\0';
	iRes = RegQueryValueEx ( hKey, L"", 0, &dwType, (PBYTE)szString, &dwSize );
	RegCloseKey ( hKey );

	if ( iRes != ERROR_SUCCESS )
		return false;

	szString [dwSize] = L'\0';

	sCommand = szString;
	
	return true;
}

int GetAppFor ( const Str_c & sFileName )
{
	Str_c sCommand;
	if ( ! GetCommandFor ( sFileName, sCommand ) )
		return -1;

	return FindAppByFilename ( FilenameFromCommand ( sCommand ) );
}

int	FindAppByFilename ( const Str_c & sFileName )
{
	for ( int i = 0; i < g_dApps.Length (); ++i )
		if ( g_dApps [i].m_sFileName == sFileName )
			return i;

	return -1;
}

int AddApp ( const AppInfo_t & tApp )
{
	return g_dApps.Add ( tApp );
}

void Associate ( const Str_c & sFileName, int iApp, bool bQuotes )
{
	if ( iApp < 0 )
		return;

	Str_c sDir, sName, sExt;

	SplitPath ( sFileName, sDir, sName, sExt );

	HKEY hKey = NULL;
	DWORD dwDisposition;
	bool bKeyExists = true;

	if ( RegCreateKeyEx ( HKEY_CLASSES_ROOT, sExt, 0, NULL, 0, 0, NULL, &hKey, &dwDisposition ) != ERROR_SUCCESS )
		return;

	if ( hKey == NULL )
		return;

	if ( dwDisposition != REG_OPENED_EXISTING_KEY )
		bKeyExists = false;

	wchar_t szString [512];
	szString [0] = L'\0';

	if ( bKeyExists )
	{
		DWORD dwSize = 512;
		DWORD dwType = REG_SZ;
		
		if ( RegQueryValueEx ( hKey, NULL, 0, &dwType, (PBYTE)szString, &dwSize ) != ERROR_SUCCESS )
			bKeyExists = false;

		szString [dwSize] = L'\0';
	}

	Str_c sType;

	// no key - create a new one
	if ( ! bKeyExists )
	{
		sType = L"PFM_";

		Str_c sNewExt = sExt;
		if ( sExt.Begins ( L"." ) )
			sNewExt = sExt.SubStr ( 1 );

		sType += sNewExt + L"file";

		RegSetValueEx ( hKey, NULL, 0, REG_SZ, (PBYTE)sType.c_str (), ( sType.Length () + 1 ) * sizeof ( wchar_t ) );
	}
	else
		sType = szString;

	RegCloseKey ( hKey );
	hKey = NULL;

	if ( RegCreateKeyEx ( HKEY_CLASSES_ROOT, sType + L"\\Shell\\Open\\Command", 0, NULL, 0, 0, NULL, &hKey, &dwDisposition ) != ERROR_SUCCESS )
		return;

	if ( hKey == NULL )
		return;

	const AppInfo_t & tApp = GetApp ( iApp );
	Str_c sCommand = NewString ( bQuotes ? L"\"%s\" \"%%1\"" : L"\"%s\" %%1", tApp.m_sFileName.c_str () );
	RegSetValueEx ( hKey, NULL, 0, REG_SZ, (PBYTE)sCommand.c_str (), ( sCommand.Length () + 1 ) * sizeof ( wchar_t ) );
	RegCloseKey ( hKey );
	hKey = NULL;

	if ( RegCreateKeyEx ( HKEY_CLASSES_ROOT, sType + L"\\DefaultIcon", 0, NULL, 0, 0, NULL, &hKey, &dwDisposition ) != ERROR_SUCCESS )
		return;

	if ( hKey == NULL )
		return;

	Str_c sIcon = NewString ( L"%s,-%d", tApp.m_sFileName.c_str (), 101 );
	RegSetValueEx ( hKey, NULL, 0, REG_SZ, (PBYTE)sIcon.c_str (), ( sIcon.Length () + 1 ) * sizeof ( wchar_t ) );
	RegCloseKey ( hKey );	
}

} //namespace apps

namespace newmenu
{
	Array_T < NewItem_t > g_dItems;

	int ItemCompare ( const NewItem_t & Item1, const NewItem_t & Item2 )
	{
		return wcscmp ( Item1.m_sName, Item2.m_sName );
	}

	void NewItem_t::Run () const
	{
		CoInitializeEx ( NULL, COINIT_MULTITHREADED );
		void * pId = NULL;
		GUID IID_INewMenuItemServer = { 0x42650bc0, 0x41c1, 0x11d2, { 0x88, 0xe3, 0x0, 0x0, 0xf8, 0x7a, 0x49, 0xdb } };
		CoCreateInstance ( m_GUID, NULL, CLSCTX_INPROC_SERVER, IID_INewMenuItemServer, &pId );
		if ( pId )
			((INewMenuItemServer*) pId)->CreateNewItem ( NULL );
		CoUninitialize ();
	}

	bool EnumNewItems ()
	{
		g_dItems.Clear ();

		HKEY hKey = NULL;
		if ( RegOpenKeyEx ( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Shell\\Extensions\\NewMenu", 0, 0, &hKey ) != ERROR_SUCCESS )
			return false;

		if ( hKey == NULL )
			return false;

		DWORD dwIndex = 0;

		wchar_t szKeyName [256];
		wchar_t szData [256];
		FILETIME FileTime;
		DWORD dwSize = 256;
		DWORD dwType = REG_SZ;
		NewItem_t NewItem;

		while ( RegEnumKeyEx ( hKey, dwIndex, szKeyName, &dwSize, NULL, NULL, NULL, &FileTime ) == ERROR_SUCCESS )
		{
			HKEY hTempKey;
			if ( RegOpenKeyEx ( HKEY_LOCAL_MACHINE, Str_c ( L"Software\\Microsoft\\Shell\\Extensions\\NewMenu\\" ) + szKeyName, 0, 0, &hTempKey ) == ERROR_SUCCESS )
			{
				dwSize = 256;
				if ( RegQueryValueEx ( hTempKey, NULL, NULL, &dwType, (PBYTE)szData, &dwSize ) == ERROR_SUCCESS )
				{
					NewItem.m_GUID = StrToGUID ( szKeyName );
					NewItem.m_sName = szData;
					g_dItems.Add ( NewItem );
				}

				RegCloseKey ( hTempKey );
			}

			dwSize = 256;
			++dwIndex;
		}

		RegCloseKey ( hKey );

		Sort ( g_dItems, ItemCompare );

		return true;
	}

	int	GetNumItems ()
	{
		return g_dItems.Length ();
	}

	const NewItem_t & GetItem ( int iItem )
	{
		return g_dItems [iItem];
	}
} //namespace newmenu