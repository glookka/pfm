#include "pch.h"
#include "filelist.h"

#include "config.h"
#include "parser.h"
#include "plugins.h"

#include "projects.h"

//////////////////////////////////////////////////////////////////////////
// color group stuff
ColorGroup_t::ColorGroup_t ()
	: m_dwIncludeAttributes ( 0 )
	, m_dwExcludeAttributes	( 0 )
	, m_uColor				( 0 )
	, m_uSelectedColor		( 0 )
	, m_id					( 0 )
{
}


ColorGroup_t g_CG;
Array_T <ColorGroup_t>  g_dColorGroups;

CMD_BEGIN ( cg_start, 0 )
{
	g_CG = ColorGroup_t ();
	g_CG.m_id = g_dColorGroups.Length ();
}
CMD_END

CMD_BEGIN ( cg_attributes, 2 )
{
	g_CG.m_dwIncludeAttributes = tArgs.GetDWORD ( 0 );
	g_CG.m_dwExcludeAttributes = tArgs.GetDWORD ( 1 );
}
CMD_END

CMD_BEGIN ( cg_ext_mask, 1 )
{
	g_CG.m_tFilter.Set ( tArgs.GetString ( 0 ) );
}
CMD_END

CMD_BEGIN ( cg_color, 2 )
{
	g_CG.m_uColor 			= tArgs.GetDWORD ( 0 );
	g_CG.m_uSelectedColor 	= tArgs.GetDWORD ( 1 );
}
CMD_END

CMD_BEGIN ( cg_end, 0 )
{
	g_dColorGroups.Add ( g_CG );
}
CMD_END


void LoadColorGroups ( const wchar_t * szFileName )
{
	g_dColorGroups.Clear ();
	Commander_c::Get ()->ExecuteFile ( szFileName );
}


void RegisterColorGroupCommands ()
{
	CMD_REG ( cg_start );
	CMD_REG ( cg_attributes );
	CMD_REG ( cg_ext_mask );
	CMD_REG ( cg_color );
	CMD_REG ( cg_end );
}


const ColorGroup_t * GetColorGroup ( const PanelItem_t & Item, bool bAttrOnly )
{
	for ( int i = 0; i < g_dColorGroups.Length (); ++i )
	{
		const ColorGroup_t * pGroup = & ( g_dColorGroups [i] );
		Assert ( pGroup );

		bool bAttrib = ! pGroup->m_dwIncludeAttributes || ( Item.m_FindData.dwFileAttributes & pGroup->m_dwIncludeAttributes );
		bool bExAttrib = ! pGroup->m_dwExcludeAttributes || ( Item.m_FindData.dwFileAttributes & ~(pGroup->m_dwExcludeAttributes) );

		if ( bAttrib && bExAttrib )
		{
			if ( bAttrOnly )
			{
				if ( !pGroup->m_tFilter.HasMasks () )
					return pGroup;
			}
			else
				if ( pGroup->m_tFilter.Fits ( Item.m_FindData.cFileName ) ) 
					return pGroup;
		}
	}

	return NULL;
}


int CompareColorGroups ( const ColorGroup_t * pCG1, const ColorGroup_t * pCG2 )
{
	if ( pCG1 )
	{
		if ( ! pCG2 )
			return 1;
	}
	else
	{
		if ( pCG2 )
			return -1;
		else
			return 0;
	}

	Assert ( pCG1 && pCG2 );
	return pCG1->m_id < pCG2->m_id ? -1 : ( pCG1->m_id > pCG2->m_id ? 1 : 0 );
}


//////////////////////////////////////////////////////////////////////////
// storage card stuff
static Array_T <WIN32_FIND_DATA> g_dCards;

void RefreshStorageCards ()
{
	g_dCards.Clear ();

	WIN32_FIND_DATA tData;
	HANDLE hCard = FindFirstFlashCard ( &tData );

	if ( hCard != INVALID_HANDLE_VALUE )
	{
		do 
		{
			if ( tData.cFileName [0] != L'\0' )
				g_dCards.Add ( tData );
		}
		while ( FindNextFlashCard ( hCard, &tData ) );
	}
}

int GetNumStorageCards ()
{
	return g_dCards.Length ();
}

void GetStorageCardData ( int iCard, WIN32_FIND_DATA & tData )
{
	tData = g_dCards [iCard];
}

//////////////////////////////////////////////////////////////////////////
// item list
bool SelectedFileList_t::OneFile () const
{
	return m_dFiles.Length () == 1;
}


bool SelectedFileList_t::IsDir () const
{
	if ( m_dFiles.Empty () )
		return false;

	return !!( m_dFiles [0]->m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );
}


bool SelectedFileList_t::IsPrevDir () const
{
	if ( m_dFiles.Empty () )
		return false;

	return !wcscmp ( m_dFiles [0]->m_FindData.cFileName, L".." );
}


///////////////////////////////////////////////////////////////////////////////////////////
// file sorter
int CompareFilenames ( const wchar_t * szFilename1, const wchar_t * szFilename2, bool bCompareExts )
{
	static wchar_t szName1 [MAX_PATH];
	static wchar_t szName2 [MAX_PATH];
	const wchar_t * szExt1 = GetNameExtFast ( szFilename1, szName1 );
	const wchar_t * szExt2 = GetNameExtFast ( szFilename2, szName2 );

	int iRes = _wcsicmp ( szName1, szName2 );

	if ( ! iRes && bCompareExts )
		iRes = _wcsicmp ( szExt1, szExt2 );

	return iRes;
}

// a hack for property-based search
static int					g_iIndex1 = -1;
static int					g_iIndex2 = -1;
static ItemProperties_t	*	g_pProperites = NULL;

static int FileCompare ( const PanelItem_t & Item1, const PanelItem_t & Item2, SortMode_e eSortMode, bool bReverse )
{
	// comparison rules:
	// 1. directories always come first (except for unsorted mode)
	// 2. directories don't get sorted by ext/name, only by name+ext
	// 3. files w/o ext come first
	// 4. same ext -> sort by name
	// 5. same name -> sort by ext

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

	if ( bCompareDirs && ( eSortMode == SORT_EXTENSION || eSortMode == SORT_SIZE ) )
		eSortMode = SORT_NAME;

	switch ( eSortMode )
	{
	case SORT_NAME:
		iRes = CompareFilenames ( Item1.m_FindData.cFileName, Item2.m_FindData.cFileName, true );
		break;

	case SORT_EXTENSION:
		{
			const wchar_t * szExt1 = GetNameExtFast ( Item1.m_FindData.cFileName );
			const wchar_t * szExt2 = GetNameExtFast ( Item2.m_FindData.cFileName );

			iRes = _wcsicmp ( szExt1, szExt2 );	
			if ( !iRes )
				iRes = _wcsicmp ( Item1.m_FindData.cFileName, Item2.m_FindData.cFileName );
		}
		break;

	case SORT_SIZE:
		if ( Item1.m_FindData.nFileSizeHigh == Item2.m_FindData.nFileSizeHigh )
			iRes = Item1.m_FindData.nFileSizeLow < Item2.m_FindData.nFileSizeLow ? -1 : ( Item1.m_FindData.nFileSizeLow == Item2.m_FindData.nFileSizeLow ? 0 : 1 );
		else
			iRes = Item1.m_FindData.nFileSizeHigh < Item2.m_FindData.nFileSizeHigh ? -1 : ( Item1.m_FindData.nFileSizeHigh == Item2.m_FindData.nFileSizeHigh ? 0 : 1 );
		break;

	case SORT_SIZE_PACKED:
		iRes = Item1.m_uPackSize < Item2.m_uPackSize ? -1 : ( Item1.m_uPackSize == Item2.m_uPackSize ? 0 : 1 );
		break;

	case SORT_TIME_ACCESS:
		iRes = CompareFileTime ( &Item1.m_FindData.ftLastAccessTime, &Item2.m_FindData.ftLastAccessTime );
		break;

	case SORT_TIME_CREATE:
		iRes = CompareFileTime ( &Item1.m_FindData.ftCreationTime, &Item2.m_FindData.ftCreationTime );
		break;

	case SORT_TIME_WRITE:
		iRes = CompareFileTime ( &Item1.m_FindData.ftLastWriteTime, &Item2.m_FindData.ftLastWriteTime );
		break;

	case SORT_GROUP:
		Assert ( g_pProperites );
		iRes = CompareColorGroups ( g_pProperites [g_iIndex1].m_pCG, g_pProperites [g_iIndex2].m_pCG );
		break;
	}

	iRes = Clamp ( iRes, -1, 1 );

	if ( iRes == 0 && eSortMode >= SORT_SIZE )
		iRes = CompareFilenames ( Item1.m_FindData.cFileName, Item2.m_FindData.cFileName, false );

	return bReverse ? -iRes : iRes;
}

///////////////////////////////////////////////////////////////////////////////////////////
// a class for file sorting/storing
FileList_c::FileList_c ()
	: m_bSortReverse 	( false )
	, m_eSortMode		( SORT_UNSORTED )
	, m_sDir			( L"\\" )
	, m_nMarked			( 0 )
	, m_bShowHidden		( true )
	, m_bShowSystem		( true )
	, m_bShowROM		( true )
	, m_hOldCursor		( NULL )
	, m_bSlowRefresh	( false )
	, m_hActivePlugin	( INVALID_HANDLE_VALUE )
	, m_pSourceItems	( NULL )
	, m_nSourceItems	( 0 )
	, m_pPluginItems	( NULL )
	, m_nPluginItems	( 0 )
	, m_eCachedSortMode	( SORT_UNSORTED )
	, m_bCachedSortReverse ( false )
{
	m_MarkedSize.QuadPart = 0;
	m_dFiles.Reserve ( 128 );
}


FileList_c::~FileList_c ()
{
	CloseActivePlugin ();
}


void FileList_c::SetDirectory ( const wchar_t * szDir )
{
	m_sDir = szDir;

	if ( m_sDir.Empty () )
		m_sDir = L'\\';

	Str_c sNewPath = m_sDir;
	RemoveSlash ( sNewPath );

	int iUpPos = sNewPath.Find ( L"\\.." );
	while ( iUpPos >= 0 )
	{
		if ( iUpPos > 0 )
		{
			int iSlashPos = sNewPath.RFind ( '\\', iUpPos - 1 );
			if ( iSlashPos >= 0 )
				sNewPath = sNewPath.SubStr ( 0, iSlashPos ) + sNewPath.SubStr ( iUpPos + 3 );
			else
				sNewPath = sNewPath.SubStr ( iUpPos + 3 );
		}
		else
			sNewPath = sNewPath.SubStr ( iUpPos + 3 );

		iUpPos = sNewPath.Find ( L"\\.." );
	}

	m_sDir = sNewPath;
	AppendSlash ( m_sDir );

	// lets see if our current plugin can handle this path
	if ( m_hActivePlugin != INVALID_HANDLE_VALUE )
	{
		if ( !g_PluginManager.SetDirectory ( m_hActivePlugin, m_sDir ) )
			CloseActivePlugin ();
	}

	// try to spawn a new one
	if ( m_hActivePlugin == INVALID_HANDLE_VALUE )
	{
		m_hActivePlugin = g_PluginManager.OpenFilePlugin ( m_sDir );
		if ( m_hActivePlugin != INVALID_HANDLE_VALUE )
		{
			Event_PluginOpened ( m_hActivePlugin );
			if ( !g_PluginManager.SetDirectory ( m_hActivePlugin, m_sDir ) )
				CloseActivePlugin ();
		}
	}

	// still no plugin? well, maybe another day...
	if ( m_hActivePlugin == INVALID_HANDLE_VALUE )
	{
		PrependSlash ( m_sDir );
		m_sDir = StepUpToValid ( m_sDir );
	}
}


const Str_c & FileList_c::GetDirectory () const
{
	return m_sDir;
}


const PanelItem_t &	FileList_c::GetItem ( int iItem ) const
{
	Assert ( iItem >= 0 && iItem < m_nSourceItems );
	return m_pSourceItems [m_dSortedIndices[iItem]];
}


DWORD FileList_c::GetItemColor ( int iItem, bool bAtCursor )
{
	ItemProperties_t & Props = GetItemProperties ( iItem );

	if ( Props.m_uFlags & FILE_MARKED )
		return clrs->pane_font_marked;

	if ( bAtCursor )
		return Props.m_pCG ? Props.m_pCG->m_uSelectedColor : clrs->pane_font_selected;
	else
		return Props.m_pCG ? Props.m_pCG->m_uColor : clrs->pane_font;
}


int FileList_c::GetItemIcon ( int iItem )
{
	ItemProperties_t & Properties = GetItemProperties ( iItem );
	PanelItem_t & Item = m_pSourceItems [m_dSortedIndices[iItem]];

	if ( !( Properties.m_uFlags & FILE_ICON_CHECKED ) )
	{
		Properties.m_uFlags |= FILE_ICON_CHECKED;
		SHFILEINFO ShFileInfo;
		ShFileInfo.iIcon = -1;
		if ( Properties.m_uFlags & FILE_PREV_DIR )
			SHGetFileInfo ( L" ", FILE_ATTRIBUTE_DIRECTORY, &ShFileInfo, sizeof (SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES );
		else
			SHGetFileInfo ( m_sDir + Item.m_FindData.cFileName, 0, &ShFileInfo, sizeof (SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON );
		Item.m_iIcon = ShFileInfo.iIcon;
	}

	return Item.m_iIcon;
}


DWORD FileList_c::GetItemFlags ( int iItem )
{
	return GetItemProperties ( iItem ).m_uFlags;
}


ItemProperties_t & FileList_c::GetItemProperties ( int iItem )
{
	return m_dProperties [m_dSortedIndices [iItem]];
}


int FileList_c::GetNumItems () const
{
	return m_nSourceItems;
}


void FileList_c::RefreshBaseFS ()
{
	// FIXME: time-based
	const int LOTS_OF_FILES = 128;

	m_dFiles.Clear ();

	PanelItem_t FindInfo;
	memset ( &FindInfo, 0, sizeof ( FindInfo ) );
	FindInfo.m_iIcon = -1;

	HANDLE hFile;
	
	bool bRootDir = m_sDir == L"\\";

	// not the root?
	if ( !bRootDir )
	{
		Str_c sDirWithNoSlash = m_sDir;
		sDirWithNoSlash.Chop ( 1 );

		PanelItem_t FirstDir;
		HANDLE hFirstFile = FindFirstFile ( sDirWithNoSlash, &FirstDir.m_FindData );
		if ( hFirstFile != INVALID_HANDLE_VALUE )
			FindClose ( hFirstFile );
		else
		{
			memset ( &FirstDir, 0, sizeof ( FirstDir ) );
			FirstDir.m_iIcon = -1;
		}

		wcscpy ( FirstDir.m_FindData.cFileName, L".." );
		m_dFiles.Add ( FirstDir );
	}

	hFile = FindFirstFile ( m_sDir + L"*", &FindInfo.m_FindData );
	if ( hFile == INVALID_HANDLE_VALUE )
	{
		HideCursor ();
		return;
	}

	int nFiles = 0;

	do
	{
		if ( ( ! ( FindInfo.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ) || m_bShowHidden ) &&
			( ! ( FindInfo.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ) || m_bShowSystem ) &&
			( ! ( FindInfo.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_INROM )  || m_bShowROM ) )
		{
			// see if its a storage card
			if ( bRootDir )
			{
				WIN32_FIND_DATA tData;
				int nStorageCards = GetNumStorageCards ();
				for ( int i = 0; i < nStorageCards; ++i )
				{
					GetStorageCardData ( i, tData );
					if ( !wcscmp ( tData.cFileName, FindInfo.m_FindData.cFileName ) )
					{
						FindInfo.m_FindData.dwFileAttributes |= FILE_ATTRIBUTE_CARD;
						break;
					}
				}
			}
			FindInfo.m_uPackSize = FindInfo.m_FindData.nFileSizeHigh ? 0xFFFFFFFF : FindInfo.m_FindData.nFileSizeLow;
			m_dFiles.Add ( FindInfo );

			++nFiles;
			if ( nFiles == LOTS_OF_FILES )
			{
				m_hOldCursor	= SetCursor ( LoadCursor ( NULL, IDC_WAIT ) );
				m_bSlowRefresh	= true;
			}
		}
	}
	while ( FindNextFile ( hFile, &FindInfo.m_FindData ) );

	FindClose ( hFile );
}


void FileList_c::UpdateItemProperties ()
{
	m_dProperties.SetLength ( m_nSourceItems );

	bool bUseHighlighting = true;
	bool bHighlightByAttr = false;
	bool bPluginActive = m_hActivePlugin != INVALID_HANDLE_VALUE;

	if ( bPluginActive )
	{
		const PluginInfo_t * pPluginInfo = g_PluginManager.GetPluginInfo ( m_hActivePlugin );
		DWORD uFlags = pPluginInfo ? pPluginInfo->m_uFlags : 0;
		if ( !( uFlags & OPIF_USEHIGHLIGHTING ) )
			bUseHighlighting = false;

		if ( uFlags & OPIF_USEATTRHIGHLIGHTING )
			bHighlightByAttr = true;
	}

	for (  int i = 0; i < GetNumItems (); ++i )
	{
		const PanelItem_t & Item = m_pSourceItems [i];
		ItemProperties_t  & Properties = m_dProperties [i];

		Properties.m_uFlags = WORD ( bPluginActive ? FILE_ICON_CHECKED : 0 ); 

		if ( !i && !wcscmp ( Item.m_FindData.cFileName, L".." ) )
			Properties.m_uFlags |= FILE_PREV_DIR;

		Properties.m_pCG = bUseHighlighting ? GetColorGroup ( Item, bHighlightByAttr ) : NULL;
	}
}

static ItemCompare_t	g_fnCompareItems = NULL;
static HANDLE			g_hActivePlugin = INVALID_HANDLE_VALUE;
static PanelItem_t *	g_pItems		= NULL;
static SortMode_e		g_eSortMode		= SORT_UNDEFINDED;
static bool				g_bSortReverse	= false;

static int ItemCompare ( const void * iIndex1, const void * iIndex2 )
{
	// a hack for property-based searches
	g_iIndex1 = *(int*)iIndex1;
	g_iIndex2 = *(int*)iIndex2;

	int iRes = -2;
	if ( g_fnCompareItems )
		iRes = g_fnCompareItems ( g_hActivePlugin, g_pItems [g_iIndex1], g_pItems [g_iIndex2], g_eSortMode, g_bSortReverse );

	if ( iRes == -2 )
		iRes = FileCompare ( g_pItems [g_iIndex1], g_pItems [g_iIndex2], g_eSortMode, g_bSortReverse );

	return iRes;
}


void FileList_c::SortItems ()
{
	m_dSortedIndices.SetLength ( m_nSourceItems );
	for ( int i = 0; i < m_nSourceItems; ++i )
		m_dSortedIndices [i] = i;

	if ( m_eSortMode != SORT_UNSORTED && GetNumItems () > 0 )
	{
		int nItemsToSort = m_nSourceItems;
		int * pItemsToSort = &(m_dSortedIndices [0]);

		g_fnCompareItems = NULL;
		g_hActivePlugin  = m_hActivePlugin;
		g_eSortMode		 = m_eSortMode;
		g_bSortReverse	 = m_bSortReverse;
		g_pItems		 = m_pSourceItems;
		g_pProperites	 = &(m_dProperties [0]);

		// exclude '..'
		if ( m_nSourceItems > 0 && m_dProperties [0].m_uFlags & FILE_PREV_DIR )
		{
			pItemsToSort++;
			nItemsToSort--;
		}

		if ( nItemsToSort > 0 )
		{
			if ( m_hActivePlugin != INVALID_HANDLE_VALUE )
				g_fnCompareItems = g_PluginManager.GetCompareFunc ( m_hActivePlugin );

			qsort ( pItemsToSort, nItemsToSort, sizeof ( m_dSortedIndices [0] ), ItemCompare );
		}
	}
}


void FileList_c::Refresh ()
{
	m_nMarked				= 0;
	m_MarkedSize.HighPart	= 0;
	m_MarkedSize.LowPart	= 0;

	m_dProperties.Clear ();
	m_dSortedIndices.Clear ();


	if ( m_hActivePlugin != INVALID_HANDLE_VALUE )
	{
		g_PluginManager.FreeFindData ( m_hActivePlugin, m_pPluginItems, m_nPluginItems );
		m_pPluginItems = NULL;
		m_nPluginItems = 0;

		if ( !g_PluginManager.GetFindData ( m_hActivePlugin, m_pPluginItems, m_nPluginItems ) )
		{
			CloseActivePlugin ();
			m_sDir = StepUpToValid ( m_sDir );
		}
	}

	if ( m_hActivePlugin == INVALID_HANDLE_VALUE )
	{
		RefreshBaseFS ();
		m_pSourceItems = m_dFiles.Data ();
		m_nSourceItems = m_dFiles.Length ();
	}
	else
	{
		m_pSourceItems = m_pPluginItems;
		m_nSourceItems = m_nPluginItems;
	}

	UpdateItemProperties ();

	SortItems ();

	HideCursor ();
}


void FileList_c::SoftRefresh ()
{
	struct StoredName_t
	{
		wchar_t m_szPath [MAX_PATH];
	};

	Array_T <StoredName_t> dStored;
	bool bAllocated = false;

	for ( int i = 0; i < GetNumItems (); ++i )
	{
		const ItemProperties_t & Properties = GetItemProperties ( i );
		const PanelItem_t & Item = GetItem ( i );

		if ( !! ( Properties.m_uFlags & FILE_MARKED ) && ! ( Properties.m_uFlags & FILE_PREV_DIR ) )
		{
			if ( !bAllocated )
			{
				dStored.Reserve ( GetNumItems () );
				bAllocated = true;
			}

			dStored.Grow ( dStored.Length () + 1 );
			wcscpy ( dStored.Last().m_szPath, Item.m_FindData.cFileName );
		}
	}

	Refresh ();

	for ( int i = 0; i < dStored.Length (); ++i )
		MarkFileByName ( dStored [i].m_szPath, true );
}


SortMode_e FileList_c::GetSortMode () const
{
	return m_eSortMode;
}


bool FileList_c::GetSortReverse () const
{
	return m_bSortReverse;
}


HANDLE FileList_c::GetActivePlugin () const
{
	return m_hActivePlugin;
}


bool FileList_c::StepToPrevDir ( Str_c * pPrevDirName )
{
	int iDirLen = m_sDir.Length ();
	if ( iDirLen > 1 )
	{
		int iIndex = m_sDir.RFind ( L'\\', iDirLen - 2 );
		if ( iIndex != -1 )
		{
			if ( pPrevDirName )
				*pPrevDirName = m_sDir.SubStr ( iIndex + 1, iDirLen  - iIndex - 2 );

			SetDirectory ( m_sDir.SubStr ( 0, iIndex + 1 ) );
			return true;
		}
	}

	return false;
}


bool FileList_c::MarkFileById ( int iItem, bool bSelect )
{
	ItemProperties_t & Props = GetItemProperties ( iItem );
	const PanelItem_t & Item = GetItem ( iItem );

	if ( Props.m_uFlags & FILE_PREV_DIR )
		return false;

	bool bWasMarked = !! ( Props.m_uFlags & FILE_MARKED );
	ULARGE_INTEGER tSize;
	tSize.HighPart	= Item.m_FindData.nFileSizeHigh;
	tSize.LowPart	= Item.m_FindData.nFileSizeLow;

	if ( bWasMarked )
	{
		if ( !bSelect )
		{
			--m_nMarked;
			m_MarkedSize.QuadPart -= tSize.QuadPart;
			Props.m_uFlags &= ~FILE_MARKED;
			return true;
		}
	}
	else
	{
		if ( bSelect )
		{
			++m_nMarked;
			m_MarkedSize.QuadPart += tSize.QuadPart;
			Props.m_uFlags |= FILE_MARKED;
			return true;
		}
	}

	return false;
}


bool FileList_c::MarkFileByName ( const wchar_t * szName, bool bSelect )
{
	for ( int i = 0; i < GetNumItems (); ++i )
		if ( !wcscmp ( GetItem ( i ).m_FindData.cFileName, szName ) )
			return MarkFileById ( i, bSelect );

	return false;
}


int FileList_c::GetNumMarked () const
{
	return m_nMarked;
}


ULARGE_INTEGER FileList_c::GetMarkedSize () const
{
	return m_MarkedSize;
}


void FileList_c::SetSortMode ( SortMode_e eMode, bool bSortReverse )
{
	m_eSortMode = eMode;
	m_bSortReverse = bSortReverse;
}


void FileList_c::SetVisibility ( bool bShowHidden, bool bShowSystem, bool bShowROM )
{
	m_bShowHidden = bShowHidden;
	m_bShowSystem = bShowSystem;
	m_bShowROM	  = bShowROM;
}


void FileList_c::Event_PluginOpened ( HANDLE hPlugin )
{
	m_eCachedSortMode	 = GetSortMode ();
	m_bCachedSortReverse = GetSortReverse ();

	const PluginInfo_t * pInfo = g_PluginManager.GetPluginInfo ( hPlugin );
	if ( !pInfo )
		return;

	SortMode_e eMode = pInfo->m_eStartSortMode;
	bool bReverse = pInfo->m_bStartSortReverse;

	if ( eMode != SORT_UNDEFINDED )
		SetSortMode ( eMode, bReverse );
}


void FileList_c::Event_PluginClosed ()
{
	SetSortMode ( m_eCachedSortMode, m_bCachedSortReverse );
}


void FileList_c::HideCursor ()
{
	if ( m_bSlowRefresh )
	{
		SetCursor ( m_hOldCursor );
		m_hOldCursor = NULL;
		m_bSlowRefresh = false;
		Sleep ( 0 );
	}
}


Str_c FileList_c::StepUpToValid ( const Str_c & sCurrentDir )
{
	Str_c sNewPath = sCurrentDir;
	PrependSlash ( sNewPath );
	
	int iSlashPos = sNewPath.RFind ( L'\\' );

	while ( iSlashPos >= 0 )
	{
		sNewPath.Chop ( sNewPath.Length () - iSlashPos );
		DWORD dwAttrib = GetFileAttributes ( sNewPath );
		if ( dwAttrib != 0xFFFFFFFF && ( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) )
			break;
		iSlashPos = sNewPath.RFind ( L'\\' );
	}

	AppendSlash ( sNewPath );

	return sNewPath;
}


void FileList_c::CloseActivePlugin ()
{
	if ( m_hActivePlugin == INVALID_HANDLE_VALUE )
		return;

	g_PluginManager.FreeFindData ( m_hActivePlugin, m_pPluginItems, m_nPluginItems );
	g_PluginManager.ClosePlugin ( m_hActivePlugin );
	
	m_pPluginItems = NULL;
	m_nPluginItems = 0;
	m_hActivePlugin = INVALID_HANDLE_VALUE;

	Event_PluginClosed ();
}