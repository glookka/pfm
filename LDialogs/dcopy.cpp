#include "pch.h"

#include "LDialogs/dcopy.h"
#include "LCore/clog.h"
#include "LFile/fmisc.h"
#include "LSettings/sconfig.h"
#include "LSettings/srecent.h"
#include "LSettings/slocal.h"
#include "LPanel/presources.h"
#include "LUI/udialogs.h"
#include "LDialogs/dcommon.h"
#include "LDialogs/dtree.h"


#include "aygshell.h"
#include "Resources/resource.h"

extern HWND g_hMainWindow;

static HWND g_hCombo = NULL;

static wchar_t g_szDest [PATH_BUFFER_SIZE];
static FileList_t * g_pList = NULL;


static void PopulateCopyList ( HWND hList, const wchar_t * szDest )
{
	SendMessage ( hList, CB_RESETCONTENT, 0, 0 );
	for ( int i = g_tRecent.GetNumCopyMove () - 1; i >= 0 ; --i )
		SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM) (g_tRecent.GetCopyMove ( i ).c_str () ) );

	SendMessage ( hList, CB_INSERTSTRING, 0, (LPARAM) szDest );
}


static void SetCopyText ( HWND hText )
{
	Assert ( g_pList );
	int nFiles = g_pList->m_dFiles.Length ();
	Assert ( nFiles );

	if ( nFiles > 1 )
	{
		wchar_t szSize [FILESIZE_BUFFER_SIZE];
		FileSizeToString ( g_pList->m_uSize, szSize, true );
		wsprintf ( g_szDest, Txt ( T_DLG_COPY_FILES ), nFiles, szSize );
	}
	else
	{
		const FileInfo_t * pInfo = g_pList->m_dFiles [0];
		Assert ( pInfo );
		wcscpy ( g_szDest, pInfo->m_tData.cFileName );
	}

	AlignFileName ( hText, g_szDest );
}


static void PopulateRenameList ( HWND hList, const wchar_t * szDest  )
{
	SendMessage ( hList, CB_RESETCONTENT, 0, 0 );
	for ( int i = g_tRecent.GetNumRename () - 1; i >= 0 ; --i )
		SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM) (g_tRecent.GetRename ( i ).c_str () ) );

	SendMessage ( hList, CB_INSERTSTRING, 0, (LPARAM) szDest );
}


static BOOL CALLBACK CopyDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
        {
			DlgTxt ( hDlg, IDC_TITLE, T_DLG_COPY_TITLE );
			DlgTxt ( hDlg, IDC_TO, T_DLG_TO );
			DlgTxt ( hDlg, IDC_COPY, T_TBAR_COPY );
			DlgTxt ( hDlg, IDC_MOVE, T_DLG_MOVE );
			DlgTxt ( hDlg, IDC_COPY_SHORTCUT, T_DLG_COPY_SHORTCUT );
			DlgTxt ( hDlg, IDCANCEL, T_TBAR_CANCEL );
			DlgTxt ( hDlg, IDC_TREE, T_CMN_TREE );

			HWND hTB = CreateToolbar ( hDlg, IDM_OK_CANCEL, 0, false );
			SetToolbarText ( hTB, IDOK, Txt ( T_TBAR_COPY ) );
			SetToolbarText ( hTB, IDCANCEL, Txt ( T_TBAR_CANCEL ) );

			g_hCombo = GetDlgItem ( hDlg, IDC_COPY_COMBO );

			SendMessage ( GetDlgItem ( hDlg, IDC_TITLE ), WM_SETFONT, (WPARAM)g_tResources.m_hBoldSystemFont, TRUE );

			PopulateCopyList ( g_hCombo, g_szDest );
			SetWindowText ( g_hCombo, g_szDest );
			SetCopyText ( GetDlgItem ( hDlg, IDC_COPY_TEXT ) );

			bool bCanShortcut = g_pList->m_dFiles.Length () == 1;
			if ( ! bCanShortcut )
				EnableWindow ( GetDlgItem ( hDlg, IDC_COPY_SHORTCUT ), FALSE );
        }
	    break;

    case WM_COMMAND:
		{
			int iCommand = LOWORD(wParam);
			if ( iCommand == IDOK )
				iCommand = IDC_COPY;

			switch ( iCommand )
			{
			case IDC_TREE:
				{
					GetWindowText ( g_hCombo, g_szDest, PATH_BUFFER_SIZE );
					Str_c sPath = g_szDest;
					if ( ShowDirTreeDlg ( hDlg, sPath ) )
						SetComboTextFocused ( g_hCombo, sPath );
				}
				break;
			case IDC_COPY:
			case IDC_COPY_SHORTCUT:
			case IDC_MOVE:
				GetWindowText ( g_hCombo, g_szDest, PATH_BUFFER_SIZE );
			case IDCANCEL:
				SipShowIM ( SIPF_OFF );
				EndDialog ( hDlg, iCommand );
				break;
			}
		}
		break;

	case WM_HELP:
		Help ( L"CopyMove" );
		return TRUE;
	}

	DWORD dwRes = HandleDlgColor ( hDlg, Msg, wParam, lParam );
	if ( dwRes )
		return dwRes;

	return MovingWindowProc ( hDlg, Msg, wParam, lParam );
}


static BOOL CALLBACK RenameDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
        {
			DlgTxt ( hDlg, IDOK, T_TBAR_OK );
			DlgTxt ( hDlg, IDCANCEL, T_TBAR_CANCEL );
			DlgTxt ( hDlg, IDC_TITLE, T_DLG_RENAME_TITLE );
			DlgTxt ( hDlg, IDC_TO, T_DLG_TO );

			CreateToolbar ( hDlg, IDM_OK_CANCEL );

			g_hCombo = GetDlgItem ( hDlg, IDC_RENAME_COMBO );
			SetFocus ( g_hCombo );

			SendMessage ( GetDlgItem ( hDlg, IDC_TITLE ), WM_SETFONT, (WPARAM)g_tResources.m_hBoldSystemFont, TRUE );

			PopulateRenameList ( g_hCombo, g_szDest );
			SetWindowText ( g_hCombo, g_szDest );

			Assert( g_pList );
			bool bDir = g_pList->IsDir ();

			Str_c sFileName = g_szDest;
			int iDotPos = sFileName.RFind ( L'.' );
			int iLast = 0;
			iLast = ( iDotPos == -1 || bDir ) ? sFileName.Length () : iDotPos;

			SendMessage ( g_hCombo, CB_SETEDITSEL, 0, iLast );

			AlignFileName ( GetDlgItem ( hDlg, IDC_RENAME_TEXT ), g_szDest );
        }
	    break;

    case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			GetWindowText ( g_hCombo, g_szDest, PATH_BUFFER_SIZE );
		case IDCANCEL:
			SipShowIM ( SIPF_OFF );
			EndDialog ( hDlg, LOWORD (wParam) );
            break;
		}
		break;

	case WM_HELP:
		Help ( L"Rename" );
		return TRUE;
	}

	DWORD dwRes = HandleDlgColor ( hDlg, Msg, wParam, lParam );
	if ( dwRes )
		return dwRes;

	return MovingWindowProc ( hDlg, Msg, wParam, lParam );
}


int ShowCopyMoveDialog ( Str_c & sDest, FileList_t & tList )
{
	g_pList = &tList;
	wcscpy ( g_szDest, sDest );
	int iRes = DialogBox ( ResourceInstance (), MAKEINTRESOURCE (IDD_COPYMOVE_FILES), g_hMainWindow, CopyDlgProc );
	if ( iRes != IDCANCEL )
		sDest = g_szDest;

	return iRes;
}


int ShowRenameFilesDialog ( Str_c & sDest, FileList_t & tList )
{
	Assert ( tList.m_dFiles.Length () == 1 );

	g_pList = &tList;
	wcscpy ( g_szDest, tList.m_dFiles [0]->m_tData.cFileName );
	int iRes = DialogBox ( ResourceInstance (), MAKEINTRESOURCE (IDD_RENAME_FILES), g_hMainWindow, RenameDlgProc );
	if ( iRes != IDCANCEL )
		sDest = g_szDest;

	return iRes;
}
