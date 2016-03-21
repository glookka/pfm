#include "pch.h"

#include "LFile/ffind.h"


FileSearch_c::FileSearch_c ( const SelectedFileList_t & tList, const FileSearchSettings_t & tSettings )
	: m_tSettings		( tSettings )
	, m_pStringSearch	( NULL )
	, m_bInStringSearch	( false )
	, m_pIterator		( NULL )
	, m_tFilter			( false )
{
	DWORD uFlags = m_tSettings.m_uSearchFlags;
	if ( uFlags & SEARCH_STRING )
		m_pStringSearch = new StringSearch_c ( tSettings.m_sContains, !!(uFlags & SEARCH_MATCH_CASE), !!(uFlags & SEARCH_WHOLE_WORD ));

	bool bRecursive = !! ( uFlags & SEARCH_RECURSIVE );

	m_sStartDir = tList.m_sRootDir;
	switch ( m_tSettings.m_eArea )
	{
		case SEARCH_AREA_FROMROOT:
			m_sStartDir = L"\\";
		case SEARCH_AREA_CURFOLDER:
			m_pIterator = new FileIteratorTree_c;
			m_pIterator->IterateStart ( m_sStartDir, bRecursive );
			break;
		case SEARCH_AREA_SELFOLDERS:
			m_pIterator = new FileIteratorPanel_c;
			m_pIterator->IterateStart ( tList, bRecursive );
			break;
	}

	m_tFilter.Set ( m_tSettings.m_sFilter );
	m_dFound.Reserve ( DEFAULT_FOUND_FILES );
}


FileSearch_c::~FileSearch_c ()
{
	SafeDelete ( m_pStringSearch );
	SafeDelete ( m_pIterator );
}


bool FileSearch_c::SearchNext ()
{
	Assert ( m_pIterator );

	if ( m_bInStringSearch )
	{
		Assert ( m_pStringSearch );
		if ( m_pStringSearch->DoSearch () )
		{
			const WIN32_FIND_DATA * pData = m_pIterator->GetData ();
			Assert ( pData );
			m_dFound.Add ( FoundFile_t ( *pData, m_pIterator->GetFullName () ) );
		}
		
		if ( m_pStringSearch->IsFileFinished () )
			m_bInStringSearch = false;
		
		return true;
	}

	for ( int i = 0; i < FIND_FILES_PAUSE; ++i )
	{
		bool bRes;
		do 
		{
			bRes = m_pIterator->IterateNext ();
		}
		while ( bRes && m_pIterator->Is2ndPassDir () );

		if ( ! bRes )
			return false;

		const WIN32_FIND_DATA * pData = m_pIterator->GetData ();
		if ( pData && m_tFilter.Fits ( pData->cFileName ) && CheckAdvancedSettings ( *pData ) )
		{
			if ( m_tSettings.m_uSearchFlags & SEARCH_STRING )
			{
				if ( ! ( pData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
				{
					if ( m_pStringSearch->StartFile ( m_pIterator->GetFullName () ) )
						m_bInStringSearch = true;
				}
			}
			else
				m_dFound.Add ( FoundFile_t ( *pData, m_pIterator->GetFullPath () ) );

			return true;
		}
	}

	return true;
}


int FileSearch_c::GetNumFiles () const
{
	return m_dFound.Length ();
}


const WIN32_FIND_DATA & FileSearch_c::GetData ( int nFile ) const
{
	return m_dFound [nFile].m_tData;
}


Str_c FileSearch_c::GetDirectory ( int nFile ) const
{
	Str_c sDir ( m_dFound [nFile].m_sDir );
	AppendSlash ( sDir );
	return sDir;
}


bool FileSearch_c::CheckAdvancedSettings ( const WIN32_FIND_DATA & tData ) const
{
    if ( ( tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && ! ( m_tSettings.m_uSearchFlags & SEARCH_FOLDERS ) )
		return false;
			
	FILETIME tDateToCheck;

	switch ( m_tSettings.m_eTimeType )
	{
	case SEARCH_TIME_CREATE:
		tDateToCheck = tData.ftCreationTime;
		break;

	case SEARCH_TIME_ACCESS:
		tDateToCheck = tData.ftLastAccessTime;
		break;

	case SEARCH_TIME_WRITE:
		tDateToCheck = tData.ftLastWriteTime;
		break;
	}
	
	if ( ( m_tSettings.m_uSearchFlags & SEARCH_DATE_1 )
		&& CompareFileTime ( &tDateToCheck, &m_tSettings.m_tTime1 ) < 0 )
		return false;

	if ( ( m_tSettings.m_uSearchFlags & SEARCH_DATE_2 )
		&& CompareFileTime ( &tDateToCheck, &m_tSettings.m_tTime2 ) > 0 )
		return false;

	ULARGE_INTEGER tSize;
	tSize.HighPart	= tData.nFileSizeHigh;
	tSize.LowPart	= tData.nFileSizeLow;

	if ( ( m_tSettings.m_uSearchFlags & SEARCH_SIZE_GREATER )
		&& tSize.QuadPart < m_tSettings.m_tSize1.QuadPart )
		return false;

	if ( ( m_tSettings.m_uSearchFlags & SEARCH_SIZE_LESS )
		&& tSize.QuadPart > m_tSettings.m_tSize2.QuadPart )
		return false;

	if ( m_tSettings.m_uSearchFlags & SEARCH_ATTRIBUTES )
	{
		bool bIncludeAttrib = ! m_tSettings.m_uIncludeAttributes || ( tData.dwFileAttributes & m_tSettings.m_uIncludeAttributes );
		bool bExcludeAttrib = ! m_tSettings.m_uExcludeAttributes || ( tData.dwFileAttributes & ~(m_tSettings.m_uExcludeAttributes) );

		if ( ! bIncludeAttrib || ! bExcludeAttrib )
			return false;
	}

	return true;
}