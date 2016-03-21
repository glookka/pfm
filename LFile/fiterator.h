#ifndef _fiterator_
#define _fiterator_

#include "LCore/cmain.h"
#include "LFile/fsorter.h"

// for various file operatons
struct FileList_t
{
	Array_T <const FileInfo_t *>m_dFiles;
	ULARGE_INTEGER					m_uSize;
	Str_c					m_sRootDir;

	bool OneFile () const
	{
		return m_dFiles.Length () == 1;
	}

	bool IsDir () const
	{
		if ( m_dFiles.Empty () )
			return false;

		return !!( m_dFiles [0]->m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );
	}

	bool IsPrevDir () const
	{
		if ( m_dFiles.Empty () )
			return false;

		return !!( m_dFiles [0]->m_uFlags & FLAG_PREV_DIR );
	}

	void Reset ()
	{
		m_dFiles.Reset ();
		m_uSize.QuadPart = 0;
		m_sRootDir = L"";
	}
};

//
// common file iterator 
//
class FileIterator_c
{
public:
	virtual void			IterateStart ( const Str_c & sRootDir, bool bRecursive = true ) {}
	virtual void			IterateStart ( FileList_t & tList, bool bRecursive = true ) {}
	virtual bool			IterateNext () = 0;

	virtual bool			IsRootDir () const = 0;
	virtual bool			Is2ndPassDir () const = 0;
	virtual Str_c	GetFileName () const = 0;
	virtual Str_c	GetDirectory () const = 0;
	virtual const WIN32_FIND_DATA *	GetData () const = 0;
};


//
// iterates all files starting from a given folder
//
class FileIteratorTree_c : public FileIterator_c
{
public:
						FileIteratorTree_c ();
						~FileIteratorTree_c ();
				
	virtual void		IterateStart ( const Str_c & sRootDir, bool bRecursive = true );
	virtual bool		IterateNext ();

	virtual bool			IsRootDir () const;
	virtual bool		Is2ndPassDir () const;
	virtual Str_c GetFileName () const;
	virtual Str_c GetDirectory () const;
	virtual const WIN32_FIND_DATA * GetData () const;

private:
	struct SearchLevel_t
	{
		WIN32_FIND_DATA		m_tData;
		Str_c		m_sDirName;
		HANDLE				m_hHandle;

		SearchLevel_t ( const Str_c & sRootDir )
			: m_sDirName ( sRootDir )
			, m_hHandle ( NULL )
		{
		}

		SearchLevel_t ()
			: m_hHandle ( NULL )
		{
		}
	};


	bool				m_bSteppedUp;
	bool				m_bRecursive;
	Str_c				m_sRootDir;
	Array_T <SearchLevel_t> m_dSearchLevels;
};


class Panel_c;

//
// Iterates all files based on panel marked state
//
class FileIteratorPanel_c : public FileIterator_c
{
public:
						FileIteratorPanel_c ();

	virtual void		IterateStart ( FileList_t & tList, bool bRecursive = true );
	virtual bool		IterateNext ();

	void				SkipRoot ( bool bSkip );
	Str_c				GetFileNameSkipped () const;		// filename with optionally skipped root directory

	virtual bool		IsRootDir () const;
	virtual bool		Is2ndPassDir () const;
	virtual Str_c		GetFileName () const;
	virtual Str_c		GetDirectory () const;
	virtual const WIN32_FIND_DATA * GetData () const;

	const Str_c & GetSourceDir () const;

private:
	FileList_t *		m_pList;
	bool				m_bSkipRoot;
	bool				m_bLastDirectory;		// last root file was a dir
	bool				m_bSteppedUp;
	Str_c		m_sSteppedUpDir;		// the directory we've stepped up from
	Str_c		m_sPassRootDir;
	bool				m_bInTree;				// we're in a tree pass
	bool				m_bRecursive;			// include subdirs
	int					m_iCurrentFile;			// current root file

	Str_c		m_sFileName;
	WIN32_FIND_DATA		m_tData;
	FileIteratorTree_c	m_tTreeIterator;
};


// FIXME!!! eats WAY too much memory;
class FileIteratorPanelCached_c : public FileIterator_c
{
public:
	struct CachedResult_t
	{
		bool			m_b2ndPassDir;
		bool			m_bRootDir;
		Str_c    m_sDir;
		WIN32_FIND_DATA	m_tData;
	};

							FileIteratorPanelCached_c ();

	virtual void			IterateStart ( FileList_t & tList, bool bRecursive = true );
	virtual bool			IterateNext ();

	virtual bool			IsRootDir () const;
	virtual bool			Is2ndPassDir () const;
	virtual Str_c	GetFileName () const;
	virtual Str_c	GetDirectory () const;
	virtual const WIN32_FIND_DATA *	GetData () const;

	const CachedResult_t &	GetCachedResult ( int iId ) const;
	int						GetIndex () const;

private:
	int 					m_iIndex;
	FileIteratorPanel_c *	m_pIterator;
	CachedResult_t *		m_pCurrentResult;
	Array_T < CachedResult_t > m_dFound;
};

#endif