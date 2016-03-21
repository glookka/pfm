#ifndef _ffind_
#define _ffind_

#include "pfm/main.h"
#include "pfm/filter.h"
#include "pfm/iterator.h"
#include "LFile/fquicksearch.h"

enum SearchFlags_e
{
	 SEARCH_MATCH_CASE			= 1 << 0
	,SEARCH_WHOLE_WORD			= 1 << 1
	,SEARCH_STRING				= 1 << 2
	,SEARCH_DATE_1				= 1 << 3
	,SEARCH_DATE_2				= 1 << 4
	,SEARCH_SIZE_GREATER		= 1 << 5
	,SEARCH_SIZE_LESS			= 1 << 6
	,SEARCH_ATTRIBUTES			= 1 << 7
	,SEARCH_FOLDERS				= 1 << 8
	,SEARCH_RECURSIVE			= 1 << 9
};


enum SearchArea_e
{
	 SEARCH_AREA_CURFOLDER
	,SEARCH_AREA_FROMROOT
	,SEARCH_AREA_SELFOLDERS
};

enum SearchTime_e
{
	  SEARCH_TIME_CREATE
	 ,SEARCH_TIME_ACCESS
	 ,SEARCH_TIME_WRITE
};


struct FileSearchSettings_t
{
	FileSearchSettings_t ()
		: m_uSearchFlags		( 0 )
		, m_eArea				( SEARCH_AREA_CURFOLDER )
		, m_eTimeType			( SEARCH_TIME_WRITE )
		, m_uIncludeAttributes	( 0 )
		, m_uExcludeAttributes	( 0 )
	{
		m_tSize1.QuadPart = m_tSize2.QuadPart = 0;
		SYSTEMTIME tTime;
		GetLocalTime ( &tTime );
		SystemTimeToFileTime ( &tTime, &m_tTime1 );
		m_tTime2 = m_tTime1;
	}

	DWORD 			m_uSearchFlags;
	Str_c 			m_sFilter;
	Str_c 			m_sContains;
	SearchArea_e	m_eArea;
	SearchTime_e	m_eTimeType;
	FILETIME		m_tTime1;
	FILETIME		m_tTime2;
	ULARGE_INTEGER	m_tSize1;
	ULARGE_INTEGER	m_tSize2;
	DWORD			m_uIncludeAttributes;
	DWORD			m_uExcludeAttributes;
};


//
// searches for files given a root directory and a filter
//
class FileSearch_c
{
public:
						FileSearch_c ( const SelectedFileList_t & tList, const FileSearchSettings_t & tSettings );
						~FileSearch_c ();
				
	bool				SearchNext ();

	int					GetNumFiles () const;
	const WIN32_FIND_DATA & GetData ( int nFile ) const;
	Str_c				GetDirectory ( int nFile ) const;

	Str_c				GetSearchDirectory () const;

private:
	enum
	{
		 FIND_FILES_PAUSE		= 10
		,DEFAULT_FOUND_FILES	= 128
	};

	struct FoundFile_t
	{
		FoundFile_t ()
		{
		}

		FoundFile_t ( const WIN32_FIND_DATA	& tData, const Str_c	& sDir )
			: m_tData ( tData )
			, m_sDir ( sDir )
		{
		}

		WIN32_FIND_DATA	m_tData;
		Str_c	m_sDir;
	};

	bool							m_bInStringSearch;
	Array_T < FoundFile_t >			m_dFound;
	Filter_c						m_tFilter;
	FileIterator_c *				m_pIterator;
	StringSearch_c *				m_pStringSearch;
	FileSearchSettings_t			m_tSettings;
	Str_c							m_sStartDir;

	bool				CheckAdvancedSettings ( const WIN32_FIND_DATA & tData ) const;
};

#endif
