#include "pch.h"

#include "LSettings/srecent.h"
#include "LParser/pcommander.h"
#include "LSettings/sconfig.h"

Recent_c g_tRecent;

///////////////////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////
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

	if ( g_tConfig.save_dlg_history )
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
			fwprintf ( pFile, L"\r\n# Filters:\r\n" );
			for ( int i = 0; i < m_dRecentFilters.Length () ; ++i )
				fwprintf ( pFile, L"recent_filter \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dRecentFilters [i] ).c_str () );
		}

		if ( ! m_dRecentCopyMove.Empty () )
		{
			fwprintf ( pFile, L"\r\n# Copy/Move:\r\n" );
			for ( int i = 0; i < m_dRecentCopyMove.Length () ; ++i )
				fwprintf ( pFile, L"recent_copymove \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dRecentCopyMove [i] ).c_str () );
		}

		if ( ! m_dRecentRename.Empty () )
		{
			fwprintf ( pFile, L"\r\n# Rename:\r\n" );
			for ( int i = 0; i < m_dRecentRename.Length () ; ++i )
				fwprintf ( pFile, L"recent_rename \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dRecentRename [i] ).c_str () );
		}

		if ( ! m_dRecentDirs.Empty () )
		{
			fwprintf ( pFile, L"\r\n# Create Folder:\r\n" );
			for ( int i = 0; i < m_dRecentDirs.Length () ; ++i )
				fwprintf ( pFile, L"recent_dir \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dRecentDirs [i] ).c_str () );
		}

		if ( ! m_dOpenDirs.Empty () )
		{
			fwprintf ( pFile, L"\r\n# Open Folder:\r\n" );
			for ( int i = 0; i < m_dOpenDirs.Length () ; ++i )
				fwprintf ( pFile, L"recent_opendir \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dOpenDirs [i] ).c_str () );
		}

		if ( ! m_dRecentFindMasks.Empty () )
		{
			fwprintf ( pFile, L"\r\n# Find masks:\r\n" );
			for ( int i = 0; i < m_dRecentFindMasks.Length () ; ++i )
				fwprintf ( pFile, L"recent_find_mask \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dRecentFindMasks [i] ).c_str () );
		}

		if ( ! m_dRecentFindTexts.Empty () )
		{
			fwprintf ( pFile, L"\r\n# Find texts:\r\n" );
			for ( int i = 0; i < m_dRecentFindTexts.Length () ; ++i )
				fwprintf ( pFile, L"recent_find_text \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dRecentFindTexts [i] ).c_str () );
		}

		if ( ! m_dRecentShortcuts.Empty () )
		{
			fwprintf ( pFile, L"\r\n# Shortcut targets:\r\n" );
			for ( int i = 0; i < m_dRecentShortcuts.Length () ; ++i )
				fwprintf ( pFile, L"recent_shortcut \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dRecentShortcuts [i] ).c_str () );
		}

		if ( m_OpenWithMap.Length () > 0 )
		{
			fwprintf ( pFile, L"\r\n# Open with:\r\n" );
			for ( int i = 0; i < m_OpenWithMap.Length () ; ++i )
				for ( int j = 0; j < m_OpenWithMap [i].Length (); ++j )
				fwprintf ( pFile, L"recent_openwith \"%s\" \"%s\" \"%s\" %d;\r\n", Parser_c::EncodeSpecialChars ( m_OpenWithMap.Key ( i ) ).c_str (),
					 Parser_c::EncodeSpecialChars ( m_OpenWithMap [i][j].m_sName ).c_str (), Parser_c::EncodeSpecialChars ( m_OpenWithMap [i][j].m_sFilename ).c_str (), m_OpenWithMap [i][j].m_bQuotes ? 1:0 );
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

		fwprintf ( pFile, L"\r\n# Bookmarks:\r\n" );
		for ( int i = 0; i < m_dRecentBookmarks.Length () ; ++i )
			fwprintf ( pFile, L"recent_bookmark \"%s\" \"%s\";\r\n", Parser_c::EncodeSpecialChars ( m_dRecentBookmarks [i].m_sName ).c_str (), Parser_c::EncodeSpecialChars ( m_dRecentBookmarks [i].m_sPath ).c_str () );

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