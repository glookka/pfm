#include "pch.h"
#include "config.h"

#include "winuserm.h"


///////////////////////////////////////////////////////////////
// main config

Config_c * Config_c::m_pSingleton = NULL;

Config_c::Config_c ()
{
// register vars
#ifdef VAR
#undef VAR
#endif

#ifdef COMMENT
#undef COMMENT
#endif

#define VAR(type,var_name,value)\
	var_name = value; \
	RegisterVar ( L#var_name, &var_name );

#define COMMENT(value) AddComment (value);
#include "config.inc"
}


///////////////////////////////////////////////////////////////
// colors

Colors_c * Colors_c::m_pSingleton = NULL;

Colors_c::Colors_c ()
{
// register vars
#ifdef VAR
#undef VAR
#endif

#ifdef COMMENT
#undef COMMENT
#endif

#define VAR(type,var_name,value)\
	var_name = value; \
	RegisterVar ( L#var_name, &var_name );

#define COMMENT(value) AddComment (value);
#include "colors.inc"
}

///////////////////////////////////////////////////////////////
// buttons
Buttons_c *	Buttons_c::m_pSingleton = NULL;

CMD_BEGIN ( btn, 7 )
{
	BtnAction_e eAction = (BtnAction_e) tArgs.GetInt ( 0 );
	buttons->SetButton ( eAction, 0, tArgs.GetInt ( 1 ), tArgs.GetInt ( 2 ) != 0 );
	buttons->SetButton ( eAction, 1, tArgs.GetInt ( 3 ), tArgs.GetInt ( 4 ) != 0 );
	buttons->SetButton ( eAction, 2, tArgs.GetInt ( 5 ), tArgs.GetInt ( 6 ) != 0 );
}
CMD_END


Buttons_c::Buttons_c ()
	: m_bChangeFlag	( false )
{
	Assert ( T_BTN_NAME_LAST - T_BTN_NAME_1 + 1 == BA_TOTAL );

	for ( int i = 0; i < BA_TOTAL; ++i )
		for ( int j = 0; j < BTNS_PER_ACTION; ++j )
	{
		m_dCfgBtns [i][j].m_iKey = -1;
		m_dCfgBtns [i][j].m_bLong = false;
	}

	CMD_REG ( btn );
}

void Buttons_c::Load ( const Str_c & sFileName )
{
	Commander_c::Get ()->ExecuteFile ( sFileName );
	m_sFileName = sFileName;

	m_bChangeFlag = false;
}

void Buttons_c::Save ()
{
	if ( ! m_bChangeFlag )
		return;

	FILE * pFile = _wfopen ( m_sFileName, L"wb" );
	if ( ! pFile )
	{
		Log ( L"Can't save buttons to [%s]", m_sFileName.c_str () );
		return;
	}

	unsigned short uSign = 0xFEFF;
	fwrite ( &uSign, sizeof ( uSign ), 1, pFile );

	for ( int i = 0; i < BA_TOTAL; ++i )
		fwprintf ( pFile, L"btn %d %d %d %d %d %d %d;\n", i
			,m_dCfgBtns [i][0].m_iKey, m_dCfgBtns [i][0].m_bLong ? 1 : 0
			,m_dCfgBtns [i][1].m_iKey, m_dCfgBtns [i][1].m_bLong ? 1 : 0
			,m_dCfgBtns [i][2].m_iKey, m_dCfgBtns [i][2].m_bLong ? 1 : 0 );

	fclose ( pFile );
}


void Buttons_c::SetButton ( BtnAction_e eAction, int iBtnIndex, int iKey, bool bLong )
{
	if ( eAction < 0 || eAction >= BA_TOTAL || iBtnIndex < 0 || iBtnIndex >= BTNS_PER_ACTION )
		return;

	m_dCfgBtns [eAction][iBtnIndex].m_iKey = iKey;
	m_dCfgBtns [eAction][iBtnIndex].m_bLong = bLong;

	m_bChangeFlag = true;
}

void Buttons_c::SetLong ( BtnAction_e eAction, int iBtnIndex, bool bLong )
{
	if ( eAction < 0 || eAction >= BA_TOTAL || iBtnIndex < 0 || iBtnIndex >= BTNS_PER_ACTION )
		return;
	
	m_dCfgBtns [eAction][iBtnIndex].m_bLong = bLong;

	m_bChangeFlag = true;
}

const wchar_t *	Buttons_c::GetLitName ( BtnAction_e eAction ) const
{
	if ( eAction < 0 || eAction >= BA_TOTAL )
		return NULL;

	return Txt ( eAction + T_BTN_NAME_1 );
}

int	Buttons_c::GetKey ( BtnAction_e eAction, int iBtnIndex ) const
{
	if ( eAction < 0 || eAction >= BA_TOTAL || iBtnIndex < 0 || iBtnIndex >= BTNS_PER_ACTION )
		return -1;

	return m_dCfgBtns [eAction][iBtnIndex].m_iKey;
}

bool Buttons_c::GetLong ( BtnAction_e eAction, int iBtnIndex ) const
{
	if ( eAction < 0 || eAction >= BA_TOTAL || iBtnIndex < 0 || iBtnIndex >= BTNS_PER_ACTION )
		return false;

	return m_dCfgBtns [eAction][iBtnIndex].m_bLong;
}

#define VK_TSPEAKERPHONE_TOGGLE VK_F16
#define VK_VOICEDIAL			VK_F24
#define VK_KEYLOCK				VK_F22

Str_c Buttons_c::GetKeyLitName ( int iKey ) const
{
	if ( iKey == -1 )
		return Txt ( T_DLG_BTN_NOT_ASSIGNED );

	switch ( iKey )
	{
	case VK_TSOFT1:			return L"Soft key 1";
	case VK_TSOFT2:			return L"Soft key 2";
	case VK_TTALK:			return L"Talk";
	case VK_TEND:			return L"End";
	case VK_THOME:			return L"Home";
	case VK_TBACK:			return L"Back";
	case VK_TRECORD:		return L"Record";
	case VK_TFLIP:			return L"Flip";
	case VK_TPOWER:			return L"Power";
	case VK_TVOLUMEUP:		return L"Volume up";
	case VK_TVOLUMEDOWN:	return L"Volume down";
	case VK_TSPEAKERPHONE_TOGGLE: return L"Speakerphone toggle";
	case VK_TUP:			return L"Up";
	case VK_TDOWN:			return L"Down";
	case VK_TLEFT:			return L"Left";
	case VK_TRIGHT:			return L"Right";
	case VK_T0:				return L"0";
	case VK_T1:				return L"1";
	case VK_T2:				return L"2";
	case VK_T3:				return L"3";
	case VK_T4:				return L"4";
	case VK_T5:				return L"5";
	case VK_T6:				return L"6";
	case VK_T7:				return L"7";
	case VK_T8:				return L"8";
	case VK_T9:				return L"9";
	case VK_TSTAR:			return L"*";
	case VK_TPOUND:			return L"#";
	case VK_SYMBOL:			return L"SYM";
	case VK_ACTION:			return L"Action";
	case VK_VOICEDIAL:		return L"Voice dial";
	case VK_KEYLOCK:		return L"Key lock";
	case VK_APP1:			return L"Application 1";
	case VK_APP2:			return L"Application 2";
	case VK_APP3:			return L"Application 3";
	case VK_APP4:			return L"Application 4";
	case VK_APP5:			return L"Application 5";
	case VK_APP6:			return L"Application 6";
	}

	return NewString ( L"Button #%d", iKey );
}

BtnAction_e	Buttons_c::GetAction ( int iKey, btns::Event_e eEvent )
{
	if ( eEvent == btns::EVENT_NONE )
		return BA_NONE;

	bool bLongEvent = eEvent == btns::EVENT_LONGPRESS;

	for ( int i = 0; i < BA_TOTAL; ++i )
		for ( int j = 0; j < BTNS_PER_ACTION; ++j )
			if ( m_dCfgBtns [i][j].m_iKey == iKey && m_dCfgBtns [i][j].m_bLong == bLongEvent )
				return BtnAction_e ( i );

	return BA_NONE;
}

static bool ShouldRegisterKey ( int iKey )
{
	return iKey >= 0 && iKey != VK_TSOFT1 && iKey != VK_TSOFT2;
}

void Buttons_c::RegisterHotKeys ( HWND hWnd )
{
	for ( int i = 0; i < BA_TOTAL; ++i )
		for ( int j = 0; j < BTNS_PER_ACTION; ++j )
		{
			int iKey = m_dCfgBtns [i][j].m_iKey;
			if ( ShouldRegisterKey ( iKey ) )
			{
				UndergisterFunc ( MOD_WIN, iKey );
				RegisterHotKey ( hWnd, iKey, MOD_WIN, iKey );
			}
		}
}

void Buttons_c::UnregisterHotKeys ( HWND hWnd )
{
	for ( int i = 0; i < BA_TOTAL; ++i )
		for ( int j = 0; j < BTNS_PER_ACTION; ++j )
		{
			int iKey = m_dCfgBtns [i][j].m_iKey;
			if ( ShouldRegisterKey ( iKey ) )
				UnregisterHotKey ( hWnd, iKey );
		}
}

//////////////////////////////////////////////////////////////////////////////////////////
// everything recent

Recent_c g_tRecent;


CMD_BEGIN ( recent_filter, 1 )
{
	g_tRecent.AddFilter ( tArgs.GetString ( 0 ) );
}
CMD_END


CMD_BEGIN ( recent_copymove, 1 )
{
	g_tRecent.AddCopyMove ( tArgs.GetString ( 0 ) );
}
CMD_END


CMD_BEGIN ( recent_rename, 1 )
{
	g_tRecent.AddRename ( tArgs.GetString ( 0 ) );
}
CMD_END


CMD_BEGIN ( recent_dir, 1 )
{
	g_tRecent.AddDir ( tArgs.GetString ( 0 ) );
}
CMD_END

CMD_BEGIN ( recent_opendir, 1 )
{
	g_tRecent.AddOpenDir ( tArgs.GetString ( 0 ) );
}
CMD_END

CMD_BEGIN ( recent_bookmark, 2 )
{
	g_tRecent.AddBookmark ( tArgs.GetString ( 0 ), tArgs.GetString ( 1 ) );
}
CMD_END


CMD_BEGIN ( recent_find_mask, 1 )
{
	g_tRecent.AddFindMask ( tArgs.GetString ( 0 ) );
}
CMD_END


CMD_BEGIN ( recent_find_text, 1 )
{
	g_tRecent.AddFindText ( tArgs.GetString ( 0 ) );
}
CMD_END


CMD_BEGIN ( recent_shortcut, 1 )
{
	g_tRecent.AddShortcut ( tArgs.GetString ( 0 ) );
}
CMD_END


CMD_BEGIN ( recent_openwith, 4 )
{
	g_tRecent.AddOpenWith ( tArgs.GetString ( 0 ), tArgs.GetString ( 1 ), tArgs.GetString ( 2 ), tArgs.GetBool ( 3 ) );
}
CMD_END

Recent_c::Recent_c ()
	: m_bChangeFlag ( false )
{
}

Recent_c::~Recent_c ()
{
}

////
void Recent_c::AddFilter ( const Str_c & sFilter )
{
	AddRecentStr ( sFilter, m_dRecentFilters, MAX_FILTERS );
}


const Str_c & Recent_c::GetFilter ( int iFilter ) const
{
	return m_dRecentFilters [iFilter];
}


int	Recent_c::GetNumFilters () const
{
	return m_dRecentFilters.Length ();
}

////
void Recent_c::AddCopyMove ( const Str_c & sCopyMove )
{
	AddRecentStr ( sCopyMove, m_dRecentCopyMove, MAX_COPYMOVE_DESTS );
}


const Str_c & Recent_c::GetCopyMove ( int iCopyMove ) const
{
	return m_dRecentCopyMove [iCopyMove];
}


int Recent_c::GetNumCopyMove () const
{
	return m_dRecentCopyMove.Length ();
}

////

void Recent_c::AddRename ( const Str_c & sRename )
{
	AddRecentStr ( sRename, m_dRecentRename, MAX_RENAME_DESTS );
}


const Str_c & Recent_c::GetRename ( int iRename ) const
{
	return m_dRecentRename [iRename];
}

int Recent_c::GetNumRename () const
{
	return m_dRecentRename.Length ();
}

////

void Recent_c::AddDir ( const Str_c & sDir )
{
	AddRecentStr ( sDir, m_dRecentDirs, MAX_DIRS );
}

const Str_c & Recent_c::GetDir ( int iDir ) const
{
	return m_dRecentDirs [iDir];
}

int	Recent_c::GetNumDirs () const
{
	return m_dRecentDirs.Length ();
}

////

void Recent_c::AddOpenDir ( const Str_c & sDir )
{
	AddRecentStr ( sDir, m_dOpenDirs, MAX_DIRS );
}

const Str_c & Recent_c::GetOpenDir ( int iDir ) const
{
	return m_dOpenDirs [iDir];
}

int	Recent_c::GetNumOpenDirs () const
{
	return m_dOpenDirs.Length ();
}

////
void Recent_c::AddBookmark ( const Str_c & sName, const Str_c & sPath )
{
	RecentBookmark_t tMark;
	tMark.m_sName = sName;
	tMark.m_sPath = sPath;

	m_dRecentBookmarks.Add ( tMark );

	m_bChangeFlag = true;
}

void Recent_c::MoveBookmark ( int iFrom, int iTo )
{
	RecentBookmark_t tMark = m_dRecentBookmarks [iFrom];
	m_dRecentBookmarks.Delete ( iFrom );
	m_dRecentBookmarks.Insert ( iTo, tMark );

	m_bChangeFlag = true;
}

void Recent_c::SetBookmarkName ( int iMark, const wchar_t * szName )
{
	m_dRecentBookmarks [iMark].m_sName = szName;
	m_bChangeFlag = true;
}

void Recent_c::SetBookmarkPath ( int iMark, const wchar_t * szPath )
{
	m_dRecentBookmarks [iMark].m_sPath = szPath;
	m_bChangeFlag = true;
}
 
const RecentBookmark_t & Recent_c::GetBookmark ( int iMark ) const
{
	return m_dRecentBookmarks [iMark];
}


int	Recent_c::GetNumBookmarks () const
{
	return m_dRecentBookmarks.Length ();
}


void Recent_c::DeleteBookmark ( int iMark )
{
	m_dRecentBookmarks.Delete ( iMark );
	m_bChangeFlag = true;
}

////
void Recent_c::AddFindMask ( const Str_c & sFindMask )
{
	AddRecentStr ( sFindMask, m_dRecentFindMasks, MAX_FIND );
}


const Str_c & Recent_c::GetFindMask ( int iId ) const
{
	return m_dRecentFindMasks [iId];
}


int Recent_c::GetNumFindMasks () const
{
	return m_dRecentFindMasks.Length ();
}

////
void Recent_c::AddFindText ( const Str_c & sFindText )
{
	AddRecentStr ( sFindText, m_dRecentFindTexts, MAX_FIND );
}


const Str_c & Recent_c::GetFindText ( int iId ) const
{
	return m_dRecentFindTexts [iId];
}


int Recent_c::GetNumFindTexts () const
{
	return m_dRecentFindTexts.Length ();
}

////
void Recent_c::AddShortcut ( const Str_c & sShortcut )
{
	AddRecentStr ( sShortcut, m_dRecentShortcuts, MAX_SHORTCUTS );
}


const Str_c & Recent_c::GetShortcut ( int iId ) const
{
	return m_dRecentShortcuts [iId];
}

int	Recent_c::GetNumShortcuts () const
{
	return m_dRecentShortcuts.Length ();
}

////
void Recent_c::AddOpenWith ( const wchar_t * szExt, const wchar_t * szName, const wchar_t * szFilename, bool bQuotes )
{
	RecentOpenWith_t OpenWith;
	OpenWith.m_sName 	= szName;
	OpenWith.m_sFilename= szFilename;
	OpenWith.m_bQuotes 	= bQuotes;

	Str_c sExt ( szExt );
	sExt.ToLower ();

	OpenWithArray_t * pArray = m_OpenWithMap.Find ( sExt );
	if ( ! pArray )
	{
		OpenWithArray_t Array;
		Array.Add ( OpenWith );
		m_OpenWithMap.Add ( sExt, Array );
	}
	else
	{
		bool bFound = false;

		for ( int i = 0; ! bFound && i < pArray->Length (); ++i )
			if ( (*pArray) [i].m_sFilename == szFilename )
			{
				pArray->Delete ( i );
				pArray->Add ( OpenWith );
				bFound = true;
			}

		if ( ! bFound )
		{
			if ( pArray->Length () >= MAX_ASSOC_PER_EXT )
				pArray->Delete ( 0 );

			pArray->Add ( OpenWith );
		}
	}

	m_bChangeFlag = true;
}

const OpenWithArray_t * Recent_c::GetOpenWithFor ( const wchar_t * szExt )
{
	if ( !*szExt )
		return NULL;

	Str_c sExt ( szExt );
	sExt.ToLower ();

	return m_OpenWithMap.Find ( sExt );
}

///////////////////////////////////////////////////////////////////////////////////////////

void Recent_c::SetRecentFilename ( const wchar_t * szFileName )
{
	m_sRecentFilename = szFileName;
}

void Recent_c::SetBookmarkFilename ( const wchar_t * szFileName )
{
	m_sBookmarkFilename = szFileName;
}

void Recent_c::Load ()
{
	Commander_c::Get ()->ExecuteFile ( m_sRecentFilename );
	Commander_c::Get ()->ExecuteFile ( m_sBookmarkFilename );

	m_bChangeFlag = false;
}

void Recent_c::Save ()
{
	if ( ! m_bChangeFlag )
		return;

	unsigned short uSign = 0xFEFF;

	if ( cfg->save_dlg_history )
	{
		FILE * pFile = _wfopen ( m_sRecentFilename ,L"wb" );
		if ( ! pFile )
		{
			Log ( L"Can't save recent list to [%s]", m_sRecentFilename.c_str () );
			return;
		}

		fwrite ( &uSign, sizeof ( uSign ), 1, pFile );

		if ( ! m_dRecentFilters.Empty () )
		{
			for ( int i = 0; i < m_dRecentFilters.Length () ; ++i )
				fwprintf ( pFile, L"recent_filter \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dRecentFilters [i] ).c_str () );

			fwprintf ( pFile, L"\r\n" );
		}

		if ( ! m_dRecentCopyMove.Empty () )
		{
			for ( int i = 0; i < m_dRecentCopyMove.Length () ; ++i )
				fwprintf ( pFile, L"recent_copymove \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dRecentCopyMove [i] ).c_str () );

			fwprintf ( pFile, L"\r\n" );
		}

		if ( ! m_dRecentRename.Empty () )
		{
			for ( int i = 0; i < m_dRecentRename.Length () ; ++i )
				fwprintf ( pFile, L"recent_rename \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dRecentRename [i] ).c_str () );

			fwprintf ( pFile, L"\r\n" );
		}

		if ( ! m_dRecentDirs.Empty () )
		{
			for ( int i = 0; i < m_dRecentDirs.Length () ; ++i )
				fwprintf ( pFile, L"recent_dir \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dRecentDirs [i] ).c_str () );

			fwprintf ( pFile, L"\r\n" );
		}

		if ( ! m_dOpenDirs.Empty () )
		{
			for ( int i = 0; i < m_dOpenDirs.Length () ; ++i )
				fwprintf ( pFile, L"recent_opendir \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dOpenDirs [i] ).c_str () );

			fwprintf ( pFile, L"\r\n" );
		}

		if ( ! m_dRecentFindMasks.Empty () )
		{
			for ( int i = 0; i < m_dRecentFindMasks.Length () ; ++i )
				fwprintf ( pFile, L"recent_find_mask \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dRecentFindMasks [i] ).c_str () );

			fwprintf ( pFile, L"\r\n" );
		}

		if ( ! m_dRecentFindTexts.Empty () )
		{
			for ( int i = 0; i < m_dRecentFindTexts.Length () ; ++i )
				fwprintf ( pFile, L"recent_find_text \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dRecentFindTexts [i] ).c_str () );

			fwprintf ( pFile, L"\r\n" );
		}

		if ( ! m_dRecentShortcuts.Empty () )
		{
			for ( int i = 0; i < m_dRecentShortcuts.Length () ; ++i )
				fwprintf ( pFile, L"recent_shortcut \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dRecentShortcuts [i] ).c_str () );

			fwprintf ( pFile, L"\r\n" );
		}

		if ( m_OpenWithMap.Length () > 0 )
		{
			for ( int i = 0; i < m_OpenWithMap.Length () ; ++i )
				for ( int j = 0; j < m_OpenWithMap [i].Length (); ++j )
				fwprintf ( pFile, L"recent_openwith \"%s\" \"%s\" \"%s\" %d;\r\n", Parser_c::EncodeSpecialChars ( m_OpenWithMap.Key ( i ) ).c_str (),
					 Parser_c::EncodeSpecialChars ( m_OpenWithMap [i][j].m_sName ).c_str (), Parser_c::EncodeSpecialChars ( m_OpenWithMap [i][j].m_sFilename ).c_str (), m_OpenWithMap [i][j].m_bQuotes ? 1:0 );

			fwprintf ( pFile, L"\r\n" );
		}

		fclose ( pFile );
	}

	if ( ! m_dRecentBookmarks.Empty () )
	{
		FILE * pFile = _wfopen ( m_sBookmarkFilename, L"wb" );
		if ( ! pFile )
		{
			Log ( L"Can't save bookmarks to [%s]", m_sBookmarkFilename.c_str () );
			return;
		}

		fwrite ( &uSign, sizeof ( uSign ), 1, pFile );

		for ( int i = 0; i < m_dRecentBookmarks.Length () ; ++i )
			fwprintf ( pFile, L"recent_bookmark \"%s\" \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dRecentBookmarks [i].m_sName ).c_str (), Parser_c::EncodeSpecialChars ( m_dRecentBookmarks [i].m_sPath ).c_str () );

		fwprintf ( pFile, L"\r\n" );

		fclose ( pFile );
	}
}


void Recent_c::Init ()
{
	CMD_REG ( recent_filter );
	CMD_REG ( recent_copymove );
	CMD_REG ( recent_rename );
	CMD_REG ( recent_dir );
	CMD_REG ( recent_opendir );
	CMD_REG ( recent_bookmark );
	CMD_REG ( recent_find_mask );
	CMD_REG ( recent_find_text );
	CMD_REG ( recent_shortcut );
	CMD_REG ( recent_openwith );
}


//////////////////////////////////////////////////////////////////////////////////////////
// locale

static Str_c g_sLangFile;
static const wchar_t * g_szDefaultLangFile = L"english.lang";
static const wchar_t * g_szEmptyString = L"";
static Array_T < const wchar_t * > g_dStrings;
static HMODULE g_hResourceModule = NULL;

const wchar_t * Txt ( int iString )
{
	if ( iString < 0 || iString >= g_dStrings.Length () )
		return g_szEmptyString;

	return g_dStrings [iString];
}

void DlgTxt ( HWND hDlg, int iItemId, int iString )
{
	SetDlgItemText ( hDlg, iItemId, Txt ( iString ) );
}

void CopyConvert ( const wchar_t * szSource, wchar_t * szDest, int nChars )
{
	int iSource = 0;
	int iDest  = 0;
	while ( iSource < nChars )
	{
		if ( szSource [iSource] == L'\\' && szSource [iSource+1] == L'n' )
		{
			szDest [iDest] = L'\n';
			iSource += 2;
		}
		else
		{
			szDest [iDest] = szSource [iSource];
			++iSource;
		}

		++iDest;
	}

	szDest [iDest] = L'\0';
}

const wchar_t * GetDefaultLangFile ()
{
	return g_szDefaultLangFile;
}

const wchar_t * GetLangFile ()
{
	return g_sLangFile;
}

bool LoadLangFile ( Array_T <const wchar_t *> & dStrings, const wchar_t * szFilename )
{
	const int MAX_BUFFER_SIZE = 102400;

	HANDLE hFile = CreateFile ( szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
	if ( hFile == INVALID_HANDLE_VALUE )
		return false;

	ULARGE_INTEGER uSize;
	uSize.LowPart = GetFileSize ( hFile, &uSize.HighPart );

	if ( uSize.LowPart == 0xFFFFFFFF )
		return false;

	DWORD uSizeDw = (int) uSize.QuadPart;

	if ( uSizeDw > MAX_BUFFER_SIZE )
		return false;

	if ( ( uSizeDw % 2 ) != 0  )
		return false;

	int iBufferSize = uSizeDw / 2;
	wchar_t * pBuffer = new wchar_t [iBufferSize + 1];

	DWORD uSizeRead = 0;
	ReadFile ( hFile, pBuffer, uSizeDw, &uSizeRead, NULL );
	CloseHandle ( hFile );

	if ( uSizeRead != uSizeDw )
	{
		SafeDeleteArray ( pBuffer );
		return false;
	}

	pBuffer [iBufferSize] = L'\0';

	wchar_t * pStart = pBuffer;
	while ( *pStart )
	{
		while ( *pStart && _X_iswspace ( *pStart ) )
			++pStart;

		wchar_t * pLineStart = pStart;
		while ( *pStart && *pStart != L'\n' )
			++pStart;

		int nChars = pStart - pLineStart - ( *pStart ? 1 : 0 );
		wchar_t * szAllocated = new wchar_t [nChars + 1];
		CopyConvert ( pLineStart, szAllocated, nChars );
		dStrings.Add ( szAllocated );
	}

	SafeDeleteArray ( pBuffer );

	return true;
}


bool Init_Lang ( const wchar_t * szPath, const wchar_t * szLangFile )
{
	if ( !LoadLangFile ( g_dStrings, Str_c ( szPath ) + szLangFile ) )
		return false;

	if ( g_dStrings.Length () != T_TOTAL )
		return false;

	g_sLangFile = szLangFile;

	return true;
}

void Shutdown_Lang ()
{
	for ( int i = 0; i < g_dStrings.Length (); ++i )
		SafeDeleteArray ( g_dStrings [i] );

	g_dStrings.Clear ();
}

bool Init_Resources ( const wchar_t * szFileName )
{
	g_hResourceModule = LoadLibraryEx ( szFileName, NULL, LOAD_LIBRARY_AS_DATAFILE );
	return !!g_hResourceModule;
}

void Shutdown_Resources ()
{
	if ( g_hResourceModule )
		::FreeLibrary ( g_hResourceModule );
}

HMODULE ResourceInstance ()
{
	return g_hResourceModule;
}