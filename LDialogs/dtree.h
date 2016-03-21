#ifndef _dtree_
#define _dtree_

#include "LCore/cmain.h"
#include "LCore/cstr.h"
#include "LFile/fsorter.h"
#include "LCore/carray.h"

class DirTree_c
{
public:
				DirTree_c ();
				~DirTree_c ();

	void		Create ( HWND hParent );
	void		Populate ( const Str_c & sPath );
	void		ExpandPath ( const Str_c & sPath );

	void		OnExpand ( TV_ITEM & tItem );
	void		OnSelect ( TV_ITEM & tItem  );

	void		EnsureVisible ();

	HWND 		Handle () const;
	const Str_c & Path () const;

protected:
	FileSorter_c	m_Sorter;

	virtual void Event_Selected () {}

private:
	struct TreeItemInfo_t
	{
		TreeItemInfo_t ()
			: m_bChildrenLoaded ( false )
		{
		}

		bool	m_bChildrenLoaded;
		Str_c 	m_sPath;
	};

	HWND			m_hTree;
	bool			m_bImageListSet;
	Str_c			m_sPath;

	Array_T < TreeItemInfo_t * > m_dAllocated;
	HTREEITEM	FindDir ( HTREEITEM hParent, const Str_c & sPath );
	void		PopulateTreeLevel ( const Str_c & sPath, HTREEITEM hParent );
};

struct TreeFilterEntry_t
{
	TreeFilterEntry_t ()
	{
	}

	TreeFilterEntry_t ( const wchar_t * szName, const wchar_t * szFilter )
		: m_sName	( szName )
		, m_sFilter	( szFilter )
	{
	}

	Str_c		m_sName;
	Str_c		m_sFilter;
};

typedef Array_T <TreeFilterEntry_t> TreeFilters_t;

class FileTree_c : public DirTree_c
{
public:
					FileTree_c ( bool bAcceptDirs );
					~FileTree_c ();

	void			Create ( HWND hParent );

	void			SetFilter ( const Str_c & sFilter );
	void			SetSortMode ( SortMode_e eSort );

	void			OnGetDispInfo ( LV_DISPINFO * pInfo );
	void			OnItemChanged ( NMLISTVIEW * pInfo );
	void			OnItemActivate ( NMLISTVIEW * pInfo );

	HWND			ListHandle ();
	void			EnsureVisibleList ();
	bool			HaveSelectedFile () const;
	bool			HaveSelectedDir () const;
	const Str_c &	Filename () const;
	void			PopulateList ();

protected:
	ExtFilter_c		m_Filter;
	Str_c			m_sFilter;
	HWND			m_hList;
	bool			m_bAcceptDirs;
	Str_c			m_sFilename;
	SortMode_e		m_eSortMode;

	virtual void	Event_Selected ();
};


bool ShowDirTreeDlg ( HWND hParent, Str_c & sPath );
bool ShowFileTreeDlg ( HWND hParent, Str_c & sPath, bool bAcceptFolders, const TreeFilters_t * dFilters = NULL );

#endif