#ifndef _iterator_
#define _iterator_

#include "main.h"
#include "filelist.h"

struct FileInfo_t
{
	WIN32_FIND_DATA			m_tData;
	DWORD					m_uFlags;
	const ColorGroup_t *	m_pCG;
	int						m_iIconIndex;
};

struct CachedResult_t
{
	bool			m_b2ndPassDir;
	bool			m_bRootDir;
	Str_c			m_sPartialDir;
	WIN32_FIND_DATA	m_tData;
};

//
// common file iterator 
//
class FileIterator_c
{
public:
	virtual void			IterateStart ( const Str_c & sRootDir, bool bRecursive = true ) {}
	virtual void			IterateStart ( const SelectedFileList_t & tList, bool bRecursive = true ) {}
	virtual bool			IterateNext () = 0;

	virtual bool			IsRootDir () const = 0;
	virtual bool			Is2ndPassDir () const = 0;
	virtual Str_c			GetFullName () const = 0;		// returns full path + filename
	virtual Str_c			GetFullPath () const = 0;		// returns full path + slash
	virtual const WIN32_FIND_DATA *	GetData () const = 0;

	virtual const CachedResult_t * GetCachedResult ( int iId ) const { return NULL; }
};


//
// iterates all files starting from a given folder
//
class FileIteratorTree_c : public FileIterator_c
{
public:
							FileIteratorTree_c ();
							~FileIteratorTree_c ();
				
	virtual void			IterateStart ( const Str_c & sRootDir, bool bRecursive = true );
	virtual bool			IterateNext ();

	virtual bool			IsRootDir () const;
	virtual bool			Is2ndPassDir () const;
	virtual Str_c			GetFullName () const;
	virtual Str_c			GetFullPath () const;
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
	friend class FileIteratorPanelCached_c;
public:
							FileIteratorPanel_c ();

	virtual void			IterateStart ( const SelectedFileList_t & tList, bool bRecursive = true );
	virtual bool			IterateNext ();

	void					SkipRoot ( bool bSkip );
	Str_c					GetFileNameSkipped () const;		// filename with optionally skipped root directory

	virtual bool			IsRootDir () const;
	virtual bool			Is2ndPassDir () const;
	virtual Str_c			GetFullName () const;
	virtual Str_c			GetFullPath () const;
	virtual const WIN32_FIND_DATA * GetData () const;

private:
	const SelectedFileList_t *	m_pList;
	bool						m_bSkipRoot;
	bool						m_bLastDirectory;		// last root file was a dir
	bool						m_bSteppedUp;
	Str_c						m_sSteppedUpDir;		// the directory we've stepped up from
	Str_c						m_sPassRootDir;
	bool						m_bInTree;				// we're in a tree pass
	bool						m_bRecursive;			// include subdirs
	int							m_iCurrentFile;			// current root file

	Str_c						m_sFileName;
	WIN32_FIND_DATA				m_tData;
	FileIteratorTree_c			m_tTreeIterator;


	Str_c					GetPartialPath () const;		// partial path w/o slash
};


// FIXME!!! eats WAY too much memory;
class FileIteratorPanelCached_c : public FileIterator_c
{
public:
							FileIteratorPanelCached_c ();

	virtual void			IterateStart ( const SelectedFileList_t & tList, bool bRecursive = true );
	virtual bool			IterateNext ();

	virtual bool			IsRootDir () const;
	virtual bool			Is2ndPassDir () const;
	virtual Str_c			GetFullName () const;
	virtual Str_c			GetFullPath () const;
	virtual const WIN32_FIND_DATA *	GetData () const;

private:
	int 						m_iIndex;
	Str_c						m_sRootDir;
	CachedResult_t *			m_pCurrentResult;
	Array_T < CachedResult_t >	m_dFound;
};

#endif