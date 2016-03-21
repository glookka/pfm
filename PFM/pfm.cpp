#include "pch.h"
#include "pfm.h"

#include "parser.h"
#include "config.h"
#include "resources.h"
#include "protection.h"
#include "gui.h"
#include "plugins.h"
#include "menu.h"
#include "filefuncs.h"
#include "dialogs/bookmarks.h"
#include "dialogs/shortcut.h"
#include "dialogs/errors.h"

#include "LFile/fclipboard.h"
#include "LComm/ccomm.h"
#include "LDialogs/doptions.h"

#include "Resource/resource.h"
#include "Dlls/Resource/resource.h"

#include "dbt.h"
#include "aygshell.h"

const wchar_t * g_szWndClassName = L"FileManagerClass";

// windows and controls
HINSTANCE 		g_hAppInstance = NULL;
HWND 			g_hMainWindow = NULL;
bool			g_bLastSIPOn = false;
bool			g_bActive = false;
int				g_iSplitterPos = 0;
bool			g_bResizeQueued = true;
bool			g_bEnableMsg = false;

// the absolute hack!!!
int				g_iWaitForUpdate = 0;
bool			g_bJustExecuted = true;
const int		SECONDS_BEFORE_UPDATE = 3;

// screen orientation stuff
bool g_bCanChangeOrientation = false;
int g_iOldOrientation = 0;

int g_iWndWidth = 0;
int g_iWndHeight = 0;

Panel_c * g_pPanel1 = NULL;
Panel_c * g_pPanel2 = NULL;

// file mark mode flag
static bool g_bMarkMode = false;

// pane splitter
class PaneSplitter_c : public SplitterBar_c
{
protected:
	void Event_SplitterMoved ()
	{
		FMSplitterMoved ();
	}
};

PaneSplitter_c g_PaneSplitter;

// pre-s
bool UpdateFullscreen ();
void CALLBACK OnTimer ( HWND hWnd, UINT uMsg, UINT uEvent, DWORD uTime );
void LoadPanelSettings ();

static bool OnCommand ( int iCommand )
{
	return menu::OnCommand ( iCommand );
}

static bool OnSize ( int iWidth, int iHeight )
{
	int iNewScrWidth, iNewScrHeight;
	GetScreenResolution ( iNewScrWidth, iNewScrHeight );
	if ( iNewScrWidth != g_tResources.m_iScreenWidth && iNewScrHeight != g_tResources.m_iScreenHeight )
		g_bResizeQueued = true;

	g_tResources.m_iScreenWidth = iNewScrWidth;
	g_tResources.m_iScreenHeight = iNewScrHeight;

	static bool bChecked = false;
	if ( !bChecked )
	{
		protection::registered	=  protection::CheckKey ();
		protection::expired		= !protection::CheckDate ();
		bChecked = true;
	}

	return true;
}


static void OnActivate ( HWND hOldWindow )
{
	if ( g_hMainWindow && ! IsInDialog () )
	{
		SetTimer ( g_hMainWindow, 666, (int)FM_TIMER_PERIOD_MS, OnTimer );

		if ( cfg->fullscreen && g_bJustExecuted )
		{
			g_iWaitForUpdate = SECONDS_BEFORE_UPDATE;
			g_bJustExecuted = false;
		}

		SipShowIM ( g_bLastSIPOn ? SIPF_ON : SIPF_OFF );

		if ( !hOldWindow )
		{
			FMGetPanel1 ()->QueueRefresh ();
			FMGetPanel2 ()->QueueRefresh ();
		}

		// restore z-order
		UpdateFullscreen ();

		FMRepositionPanels ( g_bResizeQueued );
		g_bResizeQueued = false;

		buttons->RegisterHotKeys ( g_hMainWindow );

		if ( cfg->all_keys )
			AllKeys ( TRUE );
	}

	if ( IsInDialog () )
		ActivateFullscreenDlg ();

	btns::Reset ();

	g_bActive = true;
}

static void OnDeactivate ()
{
	if ( g_hMainWindow )
	{
		buttons->UnregisterHotKeys ( g_hMainWindow );

		if ( cfg->all_keys )
			AllKeys ( FALSE );

		KillTimer ( g_hMainWindow, 666 );
	}

	SIPINFO tSipInfo;
	memset ( &tSipInfo, 0, sizeof (tSipInfo) );
	tSipInfo.cbSize = sizeof (tSipInfo);

	if ( SipGetInfo ( &tSipInfo ) )
		g_bLastSIPOn = !!( tSipInfo.fdwFlags & SIPF_ON );

	btns::Reset ();

	g_bActive = false;
}

static bool OnWndPosChanged ( WINDOWPOS * pWndPos )
{
	return false;
}

static bool OnSetFocus ( HWND hWnd )
{
	return false;
}

static void OnNotify ( int iControlId, NMHDR * pInfo )
{
	menu::OnNotify ( iControlId, pInfo );
}

static void OnSettingsChange ( int iFlag )
{
	if ( ! g_hMainWindow || ! g_bActive || IsInDialog () )
		return;

	if ( iFlag == SPI_SETSIPINFO )
	{
		UpdateFullscreen ();
		FMRepositionPanels ( true );
	}
}

static bool OnKeyEvent ( int iKey, btns::Event_e eEvent )
{
	if ( FMGetActivePanel ()->OnKeyEvent ( iKey, eEvent ) )
		return true;

	BtnAction_e eAction = buttons->GetAction ( iKey, eEvent );

	switch ( eAction )
	{
	case BA_OPENWITH:
		FMOpenWith ();
		return true;
	case BA_TGL_2NDBAR:
		menu::ShowSecondToolbar ( ! cfg->secondbar );
		return true;
	case BA_TGL_FULLSCREEN:
		FMToggleFullscreen ();
		return true;
	case BA_SWITCH_PANES:
		if ( FMGetActivePanel ()->IsMaximized () )
			FMMaximizePanel ( FMGetPassivePanel () );
		else
			FMActivatePanel ( FMGetPassivePanel () );
		return true;
	case BA_MAXIMIZE_PANE:
		if ( FMGetActivePanel ()->IsMaximized () )
			FMNormalPanels ();
		else
			FMMaximizePanel ( FMGetActivePanel () );
		return true;
	case BA_REFRESH_PANES:
		FMGetPanel1 ()->SoftRefresh ();
		FMGetPanel2 ()->SoftRefresh ();
		return true;
	case BA_EXIT:
		FMExit ();
		return true;
	case BA_MINIMIZE:
		ShowWindow( g_hMainWindow, SW_MINIMIZE );
		break;
	case BA_TGL_FASTSEL:
		FMSetMarkMode ( ! FMMarkMode () );
		return true;
	case BA_TGL_FASTNAV:
		FMSetFastNavMode ( !cfg->fastnav );
		return true;
	case BA_OP_COPYMOVE:
		FMCopyMoveFiles ();
		return true;
	case BA_OP_RENAME:
		FMRenameFiles ();
		return true;
	case BA_OP_DELETE:
		FMDeleteFiles ();
		return true;
	case BA_OP_MKDIR:
		FMMkDir ();
		return true;
	case BA_OP_SEARCH:
		FMFindFiles ();
		return true;
	case BA_OP_VIEW:
		FMViewFile ();
		return true;
	case BA_OP_SHORTCUT:
// FIXME
//		FMCreateShortcut ( FMGetActivePanel ()->GetDirectory (), true );
		return true;
	case BA_OP_PROPS:
		FMFileProperties ();
		return true;
	case BA_FL_SELECT_ALL:
		FMGetActivePanel ()->MarkAllFiles ( true );
		return true;
	case BA_FL_SELECT_NONE:
		FMGetActivePanel ()->MarkAllFiles ( false, true );
		return true;
	case BA_FL_INVERT:
		FMGetActivePanel ()->InvertMarkedFiles ();
		return true;
	case BA_FL_SELECT_FILTER:
		FMSelectFilter ( true );
		return true;
	case BA_FL_CLEAR_FILTER:
		FMSelectFilter ( false );
		return true;
	case BA_SEND_BT:
		FMSendBT ();
		return true;
	case BA_SEND_IR:
		FMSendIR ();
		return true;
	case BA_OPTIONS:
		ShowOptionsDialog ();
		return true;
	case BA_OP_CLIPCOPY:
		clipboard::Copy ( FMGetActivePanel () );
		break;
	case BA_OP_CLIPCUT:
		clipboard::Cut ( FMGetActivePanel () );
		break;
	case BA_OP_CLIPPASTE:
// FIXME
//		clipboard::Paste ( FMGetActivePanel ()->GetDirectory () );
		break;
	}

	return false;
}

static bool OnKeyUp ( int iKey )
{
	return OnKeyEvent ( iKey, btns::Event_Keyup ( iKey ) );
}

static bool OnKeyDown ( int iKey )
{
	return OnKeyEvent ( iKey, btns::Event_Keydown ( iKey ) );
}

static bool OnHotKey ( int iKey, DWORD uParams, DWORD uVirtKey )
{
	return OnKeyEvent ( iKey, btns::Event_Hotkey ( iKey, uParams, uVirtKey ) );
}

static void OnDeviceChange ( int iParam )
{
	if ( iParam == DBT_DEVICEARRIVAL || iParam == DBT_DEVICEREMOVECOMPLETE )
	{
		FMGetPanel1 ()->QueueRefresh ( true );
		FMGetPanel2 ()->QueueRefresh ( true );
		RefreshStorageCards ();
	}
}


void CALLBACK OnTimer ( HWND hWnd, UINT uMsg, UINT uEvent, DWORD uTime )
{
	Assert ( g_pPanel1 && g_pPanel2 );

	g_pPanel1->OnTimer ();
	g_pPanel2->OnTimer ();

	int iKey = -1;
	btns::Event_e eEvent = btns::Event_Timer ( iKey );
	if ( eEvent != btns::EVENT_NONE )
		OnKeyEvent ( iKey, eEvent );

	// the absolute hack!
	if ( g_iWaitForUpdate > 0 )
	{
		--g_iWaitForUpdate;
		if ( ! g_iWaitForUpdate )
			UpdateFullscreen ();
	}
}

void OnFileChange ( FILECHANGEINFO & tChange )
{
	FMGetActivePanel ()->OnFileChange ();
}

bool OnInitMenuPopup ( HMENU hMenu )
{
	return menu::OnInitMenuPopup ( hMenu );
}


///////////////////////////////////////////////////////////////////////////////////////////
// main window proc
LRESULT CALLBACK MainWndProc ( HWND hWnd, UINT Msg, UINT wParam, LONG lParam )
{
	if ( !g_bEnableMsg )
		return DefWindowProc ( hWnd, Msg, wParam, lParam );

    switch ( Msg )
    {
	case WM_FILECHANGEINFO:
		OnFileChange (  ((FILECHANGENOTIFY *) lParam)->fci );
		break;
	
	case WM_INITMENUPOPUP:
		if ( OnInitMenuPopup ( (HMENU) wParam ) )
			return 0;
		break;

	case WM_ENTERMENULOOP:
		menu::OnEnterMenu ();
		break;

	case WM_EXITMENULOOP:
		menu::OnExitMenu ();
		break;

	case WM_ERASEBKGND:
		return TRUE;

	case WM_COMMAND:
		if ( OnCommand ( (int) ( LOWORD ( wParam ) ) ) )
			return 0;
		break;

	case WM_NOTIFY:
		OnNotify ( (int) wParam, (NMHDR *) lParam );
		return 0;

	case WM_SETTINGCHANGE:
		OnSettingsChange ( (int) wParam );
		return 0;

	case WM_KEYDOWN:
		if ( OnKeyDown ( (int) wParam ) )
			return 0;
		break;

	case WM_KEYUP:
		if ( OnKeyUp ( (int) wParam ) )
			return 0;
		break;

	case WM_HOTKEY:
		OnHotKey ( (int) wParam, HIWORD(lParam), LOWORD(lParam) );
		break;

	case WM_ACTIVATE:
		if ( LOWORD ( wParam ) == WA_INACTIVE )
			OnDeactivate ();
		else
			OnActivate ( (HWND) lParam );
		break;

	case WM_SETFOCUS:
		if ( OnSetFocus ( (HWND) wParam ) )
			return 0;
		break;

	case WM_WINDOWPOSCHANGED:
		if ( OnWndPosChanged ( (WINDOWPOS *) lParam ) )
			return 0;
		break;

	case WM_SIZE:
		if ( OnSize ( LOWORD(lParam), HIWORD(lParam) ) )
			return 0;
		break;

	case WM_DEVICECHANGE:
		OnDeviceChange ( int ( wParam ) );
		return TRUE;

	case WM_HELP:
		Help ( L"Main_Contents" );
		return TRUE;

	case WM_CLOSE:
        PostQuitMessage (0);
        return 0;
	}

    return DefWindowProc ( hWnd, Msg, wParam, lParam );
}

static void UpdateAutorefresh ()
{
	Str_c sDirToWatch = FMGetActivePanel ()->GetDirectory ();
	RemoveSlash ( sDirToWatch );

	SHCHANGENOTIFYENTRY tEntry;
	tEntry.dwEventMask = SHCNE_RENAMEITEM | SHCNE_CREATE | SHCNE_DELETE | SHCNE_MKDIR | SHCNE_RMDIR | SHCNE_MEDIAINSERTED | SHCNE_MEDIAREMOVED | SHCNE_RENAMEFOLDER;
	tEntry.fRecursive = FALSE;
	tEntry.pszWatchDir = (wchar_t *)sDirToWatch.c_str ();
	SHChangeNotifyRegister ( g_hMainWindow, &tEntry );
}

Panel_c * FMGetActivePanel ()
{
	return g_pPanel1->IsActive () ? g_pPanel1 : g_pPanel2;
}

Panel_c * FMGetPassivePanel ()
{
	return g_pPanel1->IsActive () ? g_pPanel2 : g_pPanel1;
}

Str_c FMGetVersion ()
{
	Str_c sVersion = FM_VERSION;
	wchar_t szDate [64];
	AnsiToUnicode ( __DATE__, szDate );

	sVersion += Str_c ( L" (" ) + szDate + L")";

	return Str_c ( Txt ( T_VERSION ) ) + L" " + sVersion;
}

void FMFolderChange_Event ()
{
	UpdateAutorefresh ();
	menu::UpdateToolbar ();
}

void FMSelectionChange_Event ()
{
	menu::UpdateToolbar ();
}

void FMCursorChange_Event ()
{
//	menu::UpdateToolbar ();
}

void FMExecute_Event ()
{
	if ( cfg->fullscreen )
		g_bJustExecuted = true;
}

void FMLoadColorScheme ( const Str_c & sScheme )
{
	Str_c sDir = GetExecutablePath () + L"schemes\\" + sScheme;

	clrs->LoadVars ( sDir + L"\\colors.ini" );
	LoadColorGroups ( sDir + L"\\groups.ini" );

	g_tResources.Shutdown ();
	g_tResources.Init ();

	g_PaneSplitter.SetColors ( clrs->splitter_border, clrs->splitter_fill, clrs->splitter_pressed );
	g_PaneSplitter.DestroyDrawObjects ();
	g_PaneSplitter.CreateDrawObjects ();
}

void FMUpdatePanelRects ()
{
	Assert ( g_pPanel1 && g_pPanel2 );
	
	if ( g_pPanel1->IsMaximized () || g_pPanel2->IsMaximized () )
	{
		RECT tRect;
		tRect.left		= 0;
		tRect.right		= g_tResources.m_iScreenWidth;
		tRect.top		= 0;
		tRect.bottom	= g_iWndHeight;

		if ( g_pPanel1->IsMaximized () )
			g_pPanel1->SetRect ( tRect );
		else
			g_pPanel2->SetRect ( tRect );

		g_PaneSplitter.Show ( false );
	}
	else
	{
		RECT tRect1, tRect2;

		if ( Panel_c::IsLayoutVertical () )
		{
			tRect1.left		= 0;
			tRect1.right	= g_iWndWidth;
			tRect1.top		= 0;
			tRect1.bottom	= g_iSplitterPos;

			tRect2.left		= 0;
			tRect2.right	= g_iWndWidth;
			tRect2.top		= g_iSplitterPos + cfg->splitter_thickness;
			tRect2.bottom	= g_iWndHeight;
		}
		else
		{
			tRect1.left		= 0;
			tRect1.right	= g_iSplitterPos;
			tRect1.top		= 0;
			tRect1.bottom	= g_iWndHeight;

			tRect2.left		= g_iSplitterPos + cfg->splitter_thickness;
			tRect2.right	= g_iWndWidth;
			tRect2.top		= 0;
			tRect2.bottom	= g_iWndHeight;
		}

		g_pPanel1->SetRect ( tRect1 );
		g_pPanel2->SetRect ( tRect2 );

		g_PaneSplitter.Show ( true );
	}
}


void FMToggleFullscreen ()
{
	cfg->fullscreen = !cfg->fullscreen;
	UpdateFullscreen ();
	FMRepositionPanels ( true );
}


bool FMIsAnyPaneInMenu ()
{
	return FMGetPanel1 ()->IsInMenu () || FMGetPanel2 ()->IsInMenu ();
}


void FMSetFullscreen ( bool bFullscreen )
{
	cfg->fullscreen = bFullscreen;
	UpdateFullscreen ();
}

bool FMCalcWindowRect ( RECT & tFinalRect, bool bDialog, bool bTabbed )
{
	const bool bFullscreen = cfg->fullscreen;

	RECT tMenuRect, tTaskRect;
	menu::GetToolbarRect ( tMenuRect );
	HWND hTaskBar = FindWindow ( L"HHTaskBar", NULL );
	GetWindowRect ( hTaskBar, &tTaskRect );

	SIPINFO tSipInfo;
	memset ( &tSipInfo, 0, sizeof (tSipInfo) );
	tSipInfo.cbSize = sizeof (tSipInfo);

	int iScreenWidth, iScreenHeight;
	GetScreenResolution ( iScreenWidth, iScreenHeight );

	if ( ! SipGetInfo ( &tSipInfo ) )
		return false;

	int iBottom = 0;
	if ( tSipInfo.fdwFlags & SIPF_ON )
		iBottom = tSipInfo.rcVisibleDesktop.bottom;
	else
	{
		if ( menu::Is2ndBarVisible () && ! bDialog )
			iBottom = 2 * tMenuRect.top - tMenuRect.bottom + 1;
		else
		{
			// we can't just take toolbar top, 'cause there are situations when the main toolbar is hidden
			if ( bDialog )
				iBottom = iScreenHeight - ( tMenuRect.bottom - tMenuRect.top );
			else
				iBottom =  menu::IsToolbarHidden () ? iScreenHeight : tMenuRect.top;

			// ??? i can't find the tab control to get its height
			if ( bTabbed && bDialog )
				iBottom -= IsVGAScreen () ? 40 : 20;
		}
	}

	int iHeight = 0;
	if ( bFullscreen && ! bDialog )
		iHeight = iBottom;
	else
		iHeight = iBottom - ( tTaskRect.bottom - tTaskRect.top );

	tFinalRect.left = 0;
	tFinalRect.top = ( bFullscreen && ! bDialog ) ? 0 : tTaskRect.bottom;
	tFinalRect.right = iScreenWidth;
	tFinalRect.bottom = tFinalRect.top + iHeight;

	return true;
}


bool UpdateFullscreen ()
{
	if ( ! g_hMainWindow )
		return false;

	RECT tWndRect;
	Verify ( FMCalcWindowRect ( tWndRect, false ) );

	g_iWndWidth = tWndRect.right - tWndRect.left;
	g_iWndHeight = tWndRect.bottom - tWndRect.top;

	if ( ! SHFullScreen ( g_hMainWindow, cfg->fullscreen ? SHFS_HIDETASKBAR : SHFS_SHOWTASKBAR ) )
		return false;

	if ( ! SetWindowPos ( g_hMainWindow, HWND_TOP, tWndRect.left, tWndRect.top, g_iWndWidth, g_iWndHeight, SWP_SHOWWINDOW ) )
		return false;

	menu::ShowSecondToolbar ( cfg->secondbar );
	menu::HideWM5Toolbar ();

	Sleep ( 0 );

	Assert ( g_pPanel1 && g_pPanel2 );

	// fixup panel height
	bool bVertical = Panel_c::IsLayoutVertical();
	int iMin = bVertical ? g_pPanel1->GetMinHeight () : g_pPanel1->GetMinWidth ();
	int iMax = bVertical ? g_iWndHeight - g_pPanel2->GetMinHeight () : g_iWndWidth - g_pPanel2->GetMinWidth ();

	g_PaneSplitter.SetMinMax ( iMin, iMax );
	return true;
}


void SetInitialConfigValues ()
{
	if ( cfg->splitter_thickness == -1 )
		cfg->splitter_thickness = IsVGAScreen () ? 8 : 4;

	if ( cfg->pane_slider_size_x == -1 )
		cfg->pane_slider_size_x = IsVGAScreen () ? 24 : 12;

	if ( cfg->font_height == -1 )
		cfg->font_height = IsVGAScreen () ? 28 : 14;
}


void SetInitialButtonValues ()
{
	buttons->SetButton ( BA_MOVEUP,		0, 38,	false );
	buttons->SetButton ( BA_MOVEDOWN,		0, 40,	false );
	buttons->SetButton ( BA_MOVELEFT,		0, 37,	false );
	buttons->SetButton ( BA_MOVERIGHT,	0, 39,	false );
	buttons->SetButton ( BA_EXECUTE,		0, 134, false );
	buttons->SetButton ( BA_EXECUTE,		1, 13,	false );
	buttons->SetButton ( BA_SWITCH_PANES,	0, 9,	false );
	buttons->SetButton ( BA_FILE_MENU,	0, 134,	true );
}

// calculate panels' positions and store'em
void FMRepositionPanels ( bool bInitial/* = false */)
{
	bool bVerticalLayout = Panel_c::IsLayoutVertical ();
	
	// calculations are in local window coords
	int iMiddle;

	if ( bInitial )
		iMiddle = bVerticalLayout ? ( g_iWndHeight - cfg->splitter_thickness ) / 2 : ( g_iWndWidth - cfg->splitter_thickness ) / 2;
	else
	{
		iMiddle = g_iSplitterPos;

		// just some checks
		if ( iMiddle < 0 )
			iMiddle = 0;
		else
		{
			int iSizeToCheck = bVerticalLayout ? g_iWndHeight : g_iWndWidth;

			if ( iMiddle > iSizeToCheck - cfg->splitter_thickness )
				iMiddle = iSizeToCheck - cfg->splitter_thickness;
		}
	}

	g_iSplitterPos = iMiddle;

	RECT tRect;

	if ( bVerticalLayout )
	{
		tRect.left = 0;
		tRect.top = iMiddle;
		tRect.right = g_tResources.m_iScreenWidth;
		tRect.bottom = iMiddle + cfg->splitter_thickness;
	}
	else
	{
		tRect.left = iMiddle;
		tRect.top = 0;
		tRect.right = iMiddle + cfg->splitter_thickness;
		tRect.bottom = g_tResources.m_iScreenHeight;
	}
	
	g_PaneSplitter.SetRectAndFixup ( tRect );
	FMUpdatePanelRects ();
}


void FMSplitterMoved ()
{
	RECT tRect;

	g_PaneSplitter.GetRect ( tRect );
	if ( Panel_c::IsLayoutVertical () )
		g_iSplitterPos = tRect.top;
	else
		g_iSplitterPos = tRect.left;

	FMUpdatePanelRects ();
	UpdateWindow ( g_hMainWindow );
}


void FMActivatePanel ( Panel_c * pPanel )
{
	Assert ( pPanel && g_pPanel1 && g_pPanel2 );
	if ( pPanel == FMGetActivePanel () )
		return;

	pPanel->Activate ( true );
	if ( pPanel == g_pPanel1 )
		g_pPanel2->Activate ( false );
	else
		g_pPanel1->Activate ( false );

	menu::UpdateToolbar ();

	UpdateAutorefresh ();
}


void FMMaximizePanel ( Panel_c * pPanel )
{
	Assert ( pPanel && g_pPanel1 && g_pPanel2 );

	pPanel->Show ( true );
	pPanel->Maximize ( true );
	pPanel->Activate ( true );

	Panel_c * pOtherPanel = pPanel == g_pPanel1 ? g_pPanel2 : g_pPanel1;

	pOtherPanel->Show ( false );
	pOtherPanel->Maximize ( false );
	pOtherPanel->Activate ( false );
	FMUpdatePanelRects ();

	menu::UpdateToolbar ();
}


void FMNormalPanels ()
{
	Assert ( g_pPanel1 && g_pPanel2 );
	g_pPanel1->Maximize ( false );
	g_pPanel2->Maximize ( false );
	g_pPanel1->Show ( true );
	g_pPanel2->Show ( true );
	FMRepositionPanels ( true );
}


void FMUpdateSplitterThickness ()
{
	RECT tRect;
	g_PaneSplitter.GetRect ( tRect );

	if ( Panel_c::IsLayoutVertical () )
		tRect.bottom = tRect.top + cfg->splitter_thickness;
	else
		tRect.right = tRect.left + cfg->splitter_thickness;

	g_PaneSplitter.SetRectAndFixup ( tRect );
	FMSplitterMoved ();
}


bool FMMarkMode ()
{
	return g_bMarkMode;
}


void FMSetMarkMode ( bool bMark )
{
	if ( bMark == g_bMarkMode )
		return;

	g_bMarkMode = bMark;
	FMGetPanel1 ()->InvalidateHeader ();
	FMGetPanel2 ()->InvalidateHeader ();
	menu::CheckMarkMode ( bMark );
}

void FMSetFastNavMode ( bool bFastNav )
{
	cfg->fastnav = bFastNav;
	menu::CheckFastNav ( bFastNav );
}

Panel_c * FMGetPanel1 ()
{
	return g_pPanel1;
}

Panel_c * FMGetPanel2 ()
{
	return g_pPanel2;
}

bool InitializeMainWindow ()
{
	WNDCLASS tWC;
    tWC.style			= 0;
    tWC.lpfnWndProc		= (WNDPROC) MainWndProc;
    tWC.cbClsExtra		= 0;
    tWC.cbWndExtra		= 0;
    tWC.hInstance		= g_hAppInstance;
    tWC.hIcon			= LoadIcon ( g_hAppInstance, MAKEINTRESOURCE ( IDI_MAIN_ICON ) );
    tWC.hCursor			= NULL;
    tWC.hbrBackground	= NULL;
    tWC.lpszMenuName	= NULL;
    tWC.lpszClassName	= L"FileManagerClass";
    
    BOOL bSuccess = RegisterClass ( &tWC );
    if ( ! bSuccess )
		return false;

	g_hMainWindow = CreateWindow ( g_szWndClassName, Txt ( T_WND_TITLE ), WS_VISIBLE,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			NULL, NULL, g_hAppInstance, NULL );

	if ( ! g_hMainWindow )
		return false;

	menu::Init ();
	menu::ShowSecondToolbar ( cfg->secondbar );

	// the panel divider
	RECT tRect;
	tRect.left = 0;
	tRect.top = 0;
	tRect.right = g_tResources.m_iScreenWidth;
	tRect.bottom = cfg->splitter_thickness;

	g_PaneSplitter.SetColors ( clrs->splitter_border, clrs->splitter_fill, clrs->splitter_pressed );
	g_PaneSplitter.Create ( g_hMainWindow );
	
	return true;
}


void ShutdownMainWindow ()
{
	KillTimer ( g_hMainWindow, 666 );
	SHChangeNotifyDeregister ( g_hMainWindow );
	DestroyWindow ( g_hMainWindow );
	UnregisterClass ( g_szWndClassName, g_hAppInstance );
}


void SetupInitialDlgData ()
{
	SetupInitialBookmarks ();
	SetupInitialShortcuts ();
	g_tRecent.AddFindMask ( L"*" );
}


void LoadPanelSettings ( const wchar_t * szCmdLine )
{
	// this must be a path
	bool bPathProvided = szCmdLine[0] != L'\0';
	if ( bPathProvided )
	{
		// always show this stuff at panel #1
		if ( cfg->pane_2_maximized )
			cfg->pane_2_maximized = false;

		cfg->pane_1_maximized = true;
	}

	g_pPanel1->SetDirectory ( bPathProvided ? szCmdLine : cfg->pane_1_path );
	g_pPanel1->SetSortMode ( SortMode_e ( cfg->pane_1_sort ), cfg->pane_1_sort_reverse );
	g_pPanel1->SetViewMode ( ViewMode_e ( cfg->pane_1_view ) );

	g_pPanel2->SetDirectory ( cfg->pane_2_path );
	g_pPanel2->SetSortMode ( SortMode_e ( cfg->pane_2_sort ), cfg->pane_2_sort_reverse );
	g_pPanel2->SetViewMode ( ViewMode_e ( cfg->pane_2_view ) );
}


void SavePanelSettings ()
{
	if ( g_pPanel1 )
	{
		cfg->pane_1_maximized	= g_pPanel1->IsMaximized ();
		cfg->pane_1_path			= g_pPanel1->GetDirectory ();
		cfg->pane_1_view			= g_pPanel1->GetViewMode ();
		cfg->pane_1_sort			= g_pPanel2->GetSortMode ();
		cfg->pane_1_sort_reverse	= g_pPanel2->GetSortReverse ();
	}

	if ( g_pPanel2 )
	{
		cfg->pane_2_maximized	= g_pPanel2->IsMaximized ();
		cfg->pane_2_path			= g_pPanel2->GetDirectory ();
		cfg->pane_2_view			= g_pPanel2->GetViewMode ();
		cfg->pane_2_sort			= g_pPanel2->GetSortMode ();
		cfg->pane_2_sort_reverse	= g_pPanel2->GetSortReverse ();
	}
}

//
// application init
//
bool AppInit ( const wchar_t * szCmdLine )
{
	INITCOMMONCONTROLSEX tCtrls;
	tCtrls.dwSize = sizeof ( INITCOMMONCONTROLSEX );
	tCtrls.dwICC = ICC_DATE_CLASSES | ICC_TREEVIEW_CLASSES;
	InitCommonControlsEx ( &tCtrls );

	SHInitExtraControls();

	Str_c sInitialPath ( GetExecutablePath () );

	// should be the first thing to do
	Init_Core ();

#if FM_USE_LOG
	g_Log.SetLogFilePath ( sInitialPath + L"message.log" );
#endif

	Log ( L" * Starting up..." );

	Init_Parser ();

	Config_c::Init ();
	Colors_c::Init ();
	Buttons_c::Init ();

	cfg->LoadVars ( sInitialPath + L"config.ini" );

	if ( ! Init_Lang ( sInitialPath, cfg->language + L".lang" ) )
	{
		MessageBox ( NULL, L"Error loading language data", L"Error", MB_OK | MB_ICONERROR );
		return false;
	}

	if ( ! Init_Resources ( sInitialPath + L"resource.dll" ) )
	{
		MessageBox ( NULL, L"Error loading resource.dll", L"Error", MB_OK | MB_ICONERROR );
		return false;
	}

	buttons->Load ( sInitialPath + L"buttons.ini" );

	RegisterColorGroupCommands ();

	// scheme or default
	if ( ! cfg->color_scheme.Empty () )
		FMLoadColorScheme ( cfg->color_scheme );
	else
		g_tResources.Init ();

	// initial display resolution
	GetScreenResolution ( g_tResources.m_iScreenWidth, g_tResources.m_iScreenHeight );

	Recent_c::Init ();
	g_tRecent.SetRecentFilename ( sInitialPath + L"recent.txt" );
	g_tRecent.SetBookmarkFilename ( sInitialPath + L"bookmarks.txt" );
	g_tRecent.Load ();

	SetInitialConfigValues ();

	CheckBTStacks ();

	g_PluginManager.Init ();
	Init_Panels ();

	CustomControl_c::Init ();

	protection::Init ();
	btns::Reset ();

	g_bCanChangeOrientation = CanChangeOrientation ();
	if ( g_bCanChangeOrientation && cfg->orientation != -1 )
	{
		g_iOldOrientation = GetDisplayOrientation ();
		ChangeDisplayOrientation ( cfg->orientation );
	}

	// create window
	if ( ! InitializeMainWindow () )
		return false;

	Log ( L" * Init finished" );

	RefreshStorageCards ();

	if ( cfg->first_launch )
	{
		SetupInitialDlgData ();
		SetInitialButtonValues ();
	}

	// create panels
	g_pPanel1 = new Panel_c;
	g_pPanel2 = new Panel_c;

	LoadPanelSettings ( szCmdLine );

	Log ( L" * Panels created" );

	// refresh after panel positions are set
	if ( cfg->pane_1_maximized )
		FMMaximizePanel ( g_pPanel1 );
	else
		if ( cfg->pane_2_maximized )
			FMMaximizePanel ( g_pPanel2 );
		else
		{
			g_pPanel1->Show ( true );
			g_pPanel2->Show ( true );
			g_pPanel1->Activate ( true );
		}


	g_pPanel1->Refresh ();
	g_pPanel2->Refresh ();

	LoadExtraOSFuncs ();

	OnActivate ( g_hMainWindow );

	g_bEnableMsg = true;
	cfg->first_launch = false;

	Log ( L" * Started" );

	return true;
}


//
// application shutdown
//
void AppShutdown ()
{
	Log ( L" * Shutting down..." );

	UnloadExtraOSFuncs ();

	menu::Shutdown ();

	CustomControl_c::Shutdown ();

	ShutdownMainWindow ();

	if ( g_bCanChangeOrientation && GetDisplayOrientation () == cfg->orientation )
		ChangeDisplayOrientation ( g_iOldOrientation );

	// update config variables
	SavePanelSettings ();

	// save config
	cfg->SaveVars ();

	// save buttons
	buttons->Save ();

	// save recent list
	g_tRecent.Save ();

	SafeDelete ( g_pPanel1 );
	SafeDelete ( g_pPanel2 );

	// modules shutdown
	Shutdown_Panels ();
	g_PluginManager.Shutdown ();

	g_tResources.Shutdown ();

	// store key to registry
	protection::Shutdown ();

	if ( IsWidcommBTPresent () )
		Shutdown_WidcommBT ();

	Shutdown_Sockets ();

	Shutdown_Resources ();
	Shutdown_Lang ();

	Buttons_c::Shutdown ();
	Colors_c::Shutdown ();
	Config_c::Shutdown ();

	Shutdown_Parser ();
	Shutdown_Core ();

	Log ( L" * Shutdown finished" );
}


int WINAPI WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd )
{
	g_hAppInstance = hInstance;

    // Allow only one instance of the application.
	HWND hWnd = FindWindow ( g_szWndClassName, NULL );
	if ( hWnd )
	{
	    SetForegroundWindow ((HWND)(((DWORD)hWnd) | 0x01));
	    return 0;
	}

	// modules init
	if ( !AppInit ( lpCmdLine ) )
	{
		AppShutdown ();
		return 0;
	}

	ShowWindow ( g_hMainWindow, nShowCmd );
	UpdateWindow ( g_hMainWindow );

	MSG tMsg;

	while ( GetMessage ( &tMsg, NULL, 0, 0 ) )
	{
        TranslateMessage ( &tMsg );
        DispatchMessage ( &tMsg );
    }

	AppShutdown ();

    return tMsg.wParam;
}