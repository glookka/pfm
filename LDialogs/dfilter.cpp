#include "pch.h"

#include "LDialogs/dfilter.h"
#include "LSettings/sconfig.h"
#include "LSettings/srecent.h"
#include "LSettings/slocal.h"
#include "LFile/fmisc.h"
#include "LPanel/presources.h"
#include "LUI/udialogs.h"
#include "LDialogs/dcommon.h"

#include "aygshell.h"
#include "Resources/resource.h"

extern HWND g_hMainWindow;

static HWND		g_hFilterCombo = NULL;
static Str_c	g_sFilter;
static bool		g_bMark = true;


static void PopulateFilterList ( HWND hList )
{
	SendMessage ( hList, CB_RESETCONTENT, 0, 0 );

	int nFilters = g_tRecent.GetNumFilters ();
	for ( int i = nFilters - 1; i >= 0 ; --i )
		SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM) (g_tRecent.GetFilter ( i ).c_str () ) );

	if ( ! nFilters )
		SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM) L"*" );

	SendMessage ( hList, CB_SETCURSEL, 0, 0 );
}


static BOOL CALLBACK FilterDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
        {
			DlgTxt ( hDlg, IDOK, T_TBAR_OK );
			DlgTxt ( hDlg, IDCANCEL, T_TBAR_CANCEL );
			
			CreateToolbar ( hDlg, IDM_OK_CANCEL );

			g_hFilterCombo = GetDlgItem ( hDlg, IDC_FILTER_COMBO );

			HWND hHeader = GetDlgItem ( hDlg, IDC_FILTER_HEADER );
			SetWindowText ( hHeader, Txt ( g_bMark ? T_DLG_SELECT_FILES : T_DLG_CLEAR_FILES ) );
			SendMessage ( hHeader, WM_SETFONT, (WPARAM)g_tResources.m_hBoldSystemFont, TRUE );

			PopulateFilterList ( g_hFilterCombo );
        }
	    break;

    case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			{
				wchar_t szFilter [PATH_BUFFER_SIZE];
				GetWindowText ( g_hFilterCombo, szFilter, PATH_BUFFER_SIZE );
				g_sFilter = szFilter;
			}
		case IDCANCEL:
			SipShowIM ( SIPF_OFF );
			EndDialog ( hDlg, LOWORD (wParam) );
            break;
		}
		break;

	case WM_HELP:
		Help ( L"Toolbar" );
		return TRUE;
	}

	return MovingWindowProc ( hDlg, Msg, wParam, lParam );
}


bool ShowSetFilterDialog ( Str_c & sFilter, bool bMark )
{
	g_bMark = bMark;
	int iRes = DialogBox ( ResourceInstance (), MAKEINTRESOURCE (IDD_SET_FILTER), g_hMainWindow, FilterDlgProc );
	if ( iRes == IDOK )
	{
		sFilter = g_sFilter;
		return true;
	}

	return false;
}