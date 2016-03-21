#include "pch.h"
#include "parser.h"

// greatly simplified funcs
bool _X_iswspace ( int cSym )
{
	return cSym == L' ' || cSym == L'\t' || cSym == L'\r' || cSym == L'\n' || cSym == 0xFEFF;
}

inline bool _X_iswdigit ( int cSym )
{
	return cSym >= L'0' && cSym <= L'9';
}

inline bool _X_iswalpha ( int cSym )
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

	DWORD uSizeDw = (DWORD) uSize.QuadPart;

	if ( uSizeDw > MAX_BUFFER_SIZE )
		return false;

	if ( ( uSizeDw % 2 ) != 0  )
		return false;

	m_iBufferSize = uSizeDw / 2;
	m_pBuffer = new wchar_t [m_iBufferSize];

	DWORD uSizeRead = 0;
	ReadFile ( hFile, m_pBuffer, uSizeDw, &uSizeRead, NULL );
	CloseHandle ( hFile );

	if ( uSizeDw != uSizeRead )
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
	int iSym = m_tState.m_iSym;

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
	int cSym;

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


wchar_t Parser_c::GetSymbol () const
{
	Assert ( m_tState.m_eToken == TOKEN_SYMBOL );
	return m_tState.m_szToken [0];
}


const wchar_t * Parser_c::GetIdentifier () const
{
	Assert ( m_tState.m_eToken == TOKEN_IDENTIFIER );
	return m_tState.m_szToken;
}


const wchar_t * Parser_c::GetString () const
{
	Assert ( m_tState.m_eToken == TOKEN_STRING );
	return m_tState.m_szToken;
}


double Parser_c::GetNumber () const
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
	for ( unsigned int i = 0; i < wcslen ( szSource ); ++i )
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


//////////////////////////////////////////////////////////////////////////////////////////
// commander

ArgList_c::ArgList_c ()
{
	m_dArgs.Reserve ( 16 );
}

void ArgList_c::Reset ()
{
	m_dArgs.Clear ();
}

void ArgList_c::Add ( double fArg )
{
	m_dArgs.Add ( Arg_t ( ARG_TYPE_NUMBER, fArg ) );
}

void ArgList_c::Add ( const Str_c & sArg )
{
	m_dArgs.Add ( Arg_t ( ARG_TYPE_STRING, sArg ) );
}


bool ArgList_c::GetBool	( int iArg ) const
{
	Assert ( m_dArgs [iArg].m_eType == ARG_TYPE_NUMBER );
	return !!static_cast <int> ( m_dArgs [iArg].m_fNumber );
}


int	ArgList_c::GetInt ( int iArg ) const
{
	Assert ( m_dArgs [iArg].m_eType == ARG_TYPE_NUMBER );
	return static_cast <int> ( m_dArgs [iArg].m_fNumber );
}


DWORD ArgList_c::GetDWORD ( int iArg ) const
{
	Assert ( m_dArgs [iArg].m_eType == ARG_TYPE_NUMBER );
	return static_cast <DWORD> ( m_dArgs [iArg].m_fNumber );
}


float ArgList_c::GetFloat ( int iArg ) const
{
	Assert ( m_dArgs [iArg].m_eType == ARG_TYPE_NUMBER );
	return static_cast <float> ( m_dArgs [iArg].m_fNumber );
}


double ArgList_c::GetDouble	( int iArg ) const
{
	Assert ( m_dArgs [iArg].m_eType == ARG_TYPE_NUMBER );
	return m_dArgs [iArg].m_fNumber;
}


Str_c ArgList_c::GetString ( int iArg ) const
{
	Assert ( m_dArgs [iArg].m_eType == ARG_TYPE_STRING );
	return m_dArgs [iArg].m_szString;
}


int	ArgList_c::GetNumArgs () const
{
	return m_dArgs.Length ();
}


// the command
ExecutableCommand_c::ExecutableCommand_c ( int nArgs )
	: m_nArgs ( nArgs )
{
}


int ExecutableCommand_c::GetNumArgs () const
{
	return m_nArgs;
}

// the commander
Commander_c * Commander_c::m_pCommander = NULL;

Commander_c::~Commander_c ()
{
	// reset the map
	for ( int i = 0; i < m_tCommandMap.Length (); ++i )
		delete m_tCommandMap [i];
}


void Commander_c::RegisterCommand ( ExecutableCommand_c * pCommand, const wchar_t * szCommand )
{
	Str_c sCmd = szCommand;
	sCmd.ToLower ();
	Verify ( m_tCommandMap.Add ( sCmd, pCommand ) );
}


bool Commander_c::SkipCommand ()
{
	Parser_c * pParser = Parser_c::Get ();
	TokenType_e eToken;
	do 
	{
		 eToken = pParser->ParseNextToken ();
	}
	while ( eToken == TOKEN_STRING || eToken == TOKEN_NUMBER );

	bool bSkipFailed = false;
	if ( eToken == TOKEN_SYMBOL )
	{
		wchar_t cSym = pParser->GetSymbol ();
		if ( cSym != L';' )
			bSkipFailed = true;
	}
	else
		bSkipFailed = true;

	if ( bSkipFailed )
		Log ( L"Skip failed" );

	return !bSkipFailed;
}


Commander_c::ReadArgResult_e Commander_c::ReadCommandArgs ( ExecutableCommand_c * pCommand, const wchar_t * szCommand )
{
	Parser_c * pParser = Parser_c::Get ();

	m_tArgList.Reset ();
	int nCommandArgs = pCommand->GetNumArgs ();

	bool bCommandFinished = false;
	TokenType_e eToken;

	do
	{
		eToken = pParser->ParseNextToken ();
		switch ( eToken )
		{
		case TOKEN_STRING:
			m_tArgList.Add ( pParser->GetString () );
			break;

		case TOKEN_NUMBER:
			m_tArgList.Add ( pParser->GetNumber () );
			break;

		case TOKEN_SYMBOL:
			{
				wchar_t cChar = pParser->GetSymbol ();
				if ( cChar != L';' )
				{
					Log ( L"';' expected in [%s]", szCommand );
					return READ_ARG_ERROR;
				}
				else
					bCommandFinished = true;
			}
			break;

		default:
			return READ_ARG_ERROR;
		}

		if ( m_tArgList.GetNumArgs () > nCommandArgs )
		{
			Log ( L"Too many args in [%s]. Skipping", szCommand );
			return SkipCommand () ? READ_ARG_NO_EXECUTE : READ_ARG_ERROR;
		}
	}
	while ( eToken == TOKEN_STRING || eToken == TOKEN_NUMBER );

	if ( bCommandFinished && m_tArgList.GetNumArgs () < nCommandArgs )
	{
		Log ( L"Not enough args in [%s]. Skipping", szCommand );
		return READ_ARG_NO_EXECUTE;
	}

	return READ_ARG_OK;
}


bool Commander_c::ExecuteFile ( const wchar_t * szFileName )
{
	Parser_c * pParser = Parser_c::Get ();

	if ( ! pParser->StartParsing ( szFileName ) )
		return false;

	TokenType_e eToken;
	Str_c sCommand;
	bool bParseOk = true;

	do
	{
		eToken = pParser->ParseNextToken ();
		if ( eToken == TOKEN_EOF )
			break;

		if ( eToken != TOKEN_IDENTIFIER )
		{
			Log ( L"Identifier expected" );
			bParseOk = false;
			break;
		}
		
		sCommand = pParser->GetIdentifier ();
		ExecutableCommand_c ** ppCommand = m_tCommandMap.Find ( sCommand );
		if ( ! ppCommand )
		{
			Log ( L"Unknown command [%s]. Skipping", sCommand.c_str () );
			if ( ! SkipCommand () )
				bParseOk = false;
		}
		else
		{
			ReadArgResult_e eReadRes = ReadCommandArgs ( *ppCommand, sCommand );

			switch ( eReadRes )
			{
			case READ_ARG_OK:
				(*ppCommand)->Execute ( m_tArgList );
				break;
			case READ_ARG_ERROR:
				Log ( L"Error reading args for [%s]", sCommand.c_str () );
				bParseOk = false;
				break;
			}
		}
	}
	while ( ( eToken != TOKEN_EOF || eToken != TOKEN_ERROR ) && bParseOk );

	pParser->EndParsing ();

	return bParseOk;
}


void Commander_c::Init ()
{
	Assert ( ! m_pCommander );
	m_pCommander = new Commander_c;
}


void Commander_c::Shutdown ()
{
	SafeDelete ( m_pCommander );
}

Commander_c * Commander_c::Get ()
{
	Assert ( m_pCommander );
	return m_pCommander;
}


//////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
// base saved var
SavedVar_c::SavedVar_c ( const wchar_t * szName )
	: m_sName ( szName )
{
}


bool SavedVar_c::LoadVar ()
{
	return false;
}


const Str_c & SavedVar_c::GetName () const
{
	return m_sName;
}


///////////////////////////////////////////////////////////////////////////////////////////
// int
SavedVar_Int_c::SavedVar_Int_c ( const wchar_t * szName, int * pField )
	: SavedVar_c	( szName )
	, m_pField		( pField )
{
}


bool SavedVar_Int_c::LoadVar ()
{
	Parser_c * pParser = Parser_c::Get ();
	TokenType_e eToken = pParser->ParseNextToken ();

	if ( eToken != TOKEN_NUMBER )
		return false;

	Assert ( m_pField );
	*m_pField = (int) pParser->GetNumber ();

	return true;
}


void SavedVar_Int_c::SaveVar ( FILE * pFile )
{
	Assert ( pFile && m_pField );
	fwprintf ( pFile, L"%s = %d;\r\n", m_sName.c_str (), *m_pField );
}


int SavedVar_Int_c::GetValue () const
{
	return *m_pField;
}


///////////////////////////////////////////////////////////////////////////////////////////
// float
SavedVar_Float_c::SavedVar_Float_c ( const wchar_t * szName, float * pField )
	: SavedVar_c 	( szName )
	, m_pField		( pField )
{
}


bool SavedVar_Float_c::LoadVar ()
{
	Parser_c * pParser = Parser_c::Get ();
	TokenType_e eToken = pParser->ParseNextToken ();

	if ( eToken != TOKEN_NUMBER )
		return false;

	Assert ( m_pField );
	*m_pField = (float) pParser->GetNumber ();

	return true;

}


void SavedVar_Float_c::SaveVar ( FILE * pFile )
{
	Assert ( pFile && m_pField );
	fwprintf ( pFile, L"%s = %.5f;\r\n", m_sName.c_str (), *m_pField );
}


float SavedVar_Float_c::GetValue () const
{
	return *m_pField;
}


///////////////////////////////////////////////////////////////////////////////////////////
// string type
SavedVar_String_c::SavedVar_String_c ( const wchar_t * szName, Str_c * pField, LOADSTR fnLoad, SAVESTR fnSave )
	: SavedVar_c	( szName )
	, m_pField		( pField )
	, m_fnLoad		( fnLoad )
	, m_fnSave		( fnSave )
{
}


bool SavedVar_String_c::LoadVar ()
{
	Parser_c * pParser = Parser_c::Get ();
	TokenType_e eToken = pParser->ParseNextToken ();

	if ( eToken != TOKEN_STRING )
		return false;

	if ( m_fnLoad )
		m_fnLoad ( pParser->GetString () );

	if ( m_pField )
		*m_pField = pParser->GetString ();

	return true;
}


void SavedVar_String_c::SaveVar ( FILE * pFile )
{
	Assert ( pFile && m_pField );

	if ( m_fnSave )
	{
		const wchar_t * szText;
		int iVar = 0;
		while ( ( szText = m_fnSave ( iVar++ ) ) != NULL )
			fwprintf ( pFile, L"%s = \"%s\";\r\n", m_sName.c_str (), Parser_c::EncodeSpecialChars ( szText ).c_str () );
	}

	if ( m_pField )
		fwprintf ( pFile, L"%s = \"%s\";\r\n", m_sName.c_str (), Parser_c::EncodeSpecialChars ( *m_pField ).c_str () );
}


const Str_c & SavedVar_String_c::GetValue () const
{
	return *m_pField;
}

///////////////////////////////////////////////////////////////////////////////////////////
// c-string
SavedVar_CStr_c::SavedVar_CStr_c ( const wchar_t * szName, wchar_t * pField, int iMaxLength, LOADCSTR fnLoad, SAVECSTR fnSave )
	: SavedVar_c	( szName )
	, m_pField		( pField )
	, m_iMaxLength	( iMaxLength )
	, m_fnLoad		( fnLoad )
	, m_fnSave		( fnSave )
{
}


bool SavedVar_CStr_c::LoadVar ()
{
	Parser_c * pParser = Parser_c::Get ();
	if ( pParser->ParseNextToken () != TOKEN_STRING )
		return false;

	if ( m_fnLoad )
		m_fnLoad ( pParser->GetString () );

	if ( m_pField )
	{
		wcsncpy ( m_pField, pParser->GetString (), m_iMaxLength );
		m_pField [m_iMaxLength-1] = L'\0';
	}

	return true;
}


void SavedVar_CStr_c::SaveVar ( FILE * pFile )
{
	Assert ( pFile );
	if ( m_fnSave )
	{
		const wchar_t * szText;
		int iVar = 0;
		while ( ( szText = m_fnSave ( iVar++ ) ) != NULL )
			fwprintf ( pFile, L"%s = \"%s\";\r\n", m_sName.c_str (), Parser_c::EncodeSpecialChars ( szText ).c_str () );
	}

	if ( m_pField )
		fwprintf ( pFile, L"%s = \"%s\";\r\n", m_sName.c_str (), Parser_c::EncodeSpecialChars ( m_pField ).c_str () );
}


const wchar_t * SavedVar_CStr_c::GetValue () const
{
	return m_pField;
}

///////////////////////////////////////////////////////////////////////////////////////////
// bool
SavedVar_Bool_c::SavedVar_Bool_c ( const wchar_t * szName, bool * pField )
	: SavedVar_c	( szName )
	, m_pField		( pField )
{
}


bool SavedVar_Bool_c::LoadVar ()
{
	Parser_c * pParser = Parser_c::Get ();
	TokenType_e eToken = pParser->ParseNextToken ();

	if ( eToken != TOKEN_IDENTIFIER )
		return false;

	Str_c sIdent = pParser->GetIdentifier ();
	bool bValue;

	if ( sIdent == L"true" )
		bValue = true;
	else
	{
		if ( sIdent == L"false" )
			bValue = false;
		else
			return false;
	}
	
	Assert ( m_pField );
	*m_pField = bValue;

	return true;
}


void SavedVar_Bool_c::SaveVar ( FILE * pFile )
{
	Assert ( pFile && m_pField );
	fwprintf ( pFile, L"%s = %s;\r\n", m_sName.c_str (), *m_pField ? L"true" : L"false" );
}


bool SavedVar_Bool_c::GetValue () const
{
	return *m_pField;
}

///////////////////////////////////////////////////////////////////////////////////////////
// int
SavedVar_Dword_c::SavedVar_Dword_c ( const wchar_t * szName, DWORD * pField )
	: SavedVar_c	( szName )
	, m_pField		( pField )
{
}


bool SavedVar_Dword_c::LoadVar ()
{
	Parser_c * pParser = Parser_c::Get ();
	TokenType_e eToken = pParser->ParseNextToken ();

	if ( eToken != TOKEN_NUMBER )
		return false;

	Assert ( m_pField );
	*m_pField = (DWORD) pParser->GetNumber ();

	return true;
}


void SavedVar_Dword_c::SaveVar ( FILE * pFile )
{
	Assert ( pFile && m_pField );
	fwprintf ( pFile, L"%s = 0x%X;\r\n", m_sName.c_str (), *m_pField );
}


DWORD SavedVar_Dword_c::GetValue () const
{
	return *m_pField;
}


///////////////////////////////////////////////////////////////////////////////////////////
// the loader/saver
VarLoader_c::~VarLoader_c ()
{
	ResetRegistered ();
}


void VarLoader_c::AddVar ( const wchar_t * szName, SavedVar_c * pVar )
{
	m_dVars.Add ( pVar );
	Verify ( m_tVarMap.Add ( szName, pVar ) );
}

void VarLoader_c::RegisterVar ( wchar_t * szName, int * pField )
{
	AddVar ( szName, new SavedVar_Int_c ( szName, pField ) );
}


void VarLoader_c::RegisterVar ( wchar_t * szName, float * pField )
{
	AddVar ( szName, new SavedVar_Float_c ( szName, pField ) );
}


void VarLoader_c::RegisterVar ( wchar_t * szName, Str_c * pField )
{
	AddVar ( szName, new SavedVar_String_c ( szName, pField, NULL, NULL ) );
}


void VarLoader_c::RegisterVar ( wchar_t * szName, Str_c * pField, LOADSTR fnLoad, SAVESTR fnSave )
{
	AddVar ( szName, new SavedVar_String_c ( szName, pField, fnLoad, fnSave ) );
}


void VarLoader_c::RegisterVar ( wchar_t * szName, wchar_t * pField, int iMaxLength, LOADCSTR fnLoad, SAVECSTR fnSave )
{
	AddVar ( szName, new SavedVar_CStr_c ( szName, pField, iMaxLength, fnLoad, fnSave ) );
}


void VarLoader_c::RegisterVar ( wchar_t * szName, bool * pField )
{
	AddVar ( szName, new SavedVar_Bool_c ( szName, pField ) );
}


void VarLoader_c::RegisterVar ( wchar_t * szName, DWORD * pField )
{
	AddVar ( szName, new SavedVar_Dword_c ( szName, pField ) );
}


bool VarLoader_c::LoadVars ( const wchar_t * szFileName )
{
	m_sFileName = szFileName;

	// load the file
	Parser_c * pParser = Parser_c::Get ();

	if ( ! pParser->StartParsing ( szFileName ) )
		return false;

	bool bParseOk = true;
	Str_c sCommand;
	TokenType_e eToken;

	do
	{
		eToken = pParser->ParseNextToken ();

		if ( eToken == TOKEN_EOF )
			break;

		if ( eToken != TOKEN_IDENTIFIER )
		{
			Log ( L"VarLoader_c::LoadAllVars : identifier expected in [%s]!", m_sFileName.c_str () );
			bParseOk = false;
			break;
		}

		SavedVar_c ** ppVar = NULL;

		sCommand = pParser->GetIdentifier ();
		ppVar = m_tVarMap.Find ( sCommand );

		// skip to '='
		wchar_t cSym = L' ';
		eToken = pParser->ParseNextToken ();
		if ( eToken == TOKEN_SYMBOL )
			cSym = pParser->GetSymbol ();

		if ( eToken != TOKEN_SYMBOL || cSym != L'=' )
		{
			Log ( L"VarLoader_c::LoadAllVars : '=' expected after [%s] in file [%s]", sCommand.c_str (), m_sFileName.c_str () );
			bParseOk = false;
		}

		if ( bParseOk )
		{
			if ( ! ppVar )
			{
				Log ( L"VarLoader_c::LoadAllVars : var [%s] not found in file [%s]", sCommand.c_str (), m_sFileName.c_str () );

				// skip the var value
				eToken = pParser->ParseNextToken ();
			}
			else
				if ( ! (*ppVar)->LoadVar () )
				{
					Log ( L"VarLoader_c::LoadAllVars : var load failed [%s] in file [%s]", sCommand.c_str (), m_sFileName.c_str () );
					bParseOk = false;
				}
		}

		if ( bParseOk )
		{
			// skip to next ';'
			eToken = pParser->ParseNextToken ();
			if ( eToken == TOKEN_SYMBOL )
				cSym = pParser->GetSymbol ();

			if ( eToken != TOKEN_SYMBOL || cSym != L';' )
			{
				Log ( L"VarLoader_c::LoadAllVars : ';' expected after [%s] in file [%s]", sCommand.c_str (), m_sFileName.c_str () );
				bParseOk = false;
			}
		}
	}
	while ( ( eToken != TOKEN_EOF || eToken != TOKEN_ERROR ) && bParseOk );

	if ( eToken == TOKEN_ERROR || ! bParseOk )
		Log ( L"VarLoader_c::LoadAllVars : error parsing file [%s]", m_sFileName.c_str () );

	pParser->EndParsing ();

	return eToken != TOKEN_ERROR && bParseOk;
}

void VarLoader_c::SaveVars ()
{
	FILE * pFile = _wfopen ( m_sFileName, L"wb" );
	unsigned short uSign = 0xFEFF;
	fwrite ( &uSign, sizeof ( uSign ), 1, pFile );

	if ( pFile )
	{
		for ( int i = 0; i < m_dVars.Length (); ++i )
			m_dVars [i]->SaveVar ( pFile );
	}
	else
		Log ( L"VarLoader_c::SaveVars  : can't open [%s] for writing", m_sFileName.c_str () );

	fclose ( pFile );
}

void VarLoader_c::ResetRegistered ()
{
	for ( int i = 0; i < m_dVars.Length (); ++i )
		delete m_dVars [i];

	m_dVars.Clear ();
	m_tVarMap.Clear ();
}

//////////////////////////////////////////////////////////////////////////////////////////
// init/shutdown
void Init_Parser ()
{
	Parser_c::Init ();
	Commander_c::Init ();
}


void Shutdown_Parser ()
{
	Commander_c::Shutdown ();
	Parser_c::Shutdown ();
}
