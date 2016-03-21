#include "pch.h"

#include "LDialogs/ddrive.h"
#include "LCore/clog.h"
#include "LCore/cos.h"
#include "LSettings/sconfig.h"
#include "LSettings/scolors.h"
#include "LSettings/srecent.h"
#include "LSettings/slocal.h"
#include "LFile/fmisc.h"
#include "LFile/fcards.h"
#include "LPanel/presources.h"
#include "LPanel/pbase.h"
#include "LDialogs/dbookmarks.h"
#include "Filemanager/fmmenus.h"
#include "Filemanager/filemanager.h"


#include "projects.h"
#include "Resources/resource.h"
#include "shlobj.h"

extern HWND g_hMainWindow;

const int MAX_NAME_WIDTH = 135;

// max text width (in pixels)
static int g_iMaxSpaceWidth = 0;

static const int MENU_BORDER_OFFSET_ICON_LEFT = 2;
static const int MENU_BORDER_OFFSET_LEFT = 4;
static const int MENU_BORDER_OFFSET_RIGHT = 4;

const int DRIVE_NAME_LENGTH = 64;
static HIMAGELIST g_hSmallIconList = NULL;

static int GetSmallFileIcon ( const wchar_t * szName )
{
	SHFILEINFO tShFileInfo;
	tShFileInfo.iIcon = -1;
	g_hSmallIconList = (HIMAGELIST) SHGetFileInfo ( szName, 0, &tShFileInfo, sizeof (SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON );
	return tShFileInfo.iIcon;
}


struct Drive_t
{
	wchar_t				m_szPath [MAX_PATH];
	wchar_t				m_szName [DRIVE_NAME_LENGTH];
	wchar_t				m_szSpace [FILESIZE_BUFFER_SIZE*2];
	SIZE				m_tNameSize;
	SIZE				m_tSpaceSize;
	int					m_iIcon;
	int					m_iId;
	bool				m_bShowSpace;

	Drive_t ()
		: m_iId			( -1 )
		, m_bShowSpace	( false )
	{
		Reset ();
	}

	Drive_t ( const wchar_t * szPath, const wchar_t * szName, int iId, bool bShowSpace = true, bool bShowIcon = true )
		: m_iId			( iId )
		, m_bShowSpace	( bShowSpace )
	{
		Reset ();

		ULARGE_INTEGER uFree, uTotal;
		wcscpy ( m_szPath, szPath );
		wcscpy ( m_szName, szName + ( szName [0] == L'\\' ? 1 : 0 ) );

		int iNameLen = wcslen ( m_szName );
		if ( m_szName [iNameLen - 1] == L'\\' )
			m_szName [iNameLen - 1] = L'\0';

		if ( bShowIcon )
			m_iIcon = GetSmallFileIcon ( m_szPath );

		if ( bShowSpace )
		{
			wchar_t szBuf1 [FILESIZE_BUFFER_SIZE];
			wchar_t szBuf2 [FILESIZE_BUFFER_SIZE];
			GetDiskFreeSpaceEx ( m_szPath, &uFree, &uTotal, NULL );

			FileSizeToString ( uFree,	szBuf1, true );
			FileSizeToString ( uTotal,	szBuf2, true );

			wsprintf ( m_szSpace, Txt ( T_DLG_DRIVE_FREE ), szBuf1, szBuf2 );
		}
	}

	bool IsSeparator () const
	{
		bool bSeparator = true;
		int nLines = 0;
		const wchar_t * szStart = &m_szName[0];

		while ( *szStart && bSeparator )
		{
			if ( *szStart != L'-' )
				bSeparator = false;
			else
				++nLines;

			++szStart;
		}

		return bSeparator && nLines >= 3;
	}

	int GetWidth () const
	{
		int iIconWidth = g_tConfig.bookmarks_icons ? MENU_BORDER_OFFSET_ICON_LEFT * GetVGAScale () + GetSystemMetrics ( SM_CXSMICON ) : MENU_BORDER_OFFSET_LEFT * GetVGAScale ();
		return  MENU_BORDER_OFFSET_RIGHT * GetVGAScale () + iIconWidth + m_tSpaceSize.cx + m_tNameSize.cx + 1;
	}

	int GetHeight () const
	{
		int iIconHeight = g_tConfig.bookmarks_icons ? GetSystemMetrics ( SM_CYSMICON ) : 0;
		return Max ( iIconHeight, m_tNameSize.cy, m_tSpaceSize.cy );
	}


	void Draw ( HDC hDC, const RECT & tRect, int iState )
	{
		bool bSelected = iState == ODS_SELECTED;
		SetBkMode ( hDC, TRANSPARENT );
		DWORD uTextColor = bSelected ? GetSysColor ( COLOR_HIGHLIGHTTEXT ) : GetSysColor ( COLOR_MENUTEXT );

		SetTextColor ( hDC, uTextColor );

		FillRect ( hDC, &tRect, GetSysColorBrush ( bSelected ? COLOR_HIGHLIGHT : COLOR_MENU ) );

		RECT tDrawRect = tRect;
		int iIconY = tDrawRect.top + ( tDrawRect.bottom - tDrawRect.top - GetSystemMetrics ( SM_CYSMICON ) ) / 2;
		bool bIcon = g_tConfig.bookmarks_icons && m_iIcon != -1;

		tDrawRect.left += bIcon ? MENU_BORDER_OFFSET_ICON_LEFT * GetVGAScale () : MENU_BORDER_OFFSET_LEFT * GetVGAScale ();

		if ( bIcon )
		{
			ImageList_Draw ( g_hSmallIconList, m_iIcon, hDC, tDrawRect.left, iIconY, ILD_TRANSPARENT );
			tDrawRect.left += GetSystemMetrics ( SM_CXSMICON ) + 1;
		}

		HGDIOBJ hOldFont = SelectObject ( hDC, g_tPanelFont.GetHandle () );

		if ( m_tNameSize.cx > MAX_NAME_WIDTH )
			AlignFileName ( hDC, tDrawRect, m_szName );
		else
			DrawText ( hDC, m_szName, wcslen ( m_szName ), &tDrawRect, DT_LEFT | DT_VCENTER | DT_NOCLIP | DT_SINGLELINE | DT_NOPREFIX );

		if ( m_bShowSpace )
		{
			tDrawRect.right -= MENU_BORDER_OFFSET_RIGHT * GetVGAScale ();
			SelectObject ( hDC, g_tPanelFont.GetHandle () );
			SetTextColor ( hDC, bSelected ? GetSysColor ( COLOR_HIGHLIGHTTEXT ) : GetSysColor ( COLOR_MENUTEXT ) );
			DrawText ( hDC, m_szSpace, wcslen ( m_szSpace ), &tDrawRect, DT_RIGHT | DT_VCENTER | DT_NOCLIP | DT_SINGLELINE | DT_NOPREFIX );
		}

		SelectObject ( hDC, hOldFont );
	}

private:
	void Reset ()
	{
		m_szPath [0] = L'\0';
		m_szName [0] = L'\0';
		m_szSpace [0] = L'\0';

		m_tNameSize.cx = m_tNameSize.cy = 0;
		m_tSpaceSize.cx = m_tSpaceSize.cy = 0;
		m_iIcon = -1;
	}
};


static Array_T <Drive_t> g_dDrives;
static HMENU g_hDriveMenu;


static int DriveCompare ( const Drive_t & tDrive1, const Drive_t & tDrive2 )
{
	return wcscmp ( tDrive1.m_szName, tDrive2.m_szName );
}


///////////////////////////////////////////////////////////////////////////////////////////
// returns true if the list of drives has changed
static void RefreshDriveList ()
{
	g_dDrives.Clear ();

	if ( g_tConfig.bookmarks_drives )
	{
		// the root
		g_dDrives.Add ( Drive_t ( L"\\", Txt ( T_MY_DEVICE ), IDM_DRIVE_ROOT ) );

		// the flash cards
		WIN32_FIND_DATA tData;
		for ( int i = 0; i < GetNumStorageCards (); ++i )
		{
			GetStorageCardData ( i, tData );
			g_dDrives.Add ( Drive_t ( tData.cFileName, tData.cFileName, IDM_DRIVE_0 + i ) );
		}

		Sort ( g_dDrives, DriveCompare, 1 );
	}

	// add all bookmarks
	for ( int i = 0; i < g_tRecent.GetNumBookmarks (); ++i )
		g_dDrives.Add ( Drive_t ( g_tRecent.GetBookmark (i).m_sPath, g_tRecent.GetBookmark (i).m_sName, IDM_DRIVE_BM_0 + i, false ) );

	g_dDrives.Add ( Drive_t ( L"", Txt ( T_DLG_ADD_CURFOLDER ), IDM_DRIVE_BM_ADD, false ) );
	g_dDrives.Add ( Drive_t ( L"", Txt ( T_DLG_EDIT_BOOKMARKS ), IDM_DRIVE_BM_EDIT, false ) );

	// calculate text sizes
	g_iMaxSpaceWidth = 0;

	HGDIOBJ hOldFont = SelectObject ( g_tResources.m_hBitmapDC, g_tPanelFont.GetHandle () );
	for ( int i = 0; i < g_dDrives.Length (); ++i )
	{
		Drive_t & tDr = g_dDrives [i];
		GetTextExtentPoint32 ( g_tResources.m_hBitmapDC, tDr.m_szName, wcslen ( tDr.m_szName ), &tDr.m_tNameSize );

		if ( tDr.m_bShowSpace )
		{
			GetTextExtentPoint32 ( g_tResources.m_hBitmapDC, tDr.m_szSpace, wcslen ( tDr.m_szSpace ), &tDr.m_tSpaceSize );
			g_iMaxSpaceWidth = Max ( g_iMaxSpaceWidth, tDr.m_tSpaceSize.cx );
		}
	}

	SelectObject ( g_tResources.m_hBitmapDC, hOldFont );
}


bool CreateDriveMenu ()
{
	Assert ( !g_hDriveMenu );

	g_hDriveMenu = CreatePopupMenu ();
	if ( ! g_hDriveMenu )
		return false;

	for ( int i = 0; i < g_dDrives.Length (); ++i )
	{
		if ( i && ( g_dDrives [i].m_iId == IDM_DRIVE_BM_0 || g_dDrives [i].m_iId == IDM_DRIVE_BM_ADD ) )
			AppendMenu ( g_hDriveMenu, MF_SEPARATOR, IDM_DRIVE_SEPARATOR, NULL );

		if ( g_dDrives [i].IsSeparator () )
			AppendMenu ( g_hDriveMenu, MF_SEPARATOR, IDM_DRIVE_SEPARATOR, NULL );
		else
			AppendMenu ( g_hDriveMenu, MF_ENABLED | MF_OWNERDRAW, g_dDrives [i].m_iId, (wchar_t *)i );
	}

	return true;
}

void DestroyDriveMenu ()
{
	if ( g_hDriveMenu )
	{
		DestroyMenu ( g_hDriveMenu );
		g_hDriveMenu = NULL;
	}
}

bool GetDriveMenuItemSize ( int iItem, SIZE & tSize )
{
	if ( iItem < 0 || iItem > g_dDrives.Length () )
		return false;

	tSize.cx = g_dDrives [iItem].GetWidth ();
	tSize.cy = g_dDrives [iItem].GetHeight ();
	return true;
}

bool DriveMenuDrawItem ( const DRAWITEMSTRUCT * pDrawItem )
{
	int iItem = pDrawItem->itemData;

	if ( iItem < 0 || iItem > g_dDrives.Length () )
		return false;

	g_dDrives [iItem].Draw ( pDrawItem->hDC, pDrawItem->rcItem, pDrawItem->itemState );
	return true;
}

bool ShowDriveMenu ( int iX, int iY, HWND hWnd, Str_c & sPath )
{
	if ( Panel_c::IsAnyPaneInMenu () || menu::IsInMenu () )
		return false;

	RefreshDriveList ();

	DestroyDriveMenu ();
	CreateDriveMenu ();

	int iRes = TrackPopupMenu ( g_hDriveMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, iX, iY, 0, hWnd, NULL );

	switch ( iRes )
	{
		case IDM_DRIVE_BM_ADD:
			ShowAddBMDialog ( FMGetActivePanel ()->GetDirectory () );
			return true;

		case IDM_DRIVE_BM_EDIT:
			ShowEditBMDialog ( g_hMainWindow );
			return true;
		
		default:
			if ( iRes != 0 && iRes != IDM_DRIVE_SEPARATOR )
			{
				MENUITEMINFO tInfo;
				tInfo.cbSize = sizeof ( MENUITEMINFO );
				tInfo.fMask = MIIM_DATA;
				GetMenuItemInfo ( g_hDriveMenu, iRes, FALSE, &tInfo );
				sPath = g_dDrives [tInfo.dwItemData].m_szPath;
				return true;
			}
			break;
	}

	return false;
}