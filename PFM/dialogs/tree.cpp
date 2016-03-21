#include "pch.h"
#include "tree.h"

#include "pfm/resources.h"
#include "pfm/gui.h"
#include "pfm/config.h"
#include "pfm/iterator.h"

#include "pfm/dialogs/errors.h"
#include "LDialogs/doptions.h"

#include "shellapi.h"
#include "Dlls/Resource/resource.h"

extern HWND g_hMainWindow;
extern HINSTANCE g_hAppInstance;

DirTree_c::DirTree_c ()
	: m_hTree			( NULL )
	, m_bImageListSet	( false )
	, m_sPath			( L"\\" )
{
	m_Sorter.SetVisibility ( cfg->pane_show_hidden, cfg->pane_show_system, cfg->pane_show_rom );
}

DirTree_c::~DirTree_c ()
{
	for ( int i = 0; i < m_dAllocated.Length (); ++i )
		delete m_dAllocated [i];
}

void DirTree_c::Create ( HWND hParent )
{
	DWORD dwStyle = WS_VISIBLE | WS_CHILD | TVS_DISABLEDRAGDROP  | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_NOTOOLTIPS | TVS_SHOWSELALWAYS;

	m_hTree = CreateWindowEx ( 0, WC_TREEVIEW, L"", dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			    hParent, NULL, g_hAppInstance, NULL );

	SendMessage ( m_hTree, WM_SETFONT, (WPARAM)g_tResources.m_hSystemFont, TRUE );
}

void DirTree_c::OnExpand ( TV_ITEM & tItem )
{
	if ( ! tItem.lParam )
		return;

	TreeItemInfo_t * pInfo = ( TreeItemInfo_t * ) tItem.lParam;
	if ( ! pInfo->m_bChildrenLoaded )
	{
		PopulateTreeLevel ( pInfo->m_sPath, tItem.hItem );
		pInfo->m_bChildrenLoaded = true;
	}
}

void DirTree_c::OnSelect ( TV_ITEM & tItem )
{
	if ( ! tItem.lParam )
		return;

	TreeItemInfo_t * pInfo = ( TreeItemInfo_t * ) tItem.lParam;
	m_sPath = pInfo->m_sPath;

	Event_Selected ();
}

void DirTree_c::Populate ( const Str_c & sPath )
{
	// add a root level
	TV_INSERTSTRUCT tInsert;
	memset ( &tInsert, 0, sizeof ( tInsert ) );
	tInsert.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;

	TreeItemInfo_t * pItemInfo =  new TreeItemInfo_t;
	m_dAllocated.Add ( pItemInfo );
	pItemInfo->m_sPath = L"\\";

	SHFILEINFO tShFileInfo;
	tShFileInfo.iIcon = -1;
	SHGetFileInfo ( L" ", FILE_ATTRIBUTE_DIRECTORY, &tShFileInfo, sizeof (SHFILEINFO), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON );

	tInsert.item.iImage = tShFileInfo.iIcon;
	tInsert.item.iSelectedImage = tShFileInfo.iIcon;
	tInsert.item.pszText = L"\\";
	tInsert.item.lParam = (LPARAM) pItemInfo;
	tInsert.item.cChildren = HasSubDirs ( pItemInfo->m_sPath ) ? 1 : 0;
	HTREEITEM hRoot = TreeView_InsertItem ( m_hTree, & tInsert );

 	TreeView_Expand ( m_hTree, hRoot, TVE_EXPAND );
	
	ExpandPath ( sPath );
}

void DirTree_c::ExpandPath ( const Str_c & sPath )
{
	int iOffset = 0;
	int iPos = 0;
	Str_c sSourceDir = sPath;
	Str_c sLookupDir;

	AppendSlash ( sSourceDir );

	HTREEITEM hParent = NULL;

	while ( ( iPos = sSourceDir.Find ( L'\\', iOffset ) ) != -1 )
	{
		sLookupDir = sSourceDir.SubStr ( 0, iPos + 1 );
		if ( sLookupDir.Length () > 1 )
			RemoveSlash ( sLookupDir );

		HTREEITEM hNewParent = FindDir ( hParent, sLookupDir );
		if ( ! hNewParent )
		{
			if ( hParent )
				TreeView_SelectItem ( m_hTree, hParent );
			break;
		}

		hParent = hNewParent;

		TreeView_Expand ( m_hTree, hParent, TVE_EXPAND );
		iOffset = iPos + 1;
	}

	if ( hParent )
		TreeView_SelectItem ( m_hTree, hParent );
}

HTREEITEM DirTree_c::FindDir ( HTREEITEM hParent, const Str_c & sPath )
{
	if ( sPath.Empty () )
		return NULL;

	TVITEM tItem;
	tItem.mask = TVIF_PARAM | TVIF_HANDLE;

	HTREEITEM hChild = TreeView_GetChild ( m_hTree, hParent );
	while ( hChild )
	{
		tItem.hItem = hChild;
		if ( ! TreeView_GetItem ( m_hTree, &tItem ) )
			continue;
		
		if ( ! tItem.lParam )
			continue;

		TreeItemInfo_t * pItemInfo = (TreeItemInfo_t *) tItem.lParam;
		if ( pItemInfo->m_sPath == sPath )
			return hChild;

		hChild = TreeView_GetNextSibling ( m_hTree, hChild );
	}

	return NULL;
}

void DirTree_c::PopulateTreeLevel ( const Str_c & sPath, HTREEITEM hParent )
{
	m_Sorter.SetDirectory ( sPath );
	m_Sorter.SetSortMode ( SORT_NAME, false );
	m_Sorter.Refresh ();

	if ( ! m_bImageListSet )
	{
		TreeView_SetImageList ( m_hTree, g_tResources.m_hSmallIconList, TVSIL_NORMAL );
		m_bImageListSet = true;
	}
	
	HTREEITEM hInsertAfter = NULL;

	TV_INSERTSTRUCT tInsert;
	tInsert.hParent = hParent;
	tInsert.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;

	for ( int i = 0; i < m_Sorter.GetNumItems (); ++i )
	{
		const PanelItem_t & tInfo = m_Sorter.GetItem ( i );
		bool bPrevDir = !wcscmp ( tInfo.m_FindData.cFileName, L".." );
		if ( tInfo.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY && !bPrevDir )
		{
			TreeItemInfo_t * pItemInfo =  new TreeItemInfo_t;
			m_dAllocated.Add ( pItemInfo );
			pItemInfo->m_sPath = sPath;
			AppendSlash ( pItemInfo->m_sPath );
			pItemInfo->m_sPath = pItemInfo->m_sPath + tInfo.m_FindData.cFileName;
        
			int iIcon = m_Sorter.GetItemIcon ( i );

			tInsert.hInsertAfter = hInsertAfter;
			tInsert.item.iImage = iIcon;
			tInsert.item.iSelectedImage = iIcon;
			tInsert.item.pszText = (wchar_t *)tInfo.m_FindData.cFileName;
			tInsert.item.lParam = (LPARAM) pItemInfo;
			tInsert.item.cChildren = HasSubDirs ( pItemInfo->m_sPath ) ? 1 : 0;

			TreeView_InsertItem ( m_hTree, & tInsert );
		}
	}
}

void DirTree_c::EnsureVisible ()
{
	HTREEITEM hSelected = TreeView_GetSelection ( m_hTree );
	if ( hSelected )
		TreeView_EnsureVisible ( m_hTree, hSelected );
}

HWND DirTree_c::Handle () const
{
	return m_hTree;
}

const Str_c & DirTree_c::Path () const
{
	return m_sPath;
}

//////////////////////////////////////////////////////////////////////////

FileTree_c::FileTree_c ( bool bAcceptDirs )
	: m_hList			( NULL )
	, m_bAcceptDirs		( bAcceptDirs )
	, m_eSortMode		( SORT_NAME )
{
}

void FileTree_c::Create ( HWND hParent )
{
	DirTree_c::Create ( hParent );

	DWORD dwStyle = WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_LIST | LVS_SHOWSELALWAYS | LVS_SINGLESEL | LVS_SHAREIMAGELISTS;// | LVS_EX_TWOCLICKACTIVATE ;
	m_hList = CreateWindowEx ( 0, WC_LISTVIEW, L"", dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hParent, NULL, g_hAppInstance, NULL );
	ListView_SetImageList ( m_hList, g_tResources.m_hSmallIconList, LVSIL_SMALL );
	SendMessage ( m_hList, WM_SETFONT, (WPARAM)g_tResources.m_hSystemFont, TRUE );
}

void FileTree_c::Destroy ()
{
	if ( m_hList )
	{
		ListView_SetImageList ( m_hList, NULL, LVSIL_SMALL );
		DestroyWindow ( m_hList );
	}
}

HWND FileTree_c::ListHandle ()
{
	return m_hList;
}

void FileTree_c::SetFilter ( const Str_c & sFilter )
{
	if ( m_sFilter != sFilter )
	{
		m_sFilter = sFilter;
		m_Filter.Set ( sFilter );
		PopulateList ();
	}
}

void FileTree_c::SetSortMode ( SortMode_e eSort )
{
	m_eSortMode = eSort;
}

void FileTree_c::OnGetDispInfo ( LV_DISPINFO * pInfo )
{
	pInfo->item.mask |= LVIF_DI_SETITEM;
	pInfo->item.iImage = m_Sorter.GetItemIcon ( pInfo->item.lParam );
}

void FileTree_c::OnItemChanged ( NMLISTVIEW * pInfo )
{
	if ( pInfo->iItem != -1 )
	{
		wchar_t szBuf [MAX_PATH];
		ListView_GetItemText ( m_hList, pInfo->iItem, 0, szBuf, MAX_PATH );
		m_sFilename = szBuf;
	}
	else
		m_sFilename = L"";
}

void FileTree_c::OnItemActivate ( NMLISTVIEW * pInfo )
{
	LVITEM Item;
	memset ( &Item, 0, sizeof ( Item ) );
	Item.mask = LVIF_PARAM;
	Item.iItem = pInfo->iItem;
	if ( ListView_GetItem ( m_hList, &Item ) )
	{
		const PanelItem_t & Info = m_Sorter.GetItem ( Item.lParam );
		if ( Info.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			ExpandPath ( m_Sorter.GetDirectory () + Info.m_FindData.cFileName );
	}
}

void FileTree_c::EnsureVisibleList ()
{
	int iSelected = ListView_GetSelectionMark ( m_hList );
	if ( iSelected != -1 )
		ListView_EnsureVisible ( m_hList, iSelected, FALSE );
}

bool FileTree_c::HaveSelectedFile () const
{
	return ListView_GetSelectionMark ( m_hList ) != -1;
}

bool FileTree_c::HaveSelectedDir () const
{
	return !!TreeView_GetSelection ( Handle () );
}

const Str_c & FileTree_c::Filename () const
{
	return m_sFilename;
}

void FileTree_c::Event_Selected ()
{
	PopulateList ();
}

void FileTree_c::PopulateList ()
{
	ListView_DeleteAllItems ( m_hList );
	m_Sorter.SetDirectory ( Path () );
	m_Sorter.SetSortMode ( m_eSortMode, false );
	m_Sorter.Refresh ();

	LVITEM Item;
	memset ( &Item, 0, sizeof ( Item ) );
	Item.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
	Item.iImage = I_IMAGECALLBACK;
	int nAdded = 0;

	for ( int i = 0; i < m_Sorter.GetNumItems (); ++i )
	{
		const PanelItem_t & File = m_Sorter.GetItem ( i );
		
		if ( !wcscmp ( File.m_FindData.cFileName, L".." ) )
			continue;

		if ( ( File.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && ! m_bAcceptDirs )
			continue;

		if ( ! m_Filter.Fits ( File.m_FindData.cFileName ) )
			continue;

		Item.iItem = nAdded++;
		Item.pszText = (wchar_t *)m_Sorter.GetItem ( i ).m_FindData.cFileName;
		Item.lParam = i;

		ListView_InsertItem ( m_hList, &Item );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
class SelectDirDlg_c : public Dialog_Resizer_c
{
public:
	SelectDirDlg_c  ( const Str_c & sPath )
		: Dialog_Resizer_c ( L"Main_Contents", IDM_OK_CANCEL )
		, m_sPath ( sPath )
	{
	}

	virtual void OnInit ()
	{
		Dialog_Resizer_c::OnInit ();

		m_DirTree.Create ( m_hWnd );
		m_DirTree.Populate ( m_sPath );
		SetResizer ( m_DirTree.Handle () );
	}

	virtual bool OnNotify ( int iControlId, NMHDR * pInfo )
	{
		Dialog_Resizer_c::OnNotify ( iControlId, pInfo );

		switch ( pInfo->code )
		{
		case TVN_SELCHANGED:
			m_DirTree.OnSelect ( ( (NM_TREEVIEW *) pInfo)->itemNew );
			return true;
		case TVN_ITEMEXPANDING:
			m_DirTree.OnExpand ( ( (NM_TREEVIEW *) pInfo)->itemNew );
			return true;
		}

		return false;
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Resizer_c::OnCommand ( iItem, iNotify );
		Close ( iItem );
	}

	const Str_c & Path () const
	{
		return m_DirTree.Path ();
	}

protected:
	DirTree_c	m_DirTree;
	Str_c		m_sPath;
};

//////////////////////////////////////////////////////////////////////////
void UpdateDlgPositions ();

class TreeSplitter_c : public SplitterBar_c
{
protected:
	void Event_SplitterMoved ()
	{
		UpdateDlgPositions ();
	}
};

class SelectFileDlg_c : public Dialog_Resizer_c
{
public:
	SelectFileDlg_c ( const Str_c & sPath, bool bAcceptFolders, const TreeFilters_t & TreeFilters )
		: Dialog_Resizer_c ( L"Main_Contents", IDM_OK_CANCEL )
		, m_sPath			( sPath )
		, m_FileTree		( bAcceptFolders )
		, m_dTreeFilters	( &TreeFilters )
		, m_bAcceptDirs		( bAcceptFolders )
	{
		Assert ( ! m_dTreeFilters->Empty () );
	}

	virtual void OnInit ()
	{
		Dialog_Resizer_c::OnInit ();

		Loc ( IDC_SORT_STATIC,	T_DLG_TREE_SORT );
		Loc ( IDC_TYPE_STATIC,	T_DLG_TREE_TYPE );

		m_FileTree.Create ( m_hWnd );
		m_FileTree.SetFilter ( (*m_dTreeFilters) [0].m_sFilter );
		m_FileTree.Populate ( m_sPath );
		m_Splitter.Create ( m_hWnd );

		for ( int i = 0; i < m_dTreeFilters->Length (); ++i )
			SendMessage ( Item ( IDC_TYPE ), CB_ADDSTRING, 0, (LPARAM)( (*m_dTreeFilters) [i].m_sName.c_str () ) );

		SendMessage ( Item ( IDC_TYPE ), CB_SETCURSEL, 0, 0 );

		InitSortList ( m_hWnd, IDC_SORT );
		SendMessage ( Item ( IDC_SORT ), CB_SETCURSEL, 1, 0 );
	}

	virtual void OnSettingsChange ()
	{
		Dialog_Resizer_c::OnSettingsChange ();
		CustomResize ();
	}

	virtual void UpdatePositions ()
	{
		RECT ClientRect;
		GetClientRect ( m_hWnd, &ClientRect );

		RECT SplitterRect;
		m_Splitter.GetRect ( SplitterRect );

		RECT TypeRect;
		GetWindowRect ( Item ( IDC_TYPE ), &TypeRect );

		HWND hTree = m_FileTree.Handle ();
		SetWindowPos ( hTree, NULL, ClientRect.left, ClientRect.top, ClientRect.right - ClientRect.left, SplitterRect.top - ClientRect.top, SWP_SHOWWINDOW );

		HWND hList = m_FileTree.ListHandle ();
		SetWindowPos ( hList, NULL, ClientRect.left, SplitterRect.bottom, ClientRect.right - ClientRect.left, ClientRect.bottom - SplitterRect.bottom - (TypeRect.bottom - TypeRect.top + 2) * 2, SWP_SHOWWINDOW );

		m_FileTree.EnsureVisible ();
		m_FileTree.EnsureVisibleList ();
	}

	virtual void OnClose ()
	{
		m_FileTree.Destroy ();
	}

	const Str_c & Path () const
	{
		return m_sPath;
	}

protected:
	virtual bool OnNotify ( int iControlId, NMHDR * pInfo )
	{
		Dialog_Resizer_c::OnNotify ( iControlId, pInfo );

		switch ( pInfo->code )
		{
		case TVN_SELCHANGED:
			m_FileTree.OnSelect ( ( (NM_TREEVIEW *) pInfo)->itemNew );
			return true;
		case TVN_ITEMEXPANDING:
			m_FileTree.OnExpand ( ( (NM_TREEVIEW *) pInfo)->itemNew );
			return true;
		case LVN_GETDISPINFO:
			m_FileTree.OnGetDispInfo ( (LV_DISPINFO*) pInfo );
			return true;
		case LVN_ITEMCHANGED:
			m_FileTree.OnItemChanged ( (NMLISTVIEW *) pInfo );
			return true;
		case LVN_ITEMACTIVATE:
			m_FileTree.OnItemActivate ( (NMLISTVIEW *) pInfo );
			break;
		}

		return false;
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Resizer_c::OnCommand ( iItem, iNotify );

		switch ( iItem )
		{
		case IDOK:
			if ( m_FileTree.HaveSelectedFile () )
			{
				m_sPath = m_FileTree.Path ();
				AppendSlash ( m_sPath );
				m_sPath += m_FileTree.Filename ();
				Close ( iItem );
			}
			else
			{
				if ( m_bAcceptDirs && m_FileTree.HaveSelectedDir () )
				{
					m_sPath = m_FileTree.Path ();
					Close ( iItem );
				}
				else
					ShowErrorDialog ( m_hWnd, false, Txt ( T_DLG_TREE_NOFILE ), IDD_ERROR_OK );
			}
			break;
		case IDCANCEL:
			Close ( iItem );
			break;
		case IDC_TYPE:
			if ( iNotify == CBN_SELENDOK )
			{
				int iSel = SendMessage ( Item ( IDC_TYPE ), CB_GETCURSEL, 0, 0 );
				if ( iSel != CB_ERR )
					m_FileTree.SetFilter ( (*m_dTreeFilters) [iSel].m_sFilter );
			}
			break;
		case IDC_SORT:
			if ( iNotify == CBN_SELENDOK )
			{
				int iSel = SendMessage ( Item ( IDC_SORT ), CB_GETCURSEL, 0, 0 );
				if ( iSel != CB_ERR )
				{
					m_FileTree.SetSortMode ( SortMode_e ( iSel ) );
					m_FileTree.PopulateList ();
				}
			}
			break;
		}
	}

private:
	Str_c			m_sPath;
	bool			m_bAcceptDirs;
	TreeSplitter_c	m_Splitter;
	FileTree_c		m_FileTree;
	const TreeFilters_t * m_dTreeFilters;

	void CustomResize ()
	{
		RECT TypeRect;
		GetWindowRect ( Item ( IDC_TYPE ), &TypeRect );

		RECT ClientRect;
		GetClientRect ( m_hWnd, &ClientRect );
		const int iSplitterHeight = 5 * GetVGAScale ();
		RECT tSplitterRect = ClientRect;
		tSplitterRect.top = ( ClientRect.bottom - ClientRect.top - iSplitterHeight - ( TypeRect.bottom - TypeRect.top + 2 ) * 2 ) / 2;
		tSplitterRect.bottom = tSplitterRect.top + iSplitterHeight;
		m_Splitter.SetRect ( tSplitterRect );
		m_Splitter.SetMinMax ( ClientRect.top, ClientRect.bottom - ( TypeRect.bottom - TypeRect.top + 2 ) * 2 );

		UpdatePositions ();

		RECT ListRect;
		GetWindowRect ( m_FileTree.ListHandle (), &ListRect );

		POINT ListBottom = { ListRect.right, ListRect.bottom };
		ScreenToClient ( m_hWnd, &ListBottom );
		++ListBottom.y;

		int iOffsetX = 6 * GetVGAScale ();
		SetWindowPos ( Item ( IDC_TYPE_STATIC ), NULL, ClientRect.left + iOffsetX, ListBottom.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW );

		RECT TypeStaticRect;
		GetWindowRect ( Item ( IDC_TYPE_STATIC ), &TypeStaticRect );
		POINT TypeRight = { TypeStaticRect.right, TypeStaticRect.bottom };
		ScreenToClient ( m_hWnd, &TypeRight );

		SetWindowPos ( Item ( IDC_TYPE ), NULL, TypeRight.x, ListBottom.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW );

		SetWindowPos ( Item ( IDC_SORT_STATIC ), NULL, ClientRect.left + iOffsetX, TypeRight.y + 2, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW );
		SetWindowPos ( Item ( IDC_SORT ), NULL, TypeRight.x, TypeRight.y + 2, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW );
	}
};

SelectFileDlg_c * g_pSelectFileDlg = NULL;

void UpdateDlgPositions ()
{
	if ( g_pSelectFileDlg )
		g_pSelectFileDlg->UpdatePositions ();
}

///////////////////////////////////////////////////////////////////////////////////////////
bool DirTreeDlg ( HWND hParent, Str_c & sPath )
{
	SelectDirDlg_c Dlg ( sPath );
	if ( Dlg.Run ( IDD_FILE_TREE, hParent ) == IDOK )
	{
		sPath = Dlg.Path ();
		AppendSlash ( sPath );
		return true;
	}
	
	return false;
}

bool FileTreeDlg ( HWND hParent, Str_c & sPath, bool bAcceptFolders, const TreeFilters_t * dFilters )
{
	TreeFilters_t dAllFilter;
	if ( ! dFilters  )
		dAllFilter.Add ( TreeFilterEntry_t ( Txt ( T_FILTER_ALL ), L"" ) );

	SelectFileDlg_c Dlg ( sPath, bAcceptFolders, dFilters ? *dFilters : dAllFilter );
	g_pSelectFileDlg = &Dlg;
	if ( Dlg.Run ( IDD_FILE_TREE, hParent ) == IDOK )
	{
		sPath = Dlg.Path ();
		RemoveSlash ( sPath );
		DWORD dwAttrib = GetFileAttributes ( sPath );
		if ( dwAttrib != 0xFFFFFFFF && ( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) )
			AppendSlash ( sPath );

		g_pSelectFileDlg = NULL;
		return true;
	}

	g_pSelectFileDlg = NULL;
	return false;
}