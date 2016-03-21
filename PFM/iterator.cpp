#include "pch.h"

#include "iterator.h"
#include "panel.h"

///////////////////////////////////////////////////////////////////////////////////////////
FileIteratorTree_c::FileIteratorTree_c ()
	: m_bSteppedUp	( false )
	, m_bRecursive	( false )
{
}

FileIteratorTree_c::~FileIteratorTree_c ()
{
	for ( int i = 0; i < m_dSearchLevels.Length (); ++i )
		if ( m_dSearchLevels [i].m_hHandle )
			FindClose ( m_dSearchLevels [i].m_hHandle );
}


void FileIteratorTree_c::IterateStart ( const Str_c & sRootDir, bool bRecursive )
{
	m_bSteppedUp = false;
	m_bRecursive = bRecursive;
	m_sRootDir = sRootDir;
	AppendSlash ( m_sRootDir );

	m_dSearchLevels.Add ( SearchLevel_t () );
}


bool FileIteratorTree_c::IsRootDir () const
{
	if ( m_dSearchLevels.Empty () )
		return true;

	const SearchLevel_t & tLevel = m_dSearchLevels.Last ();
	if ( tLevel.m_sDirName.Empty () )
		return true;

	return false;
}


bool FileIteratorTree_c::Is2ndPassDir () const
{
	return m_bSteppedUp;
}


bool FileIteratorTree_c::IterateNext ()
{
	if ( m_dSearchLevels.Empty () )
		return false;

	SearchLevel_t & tLevel = m_dSearchLevels.Last ();

	if ( tLevel.m_hHandle == NULL )
	{
		// we didn't use this level yet so try for the first time
		Str_c sFileToFind = m_sRootDir;

		if ( ! tLevel.m_sDirName.Empty () )
		{
			sFileToFind += tLevel.m_sDirName;
			AppendSlash ( sFileToFind );
		}

		sFileToFind += L"*";

		tLevel.m_hHandle = FindFirstFile ( sFileToFind, &tLevel.m_tData );
		if ( tLevel.m_hHandle == INVALID_HANDLE_VALUE )
		{
			m_dSearchLevels.Pop ();
			m_bSteppedUp = true;
			return !m_dSearchLevels.Empty ();
		}
	}
	else
	{
		// last time we found a directory -> step into it
		if ( ! m_bSteppedUp && m_bRecursive && ( tLevel.m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
		{
			Str_c sDirName;
			if ( tLevel.m_sDirName.Empty () )
				sDirName = tLevel.m_tData.cFileName;
			else
				sDirName = tLevel.m_sDirName + L"\\" + tLevel.m_tData.cFileName;

			m_dSearchLevels.Add ( SearchLevel_t ( sDirName ) );
			return IterateNext ();
		}
		else
		{
			if ( ! FindNextFile ( tLevel.m_hHandle, &tLevel.m_tData ) )
			{
				// nothing interesting on this level
				FindClose ( tLevel.m_hHandle );
				m_bSteppedUp = true;
				m_dSearchLevels.Pop ();

				return !m_dSearchLevels.Empty ();
			}
		}
	}

	m_bSteppedUp = false;
	return true;
}

Str_c FileIteratorTree_c::GetFullName () const
{
	if ( m_dSearchLevels.Empty () )
		return GetFullPath ();
	else
		return GetFullPath () + m_dSearchLevels.Last ().m_tData.cFileName;
}


Str_c FileIteratorTree_c::GetFullPath () const
{
	if ( ! m_dSearchLevels.Empty () )
	{
		const SearchLevel_t & tLevel = m_dSearchLevels.Last ();
		return m_sRootDir + tLevel.m_sDirName;
	}

	return m_sRootDir;
}


const WIN32_FIND_DATA * FileIteratorTree_c::GetData () const
{
	return m_dSearchLevels.Empty () ? NULL : &( m_dSearchLevels.Last ().m_tData );
}



///////////////////////////////////////////////////////////////////////////////////////////
FileIteratorPanel_c::FileIteratorPanel_c ()
	: m_bLastDirectory	( false )
	, m_bSteppedUp		( false )
	, m_bInTree			( false )
	, m_iCurrentFile	( 0 )
	, m_bRecursive		( false )
	, m_bSkipRoot		( false )
	, m_pList			( NULL )
{
}


void FileIteratorPanel_c::IterateStart ( const SelectedFileList_t & tList, bool bRecursive )
{	
	m_pList				= &tList;
	m_bLastDirectory	= false;
	m_bSteppedUp		= false;
	m_bSkipRoot			= false;
	m_sSteppedUpDir		= L"";
	m_bInTree			= false;
	m_iCurrentFile		= 0;
	m_sFileName			= L"";
	m_sPassRootDir		= L"";
	m_bRecursive		= bRecursive;
}


bool FileIteratorPanel_c::IterateNext ()
{
	Assert ( m_pList );

	m_bSteppedUp = false;

	// last one was a directory -> time to read the tree
	if ( m_bLastDirectory )
	{
		m_bInTree			= true;
		m_bLastDirectory	= false;
		m_sPassRootDir		= m_sFileName;
		m_tTreeIterator.IterateStart ( m_pList->m_sRootDir + m_sPassRootDir );
	}

	// we're already reading the tree
	if ( m_bInTree )
	{
		if ( ! m_tTreeIterator.IterateNext () )
		{
			m_bSteppedUp = true;
			m_bInTree = false;
		}
		
		return true;
	}

	if ( m_iCurrentFile >= m_pList->m_dFiles.Length () )
		return false;

	const PanelItem_t * pInfo = m_pList->m_dFiles [m_iCurrentFile];
	Assert ( pInfo );

	m_sFileName = pInfo->m_FindData.cFileName;
	m_tData		= pInfo->m_FindData;

	if ( m_bRecursive )
		m_bLastDirectory = !! ( pInfo->m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );

	++m_iCurrentFile;

	// if this is the first file
	if ( m_bSkipRoot && m_iCurrentFile == 1 && ! m_bInTree && ! m_bSteppedUp )
		return IterateNext ();

	return true;
}


void FileIteratorPanel_c::SkipRoot ( bool bSkip )
{
	m_bSkipRoot = bSkip;
}


bool FileIteratorPanel_c::IsRootDir () const
{
	return ! m_bInTree;
}


bool FileIteratorPanel_c::Is2ndPassDir () const
{
	if ( m_bInTree )
		return m_tTreeIterator.Is2ndPassDir ();
	else
		return m_bSteppedUp;
}


Str_c FileIteratorPanel_c::GetFileNameSkipped () const
{
	Assert ( 0 && "needs testing" );

	if ( ! m_bSkipRoot )
		return GetFullName ();
	else
	{
		if ( m_bInTree )
			return m_tTreeIterator.GetFullName ();
		else
			return L"";
	}
}

Str_c FileIteratorPanel_c::GetFullName () const
{
	if ( m_bInTree )
		return m_tTreeIterator.GetFullName ();
	else
		return GetFullPath () + ( m_bSteppedUp ? m_sPassRootDir : m_sFileName );
}

Str_c FileIteratorPanel_c::GetFullPath () const
{
	if ( m_bInTree )
		return m_tTreeIterator.GetFullPath ();
	else
		return m_pList->m_sRootDir;
}


const WIN32_FIND_DATA * FileIteratorPanel_c::GetData () const
{
	if ( m_bInTree )
		return m_tTreeIterator.GetData ();
	else
		return &m_tData;
}

Str_c FileIteratorPanel_c::GetPartialPath () const
{
	Str_c sPath = GetFullPath ();
	if ( sPath.Begins ( m_pList->m_sRootDir ) )
	{
		Str_c sTmp = sPath.SubStr ( m_pList->m_sRootDir.Length () );
		return RemoveSlash ( sTmp );
	}
	else
		return sPath;
}


//////////////////////////////////////////////////////////////////////////////////////////

FileIteratorPanelCached_c::FileIteratorPanelCached_c ()
	: m_iIndex 			( 0 )
	, m_pCurrentResult	( NULL )
{
}

void FileIteratorPanelCached_c::IterateStart ( const SelectedFileList_t & tList, bool bRecursive )
{
	FileIteratorPanel_c * pIterator = new FileIteratorPanel_c;
	pIterator->IterateStart ( tList, bRecursive );

	m_sRootDir = tList.m_sRootDir;

	CachedResult_t tRes;

	while ( pIterator->IterateNext () )
	{
		tRes.m_b2ndPassDir	= pIterator->Is2ndPassDir ();
		tRes.m_bRootDir		= pIterator->IsRootDir ();
		tRes.m_sPartialDir	= pIterator->GetPartialPath ();
		tRes.m_tData		= *pIterator->GetData ();

		m_dFound.Add ( tRes );
	}

	SafeDelete ( pIterator );

	m_iIndex = 0;
}

bool FileIteratorPanelCached_c::IterateNext ()
{
	if ( m_iIndex >= m_dFound.Length () )
		return false;

	m_pCurrentResult = & m_dFound [m_iIndex];
	++m_iIndex;

	return true;
}

bool FileIteratorPanelCached_c::IsRootDir () const
{
	Assert ( m_pCurrentResult );
	return m_pCurrentResult->m_bRootDir;
}

bool FileIteratorPanelCached_c::Is2ndPassDir () const
{
	Assert ( m_pCurrentResult );
	return m_pCurrentResult->m_b2ndPassDir;
}

Str_c FileIteratorPanelCached_c::GetFullName () const
{
	Assert ( m_pCurrentResult );
	return GetFullPath () + m_pCurrentResult->m_tData.cFileName;
}

Str_c FileIteratorPanelCached_c::GetFullPath () const
{
	Assert ( m_pCurrentResult );
	Str_c sTmp = m_sRootDir + m_pCurrentResult->m_sPartialDir;
	return AppendSlash( sTmp );
}

const WIN32_FIND_DATA *	FileIteratorPanelCached_c::GetData () const
{
	Assert ( m_pCurrentResult );
	return &m_pCurrentResult->m_tData;
}