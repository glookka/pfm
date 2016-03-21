#ifndef _config_
#define _config_

#include "main.h"
#include "system.h"
#include "parser.h"

//////////////////////////////////////////////////////////////////////////////////////////
// main config
class Config_c : public VarLoader_c
{
public:
#ifdef VAR
#undef VAR
#endif

#ifdef COMMENT
#undef COMMENT
#endif

#define VAR(type,var_name,value) type var_name;
#define COMMENT(value)
#include "config.inc"

						Config_c ();

	static void			Init ()		{ Assert (!m_pSingleton); m_pSingleton = new Config_c; }
	static void			Shutdown ()	{ SafeDelete ( m_pSingleton); }
	static Config_c *	Get ()		{ Assert (m_pSingleton); return m_pSingleton; }

private:
	static Config_c *	m_pSingleton;
};


#define cfg Config_c::Get()


//////////////////////////////////////////////////////////////////////////////////////////
// colors
class Colors_c : public VarLoader_c
{
public:
#ifdef VAR
#undef VAR
#endif

#ifdef COMMENT
#undef COMMENT
#endif

#define VAR(type,var_name,value) type var_name;
#define COMMENT(value)
#include "colors.inc"

						Colors_c ();

	static void			Init ()		{ Assert (!m_pSingleton); m_pSingleton = new Colors_c; }
	static void			Shutdown ()	{ SafeDelete ( m_pSingleton); }
	static Colors_c *	Get ()		{ Assert (m_pSingleton); return m_pSingleton; }

private:
	static Colors_c *	m_pSingleton;
};

#define clrs Colors_c::Get()

//////////////////////////////////////////////////////////////////////////////////////////
// buttons
enum BtnAction_e
{
	 BA_NONE = -1
	,BA_EXIT = 0
	,BA_MINIMIZE
	,BA_MOVEUP
	,BA_MOVEDOWN
	,BA_MOVELEFT
	,BA_MOVERIGHT
	,BA_MOVEHOME
	,BA_MOVEEND
	,BA_PREVDIR
	,BA_EXECUTE
	,BA_OPENWITH
	,BA_SEND_BT
	,BA_SEND_IR
	,BA_OPEN_IN_OPPOSITE
	,BA_SAME_AS_OPPOSITE
	,BA_SWITCH_PANES
	,BA_MAXIMIZE_PANE
	,BA_REFRESH_PANES
	,BA_DRIVE_LIST
	,BA_FILE_MENU
	,BA_HEADER_MENU
	,BA_TGL_FULLSCREEN
	,BA_TGL_FASTSEL
	,BA_TGL_FASTNAV
	,BA_TGL_2NDBAR
	,BA_OP_VIEW
	,BA_OP_COPYMOVE
	,BA_OP_RENAME
	,BA_OP_DELETE
	,BA_OP_SHORTCUT
	,BA_OP_MKDIR
	,BA_OP_CLIPCOPY
	,BA_OP_CLIPCUT
	,BA_OP_CLIPPASTE
	,BA_OP_PROPS
	,BA_OP_SEARCH
	,BA_FL_SELECT_ALL
	,BA_FL_SELECT_NONE
	,BA_FL_INVERT
	,BA_FL_SELECT_FILTER
	,BA_FL_CLEAR_FILTER
	,BA_FL_TOGGLE
	,BA_FL_TOGGLE_AND_MOVE
	,BA_BM_ADD
	,BA_OPTIONS
	,BA_TOTAL
};

class Buttons_c
{
public:
					Buttons_c ();

	void			Load ( const Str_c & sFileName );
	void			Save ();

	void			SetButton ( BtnAction_e eAction, int iBtnIndex, int iKey, bool bLong );
	void			SetLong ( BtnAction_e eAction, int iBtnIndex, bool bLong );

	const wchar_t *	GetLitName ( BtnAction_e eAction ) const;
	int				GetKey ( BtnAction_e eAction, int iBtnIndex ) const;
	bool			GetLong ( BtnAction_e eAction, int iBtnIndex ) const;

	Str_c			GetKeyLitName ( int iKey ) const;

	BtnAction_e		GetAction ( int iKey, btns::Event_e eEvent );

	void			RegisterHotKeys ( HWND hWnd );
	void			UnregisterHotKeys ( HWND hWnd );

	static void			Init ()		{ Assert (!m_pSingleton); m_pSingleton = new Buttons_c; }
	static void			Shutdown ()	{ SafeDelete ( m_pSingleton); }
	static Buttons_c *	Get ()		{ Assert (m_pSingleton); return m_pSingleton; }

private:
	static Buttons_c *	m_pSingleton;

	bool			m_bChangeFlag;
	Str_c			m_sFileName;

	struct ConfigBtn_c
	{
		int		m_iKey;
		bool	m_bLong;
	};

	static const int BTNS_PER_ACTION = 3;

	ConfigBtn_c		m_dCfgBtns [BA_TOTAL][BTNS_PER_ACTION];
};

#define buttons Buttons_c::Get()


//////////////////////////////////////////////////////////////////////////////////////////
// everything recent
struct RecentBookmark_t
{
	Str_c m_sName;
	Str_c m_sPath;

	bool operator == ( const RecentBookmark_t & tBookmark ) const
	{
		return m_sName == tBookmark.m_sName && m_sPath == tBookmark.m_sPath;
	}
};


struct RecentOpenWith_t
{
	Str_c	m_sName;
	Str_c	m_sFilename;
	bool	m_bQuotes;
};


typedef Array_T < RecentOpenWith_t > OpenWithArray_t;


// recent stuff container
class Recent_c
{
public:
					Recent_c ();
					~Recent_c ();

	void			AddFilter ( const Str_c & sFilter );
	const Str_c & GetFilter ( int iFilter ) const;
	int				GetNumFilters () const;

	// recent copy-move destinations
	void			AddCopyMove ( const Str_c & sCopyMove );
	const Str_c & GetCopyMove ( int iCopyMove ) const;
	int				GetNumCopyMove () const;

	// recent rename destinations
	void			AddRename ( const Str_c & sRename );
	const Str_c & GetRename ( int iRename ) const;
	int				GetNumRename () const;

	// recent mkdir targets
	void			AddDir ( const Str_c & sDir );
	const Str_c &	GetDir ( int iDir ) const;
	int				GetNumDirs () const;

	// recent opendir targets
	void			AddOpenDir ( const Str_c & sDir );
	const Str_c &	GetOpenDir ( int iDir ) const;
	int				GetNumOpenDirs () const;

	// bookmarks
	void			AddBookmark ( const Str_c & sName, const Str_c & sPath );
	void 			MoveBookmark ( int iFrom, int iTo );
	void 			SetBookmarkName ( int iMark, const wchar_t * szName );
	void			SetBookmarkPath ( int iMark, const wchar_t * szPath );
	const RecentBookmark_t & GetBookmark ( int iMark ) const;
	int				GetNumBookmarks () const;
	void			DeleteBookmark ( int iMark );

	// find masks
	void			AddFindMask ( const Str_c & sFindMask );
	const Str_c & GetFindMask ( int iId ) const;
	int				GetNumFindMasks () const;

	// find texts
	void			AddFindText ( const Str_c & sFindText );
	const Str_c & GetFindText ( int iId ) const;
	int				GetNumFindTexts () const;

	// shortcut destinations
	void			AddShortcut ( const Str_c & sShortcut );
	const Str_c & GetShortcut ( int iId ) const;
	int				GetNumShortcuts () const;

	void			AddOpenWith ( const wchar_t * szExt, const wchar_t * szName, const wchar_t * szFilename, bool bQuotes );
	const OpenWithArray_t * GetOpenWithFor ( const wchar_t * szExt );
	
	void			SetRecentFilename ( const wchar_t * szFileName );
	void			SetBookmarkFilename ( const wchar_t * szFileName );
                                          
	void			Load ();
	void			Save ();

	static void		Init ();

private:
	bool			m_bChangeFlag;
	typedef Array_T	<Str_c> StringVec_t;

	enum
	{
		 MAX_FILTERS 		= 20
		,MAX_COPYMOVE_DESTS	= 20
		,MAX_RENAME_DESTS	= 20
		,MAX_DIRS			= 20
		,MAX_FIND			= 10
		,MAX_SHORTCUTS		= 20
		,MAX_ASSOC_PER_EXT	= 5
	};

	Str_c			m_sRecentFilename;
	Str_c			m_sBookmarkFilename;
	StringVec_t		m_dRecentFilters;
	StringVec_t		m_dRecentCopyMove;
	StringVec_t		m_dRecentRename;
	StringVec_t		m_dRecentDirs;
	StringVec_t		m_dOpenDirs;
	Array_T	<RecentBookmark_t> m_dRecentBookmarks;
	StringVec_t		m_dRecentFindMasks;
	StringVec_t		m_dRecentFindTexts;
	StringVec_t		m_dRecentShortcuts;
	Map_T <Str_c,OpenWithArray_t> m_OpenWithMap;

	template <typename T>
	void AddRecent ( const T & sStr, Array_T <T> & tVec, int iMaxAmount )
	{
		bool bFound = false;

		for ( int i = 0; i < tVec.Length (); ++i )
			if ( sStr == tVec [i] )
			{
				tVec.Delete ( i );
				tVec.Add ( sStr );
				bFound = true;
				break;
			}

		if ( ! bFound )
		{
			tVec.Add ( sStr );
			if ( tVec.Length () > iMaxAmount )
				tVec.Delete ( 0 );
		}

		m_bChangeFlag = true;
   	}

	void AddRecentStr  ( const Str_c & sStr, StringVec_t & tVec, int iMaxAmount )
	{
		if ( !sStr.Empty () )
			AddRecent <Str_c> ( sStr, tVec, iMaxAmount );
	}
};

extern Recent_c g_tRecent;


//////////////////////////////////////////////////////////////////////////////////////////
// locale

enum LocString_e
{
#include "loc/locale.inc"
	T_TOTAL
};

const wchar_t * Txt ( int iString );
void DlgTxt ( HWND hDlg, int iItemId, int iString );

bool Init_Lang ( const wchar_t * szPath, const wchar_t * szLangFile );
bool LoadLangFile ( Array_T <const wchar_t *> & dStrings, const wchar_t * szFilename );
const wchar_t * GetDefaultLangFile ();
const wchar_t * GetLangFile ();
void Shutdown_Lang ();

HMODULE ResourceInstance ();

bool Init_Resources ( const wchar_t * szFileName );
void Shutdown_Resources ();


#endif