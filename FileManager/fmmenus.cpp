#include "pch.h"

#include "Filemanager/fmmenus.h"
#include "LCore/cos.h"
#include "LComm/ccomm.h"
#include "LSettings/sconfig.h"
#include "LSettings/slocal.h"
#include "LPanel/pfile.h"
#include "LFile/fapps.h"
#include "LFile/fclipboard.h"
#include "LDialogs/doptions.h"
#include "LUI/udialogs.h"
#include "LDialogs/dcommon.h"
#include "LDialogs/dregister.h"
#include "Filemanager/fmfilefuncs.h"
#include "Filemanager/filemanager.h"

#include "aygshell.h"
#include "Resources/resource.h"

extern HWND g_hMainWindow;
extern HINSTANCE g_hAppInstance;

namespace menu
{
HWND 	g_hMenuBar 		= NULL;
HWND 	g_hSecondBar 	= NULL;
HMENU	g_hMarkMenu		= NULL;
HMENU	g_hMiscMenu 	= NULL;
HMENU	g_hEncryptMenu 	= NULL;
HMENU	g_hSendMenu 	= NULL;

HMENU	g_hSK1Menu		= NULL;
HMENU	g_hSK2Menu		= NULL;

const int NUM_TOOLBAR_BUTTONS = 9;
bool g_dToolbarBtnsState [NUM_TOOLBAR_BUTTONS] = {true, true, true, true, true, true, true, true, true};
const wchar_t * g_dTooltips [NUM_TOOLBAR_BUTTONS];

const int NUM_TOOLBAR_BUTTONS2 = 5;
bool g_dToolbarBtnsState2 [NUM_TOOLBAR_BUTTONS2] = {true, true, true, true, true};
const wchar_t * g_dTooltips2 [NUM_TOOLBAR_BUTTONS2];

static bool g_bInMenu = false;

// PREs
void ShowSecondToolbar ( bool bShow );
HMENU CreateUserMenu ( HMENU hMenu );
HMENU CreateToolbarMenu ( HMENU hMenu );
HMENU CreateNewMenu ( bool bWithMkDir );

///////////////////////////////////////////////////////////////////////////////////////////
// HELPERS
int TrackToolbarMenu ( HWND hMenuBar, int iBtn, HMENU hMenu, DWORD uFlags )
{
	RECT tWndRect, tBtnRect;
	GetWindowRect ( hMenuBar, &tWndRect );
	SendMessage ( hMenuBar, TB_GETITEMRECT, iBtn, (LPARAM) ( &tBtnRect ) );

	POINT tPt;
	if ( uFlags & TPM_RIGHTALIGN )
		tPt.x = tBtnRect.right + tWndRect.left;
	else
		tPt.x = tBtnRect.left + tWndRect.left;
		
	tPt.y = tBtnRect.top + tWndRect.top + 1;
	
	return TrackPopupMenu ( hMenu, uFlags, tPt.x, tPt.y, 0, g_hMainWindow, NULL );
}

///////////////////////////////////////////////////////////////////////////////////////////
// HANDLERS
bool HandleMiscMenu ( int iCommand )
{
	switch ( iCommand )
	{
	case IDM_MISC_OPTIONS:
		ShowOptionsDialog ();
		return true;
	case IDM_MISC_FULLSCREEN:
		FMToggleFullscreen ();
		return true;
	case IDM_MISC_ABOUT:
		ShowAboutDialog ();
		return true;
	case IDM_MISC_MINIMIZE:
		ShowWindow( g_hMainWindow, SW_MINIMIZE );
		return true;
	case IDM_MISC_EXIT:
		FMExit ();
		return true;
	}

	return false;
}

bool HandleCryptMenu ( int iCommand )
{
	switch ( iCommand )
	{
	case IDM_FILE_MENU_ENCRYPT:
		FMEncryptFiles ( true );
		return true;
	case IDM_FILE_MENU_DECRYPT:
		FMEncryptFiles ( false );
		return true;
	}

	return false;
}

bool HandleNewMenu ( int iRes )
{
	if ( iRes == IDC_MKDIR )
		FMMkDir ();
	else
	{
		if ( iRes >= IDM_NEWMENU_0 && iRes <= IDM_NEWMENU_64 )
			newmenu::GetItem ( iRes - IDM_NEWMENU_0 ).Run ();
		else
			return false;
	}

	return true;
}

bool HandleSelectionMenu ( int iCommand )
{
	Panel_c * pPanel = FMGetActivePanel ();

	switch ( iCommand )
	{
	case IDM_SELECTION_SELECT_ALL:
		pPanel->MarkAllFiles ( true );
		return true;
	case IDM_SELECTION_SELECT_NONE:
		pPanel->MarkAllFiles ( false );
		return true;
	case IDM_SELECTION_SELECT_INVERSE:
		pPanel->InvertMarkedFiles ();
		return true;
	case IDM_SELECTION_SELECT_FILTER:
	case IDM_SELECTION_SELECT_FILTER_INV:
		FMSelectFilter ( iCommand == IDM_SELECTION_SELECT_FILTER );
		return true;
	}

	return false;
}

bool HandleSendMenu ( int iCommand )
{
	switch ( iCommand )
	{
	case IDM_FILE_MENU_SEND_IR:
		FMSendIR ();
		return true;

	case IDM_FILE_MENU_SEND_BT:
		FMSendBT ();
		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
// WM handlers
bool OnCommand ( int iCommand )
{
	switch ( iCommand )
	{
	case IDC_FASTNAV:
		FMSetFastNavMode ( !g_tConfig.fastnav );
		return true;
		
	case IDC_SELECTION_CHECK:
		FMSetMarkMode ( ! FMMarkMode () );
		return true;

	case IDC_COPYMOVE_FILES:
		FMCopyMoveFiles ();
		return true;

	case IDC_RENAME:
		FMRenameFiles ();
		return true;

	case IDC_DELETE_FILES:
		FMDeleteFiles ();
		return true;

	case IDC_MKDIR:
		FMMkDir ();
		return true;

	case IDC_NEWDOC:
		{
			HMENU hNewMenu = CreateNewMenu ( true );
			int iRes = TrackToolbarMenu ( g_hMenuBar, 5, hNewMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD );
			DestroyMenu ( hNewMenu );
			HandleNewMenu ( iRes );
		}
		return true;

	case IDC_FIND:
		FMFindFiles ();
		return true;

	case IDC_TOOLBAR:
		ShowSecondToolbar ( ! g_tConfig.secondbar );
		FMSetFullscreen ( g_tConfig.fullscreen );
		FMRepositionPanels ( true );
		return true;

	case IDC_MISC:
		{
			CheckMenuItem ( g_hMiscMenu, IDM_MISC_FULLSCREEN, MF_BYCOMMAND | ( g_tConfig.fullscreen ? MF_CHECKED : MF_UNCHECKED ) );
			int iRes = TrackToolbarMenu ( g_hMenuBar, 8, g_hMiscMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD );
			HandleMiscMenu ( iRes );
		}
		return true;

	case IDM_BOOKMARKS:
		FMGetActivePanel ()->ShowBookmarks ();
		return true;

	case IDC_CRYPT:
		{
			int iRes = TrackToolbarMenu ( g_hSecondBar, 4, g_hEncryptMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD );
			HandleCryptMenu ( iRes );
		}
		return true;

	case IDC_VIEW:
		FMViewFile ();
		return true;

	case IDC_REFRESH:
		FMGetPanel1 ()->SoftRefresh ();
		FMGetPanel2 ()->SoftRefresh ();
		return true;

	case IDC_PROPERTIES:
		FMFileProperties ();
		return true;

	case IDC_SEND:
		{
			int iRes = TrackToolbarMenu ( g_hSecondBar, 3, g_hSendMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD );
			HandleSendMenu ( iRes );
		}
		return true;

	case IDC_SHORTCUT:
		FMCreateShortcut ( FMGetActivePanel ()->GetDirectory (), true );
		return true;

	case IDC_SWITCH_PANES:
		if ( FMGetActivePanel ()->IsMaximized () )
			FMMaximizePanel ( FMGetPassivePanel () );
		else
			FMActivatePanel ( FMGetPassivePanel () );
		break;

	case IDC_MAXIMIZE_PANE:
		if ( FMGetActivePanel ()->IsMaximized () )
			FMNormalPanels ();
		else
			FMMaximizePanel ( FMGetActivePanel () );
		break;

	case IDC_SAME_AS_OPPOSITE:
		FMGetActivePanel ()->SetDirectory ( FMGetPassivePanel ()->GetDirectory () );
		FMGetActivePanel ()->Refresh ();
		break;

	case IDC_FOLDER_UP:
		FMGetActivePanel ()->StepToPrevDir ();
		break;

	case IDC_COPYCLIP:
		clipboard::Copy ( FMGetActivePanel () );
		break;
		
	case IDC_CUTCLIP:
		clipboard::Cut ( FMGetActivePanel () );
		break;

	case IDC_PASTECLIP:
		clipboard::Paste ( FMGetActivePanel ()->GetDirectory () );
		break;
	}

	if ( menu::HandleMiscMenu ( iCommand ) )
		return true;

	if ( menu::HandleCryptMenu ( iCommand ) )
		return true;

	if ( menu::HandleSelectionMenu ( iCommand ) )
		return true;
	
	if ( menu::HandleSendMenu ( iCommand ) )
		return true;

	return false;
}

void OnNotify ( int iControlId, NMHDR * pInfo )
{
	Assert ( pInfo );

	if ( pInfo->code == TBN_DROPDOWN )
	{
		NMTOOLBAR * pToolBar = (NMTOOLBAR *) pInfo;
		if ( pToolBar->iItem == IDC_SELECTION_CHECK )
		{
			int iRes = TrackToolbarMenu ( g_hMenuBar, 0, g_hMarkMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD );
			HandleSelectionMenu ( iRes );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
// WM5 menus
bool OnInitMenuPopup ( HMENU hMenu )
{
	if ( hMenu == g_hSK1Menu )
	{
		bool bToolbarHidden = g_tConfig.wm5_toolbar && g_tConfig.wm5_hide_toolbar;
		EnableMenuItem ( hMenu, IDC_TOOLBAR, MF_BYCOMMAND | ( bToolbarHidden ? MF_GRAYED : MF_ENABLED ) );
		CheckMenuItem ( hMenu, IDC_TOOLBAR, MF_BYCOMMAND | ( Is2ndBarVisible () ? MF_CHECKED : MF_UNCHECKED ) );
		CheckMenuItem ( hMenu, IDM_MISC_FULLSCREEN, MF_BYCOMMAND | ( g_tConfig.fullscreen ? MF_CHECKED : MF_UNCHECKED ) );
		return true;
	}

	if ( hMenu == g_hSK2Menu )
	{
		CheckMenuItem ( hMenu, IDC_FASTNAV, MF_BYCOMMAND | ( g_tConfig.fastnav ? MF_CHECKED : MF_UNCHECKED ) );
		CheckMenuItem ( hMenu, IDC_SELECTION_CHECK, MF_BYCOMMAND | ( FMMarkMode () ? MF_CHECKED : MF_UNCHECKED ) );

		EnableMenuItem ( hMenu, IDC_PASTECLIP, clipboard::IsEmpty () ? MF_GRAYED : MF_ENABLED );
		return true;
	}

	return false;
}

void OnEnterMenu ()
{
	g_bInMenu = true;
}

void OnExitMenu ()
{
	g_bInMenu = false;
}

bool IsInMenu ()
{
	return g_bInMenu;
}

///////////////////////////////////////////////////////////////////////////////////////////
// toolbar funcs
void SetToolbarBtnState ( int iIndex, int iId, bool bEnable, bool bChecked = false )
{
	if ( g_dToolbarBtnsState [iIndex] != bEnable )
	{
		SendMessage ( g_hMenuBar, TB_SETSTATE, iId, bEnable ? TBSTATE_ENABLED | ( bChecked ? TBSTATE_CHECKED : 0 ) : 0 );
		g_dToolbarBtnsState [iIndex] = bEnable;
	}
}

void SetToolbarBtnState2 ( int iIndex, int iId, bool bEnable, bool bChecked = false )
{
	if ( g_dToolbarBtnsState2 [iIndex] != bEnable )
	{
		SendMessage ( g_hSecondBar, TB_SETSTATE, iId, bEnable ? TBSTATE_ENABLED | ( bChecked ? TBSTATE_CHECKED : 0 ) : 0 );
		g_dToolbarBtnsState2 [iIndex] = bEnable;
	}
}

void ShowSecondToolbar ( bool bShow )
{
	g_tConfig.secondbar = bShow;

	RECT tMenuRect;
	GetWindowRect ( g_hMenuBar, &tMenuRect );

	ShowWindow ( g_hSecondBar, bShow ? SW_SHOWNA : SW_HIDE );

	if ( bShow )
	{
		MoveWindow ( g_hSecondBar, tMenuRect.left, 2*tMenuRect.top - tMenuRect.bottom + 1, tMenuRect.right - tMenuRect.left, tMenuRect.bottom - tMenuRect.top, FALSE );
		SetForegroundWindow ( g_hMenuBar );
	}
	else
		MoveWindow ( g_hSecondBar, tMenuRect.left, tMenuRect.top - tMenuRect.bottom + 1, tMenuRect.right - tMenuRect.left, tMenuRect.bottom - tMenuRect.top, FALSE );

	SendMessage ( g_hMenuBar, TB_SETSTATE, IDC_TOOLBAR, ( g_tConfig.secondbar ? TBSTATE_CHECKED : 0 ) | TBSTATE_ENABLED );
}

void UpdateToolbar ()
{
	if ( ! FMGetPanel1 () || ! FMGetPanel2 () )
		return;

	Panel_c * pPanel = FMGetActivePanel ();

	int nFiles = pPanel->GetNumFiles ();

	bool bMultiSel = pPanel->GetNumMarked () > 1;
	bool bHaveFiles  = ! ( nFiles == 0 || ( nFiles == 1 && ( pPanel->GetFile ( 0 ).m_uFlags & FLAG_PREV_DIR ) ) );

	SetToolbarBtnState ( 1, IDC_SELECTION_CHECK,	bHaveFiles, FMMarkMode () );
	SetToolbarBtnState ( 2, IDC_COPYMOVE_FILES,		bHaveFiles );
	SetToolbarBtnState ( 3, IDC_RENAME,				bHaveFiles  && ! bMultiSel );
	SetToolbarBtnState ( 4, IDC_DELETE_FILES,		bHaveFiles );
	SetToolbarBtnState ( 6, IDC_PROPERTIES,			bHaveFiles );

	SetToolbarBtnState2 ( 1, IDC_VIEW,				bHaveFiles  && ! bMultiSel );
	SetToolbarBtnState2 ( 3, IDC_SEND,				bHaveFiles );
	SetToolbarBtnState2 ( 4, IDC_CRYPT,				bHaveFiles );
}

HWND CreateSecondBar ()
{
	// create the main menu bar
	HWND hMB = CreateToolbar ( g_hMainWindow, IDM_MENU, SHCMBF_HIDDEN );
	if ( ! hMB )
		return NULL;

	CommandBar_AddBitmap ( hMB, ResourceInstance (), IsVGAScreen () ? IDB_TOOLBAR2_VGA : IDB_TOOLBAR2, 15, 0, 0);

	const TBBUTTON atbbuttons[NUM_TOOLBAR_BUTTONS2] =
	{
		{0, IDC_FIND		,TBSTATE_ENABLED,	TBSTYLE_BUTTON, 0, 0, 0, -1},
		{1, IDC_VIEW		,TBSTATE_ENABLED,	TBSTYLE_BUTTON, 0, 0, 0, -1},
		{2, IDC_REFRESH		,TBSTATE_ENABLED,	TBSTYLE_BUTTON, 0, 0, 0, -1},
		{3, IDC_SEND		,TBSTATE_ENABLED,	TBSTYLE_BUTTON, 0, 0, 0, -1},
		{4, IDC_CRYPT		,TBSTATE_ENABLED,	TBSTYLE_BUTTON, 0, 0, 0, -1},
	};

    CommandBar_AddButtons ( hMB, NUM_TOOLBAR_BUTTONS2, atbbuttons );

	for ( int i = 0; i < NUM_TOOLBAR_BUTTONS2; ++i )
		g_dTooltips2 [i] = Txt ( T_TIP_FIND_FILES + i );

	CommandBar_AddToolTips ( hMB, NUM_TOOLBAR_BUTTONS2, g_dTooltips2 );

	return hMB;
}

HWND CreateMenuBar ()
{
	HWND hBar = NULL;

	if ( GetOSVersion () == WINCE_50 && g_tConfig.wm5_toolbar )
	{
		SHMENUBARINFO tMenuBar;
		memset ( &tMenuBar, 0, sizeof(SHMENUBARINFO) );
		tMenuBar.cbSize 	= sizeof(SHMENUBARINFO);
		tMenuBar.hwndParent = g_hMainWindow;
		tMenuBar.nToolBarId = IDR_SK1_SK2;
		tMenuBar.dwFlags	= 0;
		tMenuBar.hInstRes 	= ResourceInstance ();

		if ( SHCreateMenuBar ( &tMenuBar ) )
		{
			hBar = tMenuBar.hwndMB;

			SetToolbarText ( hBar, IDC_SK1, Txt ( T_TBAR_SK1 ) );
			SetToolbarText ( hBar, IDC_SK2, Txt ( T_TBAR_SK2 ) );

			TBBUTTONINFO tbbi;
			tbbi.cbSize = sizeof(tbbi);
			tbbi.dwMask = TBIF_LPARAM;

			SendMessage ( hBar, TB_GETBUTTONINFO, IDC_SK1, (LPARAM)&tbbi);
			g_hSK1Menu = (HMENU) tbbi.lParam;
			RemoveMenu ( g_hSK1Menu, 0, MF_BYPOSITION );
			CreateUserMenu ( g_hSK1Menu );

			SendMessage ( hBar, TB_GETBUTTONINFO, IDC_SK2, (LPARAM)&tbbi);
			g_hSK2Menu = (HMENU) tbbi.lParam;
			RemoveMenu ( g_hSK2Menu, 0, MF_BYPOSITION );
			CreateToolbarMenu ( g_hSK2Menu );
		}
	}
	else
	{
		hBar = CreateToolbar ( g_hMainWindow, IDM_MENU );
		if ( ! hBar )
			return NULL;

		CommandBar_AddBitmap ( hBar, ResourceInstance (), IsVGAScreen () ? IDB_TOOLBAR_VGA : IDB_TOOLBAR, 15, 0, 0 );

		const TBBUTTON atbbuttons[NUM_TOOLBAR_BUTTONS] =
		{
			{0, IDC_FASTNAV,		TBSTATE_ENABLED, TBSTYLE_CHECK,  0, 0, 0, -1},
			{1, IDC_SELECTION_CHECK,TBSTATE_ENABLED, TBSTYLE_DROPDOWN | TBSTYLE_CHECK,	 0, 0, 0, -1},
			{2, IDC_COPYMOVE_FILES,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1},
			{3, IDC_RENAME,			TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1},
			{4, IDC_DELETE_FILES,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1},
			{5, IDC_NEWDOC,			TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1},
			{6, IDC_PROPERTIES,		TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1},
			{7, IDC_TOOLBAR,		TBSTATE_ENABLED, TBSTYLE_BUTTON | TBSTYLE_CHECK, 0, 0, 0, -1},
			{8, IDC_MISC,			TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0, 0, -1},
		};
		CommandBar_AddButtons ( hBar, NUM_TOOLBAR_BUTTONS, atbbuttons );

		SendMessage ( hBar, TB_SETSTATE, IDC_FASTNAV, ( g_tConfig.fastnav ? TBSTATE_CHECKED : 0 ) | TBSTATE_ENABLED );
		SendMessage ( hBar, TB_SETSTATE, IDC_TOOLBAR, ( g_tConfig.secondbar ? TBSTATE_CHECKED : 0 ) | TBSTATE_ENABLED );

		for ( int i = 0; i < NUM_TOOLBAR_BUTTONS; ++i )
			g_dTooltips [i] = Txt ( T_TIP_TOGGLE_FASTNAV + i );

		CommandBar_AddToolTips ( hBar, NUM_TOOLBAR_BUTTONS, g_dTooltips );
	}

	return hBar;
}

void RecreateMenuBar ()
{
	DestroyWindow ( g_hMenuBar );
	g_hMenuBar = CreateMenuBar ();
}

bool Is2ndBarVisible ()
{
	return g_tConfig.secondbar;
}

bool IsToolbarHidden ()
{
	return g_tConfig.wm5_toolbar && g_tConfig.wm5_hide_toolbar;
}

void HideWM5Toolbar ()
{
	if ( IsToolbarHidden () )
	{
		int iWidth = 0, iHeight = 0;
		GetScreenResolution ( iWidth, iHeight );

		RECT Rect;
		GetWindowRect ( g_hMenuBar, &Rect );
		SetWindowPos ( g_hMenuBar, NULL, Rect.left, iHeight, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW );

		HWND hSip = FindWindow ( L"MS_SIPBUTTON", NULL );
		ShowWindow ( hSip, SW_HIDE );
	}
}

void GetToolbarRect ( RECT & tRect )
{
	GetWindowRect ( g_hMenuBar, &tRect );
}

void CheckMarkMode ( bool bMark )
{
	SendMessage ( g_hMenuBar, TB_SETSTATE, IDC_SELECTION_CHECK, bMark ? (TBSTATE_CHECKED | TBSTATE_ENABLED): TBSTATE_ENABLED );
}

void CheckFastNav ( bool bFastNav )
{
	SendMessage ( g_hMenuBar, TB_SETSTATE, IDC_FASTNAV, bFastNav ? (TBSTATE_CHECKED | TBSTATE_ENABLED): TBSTATE_ENABLED );
}

///////////////////////////////////////////////////////////////////////////////////////////
// menu creation funcs
HMENU CreateMarkMenu ()
{
	HMENU hMenu = CreatePopupMenu ();
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING, IDM_SELECTION_SELECT_FILTER_INV,Txt ( T_MNU_CLEAR_FILTER ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING, IDM_SELECTION_SELECT_FILTER,	Txt ( T_MNU_SELECT_FILTER ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING, IDM_SELECTION_SELECT_INVERSE,	Txt ( T_MNU_INVERT_SELECTION ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING, IDM_SELECTION_SELECT_NONE,		Txt ( T_MNU_CLEAR_SELECTION	) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING, IDM_SELECTION_SELECT_ALL,		Txt ( T_MNU_SELECT_ALL ) );

	return hMenu;
}

HMENU CreateMiscMenu ()
{
	HMENU hMenu = CreatePopupMenu ();
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING,		IDM_MISC_FULLSCREEN,Txt ( T_MNU_FULLSCREEN ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_SEPARATOR,	IDC_SEPARATOR,		L"" );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING,		IDM_MISC_OPTIONS,	Txt ( T_MNU_OPTIONS ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING,		IDM_MISC_ABOUT,		Txt ( T_MNU_ABOUT_REG ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_SEPARATOR,	IDC_SEPARATOR,		L"" );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING,		IDM_MISC_MINIMIZE,	Txt ( T_MNU_MINIMIZE ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING,		IDM_MISC_EXIT,		Txt ( T_MNU_EXIT ) );

	return hMenu;
}

HMENU CreateEncryptMenu ()
{
	HMENU hMenu = CreatePopupMenu ();
	AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_ENCRYPT, Txt ( T_MNU_ENCRYPT ) );
	AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_DECRYPT, Txt ( T_MNU_DECRYPT )	);

	return hMenu;
}

HMENU CreateSendMenu ()
{
	HMENU hMenu = CreatePopupMenu ();

	int iFlag = ( IsWidcommBTPresent () || IsMSBTPresent () ) ? 0 : MF_GRAYED;
	AppendMenu ( hMenu, MF_STRING | iFlag, IDM_FILE_MENU_SEND_BT, Txt ( T_MNU_SEND_BT ) );
	AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_SEND_IR, Txt ( T_MNU_SEND_IR ) );

	return hMenu;
}

HMENU CreateToolbarMenu ( HMENU hMenu )
{
	Assert ( hMenu );

	AppendMenu ( hMenu, MF_ENABLED | MF_STRING, IDC_FASTNAV,			Txt ( T_MNU_FASTNAV	) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING, IDC_SELECTION_CHECK,	Txt ( T_MNU_FASTSEL ) );

	HMENU hSubmenu = CreatePopupMenu ();
	AppendMenu ( hSubmenu, MF_ENABLED | MF_STRING, IDC_COPYMOVE_FILES,	Txt ( T_MNU_COPYMOVE ) );
	AppendMenu ( hSubmenu, MF_ENABLED | MF_STRING, IDC_RENAME,			Txt ( T_MNU_RENAME ) );
	AppendMenu ( hSubmenu, MF_ENABLED | MF_STRING, IDC_DELETE_FILES,	Txt ( T_MNU_DELETE ) );
	AppendMenu ( hSubmenu, MF_ENABLED | MF_STRING, IDC_MKDIR,			Txt ( T_MNU_MKDIR ) );
	AppendMenu ( hSubmenu, MF_POPUP | MF_STRING, (UINT) CreateNewMenu ( false ), Txt ( T_MNU_NEWDOC ) );
	AppendMenu ( hSubmenu, MF_ENABLED | MF_STRING, IDC_FIND,			Txt ( T_MNU_FIND_FILES ) );
	AppendMenu ( hSubmenu, MF_ENABLED | MF_STRING, IDC_VIEW,			Txt ( T_MNU_VIEW ) );
	AppendMenu ( hSubmenu, MF_ENABLED | MF_STRING, IDC_SHORTCUT,		Txt ( T_MNU_CREATE_SHORTCUT ) );
	AppendMenu ( hSubmenu, MF_ENABLED | MF_STRING, IDC_PROPERTIES,		Txt ( T_MNU_PROPS ) );
	AppendMenu ( hSubmenu, MF_POPUP | MF_STRING, (UINT) CreateSendMenu (),		Txt ( T_MNU_SEND ) );
	AppendMenu ( hSubmenu, MF_POPUP | MF_STRING, (UINT) CreateEncryptMenu (), 	Txt ( T_MNU_ENCRYPTION ) );
	AppendMenu ( hMenu, MF_POPUP | MF_STRING, (UINT) hSubmenu,			Txt ( T_MNU_FILE ) );

	hSubmenu = CreatePopupMenu ();
	AppendMenu ( hSubmenu, MF_ENABLED | MF_STRING,	IDC_COPYCLIP, 		Txt ( T_MNU_COPYCLIP ) );
	AppendMenu ( hSubmenu, MF_ENABLED | MF_STRING,	IDC_CUTCLIP, 		Txt ( T_MNU_CUTCLIP ) );
	AppendMenu ( hSubmenu, MF_ENABLED | MF_STRING,	IDC_PASTECLIP, 		Txt ( T_MNU_PASTECLIP ) );
	AppendMenu ( hMenu, MF_POPUP | MF_STRING, (UINT) hSubmenu,			Txt ( T_MNU_EDIT ) );

	AppendMenu ( hMenu, MF_POPUP | MF_STRING, (UINT) CreateMarkMenu (), Txt ( T_MNU_SELECTION ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING,		IDM_BOOKMARKS,		Txt ( T_MNU_BOOKMARKS ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_SEPARATOR,	IDC_SEPARATOR, L"" );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING,		IDM_MISC_OPTIONS,	Txt ( T_MNU_OPTIONS ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING,		IDM_MISC_ABOUT,		Txt ( T_MNU_ABOUT_REG ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING,		IDM_MISC_MINIMIZE,	Txt ( T_MNU_MINIMIZE ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING,		IDM_MISC_EXIT,		Txt ( T_MNU_EXIT ) );

	return hMenu;
}

HMENU CreateUserMenu ( HMENU hMenu )
{
	Assert ( hMenu );

	AppendMenu ( hMenu, MF_ENABLED | MF_STRING, IDC_FOLDER_UP,			Txt ( T_MNU_FOLDER_UP ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING, IDC_REFRESH,			Txt ( T_MNU_REFRESH ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING, IDC_SWITCH_PANES,		Txt ( T_MNU_SWITCH_PANES ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING, IDC_MAXIMIZE_PANE,		Txt ( T_MNU_MAXIMIZE_RESTORE ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING, IDC_SAME_AS_OPPOSITE,	Txt ( T_MNU_OPEN_2ND_PANE_FLD ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING, IDC_TOOLBAR,			Txt ( T_MNU_TOOLBAR ) );
	AppendMenu ( hMenu, MF_ENABLED | MF_STRING,	IDM_MISC_FULLSCREEN,	Txt ( T_MNU_FULLSCREEN ) );

	return hMenu;
}

HMENU CreateNewMenu ( bool bWithMkDir )
{
	HMENU hMenu = CreatePopupMenu ();

	if ( newmenu::EnumNewItems () && newmenu::GetNumItems () > 0 )
	{
		for ( int i = 0; i < newmenu::GetNumItems (); ++i )
			AppendMenu ( hMenu, MF_ENABLED | MF_STRING, IDM_NEWMENU_0 + i,	newmenu::GetItem ( i ).m_sName );

		if ( bWithMkDir )
			AppendMenu ( hMenu, MF_ENABLED | MF_SEPARATOR, IDC_SEPARATOR, L"" );
	}

	if ( bWithMkDir )
		AppendMenu ( hMenu, MF_ENABLED | MF_STRING, IDC_MKDIR, Txt ( T_MNU_MKDIR ) );

	return hMenu;
}

///////////////////////////////////////////////////////////////////////////////////////////
// init / shutdown
bool Init ()
{
	g_hMenuBar = CreateMenuBar ();
	if ( ! g_hMenuBar )
		return false;

	g_hSecondBar = CreateSecondBar ();
	if ( ! g_hSecondBar )
		return false;


	g_hMarkMenu = CreateMarkMenu ();
	if ( ! g_hMarkMenu )
		return false;

	g_hMiscMenu = CreateMiscMenu ();
	if ( ! g_hMiscMenu )
		return false;

	g_hEncryptMenu = CreateEncryptMenu ();
	if ( ! g_hEncryptMenu )
		return false;

	g_hSendMenu = CreateSendMenu ();
	if ( ! g_hSendMenu )
		return false;

	return true;
}

void Shutdown ()
{
	DestroyMenu ( g_hMarkMenu );
	DestroyMenu ( g_hMiscMenu );
	DestroyMenu ( g_hEncryptMenu );
	DestroyMenu ( g_hSendMenu );
}

}