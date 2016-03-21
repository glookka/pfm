#include "Registry.h"

#include "search.h"

HINSTANCE g_hInstance = NULL;

RegRoot_t g_RegRoot [REG_ROOT_ITEMS] =
{	 
	 { L"HKEY_CLASSES_ROOT",  L"HKCR",	HKEY_CLASSES_ROOT }
	,{ L"HKEY_CURRENT_USER",  L"HKCU",	HKEY_CURRENT_USER }
	,{ L"HKEY_LOCAL_MACHINE", L"HKLM",	HKEY_LOCAL_MACHINE }
	,{ L"HKEY_USERS",		  L"HKU",	HKEY_USERS }
};

wchar_t * g_szKey = L"Key";

// topmost is most recent
static wchar_t * g_szRecentTexts [MAX_RECENT_TEXTS];
PluginStartupInfo_t g_PSI;

HIMAGELIST			RegPlugin_c::m_hImageList = NULL;
bool				RegPlugin_c::m_bInitialized = false;
HANDLE				RegPlugin_c::m_hCfg;
wchar_t *			RegPlugin_c::m_pFolderColumns [2];
ColumnType_e		RegPlugin_c::m_dFileInfoCols [1];
PanelView_t			RegPlugin_c::m_BriefView;
PanelView_t			RegPlugin_c::m_MediumView;
PanelView_t			RegPlugin_c::m_FullView;

Config_t g_Cfg;


//////////////////////////////////////////////////////////////////////////
Config_t::Config_t ()
	: find_area			( 0 )
	, find_case			( 0 )
	, find_key_names	( 1 )
	, find_value_names	( 1 )
	, find_data_sz		( 1 )
	, find_data_multisz	( 1 )
	, find_data_dword	( 1 )
	, find_data_binary	( 1 )
	, find_bin_mode		( 0 )
{
}

void Help ( const wchar_t * szSection )
{
	wchar_t szBuffer [128];
	wsprintf ( szBuffer, L"file:PFM_registry.htm%s", szSection );
	CreateProcess ( L"peghelp.exe", szBuffer, NULL, NULL, FALSE, 0, NULL, NULL, NULL, NULL ); 
}


//////////////////////////////////////////////////////////////////////////
static void AddRecentText ( const wchar_t * szText, wchar_t ** ppContainer, int iMaxTexts, int iMaxLength )
{
	if ( !szText )
		return;

	bool bFound = false;
	for ( int i = 0; i < iMaxTexts && ppContainer [i] && !bFound; i++ )
		if ( !wcscmp ( ppContainer [i], szText ) )
		{
			bFound = true;
			wchar_t * szTmp = ppContainer [i];
			ppContainer [i] = ppContainer [0];
			ppContainer [0] = szTmp;
		}

		if ( !bFound )
		{
			delete [] ppContainer [iMaxTexts-1];

			for ( int i = iMaxTexts - 1; i > 0 ; i-- )
				ppContainer [i] = ppContainer [i-1];

			ppContainer [0] = new wchar_t [iMaxLength];
			wcsncpy ( ppContainer [0], szText, iMaxLength );
		}
}


//////////////////////////////////////////////////////////////////////////
// find texts
static void FindTextLoad ( const wchar_t * szText )
{
	for ( int i = 0; i < MAX_RECENT_TEXTS; i++ )
		if ( !g_szRecentTexts [i] )
		{
			g_szRecentTexts [i] = new wchar_t [MAX_SEARCH];
			wcsncpy ( g_szRecentTexts [i], szText, MAX_SEARCH );
			break;
		}
}

static const wchar_t * FindTextSave ( int iId )
{
	if ( iId >= MAX_RECENT_TEXTS || !g_szRecentTexts [iId] )
		return NULL;

	return g_szRecentTexts [iId];

}

void FindTextAdd ( const wchar_t * szText )
{
	AddRecentText ( szText, g_szRecentTexts, MAX_RECENT_TEXTS, MAX_SEARCH );
}


const wchar_t * FindTextGet ( int iId )
{
	return g_szRecentTexts [iId];
}


HKEY GetRootKey ( const wchar_t * szPath )
{
	for ( int i = 0; i < REG_ROOT_ITEMS; ++i )
		if ( wcsstr ( szPath, g_RegRoot [i].m_szName ) == szPath || wcsstr ( szPath, g_RegRoot [i].m_szShortName ) == szPath )
			return g_RegRoot [i].m_hKey;

	return NULL;
}


void ToShortPath ( wchar_t * szShort, const wchar_t * szFull )
{
	for ( int i = 0; i < REG_ROOT_ITEMS; ++i )
		if ( wcsstr ( szFull, g_RegRoot [i].m_szName ) == szFull )
		{
			int iStart = wcslen ( g_RegRoot [i].m_szName );
			wcscpy ( szShort, g_RegRoot [i].m_szShortName );
			wcscat ( szShort, &(szFull [iStart]) );
			break;
		}
}


int	GetIconByType ( DWORD uType, bool bKey )
{
	if ( bKey )
		return 0;

	switch ( uType )
	{
	case REG_SZ:			return 1;
	case REG_MULTI_SZ:		return 2;
	case REG_DWORD:			return 3;
	case REG_BINARY:		return 4;
	}

	return 5;
}

static wchar_t * g_szTypes [] = 
{
	 L"None"			// 0
	,L"SZ"				// 1
	,L"Binary"			// 3
	,L"DWORD"			// 4
	,L"MultiSZ"			// 7
};


const wchar_t * GetTypeString ( DWORD uType )
{
	switch ( uType )
	{
	case REG_SZ:		return g_szTypes [1];
	case REG_BINARY:	return g_szTypes [2];
	case REG_DWORD:		return g_szTypes [3];
	case REG_MULTI_SZ:	return g_szTypes [4];
	}

	return g_szTypes [0];
}

static PanelItem_t **	g_pItems = NULL;
static int				g_nItems = 0;
static RegPlugin_c *	g_pPlugin = NULL;

static BOOL CALLBACK DeleteRequestDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
		{
			SendMessage ( GetDlgItem ( hDlg, IDC_TITLE ), WM_SETFONT, (WPARAM)g_PSI.m_hBoldFont, TRUE );
			DlgTxt ( hDlg, IDOK,	TBAR_OK );
			DlgTxt ( hDlg, IDCANCEL,TBAR_CANCEL );
			
			if ( g_nItems > 1 )
			{
				wchar_t szText [128];
				wsprintf ( szText, Txt ( DLG_DELETE_N_KEYS ), g_nItems );
				SetDlgItemText ( hDlg, IDC_TITLE, szText );
			}
			else
			{
				const PanelItem_t & Info = *g_pItems[0];
				if ( Info.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
					DlgTxt ( hDlg, IDC_TITLE, DLG_DELETE_KEY );
				else
					DlgTxt ( hDlg, IDC_TITLE, DLG_DELETE_VALUE );

				wchar_t szFile [MAX_REG_PATH];
				wcsncpy ( szFile, g_pPlugin->GetPath (), MAX_REG_PATH );
				wcsncat ( szFile, L"\\", MAX_REG_PATH-wcslen ( szFile ) );
				wcsncat ( szFile, Info.m_FindData.cFileName, MAX_REG_PATH-wcslen ( szFile ) );

				g_PSI.m_fnAlignText ( GetDlgItem ( hDlg, IDC_TEXT ), szFile );
			}

			g_PSI.m_fnCreateToolbar ( hDlg, TOOLBAR_OK_CANCEL, SHCMBF_HIDESIPBUTTON );
		}
		break;

	case WM_COMMAND:
		{
			int iCommand = LOWORD(wParam);
			if ( iCommand == IDOK || iCommand == IDCANCEL )
				EndDialog ( hDlg, iCommand );
		}
		break;

	case WM_HELP:
		Help ( L"Main_Contents" );
		return TRUE;
	}

	return g_PSI.m_fnDlgMoving ( hDlg, Msg, wParam, lParam );
}

//////////////////////////////////////////////////////////////////////////
// registry plugin implementation
void RegPlugin_c::Init ()
{
	if ( m_bInitialized )
		return;

	LoadImageList ();

	memset ( &m_BriefView, 0, sizeof ( m_BriefView ) );
	m_BriefView.m_dColumns [0].m_eType		= COL_FILENAME;
	m_BriefView.m_dColumns [1].m_eType		= COL_TEXT_0;
	m_BriefView.m_nColumns	= 2;

	memset ( &m_MediumView, 0, sizeof ( m_MediumView ) );
	m_MediumView.m_dColumns [0].m_eType			= COL_FILENAME;
	m_MediumView.m_dColumns [1].m_eType			= COL_TEXT_1;
	m_MediumView.m_dColumns [1].m_bRightAlign	= true;
	m_MediumView.m_dColumns [1].m_fMaxWidth		= 0.55f;
	m_MediumView.m_nColumns	= 2;

	memset ( &m_FullView, 0, sizeof ( m_FullView ) );
	m_FullView.m_dColumns [0].m_eType		= COL_FILENAME;
	m_FullView.m_dColumns [1].m_eType		= COL_TEXT_1;
	m_FullView.m_dColumns [1].m_bRightAlign	= true;
	m_FullView.m_dColumns [1].m_fMaxWidth	= 0.4f;
	m_FullView.m_dColumns [2].m_eType		= COL_TEXT_0;
	m_FullView.m_nColumns	= 3;

	// common data for all folders
	m_pFolderColumns [0] = g_szKey;
	m_pFolderColumns [1] = g_szKey;

	m_dFileInfoCols [0] = COL_TEXT_0;

	// initialize config variables
	memset ( g_szRecentTexts, 0, sizeof ( g_szRecentTexts ) );

	m_hCfg = g_PSI.m_fnCfgCreate ();
	g_PSI.m_fnCfgInt ( m_hCfg, L"find_area",		&g_Cfg.find_area );
	g_PSI.m_fnCfgInt ( m_hCfg, L"find_case",		&g_Cfg.find_case );
	g_PSI.m_fnCfgInt ( m_hCfg, L"find_key_names",	&g_Cfg.find_key_names );
	g_PSI.m_fnCfgInt ( m_hCfg, L"find_value_names",	&g_Cfg.find_value_names );
	g_PSI.m_fnCfgInt ( m_hCfg, L"find_data_sz",		&g_Cfg.find_data_sz );
	g_PSI.m_fnCfgInt ( m_hCfg, L"find_data_multisz",&g_Cfg.find_data_multisz );
	g_PSI.m_fnCfgInt ( m_hCfg, L"find_data_dword",	&g_Cfg.find_data_dword );
	g_PSI.m_fnCfgInt ( m_hCfg, L"find_data_binary",	&g_Cfg.find_data_binary );
	g_PSI.m_fnCfgInt ( m_hCfg, L"find_bin_mode",	&g_Cfg.find_bin_mode );
	g_PSI.m_fnCfgStr ( m_hCfg, L"find_text", NULL, MAX_SEARCH, FindTextLoad, FindTextSave );

	wchar_t szConfig [MAX_PATH];
	wsprintf ( szConfig, L"%sregistry.ini", g_PSI.m_szRoot );
	g_PSI.m_fnCfgLoad ( m_hCfg, szConfig );
	m_bInitialized = true;
}


void RegPlugin_c::Shutdown ()
{
	DestroyImageList ();

	g_PSI.m_fnCfgSave ( m_hCfg );
	g_PSI.m_fnCfgDestroy ( m_hCfg );

	for ( int i = 0; i < MAX_RECENT_TEXTS; i++ )
		delete [] g_szRecentTexts [i];
}

RegPlugin_c::RegPlugin_c ()
{
	m_szPath [0] = '\0';
}

RegPlugin_c::~RegPlugin_c ()
{
}

void RegPlugin_c::LoadImageList ()
{
	bool bVGA = g_PSI.m_bVGA;

	SIZE ImgSize;
	HBITMAP hBitmap = g_PSI.m_fnLoadTGA ( g_hInstance, bVGA ? IDB_ICONS_VGA : IDB_ICONS, &ImgSize );
	if ( !hBitmap )
		return;

	m_hImageList = ImageList_Create ( ImgSize.cy, ImgSize.cy, 0x018 | ILC_MASK, 5, 0 );
	if ( !m_hImageList )
		return;

	if ( ImageList_AddMasked ( m_hImageList, hBitmap, 0xFF00FF ) == -1 )
	{
		ImageList_Destroy ( m_hImageList );
		m_hImageList = NULL;
	}

	DeleteObject ( hBitmap );
}

void RegPlugin_c::DestroyImageList ()
{
	if ( m_hImageList )
		ImageList_Destroy ( m_hImageList );
}

bool RegPlugin_c::PSetDirectory ( const wchar_t * szDir )
{
	int iLen = wcslen ( szDir );
	if ( iLen < 4 || iLen > MAX_REG_PATH )
		return false;

	if ( wcsstr ( szDir, L":reg" ) != szDir )
		return false;

	// cut front slash
	if ( szDir [4] == '\\' )
		wcscpy ( m_szPath, szDir + 5 );
	else
		wcscpy ( m_szPath, szDir + 4 );

	// cut trailing slash
	int iPathLen = wcslen ( m_szPath );
	if ( m_szPath [iPathLen-1] == '\\')
		m_szPath [iPathLen-1] = '\0';

	return true;
}

bool RegPlugin_c::PGetFindData ( PanelItem_t * & dItems, int & nItems )
{
	DWORD uDataLen = MAX_DATA_LEN;
	BYTE dData [MAX_DATA_LEN];

	if ( m_szPath [0] == '\0' )
	{
		PanelItem_t * dNewItems = new PanelItem_t [REG_ROOT_ITEMS];
		memset ( dNewItems, 0, sizeof ( PanelItem_t ) * REG_ROOT_ITEMS );
		for ( int i = 0; i < REG_ROOT_ITEMS; ++i )
			SetRootItem ( dNewItems, i, g_RegRoot [i].m_szName );

		dItems = dNewItems;
		nItems = REG_ROOT_ITEMS;
		return true;
	}
	else
	{
		HKEY hResult = NULL;

		HKEY hRootKey = GetRootKey ( m_szPath );
		if ( !hRootKey )
			return false;

		const wchar_t * szSubkey = wcsstr ( m_szPath, L"\\" );
		if ( !szSubkey )
			hResult = hRootKey;
		else
		{
			if ( RegOpenKeyEx ( hRootKey, szSubkey, 0, 0, &hResult ) != ERROR_SUCCESS )
				return false;

			if ( !hResult )
				return false;
		}

		DWORD nKeys = 0, nValues = 0;
		RegQueryInfoKey ( hResult, NULL, NULL, NULL, &nKeys, NULL, NULL, &nValues, NULL, NULL, NULL, NULL );

		PanelItem_t * dNewItems = new PanelItem_t [nKeys + nValues + 1];
		memset ( dNewItems, 0, sizeof ( PanelItem_t ) * ( nKeys + nValues + 1 ) );
		wcscpy ( dNewItems [0].m_FindData.cFileName, L".." );
		dNewItems [0].m_FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		dNewItems [0].m_FindData.nFileSizeLow = 0;
		dNewItems [0].m_iIcon = GetIconByType ( 0, true );
		dNewItems [0].m_nCustomColumns = 2;
		dNewItems [0].m_dCustomColumnData = m_pFolderColumns;

		wchar_t szBuffer [MAX_PATH];
		DWORD uBufLen = MAX_PATH;
		DWORD iKey = 0;
		while ( RegEnumKeyEx ( hResult, iKey, szBuffer, &uBufLen, NULL, NULL, NULL, NULL ) == ERROR_SUCCESS  )
		{
			int iIndex = iKey+1;
			wcscpy ( dNewItems [iIndex].m_FindData.cFileName, szBuffer );
			dNewItems [iIndex].m_FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
			dNewItems [iIndex].m_FindData.nFileSizeLow = 0;
			dNewItems [iIndex].m_iIcon = 0;
			dNewItems [iIndex].m_nCustomColumns = 2;
			dNewItems [iIndex].m_dCustomColumnData = m_pFolderColumns;
			uBufLen = MAX_PATH;
			++iKey;
		}

		DWORD iValue = 0;
		DWORD uType = REG_NONE;
		LONG uRes = RegEnumValue ( hResult, iValue, szBuffer, &uBufLen, NULL, &uType, dData, &uDataLen );
		while ( uRes == ERROR_SUCCESS || uRes == ERROR_MORE_DATA )
		{
			int iIndex = iKey+iValue+1;

			wcscpy ( dNewItems [iIndex].m_FindData.cFileName, szBuffer );
			dNewItems [iIndex].m_FindData.nFileSizeLow = uType;
			dNewItems [iIndex].m_iIcon				= GetIconByType ( uType, false );
			dNewItems [iIndex].m_nCustomColumns		= 2;
			dNewItems [iIndex].m_dCustomColumnData	= CreateColumnData ( uType, uRes == ERROR_MORE_DATA ? NULL : dData, uDataLen );
			uDataLen = MAX_DATA_LEN;
			uBufLen = MAX_PATH;
			++iValue;

			uRes = RegEnumValue ( hResult, iValue, szBuffer, &uBufLen, NULL, &uType, dData, &uDataLen );
		}

		if ( hResult != hRootKey )
			RegCloseKey ( hResult );

		dItems = dNewItems;
		nItems = iKey+iValue+1;

		return true;
	}

	return false;
}


void RegPlugin_c::PFreeFindData ( PanelItem_t * dItems, int nItems )
{
	for ( int i = 0; i < nItems; ++i )
		if ( dItems [i].m_dCustomColumnData != m_pFolderColumns )
		{
			delete [] dItems [i].m_dCustomColumnData [1];
			delete [] dItems [i].m_dCustomColumnData;
		}

	delete [] dItems;
}


bool RegPlugin_c::PDeleteFiles ( PanelItem_t ** dItems, int nItems )
{
	g_pItems = dItems;
	g_nItems = nItems;
	g_pPlugin = this;

	int iRes = DialogBox ( g_hInstance, MAKEINTRESOURCE ( IDD_DELETE_REQUEST ), g_PSI.m_hMainWindow, DeleteRequestDlgProc );
	if ( iRes != IDOK )
		return false;

	!!! 'please wait'
		
	HKEY hRootKey = GetRootKey ( m_szPath );
	if ( !hRootKey )
		return false;

	const wchar_t * szSubkey = wcsstr ( m_szPath, L"\\" );
	if ( szSubkey )
		szSubkey++;

	bool bAnyOk = false;
	LONG uResult;
	for ( int i = 0; i < nItems; ++i )
	{
		if ( !szSubkey )
			uResult = RegDeleteKey ( hRootKey, dItems [i]->m_FindData.cFileName );
		else
		{
			wchar_t szName [MAX_REG_PATH];
			wcsncpy ( szName, szSubkey, MAX_REG_PATH );
			wcsncat ( szName, L"\\", MAX_REG_PATH-wcslen(szName) );
			wcsncat ( szName, dItems [i]->m_FindData.cFileName, MAX_REG_PATH-wcslen(szName) );

			uResult = RegDeleteKey ( hRootKey, szName );
			if ( uResult != ERROR_SUCCESS )
			{
				HKEY hResultKey = NULL;
				if ( RegOpenKeyEx ( hRootKey, szSubkey, 0, KEY_SET_VALUE, &hResultKey ) == ERROR_SUCCESS )
				{
					uResult = RegDeleteValue ( hResultKey, dItems [i]->m_FindData.cFileName );
					RegCloseKey ( hResultKey );
				}
			}
		}

		if ( uResult == ERROR_SUCCESS )
			bAnyOk = true;
	}
	
	return bAnyOk;

	SetCursor ( LoadCursor ( NULL, IDC_ARROW ) );

	return false;
}


bool RegPlugin_c::PFindFiles ( PanelItem_t ** dItems, int nItems )
{
	if ( !FindRequestDlg ( dItems, nItems ) )
		return false;

	if ( !FindProgressDlg ( this, m_szPath, dItems, nItems ) )
		return false;

	return true;
}


const wchar_t *	RegPlugin_c::GetPath () const
{
	return m_szPath;
}


void RegPlugin_c::SetRootItem ( PanelItem_t * dItems, int nItem, const wchar_t * szName )
{
	wcscpy ( dItems [nItem].m_FindData.cFileName, szName );
	dItems [nItem].m_FindData.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	dItems [nItem].m_iIcon = 0;
	dItems [nItem].m_nCustomColumns = 2;
	dItems [nItem].m_dCustomColumnData = m_pFolderColumns;
}


wchar_t ** RegPlugin_c::CreateColumnData ( DWORD uType, BYTE * pData, DWORD uDataLen )
{
	const int MAX_COLUMN_CHARS = 80;

	wchar_t ** pColumnData = new wchar_t * [2];
	pColumnData [0] = (wchar_t *)GetTypeString ( uType );
	pColumnData [1] = NULL;

	switch ( uType )
	{
	case REG_BINARY:
		{
			int nGroups = Min ( (int)uDataLen, MAX_COLUMN_CHARS/3 );
			wchar_t * pString = new wchar_t [nGroups*3 + 1];

			for ( int i = 0; i < nGroups; ++i )
				wsprintf ( pString + i*3, L"%02X ", pData [i] );

			pColumnData [1] = pString;
		}
		break;

	case REG_DWORD:
		if ( uDataLen == 4 )
		{
			wchar_t * pString = new wchar_t [11];
			wsprintf ( pString, L"0x%08X", *(DWORD*) (pData) );
			pColumnData [1] = pString;
		}
		break;

	case REG_MULTI_SZ:
	case REG_SZ:
		{
			int iLen = wcslen ( (wchar_t *)pData );
			int nChars = Min ( iLen, (int) uDataLen/2, MAX_COLUMN_CHARS );
			if ( nChars > 0 )
			{
				wchar_t * pString = new wchar_t [nChars+1];
				wcsncpy ( pString, (wchar_t*)pData, nChars );
				pString [nChars] = L'\0';
				pColumnData [1] = pString;
			}
		}
		break;
	}

	return pColumnData;
}

//////////////////////////////////////////////////////////////////////////
// dll interface
bool GetFindData ( HANDLE hPlugin, PanelItem_t * & dItems, int & nItems )
{
	RegPlugin_c * pPlugin = (RegPlugin_c *) hPlugin;
	return pPlugin->PGetFindData ( dItems, nItems );
}

void FreeFindData ( HANDLE hPlugin, PanelItem_t * dItems, int nItems)
{
	RegPlugin_c * pPlugin = (RegPlugin_c *) hPlugin;
	pPlugin->PFreeFindData ( dItems, nItems );
}

bool SetDirectory ( HANDLE hPlugin, const wchar_t * szDir )
{
	RegPlugin_c * pPlugin = (RegPlugin_c *) hPlugin;
	return pPlugin->PSetDirectory ( szDir );
}

bool DeleteFiles ( HANDLE hPlugin, PanelItem_t ** dItems, int nItems )
{
	RegPlugin_c * pPlugin = (RegPlugin_c *) hPlugin;
	return pPlugin->PDeleteFiles ( dItems, nItems );
}


bool FindFiles ( HANDLE hPlugin, PanelItem_t ** dItems, int nItems )
{
	RegPlugin_c * pPlugin = (RegPlugin_c *) hPlugin;
	return pPlugin->PFindFiles ( dItems, nItems );
}


void GetPluginInfo ( PluginInfo_t & Info )
{
	RegPlugin_c::Init ();

	Info.m_uFlags = OPIF_USEHIGHLIGHTING | OPIF_USEATTRHIGHLIGHTING;
	Info.m_hImageList = RegPlugin_c::m_hImageList;
	Info.m_dPanelViews [VM_BRIEF]	= &RegPlugin_c::m_BriefView;
	Info.m_dPanelViews [VM_MEDIUM]	= &RegPlugin_c::m_MediumView;
	Info.m_dPanelViews [VM_FULL]	= &RegPlugin_c::m_FullView;
	Info.m_nFileInfoCols = 1;
	Info.m_dFileInfoCols = RegPlugin_c::m_dFileInfoCols;

}

void SetStartupInfo ( const PluginStartupInfo_t & Info )
{
	g_PSI = Info;
}

//////////////////////////////////////////////////////////////////////////
// search helpers

static bool IsDefaultValue ( const PanelItem_t & Item )
{
	if ( Item.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		return false;

	if ( Item.m_FindData.cFileName[0] != L'D' )
		return false;

	return wcscmp ( Item.m_FindData.cFileName, L"Default" ) == 0;
}

static DWORD ConvertToSort ( DWORD uType )
{
	switch ( uType )
	{
	case REG_DWORD:		return 0;
	case REG_SZ:		return 1;
	case REG_MULTI_SZ:	return 2;
	case REG_BINARY:	return 3;
	}

	return 4;
}

static int CompareKeyTypes ( DWORD uType1, DWORD uType2 )
{
	DWORD uTypeClamped1 = ConvertToSort ( uType1 );
	DWORD uTypeClamped2 = ConvertToSort ( uType2 );

	if ( uTypeClamped1 < uTypeClamped2 )
		return -1;
	else if ( uTypeClamped1 > uTypeClamped2 )
		return 1;

	return 0;
}

static int CompareFilenames ( const wchar_t * szFilename1, const wchar_t * szFilename2 )
{
	static wchar_t szName1 [MAX_PATH];
	static wchar_t szName2 [MAX_PATH];

	int iRes = _wcsicmp ( szName1, szName2 );

	return iRes;
}


int Compare ( HANDLE hPlugin, const PanelItem_t & Item1, const PanelItem_t & Item2, SortMode_e eSortMode, bool bReverse )
{
	int iRes = -1;
	bool bCompareDirs = false;

	if ( Item1.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
	{
		if ( Item2.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			bCompareDirs = true;
		else
			return -1;
	}
	else
		if ( Item2.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			return 1;

	if ( bCompareDirs && eSortMode == SORT_EXTENSION )
		eSortMode = SORT_NAME;

	// 'Default' handling
	if ( !bCompareDirs )
	{
		if ( IsDefaultValue ( Item1 ) )
		{
			if ( !IsDefaultValue ( Item2 ) )
				return -1;
		}
		else
			if ( IsDefaultValue ( Item2 ) )
				return 1;
	}

	switch ( eSortMode )
	{
	case SORT_NAME:
	case SORT_SIZE:
	case SORT_SIZE_PACKED:
	case SORT_TIME_ACCESS:
	case SORT_TIME_CREATE:
	case SORT_TIME_WRITE:
		iRes = _wcsicmp ( Item1.m_FindData.cFileName, Item2.m_FindData.cFileName );
		break;

	case SORT_GROUP:
	case SORT_EXTENSION:
		iRes = CompareKeyTypes ( Item1.m_FindData.nFileSizeLow, Item2.m_FindData.nFileSizeLow );
		if ( !iRes )
			iRes = _wcsicmp ( Item1.m_FindData.cFileName, Item2.m_FindData.cFileName );
		break;
	}

	if ( iRes < -1 )
		iRes = -1;
	else if ( iRes > 1 )
		iRes = 1;

	return bReverse ? -iRes : iRes;
}

HANDLE OpenPlugin ()
{
	return (HANDLE) new RegPlugin_c;
}

void ClosePlugin ( HANDLE hPlugin )
{
	delete (RegPlugin_c *) hPlugin;
}

void ExitPFM ()
{
	RegPlugin_c::Shutdown ();
}

BOOL WINAPI DllMain ( HANDLE hinstDLL, DWORD dwReason, LPVOID lpvReserved )
{
	g_hInstance = (HINSTANCE)hinstDLL;
	return TRUE;
}