#ifndef _srecent_
#define _srecent_

#include "LCore/cmain.h"
#include "LCore/cstr.h"
#include "LCore/carray.h"
#include "LCore/cmap.h"

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

#endif