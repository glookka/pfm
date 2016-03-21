#ifndef _pparser_
#define _pparser_

#include "LCore/cmain.h"
#include "LCore/cstr.h"

enum TokenType_e
{
	 TOKEN_ERROR
	,TOKEN_EOF
	,TOKEN_SYMBOL
	,TOKEN_NUMBER
	,TOKEN_STRING
	,TOKEN_IDENTIFIER
};


enum
{
	 MAX_PARSER_STRING		= 512			// max string length we can parse
};

bool _X_iswspace ( wchar_t cSym );

struct ParserState_t
{
	int			m_iSym;								///< last symbol read
	TokenType_e m_eToken;							///< last token
	wchar_t		m_szToken [MAX_PARSER_STRING];		///< token storage buffer
	long		m_iPos;								///< file pos
	int			m_iStateId;							///< state id

	ParserState_t ()
	{
		Reset ();
	}

	void Reset ()
	{
		m_iSym = L' ';
		m_eToken = TOKEN_ERROR;
		m_szToken [0] = L'\0';
		m_iPos = 0;
		m_iStateId = -1;
	}
};


class Parser_c
{
public:
				Parser_c ();									///< ctor
				~Parser_c ();									///< dtor

	static void			Init ();								///< init stuff
	static void			Shutdown ();							///< cleanup stuff
	static Parser_c *	Get ();									///< get me!

	bool			StartParsing ( const wchar_t * szFileName );	///< set current file
	void			EndParsing ();									///< finish parsing

	TokenType_e 	ParseNextToken ();								///< parse token and get its type
	
	wchar_t			GetSymbol ();									///< gets last symbol
	Str_c			GetIdentifier ();								///< gets last identifier
	Str_c			GetString ();									///< gets last string
	double			GetNumber ();									///< gets last number

	static Str_c 	EncodeSpecialChars ( const wchar_t * szSource );///< encode special chars

private:
	static const int MAX_BUFFER_SIZE = 102400;			///< 100 kb files are max

	wchar_t *		m_pBuffer;
	int				m_iBufferPointer;
	int				m_iBufferSize;

	ParserState_t 	m_tState;

	static Parser_c * m_pParser;					///< singleton stuff

	void			Reset ();							///< resets all inner stuff
	void			SkipSpaces ();						///< skips spaces. reads first char after spaces
	TokenType_e 	GetNextTokenType ();				///< determines next token type

	bool			ParseIdentifier ();					///< parse identifier into a buffer
	bool			ParseNumber ();						///< parse a number. warning: almost no type checks...
	bool			ParseString ();						///< parse a string
	bool			ParseSymbol ();						///< parse a symbol

	wchar_t			GetNextChar ();						///< return a char and move to the next one
};


#endif