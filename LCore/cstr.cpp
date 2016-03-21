#include "pch.h"

#include "LCore/cstr.h"
#include "LCore/clog.h"

///////////////////////////////////////////////////////////////////////////////////////////
// string stuff
const int	MAX_STRING_LENGTH = 512;		// max string length
wchar_t		g_szWStr [MAX_STRING_LENGTH];	// temp unicode buffer

wchar_t StringDynamic_c::m_cEmpty = L'\0';

StringDynamic_c::StringDynamic_c ()
	: m_szData		( NULL )
	, m_iDataSize	( 0 )
{
}

StringDynamic_c::StringDynamic_c ( const wchar_t * szTxt )
	: m_szData		( NULL )
	, m_iDataSize	( 0 )
{
	if ( ! szTxt || ! szTxt [0] )
		return;

	int iLen = wcslen ( szTxt );

	Grow ( iLen );
	wcscpy ( m_szData, szTxt );
}

StringDynamic_c::StringDynamic_c ( const StringDynamic_c & sStr )
	: m_szData		( NULL )
	, m_iDataSize	( 0 )
{
	operator = ( sStr );
}

StringDynamic_c::StringDynamic_c ( float fValue, const wchar_t * szFormat )
	: m_szData		( NULL )
	, m_iDataSize	( 0 )
{
	Assert ( fValue >= -FLT_MAX && fValue <= FLT_MAX );

	Grow ( MAX_FLOAT_LENGTH );
	if ( szFormat )
		wsprintf ( m_szData, szFormat, fValue );
	else
		wsprintf ( m_szData, L"%f", fValue );
}

StringDynamic_c::StringDynamic_c ( int iValue )
	: m_szData		( NULL )
	, m_iDataSize	( 0 )
{
	Grow ( MAX_INT_LENGTH );
	wsprintf ( m_szData, L"%d", iValue );
}

StringDynamic_c::StringDynamic_c ( wchar_t cChar )
	: m_szData		( NULL )
	, m_iDataSize	( 0 )
{
	Grow ( 1 );

	m_szData [0] = cChar;
	m_szData [1] = L'\0';
}

StringDynamic_c::~StringDynamic_c ()
{
	SafeDeleteArray ( m_szData );
}

const wchar_t * StringDynamic_c::c_str () const
{
	return m_szData ? m_szData : &m_cEmpty;
}

wchar_t & StringDynamic_c::operator [] ( int iIndex )
{
	Assert ( m_szData && iIndex >= 0 && iIndex <= Length () );
	return m_szData [iIndex];
}

StringDynamic_c & StringDynamic_c::operator = ( const wchar_t * szTxt )
{
	if ( szTxt && *szTxt )
	{
		Grow ( wcslen ( szTxt ) );
		wcscpy ( m_szData, szTxt );
	}
	else
		if ( m_szData )
			m_szData [0] = L'\0';

	return *this;
}

StringDynamic_c & StringDynamic_c::operator = ( const StringDynamic_c & sStr )
{
	operator = ( sStr.c_str () );
	return *this;
}

bool StringDynamic_c::operator < ( const StringDynamic_c & sStr ) const
{
	return wcscmp ( m_szData, sStr.c_str () ) < 0;
}

StringDynamic_c & StringDynamic_c::operator += ( const wchar_t * szTxt )
{
	if ( ! szTxt )
		return *this;

	int iOldLength = Length ();
	Grow ( iOldLength + wcslen ( szTxt ), false );

	if ( m_szData )
		wcscpy ( m_szData + iOldLength, szTxt );

	return *this;
}

StringDynamic_c StringDynamic_c::operator + ( const wchar_t * szTxt ) const
{
	if ( ! szTxt )
		return *this;

	StringDynamic_c sTmp ( m_szData );
	sTmp += szTxt ;

	return sTmp;
}

bool StringDynamic_c::operator == ( const wchar_t * szTxt ) const
{
	if ( szTxt == NULL )
		return Length () == 0;

	if ( Length () != wcslen ( szTxt ) )
		return false;

	return ! wcscmp ( c_str (), szTxt );
}

bool StringDynamic_c::operator != ( const wchar_t * szTxt ) const
{
	return ! operator == ( szTxt );
}

StringDynamic_c::operator const wchar_t * () const
{
	return c_str ();
}

int StringDynamic_c::Length () const
{
	return m_szData ? wcslen ( m_szData ) : 0;
}

bool StringDynamic_c::Empty () const
{
	return m_szData ? ( m_szData [0] == L'\0' ) : true;
}

StringDynamic_c StringDynamic_c::SubStr ( int iStart ) const
{
	Assert ( iStart >= 0 );
	if ( iStart >= Length () )
		return StringDynamic_c ();
	else
		return StringDynamic_c ( c_str () + iStart );
}

StringDynamic_c StringDynamic_c::SubStr ( int iStart, int iLength ) const
{
	Assert ( iLength >= 0 );

	StringDynamic_c sTmp ( SubStr ( iStart ) );
	int iTmpLen = sTmp.Length ();
	if ( iTmpLen > 0 )
	{
		int iLen = Min ( iLength, iTmpLen );
		sTmp [iLen] = L'\0';
	}

	return sTmp;
}

StringDynamic_c & StringDynamic_c::ToUpper ()
{
	if ( m_szData )
		_wcsupr ( m_szData );

	return *this;
}

StringDynamic_c & StringDynamic_c::ToLower ()
{
	if ( m_szData )
		_wcslwr ( m_szData );

	return *this;
}

int StringDynamic_c::Find ( const wchar_t * szPattern, int iOffset ) const
{
	if ( ! szPattern || ! m_szData )
		return -1;

	iOffset = Max ( Min ( iOffset, Length () ), 0 );
	const wchar_t * szMatch = wcsstr ( m_szData + iOffset, szPattern );
	return szMatch ? szMatch - m_szData  : -1;
}

int StringDynamic_c::Find ( wchar_t cPattern, int iOffset ) const
{
	if ( ! m_szData )
		return -1;
	
	int iLength = Length ();
	iOffset = Max ( iOffset, 0 );
	for ( int i = iOffset; i < iLength; ++i )
		if ( m_szData [i] == cPattern )
			return i;

	return -1;
}

int StringDynamic_c::RFind ( const wchar_t * szPattern, int iOffset ) const
{
	if ( ! szPattern || ! m_szData )
		return -1;

	int iPatLen = wcslen ( szPattern );

	int iLength = Length ();
	if ( iOffset == -1 )
		iOffset = iLength - 1;

	const wchar_t * szLastMatch = NULL;
	const wchar_t * szMatch = wcsstr ( m_szData, szPattern );
	while ( szMatch && szMatch - m_szData <= iPatLen + iOffset - 1 )
	{
		szLastMatch = szMatch;
		szMatch = wcsstr ( szMatch + 1, szPattern );
	}

	return szLastMatch ? szLastMatch - m_szData : -1;
}

int StringDynamic_c::RFind ( wchar_t cPattern, int iOffset ) const
{
	if ( ! m_szData )
		return -1;

	int iLength = Length ();
	if ( iOffset == -1 )
		iOffset = iLength - 1;
	else
		iOffset = Min ( iLength - 1, iOffset );

	for ( int i = iOffset; i >= 0; --i )
		if ( m_szData [i] == cPattern )
			return i;

	return -1;
}

bool StringDynamic_c::Begins ( wchar_t cPrefix ) const
{
	if ( ! m_szData )
		return false;

	return m_szData [0] == cPrefix;
}

bool StringDynamic_c::Ends ( wchar_t cSuffix ) const
{
	if ( ! m_szData )
		return false;

	return m_szData [Length () - 1] == cSuffix;
}

bool StringDynamic_c::Begins ( const wchar_t * szPrefix ) const
{
	if ( ! szPrefix || ! m_szData )
		return false;

	int iPrefixLen = (int) wcslen ( szPrefix );

	return ( iPrefixLen > Length () ) ? false :
		( _wcsnicmp ( m_szData, szPrefix, iPrefixLen ) == 0 ) ? true : false;
}

bool StringDynamic_c::Ends ( const wchar_t * szSuffix ) const
{
	if ( ! szSuffix || ! m_szData )
		return false;

	int iSuffixLen = wcslen ( szSuffix );
	int iLen = Length();

	return iSuffixLen > iLen ? false : ( _wcsnicmp ( m_szData + iLen - iSuffixLen, szSuffix, iSuffixLen ) == 0 );
}

void StringDynamic_c::LTrim ()
{
	if ( ! m_szData )
		return;

	int iPos = 0, iLen = Length ();

	while ( iPos < iLen && iswspace ( m_szData [iPos] ) ) 
		iPos++;

	if ( iPos < iLen ) 
	{
		wmemmove ( m_szData, m_szData + iPos, iLen - iPos );
		m_szData [ iLen - iPos ] = L'\0';
	}
	else 
		m_szData [0] = L'\0';
}

void StringDynamic_c::RTrim ()
{
	if ( ! m_szData )
		return;

	int iPos = Length();
	do 
	{
		--iPos;
	}
	while ( iPos >= 0 && iswspace ( m_szData [iPos] ) );

	m_szData [ iPos + 1 ] = L'\0';
}

void StringDynamic_c::Trim ()
{
	LTrim();
	RTrim();
}

void StringDynamic_c::Insert ( const wchar_t * szText, int iPos )
{
	if ( ! szText || ! *szText )
		return;

	int iLength = Length ();
	iPos = Clamp ( iPos, 0, iLength );

	int iInsLen = wcslen ( szText );

	Grow ( iLength + iInsLen );
	wmemmove ( m_szData + iPos + iInsLen, m_szData + iPos, iLength - iPos + 1 );
	wmemcpy ( m_szData + iPos, szText, iInsLen );
}

void StringDynamic_c::Erase ( int iPos, int iLen )
{
	if ( ! m_szData )
		return;

	int iLength = Length ();

	if ( iPos >= iLength )
		return;

	if ( iPos + iLen >= iLength )
	{
		m_szData [0] = L'\0';
		return;
	}

	wmemmove ( m_szData + iPos, m_szData + iPos + iLen, iLength - iPos - iLen + 1 );
}

void StringDynamic_c::Replace ( wchar_t cFind, wchar_t cReplace )
{
	if ( ! m_szData )
		return;

	int iLen = Length();

	for ( int i = 0; i < iLen; ++i )
		if ( m_szData [i] == cFind )
			m_szData [i] = cReplace;
}

StringDynamic_c & StringDynamic_c::Chop ( int iCount )
{
	if ( ! m_szData || ! iCount )
		return *this;

	int iLen = Length ();
	iCount = Min ( iCount, iLen );
	m_szData [ iLen-iCount ] = L'\0';

	return *this;
}

void StringDynamic_c::Strip ( wchar_t cChar )
{
	while ( Begins ( cChar ) )
		Erase ( 0, 1 );

	while ( Ends ( cChar ) )
		Chop ( 1 );
}

void StringDynamic_c::Grow ( int nChars, bool bDiscard )
{
	int iNeedSize = nChars + 1;

	if ( iNeedSize > m_iDataSize )
	{
		bool bHadData = false;
		if ( ! bDiscard && m_szData )
		{
			wcscpy ( g_szWStr, m_szData );
			bHadData = true;
		}

		SafeDeleteArray ( m_szData );
		m_szData = new wchar_t [iNeedSize];

		if ( bHadData )
			wcscpy ( m_szData, g_szWStr );

		m_iDataSize = iNeedSize;
	}
}

//////////////////////////////////////////////////////////////////////////

Str_c NewString ( const wchar_t * szFormat, ...  )
{
	va_list argptr;

	va_start ( argptr, szFormat );
	vswprintf ( g_szWStr, szFormat, argptr );
	va_end	 ( argptr );

	return g_szWStr;
}


void UnicodeToAnsi ( const wchar_t * szStr, char * szResult )
{
	WideCharToMultiByte ( CP_ACP, 0, szStr, -1, szResult, MAX_STRING_LENGTH, NULL, NULL );
}

void AnsiToUnicode ( const char * szStr, wchar_t * szResult )
{
	MultiByteToWideChar ( CP_ACP, 0, szStr, -1, szResult, MAX_STRING_LENGTH );
}

GUID StrToGUID ( const wchar_t * szGUID )
{
	GUID guid;
	wchar_t szLast [64];

	swscanf ( szGUID, L"{%X-%X-%X-%s", &guid.Data1, &guid.Data2, &guid.Data3, szLast );

	Str_c sLast ( szLast );
	int iPos = 0;

	while ( ( iPos = sLast.Find ( L'-' ) ) != -1 )
		sLast.Erase ( iPos, 1 );

	if ( sLast.Ends ( L"}" ) )
		sLast.Chop ( 1 );

	for ( int i = 0; i < 8; ++i )
		guid.Data4 [i] = (BYTE) wcstol ( sLast.SubStr ( i*2, 2 ), NULL, 16 );

	return guid;
}