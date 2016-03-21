#ifndef _fsorter_
#define _fsorter_

#include "LCore/cmain.h"
#include "LFile/ffilter.h"

enum SortMode_e
{
	 SORT_UNSORTED
	,SORT_NAME
	,SORT_EXTENSION
	,SORT_SIZE
	,SORT_TIME_CREATE
	,SORT_TIME_ACCESS
	,SORT_TIME_WRITE
	,SORT_GROUP
};


enum FileFlags_e
{
	 FLAG_NOTHING	= 0
	,FLAG_PREV_DIR	= 1
	,FLAG_MARKED	= 1 << 1
	,FLAG_DEVICE	= 1 << 2
};

// forward
struct ColorGroup_t;

struct FileInfo_t
{
	WIN32_FIND_DATA			m_tData;
	DWORD					m_uFlags;
	const ColorGroup_t *	m_pCG;
	int						m_iIconIndex;
};


// TODO: sorting by indices? should be faster
// a class for file sorting/storing/filtering
class FileSorter_c
{
public:
					FileSorter_c ();

	int				GetNumFiles () const;					// returns the number of files in the current directory
	const 			FileInfo_t & GetFile ( int nFile ) const;	// file info getter
	int				GetFileIconIndex ( int iFile );

	const Str_c & GetDirectory () const;				// GetDirectory

	void			SetVisibility ( bool bShowHidden, bool bShowSystem, bool bShowROM );
	void			SetSortMode ( SortMode_e eMode, bool bReverse );
	void			SetDirectory ( const wchar_t * szDir );	// sets current directory

	bool			MarkFile ( int nFile, bool bSelect );	// mark file as selected
	bool			MarkFile ( const wchar_t * szName, bool bSelect );	// mark by name
	void			MarkFilter ( const wchar_t * szFilter, bool bSelect ); // mark files that match the filter
	int 			GetNumMarked () const;					// number of marked files
	ULARGE_INTEGER	GetMarkedSize () const;					// size of marked files

	bool			StepToPrevDir ( Str_c * pPrevDirName = NULL ); 	// move to prev dir
	void			StepToNextDir ( const wchar_t * szNextDir ); // move to next dir
	void			Refresh ();								// refresh directory contents
	void			SoftRefresh ();							// refresh directory contents, but don't touch marked flags

	SortMode_e		GetSortMode () const;					// returns sort mode
	bool			GetSortReverse () const;

protected:
	Str_c			m_sDir;
	SortMode_e		m_eSortMode;
	bool			m_bSortReverse;
	bool			m_bShowHidden;
	bool			m_bShowSystem;
	bool			m_bShowROM;


	Filter_c		m_tFilter;								// file filter. used to mark files

	virtual void	Event_Refresh ();						// file refresh occured
	virtual void	Event_Directory () {}					// directory change
	virtual void	Event_MarkFile ( int nFile, bool bMarked ) {}	// this file was just marked or cleared
	virtual void	Event_SlowRefresh ();					// refresh is gonna take loong
	virtual void	Event_BeforeSoftRefresh () {}
	virtual void	Event_AfterSoftRefresh () {}

	Str_c			StepUpToValid ( const Str_c & sCurrentDir );

private:
	int				m_nMarked;
	ULARGE_INTEGER	m_tMarkedSize;
	Array_T <FileInfo_t> m_dFiles;						// sorted array of files
	bool			m_bSlowRefresh;							// slow refresh occured
	HCURSOR			m_hOldCursor;							// previous cursor
};

// global small icon list
extern HIMAGELIST g_hSmallIconList;

#endif