#include "pch.h"

#include "LDialogs/dfind_progress.h"
#include "LCore/clog.h"
#include "LCore/cos.h"
#include "LFile/fmisc.h"
#include "LSettings/sconfig.h"
#include "LSettings/slocal.h"
#include "LPanel/presources.h"
#include "LUI/udialogs.h"
#include "LDialogs/dcommon.h"

#include "Resources/resource.h"
#include "aygshell.h"
#include "commctrl.h"

extern HWND g_hMainWindow;

class FindProgressDlg_c;

const DWORD MY_UNIQUE_TIMER_FIND = 0xABCDEF02;
static FileSearch_c * g_pFileFinder = NULL;
static FindProgressDlg_c * g_pFindProgressDlg = NULL;


class FindProgressDlg_c : public WindowResizer_c
{
public:
	FindProgressDlg_c ()
		: m_uTimer 		( 0 )
		, m_hMessage	( NULL )
		, m_hList		( NULL )
		, m_bSearchFinished ( false )
		, m_nLastFiles	( 0 )
	{
	}


	~FindProgressDlg_c ()
	{
	}


	void Init ( HWND hDlg )
	{
		m_hMessage = GetDlgItem ( hDlg, IDC_FIND_NUMFILES );
		m_hList = GetDlgItem ( hDlg, IDC_FIND_FILE_LIST );

		SetWindowText ( m_hMessage, Txt ( T_DLG_SEARCHING ) );

		// init list
		ListView_SetExtendedListViewStyle ( m_hList, LVS_EX_FULLROWSELECT );

		LVCOLUMN tColumn;
		ZeroMemory ( &tColumn, sizeof ( tColumn ) );
		tColumn.mask = LVCF_TEXT | LVCF_SUBITEM;
		tColumn.iSubItem = 0;
		tColumn.pszText = (wchar_t *)Txt ( T_DLG_COLUMN_NAME );

		ListView_InsertColumn ( m_hList, tColumn.iSubItem,  &tColumn );
		++tColumn.iSubItem;
		tColumn.pszText = (wchar_t *)Txt ( T_DLG_COLUMN_PATH );
		ListView_InsertColumn ( m_hList, tColumn.iSubItem,  &tColumn );

		ListView_SetImageList ( m_hList, g_tResources.m_hSmallIconList, LVSIL_SMALL );

		UpdateProgress ( true );

		InitFullscreenDlg ( hDlg, IDM_CANCEL, SHCMBF_HIDESIPBUTTON );

		m_uTimer = SetTimer ( hDlg, MY_UNIQUE_TIMER_FIND, 0, NULL );

		SetDlg ( hDlg );
		SetResizer ( m_hList );
		if ( HandleSizeChange ( 0, 0, 0, 0, true ) )
			Resize ();
	}

	void UpdateProgress ( bool bForce = false )
	{
		Assert ( g_pFileFinder );

		int nFiles = g_pFileFinder->GetNumFiles ();
		if ( nFiles && ( m_nLastFiles != nFiles || bForce ) )
		{	
			LVITEM tItem;
			ZeroMemory ( &tItem, sizeof ( tItem ) );
			tItem.mask = LVIF_TEXT | LVIF_IMAGE;

			for ( int i = 0; i < nFiles - m_nLastFiles; ++i )
			{
				const WIN32_FIND_DATA & tData = g_pFileFinder->GetData ( m_nLastFiles + i );
				Str_c sDir = g_pFileFinder->GetDirectory ( m_nLastFiles + i );

				// retrieve file type icon
				SHFILEINFO tShFileInfo;
				tShFileInfo.iIcon = -1;

				SHGetFileInfo ( sDir + tData.cFileName, 0, &tShFileInfo, sizeof (SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON );

				tItem.pszText = (wchar_t *) &tData.cFileName;
				tItem.iItem = m_nLastFiles + i;
				tItem.iImage = tShFileInfo.iIcon;

				int iItem = ListView_InsertItem ( m_hList, &tItem );
				ListView_SetItemText( m_hList, iItem, 1, (wchar_t *)( sDir.c_str () ) );
			}

			m_nLastFiles = nFiles;

			SetWindowText ( m_hMessage, NewString ( Txt ( T_DLG_SEARCH_RESULTS ), nFiles ) );
		}
	}

	void SearchFinished ( HWND hDlg, bool bCancelled )
	{
		m_bSearchFinished = true;
		KillTimer ( hDlg, m_uTimer );

		HWND hTB = CreateToolbar ( hDlg, IDM_OK_CANCEL, SHCMBF_HIDESIPBUTTON, false );
		SetToolbarText ( hTB, IDOK, Txt ( T_TBAR_GOTO ) );
		SetToolbarText ( hTB, IDCANCEL, Txt ( T_TBAR_CANCEL ) );

		SetWindowText ( m_hMessage, NewString ( Txt ( bCancelled ? T_DLG_SEARCH_CANCELLED : T_DLG_SEARCH_FINISHED ) , g_pFileFinder->GetNumFiles () ) );
	}

	bool IsSearchFinished () const
	{
		return m_bSearchFinished;
	}

	bool MoveToSelected ()
	{
		int iSelected = ListView_GetSelectionMark ( m_hList );
		if ( iSelected == -1 )
			return false;

		m_sFileToMoveTo = g_pFileFinder->GetData ( iSelected ).cFileName;
		m_sDirToMoveTo = g_pFileFinder->GetDirectory ( iSelected );
		return true;
	}

	const Str_c & GetDirToMoveTo () const
	{
		return m_sDirToMoveTo;
	}

	const Str_c & GetFileToMoveTo () const
	{
		return m_sFileToMoveTo;
	}

	void Shutdown ()
	{
		ListView_SetImageList ( m_hList, NULL, LVSIL_SMALL );
		CloseFullscreenDlg ();
	}

	void Resize ()
	{
		WindowResizer_c::Resize ();

		ListView_SetColumnWidth ( m_hList, 0, GetColumnWidthRelative ( 0.48f ) );
		ListView_SetColumnWidth ( m_hList, 1, GetColumnWidthRelative ( 0.52f ) );
	}

private:
	UINT			m_uTimer;
	HWND			m_hMessage;
	HWND			m_hList;
	bool			m_bSearchFinished;
	int				m_nLastFiles;

	Str_c			m_sDirToMoveTo;
	Str_c			m_sFileToMoveTo;
};


static BOOL CALLBACK DlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	Assert ( g_pFindProgressDlg );

	switch ( Msg )
	{
	case WM_INITDIALOG:
		g_pFindProgressDlg->Init ( hDlg );
		break;

	case WM_TIMER:
		if ( wParam == MY_UNIQUE_TIMER_FIND && ! g_pFindProgressDlg->IsSearchFinished () )
		{
			if ( g_pFileFinder->SearchNext () )
				g_pFindProgressDlg->UpdateProgress ();
			else
				g_pFindProgressDlg->SearchFinished ( hDlg, false );
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			if ( g_pFindProgressDlg->IsSearchFinished () )
			{
				SipShowIM ( SIPF_OFF );
				g_pFindProgressDlg->Shutdown ();
				EndDialog ( hDlg, LOWORD (wParam) );
			}
			else
				g_pFindProgressDlg->SearchFinished ( hDlg, true );
			break;

		case IDOK:
			if ( !g_pFindProgressDlg->MoveToSelected () )
				break;

			SipShowIM ( SIPF_OFF );
			g_pFindProgressDlg->Shutdown ();
			EndDialog ( hDlg, LOWORD (wParam) );
			break;
		}
		break;

/*	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			if ( pInfo->code == LVN_ITEMCHANGED )
			{
				NMLISTVIEW * pNMV = (NMLISTVIEW *) lParam;
				if ( ( pNMV->uChanged & LVIF_STATE ) && ( pNMV->uNewState & LVIS_SELECTED ) )
				{
					if ( g_pFindProgressDlg->MoveToSelected () )
					{
						SipShowIM ( SIPF_OFF );
						g_pFindProgressDlg->Shutdown ();
						EndDialog ( hDlg, LOWORD (wParam) );
					}
				}
			}
		}
		break;
*/
	case WM_HELP:
		Help ( L"FileSearch" );
		return TRUE;
	}

	if ( HandleSizeChange ( hDlg, Msg, wParam, lParam ) )
		g_pFindProgressDlg->Resize ();

	HandleActivate ( hDlg, Msg, wParam, lParam );

	return FALSE;
}


///////////////////////////////////////////////////////////////////////////////////////////
// wrapper funcs
bool DlgFindProgress ( FileList_t & tList, const FileSearchSettings_t & tSettings, Str_c & sDir, Str_c & sFile )
{
	Assert ( ! g_pFindProgressDlg && ! g_pFileFinder );
	g_pFindProgressDlg = new FindProgressDlg_c;
	g_pFileFinder = new FileSearch_c ( tList, tSettings );

	DialogBox ( ResourceInstance (), MAKEINTRESOURCE ( IDD_FIND_RESULTS ), g_hMainWindow, DlgProc );

	bool bMoveToFile = false;

	sDir = g_pFindProgressDlg->GetDirToMoveTo ();
	sFile = g_pFindProgressDlg->GetFileToMoveTo ();
	bMoveToFile = ! sFile.Empty () || ! sDir.Empty ();
	
	SafeDelete ( g_pFindProgressDlg );
	SafeDelete ( g_pFileFinder );

	return bMoveToFile;
}