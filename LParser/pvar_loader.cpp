#include "pch.h"

#include "LParser/pvar_loader.h"
#include "LParser/pparser.h"
#include "LCore/clog.h"

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
// comment
SavedVar_Comment_c::SavedVar_Comment_c ( const wchar_t * szComment )
	: SavedVar_c ( szComment )
{
}


void SavedVar_Comment_c::SaveVar ( FILE * pFile )
{
	Assert ( pFile );
	fwprintf ( pFile, L"# %s\r\n", m_sName.c_str () );
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


void SavedVar_Int_c::SetValue ( int iValue ) const
{
	*m_pField = iValue;
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
SavedVar_String_c::SavedVar_String_c ( const wchar_t * szName, Str_c * pField )
	: SavedVar_c	( szName )
	, m_pField		( pField )
{
}


bool SavedVar_String_c::LoadVar ()
{
	Parser_c * pParser = Parser_c::Get ();
	TokenType_e eToken = pParser->ParseNextToken ();

	if ( eToken != TOKEN_STRING )
		return false;

	*m_pField = pParser->GetString ();
	return true;
}


void SavedVar_String_c::SaveVar ( FILE * pFile )
{
	Assert ( pFile && m_pField );
	fwprintf ( pFile, L"%s = \"%s\";\r\n", m_sName.c_str (), Parser_c::EncodeSpecialChars ( *m_pField ).c_str () );
}


const Str_c & SavedVar_String_c::GetValue () const
{
	return *m_pField;
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

void VarLoader_c::AddComment ( wchar_t * szComment )
{
	m_dVars.Add ( new SavedVar_Comment_c ( szComment ) );
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
	AddVar ( szName, new SavedVar_String_c ( szName, pField ) );
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