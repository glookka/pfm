#include "pch.h"

#include "filter.h"

enum
{
	MAX_FILTER_LEN = 256
};

wchar_t g_dFilter [MAX_FILTER_LEN];

///////////////////////////////////////////////////////////////////////////////////////////
// base filter class
BaseFilter_c::BaseFilter_c ( bool bCaseSensitive )
	: m_bCaseSensitive ( bCaseSensitive )
{
}

void BaseFilter_c::Set ( const wchar_t * szFilter )
{
	m_dMasks.Clear ();

	int iLen = wcslen ( szFilter );
	if ( ! iLen )
		return;

	int iPrev = 0;
	Str_c sMask;
	for ( int i = 0; i < iLen; ++i )
		if ( szFilter [i] == L',' || szFilter [i] == L';' )
		{
			if ( i - iPrev < MAX_FILTER_LEN )
			{
				wcsncpy ( g_dFilter, szFilter + iPrev, i - iPrev );
				g_dFilter [i - iPrev] = L'\0';

				// cut spaces
				wchar_t * szFilterStart = & ( g_dFilter [0] );
				while ( *szFilterStart && iswspace ( *szFilterStart ) )
					++szFilterStart;

				if ( !m_bCaseSensitive )
					wcslwr ( szFilterStart );

				iPrev = i+1;
				m_dMasks.Add ( szFilterStart );
			}
		}

	// string has text but there are no masks
	if ( iPrev < iLen )
	{
		int iFilterLen = wcslen ( szFilter + iPrev );
		if ( iFilterLen < MAX_FILTER_LEN )
		{
			wcsncpy ( g_dFilter, szFilter + iPrev, iFilterLen );
			g_dFilter [iFilterLen] = L'\0';

			// cut spaces
			wchar_t * szFilterStart = & ( g_dFilter [0] );
			while ( *szFilterStart && iswspace ( *szFilterStart ) )
				++szFilterStart;

			if ( !m_bCaseSensitive )
				wcslwr ( szFilterStart );

			m_dMasks.Add ( szFilterStart );
		}
	}
}


bool BaseFilter_c::Fits ( const wchar_t * szFile ) const
{
	Assert ( szFile );

	if ( m_dMasks.Empty () )
		return true;

	const wchar_t * szCompare = GetStringToCompare ( szFile );
	if ( ! szCompare )
		return false;

	bool bFits = false;
	for ( int i = 0; i < m_dMasks.Length () && !bFits; ++i )
		bFits = FitsMask ( szCompare, m_dMasks [i] );

	return bFits;

}


bool BaseFilter_c::HasMasks () const
{
	return !m_dMasks.Empty ();
}


bool BaseFilter_c::CheckRangeSet ( const wchar_t * & szFileStart, const wchar_t * & szFilterStart ) const
{
	Assert ( szFileStart && szFilterStart );

	if ( ! *szFileStart )
		return false;

	wchar_t cFirst, cSym;
	if ( ( cFirst = *szFilterStart++ ) == L'\0' )
		return false;

	if ( *szFilterStart == L'-' ) 	// it's a range!
	{
		++szFilterStart;
		if ( ( cSym = *szFilterStart++ ) == L'\0' )
			return false;

		if ( *szFileStart < cFirst || *szFileStart > cSym )
			return false;

		if ( ( cSym = *szFilterStart++ ) == L'\0' )
			return false;

		if ( cSym != L']' )
			return false;
	}
	else					// it's a symbol set
	{
		bool bFound = false, bBrace = false;
		wchar_t cFileSym = *szFileStart;

		cSym = cFirst;

		do
		{
			if ( cSym == L']' )
			{
				bBrace = true;
				break;
			}

			if ( cSym == cFileSym )
				bFound = true;
		}
		while ( ( cSym = *szFilterStart++ ) != L'\0' );

		if ( ! bFound || ! bBrace )
			return false;
	}

	return true;
}


///////////////////////////////////////////////////////////////////////////////////////////
// extension filter
ExtFilter_c::ExtFilter_c ()
	: BaseFilter_c ( false )
{
}

const wchar_t *	ExtFilter_c::GetStringToCompare ( const wchar_t * szFile ) const
{
	const wchar_t * szStr = GetNameExtFast ( szFile );
	int iLen = wcslen ( szStr );
	if ( iLen < MAX_FILTER_LEN )
	{
		wcscpy ( g_dFilter, szStr );
		wcslwr ( g_dFilter );
		return g_dFilter;
	}

	return NULL;
}


bool ExtFilter_c::FitsMask ( const wchar_t * szExt, const wchar_t * szMask ) const
{
	Assert ( szExt && szMask );

	const wchar_t * szFileStart = szExt;
	const wchar_t * szFilterStart = szMask;

	bool bFits = true;
	while ( bFits && *szFileStart && *szFilterStart )
	{
		switch ( *szFilterStart )
		{
		case L'?':	// any symbol
			++szFilterStart;
			++szFileStart;
			break;

		case L'[':	// start of range/set
			++szFilterStart;
			if ( ! CheckRangeSet ( szFileStart, szFilterStart ) )
				bFits = false;
			else
				++szFileStart;
			break;

		default:	// just a symbol
			if ( *szFilterStart != *szFileStart )
				bFits = false;
			else
			{
				++szFilterStart;
				++szFileStart;
			}
			break;
		}
	}

	// filter or name has unused chars
	if ( *szFileStart && ! *szFilterStart || ! *szFileStart && *szFilterStart )
		return false;

	return bFits;
}


///////////////////////////////////////////////////////////////////////////////////////////
// common filter
Filter_c::Filter_c ( bool bCaseSensitive )
	: BaseFilter_c ( bCaseSensitive )
{
}

void Filter_c::Set ( const wchar_t * szFilter )
{
	int iLen = wcslen ( szFilter );

	if ( iLen && wcscspn ( szFilter, L"?*[]" ) == (unsigned int)iLen && iLen + 1 < MAX_FILTER_LEN - 2 )
	{
		wchar_t szTemp [MAX_FILTER_LEN];
		wcscpy ( &(szTemp [1]), szFilter );
		szTemp [0] = L'*';
		szTemp [iLen + 1] = L'*';
		szTemp [iLen + 2] = L'\0';
		BaseFilter_c::Set ( szTemp );
	}
	else
		BaseFilter_c::Set ( szFilter );
}

const wchar_t *	Filter_c::GetStringToCompare ( const wchar_t * szFile ) const
{
	int iLen = wcslen ( szFile );
	if ( iLen + 1 < MAX_FILTER_LEN )
	{
		wcscpy ( g_dFilter, szFile );
		if ( !m_bCaseSensitive )
			wcslwr ( g_dFilter );

		return g_dFilter;
	}

	return NULL;
}


bool Filter_c::FitsMask ( const wchar_t * szFile, const wchar_t * szMask ) const
{
	Str_c sNameToMatch ( szFile );
	if ( sNameToMatch.Find ( L'.' ) == -1 )
		sNameToMatch += L".";

	return CheckMask ( sNameToMatch, szMask, true );
}


bool Filter_c::CheckMask ( const wchar_t * szFile, const wchar_t * szMask, bool bAcceptEmpty ) const
{
	Assert ( szFile && szMask );

	if ( !*szMask && bAcceptEmpty )
		return true;

	const wchar_t * szFileStart = szFile;
	const wchar_t * szFilterStart = szMask;

	bool bFits = true;
	while ( bFits && *szFilterStart )
	{
		switch ( *szFilterStart )
		{
		case L'?':	// any symbol
			++szFilterStart;
			if ( ! *szFileStart )
				bFits = false;
			else
				++szFileStart;
			break;

		case L'[':	// start of range/set
			++szFilterStart;
			if ( CheckRangeSet ( szFileStart, szFilterStart ) )
				++szFileStart;
			else
				bFits = false;
			break;

		case L'*':
			++szFilterStart;
			return CheckAnySequence ( szFileStart, szFilterStart );

		default:	// just a symbol
			if ( ! *szFileStart )
				bFits = false;
			else
			{
				if ( *szFilterStart != *szFileStart )
					bFits = false;
				else
				{
					++szFilterStart;
					++szFileStart;
				}
			}
			break;
		}
	}

	if ( *szFileStart && ! *szFilterStart )
		return false;

	return bFits;
}


// returns a pointer to a first match of pattern in a file name
const wchar_t * Filter_c::TryToMatch ( const wchar_t * szPattern, int iPatternLen,
									const wchar_t * szFile ) const
{
	// TODO: was there a faster way of matching?
	int iStrLen = wcslen ( szFile );
	const wchar_t * szMatch = NULL;
	for ( int i = 0; i < iStrLen - iPatternLen + 1; ++i )
	{
		bool bFound = true;
		for ( int j = 0; j < iPatternLen && bFound; ++j )
		{
			if ( szFile [i+j] != szPattern [j] )
				bFound = false;
		}

		if ( bFound )
		{
			szMatch = szFile + i;
			break;
		}
	}

	return szMatch;
}


bool Filter_c::CheckAnySequence ( const wchar_t * szFileStart, const wchar_t * szFilterStart ) const
{
	Assert ( szFileStart && szFilterStart );
	const wchar_t * szPattern = szFilterStart;

	// determine pattern to match
	while ( *szPattern && *szPattern != L'*' && *szPattern != L'?' && *szPattern != L'[' )
		++szPattern;

	// match a pattern
	int iPatternLen = szPattern - szFilterStart;
	const wchar_t * szMatch = szFileStart;
	while ( ( szMatch = TryToMatch ( szFilterStart, iPatternLen, szMatch ) ) != NULL )
	{
		if ( CheckMask ( szMatch + iPatternLen, szFilterStart + iPatternLen, false ) )
			return true;

		if ( *szMatch )
			++szMatch;
		else
			break;
	}

	return false;
}