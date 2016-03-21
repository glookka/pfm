#include "pch.h"

#include "LFile/fiterator.h"
#include "LCore/clog.h"
#include "LCore/cfile.h"
#include "LPanel/pbase.h"

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
			AppendSlash ( sFileToFind );
			sFileToFind += tLevel.m_sDirName;
		}

		AppendSlash ( sFileToFind );

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


Str_c FileIteratorTree_c::GetFileName () const
{
	if ( m_dSearchLevels.Empty () )
		return L"";
	else
	{
		const SearchLevel_t & tLevel = m_dSearchLevels.Last ();
		if ( tLevel.m_sDirName.Empty () )
			return tLevel.m_tData.cFileName;
		else
			return tLevel.m_sDirName + L"\\" + tLevel.m_tData.cFileName;
	}
}


Str_c FileIteratorTree_c::GetDirectory () const
{
	if ( ! m_dSearchLevels.Empty () )
	{
		const SearchLevel_t & tLevel = m_dSearchLevels.Last ();
		return tLevel.m_sDirName;
	}

	return L"";
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


void FileIteratorPanel_c::IterateStart ( FileList_t & tList, bool bRecursive )
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

	const FileInfo_t * pInfo = m_pList->m_dFiles [m_iCurrentFile];
	Assert ( pInfo );

	m_sFileName = pInfo->m_tData.cFileName;
	m_tData		= pInfo->m_tData;

	if ( m_bRecursive )
		m_bLastDirectory = !! ( pInfo->m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );

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
	if ( ! m_bSkipRoot )
		return GetFileName ();
	else
	{
		if ( m_bInTree )
			return m_tTreeIterator.GetFileName ();
		else
			return L"";
	}
}

Str_c FileIteratorPanel_c::GetFileName () const
{
	if ( m_bInTree )
	{
		Str_c sRetDir = m_sPassRootDir;
		AppendSlash ( sRetDir );
		return sRetDir + m_tTreeIterator.GetFileName ();
	}
	else
	{
		if ( m_bSteppedUp )
			return m_sPassRootDir;
		else
			return m_sFileName;
	}
}

Str_c FileIteratorPanel_c::GetDirectory () const
{
	if ( m_bInTree )
	{
		Str_c sRetDir = m_sPassRootDir;
		Str_c sIDir = m_tTreeIterator.GetDirectory ();

		if ( sIDir.Empty () )
			return sRetDir;
		else
		{
			AppendSlash ( sRetDir );
			return sRetDir + sIDir;
		}
	}
	else
	{
		if ( m_bSteppedUp )
			return m_sPassRootDir;
		else
			return L"";
	}
}


const WIN32_FIND_DATA * FileIteratorPanel_c::GetData () const
{
	if ( m_bInTree )
		return m_tTreeIterator.GetData ();
	else
		return &m_tData;
}


const Str_c & FileIteratorPanel_c::GetSourceDir () const
{
	Assert ( m_pList );
	return m_pList->m_sRootDir;
}


//////////////////////////////////////////////////////////////////////////////////////////

FileIteratorPanelCached_c::FileIteratorPanelCached_c ()
	: m_iIndex 			( 0 )
	, m_pIterator		( NULL )
	, m_pCurrentResult	( NULL )
{
}

void FileIteratorPanelCached_c::IterateStart ( FileList_t & tList, bool bRecursive )
{
	m_pIterator = new FileIteratorPanel_c;
	m_pIterator->IterateStart ( tList, bRecursive );

	CachedResult_t tRes;

	while ( m_pIterator->IterateNext () )
	{
		tRes.m_b2ndPassDir = m_pIterator->Is2ndPassDir ();
		tRes.m_bRootDir = m_pIterator->IsRootDir ();
		tRes.m_sDir = m_pIterator->GetDirectory ();
		tRes.m_tData = *m_pIterator->GetData ();

		m_dFound.Add ( tRes );
	}

	SafeDelete ( m_pIterator );

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

Str_c FileIteratorPanelCached_c::GetFileName () const
{
	Assert ( m_pCurrentResult );
	if ( m_pCurrentResult->m_sDir.Empty () )
		return m_pCurrentResult->m_tData.cFileName;
	else
		return m_pCurrentResult->m_sDir + L"\\" + m_pCurrentResult->m_tData.cFileName;
}

Str_c FileIteratorPanelCached_c::GetDirectory () const
{
	Assert ( m_pCurrentResult );
	return m_pCurrentResult->m_sDir;
}

const WIN32_FIND_DATA *	FileIteratorPanelCached_c::GetData () const
{
	Assert ( m_pCurrentResult );
	return &m_pCurrentResult->m_tData;
}

const FileIteratorPanelCached_c::CachedResult_t & FileIteratorPanelCached_c::GetCachedResult ( int iId ) const
{
	return m_dFound [iId];
}

int	FileIteratorPanelCached_c::GetIndex () const
{
	return m_iIndex - 1;
}