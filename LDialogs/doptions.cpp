#include "pch.h"

#include "LDialogs/doptions.h"
#include "pfm/std.h"
#include "pfm/config.h"
#include "pfm/resources.h"
#include "pfm/gui.h"
#include "pfm/panel.h"
#include "pfm/menu.h"
#include "pfm/pfm.h"
#include "pfm/iterator.h"

#include "pfm/dialogs/apps.h"
#include "pfm/dialogs/bookmarks.h"

#include "LFile/fapps.h"
#include "pfm/dialogs/errors.h"

#include "aygshell.h"
#include "Dlls/Resource/resource.h"

extern HINSTANCE g_hAppInstance;
extern HWND g_hMainWindow;

Array_T < LOGFONT > g_dFonts;
HWND g_hOptionsWnd = NULL;
bool g_bRegister = false;
bool g_bSheduleRefresh = false;
bool g_bUpdateAfterLoad = false;

int StrCompare ( const void * sStr1, const void * sStr2 )
{
	return wcscmp ( *(Str_c *)sStr1, *(Str_c *)sStr2 );
}


static bool CheckScheme ( const Str_c & sPath )
{
	DWORD uAttr = GetFileAttributes ( sPath + L"colors.ini" );

	if ( uAttr == 0xFFFFFFFF )
		return false;

	uAttr = GetFileAttributes ( sPath + L"groups.ini" );

	if ( uAttr == 0xFFFFFFFF )
		return false;

	return true;
}

// enumerate available color schemes
static void FindColorSchemes ( Array_T <Str_c> & dSchemes )
{
	dSchemes.Clear ();
	FileIteratorTree_c tIterator;
	Str_c sRoot = GetExecutablePath ();
	Str_c sFullSchemeName;
	AppendSlash ( sRoot );
	sRoot += L"schemes\\";

	tIterator.IterateStart ( sRoot, false );
	while ( tIterator.IterateNext () )
	{
		const WIN32_FIND_DATA * pData = tIterator.GetData ();
		if ( pData && ( pData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && ! tIterator.Is2ndPassDir () )
		{
			sFullSchemeName = tIterator.GetFullName ();
			if ( CheckScheme ( AppendSlash ( sFullSchemeName ) ) )
				dSchemes.Add ( sFullSchemeName );
		}
	}

	Sort ( dSchemes, StrCompare );
}


static int CALLBACK EnumFontFamProc1 ( const LOGFONT * pLogFont, const TEXTMETRIC * pMetric, ulong FontType, long lParam )
{
	g_dFonts.Add ( *pLogFont );
	return 1;
}


void ApplyDisplayChanges ( HWND hDlg )
{
	wchar_t szBuf [256];
	GetDlgItemText ( hDlg, IDC_FONT_NAME, szBuf, 255 );
	cfg->font_name = szBuf;

	cfg->font_height = GetDlgItemInt ( hDlg, IDC_FONT_SIZE, NULL, FALSE );

	cfg->font_bold = IsDlgButtonChecked ( hDlg, IDC_FONT_BOLD ) == BST_CHECKED;
	cfg->font_clear_type = IsDlgButtonChecked ( hDlg, IDC_FONT_SMOOTH ) == BST_CHECKED;

	g_tPanelFont.Shutdown ();
	g_tPanelFont.Init ();
}


void ApplyColorChanges ( HWND hDlg )
{
	wchar_t szBuf [256];

	HWND hScheme = GetDlgItem ( hDlg, IDC_SCHEME );
	if ( IsWindowEnabled ( hScheme ) )
	{
		GetDlgItemText ( hDlg, IDC_SCHEME, szBuf, 255 );
		if ( cfg->color_scheme != szBuf )
		{
			cfg->color_scheme = szBuf;
			FMLoadColorScheme ( cfg->color_scheme );
		}
	}
}


static BOOL CALLBACK DlgProcStyle ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	static Str_c sOldColorScheme;
	static Str_c sOldFontName;
	static int iOldFontHeight = 0;
	static bool bOldFontBold = false;
	static bool bOldFontClearType = false;

	static bool bInitFinished = false;

	switch ( Msg )
	{
	case WM_INITDIALOG:
		{
			bInitFinished = false;

			DlgTxt ( hDlg, IDC_FONT_STATIC, T_DLG_OPTIONS_FONT );
			DlgTxt ( hDlg, IDC_SIZE_STATIC, T_DLG_OPTIONS_SIZE );
			DlgTxt ( hDlg, IDC_FONT_BOLD, T_DLG_OPTIONS_BOLD );
			DlgTxt ( hDlg, IDC_FONT_SMOOTH, T_DLG_OPTIONS_SMOOTH );
			DlgTxt ( hDlg, IDC_SCHEME_STATIC, T_DLG_OPTIONS_SCHEME );
			DlgTxt ( hDlg, IDC_APPLY, T_DLG_OPTIONS_APPLY );
			DlgTxt ( hDlg, IDC_CURSOR_BAR, T_DLG_OPTIONS_CURSORBAR );

			// populate font list
			g_dFonts.Clear ();
			EnumFontFamilies ( GetWindowDC ( NULL ), NULL, EnumFontFamProc1, NULL );

			HWND hFontCombo = GetDlgItem ( hDlg, IDC_FONT_NAME );
			for ( int i = 0; i < g_dFonts.Length (); ++i )
				SendMessage ( hFontCombo, CB_ADDSTRING, 0, (LPARAM)( g_dFonts [i].lfFaceName ) );

			SendMessage ( hFontCombo, CB_SELECTSTRING, -1, (LPARAM)( cfg->font_name.c_str () ) );

			SetDlgItemInt ( hDlg, IDC_FONT_SIZE, cfg->font_height, FALSE );

			HWND hSizeSpin = GetDlgItem ( hDlg, IDC_FONT_SIZE_SPIN );
			SendMessage ( hSizeSpin, UDM_SETRANGE, 0, (LPARAM)MAKELONG ( 50, 1 ) );

			CheckDlgButton ( hDlg, IDC_FONT_BOLD, cfg->font_bold ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_FONT_SMOOTH, cfg->font_clear_type ? BST_CHECKED : BST_UNCHECKED );

			// schemes
			Array_T <Str_c> dSchemes;
			FindColorSchemes ( dSchemes );

			HWND hSchemeCombo = GetDlgItem ( hDlg, IDC_SCHEME );
			SendMessage ( hSchemeCombo, CB_RESETCONTENT, 0, 0 );

			if ( dSchemes.Empty () )
			{
				EnableWindow ( hSchemeCombo, FALSE );
			}
			else
			{
				EnableWindow ( hSchemeCombo, TRUE );
				for ( int i = 0; i < dSchemes.Length (); ++i )
					SendMessage ( hSchemeCombo, CB_ADDSTRING, 0, (LPARAM)( dSchemes [i].c_str () ) );

				SendMessage ( hSchemeCombo, CB_SELECTSTRING, -1, (LPARAM)( cfg->color_scheme.c_str () ) );
			}

//			FIXME
//			CheckDlgButton ( hDlg, IDC_CURSOR_BAR, cfg->pane_cursor_bar ? BST_CHECKED : BST_UNCHECKED );

			sOldColorScheme = cfg->color_scheme;
			sOldFontName = cfg->font_name;
			iOldFontHeight = cfg->font_height;
			bOldFontBold = cfg->font_bold;
			bOldFontClearType = cfg->font_clear_type;
	
			bInitFinished = true;
		}
	    break;

	case WM_PAINT:
		{
			RECT tRect;
			GetWindowRect ( GetDlgItem ( hDlg, IDC_BORDER ), &tRect );

			POINT tPt1 = { tRect.left, tRect.top };
			POINT tPt2 = { tRect.right, tRect.bottom };

			ScreenToClient ( hDlg, &tPt1 );
			ScreenToClient ( hDlg, &tPt2 );
			tRect.left = tPt1.x;
			tRect.top = tPt1.y;
			tRect.right = tPt2.x;
			tRect.bottom = tPt2.y;

			PAINTSTRUCT tPS;
			HDC hDC = BeginPaint ( hDlg, &tPS );

			SetBkMode ( hDC, TRANSPARENT );
			SetTextColor ( hDC, clrs->pane_font );

			FillRect ( hDC, &tRect, g_tResources.m_hBackgroundBrush );

			HGDIOBJ hOldFont = SelectObject ( hDC, g_tPanelFont.GetHandle () );
			DrawText ( hDC, L"Text", 4, &tRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE );
			SelectObject ( hDC, hOldFont );
			
			EndPaint ( hDlg, &tPS );
		}
		break;

    case WM_COMMAND:
		{
			switch ( LOWORD ( wParam ) )
			{
			case IDC_APPLY:
				ApplyColorChanges ( hDlg );
				InvalidateRect ( hDlg, NULL, FALSE );
				break;
			}

			bool bUpdate = false;
			int iId = (int)LOWORD(wParam);

			switch ( HIWORD(wParam) )
			{
			case BN_CLICKED:
				bUpdate = iId != IDC_APPLY;
				break;
			case CBN_SELCHANGE:
				bUpdate = iId != IDC_SCHEME;
				break;
			case EN_CHANGE:
				bUpdate = true;
				break;
			}

			if ( bInitFinished && bUpdate )
			{
				ApplyDisplayChanges ( hDlg );
				InvalidateRect ( hDlg, NULL, FALSE );
			}
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"Options" );
				break;
			case PSN_APPLY:
				bInitFinished = false;
//			FIXME
//				cfg->pane_cursor_bar = IsDlgButtonChecked ( hDlg, IDC_CURSOR_BAR ) == BST_CHECKED;

				ApplyDisplayChanges ( hDlg );
				ApplyColorChanges ( hDlg );
				FMUpdatePanelRects ();
				g_bSheduleRefresh = true;

				CloseFullscreenDlg ();
				break;
			case PSN_RESET:
				bInitFinished = false;
				cfg->font_name = sOldFontName;
				cfg->font_height = iOldFontHeight;
				cfg->font_bold = bOldFontBold;
				cfg->font_clear_type = bOldFontClearType;
				g_tPanelFont.Shutdown ();
				g_tPanelFont.Init ();

				if ( cfg->color_scheme != sOldColorScheme )
				{
					cfg->color_scheme = sOldColorScheme;
					FMLoadColorScheme ( sOldColorScheme );
					FMUpdatePanelRects ();
					g_bSheduleRefresh = true;
				}

				CloseFullscreenDlg ();
				break;
			}
		}
		break;
	}

	return FALSE;
}


static void InitViewList ( HWND hDlg, int iId )
{
	HWND hList = GetDlgItem ( hDlg, iId );
	SendMessage ( hList, CB_RESETCONTENT, 0, 0 );

	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_MNU_VIEW_1C ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_MNU_VIEW_2C ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_MNU_VIEW_3C ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_MNU_VIEW_BRIEF ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_MNU_VIEW_MEDIUM ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_MNU_VIEW_FULL ) ) );
}


void InitSortList ( HWND hDlg, int iId )
{
	HWND hList = GetDlgItem ( hDlg, iId );
	SendMessage ( hList, CB_RESETCONTENT, 0, 0 );

	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_MNU_SORT_UNSORTED ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_MNU_SORT_NAME ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_MNU_SORT_EXT ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_MNU_SORT_SIZE ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_MNU_SORT_TIME_CREATE ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_MNU_SORT_TIME_ACCESS ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_MNU_SORT_TIME_WRITE ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_MNU_SORT_GROUP ) ) );
}


static void InitOrientation ( HWND hDlg, int iId )
{
	HWND hList = GetDlgItem ( hDlg, iId );
	SendMessage ( hList, CB_RESETCONTENT, 0, 0 );

	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_DLG_LAYOUT_AUTO	) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_DLG_LAYOUT_VERT	) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_DLG_LAYOUT_HOR	) ) );
}


static void InitLongcut ( HWND hDlg, int iId )
{
	HWND hList = GetDlgItem ( hDlg, iId );
	SendMessage ( hList, CB_RESETCONTENT, 0, 0 );

	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_DLG_OPTIONS_LONGCUT1 ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_DLG_OPTIONS_LONGCUT2 ) ) );

	SendMessage ( hList, CB_SETCURSEL, cfg->pane_long_cut, 0 );
}


static BOOL CALLBACK DlgProcPanes ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
		{
			DlgTxt ( hDlg, IDC_LAYOUT_STATIC, T_DLG_OPTIONS_LAYOUT );
			DlgTxt ( hDlg, IDC_1ST_PANE_STATIC, T_DLG_OPTIONS_1ST_PANE );
			DlgTxt ( hDlg, IDC_LEFT_REVERSE, T_DLG_OPTIONS_PANE_REVERSE );
			DlgTxt ( hDlg, IDC_2ND_PANE_STATIC, T_DLG_OPTIONS_2ND_PANE );
			DlgTxt ( hDlg, IDC_RIGHT_REVERSE, T_DLG_OPTIONS_PANE_REVERSE );

			InitViewList ( hDlg, IDC_LEFT_VIEW );
			InitSortList ( hDlg, IDC_LEFT_SORT );
			InitViewList ( hDlg, IDC_RIGHT_VIEW );
			InitSortList ( hDlg, IDC_RIGHT_SORT );

			SendMessage ( GetDlgItem ( hDlg, IDC_LEFT_VIEW ), CB_SETCURSEL, cfg->pane_1_view, 0 );
			SendMessage ( GetDlgItem ( hDlg, IDC_LEFT_SORT ), CB_SETCURSEL, cfg->pane_1_sort, 0 );
			CheckDlgButton ( hDlg, IDC_LEFT_REVERSE, cfg->pane_1_sort_reverse ? BST_CHECKED : BST_UNCHECKED );

			SendMessage ( GetDlgItem ( hDlg, IDC_RIGHT_VIEW ), CB_SETCURSEL, cfg->pane_2_view, 0 );
			SendMessage ( GetDlgItem ( hDlg, IDC_RIGHT_SORT ), CB_SETCURSEL, cfg->pane_2_sort, 0 );
			CheckDlgButton ( hDlg, IDC_RIGHT_REVERSE, cfg->pane_2_sort_reverse ? BST_CHECKED : BST_UNCHECKED );

			// combo
			// 0 == auto
			// 1 == vertical
			// 2 == horizontal
			InitOrientation ( hDlg, IDC_LAYOUT );
			SendMessage ( GetDlgItem ( hDlg, IDC_LAYOUT ), CB_SETCURSEL, cfg->pane_force_layout + 1, 0 );
		}
	    break;

	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"Options" );
				break;
			case PSN_APPLY:
				cfg->pane_1_view = SendMessage ( GetDlgItem ( hDlg, IDC_LEFT_VIEW ), CB_GETCURSEL, 0, 0 );
				cfg->pane_1_sort = SendMessage ( GetDlgItem ( hDlg, IDC_LEFT_SORT ), CB_GETCURSEL, 0, 0 );
				cfg->pane_1_sort_reverse = IsDlgButtonChecked ( hDlg, IDC_LEFT_REVERSE ) == BST_CHECKED;

				cfg->pane_2_view = SendMessage ( GetDlgItem ( hDlg, IDC_RIGHT_VIEW ), CB_GETCURSEL, 0, 0 );
				cfg->pane_2_sort = SendMessage ( GetDlgItem ( hDlg, IDC_RIGHT_SORT ), CB_GETCURSEL, 0, 0 );
				cfg->pane_2_sort_reverse = IsDlgButtonChecked ( hDlg, IDC_RIGHT_REVERSE ) == BST_CHECKED;

				int iLayout = SendMessage ( GetDlgItem ( hDlg, IDC_LAYOUT ), CB_GETCURSEL, 0, 0 ) - 1;

				g_bUpdateAfterLoad = true;

				if ( iLayout != cfg->pane_force_layout )
				{
					cfg->pane_force_layout = iLayout;
					FMRepositionPanels ( true );
				}

				break;
			}
		}
		break;
	}

	return FALSE;
}


static BOOL CALLBACK DlgProcView ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
		{
			DlgTxt ( hDlg, IDC_MODES_STATIC, T_DLG_OPTIONS_PANE_MODES );
			DlgTxt ( hDlg, IDC_1C_STATIC, T_DLG_OPTIONS_1C );
			DlgTxt ( hDlg, IDC_2C_STATIC, T_DLG_OPTIONS_2C );
			DlgTxt ( hDlg, IDC_3C_STATIC, T_DLG_OPTIONS_3C );
			DlgTxt ( hDlg, IDC_D_STATIC, T_DLG_OPTIONS_D );
			DlgTxt ( hDlg, IDC_ICON_ONE, T_DLG_OPTIONS_ICONS );
			DlgTxt ( hDlg, IDC_EXT_ONE, T_DLG_OPTIONS_SEPEXT );
			DlgTxt ( hDlg, IDC_ICON_MID, T_DLG_OPTIONS_ICONS );
			DlgTxt ( hDlg, IDC_EXT_MID, T_DLG_OPTIONS_SEPEXT );
			DlgTxt ( hDlg, IDC_ICON_SHORT, T_DLG_OPTIONS_ICONS );
			DlgTxt ( hDlg, IDC_EXT_SHORT, T_DLG_OPTIONS_SEPEXT );
			DlgTxt ( hDlg, IDC_ICON_FULL, T_DLG_OPTIONS_ICONS );
			DlgTxt ( hDlg, IDC_EXT_FULL, T_DLG_OPTIONS_SEPEXT );

			DlgTxt ( hDlg, IDC_FILEINFO_STATIC, T_DLG_OPTIONS_FILEINFO );
			DlgTxt ( hDlg, IDC_ICON_BOTTOM, T_DLG_OPTIONS_FILEINFO_ICON );
			DlgTxt ( hDlg, IDC_DATE_BOTTOM, T_DLG_OPTIONS_FILEINFO_DATE );
			DlgTxt ( hDlg, IDC_TIME_BOTTOM, T_DLG_OPTIONS_FILEINFO_TIME );

			DlgTxt ( hDlg, IDC_LONG_STATIC, T_DLG_OPTIONS_LONGCUT );
			DlgTxt ( hDlg, IDC_DIR, T_DLG_OPTIONS_ADDDIR );

//			FIXME
/*			CheckDlgButton ( hDlg, IDC_ICON_ONE, cfg->pane_1c_file_icon ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_ICON_MID, cfg->pane_2c_file_icon ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_ICON_SHORT, cfg->pane_3c_file_icon ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_ICON_FULL, cfg->pane_full_file_icon ? BST_CHECKED : BST_UNCHECKED );

			CheckDlgButton ( hDlg, IDC_EXT_ONE, cfg->pane_1c_separate_ext ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_EXT_MID, cfg->pane_2c_separate_ext ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_EXT_SHORT, cfg->pane_3c_separate_ext ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_EXT_FULL, cfg->pane_full_separate_ext ? BST_CHECKED : BST_UNCHECKED );*/

			CheckDlgButton ( hDlg, IDC_ICON_BOTTOM, cfg->pane_fileinfo_icon ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_DATE_BOTTOM, cfg->pane_fileinfo_date ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_TIME_BOTTOM, cfg->pane_fileinfo_time ? BST_CHECKED : BST_UNCHECKED );

			CheckDlgButton ( hDlg, IDC_DIR, cfg->pane_dir_marker ? BST_CHECKED : BST_UNCHECKED );

			InitLongcut ( hDlg, IDC_LONGCUT );
		}
		break;
	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"Options" );
				break;
			case PSN_APPLY:
				// FIXME
				/*
				cfg->pane_1c_file_icon = 	IsDlgButtonChecked ( hDlg, IDC_ICON_ONE ) == BST_CHECKED;
				cfg->pane_2c_file_icon = 	IsDlgButtonChecked ( hDlg, IDC_ICON_MID ) == BST_CHECKED;
				cfg->pane_3c_file_icon = 	IsDlgButtonChecked ( hDlg, IDC_ICON_SHORT ) == BST_CHECKED;
				cfg->pane_full_file_icon = 	IsDlgButtonChecked ( hDlg, IDC_ICON_FULL ) == BST_CHECKED;

				cfg->pane_1c_separate_ext = 	IsDlgButtonChecked ( hDlg, IDC_EXT_ONE ) == BST_CHECKED;
				cfg->pane_2c_separate_ext =	IsDlgButtonChecked ( hDlg, IDC_EXT_MID ) == BST_CHECKED;
				cfg->pane_3c_separate_ext	=	IsDlgButtonChecked ( hDlg, IDC_EXT_SHORT ) == BST_CHECKED;
				cfg->pane_full_separate_ext =	IsDlgButtonChecked ( hDlg, IDC_EXT_FULL ) == BST_CHECKED;*/

				cfg->pane_fileinfo_icon = IsDlgButtonChecked ( hDlg, IDC_ICON_BOTTOM ) == BST_CHECKED;
				cfg->pane_fileinfo_date = IsDlgButtonChecked ( hDlg, IDC_DATE_BOTTOM ) == BST_CHECKED;
				cfg->pane_fileinfo_time = IsDlgButtonChecked ( hDlg, IDC_TIME_BOTTOM ) == BST_CHECKED;

				cfg->pane_dir_marker =	IsDlgButtonChecked ( hDlg, IDC_DIR ) == BST_CHECKED;
				cfg->pane_long_cut = SendMessage ( GetDlgItem ( hDlg, IDC_LONGCUT ), CB_GETCURSEL, 0, 0 );
				break;
			}
		}
		break;
	}

	return FALSE;
}

static void UpdateForcedControls ( HWND hDlg, bool bEnable )
{
	EnableWindow ( GetDlgItem ( hDlg, IDC_VIEW_COMBO ), bEnable ? TRUE : FALSE );
}

static BOOL CALLBACK DlgProcPanes2 ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
		DlgTxt ( hDlg, IDC_FORCE, T_DLG_OPTIONS_FORCEMAX );
// FIXME
//		CheckDlgButton ( hDlg, IDC_FORCE, cfg->pane_maximized_force ? BST_CHECKED : BST_UNCHECKED );
		InitViewList ( hDlg, IDC_VIEW_COMBO );
		SendMessage ( GetDlgItem ( hDlg, IDC_VIEW_COMBO ), CB_SETCURSEL, cfg->pane_maximized_view, 0 );
//		UpdateForcedControls ( hDlg, cfg->pane_maximized_force );
		break;

	case WM_COMMAND:
		{
			int iId = (int)LOWORD(wParam);

			switch ( HIWORD(wParam) )
			{
			case BN_CLICKED:
				if ( iId == IDC_FORCE )
				{
					bool bChecked = IsDlgButtonChecked ( hDlg, IDC_FORCE ) == BST_CHECKED;
					UpdateForcedControls ( hDlg, bChecked );
				}
				break;
			}
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"Options" );
				break;
			case PSN_APPLY:
// FIXME
//				cfg->pane_maximized_force = IsDlgButtonChecked ( hDlg, IDC_FORCE ) == BST_CHECKED;
				cfg->pane_maximized_view = SendMessage ( GetDlgItem ( hDlg, IDC_VIEW_COMBO ), CB_GETCURSEL, 0, 0 );
				g_bUpdateAfterLoad = true;
				break;
			}
		}
		break;
	}

	return FALSE;
}


static BOOL CALLBACK DlgProcFiles ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
		DlgTxt ( hDlg, IDC_FILES_STATIC, T_DLG_OPTIONS_SHOW_FILES );
		DlgTxt ( hDlg, IDC_SHOW_SYSTEM, T_DLG_OPTIONS_SYSTEM );
		DlgTxt ( hDlg, IDC_SHOW_HIDDEN, T_DLG_OPTIONS_HIDDEN );
		DlgTxt ( hDlg, IDC_SHOW_ROM, T_DLG_OPTIONS_ROM );
		DlgTxt ( hDlg, IDC_FILTERDIRS, T_DLG_OPTIONS_FILTERDIRS );

		CheckDlgButton ( hDlg, IDC_SHOW_HIDDEN, cfg->pane_show_hidden ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton ( hDlg, IDC_SHOW_SYSTEM, cfg->pane_show_system ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton ( hDlg, IDC_SHOW_ROM, cfg->pane_show_rom ? BST_CHECKED : BST_UNCHECKED );

		CheckDlgButton ( hDlg, IDC_FILTERDIRS, cfg->filter_include_dirs ? BST_CHECKED : BST_UNCHECKED );
		break;

	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"Options" );
				break;
			case PSN_APPLY:
				{
					bool bHidden = IsDlgButtonChecked ( hDlg, IDC_SHOW_HIDDEN ) == BST_CHECKED;
					bool bSystem = IsDlgButtonChecked ( hDlg, IDC_SHOW_SYSTEM ) == BST_CHECKED;
					bool bROM = IsDlgButtonChecked ( hDlg, IDC_SHOW_ROM ) == BST_CHECKED;

					if ( bHidden != cfg->pane_show_hidden ||
						bSystem != cfg->pane_show_system ||
						bROM != cfg->pane_show_rom )
					{
						cfg->pane_show_hidden = bHidden;
						cfg->pane_show_system = bSystem;
						cfg->pane_show_rom = bROM;

						FMGetPanel1 ()->SetVisibility ( cfg->pane_show_hidden, cfg->pane_show_system, cfg->pane_show_rom );
						FMGetPanel2 ()->SetVisibility ( cfg->pane_show_hidden, cfg->pane_show_system, cfg->pane_show_rom );
						g_bSheduleRefresh = true;
					}

					cfg->filter_include_dirs = IsDlgButtonChecked ( hDlg, IDC_FILTERDIRS ) == BST_CHECKED;
				}
				break;
			}
		}
		break;
	}

	return FALSE;
}


static void InitDisplayOrientation ( HWND hDlg, int iId )
{
	HWND hList = GetDlgItem ( hDlg, iId );
	SendMessage ( hList, CB_RESETCONTENT, 0, 0 );

	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_DLG_ORIENT_DEFAULT ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_DLG_ORIENT_0 ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_DLG_ORIENT_90 ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_DLG_ORIENT_180 ) ) );
	SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM)( Txt ( T_DLG_ORIENT_270 ) ) );
}

static void UpdateWM5Buttons ( HWND hDlg )
{
	bool bWM5Toolbar = IsDlgButtonChecked ( hDlg, IDC_WM5_TOOLBAR ) == BST_CHECKED;
	EnableWindow ( GetDlgItem ( hDlg, IDC_WM5_HIDE_TOOLBAR ), GetOSVersion () == WINCE_50 && bWM5Toolbar );
}

static bool g_bDoubleWarn = false;

static BOOL CALLBACK DlgProcDisplay ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
		{
			g_bDoubleWarn = false;
			DlgTxt ( hDlg, IDC_FULLSCREEN_CHECK, T_DLG_OPTIONS_FULLSCREEN );
			DlgTxt ( hDlg, IDC_ORIENT_STATIC, T_DLG_OPTIONS_ORIENTATION );
			DlgTxt ( hDlg, IDC_SPLITTER_STATIC, T_DLG_OPTIONS_SPLITTER );
			DlgTxt ( hDlg, IDC_SLIDER_STATIC, T_DLG_OPTIONS_SLIDER );
			DlgTxt ( hDlg, IDC_SLIDER_LEFT_CHECK, T_DLG_OPTIONS_SLIDER_LEFT );
			DlgTxt ( hDlg, IDC_WM5_TOOLBAR, T_DLG_OPTIONS_WM5BAR );
			DlgTxt ( hDlg, IDC_WM5_HIDE_TOOLBAR, T_DLG_OPTIONS_HIDEBAR );
			DlgTxt ( hDlg, IDC_2ND_TOOLBAR, T_DLG_OPTIONS_2NDBAR );
			DlgTxt ( hDlg, IDC_DOUBLE, T_DLG_OPTIONS_DOUBLE );

			CheckDlgButton ( hDlg, IDC_FULLSCREEN_CHECK, cfg->fullscreen ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_SLIDER_LEFT_CHECK, cfg->pane_slider_left ? BST_CHECKED : BST_UNCHECKED );

			EnableWindow ( GetDlgItem ( hDlg, IDC_WM5_TOOLBAR ), GetOSVersion () == WINCE_50 );
			CheckDlgButton ( hDlg, IDC_WM5_TOOLBAR, cfg->wm5_toolbar ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_WM5_HIDE_TOOLBAR, cfg->wm5_hide_toolbar ? BST_CHECKED : BST_UNCHECKED );

			UpdateWM5Buttons ( hDlg );

			CheckDlgButton ( hDlg, IDC_2ND_TOOLBAR, cfg->secondbar ? BST_CHECKED : BST_UNCHECKED );
			CheckDlgButton ( hDlg, IDC_DOUBLE, cfg->double_buffer ? BST_CHECKED : BST_UNCHECKED );

			InitDisplayOrientation ( hDlg, IDC_ORIENT_COMBO );

			HWND hOrientCombo = GetDlgItem ( hDlg, IDC_ORIENT_COMBO );
			if ( CanChangeOrientation () )
			{
				if ( cfg->orientation != -1 )
					cfg->orientation = GetDisplayOrientation ();

				SendMessage ( hOrientCombo, CB_SETCURSEL, cfg->orientation + 1, 0 );
			}
			else
			{
				SendMessage ( hOrientCombo, CB_SETCURSEL, 0, 0 );
				EnableWindow ( hOrientCombo, FALSE );
			}

			// splitter
			SetDlgItemInt ( hDlg, IDC_SPLITTER_HEIGHT, cfg->splitter_thickness, FALSE );
			HWND hSplitSpin = GetDlgItem ( hDlg, IDC_SPLITTER_HEIGHT_SPIN );
			SendMessage ( hSplitSpin, UDM_SETRANGE, 0, (LPARAM)MAKELONG ( 20, 3 ) );
			// slider
			SetDlgItemInt ( hDlg, IDC_SLIDER_WIDTH, cfg->pane_slider_size_x, FALSE );
			HWND hSliderSpin = GetDlgItem ( hDlg, IDC_SLIDER_WIDTH_SPIN );
			SendMessage ( hSliderSpin, UDM_SETRANGE, 0, (LPARAM)MAKELONG ( 50, 5 ) );
		}
		break;

	case WM_COMMAND:
		{
			int iId = (int)LOWORD(wParam);

			switch ( HIWORD(wParam) )
			{
			case BN_CLICKED:
				if ( iId == IDC_WM5_TOOLBAR )
					UpdateWM5Buttons ( hDlg );

				if ( iId == IDC_DOUBLE )
				{
					bool bChecked = IsDlgButtonChecked ( hDlg, IDC_DOUBLE ) == BST_CHECKED;
					if ( bChecked && ! cfg->double_buffer && !g_bDoubleWarn )
					{
						ShowErrorDialog ( g_hMainWindow, true, Txt ( T_DLG_OPTIONS_DOUBLE_WARN ), IDD_ERROR_OK );
						g_bDoubleWarn = true;
					}
				}
				break;
			}
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"Options" );
				break;

			case PSN_APPLY:
				{
					bool bNewWM5Toolbar = IsDlgButtonChecked ( hDlg, IDC_WM5_TOOLBAR ) == BST_CHECKED;
					bool bNewWM5HideToolbar = IsDlgButtonChecked ( hDlg, IDC_WM5_HIDE_TOOLBAR ) == BST_CHECKED;

					bool bHideDiffers = bNewWM5HideToolbar != cfg->wm5_hide_toolbar;
					if ( bNewWM5Toolbar != cfg->wm5_toolbar || bHideDiffers )
					{
						cfg->wm5_toolbar = bNewWM5Toolbar;
						cfg->wm5_hide_toolbar = bNewWM5HideToolbar;
						menu::RecreateMenuBar ();
						menu::HideWM5Toolbar ();

						if ( bHideDiffers )
							FMRepositionPanels ( true );
					}

					bool bFullscreen = IsDlgButtonChecked ( hDlg, IDC_FULLSCREEN_CHECK ) == BST_CHECKED;
					if ( cfg->fullscreen != bFullscreen )
						FMSetFullscreen ( bFullscreen );

					cfg->pane_slider_left = IsDlgButtonChecked ( hDlg, IDC_SLIDER_LEFT_CHECK ) == BST_CHECKED;
			
					cfg->orientation = SendMessage ( GetDlgItem ( hDlg, IDC_ORIENT_COMBO ), CB_GETCURSEL, 0, 0 ) - 1;
					cfg->orientation = Clamp ( cfg->orientation, -1, 3 );
					if ( cfg->orientation != -1 )
						ChangeDisplayOrientation ( cfg->orientation );
					
					cfg->splitter_thickness = 	GetDlgItemInt ( hDlg, IDC_SPLITTER_HEIGHT, NULL, FALSE );
					cfg->pane_slider_size_x = GetDlgItemInt ( hDlg, IDC_SLIDER_WIDTH, NULL, FALSE );

					FMUpdateSplitterThickness ();

					bool b2ndBar = IsDlgButtonChecked ( hDlg, IDC_2ND_TOOLBAR ) == BST_CHECKED;
					if ( b2ndBar != cfg->secondbar )
						menu::ShowSecondToolbar ( b2ndBar );

					cfg->double_buffer = IsDlgButtonChecked ( hDlg, IDC_DOUBLE ) == BST_CHECKED;
				}
				break;
			}
		}
		break;
	}
	
	return FALSE;
}

static bool g_bLongPress = false;
static HWND g_hRealAssignWnd = NULL;
static HWND g_hAssignDlg = NULL;


static BOOL CALLBACK BtnProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{
	btns::Event_e eEvent = btns::EVENT_NONE;
	int iKey = (int) wParam;

	switch ( Msg )
	{
	case WM_KEYUP:
		eEvent = btns::Event_Keyup ( iKey );
		break;

	case WM_KEYDOWN:
		eEvent = btns::Event_Keydown ( iKey );
		break;

	case WM_HOTKEY:
		eEvent = btns::Event_Hotkey ( (int) wParam, HIWORD(lParam), LOWORD(lParam) );
		break;

	case WM_TIMER:
		eEvent = btns::Event_Timer ( iKey );
		break;

	case WM_COMMAND:
		{
			switch ( LOWORD ( wParam ) )
			{
			case IDCANCEL:
				EndDialog ( g_hAssignDlg, -1 );
				break;
			}
		}
	}

	switch ( eEvent )
	{
	case btns::EVENT_LONGPRESS:
		g_bLongPress = true;
	case btns::EVENT_PRESS:
		EndDialog ( g_hAssignDlg, iKey );
		break;
	}

	return FALSE;
}

static BOOL CALLBACK AssignBtnProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	btns::Event_e eEvent = btns::EVENT_NONE;
	int iKey = (int) wParam;

	switch ( Msg )
	{
	case WM_INITDIALOG:
		{
			g_bLongPress = false;
			g_hAssignDlg = hDlg;
			DlgTxt ( hDlg, IDC_TEXT, T_DLG_ASSIGN_BTN );
			PostMessage ( hDlg, WM_USER, 0, 0 );
		}
		break;

	case WM_USER:
		{
			WNDCLASS tWC;
			memset ( &tWC, 0, sizeof ( tWC ) );
			tWC.lpfnWndProc		= (WNDPROC) BtnProc;
			tWC.hInstance		= g_hAppInstance;
			tWC.lpszClassName	= L"AssignWnd";
			RegisterClass ( &tWC );

			g_hRealAssignWnd = CreateWindow ( L"AssignWnd", Txt ( T_WND_TITLE ), WS_VISIBLE | WS_POPUP | WS_DLGFRAME, 0, 0, 0, 0, NULL, NULL, g_hAppInstance, NULL );
			SetTimer ( g_hRealAssignWnd, 123, 100, NULL );

			Assert ( UndergisterFunc );
			for ( int i = 0; i <= 0xFF; i++ )
			{
				UndergisterFunc ( MOD_WIN, i );
				RegisterHotKey ( g_hRealAssignWnd, i, MOD_WIN, i );
			}

			CreateToolbar ( g_hRealAssignWnd, IDM_CANCEL, SHCMBF_EMPTYBAR );
		}
		break;

	case WM_DESTROY:
		for ( int i = 0; i <= 0xFF; i++ )
			UnregisterHotKey ( g_hRealAssignWnd, i );

		DestroyWindow ( g_hRealAssignWnd );
		g_hRealAssignWnd = NULL;
		break;
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
class ButtonOfThreeDlg_c : public Dialog_Fullscreen_c
{
public:
	ButtonOfThreeDlg_c ( BtnAction_e eAction )
		: Dialog_Fullscreen_c ( L"Options", IDM_OK )
		, m_eAction	( eAction )
	{
		m_dItems [0].m_iGroup	= IDC_GROUP1;
		m_dItems [0].m_iLong	= IDC_LONG1;
		m_dItems [0].m_iAssign	= IDC_ASSIGN1;
		m_dItems [0].m_iClear	= IDC_CLEAR1;
		m_dItems [0].m_iName	= IDC_NAME1;
		m_dItems [1].m_iGroup	= IDC_GROUP2;
		m_dItems [1].m_iLong	= IDC_LONG2;
		m_dItems [1].m_iAssign	= IDC_ASSIGN2;
		m_dItems [1].m_iClear	= IDC_CLEAR2;
		m_dItems [1].m_iName	= IDC_NAME2;
		m_dItems [2].m_iGroup	= IDC_GROUP3;
		m_dItems [2].m_iLong	= IDC_LONG3;
		m_dItems [2].m_iAssign	= IDC_ASSIGN3;
		m_dItems [2].m_iClear	= IDC_CLEAR3;
		m_dItems [2].m_iName	= IDC_NAME3;
	}

	virtual void OnInit ()
	{
		Dialog_Fullscreen_c::OnInit ();

		Bold ( IDC_TITLE );
		Bold ( IDC_ACTION );

		Loc ( IDC_TITLE, T_DLG_BTN_ASSIGNMENT );
		ItemTxt ( IDC_ACTION, Str_c ( Txt ( T_DLG_BTN_ACTION ) ) + L"\"" + buttons->GetLitName ( m_eAction ) + L"\"" );
		
		for ( int i = 0; i < 3; ++i )
		{
			ItemTxt ( m_dItems [i].m_iGroup, NewString ( Txt ( T_DLG_NUMBTN ), i + 1 ) );
			Loc ( m_dItems [i].m_iLong,		T_DLG_LONGLONG );
			Loc ( m_dItems [i].m_iAssign,	T_DLG_OPTIONS_BTN_ASSIGN );
			Loc ( m_dItems [i].m_iClear,	T_DLG_OPTIONS_BTN_CLEAR );
			Update ( i );
		}
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Fullscreen_c::OnCommand ( iItem, iNotify );

		for ( int i = 0; i < 3; ++i )
		{
			if ( m_dItems [i].m_iAssign == iItem )
			{
				if ( Assign ( i ) )
				{
					for ( int i = 0; i < 3; ++i )
						Update ( i );
				}
				break;
			}

			if ( m_dItems [i].m_iClear == iItem )
			{
				Clear ( i );
				Update ( i );
				break;
			}
		}


		switch ( iItem )
		{
		case IDOK:
			for ( int i = 0; i < 3; ++i )
				buttons->SetLong ( m_eAction, i, IsChecked ( m_dItems [i].m_iLong ) );
			
			Close ( iItem );
			break;
		}
	}

private:
	struct Item_t
	{
		int	m_iGroup;
		int m_iLong;
		int m_iAssign;
		int m_iClear;
		int m_iName;
	};

	Item_t 		m_dItems [3];
	BtnAction_e m_eAction;

	bool Assign ( int iBtn )
	{
		int iAssignedBtn = DialogBox ( ResourceInstance (), MAKEINTRESOURCE (IDD_WAITBOX), m_hWnd, AssignBtnProc );
		if ( iAssignedBtn == -1 )
			return false;

		bool bCanAssign = true;

		// check if it was already assigned
		for ( int i = 0; bCanAssign && i < BA_TOTAL; ++i )
			for ( int j = 0; bCanAssign && j < 3; ++j )
				if ( ( i != m_eAction || j != iBtn ) && buttons->GetKey ( BtnAction_e ( i ), j ) == iAssignedBtn && buttons->GetLong ( BtnAction_e ( i ), j ) == g_bLongPress )
				{
					wchar_t szStr [128];
					wsprintf ( szStr, Txt ( T_DLG_ALREADY_ASSIGNED ), buttons->GetLitName ( BtnAction_e ( i ) ) );
					int iRes = ShowErrorDialog ( m_hWnd, true, szStr, IDD_ERROR_YES_NO );
					if ( iRes == IDOK )
						buttons->SetButton ( BtnAction_e (i), j, -1, false );
					else
						bCanAssign = false;
				}

		if ( bCanAssign )
			buttons->SetButton ( m_eAction, iBtn, iAssignedBtn, g_bLongPress );
			
		return bCanAssign;
	}

	void Clear ( int iBtn )
	{
		buttons->SetButton ( m_eAction, iBtn, -1, false );
		Update ( iBtn );
	}

	void Update ( int iBtn )
	{
		int iKey = buttons->GetKey ( m_eAction, iBtn );
		ItemTxt ( m_dItems [iBtn].m_iName, buttons->GetKeyLitName ( iKey ) );
		CheckBtn ( m_dItems [iBtn].m_iLong, buttons->GetLong ( m_eAction, iBtn ) );

		EnableWindow ( Item ( m_dItems [iBtn].m_iClear ), iKey != -1 );
		EnableWindow ( Item ( m_dItems [iBtn].m_iLong ), iKey != -1 );
	}
};

///////////////////////////
class ButtonAssignDlg_c
{
public:
	ButtonAssignDlg_c ()
		: m_hList ( NULL )
	{
	}

	void Init ( HWND hDlg )
	{
		DlgTxt ( hDlg, IDC_TITLE, T_DLG_OPTIONS_COMMANDS );
		DlgTxt ( hDlg, IDC_BTN_ASSIGN, T_DLG_OPTIONS_BTN_ASSIGN );
		DlgTxt ( hDlg, IDC_BTN_CLEAR, T_DLG_OPTIONS_BTN_CLEAR );
		DlgTxt ( hDlg, IDC_BLOCK, T_DLG_OPTIONS_BLOCK );

		SendMessage ( GetDlgItem ( hDlg, IDC_TITLE ), WM_SETFONT, (WPARAM)g_tResources.m_hBoldSystemFont, TRUE );

		m_dStayers.Add ( GetDlgItem ( hDlg, IDC_BTN_ASSIGN ) );
		m_dStayers.Add ( GetDlgItem ( hDlg, IDC_BTN_CLEAR ) );
		m_dStayers.Add ( GetDlgItem ( hDlg, IDC_BLOCK ) );

		m_hList = GetDlgItem ( hDlg, IDC_COMMAND_LIST );

		LVCOLUMN tColumn;
		ListView_SetExtendedListViewStyle ( m_hList, LVS_EX_FULLROWSELECT );

		ZeroMemory ( &tColumn, sizeof ( tColumn ) );
		tColumn.mask = LVCF_TEXT | LVCF_SUBITEM;
		tColumn.fmt = LVCFMT_LEFT;
		tColumn.iSubItem = 0;
		tColumn.pszText = L"";

		ListView_InsertColumn ( m_hList, tColumn.iSubItem,  &tColumn );
		++tColumn.iSubItem;
		ListView_InsertColumn ( m_hList, tColumn.iSubItem,  &tColumn );

		LVITEM tItem;
		ZeroMemory ( &tItem, sizeof ( tItem ) );
		tItem.mask = LVIF_TEXT;

		int nItems = 0;
		for ( int i = 0; i < BA_TOTAL; ++i )
		{
			BtnAction_e eAction = BtnAction_e ( i );
			tItem.pszText = (wchar_t*)buttons->GetLitName ( eAction );
			tItem.iItem = nItems;
			int iItem = ListView_InsertItem ( m_hList, &tItem );
			RefreshListItem ( eAction );
			++nItems;
		}

		ListView_SetItemState ( m_hList, 0, LVIS_SELECTED, 0 );

		CheckDlgButton ( hDlg, IDC_BLOCK, cfg->all_keys ? BST_CHECKED : BST_UNCHECKED );

		RECT tWndRect;
		Verify ( FMCalcWindowRect ( tWndRect, true, true ) );
		MoveWindow ( hDlg, tWndRect.left, tWndRect.top, tWndRect.right - tWndRect.left, tWndRect.bottom - tWndRect.top, TRUE );
		if ( HandleTabbedSizeChange ( 0, 0, 0, 0, true ) )
			Resize ( hDlg );
	}

	void AssignBtn ( HWND hDlg )
	{
		int iSelected = ListView_GetSelectionMark ( m_hList );
		if ( iSelected >= 0 )
		{
			ButtonOfThreeDlg_c Dlg ( (BtnAction_e) iSelected );
			Dlg.Run ( IDD_BTN_ASSIGN, hDlg );

			for ( int i = 0; i < BA_TOTAL; ++i )
				RefreshListItem ( BtnAction_e ( i ) );
		}
	}

	void ClearAllBtn ( HWND hDlg )
	{
		int iSelected = ListView_GetSelectionMark ( m_hList );
		if ( iSelected >= 0 )
		{
			for ( int i = 0; i < 3; ++i )
				buttons->SetButton ( BtnAction_e ( iSelected ), i, -1, false );

			RefreshListItem ( BtnAction_e ( iSelected ) );
		}
	}

	void OnContextMenu ( HWND hDlg, int iX, int iY )
	{
		int iSelected = ListView_GetSelectionMark ( m_hList );
		if ( iSelected < 0 )
			return;

		HMENU hMenu = CreatePopupMenu ();
		AppendMenu ( hMenu, MF_STRING, IDC_BTN_ASSIGN, Txt ( T_DLG_OPTIONS_BTN_ASSIGN ) );
		AppendMenu ( hMenu, MF_STRING, IDC_BTN_CLEAR, Txt ( T_DLG_OPTIONS_BTN_CLEAR ) );

		int iRes = TrackPopupMenu ( hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, iX, iY, 0, hDlg, NULL );
		DestroyMenu ( hMenu );

		switch ( iRes )
		{
		case IDC_BTN_ASSIGN:
			AssignBtn ( hDlg );
			break;
		case IDC_BTN_CLEAR:
			ClearAllBtn ( hDlg );
			break;
		}
	}

	void Resize ( HWND hDlg )
	{
		DoResize ( hDlg, m_hList, m_dStayers.Length () ? &(m_dStayers [0]) : NULL, m_dStayers.Length () );

		ListView_SetColumnWidth ( m_hList, 0, GetColumnWidthRelative ( 0.8f ) );
		ListView_SetColumnWidth ( m_hList, 1, GetColumnWidthRelative ( 0.2f ) );
	}

private:
	HWND 	m_hList;
	Array_T <HWND> m_dStayers;

	void RefreshListItem ( BtnAction_e eAction )
	{
		int nValid = 0;
		for ( int j = 0; j < 3; ++j )
			if ( buttons->GetKey ( eAction, j ) != -1 )
				++nValid;

		switch ( nValid )
		{
		case 0:
			ListView_SetItemText ( m_hList, eAction, 1, L"" );
			break;
		case 1:
			ListView_SetItemText ( m_hList, eAction, 1, L"+" );
			break;
		case 2:
			ListView_SetItemText ( m_hList, eAction, 1, L"++" );
			break;
		case 3:
			ListView_SetItemText ( m_hList, eAction, 1, L"+++" );
			break;
		}
	}
};

ButtonAssignDlg_c * g_pButtonAssignDlg = NULL;

static BOOL CALLBACK DlgProcButtons ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{
	int iKey = (int) wParam;

	switch ( Msg )
	{
	case WM_INITDIALOG:	
		Assert ( ! g_pButtonAssignDlg );
		g_pButtonAssignDlg = new ButtonAssignDlg_c ();
		g_pButtonAssignDlg->Init ( hDlg );
		break;

	case WM_DESTROY:
		SafeDelete ( g_pButtonAssignDlg );
		break;

	case WM_CONTEXTMENU:
		Assert ( g_pButtonAssignDlg );
		g_pButtonAssignDlg->OnContextMenu ( hDlg, LOWORD ( lParam ), HIWORD ( lParam ) );
		break;

	case WM_COMMAND:
		{
			int iId = LOWORD ( wParam );

			switch ( iId )
			{
			case IDC_BTN_ASSIGN:
				Assert ( g_pButtonAssignDlg );
				g_pButtonAssignDlg->AssignBtn ( hDlg );
				break;
			case IDC_BTN_CLEAR:
				Assert ( g_pButtonAssignDlg );
				g_pButtonAssignDlg->ClearAllBtn ( hDlg );
				break;
			}

			switch ( HIWORD(wParam) )
			{
			case BN_CLICKED:
				if ( iId == IDC_BLOCK )
				{
					cfg->all_keys = IsDlgButtonChecked ( hDlg, IDC_BLOCK ) == BST_CHECKED;
					AllKeys ( cfg->all_keys ? TRUE : FALSE );
				}
				break;
			}
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"Options" );
				break;

			case PSN_APPLY:
				cfg->all_keys = IsDlgButtonChecked ( hDlg, IDC_BLOCK ) == BST_CHECKED;
				buttons->RegisterHotKeys ( g_hMainWindow );
				break;
			}
		}
		break;
	}

	if ( HandleTabbedSizeChange ( hDlg, Msg, wParam, lParam ) )
		if ( g_pButtonAssignDlg )
			g_pButtonAssignDlg->Resize ( hDlg );

	return FALSE;
}


static BOOL CALLBACK DlgProcBookmarks ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
		DlgTxt ( hDlg, IDC_BM_DRIVES, T_DLG_OPTIONS_BM_CARDS );
		DlgTxt ( hDlg, IDC_BM_ICONS, T_DLG_OPTIONS_BM_ICONS );
		DlgTxt ( hDlg, IDC_EDIT, T_DLG_OPTIONS_BM_EDIT );

		CheckDlgButton ( hDlg, IDC_BM_DRIVES, cfg->bookmarks_drives ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton ( hDlg, IDC_BM_ICONS,  cfg->bookmarks_icons ? BST_CHECKED : BST_UNCHECKED );
		break;

	case WM_COMMAND:
		switch ( LOWORD ( wParam ) )
		{
		case IDC_EDIT:
			ShowEditBMDialog ( hDlg );
			break;
		}
		break;
		
	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"Options" );
				break;

			case PSN_APPLY:
				cfg->bookmarks_drives = IsDlgButtonChecked ( hDlg, IDC_BM_DRIVES ) == BST_CHECKED;
				cfg->bookmarks_icons =  IsDlgButtonChecked ( hDlg, IDC_BM_ICONS ) == BST_CHECKED;
				break;
			}
		}
		break;
	}

	return FALSE;
}


static void UpdateRefresh ( HWND hDlg )
{
	CheckDlgButton ( hDlg, IDC_AUTOREFRESH, cfg->refresh ? BST_CHECKED : BST_UNCHECKED );
	EnableWindow ( GetDlgItem ( hDlg, IDC_PERIOD ), cfg->refresh ? TRUE : FALSE );
	EnableWindow ( GetDlgItem ( hDlg, IDC_MAXFILES ), cfg->refresh ? TRUE : FALSE );
}

static BOOL CALLBACK DlgProcRefresh ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{
	switch ( Msg )
	{
	case WM_INITDIALOG:
		DlgTxt ( hDlg, IDC_AUTOREFRESH, T_DLG_OPTIONS_AUTOREFRESH );
		DlgTxt ( hDlg, IDC_PERIOD_STATIC, T_DLG_OPTIONS_PERIOD );
		DlgTxt ( hDlg, IDC_MAX_STATIC, T_DLG_OPTIONS_MAXFILES );

		UpdateRefresh ( hDlg );

		SetDlgItemText ( hDlg, IDC_PERIOD, Str_c ( cfg->refresh_period, L"%.1f" ).c_str () );
		SetDlgItemInt ( hDlg, IDC_MAXFILES, cfg->refresh_max_files, FALSE );
		break;

	case WM_COMMAND:
		{
			int iId = (int)LOWORD(wParam);

			switch ( HIWORD(wParam) )
			{
			case BN_CLICKED:
				if ( iId == IDC_AUTOREFRESH )
				{
					cfg->refresh = IsDlgButtonChecked ( hDlg, IDC_AUTOREFRESH ) == BST_CHECKED;
					UpdateRefresh ( hDlg );
				}
				break;
			}
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"Options" );
				break;

			case PSN_APPLY:
				cfg->refresh = IsDlgButtonChecked ( hDlg, IDC_AUTOREFRESH ) == BST_CHECKED;
				{
					const int BUF_SIZE = 128;
					wchar_t szBuf [BUF_SIZE];
					GetDlgItemText ( hDlg, IDC_PERIOD, szBuf, BUF_SIZE );
					cfg->refresh_period = (float) wcstod ( szBuf, L'\0' );

					GetDlgItemText ( hDlg, IDC_MAXFILES, szBuf, BUF_SIZE );
					cfg->refresh_max_files = _wtoi ( szBuf );

					if ( cfg->refresh_period <= 0.0f )
						cfg->refresh_period = 0.1f;

					if ( cfg->refresh_max_files < 0 )
						cfg->refresh_max_files = 0;
				}
				break;
			}
		}
		break;
	}

	return FALSE;
}


static BOOL CALLBACK DlgProcExternal ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
		DlgTxt ( hDlg, IDC_VIEWER_STATIC,	T_DLG_OPTIONS_EXT_VIEWER );
		DlgTxt ( hDlg, IDC_BROWSE,			T_DLG_OPTIONS_SELECT_APP );
		DlgTxt ( hDlg, IDC_PARAMS_STATIC,	T_DLG_RUN_PARAMS );
		SetDlgItemText ( hDlg, IDC_VIEWER,	cfg->ext_viewer );
		SetDlgItemText ( hDlg, IDC_PARAMS,	cfg->ext_viewer_params );
		break;

	case WM_COMMAND:
		switch ( LOWORD ( wParam ) )
		{
		case IDC_BROWSE:
			{
				int iApp = SelectAppDlg ( hDlg );
				if ( iApp != -1 )
				{
					const apps::AppInfo_t & tApp = apps::GetApp ( iApp );
					SetDlgItemText ( hDlg, IDC_VIEWER, Str_c ( L"\"" ) + tApp.m_sFileName + L"\"" );
				}
			}
			break;
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"Options" );
				break;

			case PSN_APPLY:
				{
					wchar_t szTmp [MAX_PATH];
					GetDlgItemText ( hDlg, IDC_VIEWER, szTmp, MAX_PATH );
					cfg->ext_viewer = szTmp;

					GetDlgItemText ( hDlg, IDC_PARAMS, szTmp, MAX_PATH );
					cfg->ext_viewer_params = szTmp;
				}
				break;
			}
		}
		break;
	}

	return FALSE;
}

static BOOL CALLBACK DlgProcConfirm ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
		DlgTxt ( hDlg, IDC_CONF_HISTORY, T_DLG_OPTIONS_SAVEHISTORY );
		DlgTxt ( hDlg, IDC_CONF_STATIC, T_DLG_OPTIONS_CONF_HEAD );
		DlgTxt ( hDlg, IDC_CONF_COPY_OVER, T_DLG_OPTIONS_CONF_COPY );
		DlgTxt ( hDlg, IDC_CONF_MOVE_OVER, T_DLG_OPTIONS_CONF_MOVE );
		DlgTxt ( hDlg, IDC_CONF_EXIT, T_DLG_OPTIONS_CONF_EXIT );

		CheckDlgButton ( hDlg, IDC_CONF_HISTORY,    ( ! cfg->save_dlg_history ) ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton ( hDlg, IDC_CONF_COPY_OVER, 	cfg->conf_copy_over ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton ( hDlg, IDC_CONF_MOVE_OVER, 	cfg->conf_move_over ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton ( hDlg, IDC_CONF_EXIT, 		cfg->conf_exit ? BST_CHECKED : BST_UNCHECKED );
		break;

	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"Options" );
				break;

			case PSN_APPLY:
				cfg->save_dlg_history= IsDlgButtonChecked ( hDlg, IDC_CONF_HISTORY ) != BST_CHECKED;
				cfg->conf_copy_over	= IsDlgButtonChecked ( hDlg, IDC_CONF_COPY_OVER ) == BST_CHECKED;
				cfg->conf_move_over	= IsDlgButtonChecked ( hDlg, IDC_CONF_MOVE_OVER ) == BST_CHECKED;
				cfg->conf_exit		= IsDlgButtonChecked ( hDlg, IDC_CONF_EXIT ) == BST_CHECKED;
				break;
			}
		}
		break;
	}

	return FALSE;
}

// enumerate available color schemes
static void EnumLangs ( Array_T <Str_c> & dLangs )
{
	dLangs.Clear ();

	Str_c sDir, sName, sExt;

	FileIteratorTree_c tIterator;
	tIterator.IterateStart ( GetExecutablePath (), false );
	while ( tIterator.IterateNext () )
	{
		SplitPath ( tIterator.GetData ()->cFileName, sDir, sName, sExt );
		if ( sExt == L".lang" )
			dLangs.Add ( sName );
	}

	Sort ( dLangs, StrCompare );
}


static BOOL CALLBACK DlgProcLanguage ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
		{
			DlgTxt ( hDlg, IDC_LANGUAGE_STATIC, T_DLG_OPTIONS_LANGUAGE );

			Array_T <Str_c> dLangs;
			EnumLangs ( dLangs );

			HWND hLangCombo = GetDlgItem ( hDlg, IDC_LANGUAGE_COMBO );
			for ( int i = 0; i < dLangs.Length (); ++i )
				SendMessage ( hLangCombo, CB_ADDSTRING, 0, (LPARAM)( dLangs [i].c_str () ) );

			SendMessage ( hLangCombo, CB_SELECTSTRING, -1, (LPARAM)( cfg->language.c_str () ) );
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"Options" );
				break;

			case PSN_APPLY:
				{
					wchar_t szPath [MAX_PATH];
					GetDlgItemText ( hDlg, IDC_LANGUAGE_COMBO, szPath, MAX_PATH );
					if ( cfg->language != szPath )
					{
						cfg->language = szPath;
						if ( MessageBox ( hDlg, Txt ( T_DLG_REG_RESTART ), Txt ( T_MSG_ATTENTION ), MB_OKCANCEL | MB_ICONINFORMATION ) == IDOK )
							PostQuitMessage ( 0 );
					}
				}
				break;
			}
		}
		break;
	}

	return FALSE;
}


static void SetupPage ( PROPSHEETPAGE & tPage, const wchar_t * szTitle, int iResource, DLGPROC fnDlgProc )
{
	ZeroMemory ( &tPage, sizeof ( tPage ) );
    tPage.dwSize 		= sizeof(PROPSHEETPAGE);
    tPage.dwFlags 		= PSP_DEFAULT | PSP_USETITLE;
    tPage.hInstance		= ResourceInstance ();
    tPage.pszTemplate	= MAKEINTRESOURCE(iResource);
    tPage.pszIcon		= NULL;
    tPage.pfnDlgProc 	= fnDlgProc;
	tPage.pszTitle 		= szTitle;
    tPage.lParam 		= 0;
    tPage.pfnCallback 	= NULL;
}


static int CALLBACK PropSheetProc ( HWND hDlg, UINT uMsg, LPARAM lParam )
{
	switch ( uMsg )
	{
		case PSCB_GETVERSION:
			return COMCTL32_VERSION;
		case PSCB_INITIALIZED:
			g_hOptionsWnd = hDlg;
			InitFullscreenDlg ( hDlg, IDM_OK_CANCEL, 0 );
			break;
	}
	
	return 0;
}


void ShowOptionsDialog ()
{
	g_bSheduleRefresh = false;
	g_bUpdateAfterLoad = false;

	const int nPages = 12;
	PROPSHEETPAGE dPages [nPages];
    PROPSHEETHEADER tHeader;
	
	SetupPage ( dPages [0], Txt ( T_DLG_TAB_STYLE ),	IDD_OPTIONS_STYLE, 		DlgProcStyle );	
	SetupPage ( dPages [1], Txt ( T_DLG_TAB_PANES_1 ),	IDD_OPTIONS_PANELS,		DlgProcPanes );
	SetupPage ( dPages [2], Txt ( T_DLG_TAB_PANES_2 ),	IDD_OPTIONS_VIEW2, 		DlgProcPanes2 );
	SetupPage ( dPages [3], Txt ( T_DLG_TAB_VIEW ),		IDD_OPTIONS_VIEW, 		DlgProcView );
	SetupPage ( dPages [4], Txt ( T_DLG_TAB_FILES ),	IDD_OPTIONS_FILES, 		DlgProcFiles );
	SetupPage ( dPages [5], Txt ( T_DLG_TAB_DISPLAY	),	IDD_OPTIONS_DISPLAY, 	DlgProcDisplay );
	SetupPage ( dPages [6], Txt ( T_DLG_TAB_BUTTONS	),	IDD_OPTIONS_BUTTONS,	DlgProcButtons );
	SetupPage ( dPages [7], Txt ( T_DLG_TAB_BOOKMARKS ),IDD_OPTIONS_BOOKMARKS,	DlgProcBookmarks );
	SetupPage ( dPages [8], Txt ( T_DLG_TAB_REFRESH ),	IDD_OPTIONS_REFRESH,	DlgProcRefresh );
	SetupPage ( dPages [9], Txt ( T_DLG_TAB_EXTERNAL ),	IDD_OPTIONS_EXTERNAL,	DlgProcExternal );
	SetupPage ( dPages [10], Txt ( T_DLG_TAB_DIALOGS ),	IDD_OPTIONS_CONFIRM,	DlgProcConfirm );
	SetupPage ( dPages [11], Txt ( T_DLG_TAB_LANGUAGE ),IDD_OPTIONS_LANGUAGE,	DlgProcLanguage );

    tHeader.dwSize 		= sizeof(PROPSHEETHEADER);
    tHeader.dwFlags 	= PSH_MAXIMIZE | PSH_PROPSHEETPAGE | PSH_USECALLBACK;
    tHeader.hwndParent 	= g_hMainWindow;
    tHeader.hInstance 	= ResourceInstance ();
    tHeader.nPages 		= nPages;
    tHeader.nStartPage 	= 0;
    tHeader.ppsp 		= (LPCPROPSHEETPAGE) &dPages;
    tHeader.pfnCallback = PropSheetProc;
    
	PropertySheet ( &tHeader );

	if ( g_bUpdateAfterLoad )
	{
// FIXME
//		FMGetPanel1 ()->UpdateAfterLoad ();
//		FMGetPanel2 ()->UpdateAfterLoad ();
	}
	
	if ( g_bSheduleRefresh )
	{
		FMGetPanel1 ()->Refresh ();
		FMGetPanel2 ()->Refresh ();
	}
}