#ifndef _parser_
#define _parser_

#include "main.h"
#include "std.h"

//////////////////////////////////////////////////////////////////////////////////////////
// base parser stuff
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

bool _X_iswspace ( int cSym );

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

	TokenType_e 	ParseNextToken ();							///< parse token and get its type
	
	wchar_t			GetSymbol () const;							///< gets last symbol
	const wchar_t *	GetIdentifier () const;						///< gets last identifier
	const wchar_t * GetString () const;							///< gets last string
	double			GetNumber () const;							///< gets last number

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


//////////////////////////////////////////////////////////////////////////////////////////
// commander. executes commands from a file

// start command declaration
#define CMD_BEGIN(cmd,nargs) \
class ExCmd_##cmd##_c : public ExecutableCommand_c \
	{ \
	public: \
	ExCmd_##cmd##_c () : ExecutableCommand_c(nargs) {} \
	virtual void Execute ( const ArgList_c & tArgs ) \
	{

// end command declaration
#define CMD_END \
	} \
	};

// register a command
#define CMD_REG(cmd)\
	Commander_c::Get ()->RegisterCommand ( new ExCmd_##cmd##_c,L#cmd);


// argument list
class ArgList_c
{
public:
	enum ArgType_e
	{
		 ARG_TYPE_NUMBER
		,ARG_TYPE_STRING
	};
		
					ArgList_c ();

	void			Reset ();
	void			Add ( double fArg );
	void			Add ( const Str_c & sArg );

	bool			GetBool		( int iArg ) const;
	int				GetInt		( int iArg ) const;
	DWORD			GetDWORD	( int iArg ) const;
	float			GetFloat	( int iArg ) const;
	double			GetDouble	( int iArg ) const;
	Str_c	GetString	( int iArg ) const;

	int				GetNumArgs () const;

private:
	struct Arg_t
	{
		ArgType_e	m_eType;
		union
		{
			wchar_t	m_szString [MAX_PARSER_STRING];
			double	m_fNumber;
		};

		Arg_t ()
			: m_fNumber ( 0.0 )
		{
			m_szString [0] = L'\0';
		}


		Arg_t ( ArgType_e eType, const Str_c & sString )
		{
			Assert ( sString.Length () < MAX_PARSER_STRING );

			m_eType = eType;
			wcscpy ( m_szString, sString.c_str () );
		}

		Arg_t ( ArgType_e eType, double	fNumber )
		{
			m_eType = eType;
			m_fNumber = fNumber;
		}
	};

	Array_T <Arg_t> m_dArgs;
};


// base executable command class
class ExecutableCommand_c
{
public:
	explicit			ExecutableCommand_c ( int nArgs );

	virtual void		Execute ( const ArgList_c & tArgs ) = 0;

	int					GetNumArgs () const;

protected:
	int					m_nArgs;			// verification stuff
};


// holds commands
class Commander_c
{
public:
						~Commander_c ();

	void				RegisterCommand ( ExecutableCommand_c * pCommand, const wchar_t * szCommand );
	bool				ExecuteFile ( const wchar_t * szFileName );

	static void			Init ();
	static void			Shutdown ();
	static Commander_c * Get ();

private:
	enum ReadArgResult_e
	{
		 READ_ARG_OK
		,READ_ARG_ERROR
		,READ_ARG_NO_EXECUTE
	};

	typedef Map_T < Str_c, ExecutableCommand_c * > CommandMap_t;

	CommandMap_t		m_tCommandMap;
	ArgList_c			m_tArgList;

	static Commander_c * m_pCommander;

	ReadArgResult_e		ReadCommandArgs ( ExecutableCommand_c * pCommand, const wchar_t * szCommand );	// read command args
	bool				SkipCommand ();			// skips the current command up to ';'
};


//////////////////////////////////////////////////////////////////////////////////////////
// loades/saves pre-registered vars

// callbacks

// for c-str
typedef void			(*LOADCSTR)	( const wchar_t * );
typedef const wchar_t *	(*SAVECSTR)	( int iId );

// for str
typedef void			(*LOADSTR)	( const Str_c & );
typedef const wchar_t *	(*SAVESTR)	( int iId );


// a base load-save var
class SavedVar_c
{
public:
					SavedVar_c ( const wchar_t * szName );

	virtual bool	LoadVar ();
	virtual void	SaveVar ( FILE * pFile ) = 0;

	const Str_c &	GetName () const;

protected:
	Str_c	m_sName;
};


// int type
class SavedVar_Int_c : public SavedVar_c
{
public:
					SavedVar_Int_c ( const wchar_t * szName, int * pField );

	virtual bool	LoadVar ();
	virtual void	SaveVar ( FILE * pFile );

	int				GetValue () const;

private:
	int *			m_pField;
};


// float type
class SavedVar_Float_c : public SavedVar_c
{
public:
					SavedVar_Float_c ( const wchar_t * szName, float * pField );

	virtual bool	LoadVar ();
	virtual void	SaveVar ( FILE * pFile );

	float			GetValue () const;

protected:
	float *			m_pField;
};


// string type
class SavedVar_String_c : public SavedVar_c
{
public:
					SavedVar_String_c ( const wchar_t * szName, Str_c * pField, LOADSTR fnLoad = NULL, SAVESTR fnSave = NULL );

	virtual bool	LoadVar ();
	virtual void	SaveVar ( FILE * pFile );

	const Str_c & GetValue () const;

protected:
	Str_c *	m_pField;
	LOADSTR	m_fnLoad;
	SAVESTR	m_fnSave;
};


// ñ-string type
class SavedVar_CStr_c : public SavedVar_c
{
public:
					SavedVar_CStr_c ( const wchar_t * szName, wchar_t * pField, int iMaxLength, LOADCSTR fnLoad = NULL, SAVECSTR fnSave = NULL );

	virtual bool	LoadVar ();
	virtual void	SaveVar ( FILE * pFile );

	const wchar_t *	GetValue () const;

protected:
	int			m_iMaxLength;
	wchar_t *	m_pField;
	LOADCSTR	m_fnLoad;
	SAVECSTR	m_fnSave;
};


// bool type
class SavedVar_Bool_c : public SavedVar_c
{
public:
					SavedVar_Bool_c ( const wchar_t * szName, bool * pField );

	virtual bool	LoadVar ();
	virtual void	SaveVar ( FILE * pFile );

	bool			 GetValue () const;

protected:
	bool *			m_pField;
};


// dword type
class SavedVar_Dword_c : public SavedVar_c
{
public:
					SavedVar_Dword_c ( const wchar_t * szName, DWORD * pField );

	virtual bool	LoadVar ();
	virtual void	SaveVar ( FILE * pFile );

	DWORD			GetValue () const;

protected:
	DWORD *			m_pField;
};


// variable loader-saver
class VarLoader_c
{
public:
					~VarLoader_c ();

	void			RegisterVar ( wchar_t * szName, int *	pField );
	void			RegisterVar ( wchar_t * szName, float *	pField );
	void			RegisterVar ( wchar_t * szName, Str_c * pField );
	void			RegisterVar ( wchar_t * szName, Str_c * pField, LOADSTR fnLoad, SAVESTR fnSave );
	void			RegisterVar ( wchar_t * szName, wchar_t * pField, int iMaxLength, LOADCSTR fnLoad, SAVECSTR fnSave );
	void			RegisterVar ( wchar_t * szName, bool *	pField );
	void			RegisterVar ( wchar_t * szName, DWORD *	pField );

	bool			LoadVars ( const wchar_t * szFileName );
	void			SaveVars ();

	void			ResetRegistered ();

private:
	typedef Map_T < Str_c, SavedVar_c * >	VarMap_t;
	typedef Array_T < SavedVar_c * >		VarVec_t;

	Str_c			m_sFileName;
	VarVec_t		m_dVars;
	VarMap_t		m_tVarMap;

	void			AddVar ( const wchar_t * szName, SavedVar_c * pVar );
};


//////////////////////////////////////////////////////////////////////////////////////////
// init/shutdown
void Init_Parser ();
void Shutdown_Parser ();

#endif