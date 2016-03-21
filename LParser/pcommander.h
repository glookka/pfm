#ifndef _pcommander_
#define _pcommander_

#include "LCore/cmain.h"
#include "LCore/clog.h"
#include "LCore/carray.h"
#include "LCore/cmap.h"
#include "LParser/pparser.h"

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
			wcscpy ( m_szString, sString );
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


#endif