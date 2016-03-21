#ifndef _filelist_
#define _filelist_

#include "main.h"
#include "filter.h"
#include "pluginapi/pluginapi.h"

//////////////////////////////////////////////////////////////////////////
// color group stuff
struct ColorGroup_t
{
	ColorGroup_t ();

	DWORD			m_dwIncludeAttributes;
	DWORD			m_dwExcludeAttributes;
	ExtFilter_c		m_tFilter;
	DWORD			m_uColor;
	DWORD			m_uSelectedColor;
	int				m_id;
};

void	RegisterColorGroupCommands ();
void	LoadColorGroups ( const wchar_t * szFileName );
int		CompareColorGroups ( const ColorGroup_t * pCG1, const ColorGroup_t * pCG2 );

//////////////////////////////////////////////////////////////////////////
// storage card stuff
void 	RefreshStorageCards ();
int  	GetNumStorageCards ();
void	GetStorageCardData ( int iCard, WIN32_FIND_DATA & tData );

//////////////////////////////////////////////////////////////////////////
// file sorter
const DWORD FILE_ATTRIBUTE_CARD = 0x00040000;

enum
{
	 FILE_ICON_CHECKED	= 1 << 0
	,FILE_PREV_DIR		= 1 << 1
	,FILE_MARKED		= 1 << 2
};


struct SelectedFileList_t
{
	Array_T <const PanelItem_t *>	m_dFiles;
	ULARGE_INTEGER					m_uSize;
	Str_c							m_sRootDir;			// guaranteed to have a slash on the end

	bool	OneFile () const;
	bool	IsDir () const;
	bool	IsPrevDir () const;
};


struct ItemProperties_t
{
	WORD					m_uFlags;
	const ColorGroup_t *	m_pCG;
};


class FileList_c
{
public:
					FileList_c ();
					~FileList_c ();

	void			SetDirectory ( const wchar_t * szDir );		// sets current directory. activates plugins if needed
	const Str_c &	GetDirectory () const;

	const PanelItem_t &	GetItem ( int iItem ) const;
	DWORD			GetItemColor ( int iItem, bool bAtCursor );
	int				GetItemIcon ( int iItem );
	DWORD			GetItemFlags ( int iItem );
	int				GetNumItems () const;

	void			SetVisibility ( bool bShowHidden, bool bShowSystem, bool bShowROM );
	void			SetSortMode ( SortMode_e eMode, bool bReverse );

	virtual bool	MarkFileById ( int iItem, bool bSelect );	// mark file as selected
	virtual bool	MarkFileByName ( const wchar_t * szName, bool bSelect );	// mark by name
	int 			GetNumMarked () const;					// number of marked files
	ULARGE_INTEGER	GetMarkedSize () const;					// size of marked files

	bool			StepToPrevDir ( Str_c * pPrevDirName = NULL ); 	// move to prev dir
	void			StepToNextDir ( const wchar_t * szNextDir ); // move to next dir

	void			Refresh ();								// refresh directory contents
	void			SoftRefresh ();							// refresh directory contents, but don't touch marked flags
	void			SortItems ();							// sort items

	SortMode_e		GetSortMode () const;					// returns sort mode
	bool			GetSortReverse () const;

	HANDLE			GetActivePlugin () const;

protected:
	Str_c			m_sDir;
	SortMode_e		m_eSortMode;
	bool			m_bSortReverse;
	bool			m_bShowHidden;
	bool			m_bShowSystem;
	bool			m_bShowROM;

	HANDLE			m_hActivePlugin;

	virtual void	Event_PluginOpened ( HANDLE hPlugin );
	virtual void	Event_PluginClosed ();

	Str_c			StepUpToValid ( const Str_c & sCurrentDir );

private:
	Array_T <PanelItem_t>		m_dFiles;
	Array_T <ItemProperties_t>	m_dProperties;
	Array_T <int>	m_dSortedIndices;

	PanelItem_t *	m_pSourceItems;
	int				m_nSourceItems;

	PanelItem_t *	m_pPluginItems;
	int				m_nPluginItems;

	SortMode_e		m_eCachedSortMode;						// sort mode before plugin
	bool			m_bCachedSortReverse;					// sort reverse before plugin

	int				m_nMarked;
	ULARGE_INTEGER	m_MarkedSize;
	bool			m_bSlowRefresh;							// slow refresh occured
	HCURSOR			m_hOldCursor;							// previous cursor

	ItemProperties_t &	GetItemProperties ( int iItem );

	void			RefreshBaseFS ();
	void			UpdateItemProperties ();
	void			CloseActivePlugin ();
	void			HideCursor ();
};

#endif