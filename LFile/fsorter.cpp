#include "pch.h"

#include "LFile/fsorter.h"
#include "LCore/cfile.h"
#include "LFile/fcards.h"
#include "LFile/fcolorgroups.h"
#include "LSettings/sconfig.h"
#include "LCore/clog.h"

enum
{
	 FILE_RESERVE	= 64
	,LOTS_OF_FILES	= 100
};

///////////////////////////////////////////////////////////////////////////////////////////
// file sorter
static SortMode_e g_eSortMode = SORT_UNSORTED;
static bool g_bSortReverse = true;

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

static int FileCompare ( const FileInfo_t & tFile1, const FileInfo_t & tFile2 )
{
	// comparison rules:
	// 1. directories always come first (except for unsorted mode)
	// 2. directories don't get sorted by ext/name, only by name+ext
	// 3. files w/o ext come first
	// 4. same ext -> sort by name
	// 5. same name -> sort by ext

	int iRes = -1;
	bool bCompareDirs = false;

	if ( tFile1.m_uFlags & FLAG_PREV_DIR )
	{
		if ( tFile2.m_uFlags & FLAG_PREV_DIR )
			return 0;
		else
			return -1;
	}
	else
	{
		if ( tFile2.m_uFlags & FLAG_PREV_DIR )
			return 1;
	}

	if ( tFile1.m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
	{
		if ( tFile2.m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			bCompareDirs = true;
		else
			return -1;
	}
	else
	{
		if ( tFile2.m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			return 1;
	}

	SortMode_e eSortMode = g_eSortMode;
	if ( bCompareDirs && ( eSortMode == SORT_EXTENSION || eSortMode == SORT_SIZE ) )
		eSortMode = SORT_NAME;

	switch ( eSortMode )
	{
	case SORT_NAME:
		iRes = CompareFilenames ( tFile1.m_tData.cFileName, tFile2.m_tData.cFileName, true );
		break;

	case SORT_EXTENSION:
		{
			const wchar_t * szExt1 = GetNameExtFast ( tFile1.m_tData.cFileName );
			const wchar_t * szExt2 = GetNameExtFast ( tFile2.m_tData.cFileName );

			iRes = _wcsicmp ( szExt1, szExt2 );	
			if ( !iRes )
				iRes = _wcsicmp ( tFile1.m_tData.cFileName, tFile2.m_tData.cFileName );
		}
		break;

	case SORT_SIZE:
		if ( tFile1.m_tData.nFileSizeHigh == tFile2.m_tData.nFileSizeHigh )
			iRes = tFile1.m_tData.nFileSizeLow < tFile2.m_tData.nFileSizeLow ? -1 : ( tFile1.m_tData.nFileSizeLow == tFile2.m_tData.nFileSizeLow ? 0 : 1 );
		else
			iRes = tFile1.m_tData.nFileSizeHigh < tFile2.m_tData.nFileSizeHigh ? -1 : ( tFile1.m_tData.nFileSizeHigh == tFile2.m_tData.nFileSizeHigh ? 0 : 1 );
		break;

	case SORT_TIME_ACCESS:
		iRes = CompareFileTime ( &tFile1.m_tData.ftLastAccessTime, &tFile2.m_tData.ftLastAccessTime );
		break;

	case SORT_TIME_CREATE:
		iRes = CompareFileTime ( &tFile1.m_tData.ftCreationTime, &tFile2.m_tData.ftCreationTime );
		break;

	case SORT_TIME_WRITE:
		iRes = CompareFileTime ( &tFile1.m_tData.ftLastWriteTime, &tFile2.m_tData.ftLastWriteTime );
		break;

	case SORT_GROUP:
		iRes = CompareColorGroups ( tFile1.m_pCG, tFile2.m_pCG );
		break;
	}

	iRes = Clamp ( iRes, -1, 1 );

	if ( iRes == 0 && eSortMode >= SORT_SIZE )
		iRes = CompareFilenames ( tFile1.m_tData.cFileName, tFile2.m_tData.cFileName, false );

	return g_bSortReverse ? -iRes : iRes;
}

///////////////////////////////////////////////////////////////////////////////////////////
// a class for file sorting/storing
FileSorter_c::FileSorter_c ()
	: m_bSortReverse 	( false )
	, m_eSortMode		( SORT_UNSORTED )
	, m_sDir			( L"\\" )
	, m_nMarked			( 0 )
	, m_bShowHidden		( true )
	, m_bShowSystem		( true )
	, m_bShowROM		( true )
	, m_hOldCursor		( NULL )
	, m_bSlowRefresh	( false )
{
	m_tMarkedSize.QuadPart = 0;
	m_dFiles.Resize ( FILE_RESERVE );
}


int FileSorter_c::GetNumFiles () const
{
	return m_dFiles.Length ();
}


const FileInfo_t & FileSorter_c::GetFile ( int nFile ) const
{
	return m_dFiles [nFile];
}


const Str_c & FileSorter_c::GetDirectory () const
{
	return m_sDir;
}


void FileSorter_c::Refresh ()
{
	m_nMarked = 0;
	m_tMarkedSize.HighPart	= 0;
	m_tMarkedSize.LowPart	= 0;

	m_dFiles.Clear ();

	FileInfo_t tFindInfo;
	HANDLE hFile;

	bool bRootDir = false;

	// check if the dir exists
	if ( m_sDir != L"\\" )
	{
		Str_c sDirWithNoSlash = m_sDir;
		sDirWithNoSlash.Chop ( 1 );

		DWORD dwAttrib = GetFileAttributes ( sDirWithNoSlash );
		if ( dwAttrib == 0xFFFFFFFF || ! ( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) )
		{
			SetDirectory ( StepUpToValid ( m_sDir ) );
			bRootDir = m_sDir == L"\\";
		}
	}
	else
		bRootDir = true;

	// not the root?
	if ( ! bRootDir )
	{
		Str_c sDirWithNoSlash = m_sDir;
		sDirWithNoSlash.Chop ( 1 );
		
		FileInfo_t tFirstDir;
		HANDLE hFirstFile = FindFirstFile ( sDirWithNoSlash, &tFirstDir.m_tData );
		if ( hFirstFile != INVALID_HANDLE_VALUE )
			FindClose ( hFirstFile );
		else
			memset ( &tFirstDir, 0, sizeof ( tFirstDir ) );

		wcscpy ( tFirstDir.m_tData.cFileName, L".." );
		tFirstDir.m_uFlags 					= FLAG_PREV_DIR;
		tFirstDir.m_pCG 					= GetColorGroup ( tFirstDir );
		tFirstDir.m_iIconIndex 				= -1;
		m_dFiles.Add ( tFirstDir );
	}

	hFile = FindFirstFile ( m_sDir + L"*", &tFindInfo.m_tData );
	if ( hFile == INVALID_HANDLE_VALUE )
	{
		Event_Refresh ();
		return;
	}

	int nFiles = 0;

	do
	{
		if ( ( ! ( tFindInfo.m_tData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ) || m_bShowHidden ) &&
			 ( ! ( tFindInfo.m_tData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ) || m_bShowSystem ) &&
			 ( ! ( tFindInfo.m_tData.dwFileAttributes & FILE_ATTRIBUTE_INROM )	|| m_bShowROM ) )
		{
			tFindInfo.m_uFlags = FLAG_NOTHING;
			tFindInfo.m_iIconIndex = -1;
			
			// see if its a storage card
			if ( bRootDir )
			{
				WIN32_FIND_DATA tData;
				int nStorageCards = GetNumStorageCards ();
				for ( int i = 0; i < nStorageCards; ++i )
				{
					GetStorageCardData ( i, tData );
					if ( ! wcscmp ( tData.cFileName, tFindInfo.m_tData.cFileName ) )
					{
						tFindInfo.m_uFlags |= FLAG_DEVICE;
						break;
					}
				}
			}
			tFindInfo.m_pCG = GetColorGroup ( tFindInfo );
			m_dFiles.Add ( tFindInfo );

			++nFiles;
			if ( nFiles == LOTS_OF_FILES )
				Event_SlowRefresh ();
		}
	}
	while ( FindNextFile ( hFile, &tFindInfo.m_tData ) );

	FindClose ( hFile );

	if ( m_eSortMode != SORT_UNSORTED )
	{
		g_eSortMode = m_eSortMode;
		g_bSortReverse = m_bSortReverse;

		Sort ( m_dFiles, FileCompare );
	}

	Event_Refresh ();
}


void FileSorter_c::SoftRefresh ()
{
	Event_BeforeSoftRefresh ();

	Array_T <Str_c> dStored;
	for ( int i = 0; i < GetNumFiles (); ++i )
	{
		const FileInfo_t & tInfo = GetFile ( i );

		if ( !! ( tInfo.m_uFlags & FLAG_MARKED ) && ! ( tInfo.m_uFlags & FLAG_PREV_DIR ) )
			dStored.Add ( tInfo.m_tData.cFileName );
	}

	Refresh ();

	for ( int i = 0; i < dStored.Length (); ++i )
		MarkFile ( dStored [i], true );

	Event_AfterSoftRefresh ();
}


SortMode_e FileSorter_c::GetSortMode () const
{
	return m_eSortMode;
}


bool FileSorter_c::GetSortReverse () const
{
	return m_bSortReverse;
}


bool FileSorter_c::StepToPrevDir ( Str_c * pPrevDirName )
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


void FileSorter_c::StepToNextDir ( const wchar_t * szNextDir )
{
	SetDirectory ( m_sDir + szNextDir );
}


void FileSorter_c::SetDirectory ( const wchar_t * szDir )
{
	m_sDir = szDir;

	int iLength = m_sDir.Length ();
	if ( ! iLength )
		m_sDir = L'\\';
	else
	{
		if ( m_sDir [0] != L'\\' )
			m_sDir = Str_c ( L'\\' ) + m_sDir;

		if ( m_sDir [iLength - 1] != L'\\' )
			m_sDir += L"\\";
	}

	Event_Directory ();
}


bool FileSorter_c::MarkFile ( int nFile, bool bSelect )
{
	FileInfo_t & tInfo = m_dFiles [nFile];
	if ( tInfo.m_uFlags & FLAG_PREV_DIR )
		return false;

	bool bWasMarked = !! ( tInfo.m_uFlags & FLAG_MARKED );
	ULARGE_INTEGER tSize;
	tSize.HighPart	= tInfo.m_tData.nFileSizeHigh;
	tSize.LowPart	= tInfo.m_tData.nFileSizeLow;

	if ( bWasMarked )
	{
		if ( ! bSelect )
		{
			--m_nMarked;
			m_tMarkedSize.QuadPart -= tSize.QuadPart;
			tInfo.m_uFlags &= ~FLAG_MARKED;
			Event_MarkFile ( nFile, false );
			return true;
		}
	}
	else
	{
		if ( bSelect )
		{
			++m_nMarked;
			m_tMarkedSize.QuadPart += tSize.QuadPart;
			tInfo.m_uFlags |= FLAG_MARKED;
			Event_MarkFile ( nFile, true );
			return true;
		}
	}

	return false;
}


bool FileSorter_c::MarkFile ( const wchar_t * szName, bool bSelect )
{
	for ( int i = 0; i < GetNumFiles (); ++i )
		if ( ! wcscmp ( GetFile ( i ).m_tData.cFileName, szName ) )
			return MarkFile ( i, bSelect );

	return false;
}


void FileSorter_c::MarkFilter ( const wchar_t * szFilter, bool bSelect )
{
	bool bUseDirs = g_tConfig.filter_include_folders;

	m_tFilter.Set ( szFilter );
	for ( int i = 0; i < m_dFiles.Length (); ++i )
	{
		if ( ( ! ( m_dFiles [i].m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) || bUseDirs )
				&& m_tFilter.Fits ( m_dFiles [i].m_tData.cFileName ) )
			MarkFile ( i, bSelect );
	}
}


int FileSorter_c::GetNumMarked () const
{
	return m_nMarked;
}


ULARGE_INTEGER FileSorter_c::GetMarkedSize () const
{
	return m_tMarkedSize;
}


int FileSorter_c::GetFileIconIndex ( int iFile )
{
	FileInfo_t & tInfo = m_dFiles [iFile];

	if ( tInfo.m_iIconIndex ==  -1 )
	{
		SHFILEINFO tShFileInfo;
		tShFileInfo.iIcon = -1;
		if ( tInfo.m_uFlags & FLAG_PREV_DIR )
			SHGetFileInfo ( L" ", FILE_ATTRIBUTE_DIRECTORY, &tShFileInfo, sizeof (SHFILEINFO), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON );
		else
			SHGetFileInfo ( m_sDir + tInfo.m_tData.cFileName, 0, &tShFileInfo, sizeof (SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON );

		Assert ( tShFileInfo.iIcon != -1 );

		tInfo.m_iIconIndex = tShFileInfo.iIcon;
	}

	return tInfo.m_iIconIndex;
}


void FileSorter_c::SetSortMode ( SortMode_e eMode, bool bSortReverse )
{
	m_eSortMode = eMode;
	m_bSortReverse = bSortReverse;
}


void FileSorter_c::SetVisibility ( bool bShowHidden, bool bShowSystem, bool bShowROM )
{
	m_bShowHidden = bShowHidden;
	m_bShowSystem = bShowSystem;
	m_bShowROM	  = bShowROM;
}

void FileSorter_c::Event_Refresh ()
{
	if ( m_bSlowRefresh )
	{
		SetCursor ( m_hOldCursor );
		m_hOldCursor = NULL;
		m_bSlowRefresh = false;
		Sleep ( 0 );
	}
}

void FileSorter_c::Event_SlowRefresh ()
{
	m_hOldCursor	= SetCursor ( LoadCursor ( NULL, IDC_WAIT ) );
	m_bSlowRefresh	= true;
}

Str_c FileSorter_c::StepUpToValid ( const Str_c & sCurrentDir )
{
	Str_c sNewPath = sCurrentDir;
	
	int iSlashPos = sNewPath.RFind ( L'\\' );

	while ( iSlashPos >= 0 )
	{
		sNewPath.Chop ( sNewPath.Length () - iSlashPos );
		DWORD dwAttrib = GetFileAttributes ( sNewPath );
		if ( dwAttrib != 0xFFFFFFFF && ( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) )
			break;
		iSlashPos = sNewPath.RFind ( L'\\' );
	}

	return sNewPath;
}
