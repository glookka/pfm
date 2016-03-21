#include "pch.h"

#include "LFile/fquicksearch.h"
#include "LCore/clog.h"


StringSearch_c::StringSearch_c ( const wchar_t * szPattern, bool bMatchCase, bool bWholeWord )
	: m_iPatLen 	( wcslen ( szPattern ) )
	, m_iMin		( szPattern [0] )
	, m_pFile		( NULL )
	, m_bEndReached	( false )
	, m_bMatchCase	( bMatchCase )
	, m_bWholeWord	( bWholeWord )
	, m_iNextOffset	( 0 )
{
	Assert ( ! ( READ_BUFFER_SIZE & 1 ) );
	Assert ( m_iPatLen < READ_BUFFER_SIZE );
	m_iMax = m_iMin;

	const wchar_t * szStart = szPattern;
	while ( *szStart )
	{
		if ( *szStart > m_iMax )
			m_iMax = *szStart;
		else 
			if ( *szStart < m_iMin )
				m_iMin = *szStart;

		++szStart;
	}

	int iSize = m_iMax - m_iMin + 1;
	m_dBadChar = new unsigned short [iSize];
	wmemset ( (wchar_t*)m_dBadChar, m_iPatLen + 1, iSize );

	m_szPattern = new wchar_t [ wcslen ( szPattern ) + 1 ];
	wcscpy ( m_szPattern, szPattern );
	
	if ( ! m_bMatchCase )
		for ( int i = 0; i < m_iPatLen; ++i )
			m_szPattern [i] = towlower ( m_szPattern [i] );

	for ( int i = 0; i < m_iPatLen; ++i )
		m_dBadChar [ szPattern [i] - m_iMin ] = m_iPatLen - i;
}


StringSearch_c::~StringSearch_c ()
{
	delete [] m_dBadChar;
	delete [] m_szPattern;
}


bool StringSearch_c::StartFile ( const wchar_t * szFile )
{
	Assert ( !m_pFile );
	m_iNextOffset = 0;
	m_pFile = _wfopen ( szFile, L"rb" );
	return !!m_pFile;
}


void StringSearch_c::EndOfFile ()
{
	fclose ( m_pFile );
	m_pFile = NULL;
	m_bEndReached = true;
}


bool StringSearch_c::CheckWholeWord ( wchar_t * pBuffer, int iIndex, int nSymbolsToTest ) const
{
	if ( ! m_bWholeWord )
		return true;
	
	bool bFrontOk = false;
	bool bEndOk = false;

	if ( iIndex == 0 )
		bFrontOk = true;
	
	if ( m_bEndReached && iIndex + m_iPatLen == nSymbolsToTest )
		bEndOk = true;

	if ( ! bFrontOk )
		bFrontOk = !!iswspace ( pBuffer [iIndex - 1] );

	if ( ! bEndOk )
		bEndOk = !!iswspace ( pBuffer [iIndex + m_iPatLen ] );

	return bEndOk && bFrontOk;
}


bool StringSearch_c::Test ( wchar_t * pBuffer, int nSymbolsToTest, int & iNextOffset )
{
	int iIndex = 0;
	while ( iIndex <= nSymbolsToTest - m_iPatLen )
	{
		if ( !m_bMatchCase )
			for ( int i = 0; i < m_iPatLen; ++i )
				pBuffer [iIndex + i] = towlower ( pBuffer [iIndex + i] );

		if ( !wmemcmp ( &pBuffer [iIndex], m_szPattern, m_iPatLen ) )
		{	
			if ( CheckWholeWord ( pBuffer, iIndex, nSymbolsToTest ) )
				return true;
		}

		wchar_t iTmpIndex = pBuffer [iIndex + m_iPatLen];
		int iShift = ( iTmpIndex > m_iMax || iTmpIndex < m_iMin ) ? m_iPatLen + 1 : m_dBadChar [ iTmpIndex - m_iMin ];
		iIndex += iShift;
	 }

	iNextOffset = iIndex - nSymbolsToTest;

	return false;
}


bool StringSearch_c::DoSearch ()
{
	Assert ( m_pFile );

	int nBytesToRead = READ_BUFFER_SIZE;
	int iBufStart = 0;

	if ( m_iNextOffset > 0 )
		fseek ( m_pFile, m_iNextOffset, SEEK_CUR );
	else
		if ( m_iNextOffset < 0 )
		{
			nBytesToRead += m_iNextOffset * 2;
			iBufStart -= m_iNextOffset * 2;
		}

	int nBytesRead = fread ( m_dBuffer + iBufStart, 1, nBytesToRead, m_pFile );
	if ( nBytesRead < nBytesToRead )
		m_bEndReached = true;

	int iTestOffset = m_iNextOffset == 0 ? 0 : 1;
	
	if ( Test ( (wchar_t *)m_dBuffer + iTestOffset, nBytesRead / 2, m_iNextOffset ) )
	{
		EndOfFile ();
		return true;
	}

	if ( ! m_bEndReached )
	{
		--m_iNextOffset;
		if ( m_iNextOffset < 0 )
		{
			for ( int i = 0; i < -m_iNextOffset; ++i )
				m_dBuffer [i] = m_dBuffer [nBytesRead + m_iNextOffset + i];
		}
	}
	else
		EndOfFile ();

	return false;
}


bool StringSearch_c::IsFileFinished () const
{
	return m_bEndReached;
}