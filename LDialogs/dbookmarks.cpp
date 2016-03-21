#include "pch.h"

#include "LDialogs/dbookmarks.h"
#include "LCore/clog.h"
#include "LCore/cos.h"
#include "LCore/cfile.h"
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

class AddBMDlg_c : public Dialog_Moving_c
{
public:
	AddBMDlg_c ( const Str_c & sPath )
		: Dialog_Moving_c ( L"Bookmarks", IDM_OK_CANCEL )
		, m_sPath ( sPath )
	{
	}

	virtual void OnInit ()
	{
		Dialog_Moving_c::OnInit ();

		Loc ( IDC_TITLE,		T_DLG_BM_ADD );
		Loc ( IDC_NAME_STATIC,	T_DLG_NAME_HEAD );
		Loc ( IDOK,				T_TBAR_OK );
		Loc ( IDCANCEL,			T_TBAR_CANCEL );

		AlignFileName ( Item ( IDC_BM_PATH ), m_sPath );

		int iEndOffset = m_sPath.Length () - ( EndsInSlash ( m_sPath ) ? 2 : 1 );

		int iIndex = m_sPath.RFind ( L'\\', iEndOffset );
		if ( iEndOffset >= 0 && iIndex != -1 )
			m_sName = m_sPath.SubStr ( iIndex + 1, iEndOffset - iIndex );
		else
			m_sName = m_sPath;

		ItemTxt ( IDC_BM_NAME, m_sName );
		Bold ( IDC_TITLE );
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Moving_c::OnCommand ( iItem, iNotify );

		switch ( iItem )
		{
		case IDOK:
			g_tRecent.AddBookmark ( GetItemTxt ( IDC_BM_NAME ), m_sPath );
		case IDCANCEL:
			Close ( iItem );
			break;
		}
	}

private:
	Str_c 	m_sPath;
	Str_c 	m_sName;
};

//////////////////////////////////////////////////////////////////////////

class ModifyBMDlg_c : public Dialog_Moving_c
{
public:
	ModifyBMDlg_c ( const Str_c & sName, const Str_c & sPath, bool bAdd )
		: Dialog_Moving_c ( L"Bookmarks", IDM_OK_CANCEL )
		, m_sPath	( sPath )
		, m_sName	( sName )
		, m_bAdd	( bAdd )
	{
	}

	virtual void OnInit ()
	{
		Dialog_Moving_c::OnInit ();

		Loc ( IDOK,				T_TBAR_OK );
		Loc ( IDCANCEL,			T_TBAR_CANCEL );
		Loc ( IDC_TITLE,		m_bAdd ? T_DLG_BMM_TITLE_ADD : T_DLG_BMM_TITLE_MOD );
		Loc ( IDC_PATH,			T_DLG_BMM_PATH );
		Loc ( IDC_NAME,			T_DLG_BMM_NAME );
		Loc	( IDC_TREE,			T_CMN_TREE );
		
		ItemTxt ( IDC_NAME_EDIT, m_sName );
		ItemTxt ( IDC_PATH_EDIT, m_sPath );

		Bold ( IDC_TITLE );
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Moving_c::OnCommand ( iItem, iNotify );

		switch ( iItem )
		{
		case IDC_TREE:
			if ( ShowFileTreeDlg ( m_hWnd, m_sPath, true ) )
				SetEditTextFocused ( Item ( IDC_PATH_EDIT ), m_sPath );
			break;

		case IDOK:
			m_sName = GetItemTxt ( IDC_NAME_EDIT );
			m_sPath = GetItemTxt ( IDC_PATH_EDIT );
		case IDCANCEL:
			Close ( iItem );
			break;
		}
	}

	const Str_c & Name () const
	{
		return m_sName;
	}

	const Str_c & Path () const
	{
		return m_sPath;
	}

private:
	Str_c 	m_sPath;
	Str_c 	m_sName;
	bool	m_bAdd;
};

//////////////////////////////////////////////////////////////////////////
class EditBMDlg_c : public Dialog_Resizer_c
{
public:
	EditBMDlg_c ()
		: Dialog_Resizer_c ( L"Bookmarks", IDM_OK )
		, m_hList	( NULL )
	{
	}

	virtual void OnInit ()
	{
		Dialog_Resizer_c::OnInit ();

		Loc ( IDC_TITLE,		T_DLG_BM_EDIT );
		Loc ( IDC_BM_UP,		T_DLG_BM_UP );
		Loc ( IDC_BM_DOWN,		T_DLG_BM_DOWN );
		Loc ( IDC_BM_DELETE,	T_DLG_BM_DELETE );
		Loc ( IDC_BM_ADD,		T_DLG_BM_NEW );
		Loc ( IDC_BM_EDIT,		T_DLG_BM_MOD );

		m_hList = Item ( IDC_BM_LIST );
		Bold ( IDC_TITLE );

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

		PopulateList ();

		SetResizer ( m_hList );
		AddStayer ( Item ( IDC_BM_UP ) );
		AddStayer ( Item ( IDC_BM_DOWN ) );
		AddStayer ( Item ( IDC_BM_EDIT ) );
		AddStayer ( Item ( IDC_BM_ADD ) );
		AddStayer ( Item ( IDC_BM_DELETE ) );
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Resizer_c::OnCommand ( iItem, iNotify );

		switch ( iItem )
		{
		case IDC_BM_DELETE:
			DeleteCurrent ();
			break;

		case IDC_BM_UP:
			MoveTo ( -1 );
			break;

		case IDC_BM_DOWN:
			MoveTo ( 1 );
			break;

		case IDC_BM_ADD:
			{
				ModifyBMDlg_c Dlg ( L"", L"", true );
				if ( Dlg.Run ( IDD_BM_MOD, m_hWnd ) == IDOK )
				{
					g_tRecent.AddBookmark ( Dlg.Name (), Dlg.Path () );
					PopulateList ();
				}
			}
			break;

		case IDC_BM_EDIT:
			{
				int iSelected = ListView_GetSelectionMark ( m_hList );
				if ( iSelected != -1 )
				{
					const RecentBookmark_t & Bookmark = g_tRecent.GetBookmark ( iSelected );
					ModifyBMDlg_c Dlg ( Bookmark.m_sName, Bookmark.m_sPath, false );
					if ( Dlg.Run ( IDD_BM_MOD, m_hWnd ) == IDOK )
					{
						g_tRecent.SetBookmarkName ( iSelected, Dlg.Name () );
						g_tRecent.SetBookmarkPath ( iSelected, Dlg.Path () );
						PopulateList ();
					}
				}
			}
			break;

		case IDOK:
			Close ( IDOK );
			break;
		}

	}

	virtual void OnContextMenu ( int iX, int iY )
	{
		HMENU hMenu = CreatePopupMenu ();

		if ( ListView_GetSelectionMark ( m_hList ) == -1 )
			AppendMenu ( hMenu, MF_STRING, IDC_BM_ADD,		Txt ( T_DLG_BM_NEW) );
		else
		{
			AppendMenu ( hMenu, MF_STRING, IDC_BM_EDIT,		Txt ( T_DLG_BM_MOD ) );
			AppendMenu ( hMenu, MF_STRING, IDC_BM_DELETE,	Txt ( T_DLG_BM_DELETE ) );

			if ( ListView_GetItemCount ( m_hList ) > 1 )
			{
				AppendMenu ( hMenu, MF_STRING, IDC_BM_UP,	Txt ( T_DLG_BM_UP ) );
				AppendMenu ( hMenu, MF_STRING, IDC_BM_DOWN, Txt ( T_DLG_BM_DOWN ) );
			}
		}

		int iRes = TrackPopupMenu ( hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, iX, iY, 0, m_hWnd, NULL );
		DestroyMenu ( hMenu );

		if ( iRes )
			OnCommand ( iRes, 0 );
	}

private:
	HWND	m_hList;

	void PopulateList ()
	{
		ListView_DeleteAllItems ( m_hList );

		LVITEM tItem;
		ZeroMemory ( &tItem, sizeof ( tItem ) );
		tItem.mask = LVIF_TEXT;

		for ( int i = 0; i < g_tRecent.GetNumBookmarks (); ++i )
		{
			const RecentBookmark_t & tMark = g_tRecent.GetBookmark ( i );

			tItem.pszText = (wchar_t *) tMark.m_sName.c_str ();
			tItem.iItem = i;
			int iItem = ListView_InsertItem ( m_hList, &tItem );
			ListView_SetItemText( m_hList, iItem, 1, (wchar_t *) tMark.m_sPath.c_str () );
		}

		RefreshButtons ();
	}

	void RefreshButtons ()
	{
		bool bMoreThanOne = ListView_GetItemCount ( m_hList ) > 1;
		EnableWindow ( Item ( IDC_BM_UP ), bMoreThanOne ? TRUE : FALSE );
		EnableWindow ( Item ( IDC_BM_DOWN ), bMoreThanOne ? TRUE : FALSE );
	}

	void DeleteCurrent ()
	{
		int nTotal = ListView_GetItemCount ( m_hList );
		int iSelected = ListView_GetSelectionMark ( m_hList );
		if ( iSelected != -1 )
		{
			g_tRecent.DeleteBookmark ( iSelected );
			ListView_DeleteItem ( m_hList, iSelected );
			if ( iSelected != nTotal - 1 )
			{
				ListView_SetItemState ( m_hList, iSelected, LVIS_SELECTED, LVIS_SELECTED );
				ListView_SetSelectionMark ( m_hList, iSelected );
			}
			else
				ListView_SetSelectionMark ( m_hList, -1 );
		}
	}

	void MoveTo ( int iOffset )
	{
		int iSelected = ListView_GetSelectionMark ( m_hList );
		int nTotal = ListView_GetItemCount ( m_hList );
		if ( iSelected + iOffset >= 0 && iSelected + iOffset < nTotal )
		{
			g_tRecent.MoveBookmark ( iSelected, iSelected + iOffset );
			PopulateList ();
			ListView_SetItemState ( m_hList, iSelected + iOffset, LVIS_SELECTED, LVIS_SELECTED );
			ListView_SetSelectionMark ( m_hList, iSelected + iOffset );
		}
	}

	void OnSettingsChange ()
	{
		ListView_SetColumnWidth ( m_hList, 0, GetColumnWidthRelative ( 0.4f ) );
		ListView_SetColumnWidth ( m_hList, 1, GetColumnWidthRelative ( 0.6f ) );

		Dialog_Resizer_c::OnSettingsChange ();
	}
};

// add a bookmark
bool ShowAddBMDialog ( const Str_c & sDir )
{
	AddBMDlg_c AddBMDlg ( sDir );
	return AddBMDlg.Run ( IDD_BM_ADD, g_hMainWindow ) == IDOK;
}

// edit your bookmarks
void ShowEditBMDialog ( HWND hParent )
{
	EditBMDlg_c EditBMDlg;
	EditBMDlg.Run ( IDD_BM_EDIT, hParent );
}

void SetupInitialBookmarks ()
{
	static wchar_t szDocuments [MAX_PATH];
	Verify ( SHGetDocumentsFolder ( L"\\", szDocuments ) );
	Str_c sMyDocs ( szDocuments );
	Str_c sPath ( szDocuments );
	CutToSlash ( sMyDocs );

	AppendSlash ( sPath );

	g_tRecent.AddBookmark ( sMyDocs, sPath );
}