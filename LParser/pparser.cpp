#include "pch.h"

#include "LParser/pparser.h"
#include "LCore/clog.h"

// greatly simplified funcs
bool _X_iswspace ( wchar_t cSym )
{
	return cSym == L' ' || cSym == L'\t' || cSym == L'\r' || cSym == L'\n' || cSym == 0xFEFF;
}

inline bool _X_iswdigit ( wchar_t cSym )
{
	return cSym >= L'0' && cSym <= L'9';
}

inline bool _X_iswalpha ( wchar_t cSym )
{
	return ( cSym >= L'a' && cSym <= L'z' ) || ( cSym >= L'A' && cSym <= L'Z' );
}

Parser_c * Parser_c::m_pParser = NULL;

Parser_c::Parser_c ()
	: m_pBuffer			( NULL )
{
	Reset ();
}


Parser_c::~Parser_c ()
{
	Reset ();
}


void Parser_c::Init ()
{
	m_pParser = new Parser_c;
}


void Parser_c::Shutdown ()
{
	SafeDelete ( m_pParser );
}


Parser_c * Parser_c::Get ()
{
	Assert ( m_pParser );
	return m_pParser;
}


void Parser_c::Reset ()
{
	m_tState.Reset ();
	SafeDeleteArray ( m_pBuffer );
	m_iBufferPointer = 0;
	m_iBufferSize = 0;
}


bool Parser_c::StartParsing ( const wchar_t * szFileName )
{
	Reset ();

	Log ( L"Parsing [%s]", szFileName );

	HANDLE hFile = CreateFile ( szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
	if ( hFile == INVALID_HANDLE_VALUE )
		return false;

	ULARGE_INTEGER uSize;
	uSize.LowPart = GetFileSize ( hFile, &uSize.HighPart );
	
	if ( uSize.LowPart == 0xFFFFFFFF )
		return false;

	int iSize = (int) uSize.QuadPart;

	if ( iSize > MAX_BUFFER_SIZE )
		return false;

	if ( ( iSize % 2 ) != 0  )
		return false;

	m_iBufferSize = iSize / 2;
	m_pBuffer = new wchar_t [m_iBufferSize];

	DWORD uSizeRead = 0;
	ReadFile ( hFile, m_pBuffer, iSize, &uSizeRead, NULL );
	CloseHandle ( hFile );

	if ( iSize != uSizeRead )
		return false;
	
	return true;
}


void Parser_c::EndParsing ()
{
	Reset ();
}


TokenType_e Parser_c::ParseNextToken ()
{
	SkipSpaces ();

	m_tState.m_eToken = GetNextTokenType ();
	bool bRes = true;

	switch ( m_tState.m_eToken )
	{
		case TOKEN_IDENTIFIER:
			bRes = ParseIdentifier ();
			break;

		case TOKEN_NUMBER:
			bRes = ParseNumber ();
			break;

		case TOKEN_STRING:
			bRes = ParseString ();
			break;

		case TOKEN_SYMBOL:
			bRes = ParseSymbol ();
			break;
	}

	if ( ! bRes  )
		m_tState.m_eToken = TOKEN_ERROR;

	return m_tState.m_eToken;
}


TokenType_e Parser_c::GetNextTokenType ()
{
	wchar_t iSym = m_tState.m_iSym;

	if ( iSym == WEOF )
		return TOKEN_EOF;

	if ( _X_iswdigit ( iSym ) || iSym == L'+' || iSym == L'-' || iSym == L'.' )
		return TOKEN_NUMBER;

	if ( _X_iswalpha ( iSym ) )
		return TOKEN_IDENTIFIER;

	if ( iSym == L'\"' )
		return TOKEN_STRING;

	return TOKEN_SYMBOL;
}

void Parser_c::SkipSpaces ()
{
	bool bSkipLine = false;

	while ( ( _X_iswspace ( m_tState.m_iSym ) || bSkipLine ) && m_tState.m_iSym != WEOF )
	{
		m_tState.m_iSym = GetNextChar ();
		if ( m_tState.m_iSym == L'#' )
			bSkipLine = true;
		else
   			if ( bSkipLine && m_tState.m_iSym == L'\n' )
				bSkipLine = false;
	}
}


bool Parser_c::ParseIdentifier ()
{
	bool bRes = true;
	int iInd = 0;

	do
	{
		m_tState.m_szToken [iInd++] = wchar_t ( m_tState.m_iSym );

		if ( iInd == MAX_PARSER_STRING - 1 )
		{
			Log ( L"Parser_c::ParseIdentifier : Token too long" );
			bRes = false;
			break;
		}
			
		m_tState.m_iSym = GetNextChar ();
	}
	while ( m_tState.m_iSym != WEOF && ( _X_iswalpha ( m_tState.m_iSym ) || _X_iswdigit ( m_tState.m_iSym ) || m_tState.m_iSym == L'_' ) );

	m_tState.m_szToken [iInd] = L'\0';

	return bRes;
}


bool Parser_c::ParseNumber ()
{
	bool bRes = true;
	int iInd = 0;
	wchar_t cSym;

	do
	{
		m_tState.m_szToken [iInd++] = wchar_t ( m_tState.m_iSym );

		if ( iInd == MAX_PARSER_STRING - 1 )
		{
			Log ( L"Parser_c::ParseNumber : Token too long" );
			bRes = false;
			break;
		}

		m_tState.m_iSym = GetNextChar ();
		cSym = m_tState.m_iSym;
	}
	while ( cSym != WEOF && ( _X_iswdigit ( cSym )
		|| cSym == L'.' || ( cSym >= L'a' && cSym <= L'f' )
		|| cSym == L'x' || ( cSym >= L'A' && cSym <= L'F' )
		|| cSym == L'+' || cSym == L'-' ) );

	m_tState.m_szToken [iInd] = L'\0';

	return bRes;
}


bool Parser_c::ParseString ()
{
	bool bRes = true;
	int iInd = 0;
	bool bSpecial = false;

	m_tState.m_iSym = GetNextChar ();

	while ( m_tState.m_iSym != WEOF && ( m_tState.m_iSym != L'\n' && m_tState.m_iSym != L'\"' ) )
	{
		// check for special characters
		if ( m_tState.m_iSym == L'\\' )
		{
			bSpecial = true;

			m_tState.m_iSym = GetNextChar ();
			switch ( m_tState.m_iSym  )
			{
				case L'\\':
					m_tState.m_iSym = L'\\';
					break;
				
				case L'\"':
					m_tState.m_iSym = L'\"';
					break;

				case L'n':
					m_tState.m_iSym = L'\n';
					break;

				default:
					bSpecial = false;
					break;
			}
		}
		else
			bSpecial = false;

		m_tState.m_szToken [iInd++] = wchar_t ( m_tState.m_iSym );

		if ( iInd == MAX_PARSER_STRING - 1 )
		{
			Log ( L"Parser_c::ParseString : Token too long" );
			bRes = false;
			break;
		}

		m_tState.m_iSym = GetNextChar ();
	}

	m_tState.m_szToken [iInd] = L'\0';

	// skip trailing "
	m_tState.m_iSym = GetNextChar ();

	return bRes;
}


bool Parser_c::ParseSymbol ()
{
	m_tState.m_szToken [0] = wchar_t ( m_tState.m_iSym );
	m_tState.m_szToken [1] = L'\0';
	m_tState.m_iSym = GetNextChar ();

	return true;
}


wchar_t Parser_c::GetSymbol ()
{
	Assert ( m_tState.m_eToken == TOKEN_SYMBOL );

	return m_tState.m_szToken [0];
}


Str_c Parser_c::GetIdentifier ()
{
	Assert ( m_tState.m_eToken == TOKEN_IDENTIFIER );

	return m_tState.m_szToken;
}


Str_c Parser_c::GetString ()
{
	Assert ( m_tState.m_eToken == TOKEN_STRING );

	return m_tState.m_szToken;
}


double Parser_c::GetNumber ()
{
	Assert ( m_tState.m_eToken == TOKEN_NUMBER );
	wchar_t * szStop;

	// hex?
	return m_tState.m_szToken [1] == L'x' ? wcstol ( m_tState.m_szToken, &szStop, 16 ) : wcstod ( m_tState.m_szToken, L'\0' );
}


wchar_t Parser_c::GetNextChar ()
{
	if ( m_iBufferPointer == m_iBufferSize )
		return WEOF;

	return m_pBuffer [m_iBufferPointer++];
}

Str_c Parser_c::EncodeSpecialChars ( const wchar_t * szSource )
{
	wchar_t szResult [MAX_PARSER_STRING];

	int iResultIndex = 0;
	for ( int i = 0; i < wcslen ( szSource ); ++i )
	{
		if ( szSource [i] == '\n' || szSource [i] == '\"' || szSource [i] == '\\' )
			szResult [iResultIndex++] = '\\';

		if ( iResultIndex == MAX_PARSER_STRING - 1 )
			break;

		szResult [iResultIndex++] = szSource [i];

		if ( iResultIndex == MAX_PARSER_STRING - 1 )
			break;
	}

	if ( iResultIndex == MAX_PARSER_STRING - 1 )
	{
		Log ( L"Parser_c::EncodeSpecialChars : Source too long" );
		return L"";
	}

	szResult [iResultIndex] = L'\0';
	return szResult;
}