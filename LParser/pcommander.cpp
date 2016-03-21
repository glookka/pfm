#include "pch.h"

#include "LParser/pcommander.h"

Commander_c * Commander_c::m_pCommander = NULL;

///////////////////////////////////////////////////////////////////////////////////////////
// the argument list
ArgList_c::ArgList_c ()
{
	m_dArgs.Resize ( 16 );
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


///////////////////////////////////////////////////////////////////////////////////////////
// the command
ExecutableCommand_c::ExecutableCommand_c ( int nArgs )
	: m_nArgs ( nArgs )
{
}


int ExecutableCommand_c::GetNumArgs () const
{
	return m_nArgs;
}

///////////////////////////////////////////////////////////////////////////////////////////
// the commander
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
