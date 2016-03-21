#ifndef _pluginapi_
#define _pluginapi_

#include "pfm/dlg_enums.inc"

struct PanelItem_t
{
	WIN32_FIND_DATA m_FindData;
	int				m_iIcon;
	DWORD			m_uPackSize;
	wchar_t **		m_dCustomColumnData;
	int				m_nCustomColumns;
};


enum
{
	 OPIF_USEHIGHLIGHTING		= 1 << 0
	,OPIF_USEATTRHIGHLIGHTING	= 1 << 1
	,OPIF_REALNAMES				= 1 << 2
};


enum ViewMode_e
{
	 VM_UNDEFINED = -1
	,VM_COLUMN_1 = 0	// 1 column
	,VM_COLUMN_2		// 2 column
	,VM_COLUMN_3		// 3 column
	,VM_BRIEF			// name/size
	,VM_MEDIUM			// name/size/date
	,VM_FULL			// name/size/date/time
	,VM_TOTAL
};


enum SortMode_e
{
	 SORT_UNDEFINDED = -1
	,SORT_UNSORTED = 0
	,SORT_NAME
	,SORT_EXTENSION
	,SORT_SIZE
	,SORT_TIME_CREATE
	,SORT_TIME_ACCESS
	,SORT_TIME_WRITE
	,SORT_GROUP
	,SORT_SIZE_PACKED
	,SORT_TOTAL
};


enum ColumnType_e
{
	 COL_FILENAME
	,COL_TIME_CREATION
	,COL_TIME_ACCESS
	,COL_TIME_WRITE
	,COL_DATE_CREATION
	,COL_DATE_ACCESS
	,COL_DATE_WRITE
	,COL_SIZE
	,COL_SIZE_PACKED
	,COL_TEXT_0
	,COL_TEXT_1
	,COL_TEXT_2
	,COL_TEXT_3
	,COL_TEXT_4
	,COL_TEXT_5
	,COL_TEXT_6
	,COL_TEXT_7
	,COL_TEXT_8
	,COL_TEXT_9
};

typedef int ( * ItemCompare_t ) ( HANDLE, const PanelItem_t &, const PanelItem_t &, SortMode_e, bool );

struct PanelColumn_t
{
	ColumnType_e	m_eType;
	float			m_fMaxWidth;			// 0.0f == no limit
	bool			m_bRightAlign;
};


const int MAX_COLUMNS = 10;

struct PanelView_t
{
	PanelColumn_t 	m_dColumns [MAX_COLUMNS];
	int				m_nColumns;
};

struct PluginInfo_t
{
	DWORD			m_uFlags;
	HIMAGELIST		m_hImageList;
	PanelView_t *	m_dPanelViews [VM_TOTAL];
	ViewMode_e		m_eStartViewMode;
	SortMode_e		m_eStartSortMode;
	bool			m_bStartSortReverse;
	int 			m_nFileInfoCols;		// -1 == no override at all
	ColumnType_e *	m_dFileInfoCols;
};

enum IteratorType_e
{
	 ITERATOR_TREE
	,ITERATOR_PANEL
	,ITERATOR_PANEL_CACHED
};

struct IteratorInfo_t
{
	bool					m_bRootDir;
	bool					m_b2ndPassDir;
	wchar_t 				m_szFullPath [MAX_PATH];
	const WIN32_FIND_DATA *	m_pFindData;
};

typedef void ( * SlowOperationCallback_t ) ();

typedef HBITMAP			(*PFMLOADTGA)			( HINSTANCE, DWORD, SIZE * );

typedef BOOL			(*PFMDLGMOVING)			( HWND, UINT, WPARAM, LPARAM );
typedef void			(*PFMDLGACTIVATE)		( HWND, UINT, WPARAM, LPARAM );
typedef bool			(*PFMDLGINITFS)			( HWND, bool );
typedef void			(*PFMDLGCLOSEFS)		();
typedef bool			(*PFMDLGSIZE)			( HWND, UINT, WPARAM, LPARAM, bool );
typedef void			(*PFMDLGRESIZE)			( HWND, HWND, HWND *, int );

typedef void			(*PFMALIGNTEXT)			( HWND, const wchar_t * );
typedef const wchar_t *	(*PFMSIZETOSTR)			( const ULARGE_INTEGER &, wchar_t *, bool );
typedef float			(*PFMTIMESEC)			();
typedef void			(*PFMCALCSIZE)			( const wchar_t *, PanelItem_t **, int, ULARGE_INTEGER &, int &, int &, DWORD &, DWORD &, SlowOperationCallback_t );
typedef void			(*PFMSPLITPATH)			( const wchar_t *, wchar_t *, wchar_t *, wchar_t * );

typedef int				(*PFMDIALOGERROR)		( HWND, bool, const wchar_t *, int );
typedef HWND			(*PFMCREATETBAR)		( HWND, int, DWORD );

typedef void			(*PFMCREATEWAITWND)		();
typedef void			(*PFMDESTROYWAITWND)	();

// config callbacks
typedef void			(*LOADCSTR)	( const wchar_t * );
typedef const wchar_t *	(*SAVECSTR)	( int iId );

// config-related functions
typedef void	(*PFMCFGINT)	( HANDLE, wchar_t *, int * );
typedef void	(*PFMCFGDWORD)	( HANDLE, wchar_t *, DWORD * );
typedef void	(*PFMCFGFLOAT)	( HANDLE, wchar_t *, float * );
typedef void	(*PFMCFGSTR)	( HANDLE, wchar_t *, wchar_t *, int iMaxLength, LOADCSTR fnLoad, SAVECSTR fnSave );
typedef HANDLE	(*PFMCFGCREATE)	();
typedef void	(*PFMCFGDESTROY)( HANDLE );
typedef bool	(*PFMCFGLOAD)	( HANDLE, const wchar_t * );
typedef void	(*PFMCFGSAVE)	( HANDLE );

// filter-related funcs
typedef HANDLE	(*PFMFLTCREATE)	( const wchar_t *, bool );
typedef void	(*PFMFLTDESTROY) ( HANDLE );
typedef bool	(*PFMFLTFITS) ( HANDLE, const wchar_t * );

// iterator-related funcs
typedef HANDLE	(*PFMITERCREATE)	( IteratorType_e );
typedef void	(*PFMITERDESTROY)	( HANDLE );
typedef void	(*PFMITERSTART)		( HANDLE, const wchar_t *, PanelItem_t **, int, bool );
typedef bool	(*PFMITERNEXT)		( HANDLE );
typedef const IteratorInfo_t * (*PFMITERINFO) ( HANDLE );

// panel-related funcs
typedef void	(*PFMPANELSETDIR)		( HANDLE, const wchar_t * );
typedef void	(*PFMPANELREFRESH)		( HANDLE );
typedef void	(*PFMPANELSOFTREFRESH)	();
typedef void	(*PFMPANELSETCURSOR)	( HANDLE, const wchar_t * );
typedef void	(*PFMPANELMARKFILE)		( const wchar_t *, bool );

// error handling funcs
typedef HANDLE			(*PFMERRCREATEOP)		( const wchar_t *, int, bool );
typedef void			(*PFMERRDESTROYOP)		( HANDLE );
typedef void			(*PFMERRADDWIN)			( HANDLE, int, int, const wchar_t * );
typedef void			(*PFMERRADDCUSTOM)		( HANDLE, bool, int, int, const wchar_t * );
typedef void			(*PFMERRSETOP)			( HANDLE );
typedef ErrResponse_e	(*PFMERRSHOWDLG)		( HWND, int, bool, const wchar_t *, const wchar_t * );
typedef bool			(*PFMERRISINDLG)		();
										
struct PluginStartupInfo_t
{
	HWND				m_hMainWindow;
	bool				m_bVGA;
	HFONT				m_hBoldFont;
	HFONT				m_hSmallFont;
	const wchar_t **	m_dStrings;
	int					m_nStrings;
	const wchar_t *		m_szRoot;
	const wchar_t *		m_szRootFSShortcut;

	// misc
	PFMLOADTGA			m_fnLoadTGA;
	PFMDLGMOVING		m_fnDlgMoving;
	PFMDLGINITFS		m_fnDlgInitFullscreen;
	PFMDLGCLOSEFS		m_fnDlgCloseFullscreen;
	PFMDLGACTIVATE		m_fnDlgHandleActivate;
	PFMDLGSIZE			m_fnDlgHandleSizeChange;
	PFMDLGRESIZE		m_fnDlgDoResize;

	PFMALIGNTEXT		m_fnAlignText;
	PFMSIZETOSTR		m_fnSizeToStr;
	PFMTIMESEC			m_fnTimeSec;
	PFMCALCSIZE			m_fnCalcSize;
	PFMSPLITPATH		m_fnSplitPath;

	// dialogs
	PFMDIALOGERROR		m_fnDialogError;
	PFMCREATETBAR		m_fnCreateToolbar;
	PFMCREATEWAITWND	m_fnCreateWaitWnd;
	PFMDESTROYWAITWND	m_fnDestroyWaitWnd;

	// config-related
	PFMCFGINT			m_fnCfgInt;
	PFMCFGDWORD			m_fnCfgDword;
	PFMCFGFLOAT			m_fnCfgFloat;
	PFMCFGSTR			m_fnCfgStr;

	PFMCFGCREATE		m_fnCfgCreate;
	PFMCFGDESTROY		m_fnCfgDestroy;
	PFMCFGLOAD			m_fnCfgLoad;
	PFMCFGSAVE			m_fnCfgSave;

	// filter-related
	PFMFLTCREATE		m_fnFilterCreate;
	PFMFLTDESTROY		m_fnFilterDestroy;
	PFMFLTFITS			m_fnFilterFits;

	// file iterator-related
	PFMITERCREATE		m_fnIteratorCreate;
	PFMITERDESTROY		m_fnIteratorDestroy;
	PFMITERSTART		m_fnIteratorStart;
	PFMITERNEXT			m_fnIteratorNext;
	PFMITERINFO			m_fnIteratorInfo;

	// panel-related
	PFMPANELSETDIR		m_fnPanelSetDir;
	PFMPANELREFRESH		m_fnFSPanelRefresh;
	PFMPANELSOFTREFRESH	m_fnPanelSoftRefreshAll;
	PFMPANELSETCURSOR	m_fnPanelSetCursor;
	PFMPANELMARKFILE	m_fnPanelMarkFile;			// marks file on an active panel

	// error handling
	PFMERRCREATEOP		m_fnErrCreateOperation;
	PFMERRDESTROYOP		m_fnErrDestroyOperation;
	PFMERRADDWIN		m_fnErrAddWinErrorHandler;
	PFMERRADDCUSTOM		m_fnErrAddCustomErrorHandler;
	PFMERRSETOP			m_fnErrSetOperation;
	PFMERRSHOWDLG		m_fnErrShowErrorDlg;
	PFMERRISINDLG		m_fnErrIsInDialog;
};

#endif