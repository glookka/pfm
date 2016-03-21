#include "pch.h"
#include "plugins.h"

#include "system.h"
#include "gui.h"
#include "config.h"
#include "iterator.h"
#include "panel.h"
#include "pfm.h"
#include "dialogs/errors.h"
#include "resources.h"

PluginStartupInfo_t	Plugin_c::m_PluginStartupInfo;
PluginInfo_t Plugin_c::m_PluginInfo;

extern HWND g_hMainWindow;

//////////////////////////////////////////////////////////////////////////
// helpers
float TimerGetTimeSec ()
{
	return (float)(g_Timer.GetTimeSec ());
}

//////////////////////////////////////////////////////////////////////////
// a plugin
Plugin_c::Plugin_c ( const wchar_t * szPath, const wchar_t * szDll, DWORD uFlags )
	: m_sPath			( szPath )
	, m_sDll			( szDll )
	, m_uFlags			( uFlags )
	, m_hLibrary		( NULL )
	, m_fnOpenPlugin	( NULL )
	, m_fnClosePlugin	( NULL )
	, m_fnGetFindData	( NULL )
	, m_fnFreeFindData	( NULL )
	, m_fnSetDirectory	( NULL )
	, m_fnCompare		( NULL )
	, m_fnExitPFM		( NULL )
	, m_fnDeleteFiles	( NULL )
	, m_fnFindFiles		( NULL )
	, m_fnHandleMenu	( NULL )
{
}


Plugin_c::~Plugin_c ()
{
	if ( m_fnExitPFM )
		m_fnExitPFM ();

	if ( m_hLibrary )
		FreeLibrary ( m_hLibrary );

	for ( int i = 0; i < m_dStrings.Length (); ++i )
		SafeDeleteArray ( m_dStrings [i] );
}


HANDLE Plugin_c::OpenFilePlugin ( const wchar_t * szPath )
{
	if ( m_uFlags & PLUGIN_ROOT_FS )
	{
		// early time checking - no need to load dlls
		if ( wcsstr ( szPath, m_sRootFSShortcut.c_str () ) != szPath )
			return INVALID_HANDLE_VALUE;
		else
		{
			LoadPluginLibrary ();

			if ( m_fnOpenPlugin )
				return m_fnOpenPlugin ();
		}
	}

	return INVALID_HANDLE_VALUE;
}


const PluginInfo_t * Plugin_c::GetPluginInfo ()
{
	return m_fnGetPluginInfo ? &m_PluginInfo : NULL;
}


void Plugin_c::ClosePlugin ( HANDLE hPlugin )
{
	if ( m_fnClosePlugin )
		m_fnClosePlugin ( hPlugin );
}


bool Plugin_c::GetFindData ( HANDLE hPlugin, PanelItem_t * & dItems, int & nItems )
{
	if ( !m_fnGetFindData )
		return FALSE;

	return m_fnGetFindData ( hPlugin, dItems, nItems );
}


void Plugin_c::FreeFindData ( HANDLE hPlugin, PanelItem_t * dItems, int nItems)
{
	if ( !m_fnFreeFindData )
		return;

	m_fnFreeFindData ( hPlugin, dItems, nItems );
}


bool Plugin_c::SetDirectory ( HANDLE hPlugin, const wchar_t * szDir )
{
	if ( !m_fnSetDirectory )
		return false;
		
	return m_fnSetDirectory ( hPlugin, szDir );
}


ItemCompare_t Plugin_c::GetCompareFunc () const
{
	return m_fnCompare;
}


bool Plugin_c::DeleteFiles ( HANDLE hPlugin, PanelItem_t ** dItems, int nItems )
{
	if ( !m_fnDeleteFiles )
		return true;

	return m_fnDeleteFiles ( hPlugin, dItems, nItems );
}


bool Plugin_c::FindFiles ( HANDLE hPlugin, PanelItem_t ** dItems, int nItems )
{
	if ( !m_fnFindFiles )
		return true;

	return m_fnFindFiles ( hPlugin, dItems, nItems );
}


void Plugin_c::HandleMenu ( int iSubmenuId, const wchar_t * szRoot, PanelItem_t ** dItems, int nItems )
{
	// no open plugin here, so the library may not be loaded
	if ( !m_hLibrary )
		LoadPluginLibrary ();

	if ( m_fnHandleMenu )
		m_fnHandleMenu ( iSubmenuId, szRoot, dItems, nItems );
}


DWORD Plugin_c::GetFlags () const
{
	return m_uFlags;
}


const wchar_t *	Plugin_c::GetDLLName () const
{
	return m_sDll.c_str ();
}


const wchar_t * Plugin_c::GetRootFSName () const
{
	return m_sRootFSName;
}


const wchar_t * Plugin_c::GetRootFSShortcut () const
{
	return m_sRootFSShortcut;
}


const wchar_t *	Plugin_c::GetMenuName () const
{
	return m_sSubmenu;
}


const wchar_t *	Plugin_c::GetSubmenuName ( int iSubmenu ) const
{
	return m_dSubmenu [iSubmenu];
}


int	Plugin_c::GetNumSubmenuItems () const
{
	return m_dSubmenu.Length ();
}


void Plugin_c::LoadPluginLibrary ()
{
	if ( !m_hLibrary )
		m_hLibrary = LoadLibrary ( m_sPath + m_sDll );

	if ( !m_hLibrary )
		return;

	m_fnOpenPlugin		= (OpenPlugin_fn)		GetProcAddress ( m_hLibrary, L"OpenPlugin" );
	m_fnGetPluginInfo	= (GetPluginInfo_fn)	GetProcAddress ( m_hLibrary, L"GetPluginInfo" );
	m_fnClosePlugin		= (ClosePlugin_fn)		GetProcAddress ( m_hLibrary, L"ClosePlugin" );
	m_fnExitPFM			= (ExitPFM_fn)			GetProcAddress ( m_hLibrary, L"ExitPFM" );
	m_fnGetFindData		= (GetFindData_fn)		GetProcAddress ( m_hLibrary, L"GetFindData" );
	m_fnFreeFindData	= (FreeFindData_fn)		GetProcAddress ( m_hLibrary, L"FreeFindData" );
	m_fnSetDirectory	= (SetDirectory_fn)		GetProcAddress ( m_hLibrary, L"SetDirectory" );
	m_fnCompare			= (ItemCompare_t)		GetProcAddress ( m_hLibrary, L"Compare" );
	m_fnSetStartupInfo	= (SetStartupInfo_fn)	GetProcAddress ( m_hLibrary, L"SetStartupInfo" );
	m_fnDeleteFiles		= (DeleteFiles_fn)		GetProcAddress ( m_hLibrary, L"DeleteFiles" );
	m_fnFindFiles		= (FindFiles_fn)		GetProcAddress ( m_hLibrary, L"FindFiles" );
	m_fnHandleMenu		= (HandleMenu_fn)		GetProcAddress ( m_hLibrary, L"HandleMenu" );

	if ( !LoadLangFile ( m_dStrings, m_sPath + GetLangFile () ) )
		LoadLangFile ( m_dStrings, m_sPath + GetDefaultLangFile () );

	m_PluginStartupInfo.m_dStrings = m_dStrings.Length () ? &m_dStrings [0] : NULL;
	m_PluginStartupInfo.m_nStrings = m_dStrings.Length ();
	m_PluginStartupInfo.m_szRoot   = m_sPath.c_str ();
	m_PluginStartupInfo.m_szRootFSShortcut = m_sRootFSShortcut.c_str ();

	if ( m_fnSetStartupInfo )
		m_fnSetStartupInfo ( m_PluginStartupInfo );

	memset ( &m_PluginInfo, 0, sizeof (m_PluginInfo) );
	m_PluginInfo.m_uFlags			= OPIF_REALNAMES;
	m_PluginInfo.m_eStartSortMode	= SORT_UNDEFINDED;
	m_PluginInfo.m_eStartViewMode	= VM_UNDEFINED;
	m_PluginInfo.m_nFileInfoCols	= -1;

	if ( m_fnGetPluginInfo )
		m_fnGetPluginInfo ( m_PluginInfo );
}

//////////////////////////////////////////////////////////////////////////
// helpers
static void ToSelectedList ( SelectedFileList_t & tList, const wchar_t * szRoot, PanelItem_t ** ppItems, int nItems )
{
	tList.m_sRootDir = szRoot;
	tList.m_uSize.QuadPart = 0;
	for ( int i = 0; i < nItems; i++ )
		tList.m_dFiles.Add ( ppItems [i] );
}

static void PluginCalcSize ( const wchar_t * szRoot, PanelItem_t ** ppItems, int nItems, ULARGE_INTEGER & uSize, int & nFiles, int & nFolders, DWORD & dwIncAttrib, DWORD & dwExAttrib, SlowOperationCallback_t fnCallback )
{
	SelectedFileList_t tList;
	ToSelectedList ( tList, szRoot, ppItems, nItems );
	CalcTotalSize ( tList, uSize, nFiles, nFolders, dwIncAttrib, dwExAttrib, fnCallback );
}

static void PluginCfgInt ( HANDLE hCfg, wchar_t * szName, int * pField )
{
	if ( hCfg == INVALID_HANDLE_VALUE )
		return;

	((VarLoader_c *) hCfg)->RegisterVar ( szName, pField );
}

static void PluginCfgDword ( HANDLE hCfg, wchar_t * szName, DWORD * pField )
{
	if ( hCfg == INVALID_HANDLE_VALUE )
		return;

	((VarLoader_c *) hCfg)->RegisterVar ( szName, pField );
}

static void PluginCfgFloat ( HANDLE hCfg, wchar_t * szName, float * pField )
{
	if ( hCfg == INVALID_HANDLE_VALUE )
		return;

	((VarLoader_c *) hCfg)->RegisterVar ( szName, pField );
}

static void PluginCfgStr ( HANDLE hCfg, wchar_t * szName, wchar_t * pField, int iMaxLength, LOADCSTR fnLoad, SAVECSTR fnSave )
{
	if ( hCfg == INVALID_HANDLE_VALUE )
		return;

	((VarLoader_c *) hCfg)->RegisterVar ( szName, pField, iMaxLength, fnLoad, fnSave );
}

static HANDLE PluginCfgCreate ()
{
	return (HANDLE) new VarLoader_c;
}

static void PluginCfgDestroy ( HANDLE hCfg )
{
	if ( hCfg && hCfg != INVALID_HANDLE_VALUE )
		delete (VarLoader_c *) hCfg;
}

static bool PluginCfgLoad ( HANDLE hCfg, const wchar_t * szFilename )
{
	if ( hCfg == INVALID_HANDLE_VALUE )
		return false;

	return ((VarLoader_c *) hCfg)->LoadVars ( szFilename );
}

static void PluginCfgSave ( HANDLE hCfg )
{
	if ( hCfg == INVALID_HANDLE_VALUE )
		return;

	((VarLoader_c *) hCfg)->SaveVars ();
}

// filters
static HANDLE PluginFltCreate ( const wchar_t * szText, bool bCaseSensitive )
{
	Filter_c * pFilter = new Filter_c ( bCaseSensitive );
	pFilter->Set ( szText );
	return (HANDLE)pFilter;
}


static void PluginFltDestroy ( HANDLE hFilter )
{
	if ( hFilter && hFilter != INVALID_HANDLE_VALUE )
		delete (Filter_c *) hFilter;

}


static bool PluginFltFits ( HANDLE hFilter, const wchar_t * szText )
{
	if ( !hFilter || hFilter == INVALID_HANDLE_VALUE )
		return false;

	return ((Filter_c *)hFilter)->Fits ( szText );
}

// iterators
static HANDLE PluginIterCreate ( IteratorType_e eType )
{
	switch ( eType )
	{
	case ITERATOR_TREE:			return HANDLE((FileIterator_c *)(new FileIteratorTree_c));
	case ITERATOR_PANEL:		return HANDLE((FileIterator_c *)(new FileIteratorPanel_c));
	case ITERATOR_PANEL_CACHED:	return HANDLE((FileIterator_c *)(new FileIteratorPanelCached_c));
	}

	return INVALID_HANDLE_VALUE;
}

static void	PluginIterDestroy ( HANDLE hIterator )
{
	if ( hIterator == NULL || hIterator == INVALID_HANDLE_VALUE )
		return;

	delete ((FileIterator_c *)hIterator);
}

static void	PluginIterStart ( HANDLE hIterator, const wchar_t * szRoot, PanelItem_t ** ppItems, int nItems, bool bRecursive )
{
	if ( hIterator == NULL || hIterator == INVALID_HANDLE_VALUE )
		return;

	SelectedFileList_t tList;
	ToSelectedList ( tList, szRoot, ppItems, nItems );

	FileIterator_c * pIterator = (FileIterator_c*)hIterator;
	pIterator->IterateStart ( tList, bRecursive );
}

static bool PluginIterNext ( HANDLE hIterator )
{
	if ( hIterator == NULL || hIterator == INVALID_HANDLE_VALUE )
		return false;

	return ((FileIterator_c *)hIterator)->IterateNext ();
}

static IteratorInfo_t g_IteratorInfo;

static const IteratorInfo_t * PluginIterInfo ( HANDLE hIterator )
{
	if ( hIterator == NULL || hIterator == INVALID_HANDLE_VALUE )
		return NULL;

	FileIterator_c * pIterator = (FileIterator_c*)hIterator;
	g_IteratorInfo.m_b2ndPassDir	= pIterator->Is2ndPassDir ();
	g_IteratorInfo.m_bRootDir		= pIterator->IsRootDir ();
	g_IteratorInfo.m_pFindData		= pIterator->GetData ();
	wcsncpy ( g_IteratorInfo.m_szFullPath, pIterator->GetFullPath (), MAX_PATH );

	return &g_IteratorInfo;
}

// panels
static void PluginPanelSetDir ( HANDLE hPlugin, const wchar_t * szDir )
{
	if ( !hPlugin || hPlugin == INVALID_HANDLE_VALUE )
		return;

	if ( FMGetPanel1 ()->GetActivePlugin () == hPlugin )
		FMGetPanel1 ()->SetDirectory ( szDir );

	if ( FMGetPanel2 ()->GetActivePlugin () == hPlugin )
		FMGetPanel2 ()->SetDirectory ( szDir );
}

static void PluginFSPanelRefresh ( HANDLE hPlugin )
{
	if ( !hPlugin || hPlugin == INVALID_HANDLE_VALUE )
		return;

	if ( FMGetPanel1 ()->GetActivePlugin () == hPlugin )
		FMGetPanel1 ()->Refresh ();

	if ( FMGetPanel2 ()->GetActivePlugin () == hPlugin )
		FMGetPanel2 ()->Refresh ();
}

static void PluginPanelSoftReshreshAll ()
{
	FMGetPanel1 ()->SoftRefresh ();
	FMGetPanel2 ()->SoftRefresh ();
}

static void PluginPanelSetCursor ( HANDLE hPlugin, const wchar_t * szFile )
{
	if ( !hPlugin || hPlugin == INVALID_HANDLE_VALUE )
		return;

	if ( FMGetPanel1 ()->GetActivePlugin () == hPlugin )
		FMGetPanel1 ()->SetCursorAtItem ( szFile );

	if ( FMGetPanel2 ()->GetActivePlugin () == hPlugin )
		FMGetPanel2 ()->SetCursorAtItem ( szFile );
}

static void PluginPanelMarkFile ( const wchar_t * szFilename, bool bMark )
{
	FMGetActivePanel()->MarkFileByName ( szFilename, bMark );
}


//////////////////////////////////////////////////////////////////////////
// plugin manager
PluginManager_c	g_PluginManager;

PluginManager_c::PluginManager_c ()
	: m_pLoader	( NULL )
{
}


void PluginManager_c::Init ()
{
	m_pLoader = new VarLoader_c;

	// find plugins
	Str_c sPluginRoot = GetExecutablePath () + L"Plugins\\";
	FileIteratorTree_c tIterator;
	tIterator.IterateStart ( sPluginRoot, false );
	while ( tIterator.IterateNext () )
	{
		const WIN32_FIND_DATA * pData = tIterator.GetData ();
		if ( pData && ( pData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && ! tIterator.Is2ndPassDir () )
		{
			Str_c sFullPluginPath = tIterator.GetFullName ();
			Plugin_c * pPlugin = CreatePlugin ( AppendSlash ( sFullPluginPath ) );
			if ( pPlugin )
			{
				Log ( L"Found plugin: %s", pPlugin->m_sPath + pPlugin->m_sDll );
				m_dPlugins.Add ( pPlugin );
			}
		}
	}

	SafeDelete ( m_pLoader );

	PluginStartupInfo_t & SI = Plugin_c::m_PluginStartupInfo;
	SI.m_hMainWindow	= g_hMainWindow;
	SI.m_bVGA			= IsVGAScreen ();
	SI.m_hBoldFont		= g_tResources.m_hBoldSystemFont;
	SI.m_hSmallFont		= g_tResources.m_hSmallSystemFont;
	SI.m_fnLoadTGA		= ReadTGA;

	SI.m_fnDlgMoving			= MovingWindowProc;
	SI.m_fnDlgInitFullscreen	= InitFullscreenDlg;
	SI.m_fnDlgCloseFullscreen	= CloseFullscreenDlg;
	SI.m_fnDlgHandleActivate	= HandleActivate;
	SI.m_fnDlgHandleSizeChange	= HandleSizeChange;
	SI.m_fnDlgDoResize			= DoResize;

	SI.m_fnAlignText	= AlignFileName;
	SI.m_fnSizeToStr	= FileSizeToStringUL;
	SI.m_fnTimeSec		= TimerGetTimeSec;
	SI.m_fnCalcSize		= PluginCalcSize;
	SI.m_fnSplitPath	= SplitPathW;

	SI.m_fnDialogError		= ShowErrorDialog;
	SI.m_fnCreateToolbar	= CreateToolbar;
	SI.m_fnCreateWaitWnd	= PrepareCallback;
	SI.m_fnDestroyWaitWnd	= DestroyPrepareDialog;

	SI.m_fnCfgInt		= PluginCfgInt;
	SI.m_fnCfgDword		= PluginCfgDword;
	SI.m_fnCfgFloat		= PluginCfgFloat;
	SI.m_fnCfgStr		= PluginCfgStr;
	SI.m_fnCfgCreate	= PluginCfgCreate;
	SI.m_fnCfgDestroy	= PluginCfgDestroy;
	SI.m_fnCfgLoad		= PluginCfgLoad;
	SI.m_fnCfgSave		= PluginCfgSave;

	SI.m_fnFilterCreate		= PluginFltCreate;
	SI.m_fnFilterDestroy	= PluginFltDestroy;
	SI.m_fnFilterFits		= PluginFltFits;

	SI.m_fnIteratorCreate		= PluginIterCreate;
	SI.m_fnIteratorDestroy		= PluginIterDestroy;
	SI.m_fnIteratorStart		= PluginIterStart;
	SI.m_fnIteratorNext			= PluginIterNext;
	SI.m_fnIteratorInfo			= PluginIterInfo;

	SI.m_fnPanelSetDir			= PluginPanelSetDir;
	SI.m_fnFSPanelRefresh		= PluginFSPanelRefresh;
	SI.m_fnPanelSoftRefreshAll	= PluginPanelSoftReshreshAll;
	SI.m_fnPanelSetCursor		= PluginPanelSetCursor;
	SI.m_fnPanelMarkFile		= PluginPanelMarkFile;

	SI.m_fnErrCreateOperation		= ErrCreateOperation;
	SI.m_fnErrDestroyOperation		= ErrDestroyOperation;
	SI.m_fnErrAddWinErrorHandler	= ErrAddWinErrorHandler;
	SI.m_fnErrAddCustomErrorHandler = ErrAddCustomErrorHandler;
	SI.m_fnErrSetOperation			= ErrSetOperation;
	SI.m_fnErrShowErrorDlg			= ErrShowErrorDlg;
	SI.m_fnErrIsInDialog			= ErrIsInDialog;
}


void PluginManager_c::Shutdown ()
{
	while ( !m_dInstances.Empty () )
		ClosePlugin ( m_dInstances.Last ().m_hInstance );

	for ( int i = 0; i < m_dPlugins.Length (); ++i )
		SafeDelete ( m_dPlugins [i] );
}


HANDLE PluginManager_c::OpenFilePlugin ( const wchar_t * szPath )
{
	for ( int i = 0; i < m_dPlugins.Length (); ++i )
	{
		Plugin_c * pPlugin = m_dPlugins [i];
		HANDLE hInstance = pPlugin->OpenFilePlugin ( szPath );
		if ( hInstance != INVALID_HANDLE_VALUE )
		{
			PluginInstance_t PInstance;
			PInstance.m_pOwner = pPlugin;
			PInstance.m_hInstance = hInstance;
			m_dInstances.Add ( PInstance );
			return hInstance;
		}
	}

	return INVALID_HANDLE_VALUE;
}


void PluginManager_c::ClosePlugin ( HANDLE hInstance )
{
	for ( int i = 0; i < m_dInstances.Length (); ++i )
		if ( m_dInstances [i].m_hInstance == hInstance )
		{
			m_dInstances [i].m_pOwner->ClosePlugin ( hInstance );
			m_dInstances.Delete ( i );
			break;
		}
}


bool PluginManager_c::GetFindData ( HANDLE hPlugin, PanelItem_t * & dItems, int & nItems )
{
	Plugin_c * pPlugin = FindPlugin ( hPlugin );
	return pPlugin ? pPlugin->GetFindData ( hPlugin, dItems, nItems ) : FALSE;
}


void PluginManager_c::FreeFindData ( HANDLE hPlugin, PanelItem_t * dItems, int nItems)
{
	if ( !dItems )
		return;

	Plugin_c * pPlugin = FindPlugin ( hPlugin );
	if ( pPlugin )
		pPlugin->FreeFindData ( hPlugin, dItems, nItems );
}


bool PluginManager_c::SetDirectory ( HANDLE hPlugin, const wchar_t * szDir )
{
	Plugin_c * pPlugin = FindPlugin ( hPlugin );
	return pPlugin ? pPlugin->SetDirectory ( hPlugin, szDir ) : FALSE;
}


ItemCompare_t PluginManager_c::GetCompareFunc ( HANDLE hPlugin )
{
	Plugin_c * pPlugin = FindPlugin ( hPlugin );
	return pPlugin ? pPlugin->GetCompareFunc () : NULL;
}


const PluginInfo_t * PluginManager_c::GetPluginInfo ( HANDLE hPlugin ) const
{
	Plugin_c * pPlugin = FindPlugin ( hPlugin );
	return pPlugin ? pPlugin->GetPluginInfo () : NULL;
}


bool PluginManager_c::DeleteFiles ( HANDLE hPlugin, PanelItem_t ** dItems, int nItems )
{
	Plugin_c * pPlugin = FindPlugin ( hPlugin );
	return pPlugin ? pPlugin->DeleteFiles ( hPlugin, dItems, nItems ) : false;
}


bool PluginManager_c::HandlesDelete ( HANDLE hPlugin ) const
{
	Plugin_c * pPlugin = FindPlugin ( hPlugin );
	if ( !pPlugin )
		return false;

	return !!pPlugin->m_fnDeleteFiles;
}


bool PluginManager_c::FindFiles ( HANDLE hPlugin, PanelItem_t ** dItems, int nItems )
{
	Plugin_c * pPlugin = FindPlugin ( hPlugin );
	return pPlugin ? pPlugin->FindFiles ( hPlugin, dItems, nItems ) : false;
}


bool PluginManager_c::HandlesFind ( HANDLE hPlugin ) const
{
	Plugin_c * pPlugin = FindPlugin ( hPlugin );
	if ( !pPlugin )
		return false;

	return !!pPlugin->m_fnFindFiles;
}


bool PluginManager_c::HandlesRealNames ( HANDLE hPlugin ) const
{
	const PluginInfo_t * pInfo = GetPluginInfo ( hPlugin );
	return pInfo ? !!(pInfo->m_uFlags & OPIF_REALNAMES) : true;
}


int PluginManager_c::GetNumPlugins () const
{
	return m_dPlugins.Length ();
}


Plugin_c * PluginManager_c::GetPlugin ( int iPlugin ) const
{
	return m_dPlugins [iPlugin];
}


static Array_T <Str_c> g_dPluginSubmenus;
static void LoadMenuItem ( const Str_c & sName )
{
	g_dPluginSubmenus.Add ( sName );
}


Plugin_c * PluginManager_c::CreatePlugin ( const Str_c & sPath )
{
	Assert ( m_pLoader );

	Str_c sIniFile = sPath + L"plugin.ini";

	if ( GetFileAttributes ( sIniFile ) == 0xFFFFFFFF )
		return false;

	Str_c sDll, sRootFSShortcut, sRootFSName, sSubmenu;
	DWORD uCapabilities = 0;

	m_pLoader->RegisterVar ( L"dll",			  &sDll );
	m_pLoader->RegisterVar ( L"capabilities",	  &uCapabilities );

	// root fs specific
	m_pLoader->RegisterVar ( L"root_fs_shortcut", &sRootFSShortcut );
	m_pLoader->RegisterVar ( L"root_fs_name",	  &sRootFSName );

	// file menu specific
	m_pLoader->RegisterVar ( L"file_menu_submenu",&sSubmenu );
	m_pLoader->RegisterVar ( L"file_menu_item",	NULL, LoadMenuItem, NULL );

	g_dPluginSubmenus.SetLength ( 0 );

	bool bLoadOk = m_pLoader->LoadVars ( sIniFile );
	m_pLoader->ResetRegistered ();
	
	if ( !bLoadOk || sDll.Empty () )
		return NULL;

	if ( GetFileAttributes ( sPath + sDll ) == 0xFFFFFFFF )
		return NULL;

	if ( !uCapabilities )
		return NULL;
	
	sRootFSShortcut.ToLower ();

	Plugin_c * pPlugin = new Plugin_c ( sPath, sDll, uCapabilities );
	pPlugin->m_sRootFSShortcut	= sRootFSShortcut;
	pPlugin->m_sRootFSName		= sRootFSName;
	pPlugin->m_sSubmenu			= sSubmenu;
	pPlugin->m_dSubmenu			= g_dPluginSubmenus;

	g_dPluginSubmenus.Reset ();

	return pPlugin;
}


Plugin_c * PluginManager_c::FindPlugin ( HANDLE hPlugin ) const
{
	for ( int i = 0; i < m_dInstances.Length (); ++i )
		if ( m_dInstances [i].m_hInstance == hPlugin )
			return m_dInstances [i].m_pOwner;

	return NULL;
}