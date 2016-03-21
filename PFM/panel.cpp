#include "pch.h"

#include "panel.h"
#include "config.h"
#include "protection.h"
#include "resources.h"
#include "plugins.h"
#include "pfm.h"
#include "filefuncs.h"
#include "filemisc.h"
#include "menu.h"

#include "dialogs/bmmenu.h"
#include "dialogs/bookmarks.h"
#include "dialogs/register.h"

#include "LFile/fclipboard.h"

#include "aygshell.h"
#include "Dlls/Resource/resource.h"

WNDPROC	Panel_c::m_pOldBtnProc = NULL;

extern HINSTANCE	g_hAppInstance;
extern HWND			g_hMainWindow;

static const wchar_t g_szPanelClass [] = L"PanelClass";

const int SPACE_BETWEEN_LINES = 1;

//////////////////////////////////////////////////////////////////////////
static bool FixupColumns ( ViewMode_t & Mode )
{
	bool bFilenameColMode = false;

	int nFilenameColumns = 0;

	for ( int i = 0; i < Mode.m_nColumns; ++i )
		if ( Mode.m_dColumns [i].m_eType == COL_FILENAME )
			nFilenameColumns++;

	bFilenameColMode = nFilenameColumns > 1 || nFilenameColumns == Mode.m_nColumns;

	if ( bFilenameColMode )
	{
		Mode.m_nColumns = nFilenameColumns;
		for ( int i = 0; i < Mode.m_nColumns; ++i )
			Mode.m_dColumns [i].m_eType = COL_FILENAME;
	}

	return bFilenameColMode;
}

//////////////////////////////////////////////////////////////////////////
ViewModes_t::ViewModes_t ()
{
	Reset ();
}


void ViewModes_t::Reset ()
{
	memset ( m_dViewModes, 0, sizeof ( m_dViewModes ) );
	ViewMode_t * pVM = &(m_dViewModes [VM_COLUMN_1]);
	pVM->m_dColumns [0].m_eType	= COL_FILENAME;
	pVM->m_nColumns		= 1;
	pVM->m_bSeparateExt	= cfg->pane_col1_ext;
	pVM->m_bShowIcons	= cfg->pane_col1_icons;

	pVM = &(m_dViewModes [VM_COLUMN_2]);
	pVM->m_dColumns [0].m_eType = COL_FILENAME;
	pVM->m_dColumns [1].m_eType = COL_FILENAME;
	pVM->m_nColumns		= 2;
	pVM->m_bSeparateExt = cfg->pane_col2_ext;
	pVM->m_bShowIcons	= cfg->pane_col2_icons;

	pVM = &(m_dViewModes [VM_COLUMN_3]);
	pVM->m_dColumns [0].m_eType = COL_FILENAME;
	pVM->m_dColumns [1].m_eType = COL_FILENAME;
	pVM->m_dColumns [2].m_eType = COL_FILENAME;
	pVM->m_nColumns		= 3;
	pVM->m_bSeparateExt = cfg->pane_col3_ext;
	pVM->m_bShowIcons	= cfg->pane_col3_icons;

	pVM = &(m_dViewModes [VM_BRIEF]);
	pVM->m_dColumns [0].m_eType			= COL_FILENAME;
	pVM->m_dColumns [0].m_bRightAlign	= false;
	pVM->m_dColumns [1].m_eType			= COL_SIZE;
	pVM->m_dColumns [1].m_bRightAlign	= true;
	pVM->m_nColumns		= 2;
	pVM->m_bSeparateExt = cfg->pane_brief_ext;
	pVM->m_bShowIcons	= cfg->pane_brief_icons;

	pVM = &(m_dViewModes [VM_MEDIUM]);
	pVM->m_dColumns [0].m_eType			= COL_FILENAME;
	pVM->m_dColumns [0].m_bRightAlign	= false;
	pVM->m_dColumns [1].m_eType			= COL_SIZE;
	pVM->m_dColumns [1].m_bRightAlign	= true;
	pVM->m_dColumns [2].m_eType			= COL_DATE_WRITE;
	pVM->m_dColumns [2].m_bRightAlign	= true;
	pVM->m_nColumns		= 3;
	pVM->m_bSeparateExt = cfg->pane_medium_ext;
	pVM->m_bShowIcons	= cfg->pane_medium_icons;

	pVM = &(m_dViewModes [VM_FULL]);
	pVM->m_dColumns [0].m_eType			= COL_FILENAME;
	pVM->m_dColumns [0].m_bRightAlign	= false;
	pVM->m_dColumns [1].m_eType			= COL_SIZE;
	pVM->m_dColumns [1].m_bRightAlign	= true;
	pVM->m_dColumns [2].m_eType			= COL_DATE_WRITE;
	pVM->m_dColumns [2].m_bRightAlign	= true;
	pVM->m_dColumns [3].m_eType			= COL_TIME_WRITE;
	pVM->m_dColumns [3].m_bRightAlign	= true;
	pVM->m_nColumns		= 4;
	pVM->m_bSeparateExt = cfg->pane_full_ext;
	pVM->m_bShowIcons	= cfg->pane_full_icons;
}


const PanelColumn_t & ViewModes_t::GetColumn ( ViewMode_e eMode, int iColumn ) const
{
	Assert ( iColumn >= 0 && iColumn < MAX_COLUMNS );
	return m_dViewModes [eMode].m_dColumns [iColumn];
}


///////////////////////////////////////////////////////////////////////////////////////////
// panel button
Panel_c::Button_t::Button_t ()
	: m_hButton ( NULL )
{
}


void Panel_c::Button_t::SetPosition ( HWND hParent, int iX, int iY, bool bVisible )
{
	if ( bVisible )
		SetWindowPos ( m_hButton, hParent, iX, iY, g_tResources.m_iTopIconSize, g_tResources.m_iTopIconSize, SWP_SHOWWINDOW );
	else
		ShowWindow ( m_hButton, SW_HIDE );
}


void Panel_c::Button_t::GetPosition ( int & iX, int & iY )
{
	RECT Rect;
	GetWindowRect ( m_hButton, &Rect );
	iX = Rect.left;
	iY = Rect.top;
}


void Panel_c::Button_t::Draw ( const DRAWITEMSTRUCT * pDrawItem, HBITMAP hBmp, HBITMAP hBmpPressed, bool bActive )
{
	FillRect ( pDrawItem->hDC, &pDrawItem->rcItem, bActive ? g_tResources.m_hActiveHdrBrush : g_tResources.m_hPassiveHdrBrush );

	bool bPressed = pDrawItem->itemState & ODS_SELECTED;

	SelectObject ( g_tResources.m_hBitmapDC, bPressed ? hBmpPressed : hBmp );

	TransparentBlt ( pDrawItem->hDC, 0, 0, g_tResources.m_iTopIconSize, g_tResources.m_iTopIconSize,
		g_tResources.m_hBitmapDC, 0, 0, g_tResources.m_iTopIconSize, g_tResources.m_iTopIconSize, BMP_TRANS_CLR );
}

//////////////////////////////////////////////////////////////////////////
Panel_c::ColumnSize_t::ColumnSize_t ()
	: m_iMaxTextWidth	( 0 )
	, m_iLeft			( 0 )
	, m_iRight			( 0 )
{
}

///////////////////////////////////////////////////////////////////////////////////////////
// common panel view
Panel_c::Panel_c  ()
	: m_bActive				( false )
	, m_hWnd				( NULL )
	, m_bMaximized			( false )
	, m_bVisible			( false )
	, m_bInMenu				( false )
	, m_hMemBitmap			( NULL )
	, m_hMemDC				( NULL )
	, m_hSlider				( NULL )
	, m_iFirstItem			( 0 )
	, m_iCursorItem			( 0 )
	, m_bSliderVisible		( false )
	, m_bFirstDrawAfterRefresh	( false )
	, m_bFilenameColMode	( false )
	, m_hImageList			( g_tResources.m_hSmallIconList )
	, m_hCachedImageList	( g_tResources.m_hSmallIconList )
	, m_hSortModeMenu		( NULL )
	, m_hViewModeMenu		( NULL )
	, m_hHeaderMenu			( NULL )
	, m_eViewMode			( VM_COLUMN_2 )
	, m_eViewModeBeforeMax	( VM_COLUMN_2 )
	, m_eCachedViewMode		( VM_COLUMN_2 )
	, m_iLastMarkFile		( -1 )
	, m_bMarkOnDrag			( false )
	, m_fTimeSinceRefresh	( 0.0f )
	, m_bRefreshAnyway		( false )
	, m_bRefreshQueued		( false )
{
	memset ( &m_BmpSize,	0, sizeof ( m_BmpSize ) );
	memset ( &m_Rect,		0, sizeof ( m_Rect ) );
	memset ( &m_dColumnSizes, 0, sizeof ( m_dColumnSizes ) );
	memset ( &m_MarkedRect, 0, sizeof ( m_MarkedRect ) );
	memset ( &m_LastMouse, 0, sizeof ( m_LastMouse ) );

	m_hWnd = CreateWindow ( g_szPanelClass, L"", WS_CHILD | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, g_hMainWindow, NULL, g_hAppInstance, NULL );
	Assert ( m_hWnd );

	m_hSlider = CreateWindow ( L"SCROLLBAR", L"", WS_CHILD | SBS_VERT, CW_USEDEFAULT, CW_USEDEFAULT, cfg->pane_slider_size_x, CW_USEDEFAULT, m_hWnd, NULL, g_hAppInstance, NULL );
	Assert ( m_hSlider );

	CreateMenus ();
	CreateDrawObjects ();
}


Panel_c::~Panel_c ()
{
	if ( m_hSlider )
		DestroyWindow ( m_hSlider );

	if ( m_hMemBitmap )
		DeleteObject ( m_hMemBitmap );

	if ( m_hMemDC )
		DeleteDC ( m_hMemDC );

	if ( m_hWnd )
		DestroyWindow ( m_hWnd );

	DestroyMenu ( m_hHeaderMenu );
}


bool Panel_c::Init ()
{
	WNDCLASS tWC;
	memset ( &tWC, 0, sizeof ( tWC ) );
	tWC.lpfnWndProc		= (WNDPROC) Panel_c::WndProc;
	tWC.style			= CS_DBLCLKS;
	tWC.hInstance		= g_hAppInstance;
	tWC.hIcon			= NULL;
	tWC.hCursor			= NULL;
	tWC.hbrBackground	= (HBRUSH) GetStockObject ( NULL_BRUSH );
	tWC.lpszMenuName	= NULL;
	tWC.lpszClassName	= g_szPanelClass;

	return !!RegisterClass ( &tWC );
}


void Panel_c::Shutdown ()
{
	UnregisterClass ( g_szPanelClass, g_hAppInstance );
}


const wchar_t * Panel_c::FormatTimeCreation ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return FormatTime ( szBuf, Item.m_FindData.ftCreationTime );
}


const wchar_t * Panel_c::FormatTimeAccess ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return FormatTime ( szBuf, Item.m_FindData.ftLastAccessTime );
}


const wchar_t * Panel_c::FormatTimeWrite ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return FormatTime ( szBuf, Item.m_FindData.ftLastWriteTime );
}


const wchar_t * Panel_c::FormatDateCreation ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return FormatDate ( szBuf, Item.m_FindData.ftCreationTime );
}


const wchar_t * Panel_c::FormatDateAccess ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return FormatDate ( szBuf, Item.m_FindData.ftLastAccessTime );
}


const wchar_t * Panel_c::FormatDateWrite ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return FormatDate ( szBuf, Item.m_FindData.ftLastWriteTime );
}


const wchar_t * Panel_c::FormatSize ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return FormatSize ( szBuf, Item.m_FindData.nFileSizeHigh, Item.m_FindData.nFileSizeLow, !!(Item.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) );
}


const wchar_t * Panel_c::FormatSizePacked ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return FormatSize ( szBuf, 0, Item.m_uPackSize, !!(Item.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) );
}


const wchar_t * Panel_c::FormatText0 ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return Item.m_nCustomColumns > 0 ? Item.m_dCustomColumnData [0] : NULL;
}


const wchar_t * Panel_c::FormatText1 ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return Item.m_nCustomColumns > 1 ? Item.m_dCustomColumnData [1] : NULL;
}


const wchar_t * Panel_c::FormatText2 ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return Item.m_nCustomColumns > 2 ? Item.m_dCustomColumnData [2] : NULL;
}


const wchar_t * Panel_c::FormatText3 ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return Item.m_nCustomColumns > 3 ? Item.m_dCustomColumnData [3] : NULL;
}


const wchar_t * Panel_c::FormatText4 ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return Item.m_nCustomColumns > 4 ? Item.m_dCustomColumnData [4] : NULL;
}


const wchar_t * Panel_c::FormatText5 ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return Item.m_nCustomColumns > 5 ? Item.m_dCustomColumnData [5] : NULL;
}


const wchar_t * Panel_c::FormatText6 ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return Item.m_nCustomColumns > 6 ? Item.m_dCustomColumnData [6] : NULL;
}


const wchar_t * Panel_c::FormatText7 ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return Item.m_nCustomColumns > 7 ? Item.m_dCustomColumnData [7] : NULL;
}


const wchar_t * Panel_c::FormatText8 ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return Item.m_nCustomColumns > 8 ? Item.m_dCustomColumnData [8] : NULL;
}


const wchar_t * Panel_c::FormatText9 ( wchar_t * szBuf, const PanelItem_t & Item )
{
	return Item.m_nCustomColumns > 9 ? Item.m_dCustomColumnData [9] : NULL;
}


const wchar_t * Panel_c::FormatTime ( wchar_t * szBuf, const FILETIME & Time )
{
	szBuf [0] = L'\0';
	SYSTEMTIME SysTime;
	FILETIME LocalFileTime;
	FileTimeToLocalFileTime ( &Time, &LocalFileTime );
	FileTimeToSystemTime ( &LocalFileTime, &SysTime );
	GetTimeFormat ( LOCALE_SYSTEM_DEFAULT, TIME_NOSECONDS | TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT, &SysTime, NULL, szBuf, TIMEDATE_BUFFER_SIZE );
	return szBuf;
}


const wchar_t * Panel_c::FormatDate ( wchar_t * szBuf, const FILETIME & Date )
{
	szBuf [0] = L'\0';
	SYSTEMTIME SysTime;
	FILETIME LocalFileTime;
	FileTimeToLocalFileTime ( &Date, &LocalFileTime );
	FileTimeToSystemTime ( &LocalFileTime, &SysTime );
	GetDateFormat ( LOCALE_SYSTEM_DEFAULT, DATE_SHORTDATE, &SysTime, NULL, szBuf, TIMEDATE_BUFFER_SIZE );
	return szBuf;
}


Panel_c::fnFormat Panel_c::GetFormatFunc ( ColumnType_e eType ) const
{
	switch ( eType )
	{
		case COL_TIME_CREATION:	return FormatTimeCreation;
		case COL_TIME_ACCESS:	return FormatTimeAccess;
		case COL_TIME_WRITE:	return FormatTimeWrite;
		case COL_DATE_CREATION:	return FormatDateCreation;
		case COL_DATE_ACCESS:	return FormatDateAccess;
		case COL_DATE_WRITE:	return FormatDateWrite;
		case COL_SIZE:			return FormatSize;
		case COL_SIZE_PACKED:	return FormatSizePacked;
		case COL_TEXT_0:		return FormatText0;
		case COL_TEXT_1:		return FormatText1;
		case COL_TEXT_2:		return FormatText2;
		case COL_TEXT_3:		return FormatText3;
		case COL_TEXT_4:		return FormatText4;
		case COL_TEXT_5:		return FormatText5;
		case COL_TEXT_6:		return FormatText6;
		case COL_TEXT_7:		return FormatText7;
		case COL_TEXT_8:		return FormatText8;
		case COL_TEXT_9:		return FormatText9;
	}

	return NULL;
}


void Panel_c::DrawColumn ( int iId, HDC hDC, int iStartItem, int nItems, const RECT & DrawRect )
{
	RECT Rect = DrawRect;

	if ( iStartItem >= GetNumItems () )
		return;

	const PanelColumn_t & Column = m_ViewModes.GetColumn ( m_eViewMode, iId );
	bool bAlignRight = Column.m_bRightAlign;

	if ( Column.m_eType == COL_FILENAME )
		DrawFileNames ( hDC, iStartItem, nItems, Rect );
	else
	{
		fnFormat FormatFunc = GetFormatFunc ( Column.m_eType );
		if ( FormatFunc )
		{
			bool bClip = DrawRect.right - DrawRect.left < m_dColumnSizes [iId].m_iMaxTextWidth;
			DrawColText ( hDC, iStartItem, nItems, Rect, bAlignRight, bClip, FormatFunc );
		}
	}
}


void Panel_c::DrawFileNames ( HDC hDC, int iStartItem, int nItems, const RECT & DrawRect )
{
	const int iIconWidth	= GetSystemMetrics ( SM_CXSMICON );
	const int iIconHeight	= GetSystemMetrics ( SM_CYSMICON );

	int iSymbolHeight		= GetItemHeight ();
	int nItemsTotal			= GetNumItems ();
	bool bShowIcons			= m_ViewModes.m_dViewModes [m_eViewMode].m_bShowIcons;
	bool bSeparateExt		= m_ViewModes.m_dViewModes [m_eViewMode].m_bSeparateExt;

	const wchar_t * szDirMarker = NULL;
	int iDirMarkerLen = 0;
	RECT MarkerRect;
	memset ( &MarkerRect, 0, sizeof ( MarkerRect ) );

	if ( cfg->pane_dir_marker )
	{
		szDirMarker = cfg->pane_dir_marker_txt;
		iDirMarkerLen = wcslen ( szDirMarker );
		DrawText ( hDC, szDirMarker, iDirMarkerLen, &MarkerRect, DT_NOCLIP | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX );
	}

	RECT CalcRect;
	RECT StringRect;

	HGDIOBJ hOldPen = SelectObject ( hDC, g_tResources.m_hOverflowPen );

	static wchar_t szName [MAX_PATH];

	for ( int iCurItem = iStartItem; iCurItem - iStartItem < nItems && iCurItem < nItemsTotal; ++iCurItem )
	{
		const PanelItem_t & Item = GetItem ( iCurItem );
		const wchar_t * szExt = L"";

		szExt = GetNameExtFast ( Item.m_FindData.cFileName, szName );

		int iLen		= wcslen ( Item.m_FindData.cFileName );
		int iNameLen	= wcslen ( szName );
		int iExtLen		= wcslen ( szExt );

		// 1 for space between lines and 1 for space before 1st line
		StringRect.top		= DrawRect.top + ( iCurItem - iStartItem ) * ( iSymbolHeight + SPACE_BETWEEN_LINES ) + SPACE_BETWEEN_LINES;
		StringRect.bottom	= StringRect.top + iSymbolHeight;
		StringRect.left		= DrawRect.left + 1;
		StringRect.right	= DrawRect.right - 1;

		// draw icon
		if ( bShowIcons && StringRect.right - StringRect.left >= iIconWidth )
		{
			int iIcon = GetItemIcon ( iCurItem );
			if ( iIcon != -1 )
			{
				int iIconY = ( StringRect.bottom - StringRect.top - iIconHeight ) / 2 + StringRect.top;
				ImageList_Draw ( m_hImageList, iIcon, hDC, StringRect.left, iIconY, ILD_TRANSPARENT );
			}
			StringRect.left += iIconWidth + 1;
		}
		
		SetTextColor ( hDC, GetItemColor ( iCurItem, iCurItem == m_iCursorItem ) );
		bool bDirectory = !! ( Item.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );

		// calc the filename's precise width
		memset ( &CalcRect, 0, sizeof ( CalcRect ) );
		DrawText ( hDC, Item.m_FindData.cFileName, iLen, &CalcRect, DT_NOCLIP | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX );
		int iNameWidth = CalcRect.right - CalcRect.left;
		bool bDrawClipped = false;
		bool bNameOnly = false;

		// special case: dirname <dir>
		if ( bDirectory && cfg->pane_dir_marker )
		{
			DrawText ( hDC, szDirMarker, iDirMarkerLen, &StringRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
			StringRect.right -= MarkerRect.right - MarkerRect.left + 1;

			if ( iNameWidth > StringRect.right - StringRect.left )
			{
				StringRect.right -= 2;
				bDrawClipped = true;
			}
			else
				DrawText ( hDC, szName, iNameLen, &StringRect, DT_LEFT | DT_NOCLIP | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
		}
		else
		{
			if ( iNameWidth > StringRect.right - StringRect.left )
			{
				bDrawClipped = true;

				if ( cfg->pane_long_cut == 0 && ! bDirectory )
				{
					// draw the ext aligned right
					DrawText ( hDC, szExt, iExtLen, &StringRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
					memset ( &CalcRect, 0, sizeof ( CalcRect ) );
					DrawText ( hDC, szExt, iExtLen, &CalcRect, DT_NOCLIP | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX );

					StringRect.right -= CalcRect.right - CalcRect.left + 3;
				}
				else
					StringRect.right -= 2;
			}
			else
				if ( bSeparateExt && !bDirectory )
				{
					bNameOnly = true;
					DrawText ( hDC, szExt, iExtLen, &StringRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
				}
		}

		if ( StringRect.right > StringRect.left )
		{
			if ( bDrawClipped )
			{
				MoveToEx ( hDC, StringRect.right + 1, StringRect.top, NULL );
				LineTo ( hDC, StringRect.right + 1, StringRect.bottom );
			}

			DrawText ( hDC, Item.m_FindData.cFileName, bNameOnly ? iNameLen : iLen,
					&StringRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | ( bDrawClipped ? 0 : DT_NOCLIP ) );
		}
	}

	SelectObject ( hDC, hOldPen );
}


void Panel_c::DrawColText ( HDC hDC, int iStartItem, int nItems, const RECT & DrawRect, bool bAlignRight, bool bClip, fnFormat Format )
{
	int iSymbolHeight		= GetItemHeight ();
	int nItemsTotal			= GetNumItems ();

	wchar_t szTmpBuf [TIMEDATE_BUFFER_SIZE];
	RECT StringRect, CalcRect;

	HGDIOBJ hOldPen = SelectObject ( hDC, g_tResources.m_hOverflowPen );

	for ( int iCurItem = iStartItem; iCurItem - iStartItem < nItems && iCurItem < nItemsTotal; ++iCurItem )
	{
		const PanelItem_t & Item = GetItem ( iCurItem );

		// 1 for space between lines and 1 for space before 1st line
		StringRect.top		= DrawRect.top + ( iCurItem - iStartItem ) * ( iSymbolHeight + SPACE_BETWEEN_LINES ) + SPACE_BETWEEN_LINES;
		StringRect.bottom	= StringRect.top + iSymbolHeight;
		StringRect.left		= DrawRect.left + 1;
		StringRect.right	= DrawRect.right - 1;

		const wchar_t * szRes = Format ( szTmpBuf, Item );
		if ( !szRes )
			continue;

		SetTextColor ( hDC, GetItemColor ( iCurItem, iCurItem == m_iCursorItem ) );

		bool bDrawClipped = bClip;

		int iLen = wcslen ( szRes );

		if ( bClip )
		{
			memset ( &CalcRect, 0, sizeof ( CalcRect ) );
			DrawText ( hDC, szRes, iLen, &CalcRect, DT_NOCLIP | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX );
			if ( CalcRect.right - CalcRect.left > StringRect.right - StringRect.left )
			{
				bDrawClipped	= true;
				bAlignRight		= false;
				StringRect.right -= 2;
				MoveToEx ( hDC, StringRect.right + 1, StringRect.top, NULL );
				LineTo ( hDC, StringRect.right + 1, StringRect.bottom );
			}
		}

		DrawText ( hDC, szRes, iLen, &StringRect, ( bAlignRight ? DT_RIGHT : DT_LEFT ) | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | ( bDrawClipped ? 0 : DT_NOCLIP ) );
	}

	SelectObject ( hDC, hOldPen );
}


int Panel_c::GetMinHeight () const
{
	return GetHeaderHeight () + GetFooterHeight () + 2 + cfg->pane_slider_size_x * 2;
}


int Panel_c::GetMinWidth () const
{
	return Max ( g_tResources.m_iTopIconSize * 3, cfg->pane_slider_size_x );
}


LRESULT CALLBACK Panel_c::BtnProc ( HWND hWnd, UINT Msg, UINT wParam, LONG lParam )
{
	Assert ( m_pOldBtnProc );

	if ( Msg == WM_CAPTURECHANGED  )
		SetFocus ( g_hMainWindow );

	return CallWindowProc ( m_pOldBtnProc, hWnd, Msg, wParam, lParam );
}


void Panel_c::SetRect ( const RECT & tRect )
{
	Assert ( m_hWnd );

	SetWindowPos ( m_hWnd, g_hMainWindow, tRect.left, tRect.top, tRect.right - tRect.left, tRect.bottom - tRect.top, m_bVisible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW );
	GetClientRect ( m_hWnd, &m_Rect );

	int iIconSize = g_tResources.m_iTopIconSize;

	m_BtnBookmarks.SetPosition ( m_hWnd, m_Rect.left, m_Rect.top, true );
	m_BtnMax.SetPosition ( m_hWnd, m_Rect.right - iIconSize, m_Rect.top, true );
	m_BtnUp.SetPosition ( m_hWnd, m_Rect.right - iIconSize * 2, m_Rect.top, true );
	m_BtnSwitch.SetPosition ( m_hWnd, m_Rect.right - iIconSize * 3, m_Rect.top, m_bMaximized );

	UpdateScrollInfo ();
	InvalidateRect ( m_hWnd, NULL, FALSE );
}


bool Panel_c::IsActive () const
{
	return m_bActive;
}


void Panel_c::Activate ( bool bActive )
{
	if ( m_bActive != bActive )
	{
		m_bActive = bActive;
		InvalidateHeader ();
		InvalidateCursor ();
	}
}


void Panel_c::EnterMenu ()
{
	m_bInMenu = true;
}


void Panel_c::ExitMenu ()
{
	m_bInMenu = false;
}


bool Panel_c::IsInMenu () const
{
	return m_bInMenu;
}


FindOnScreen_e Panel_c::FindFile ( int & iResult, int iX, int iY ) const
{
	iResult = -1;

	RECT HeaderRect;
	GetHeaderRect ( HeaderRect );

	if ( IsInRect ( iX, iY, HeaderRect ) )
		return FIND_PANEL_TOP;
	else
	{
		RECT FooterRect;
		GetFooterRect ( FooterRect );
		if ( IsInRect ( iX, iY, FooterRect ) )
			return FIND_PANEL_BOTTOM;
	}

	RECT CenterRect;
	GetCenterRect ( CenterRect );
	if ( !IsInRect ( iX, iY, CenterRect) )
		return FIND_OUT_OF_PANEL;

	int nItems = GetNumItems ();
	int iOffset = ( iY - CenterRect.top ) / GetRowHeight ();

	if ( m_bFilenameColMode )
	{
		int nColumns = m_ViewModes.m_dViewModes [m_eViewMode].m_nColumns;

		int iColumn = -1;
		for ( int i = 0; i < nColumns && iColumn == -1; ++i )
		{
			if ( iX >= m_dColumnSizes [i].m_iLeft && iX <= m_dColumnSizes [i].m_iRight )
				iColumn = i;
		}
		
		if ( iColumn == -1 )
			return FIND_OUT_OF_PANEL;

		iOffset += iColumn * GetMaxItemsInColumn ();
	}

	if ( m_iFirstItem + iOffset >= nItems )
		return FIND_EMPTY_PANEL;

	iResult = m_iFirstItem + iOffset;
	return FIND_FILE_FOUND;
}


void Panel_c::SetCursorItem ( int iCursorItem )
{
	m_iCursorItem = iCursorItem;
}


void Panel_c::SetFirstItem ( int iFirstItem )
{
	m_iFirstItem = iFirstItem;
}


int Panel_c::ClampCursorItem ( int iCursor ) const
{
	int nViewFiles	= GetMaxItemsInView ();
	int nFiles		= GetNumItems ();
	int nMinCursor	= Min ( m_iFirstItem + nViewFiles - 1,  nFiles - 1 );
	int iRes		= iCursor;

	// in case there are no files
	if ( nMinCursor < 0 )
		return 0;

	if ( iCursor > nMinCursor )
		iRes = nMinCursor;

	if ( iCursor < m_iFirstItem )
		iRes = m_iFirstItem;

	return iRes;
}


int Panel_c::ClampFirstItem ( int iFirst ) const
{
	const int iDelta = GetNumItems () - GetMaxItemsInView ();
	int iRes = iFirst;
	if ( iRes > iDelta )
		iRes = iDelta;

	if ( iRes < 0 )
		iRes = 0;

	return iRes;
}



void Panel_c::ScrollItemsTo ( int iFirstItem )
{
	int iNewFirstItem	= ClampFirstItem ( iFirstItem );
	int iNewCursorItem	= m_iCursorItem;

	if ( iNewFirstItem != m_iFirstItem )
	{
		m_iFirstItem = iNewFirstItem;
		InvalidateCenter ();

		iNewCursorItem = ClampCursorItem ( iNewCursorItem );

		if ( iNewCursorItem != m_iCursorItem )
		{
			SetCursorItem ( iNewCursorItem );
			InvalidateFooter ();
		}
	}
}


void Panel_c::SetCursorItemInvalidate ( int iCursor )
{
	int iNewCursor = ClampCursorItem ( iCursor );
	if ( m_iCursorItem != iNewCursor )
	{
		InvalidateCursor ();
		SetCursorItem ( iNewCursor );
		InvalidateCursor ();
		InvalidateFooter ();
	}
}


void Panel_c::SetCursorAtItem ( const wchar_t * szName )
{
	for ( int i = 0; i < GetNumItems (); ++i )
	{
		const PanelItem_t & Item = GetItem ( i );
		if ( !wcscmp ( Item.m_FindData.cFileName, szName ) )
		{
			SetCursorItem ( i );
			break;
		}
	}

	int nViewItems = GetMaxItemsInView ();
	if ( m_iCursorItem >= m_iFirstItem + nViewItems )
		m_iFirstItem = m_iCursorItem - nViewItems + 1;

	m_iFirstItem = ClampFirstItem ( m_iFirstItem );
	SetCursorItem ( ClampCursorItem ( m_iCursorItem ) );

	UpdateScrollPos ();
}


void Panel_c::ScrollCursorTo ( int iCursorItem )
{
	if ( iCursorItem < m_iFirstItem )
	{
		ScrollItemsTo ( iCursorItem );
		SetCursorItemInvalidate ( iCursorItem );
		UpdateScrollPos ();
	}
	else
	{
		int nViewItems = GetMaxItemsInView ();
		if ( iCursorItem < nViewItems + m_iFirstItem )
			SetCursorItemInvalidate ( iCursorItem );
		else
		{
			ScrollItemsTo ( iCursorItem - nViewItems + 1 );
			SetCursorItemInvalidate ( iCursorItem );
			UpdateScrollPos ();
		}
	}
}


bool Panel_c::IsClickAndHoldDetected ( int iX, int iY ) const
{
	SHRGINFO tGesture;
	tGesture.cbSize        = sizeof(SHRGINFO);
	tGesture.hwndClient    = m_hWnd;
	tGesture.ptDown.x      = iX;
	tGesture.ptDown.y      = iY;
	tGesture.dwFlags       = SHRG_RETURNCMD;

	return SHRecognizeGesture ( &tGesture ) == GN_CONTEXTMENU;
}


void Panel_c::ResetTemporaryStates ()
{
	m_iLastMarkFile = -1;
}


void Panel_c::Show ( bool bShow )
{
	m_bVisible = bShow;

	ShowWindow ( m_hWnd, bShow ? SW_SHOWNORMAL : SW_HIDE );
	if ( ! bShow )
		ResetTemporaryStates ();
}


bool Panel_c::IsVisible	() const
{
	return m_bVisible;
}


void Panel_c::Maximize ( bool bMaximize )
{
	m_bMaximized = bMaximize;

	if ( bMaximize )
	{
		if ( cfg->pane_maximized_view != VM_UNDEFINED )
		{
			m_eViewModeBeforeMax = m_eViewMode;
			SetViewMode ( ViewMode_e ( cfg->pane_maximized_view ) );
		}
	}
	else
		SetViewMode ( m_eViewModeBeforeMax );
	
	InvalidateButtons ();
}


bool Panel_c::IsMaximized () const
{
	return m_bMaximized;
}


bool Panel_c::IsLayoutVertical ()
{
	if ( cfg->pane_force_layout == -1 )
	{
		int iScrWidth, iScrHeight;
		GetScreenResolution ( iScrWidth, iScrHeight );
		return iScrWidth < iScrHeight;
	}
	else
		return cfg->pane_force_layout == 0;
}


void Panel_c::SetViewMode ( ViewMode_e eViewMode )
{
	m_eViewMode = eViewMode;
	if ( m_bMaximized && cfg->pane_maximized_view != VM_UNDEFINED )
		cfg->pane_maximized_view = eViewMode;

	m_bFilenameColMode = FixupColumns ( m_ViewModes.m_dViewModes [eViewMode] );
	m_bFirstDrawAfterRefresh = true;
}


ViewMode_e Panel_c::GetViewMode () const
{
	return m_eViewMode;
}


void Panel_c::OnLButtonDown ( int iX, int iY )
{
	FMActivatePanel ( this );

	RECT HeaderRect;
	GetHeaderViewRect ( HeaderRect );

	if ( IsInRect ( iX, iY, HeaderRect ) && IsClickAndHoldDetected ( iX, iY ) )
	{
		ShowHeaderMenu ( iX, iY );
		return;
	}

	RECT CenterRect;
	GetCenterRect ( CenterRect );
	if ( IsInRect ( iX, iY, CenterRect ) )
	{
		bool bPrevDir = false;
		int iFile = -1;
		FindOnScreen_e eFindRes = FindFile ( iFile, iX, iY );

		if ( eFindRes == FIND_FILE_FOUND )
		{
			SetCursorItemInvalidate ( iFile );
			if ( GetItemFlags ( iFile ) & FILE_PREV_DIR )
				bPrevDir = true;
		}

		if ( !bPrevDir && IsClickAndHoldDetected ( iX, iY ) )
		{
			if ( eFindRes == FIND_EMPTY_PANEL )
				ShowEmptyMenu ( iX, iY );
			else
				if ( eFindRes == FIND_FILE_FOUND )
					ShowFileMenu ( iFile, iX, iY );
		}
		else
		{
			if ( eFindRes == FIND_EMPTY_PANEL )
			{
				if ( GetNumMarked () > 0 )
					MarkAllFiles ( false, true );
			}
			else
				if ( eFindRes == FIND_FILE_FOUND )
				{
					if ( !bPrevDir && FMMarkMode () )
					{
						SetCapture ( m_hWnd );
						m_bMarkOnDrag = !(GetItemFlags ( m_iCursorItem ) & FILE_MARKED);
						MarkFileById ( m_iCursorItem, m_bMarkOnDrag );
						m_iLastMarkFile = eFindRes == FIND_FILE_FOUND ? m_iCursorItem : -1;
					}
					else
						if ( cfg->fastnav )
							DoFileExecute ( iFile, false );
				}
		}
	}
}


void Panel_c::OnLButtonDblClk ( int iX, int iY )
{
	RECT FooterRect;
	GetFooterRect ( FooterRect );

	if ( IsInRect ( iX, iY, FooterRect ) )
	{
		FMSetMarkMode ( !FMMarkMode () );
		return;
	}

	RECT HeaderRect;
	GetHeaderViewRect ( HeaderRect );
	if ( IsInRect ( iX, iY, HeaderRect ) )
	{
		if ( ( GetNumMarked () > 0 || FMMarkMode () ) && IsInRect ( iX, iY, m_MarkedRect ) )
			MarkAllFiles ( false, true );
		else
			ShowRecentMenu ( iX, iY );
		return;
	}

	if ( !cfg->fastnav )
	{
		int iFile = -1;
		FindOnScreen_e eFindRes = FindFile ( iFile, iX, iY );
		if ( eFindRes == FIND_FILE_FOUND )
			DoFileExecute ( iFile, false );
	}
}

void Panel_c::OnLButtonUp ( int iX, int iY )
{
	if ( GetCapture () == m_hWnd )
		ReleaseCapture ();

	m_iLastMarkFile = -1;
}


void Panel_c::OnMouseMove ( int iX, int iY )
{
	m_LastMouse.x = iX;
	m_LastMouse.y = iY;

	if ( GetCapture () != m_hWnd || !FMMarkMode () || m_iLastMarkFile == -1 )
		return;

	int iFile = -1;
	FindOnScreen_e eFindResult = FindFile ( iFile, iX, iY );
	if ( eFindResult == FIND_EMPTY_PANEL )
	{
		eFindResult = FIND_FILE_FOUND;
		iFile = GetNumItems () - 1;
	}

	if ( iFile == -1 || eFindResult != FIND_FILE_FOUND )
		return;

	MarkFilesTo ( iFile );
}


void Panel_c::DoFileExecute ( int iFile, bool bKeyboard )
{
	int iOldFile = m_iCursorItem;
	SetCursorItemInvalidate ( iFile );

	bool bSameFile = m_iCursorItem == iOldFile || bKeyboard;
	bool bOtherFile = m_iCursorItem != iOldFile || bKeyboard;
	bool bRunFile = false;
	if ( FMMarkMode () )
	{
		if ( GetItemFlags ( m_iCursorItem ) & FILE_PREV_DIR )
		{
			if ( bSameFile )
			{
				FMSetMarkMode ( false );
				bRunFile = true;
			}
		}
		else
			if ( bOtherFile )
				MarkFileById ( m_iCursorItem, !(GetItemFlags ( m_iCursorItem ) & FILE_MARKED) );
	}
	else
		bRunFile = true;

	if ( bRunFile && bSameFile )
	{
		const PanelItem_t & tInfo = GetItem ( iFile );
		if ( tInfo.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			if ( !wcscmp ( tInfo.m_FindData.cFileName, L".." ) )
				StepToPrevDir ();
			else
			{
				SetDirectory ( GetDirectory () + tInfo.m_FindData.cFileName );
				Refresh ();
			}
		}
		else
			FMExecuteFile ( GetDirectory () + tInfo.m_FindData.cFileName );
	}
}


void Panel_c::OnPaint ( HDC hDC )
{
	// setup the font
	SetBkMode ( hDC, TRANSPARENT );
	SetTextColor ( hDC, clrs->pane_font );

	HGDIOBJ hOldFont = SelectObject ( hDC, g_tPanelFont.GetHandle () );
	HGDIOBJ hOldPen = SelectObject ( hDC, g_tResources.m_hBorderPen );

	DrawHeader ( hDC );
	DrawCenter ( hDC );
	DrawFooter ( hDC );

	// restore
	SelectObject ( hDC, hOldFont );
	SelectObject ( hDC, hOldPen );
}


bool Panel_c::OnCommand ( WPARAM wParam, LPARAM lParam )
{
	bool bClicked = HIWORD ( wParam ) == BN_CLICKED;

	if ( (HWND) lParam == m_BtnMax.m_hButton && bClicked )
	{
		if ( m_bMaximized )
			FMNormalPanels ();
		else
			FMMaximizePanel ( this );

		return true;
	}

	if ( (HWND) lParam == m_BtnSwitch.m_hButton && bClicked )
	{
		FMMaximizePanel ( FMGetPassivePanel () );
		return true;
	}

	if ( (HWND) lParam == m_BtnBookmarks.m_hButton && bClicked )
	{
		ShowBookmarks ();
		return true;
	}

	if ( (HWND) lParam == m_BtnUp.m_hButton && bClicked )
	{
		StepToPrevDir ();
		return true;
	}

	return false;
}


bool Panel_c::OnMeasureItem ( int iId, MEASUREITEMSTRUCT * pMeasureItem )
{
	if ( pMeasureItem->CtlType == ODT_MENU && IsDriveMenuItem ( pMeasureItem->itemID ) )
	{
		// that's the drive menu
		SIZE Size;
		if ( GetDriveMenuItemSize ( pMeasureItem->itemData, Size ) )
		{
			pMeasureItem->itemWidth	 = Size.cx;
			pMeasureItem->itemHeight = Size.cy;
			return true;
		}

		return false;
	}

	return false;
}


bool Panel_c::OnDrawItem ( int iId, const DRAWITEMSTRUCT * pDrawItem )
{
	Assert ( pDrawItem );

    if ( pDrawItem->CtlType == ODT_BUTTON )
	{		
		// min/max button
		if ( pDrawItem->hwndItem == m_BtnMax.m_hButton )
		{
			if ( m_bMaximized )
				m_BtnMax.Draw ( pDrawItem, g_tResources.m_hBmpBtnMid, g_tResources.m_hBmpBtnMidPressed, m_bActive );
			else
				m_BtnMax.Draw ( pDrawItem, g_tResources.m_hBmpBtnMax, g_tResources.m_hBmpBtnMaxPressed, m_bActive );

			return true;
		}

		// switch panes button
		if ( pDrawItem->hwndItem == m_BtnSwitch.m_hButton && m_bMaximized )
		{
			if ( FMGetPanel1 () == this )
				m_BtnSwitch.Draw ( pDrawItem, g_tResources.m_hBmpChangeTo2, g_tResources.m_hBmpChangeTo2Pressed, m_bActive );
			else
				m_BtnSwitch.Draw ( pDrawItem, g_tResources.m_hBmpChangeTo1, g_tResources.m_hBmpChangeTo1Pressed, m_bActive );

			return true;
		}

		// bookmarks button
		if ( pDrawItem->hwndItem == m_BtnBookmarks.m_hButton )
		{
			m_BtnBookmarks.Draw ( pDrawItem, g_tResources.m_hBmpBtnDir, g_tResources.m_hBmpBtnDirPressed, m_bActive );
			return true;
		}

		// the up button
		if ( pDrawItem->hwndItem == m_BtnUp.m_hButton )
		{
			m_BtnUp.Draw ( pDrawItem, g_tResources.m_hBmpBtnUp, g_tResources.m_hBmpBtnUpPressed, m_bActive );
			return true;
		}
	}

	if ( pDrawItem->CtlType == ODT_MENU && IsDriveMenuItem ( pDrawItem->itemID ) )
		return DriveMenuDrawItem ( pDrawItem );

	return false;
}


void Panel_c::OnVScroll ( int iPos, int iFlags, HWND hSlider )
{
	int nViewFiles	= GetMaxItemsInView ();

	switch ( iFlags )
	{
	case SB_LINEUP:
		ScrollItemsTo ( m_iFirstItem - 1 );
		break;
	case SB_LINEDOWN:
		ScrollItemsTo ( m_iFirstItem + 1 );
		break;
	case SB_PAGEUP:
		ScrollItemsTo ( m_iFirstItem - nViewFiles );
		break;
	case SB_PAGEDOWN:
		ScrollItemsTo ( m_iFirstItem + nViewFiles );
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		ScrollItemsTo ( iPos );
		break;
	case SB_TOP:
	case SB_BOTTOM:
	case SB_ENDSCROLL:
		{
			SCROLLINFO tScrollInfo;
			tScrollInfo.cbSize	= sizeof ( SCROLLINFO );
			tScrollInfo.fMask	= SIF_POS;
			tScrollInfo.nPos	= m_iFirstItem;
			SetScrollInfo ( m_hSlider, SB_CTL, &tScrollInfo, TRUE );
		}
		break;
	}
}


void Panel_c::OnTimer ()
{
	if ( GetCapture () == m_hWnd && m_iLastMarkFile != -1 )
	{
		RECT HeaderRect;
		GetHeaderRect ( HeaderRect );

		if ( IsInRect ( m_LastMouse.x, m_LastMouse.y, HeaderRect ) )
		{
			ScrollItemsTo ( m_iFirstItem - 1 );
			MarkFilesTo ( m_iFirstItem );
			UpdateScrollPos ();
			return;
		}

		RECT FooterRect;
		GetFooterRect ( FooterRect );

		if ( IsInRect ( m_LastMouse.x, m_LastMouse.y, FooterRect ) )
		{
			ScrollItemsTo ( m_iFirstItem + 1 );
			MarkFilesTo ( ClampCursorItem ( m_iFirstItem + GetMaxItemsInView () - 1 ) );
			UpdateScrollPos ();
			return;
		}
	}

	m_fTimeSinceRefresh += FM_TIMER_PERIOD_MS;

	bool bEnoughFiles = m_bRefreshAnyway || ( GetNumItems () <= cfg->refresh_max_files || !cfg->refresh_max_files );
	if ( m_bRefreshQueued && bEnoughFiles && ( m_bRefreshAnyway || cfg->refresh ) && ( m_fTimeSinceRefresh / 1000.0f >= cfg->refresh_period ) )
		SoftRefresh ();
}


bool Panel_c::OnKeyEvent ( int iBtn, btns::Event_e eEvent )
{
	BtnAction_e eAñtion = buttons->GetAction ( iBtn, eEvent );

	switch ( eAñtion )
	{
	case BA_MOVEUP:
		ScrollCursorTo ( m_iCursorItem - 1 );
		return true;
	case BA_MOVEDOWN:
		ScrollCursorTo ( m_iCursorItem + 1 );
		return true;
	case BA_MOVELEFT:
		{
			if ( m_iCursorItem == 0 )
				StepToPrevDir ();
			else
			{
				int nColumnItems = GetMaxItemsInColumn ();
				if ( m_iCursorItem - nColumnItems < m_iFirstItem )
				{
					ScrollItemsTo ( m_iFirstItem - nColumnItems );
					UpdateScrollPos ();
				}

				SetCursorItemInvalidate ( m_iCursorItem - nColumnItems );
			}
		}
		return true;
	case BA_MOVERIGHT:
		{
			int nColumnItems = GetMaxItemsInColumn ();
			int nViewItems =  GetMaxItemsInView ();
			if ( m_iCursorItem + nColumnItems >= m_iFirstItem + nViewItems )
			{
				ScrollItemsTo ( m_iFirstItem + nColumnItems );
				UpdateScrollPos ();
			}

			SetCursorItemInvalidate ( m_iCursorItem + nColumnItems );
		}
		return true;
	case BA_MOVEHOME:
		ScrollItemsTo ( 0 );
		UpdateScrollPos ();
		SetCursorItemInvalidate ( 0 );
		return true;
	case BA_MOVEEND:
		{
			int nItems = GetNumItems ();
			ScrollItemsTo ( nItems );
			UpdateScrollPos ();
			SetCursorItemInvalidate ( nItems );
		}
		return true;
	case BA_OPEN_IN_OPPOSITE:
		FMOpenInOpposite ();
		return true;
	case BA_SAME_AS_OPPOSITE:
		SetDirectory ( FMGetPassivePanel ()->GetDirectory () );
		Refresh ();
		return true;
	case BA_HEADER_MENU:
		{
			int iX = 0, iY = 0;
			m_BtnBookmarks.GetPosition ( iX, iY );
			ShowHeaderMenu ( iX + g_tResources.m_iTopIconSize, iY );
		}
		return true;
	case BA_DRIVE_LIST:
		ShowBookmarks ();
		return true;
	case BA_EXECUTE:
		DoFileExecute ( m_iCursorItem, true );
		return true;
	case BA_PREVDIR:
		StepToPrevDir ();	
		return true;
	case BA_FL_TOGGLE:
		if ( MarkFileById ( m_iCursorItem, !( GetItemFlags ( m_iCursorItem ) & FILE_MARKED ) ) )
			InvalidateHeader ();
		return true;
	case BA_FL_TOGGLE_AND_MOVE:
		if ( MarkFileById ( m_iCursorItem, !( GetItemFlags ( m_iCursorItem ) & FILE_MARKED ) ) )
			InvalidateHeader ();
		ScrollCursorTo ( m_iCursorItem + 1 );
		return true;
	case BA_FILE_MENU:
		{
			RECT Rect;
			if ( GetItemTextRect ( m_iCursorItem, Rect ) )
				ShowFileMenu ( m_iCursorItem, ( Rect.left + Rect.right ) / 2, ( Rect.top + Rect.bottom ) / 2 );
		}
		return true;
	case BA_BM_ADD:
		FMAddBookmark ();
		return true;
	}

	return false;
}


void Panel_c::OnFileChange ()
{
	QueueRefresh ();
}


HDC	Panel_c::BeginPaint ( HDC hDC )
{
	if ( cfg->double_buffer )
	{
		RECT WinRect;
		GetWindowRect ( m_hWnd, &WinRect );

		bool bCreate = true;
		if ( m_hMemDC )
			bCreate = ( m_BmpSize.cx != WinRect.right - WinRect.left ) || ( m_BmpSize.cy != WinRect.bottom - WinRect.top );

		if ( bCreate )
		{
			if ( m_hMemBitmap )
				DeleteObject ( m_hMemBitmap );

			if ( m_hMemDC )
				DeleteDC ( m_hMemDC );

			m_hMemDC	 = CreateCompatibleDC ( hDC );
			m_hMemBitmap = CreateCompatibleBitmap ( hDC, WinRect.right - WinRect.left, WinRect.bottom - WinRect.top );
			SelectObject ( m_hMemDC, m_hMemBitmap );
			m_BmpSize.cx = WinRect.right - WinRect.left;
			m_BmpSize.cy = WinRect.bottom - WinRect.top;
		}

		return m_hMemDC;
	}
	else
		return hDC;
}


void Panel_c::EndPaint ( HDC hDC, const PAINTSTRUCT & PS )
{
	if ( cfg->double_buffer )
		BitBlt ( hDC, PS.rcPaint.left, PS.rcPaint.top, PS.rcPaint.right - PS.rcPaint.left, PS.rcPaint.bottom - PS.rcPaint.top, m_hMemDC, PS.rcPaint.left, PS.rcPaint.top, SRCCOPY );
}


void Panel_c::CreateButton ( Button_t & Button )
{
	Button.m_hButton = CreateWindow ( L"BUTTON", L"", WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, NULL, g_hAppInstance, NULL );
	Assert ( Button.m_hButton );

	if ( ! m_pOldBtnProc )
		m_pOldBtnProc = (WNDPROC) GetWindowLong  ( Button.m_hButton, GWL_WNDPROC );

	SetWindowLong ( Button.m_hButton, GWL_WNDPROC, (LONG) BtnProc );
}


void Panel_c::CreateDrawObjects ()
{
	CreateButton ( m_BtnMax );
	CreateButton ( m_BtnBookmarks );
	CreateButton ( m_BtnSwitch );
	CreateButton ( m_BtnUp );
}


void Panel_c::DestroyDrawObjects ()
{
	DestroyWindow ( m_BtnMax.m_hButton );
	DestroyWindow ( m_BtnBookmarks.m_hButton );
	DestroyWindow ( m_BtnSwitch.m_hButton );
	DestroyWindow ( m_BtnUp.m_hButton );

	m_BtnMax.m_hButton =  m_BtnBookmarks.m_hButton = m_BtnSwitch.m_hButton = m_BtnUp.m_hButton = NULL;
}


void Panel_c::DrawCursor ( HDC hDC )
{
	RECT Rect;
	if ( GetCursorRect ( Rect ) )
		FillRect ( hDC, &Rect, g_tResources.GetCursorBrush ( IsActive () ) );
}


void Panel_c::DrawHeader ( HDC hDC )
{
	RECT tHeader;
	GetHeaderRect ( tHeader );

	// header background
	FillRect ( hDC, &tHeader, m_bActive ? g_tResources.m_hActiveHdrBrush : g_tResources.m_hPassiveHdrBrush );

	// title line
	MoveToEx ( hDC, m_Rect.left, tHeader.bottom, NULL );
	LineTo ( hDC, m_Rect.right, tHeader.bottom );

	// draw current folder
	GetHeaderViewRect ( tHeader );
	CollapseRect ( tHeader, 1 );

	int nMarked = GetNumMarked ();

	// if there's anything marked, draw it!
	if ( FMMarkMode () || nMarked > 0 )
	{
		m_MarkedRect = tHeader;

		wchar_t szText [FILESIZE_BUFFER_SIZE * 2];
		FileSizeToStringUL ( GetMarkedSize (), szText, false );
		wcscat ( szText, L"|" );
		wsprintf ( &(szText [wcslen ( szText)]), L"%d", nMarked );

		int iLen = wcslen ( szText );

		RECT tCalcMarked;
		memset ( &tCalcMarked, 0, sizeof ( tCalcMarked ) );
		DrawText ( hDC, szText, iLen, &tCalcMarked, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX );
		int iMarkTextWidth = tCalcMarked.right - tCalcMarked.left;

		SetTextColor ( hDC, clrs->pane_font_marked );

		if ( tHeader.right - tHeader.left > iMarkTextWidth )
		{
			m_MarkedRect.left = m_MarkedRect.right - ( iMarkTextWidth + 1 );
			if ( m_MarkedRect.left < tHeader.left )
				m_MarkedRect.left = tHeader.left;

			DrawText ( hDC, szText, iLen, &m_MarkedRect, DT_LEFT | DT_VCENTER | DT_NOCLIP | DT_SINGLELINE | DT_NOPREFIX );
		}
		else
		{
			// draw clipped size and a '|'
			m_MarkedRect.right -= 2;

			HGDIOBJ hOldPen = SelectObject ( hDC, g_tResources.m_hOverflowPen );
			DrawText ( hDC, szText, iLen, &m_MarkedRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
			MoveToEx ( hDC, m_MarkedRect.right + 1, m_MarkedRect.top, NULL );
			LineTo ( hDC, m_MarkedRect.right + 1, m_MarkedRect.bottom );
			SelectObject ( hDC, hOldPen );

			// for later usage
			m_MarkedRect.right += 2;
		}

		if ( FMMarkMode () )
		{
			m_MarkedRect.left -= g_tResources.m_iTopIconSize;

			SelectObject ( g_tResources.m_hBitmapDC, g_tResources.m_hBmpSelect );
			TransparentBlt ( hDC, m_MarkedRect.left, tHeader.top - 1, g_tResources.m_iTopIconSize, g_tResources.m_iTopIconSize,
				g_tResources.m_hBitmapDC, 0, 0, g_tResources.m_iTopIconSize, g_tResources.m_iTopIconSize, BMP_TRANS_CLR );

			tHeader.right = m_MarkedRect.left;
		}
		else
			tHeader.right = m_MarkedRect.left - g_tPanelFont.GetWidth ( L' ' );
	}

	SetTextColor ( hDC, clrs->pane_font );

	if ( tHeader.right > tHeader.left )
	{
		const Str_c & sDir = GetDirectory ();

		RECT tCalcHeader;
		memset ( &tCalcHeader, 0, sizeof ( tCalcHeader ) );
		DrawText ( hDC, sDir, sDir.Length (), &tCalcHeader, DT_SINGLELINE | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX );

		if ( tCalcHeader.right - tCalcHeader.left > tHeader.right - tHeader.left )
		{
			HGDIOBJ hOldPen = SelectObject ( hDC, g_tResources.m_hOverflowPen );
			MoveToEx ( hDC, tHeader.left, tHeader.top, NULL );
			LineTo ( hDC, tHeader.left, tHeader.bottom );
			SelectObject ( hDC, hOldPen );
			tHeader.left += 2;
			DrawText ( hDC, sDir, sDir.Length (), &tHeader, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX ); //align right
		}
		else
			DrawText ( hDC, sDir, sDir.Length (), &tHeader, DT_LEFT | DT_VCENTER | DT_NOCLIP | DT_SINGLELINE | DT_NOPREFIX ); //align left
	}
}


void Panel_c::DrawCenter ( HDC hDC )
{
	if ( m_bFirstDrawAfterRefresh )
	{
		RecalcColumnWidths ( hDC );
		m_bFirstDrawAfterRefresh = false;
	}

	const int MIN_COLUMN_PX = 4;

	RECT CenterRect;
	GetCenterRect ( CenterRect );

	int nColumns = m_ViewModes.m_dViewModes [m_eViewMode].m_nColumns;

	if ( nColumns == 0 )
	{
		FillRect ( hDC, &CenterRect, g_tResources.m_hBackgroundBrush );
		return;
	}

	int nColumnItems = ( CenterRect.bottom - CenterRect.top ) / GetRowHeight ();

	// calculate precise column widths
	if ( m_bFilenameColMode )
	{
		int iColumnWidth = ( CenterRect.right - CenterRect.left ) / nColumns;

		for ( int i = 0; i < nColumns; ++i )
		{
			m_dColumnSizes [i].m_iLeft	= i * iColumnWidth;
			m_dColumnSizes [i].m_iRight	= ( i == nColumns - 1 ) ? CenterRect.right : ( i + 1 ) * iColumnWidth;
		}
	}
	else
	{
		int iTotalColWidth = 0;
		for ( int i = 0; i < nColumns; ++i )
		{
			const PanelColumn_t & Column = m_ViewModes.GetColumn ( m_eViewMode, i );

			int iColWidth = 0;
			if ( Column.m_eType != COL_FILENAME )
			{
				iColWidth = m_dColumnSizes [i].m_iMaxTextWidth + ( ( i != nColumns ) ? 3 : 2 );
				if ( Column.m_fMaxWidth > 0.0f && iColWidth > ( CenterRect.right - CenterRect.left ) * Column.m_fMaxWidth )
					iColWidth = int ( ( CenterRect.right - CenterRect.left ) * Column.m_fMaxWidth );
			}

			m_dColumnSizes [i].m_iWidth = iColWidth;
			iTotalColWidth += iColWidth;
		}

		int iPxLeft = CenterRect.right - CenterRect.left - iTotalColWidth;
		if ( iPxLeft < 0 )
			iPxLeft = 0;

		int iLeft = CenterRect.left;
		for ( int i = 0; i < nColumns; ++i )
		{
			int iWidth = m_dColumnSizes [i].m_iWidth;

			if ( m_ViewModes.GetColumn ( m_eViewMode, i ).m_eType == COL_FILENAME )
				iWidth = iPxLeft;

			m_dColumnSizes [i].m_iLeft	= iLeft;
			m_dColumnSizes [i].m_iRight = iLeft + iWidth;

			if ( m_dColumnSizes [i].m_iLeft < CenterRect.right )
			{
				if ( m_dColumnSizes [i].m_iRight > CenterRect.right )
					m_dColumnSizes [i].m_iRight = CenterRect.right;
			}

			iLeft += iWidth;
		}
	}

	// fill column backgrounds
	RECT Rect = CenterRect;
	for ( int i = 0; i < nColumns; ++i )
	{
		Rect.left	= m_dColumnSizes [i].m_iLeft;
		Rect.right	= m_dColumnSizes [i].m_iRight;

		if ( i != nColumns - 1 )
			Rect.right--;

		FillRect ( hDC, &Rect, g_tResources.m_hBackgroundBrush );
	}

	DrawCursor ( hDC );

	// draw lines and column contents
	int iStartItem = m_iFirstItem;
	for ( int i = 0; i < nColumns; ++i )
	{
		Rect.left	= m_dColumnSizes [i].m_iLeft;
		Rect.right	= m_dColumnSizes [i].m_iRight;

		if ( i != nColumns - 1 )
		{
			Rect.right--;
			MoveToEx ( hDC, Rect.right, Rect.top, NULL );
			LineTo	 ( hDC, Rect.right, Rect.bottom );
		}

		if ( m_bFilenameColMode )
			DrawFileNames ( hDC, iStartItem, nColumnItems, Rect );
		else
			if ( Rect.right - Rect.left > MIN_COLUMN_PX )
				DrawColumn ( i, hDC, m_iFirstItem, nColumnItems, Rect );

		iStartItem += nColumnItems;
	}
}


void Panel_c::DrawTextInRect ( HDC hDC, RECT & tRect, const wchar_t * szText, DWORD uColor )
{
	if ( tRect.left >= tRect.right )
		return;

	int iTextLen = wcslen ( szText );

	RECT tCalcRect;
	memset ( &tCalcRect, 0, sizeof ( tCalcRect ) );
	DrawText ( hDC, szText, iTextLen, &tCalcRect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX );

	SetTextColor ( hDC, uColor );
	DrawText ( hDC, szText, iTextLen, &tRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
	tRect.right -= tCalcRect.right - tCalcRect.left + g_tPanelFont.GetWidth ( L' ' );
}


void Panel_c::DrawFooter ( HDC hDC )
{
	RECT FooterRect;
	GetFooterRect ( FooterRect );

	// footer background
	FillRect ( hDC, &FooterRect, g_tResources.m_hBackgroundBrush );

	// status bar line
	MoveToEx ( hDC, FooterRect.left, FooterRect.top - 1, NULL );
	LineTo ( hDC, FooterRect.right, FooterRect.top - 1 );

	if ( !GetNumItems () )
		return;

	const PanelItem_t & Item = GetItem ( m_iCursorItem );
	
	CollapseRect ( FooterRect, 1 );

	RECT StringRect = FooterRect;

	// correct the rect in case of an icon
	if ( cfg->pane_fileinfo_icon )
		StringRect.left += GetSystemMetrics ( SM_CXSMICON ) + 1;

	const PluginInfo_t * pInfo = m_hActivePlugin ? g_PluginManager.GetPluginInfo ( m_hActivePlugin ) : NULL;
	if ( pInfo && pInfo->m_nFileInfoCols != -1 )
	{
		wchar_t szTmpBuf [TIMEDATE_BUFFER_SIZE];

		for ( int i = 0; i < pInfo->m_nFileInfoCols; ++i )
		{
			fnFormat FormatFunc = GetFormatFunc ( pInfo->m_dFileInfoCols [i] );
			DrawTextInRect ( hDC, StringRect, FormatFunc ( szTmpBuf, Item ), clrs->pane_fileinfo_size );
		}
	}
	else
	{
		wchar_t szTime [TIMEDATE_BUFFER_SIZE];
		wchar_t szDate [TIMEDATE_BUFFER_SIZE];
		wchar_t szSize [FILESIZE_BUFFER_SIZE];
		szTime [0] = szDate [0] = szSize [0] = L'\0';

		if ( cfg->pane_fileinfo_time )
		{
			FormatTimeWrite ( szTime, Item );
			DrawTextInRect ( hDC, StringRect, szTime, clrs->pane_fileinfo_time );
		}

		if ( cfg->pane_fileinfo_date )
		{
			FormatDateWrite ( szDate, Item );
			DrawTextInRect ( hDC, StringRect, szDate, clrs->pane_fileinfo_date );
		}

		FormatSize ( szSize, Item );
		DrawTextInRect ( hDC, StringRect, szSize, clrs->pane_fileinfo_size );
	}

	int iSpaceLeft = StringRect.right - StringRect.left;

	// do we have any space left? :)
	if ( iSpaceLeft > 0 )
	{
		SetTextColor ( hDC, clrs->pane_font );

		int iFileNameLen = wcslen ( Item.m_FindData.cFileName );
		RECT Rect;
		memset ( &Rect, 0, sizeof ( Rect ) );
		DrawText ( hDC, Item.m_FindData.cFileName, iFileNameLen, &Rect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX );

		if ( Rect.right - Rect.left <= iSpaceLeft )
			DrawText ( hDC, Item.m_FindData.cFileName, iFileNameLen, &StringRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX );
		else
		{
			StringRect.right = StringRect.left + iSpaceLeft;

			HGDIOBJ hOldPen = SelectObject ( hDC, g_tResources.m_hOverflowPen );
			MoveToEx ( hDC, StringRect.left, StringRect.top, NULL );
			LineTo ( hDC, StringRect.left, StringRect.bottom );
			SelectObject ( hDC, hOldPen );

			StringRect.left++;
			DrawText ( hDC, Item.m_FindData.cFileName, iFileNameLen, &StringRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
		}
	}

	if ( cfg->pane_fileinfo_icon )
		ImageList_Draw ( m_hImageList, GetItemIcon ( m_iCursorItem ), hDC, FooterRect.left, FooterRect.top, ILD_TRANSPARENT );
}


int	Panel_c::GetHeaderHeight () const
{
	return Max ( g_tPanelFont.GetHeight () + 2, g_tResources.m_iTopIconSize );
}


void Panel_c::GetHeaderRect ( RECT & Rect ) const
{
	Rect = m_Rect;
	Rect.bottom = m_Rect.top + GetHeaderHeight ();
}


void Panel_c::GetHeaderViewRect ( RECT & Rect ) const
{
	Rect.left	 = m_Rect.left + g_tResources.m_iTopIconSize;
	Rect.right	 = m_Rect.right - g_tResources.m_iTopIconSize * ( m_bMaximized ? 3 : 2 );
	Rect.top	 = m_Rect.top;
	Rect.bottom	 = m_Rect.top + GetHeaderHeight ();
}


int Panel_c::GetFooterHeight () const
{
	return Max ( cfg->pane_fileinfo_icon ?  GetSystemMetrics ( SM_CYSMICON ) : 0, g_tPanelFont.GetHeight () ) + 2;
}


void Panel_c::GetFooterRect ( RECT & Rect ) const
{
	Rect		= m_Rect;
	Rect.top	= Rect.bottom - GetFooterHeight ();
}


void Panel_c::GetCenterRect ( RECT & Rect ) const
{
	Rect = m_Rect;
	Rect.top += GetHeaderHeight () + 1;
	Rect.bottom -= GetFooterHeight () + 1;

	if ( m_bSliderVisible )
		Rect.right -= cfg->pane_slider_size_x;
}


bool Panel_c::GetItemTextRect ( int nItem, RECT & Rect ) const
{
	int nColumns = m_ViewModes.m_dViewModes [m_eViewMode].m_nColumns;

	if ( nItem < 0 || nItem >= GetNumItems () || nColumns == 0 )
		return false;

	int iOffset = nItem - m_iFirstItem;
	if ( iOffset < 0 )
		return false;

	int nViewItems = GetMaxItemsInView ();

	if ( iOffset >= nViewItems )
		return false;

	int iSymbolHeight = GetItemHeight ();

	RECT CenterRect;
	GetCenterRect ( CenterRect );

	int nRow = iOffset;

	if ( m_bFilenameColMode )
	{
		int nColumnFiles = nViewItems / nColumns;
		int iColumn = iOffset / nColumnFiles;
		nRow %= nColumnFiles;
		Rect.left	= m_dColumnSizes [iColumn].m_iLeft + 1;
		Rect.right	= m_dColumnSizes [iColumn].m_iRight - 2;
	}
	else
	{
		Rect.left	= m_dColumnSizes [0].m_iLeft + 1;
		Rect.right	= m_dColumnSizes [nColumns - 1].m_iRight - 2;
	}

	Rect.top	= CenterRect.top + nRow * ( iSymbolHeight + SPACE_BETWEEN_LINES ) + SPACE_BETWEEN_LINES;
	Rect.bottom	= Rect.top + iSymbolHeight;

	return true;
}


bool Panel_c::GetCursorRect ( RECT & Rect ) const
{
	if ( ! GetItemTextRect ( m_iCursorItem, Rect ) )
		return false;

	CollapseRect ( Rect, -1 );
	++Rect.right;

	return true;
}


int Panel_c::GetMaxItemsInColumn () const
{
	if ( !m_ViewModes.m_dViewModes [m_eViewMode].m_nColumns )
		return 0;

	RECT Rect;
	GetCenterRect ( Rect );

	return ( Rect.bottom - Rect.top ) / GetRowHeight ();
}


int	Panel_c::GetMaxItemsInView () const
{
	int nColumns = m_ViewModes.m_dViewModes [m_eViewMode].m_nColumns;
	return Max ( 0, GetMaxItemsInColumn () * ( m_bFilenameColMode ? nColumns : 1 ) );
}


int Panel_c::GetItemHeight () const
{
	bool bHaveFilenameCol = false;
	int nColumns = m_ViewModes.m_dViewModes [m_eViewMode].m_nColumns;
	bool bShowIcons = m_ViewModes.m_dViewModes [m_eViewMode].m_bShowIcons;

	for ( int i = 0; i < nColumns && !bHaveFilenameCol; ++i  )
		bHaveFilenameCol = m_ViewModes.GetColumn ( m_eViewMode, i ).m_eType == COL_FILENAME;

	return Max ( g_tPanelFont.GetHeight (), bHaveFilenameCol && bShowIcons ? GetSystemMetrics ( SM_CYSMICON ) : 0 );
}


int	Panel_c::GetRowHeight () const
{
	return GetItemHeight () + SPACE_BETWEEN_LINES;
}


void Panel_c::Event_PluginOpened ( HANDLE hPlugin )
{
	FileList_c::Event_PluginOpened ( hPlugin );

	m_hCachedImageList = m_hImageList;

	const PluginInfo_t * pInfo = g_PluginManager.GetPluginInfo ( hPlugin );
	if ( !pInfo )
		return;

	if ( pInfo->m_hImageList )
		m_hImageList = pInfo->m_hImageList;

	m_eCachedViewMode = m_eViewMode;

	if ( pInfo->m_eStartViewMode != VM_UNDEFINED )
		m_eViewMode = pInfo->m_eStartViewMode;
		
	for ( int i = 0; i < VM_TOTAL; i++ )
		if ( pInfo->m_dPanelViews [i] )
		{
			for ( int j = 0; j < MAX_COLUMNS; j++ )
				m_ViewModes.m_dViewModes [i].m_dColumns [j] = pInfo->m_dPanelViews [i]->m_dColumns [j];

			m_ViewModes.m_dViewModes [i].m_nColumns = pInfo->m_dPanelViews [i]->m_nColumns;
		}
}


void Panel_c::Event_PluginClosed ()
{
	FileList_c::Event_PluginClosed ();

	m_hImageList = m_hCachedImageList;
	m_ViewModes.Reset ();
	m_eViewMode = m_eCachedViewMode;
}


void Panel_c::InvalidateHeader ()
{
	RECT tRect;
	GetHeaderRect ( tRect );
	InvalidateRect ( m_hWnd, &tRect, FALSE );
	InvalidateButtons ();
}


void Panel_c::InvalidateFooter ()
{
	RECT Rect;
	GetFooterRect ( Rect );
	InvalidateRect ( m_hWnd, &Rect, FALSE );
}


void Panel_c::InvalidateCenter ()
{
	RECT Rect;
	GetCenterRect ( Rect );
	InvalidateRect ( m_hWnd, &Rect, FALSE );
}


void Panel_c::InvalidateCursor ()
{
	RECT Rect;
	if ( GetCursorRect ( Rect ) )
		InvalidateRect ( m_hWnd, &Rect, FALSE );
}


void Panel_c::InvalidateAll ()
{
	InvalidateRect ( m_hWnd, NULL, FALSE );
	InvalidateButtons ();
}


void Panel_c::RedrawWindow ()
{
	::RedrawWindow ( m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE );
}


void Panel_c::QueueRefresh ( bool bAnyway )
{
	m_bRefreshAnyway = bAnyway;
	m_bRefreshQueued = true;
}


void Panel_c::Refresh ()
{
	FileList_c::Refresh ();

	SetCursorItem ( ClampCursorItem ( m_iCursorItem ) );
	SetFirstItem ( ClampFirstItem ( m_iFirstItem ) );

	AfterRefresh ();
}


void Panel_c::SoftRefresh ()
{
	Str_c sFileAtCursor;

	if ( GetNumItems () != 0 )
		sFileAtCursor = GetItem ( m_iCursorItem ).m_FindData.cFileName;

	FileList_c::SoftRefresh ();

	if ( !sFileAtCursor.Empty () )
		SetCursorAtItem ( sFileAtCursor );

	AfterRefresh ();
}


void Panel_c::AfterRefresh ()
{
	InvalidateHeader ();
	InvalidateCenter ();
	InvalidateFooter ();

	UpdateScrollInfo ();

	m_bRefreshQueued = false;
	m_bRefreshAnyway = false;
	m_fTimeSinceRefresh = 0.0f;

	m_bFirstDrawAfterRefresh = true;
}


void Panel_c::Resort ()
{
	FileList_c::SortItems ();

	SetCursorItem ( ClampCursorItem ( m_iCursorItem ) );
	SetFirstItem ( ClampFirstItem ( m_iFirstItem ) );

	InvalidateHeader ();
	InvalidateCenter ();
	InvalidateFooter ();

	UpdateScrollInfo ();
}


void Panel_c::SetDirectory ( const wchar_t * szDir )
{
	FileList_c::SetDirectory ( szDir );

	SetCursorItem ( 0 );
	SetFirstItem ( 0 );

	FMSetMarkMode ( false );
}


void Panel_c::StepToPrevDir ()
{
	Str_c sPrevDir;
	if ( FileList_c::StepToPrevDir ( &sPrevDir ) )
	{
		Refresh ();
		SetCursorAtItem ( sPrevDir );
		FMSetMarkMode ( false );
	}
	else
		ScrollCursorTo ( 0 );
}


bool Panel_c::MarkFileById ( int iFile, bool bMark )
{
	if ( FileList_c::MarkFileById ( iFile, bMark ) )
	{
		RECT Rect;
		if ( GetItemTextRect ( iFile, Rect ) )
			InvalidateRect ( m_hWnd, &Rect, FALSE );
		InvalidateHeader ();

		return true;
	}

	return false;
}


void Panel_c::MarkAllFiles ( bool bMark, bool bForceDirs )
{
	for ( int i = 0; i < GetNumItems (); ++i )
	{
		const PanelItem_t & Item = GetItem ( i );
		bool bDoFile = true;
		if ( !bForceDirs )
			bDoFile = !( Item.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) || cfg->filter_include_dirs;

		if ( bDoFile )
			MarkFileById ( i, bMark );
	}

	InvalidateHeader ();
}


void Panel_c::MarkFilter ( const wchar_t * szFilter, bool bMark )
{
	 Filter_c Filter ( false );
	 Filter.Set ( szFilter );

	 for ( int i = 0; i < GetNumItems (); ++i )
	 {
		 const PanelItem_t & Item = GetItem ( i );

		 if ( !cfg->filter_include_dirs && (Item.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			 continue;

		 if ( Filter.Fits ( Item.m_FindData.cFileName ) )
			 MarkFileById ( i, bMark );
	 }

	 InvalidateHeader ();
}


void Panel_c::InvertMarkedFiles ()
{
	for ( int i = 0; i < GetNumItems (); ++i )
	{
		const PanelItem_t & Item = GetItem ( i );

		if ( ( Item.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && !cfg->filter_include_dirs )
			MarkFileById ( i, false );
		else
			MarkFileById ( i, !(GetItemFlags ( i ) & FILE_MARKED) );
	}

	InvalidateHeader ();
}


void Panel_c::MarkFilesTo ( int iFile )
{
	if ( m_iCursorItem != iFile )
		SetCursorItemInvalidate ( iFile );

	if ( m_iLastMarkFile == iFile )
		return;

	bool bMarkedSmth = false;
	RECT Rect;
	for ( int i = Min ( m_iLastMarkFile, iFile ); i <= Max ( m_iLastMarkFile, iFile ); ++i )
		if ( FileList_c::MarkFileById ( i, m_bMarkOnDrag ) )
		{
			bMarkedSmth = true;
			GetItemTextRect ( i, Rect );
			InvalidateRect ( m_hWnd, &Rect, FALSE );
		}

	if ( bMarkedSmth )
		InvalidateHeader ();

	m_iLastMarkFile = iFile;
}


void Panel_c::RecalcColumnWidths ( HDC hDC )
{
	if ( m_bFilenameColMode )
		return;

	int nItems = GetNumItems ();
	bool bExactCalc = nItems <= 32;

	wchar_t szTmp [MAX_PATH];
	int nColumns = m_ViewModes.m_dViewModes [m_eViewMode].m_nColumns;

	bool bHaveTimeDate = false;
	bool bHaveSize = false;
	for ( int i = 0; i < nColumns; ++i )
	{
		const PanelColumn_t & Column = m_ViewModes.GetColumn ( m_eViewMode, i );
		ColumnType_e eType = Column.m_eType;
		if ( eType == COL_TIME_CREATION || eType == COL_TIME_ACCESS || eType == COL_TIME_WRITE || eType == COL_DATE_CREATION || eType == COL_DATE_ACCESS || eType == COL_DATE_WRITE )
			bHaveTimeDate = true;

		if ( eType == COL_SIZE || eType == COL_SIZE_PACKED )
			bHaveSize = true;
	}

	int iMaxTimeWidth = 0;
	int iMaxDateWidth = 0;
	if ( bHaveTimeDate && !bExactCalc )
		GetMaxTimeDateWidth ( hDC, iMaxTimeWidth, iMaxDateWidth );

	int iMaxSizeWidth = 0;
	if ( bHaveSize && !bExactCalc )
		iMaxSizeWidth = GetMaxSizeWidth ( hDC );

	const int iIconWidth = GetSystemMetrics ( SM_CXSMICON );

	for ( int i = 0; i < nColumns; ++i )
	{
		int iMaxColPx = 0;

		for ( int j = 0; j < nItems; ++j )
		{
			int iColPx = 0;
			const PanelItem_t & Item = GetItem ( j );
			const PanelColumn_t & Column = m_ViewModes.GetColumn ( m_eViewMode, i );

			bool bDir = !!(Item.m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	
			switch ( Column.m_eType )
			{
			case COL_FILENAME:
				iColPx = CalcTextWidth ( hDC, Item.m_FindData.cFileName );
				if ( m_ViewModes.m_dViewModes [m_eViewMode].m_bShowIcons )
					iColPx += iIconWidth + 1;
				break;

			case COL_TIME_CREATION:	iColPx = bExactCalc ? CalcTextWidth ( hDC, FormatTimeCreation ( szTmp, Item ) ) : iMaxTimeWidth;	break;
			case COL_TIME_ACCESS:	iColPx = bExactCalc ? CalcTextWidth ( hDC, FormatTimeAccess ( szTmp, Item ) )	: iMaxTimeWidth;	break;
			case COL_TIME_WRITE:	iColPx = bExactCalc ? CalcTextWidth ( hDC, FormatTimeWrite ( szTmp, Item ) )	: iMaxTimeWidth;	break;
			case COL_DATE_CREATION:	iColPx = bExactCalc ? CalcTextWidth ( hDC, FormatDateCreation ( szTmp, Item ) ) : iMaxDateWidth;	break;
			case COL_DATE_ACCESS:	iColPx = bExactCalc ? CalcTextWidth ( hDC, FormatDateAccess ( szTmp, Item ) )	: iMaxDateWidth;	break;
			case COL_DATE_WRITE:	iColPx = bExactCalc ? CalcTextWidth ( hDC, FormatDateWrite ( szTmp, Item ) )	: iMaxDateWidth;	break;
			case COL_SIZE:			iColPx = bExactCalc ? CalcTextWidth ( hDC, FormatSize ( szTmp, Item.m_FindData.nFileSizeHigh, Item.m_FindData.nFileSizeLow, bDir ) ) : iMaxSizeWidth; break;
			case COL_SIZE_PACKED:	iColPx = bExactCalc ? CalcTextWidth ( hDC, FormatSize ( szTmp, 0, Item.m_uPackSize, bDir ) ) : iMaxSizeWidth;  break;

			default:
				if ( Column.m_eType >= COL_TEXT_0 && Column.m_eType <= COL_TEXT_9 )
				{
					if ( Item.m_dCustomColumnData )
					{
						int iColumn = Column.m_eType - COL_TEXT_0;

						if ( iColumn < Item.m_nCustomColumns )
							iColPx = CalcTextWidth ( hDC, Item.m_dCustomColumnData [iColumn] );
					}
				}
				break;
			}

			if ( iColPx > iMaxColPx )
				iMaxColPx = iColPx;
		}

		m_dColumnSizes [i].m_iMaxTextWidth = iMaxColPx;
	}
}


void Panel_c::InvalidateButtons ()
{
	InvalidateRect ( m_BtnMax.m_hButton,		NULL, FALSE );
	InvalidateRect ( m_BtnBookmarks.m_hButton,	NULL, FALSE );
	InvalidateRect ( m_BtnUp.m_hButton,			NULL, FALSE );

	if ( m_bMaximized )
		InvalidateRect ( m_BtnSwitch.m_hButton, NULL, FALSE );
}


const wchar_t *	Panel_c::FormatSize ( wchar_t * szBuf, DWORD uSizeHigh, DWORD uSizeLow, bool bDirectory )
{
	if ( bDirectory )
		wsprintf ( szBuf, Txt ( T_DIR_MARKER ) );
	else
		FileSizeToString ( uSizeHigh, uSizeLow, szBuf, false );

	return szBuf;
}


void Panel_c::GetMaxTimeDateWidth ( HDC hDC, int & iTimeWidth, int & iDateWidth ) const
{
	static wchar_t szTime [TIMEDATE_BUFFER_SIZE];
	static wchar_t szDate [TIMEDATE_BUFFER_SIZE];

	// setup max time
	SYSTEMTIME tTime;
	tTime.wYear			= 9999;
	tTime.wMonth		= 12;
	tTime.wDayOfWeek	= 6;
	tTime.wDay			= 31;
	tTime.wHour			= 23;
	tTime.wMinute		= 59;
	tTime.wSecond		= 59;
	tTime.wMilliseconds	= 999;
	GetTimeFormat ( LOCALE_SYSTEM_DEFAULT, TIME_NOSECONDS | TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT, &tTime, NULL, szTime, TIMEDATE_BUFFER_SIZE );
	GetDateFormat ( LOCALE_SYSTEM_DEFAULT, DATE_SHORTDATE, &tTime, NULL, szDate, TIMEDATE_BUFFER_SIZE );

	iTimeWidth = CalcMaxStringWidth ( hDC, szTime );
	iDateWidth = CalcMaxStringWidth ( hDC, szDate );
}


int	Panel_c::GetMaxSizeWidth ( HDC hDC ) const
{
	int iMaxWidth = 0;
	wchar_t szSize [128];

	// byte size
	wcscpy ( szSize, L"999999" );
	int iWidth = CalcMaxStringWidth ( hDC, szSize );
	if ( iWidth > iMaxWidth )
		iMaxWidth = iWidth;

	// kbyte size
	wcscpy ( szSize, L"9999 " );
	wcscat ( szSize, Txt ( T_SIZE_KBYTES ) );
	iWidth = CalcMaxStringWidth ( hDC, szSize );
	if ( iWidth > iMaxWidth )
		iMaxWidth = iWidth;

	// mbyte
	wcscpy ( szSize, L"999.9 " );
	wcscat ( szSize, Txt ( T_SIZE_MBYTES ) );
	iWidth = CalcMaxStringWidth ( hDC, szSize );
	if ( iWidth > iMaxWidth )
		iMaxWidth = iWidth;

	// gbyte
	wcscpy ( szSize, L"999.9 " );
	wcscat ( szSize, Txt ( T_SIZE_GBYTES ) );
	iWidth = CalcMaxStringWidth ( hDC, szSize );
	if ( iWidth > iMaxWidth )
		iMaxWidth = iWidth;

	return iMaxWidth;
}


int Panel_c::CalcMaxStringWidth ( HDC hDC, const wchar_t * szString ) const
{
	Assert ( szString );

	const wchar_t * szStart = szString;
	int iWidth = 0;
	while ( *szStart )
	{
		ABC tABC;
		if ( *szStart >= L'0' && *szStart <= L'9' )
			tABC = g_tPanelFont.GetMaxDigitWidthABC ();
		else
			GetCharABCWidths ( hDC, *szStart, *szStart, &tABC );

		iWidth += tABC.abcA + tABC.abcB + tABC.abcC;

		++szStart;
	}

	return iWidth;
}


int	Panel_c::CalcTextWidth ( HDC hDC, const wchar_t * szString ) const
{
	if ( !szString )
		return 0;

	RECT CalcRect;
	memset ( &CalcRect, 0, sizeof ( CalcRect ) );
	DrawText ( hDC, szString, wcslen ( szString ), &CalcRect, DT_NOCLIP | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX );
	return CalcRect.right - CalcRect.left;
}


void Panel_c::UpdateScrollInfo ()
{
	const int nNumItems = GetNumItems ();
	const int nViewItems = GetMaxItemsInView ();

	m_bSliderVisible = nViewItems > 0 && nViewItems < nNumItems;

	if ( m_bSliderVisible )
	{
		RECT Center;
		GetCenterRect ( Center );

		int iSliderLeft = cfg->pane_slider_left ? 0 : m_Rect.right - cfg->pane_slider_size_x;
		SetWindowPos ( m_hSlider, HWND_TOP, iSliderLeft, Center.top, cfg->pane_slider_size_x, Center.bottom - Center.top, SWP_SHOWWINDOW );

		SCROLLINFO tScrollInfo;
		tScrollInfo.cbSize	= sizeof ( SCROLLINFO );
		tScrollInfo.fMask	= SIF_ALL;
		tScrollInfo.nMin	= 0;
		tScrollInfo.nMax	= nNumItems - 1;
		tScrollInfo.nPage	= nViewItems;
		tScrollInfo.nPos	= m_iFirstItem;
		tScrollInfo.nTrackPos= 0;
		SetScrollInfo ( m_hSlider, SB_CTL, &tScrollInfo, TRUE );
	}
	else
		ShowWindow ( m_hSlider, SW_HIDE );
}


int	Panel_c::GetCursorItem () const
{
	return m_iCursorItem;
}


const SelectedFileList_t * Panel_c::GetSelectedItems ()
{
	m_SelectedList.m_dFiles.Clear ();
	m_SelectedList.m_sRootDir = GetDirectory ();

	if ( GetNumMarked () )
		m_SelectedList.m_uSize = GetMarkedSize ();
	else
	{
		m_SelectedList.m_uSize.HighPart = GetItem ( m_iCursorItem ).m_FindData.nFileSizeHigh;
		m_SelectedList.m_uSize.LowPart = GetItem ( m_iCursorItem ).m_FindData.nFileSizeLow;
	}

	if ( GetNumMarked () )
	{
		for ( int i = 0; i < GetNumItems (); ++i )
		{
			if ( GetItemFlags ( i ) & FILE_MARKED )
				m_SelectedList.m_dFiles.Add ( &GetItem ( i ) );
		}
	}
	else
	{
		if ( !GetNumItems () )
			return &m_SelectedList;

		if ( GetItemFlags ( m_iCursorItem ) & FILE_PREV_DIR )
			return &m_SelectedList;

		m_SelectedList.m_dFiles.Add ( &GetItem ( m_iCursorItem ) );
	}

	return &m_SelectedList;
}



void Panel_c::UpdateScrollPos ()
{
	SCROLLINFO tScrollInfo;
	tScrollInfo.cbSize	= sizeof ( SCROLLINFO );
	tScrollInfo.fMask	= SIF_POS;
	tScrollInfo.nPos	= m_iFirstItem;
	tScrollInfo.nTrackPos= 0;
	SetScrollInfo ( m_hSlider, SB_CTL, &tScrollInfo, TRUE );
}


void Panel_c::ShowBookmarks ()
{
	int iX, iY;
	m_BtnBookmarks.GetPosition ( iX, iY );

	Str_c sPath;
	if ( ShowDriveMenu ( iX, iY, m_hWnd, sPath ) )
		ProcessDriveMenuResults ( sPath );
}


void Panel_c::ProcessDriveMenuResults ( const wchar_t * szPath )
{
	Str_c sPath ( szPath );
	RemoveSlash ( sPath );

	DWORD dwAttributes = GetFileAttributes ( sPath );
	if ( dwAttributes != 0xFFFFFFFF )
	{
		if ( ! ( dwAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
		{
			FMExecuteFile ( sPath );
			return;
		}
	}

	SetDirectory ( sPath );
	Refresh ();
}


void Panel_c::CreateMenus ()
{
	Assert ( ! m_hSortModeMenu );
	m_hSortModeMenu = CreatePopupMenu ();
	AppendMenu ( m_hSortModeMenu, MF_STRING,	IDM_HEADER_SORT_UNSORTED,		Txt ( T_MNU_SORT_UNSORTED ) );
	AppendMenu ( m_hSortModeMenu, MF_STRING,	IDM_HEADER_SORT_NAME,			Txt ( T_MNU_SORT_NAME ) );
	AppendMenu ( m_hSortModeMenu, MF_STRING,	IDM_HEADER_SORT_EXTENSION,		Txt ( T_MNU_SORT_EXT ) );
	AppendMenu ( m_hSortModeMenu, MF_STRING,	IDM_HEADER_SORT_SIZE,			Txt ( T_MNU_SORT_SIZE ) );
	AppendMenu ( m_hSortModeMenu, MF_STRING,	IDM_HEADER_SORT_TIME_CREATE,	Txt ( T_MNU_SORT_TIME_CREATE ) );
	AppendMenu ( m_hSortModeMenu, MF_STRING,	IDM_HEADER_SORT_TIME_ACCESS,	Txt ( T_MNU_SORT_TIME_ACCESS ) );
	AppendMenu ( m_hSortModeMenu, MF_STRING,	IDM_HEADER_SORT_TIME_WRITE,		Txt ( T_MNU_SORT_TIME_WRITE ) );
	AppendMenu ( m_hSortModeMenu, MF_STRING,	IDM_HEADER_SORT_SIZE_PACKED,	Txt ( T_MNU_SORT_SIZE_PACKED ) );
	AppendMenu ( m_hSortModeMenu, MF_STRING,	IDM_HEADER_SORT_GROUP,			Txt ( T_MNU_SORT_GROUP ) );
	AppendMenu ( m_hSortModeMenu, MF_SEPARATOR,	IDM_HEADER_SORT_SEPARATOR,	NULL );
	AppendMenu ( m_hSortModeMenu, MF_STRING,	IDM_HEADER_SORT_REVERSE,		Txt ( T_MNU_SORT_REVERSE ) );
	m_hViewModeMenu = CreatePopupMenu ();
	AppendMenu ( m_hViewModeMenu, MF_STRING,	IDM_HEADER_VIEW_1C,				Txt ( T_MNU_VIEW_1C ) );
	AppendMenu ( m_hViewModeMenu, MF_STRING,	IDM_HEADER_VIEW_2C,				Txt ( T_MNU_VIEW_2C ) );
	AppendMenu ( m_hViewModeMenu, MF_STRING,	IDM_HEADER_VIEW_3C,				Txt ( T_MNU_VIEW_3C ) );
	AppendMenu ( m_hViewModeMenu, MF_STRING,	IDM_HEADER_VIEW_BRIEF,			Txt ( T_MNU_VIEW_BRIEF ) );
	AppendMenu ( m_hViewModeMenu, MF_STRING,	IDM_HEADER_VIEW_MEDIUM,			Txt ( T_MNU_VIEW_MEDIUM ) );
	AppendMenu ( m_hViewModeMenu, MF_STRING,	IDM_HEADER_VIEW_FULL,			Txt ( T_MNU_VIEW_FULL ) );
	m_hHeaderMenu = CreatePopupMenu ();
	AppendMenu ( m_hHeaderMenu, MF_STRING, 		IDM_HEADER_OPEN_OPPOSITE, 		Txt ( T_MNU_OPEN_2ND_PANE_FLD ) );
	AppendMenu ( m_hHeaderMenu, MF_STRING, 		IDM_HEADER_OPEN, 				Txt ( T_MNU_OPEN_DOT ) );
	AppendMenu ( m_hHeaderMenu, MF_SEPARATOR,	NULL, L"" );
	AppendMenu ( m_hHeaderMenu, MF_POPUP | MF_STRING, (UINT) m_hSortModeMenu, 	Txt ( T_MNU_HEADER_SORT ) );
	AppendMenu ( m_hHeaderMenu, MF_POPUP | MF_STRING, (UINT) m_hViewModeMenu, 	Txt ( T_MNU_HEADER_VIEW ) );
}


void Panel_c::ShowHeaderMenu ( int iX, int iY )
{
	if ( FMIsAnyPaneInMenu () || menu::IsInMenu () )
		return;

	Assert ( m_hHeaderMenu );

	POINT PtShow = { iX, iY };
	ClientToScreen ( m_hWnd, &PtShow );

	// update sort mode menu
	CheckMenuRadioItem ( m_hSortModeMenu, IDM_HEADER_SORT_UNSORTED, IDM_HEADER_SORT_TIME_WRITE, GetSortMode () + IDM_HEADER_SORT_UNSORTED, MF_BYCOMMAND );
	CheckMenuItem ( m_hSortModeMenu, IDM_HEADER_SORT_REVERSE, MF_BYCOMMAND | ( GetSortReverse () ? MF_CHECKED : MF_UNCHECKED ) );

	// update view mode menu
	int iModeCheck = m_eViewMode;
	CheckMenuRadioItem ( m_hViewModeMenu, IDM_HEADER_VIEW_1C, IDM_HEADER_VIEW_FULL, iModeCheck + IDM_HEADER_VIEW_1C, MF_BYCOMMAND );

	int iRes = TrackPopupMenu ( m_hHeaderMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, PtShow.x, PtShow.y, 0, m_hWnd, NULL );
	if ( !iRes )
		return;

	SortMode_e eOldSortMode = GetSortMode ();
	SortMode_e eNewSortMode = eOldSortMode;

	bool bOldReverse = GetSortReverse ();
	bool bNewReverse = bOldReverse;

	ViewMode_e eOldView = m_eViewMode;
	ViewMode_e eNewView	= eOldView;

	switch ( iRes )
	{
	case IDM_HEADER_OPEN_OPPOSITE:
		SetDirectory ( FMGetPassivePanel ()->GetDirectory () );
		Refresh ();
		break;
	case IDM_HEADER_OPEN:
		FMOpenFolder ();
		break;
	case IDM_HEADER_SORT_UNSORTED:		eNewSortMode = SORT_UNSORTED;		break;
	case IDM_HEADER_SORT_NAME:			eNewSortMode = SORT_NAME;			break;
	case IDM_HEADER_SORT_EXTENSION:		eNewSortMode = SORT_EXTENSION;		break;
	case IDM_HEADER_SORT_SIZE:			eNewSortMode = SORT_SIZE;			break;
	case IDM_HEADER_SORT_TIME_CREATE:	eNewSortMode = SORT_TIME_CREATE;	break;
	case IDM_HEADER_SORT_TIME_ACCESS:	eNewSortMode = SORT_TIME_ACCESS;	break;
	case IDM_HEADER_SORT_TIME_WRITE:	eNewSortMode = SORT_TIME_WRITE;		break;
	case IDM_HEADER_SORT_GROUP:			eNewSortMode = SORT_GROUP;			break;
	case IDM_HEADER_SORT_SIZE_PACKED:	eNewSortMode = SORT_SIZE_PACKED;	break;
	case IDM_HEADER_SORT_REVERSE:		bNewReverse = !bOldReverse;			break;
	case IDM_HEADER_VIEW_1C:			eNewView = VM_COLUMN_1;				break;
	case IDM_HEADER_VIEW_2C:			eNewView = VM_COLUMN_2;				break;
	case IDM_HEADER_VIEW_3C:			eNewView = VM_COLUMN_3;				break;
	case IDM_HEADER_VIEW_BRIEF:			eNewView = VM_BRIEF;				break;
	case IDM_HEADER_VIEW_MEDIUM:		eNewView = VM_MEDIUM;				break;
	case IDM_HEADER_VIEW_FULL:			eNewView = VM_FULL;					break;
	}

	if ( bNewReverse != bOldReverse || eNewSortMode != eOldSortMode )
	{
		SetSortMode ( eNewSortMode, bNewReverse );
		Resort ();
	}

	if ( eNewView != eOldView )
	{
		SetViewMode ( eNewView );
		InvalidateCenter ();
		UpdateScrollInfo ();
	}
}


void Panel_c::ShowEmptyMenu ( int iX, int iY )
{
	if ( FMIsAnyPaneInMenu () || menu::IsInMenu () )
		return;

	POINT tShow = {iX, iY};
	ClientToScreen ( m_hWnd, &tShow );

	bool bMarked = GetNumMarked () > 0;
	bool bHaveFiles = GetNumItems () > 0 && !( GetNumItems () == 1 && ( GetItemFlags ( 0 ) & FILE_PREV_DIR ) );

	HMENU hMenu = CreatePopupMenu ();
	if ( bHaveFiles )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_MARK_ALL,	Txt ( T_MNU_SELECT_ALL ) );

	if ( bMarked )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_CLEAR_ALL,	Txt ( T_MNU_CLEAR_SELECTION ) );

	if ( ( bMarked || bHaveFiles ) && ( bMarked || ! clipboard::IsEmpty () ) )
		AppendMenu ( hMenu, MF_SEPARATOR, IDM_FILE_MENU_SEPARATOR,	L"" );

	if ( bMarked )
	{
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_COPYCLIP,	Txt ( T_MNU_COPYCLIP ) );
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_CUTCLIP,	Txt ( T_MNU_CUTCLIP ) );
	}

	if ( !clipboard::IsEmpty () )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_PASTECLIP,	Txt ( clipboard::IsMoveMode () ? T_MNU_PASTEMOVE : T_MNU_PASTECOPY ) );

	//////////////////////////////////////////////////////////////////////////

	int iRes = TrackPopupMenu ( hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, tShow.x, tShow.y, 0, m_hWnd, NULL );
	DestroyMenu ( hMenu );

	switch ( iRes )
	{
	case IDM_FILE_MENU_MARK_ALL:
		MarkAllFiles ( true );
		break;
	case IDM_FILE_MENU_CLEAR_ALL:
		MarkAllFiles ( false, true );
		break;
	case IDM_FILE_MENU_COPYCLIP:
		clipboard::Copy ( this );
		break;
	case IDM_FILE_MENU_CUTCLIP:
		clipboard::Cut ( this );
		break;
	case IDM_FILE_MENU_PASTECLIP:
		clipboard::Paste ( GetDirectory () );
		break;
	}
}


void Panel_c::ShowRecentMenu ( int iX, int iY )
{
	// FIXME
}


static int ComparePlugins ( const void * pPlugin1, const void * pPlugin2 )
{
	return wcscmp ( ((Plugin_c*)pPlugin1)->GetDLLName (), ((Plugin_c*)pPlugin2)->GetDLLName () );
}

struct MenuToPlugin_t
{
	int			m_iMenuItem;
	int			m_iSubmenuItem;
	Plugin_c *	m_pPlugin;
};


static void AddPlugin ( HMENU hSubMenu, Plugin_c * pPlugin, Array_T<MenuToPlugin_t> & dMenuToPlugin )
{
	for ( int i = 0; i < pPlugin->GetNumSubmenuItems (); ++i )
	{
		int nMenuItemsUsed = dMenuToPlugin.Length ();
		AppendMenu ( hSubMenu, MF_STRING, IDM_FILE_MENU_PLUGIN_0 + nMenuItemsUsed, pPlugin->GetSubmenuName(i) );
		dMenuToPlugin.Grow ( dMenuToPlugin.Length () + 1 );
		dMenuToPlugin.Last ().m_iMenuItem = IDM_FILE_MENU_PLUGIN_0 + nMenuItemsUsed;
		dMenuToPlugin.Last ().m_iSubmenuItem = i;
		dMenuToPlugin.Last ().m_pPlugin = pPlugin;
	}
}


void Panel_c::ShowFileMenu ( int iFile, int iX, int iY )
{
	if ( FMIsAnyPaneInMenu () || menu::IsInMenu () )
		return;

	if ( GetItemFlags ( iFile ) & FILE_PREV_DIR )
		return;

	SetCursorItemInvalidate ( iFile );

	POINT tShow = {iX, iY};
	ClientToScreen ( m_hWnd, &tShow );

	const PanelItem_t & Item = GetItem ( iFile );

	bool bDir = !!(GetItem ( iFile ).m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	bool bFileMarked = !!( GetItemFlags ( iFile ) & FILE_MARKED );
	bool bMarked = GetNumMarked () > 0;
	bool bOneFile = !bMarked;
	bool bPlugin = m_hActivePlugin != INVALID_HANDLE_VALUE;
	bool bHandlesRealNames = bPlugin && g_PluginManager.HandlesRealNames ( m_hActivePlugin );

	//////////////////////////////////////////////////////////////////////////
	HMENU hMenu = CreatePopupMenu ();
	if ( !bPlugin || bHandlesRealNames )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_PROPERTIES, 	Txt ( T_MNU_PROPS ) );

	const OpenWithArray_t * pArray = NULL;

	if ( bOneFile && !bDir && ( !bPlugin || bHandlesRealNames ) )
	{
		HMENU hSubMenu = CreatePopupMenu ();

		AppendMenu ( hSubMenu, MF_STRING, IDM_FILE_MENU_OPEN, Txt ( T_MNU_OPEN ) );
		AppendMenu ( hSubMenu, MF_STRING, IDM_FILE_MENU_OPENWITH, Txt ( T_MNU_OPEN_WITH ) );

		const wchar_t * szExt = GetNameExtFast ( Item.m_FindData.cFileName );
		if ( !wcscmp ( szExt, L".exe" ) )
			AppendMenu ( hSubMenu, MF_STRING, IDM_FILE_MENU_RUNPARAMS, Txt ( T_MNU_RUN_DOT ) );

		pArray = g_tRecent.GetOpenWithFor ( szExt );
		if ( pArray )
		{
			AppendMenu ( hSubMenu, MF_SEPARATOR, IDM_FILE_MENU_SEPARATOR, L"" );
			for ( int i = pArray->Length () - 1; i >= 0 ; --i )
				AppendMenu ( hSubMenu, MF_STRING, IDM_FILE_MENU_OPEN_0 + i, (*pArray) [i].m_sName );
		}

		AppendMenu ( hMenu, MF_POPUP | MF_STRING, (UINT) hSubMenu, Txt ( T_MNU_OPEN_DOT ) );
	}

	if ( bOneFile && bDir )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_OPPOSITE, 	Txt ( T_MNU_OPEN_IN_2ND ) );

	AppendMenu ( hMenu, MF_SEPARATOR, IDM_FILE_MENU_SEPARATOR,	L"" );

	if ( bOneFile && !bDir && ( !bPlugin || bHandlesRealNames ) )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_VIEW, 		Txt ( T_MNU_VIEW ) );

	if ( !bPlugin || bHandlesRealNames )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_COPYMOVE, 		Txt ( T_MNU_COPYMOVE ) );

	if ( bOneFile && ( !bPlugin || bHandlesRealNames ) )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_RENAME, 	Txt ( T_MNU_RENAME ) );

	if ( !bPlugin || bHandlesRealNames || bPlugin && g_PluginManager.HandlesDelete ( m_hActivePlugin ) )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_DELETE, 		Txt ( T_MNU_DELETE ) );

	if ( bOneFile && ( !bPlugin || bHandlesRealNames ) )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_SHORTCUT, 	Txt ( T_MNU_CREATE_SHORTCUT ) );

	AppendMenu ( hMenu, MF_SEPARATOR, IDM_FILE_MENU_SEPARATOR,	L"" );
	if ( !bPlugin )
	{
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_COPYCLIP,		Txt ( T_MNU_COPYCLIP ) );
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_CUTCLIP,		Txt ( T_MNU_CUTCLIP ) );

		if ( !clipboard::IsEmpty () )
			AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_PASTECLIP,	Txt ( clipboard::IsMoveMode () ? T_MNU_PASTEMOVE : T_MNU_PASTECOPY ) );

		AppendMenu ( hMenu, MF_SEPARATOR, IDM_FILE_MENU_SEPARATOR,	L"" );
	}

	HMENU hSelMenu = CreatePopupMenu ();
	if ( bFileMarked )
		AppendMenu ( hSelMenu, MF_STRING, IDM_FILE_MENU_CLEAR, 		Txt ( T_MNU_CLEAR ) );

	if ( bMarked )
		AppendMenu ( hSelMenu, MF_STRING, IDM_FILE_MENU_CLEAR_ALL,	Txt ( T_MNU_CLEAR_SELECTION ) );
	
	if ( !bFileMarked )
		AppendMenu ( hSelMenu, MF_STRING, IDM_FILE_MENU_MARK, 		Txt ( T_MNU_SELECT ) );

	AppendMenu ( hSelMenu, MF_STRING, IDM_FILE_MENU_MARK_ALL,		Txt ( T_MNU_SELECT_ALL ) );
	AppendMenu ( hMenu, MF_POPUP | MF_STRING, (UINT) hSelMenu,		Txt ( T_MNU_SELECTION ) );
	if ( bOneFile && ( !bPlugin || bDir ) )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_BOOKMARK, 	Txt ( T_MNU_ADD_BM ) );

	AppendMenu ( hMenu, MF_SEPARATOR, IDM_FILE_MENU_SEPARATOR,	L"" );

	// and now the plugins
	Array_T <MenuToPlugin_t> dMenuToPlugin;
	
	if ( !bPlugin || bHandlesRealNames )
	{
		Array_T <Plugin_c *> dPlugins;
		dPlugins.Reserve ( 16 );

		for ( int i = 0; i < g_PluginManager.GetNumPlugins (); ++i )
		{
			Plugin_c * pPlugin = g_PluginManager.GetPlugin(i);
			if ( pPlugin->GetFlags () & PLUGIN_MENU )
				dPlugins.Add ( pPlugin );
		}

		Sort ( dPlugins, ComparePlugins );

		bool dPluginUsed[64];
		memset ( dPluginUsed, 0, sizeof ( dPluginUsed ) );
		dMenuToPlugin.Reserve ( 16 );

		for ( int i = 0; i < dPlugins.Length (); ++i )
			if ( !dPluginUsed[i] )
			{	
				dPluginUsed[i] = true;
				Plugin_c * pPlugin = dPlugins [i];

				if ( wcslen ( pPlugin->GetMenuName () ) > 0 )
				{
					HMENU hSubMenu = CreatePopupMenu ();
					AddPlugin ( hSubMenu, pPlugin, dMenuToPlugin );
					
					// find common menu names
					for ( int j = i+1; j < dPlugins.Length (); ++j )
						if ( !dPluginUsed[j] && !wcscmp ( dPlugins[j]->GetMenuName (), pPlugin->GetMenuName () ) )
						{
							AddPlugin ( hSubMenu, dPlugins[j], dMenuToPlugin );
							dPluginUsed[j] = true;
						}

					AppendMenu ( hMenu, MF_POPUP | MF_STRING, (UINT) hSubMenu, pPlugin->GetMenuName () );
				}
				else
					AddPlugin ( hMenu, pPlugin, dMenuToPlugin );
			}
	}

//	AppendMenu ( hMenu, MF_POPUP | MF_STRING, (UINT) menu::CreateSendMenu (), Txt ( T_MNU_SEND ) );

	//////////////////////////////////////////////////////////////////////////

	int iRes = TrackPopupMenu ( hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, tShow.x, tShow.y, 0, m_hWnd, NULL );
	DestroyMenu ( hMenu );

	switch ( iRes )
	{
	case IDM_FILE_MENU_OPEN:
		//ExecuteFile ( tInfo );
		break;
	case IDM_FILE_MENU_OPENWITH:
		FMOpenWith ();
		break;
	case IDM_FILE_MENU_RUNPARAMS:
		//FMRunParams ();
		break;
	case IDM_FILE_MENU_VIEW:
		//FMViewFile ();
		break;
	case IDM_FILE_MENU_OPPOSITE:
		FMOpenInOpposite ();
		break;
	case IDM_FILE_MENU_PROPERTIES:
		FMFileProperties ();
		break;
	case IDM_FILE_MENU_COPYMOVE:
		FMCopyMoveFiles ();
		break;
	case IDM_FILE_MENU_RENAME:
		FMRenameFiles ();
		break;
	case IDM_FILE_MENU_DELETE:
		FMDeleteFiles ();
		break;
	case IDM_FILE_MENU_SHORTCUT:
		FMCreateShortcut ( GetDirectory (), true );
		break;
	case IDM_FILE_MENU_BOOKMARK:
		ShowAddBMDialog ( GetDirectory () + Item.m_FindData.cFileName );
		break;
	case IDM_FILE_MENU_MARK:
		if ( MarkFileById ( m_iCursorItem, true ) )
			InvalidateHeader ();
		break;
	case IDM_FILE_MENU_MARK_ALL:
		MarkAllFiles ( true );
		break;
	case IDM_FILE_MENU_CLEAR:
		if ( MarkFileById ( m_iCursorItem, false ) )
			InvalidateHeader ();
		break;
	case IDM_FILE_MENU_CLEAR_ALL:
		MarkAllFiles ( false, true );
		break;
	case IDM_FILE_MENU_COPYCLIP:
		clipboard::Copy ( this );
		break;
	case IDM_FILE_MENU_CUTCLIP:
		clipboard::Cut ( this );
		break;
	case IDM_FILE_MENU_PASTECLIP:
		clipboard::Paste ( GetDirectory () );
		break;
	default:
		if ( iRes >= IDM_FILE_MENU_OPEN_0 && iRes <= IDM_FILE_MENU_OPEN_10 )
		{
			const RecentOpenWith_t & OpenWith = (*pArray) [iRes - IDM_FILE_MENU_OPEN_0];

			Str_c sFileName = GetDirectory () + Item.m_FindData.cFileName;
			Str_c sNameToExec = Str_c ( L"\"" ) + sFileName + L"\"";

			SHELLEXECUTEINFO Execute;
			memset ( &Execute, 0, sizeof ( SHELLEXECUTEINFO ) );
			Execute.cbSize		= sizeof ( SHELLEXECUTEINFO );
			Execute.fMask		= SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
			Execute.lpFile		= OpenWith.m_sFilename;
			Execute.lpParameters= OpenWith.m_bQuotes ? sNameToExec : sFileName;
			Execute.nShow		= SW_SHOW;
			Execute.hInstApp	= g_hAppInstance;
			ShellExecuteEx ( &Execute );
		}

		if ( iRes >= IDM_FILE_MENU_PLUGIN_0 && iRes <= IDM_FILE_MENU_PLUGIN_63 )
		{
			if ( !bPlugin || bHandlesRealNames )
				for ( int i = 0; i < dMenuToPlugin.Length (); ++i )
					if ( dMenuToPlugin [i].m_iMenuItem == iRes )
					{
						const SelectedFileList_t * pList = GetSelectedItems ();
						if ( pList )
							dMenuToPlugin [i].m_pPlugin->HandleMenu ( dMenuToPlugin [i].m_iSubmenuItem, pList->m_sRootDir, (PanelItem_t **)&(pList->m_dFiles [0]), pList->m_dFiles.Length () );

						break;
					}
		}
		break;
	}
}


LRESULT CALLBACK Panel_c::WndProc ( HWND hWnd, UINT Msg, UINT wParam, LONG lParam )
{
	Panel_c * pPanel = NULL;

	if ( FMGetPanel1 () && FMGetPanel1 ()->m_hWnd == hWnd )
		pPanel = FMGetPanel1 ();
	else
		if ( FMGetPanel2 () && FMGetPanel2 ()->m_hWnd == hWnd )
			pPanel = FMGetPanel2 ();

	if ( ! pPanel )
		return DefWindowProc ( hWnd, Msg, wParam, lParam ); 

	bool bCallRegDlg = false;
	static int actioncounter = 0;
	static bool bShown = false;

	switch ( Msg )
	{
	case WM_PAINT:
		{
			PAINTSTRUCT tPS;
			HDC hDC = ::BeginPaint ( hWnd, &tPS );

			if ( cfg->double_buffer )
			{
				HDC hPaintDC = pPanel->BeginPaint ( hDC );
				pPanel->OnPaint ( hPaintDC );
				pPanel->EndPaint ( hDC, tPS );
			}
			else
				pPanel->OnPaint ( hDC );

			::EndPaint ( hWnd, &tPS );

			// time bomb
			if ( actioncounter >= protection::interval*2 && protection::expired && !protection::nag_shown )
				PostQuitMessage ( 0 );
		}
		break;

	case WM_ENTERMENULOOP:
		pPanel->EnterMenu ();
		break;

	case WM_EXITMENULOOP:
		pPanel->ExitMenu ();
		break;

	case WM_ERASEBKGND:
		++actioncounter;
		if ( actioncounter >= protection::interval && ! protection::registered && ! bShown )
		{
			bCallRegDlg = true;
			break;
		}
		return TRUE;

	case WM_LBUTTONDOWN:
		pPanel->OnLButtonDown ( LOWORD(lParam), HIWORD(lParam) );
		return 0;

	case WM_LBUTTONDBLCLK:
		pPanel->OnLButtonDblClk ( LOWORD(lParam), HIWORD(lParam) );
		return 0;

	case WM_LBUTTONUP:
		pPanel->OnLButtonUp ( LOWORD(lParam), HIWORD(lParam) );
		return 0;

	case WM_MOUSEMOVE:
		pPanel->OnMouseMove ( LOWORD(lParam), HIWORD(lParam) );
		return 0;

	case WM_VSCROLL:
		pPanel->OnVScroll ( int ( HIWORD ( wParam ) ), int ( LOWORD ( wParam ) ), (HWND) lParam );
		return 0;

	case WM_MEASUREITEM:
		if ( pPanel->OnMeasureItem ( (int) wParam, ( MEASUREITEMSTRUCT*) lParam ) )
			return TRUE;
		break;

	case WM_DRAWITEM:
		if ( pPanel->OnDrawItem ( (int) wParam, (DRAWITEMSTRUCT*) lParam ) )
			return TRUE;
		break;

	case WM_COMMAND:
		if ( pPanel->OnCommand ( wParam, lParam ) )
			return 0;
		break;
	}

#if FM_NAG_SCREEN
	if ( bCallRegDlg )
	{
		ShowRegisterDialog ( g_hMainWindow );
		bShown = true;
		return FALSE;
	}
#endif

	return DefWindowProc ( hWnd, Msg, wParam, lParam );
}

//////////////////////////////////////////////////////////////////////////
// init the panels
void Init_Panels ()
{
	g_tPanelFont.Init ();
	Panel_c::Init ();
}

// shutdown the panels
void Shutdown_Panels ()
{
	Panel_c::Shutdown ();
	g_tPanelFont.Shutdown ();
}