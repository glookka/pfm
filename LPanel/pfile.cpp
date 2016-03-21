#include "pch.h"

#include "LPanel/pfile.h"

#include "LCore/clog.h"
#include "LSettings/sconfig.h"
#include "LSettings/scolors.h"
#include "LSettings/sbuttons.h"
#include "LSettings/srecent.h"
#include "LSettings/slocal.h"
#include "LFile/fcolorgroups.h"
#include "LFile/fmessages.h"
#include "LCore/cfile.h"
#include "LFile/fmisc.h"
#include "LFile/fclipboard.h"
#include "LPanel/presources.h"
#include "LDialogs/ddrive.h"
#include "LDialogs/dbookmarks.h"
#include "FileManager/fmmenus.h"
#include "FileManager/fmfilefuncs.h"
#include "FileManager/filemanager.h"


#include "aygshell.h"
#include "Resources/resource.h"

enum
{
	MARKEDTEXT_BUFFER_SIZE = 128
};

extern HINSTANCE	g_hAppInstance;
extern HWND			g_hMainWindow;

///////////////////////////////////////////////////////////////////////////////////////////
// the file panel
FilePanel_c::FilePanel_c ()
	: m_hSortModeMenu	( NULL )
	, m_hViewModeMenu	( NULL )
	, m_hHeaderMenu		( NULL )
	, m_hSlider			( NULL )
	, m_hBtnDirs		( NULL )
	, m_hBtnUp			( NULL )
	, m_iCursorFile		( -1 )
	, m_iFirstFile		( 0 )
	, m_iInitialMarkFile( -1 )
	, m_iFirstMarkFile	( -1 )
	, m_iLastMarkFile	( -1 )
	, m_eLastFindResult	( FIND_OUT_OF_PANEL )
	, m_bSingleFileOperation ( false )
	, m_pConfigPath		( NULL )
	, m_pConfigSortMode	( NULL )
	, m_pConfigSortReverse ( NULL )
	, m_pConfigViewMode	( NULL )
	, m_bWasSelected	( false )
	, m_bJustActivated	( false )
	, m_bRefreshQueued	( false )
	, m_bRefreshAnyway	( false )
	, m_tTimeSinceRefresh ( 0.0f )
{
	m_tDirBtnRect.left = m_tDirBtnRect.right = m_tDirBtnRect.top = m_tDirBtnRect.bottom = 0;
	m_tUpBtnRect.left = m_tUpBtnRect.right = m_tUpBtnRect.top = m_tUpBtnRect.bottom = 0;
	CreateDrawObjects ();

	for ( int i = 0; i < FILE_VIEW_TOTAL; ++i )
		m_dFileViews [i] = FileView_c::Create ( FileViewMode_e ( i ) );

	SetViewMode ( FILE_VIEW_2C );
}


FilePanel_c::~FilePanel_c ()
{
	for ( int i = 0; i < FILE_VIEW_TOTAL; ++i )
		SafeDelete ( m_dFileViews [i] );

	DestroyDrawObjects ();
}

void FilePanel_c::OnMaximize ()
{
	if ( IsMaximized () )
	{
		if ( g_tConfig.pane_maximized_force )
			SetViewMode ( FileViewMode_e ( g_tConfig.pane_maximized_view ) );
	}
	else
		SetViewMode ( FileViewMode_e ( *m_pConfigViewMode ) );
}

void FilePanel_c::OnPaint ( HDC hDC )
{
	Panel_c::OnPaint ( hDC );

	HGDIOBJ hOldFont = SelectObject ( hDC, g_tPanelFont.GetHandle () );
	HGDIOBJ hOldPen = SelectObject ( hDC, g_tResources.m_hBorderPen );

	DrawFileInfo ( hDC );

	Assert ( m_pFileView );
	RECT tRect;
	GetFileViewRect ( tRect );
	m_pFileView->SetViewRect ( tRect );
	m_pFileView->Draw ( hDC );

	// restore
	SelectObject ( hDC, hOldFont );
	SelectObject ( hDC, hOldPen );
}


void FilePanel_c::OnLButtonDown ( int iX, int iY )
{
	m_bJustActivated = !IsActive ();

	Panel_c::OnLButtonDown ( iX, iY );

	RECT tRect;
	GetHeaderViewRect ( tRect );

	if ( IsInRect ( iX, iY, tRect ) )
	{
		// we clicked the header
		if ( IsClickAndHoldDetected ( iX, iY ) )
		{
			ShowHeaderMenu ( iX, iY );
			m_bJustActivated = false;
			return;
		}
	}

	m_bWasSelected = false;

	GetFileViewRect ( tRect );
	if ( IsInRect ( iX, iY, tRect ) )
	{
		int iFile = -1;
		FindOnScreen_e eFindRes = FindFileOnScreen ( iFile, iX, iY, tRect );

		bool bPrevDir = false;

		if ( iFile != -1 && eFindRes == FIND_FILE_FOUND )
		{
			m_bWasSelected = iFile == m_iCursorFile;
			ChangeSelectedFile ( iFile );
			const FileInfo_t & tInfo = GetFile ( iFile );
			if ( tInfo.m_uFlags & FLAG_PREV_DIR )
				bPrevDir = true;
		}

		if ( ! bPrevDir && IsClickAndHoldDetected ( iX, iY ) )
		{
			if ( eFindRes == FIND_EMPTY_PANEL )
				ShowEmptyMenu ( iX, iY );
			else
				if ( eFindRes == FIND_FILE_FOUND && iFile != -1 )
					ShowFileMenu ( iFile, iX, iY );
		}
		else
		{
			if ( eFindRes == FIND_EMPTY_PANEL )
			{
				if ( GetNumMarked () > 0 )
					MarkAllFiles ( false, true );
				else
				{
					if ( iFile != -1 )
					{
						m_bWasSelected = iFile == m_iCursorFile;
						ChangeSelectedFile ( iFile );
					}
				}
			}
			else
			{
				SetCapture ( m_hWnd );

				m_iInitialMarkFile = iFile;
				if ( eFindRes != FIND_FILE_FOUND )
					m_iInitialMarkFile = -1;
			}
		}
	}
}


bool FilePanel_c::OnKeyEvent ( int iBtn, btns::Event_e eEvent )
{
	BtnAction_e eAñtion = g_tButtons.GetAction ( iBtn, eEvent );

	switch ( eAñtion )
	{
	case BA_MOVEUP:
		ScrollCursorTo ( m_iCursorFile - 1 );
		return true;
	case BA_MOVEDOWN:
		ScrollCursorTo ( m_iCursorFile + 1 );
		return true;
	case BA_MOVELEFT:
		{
			if ( m_iCursorFile == 0 )
				StepToPrevDir ();
			else
			{
				Assert ( m_pFileView );
				const int nColumnFiles =  m_pFileView->GetNumColumnFiles ();
				if ( m_iCursorFile - nColumnFiles < m_iFirstFile )
				{
					ScrollFilesTo ( m_iFirstFile - nColumnFiles );
					UpdateScrollPos ();
				}

				ChangeSelectedFile ( m_iCursorFile - nColumnFiles );
			}
		}
		return true;
	case BA_MOVERIGHT:
		{
			Assert ( m_pFileView );
			const int nColumnFiles =  m_pFileView->GetNumColumnFiles ();
			const int nViewFiles =  m_pFileView->GetNumViewFiles ();
			if ( m_iCursorFile + nColumnFiles >= m_iFirstFile +  nViewFiles )
			{
				ScrollFilesTo ( m_iFirstFile + nColumnFiles );
				UpdateScrollPos ();
			}

			ChangeSelectedFile ( m_iCursorFile + nColumnFiles );
		}
		return true;
	case BA_MOVEHOME:
		ScrollFilesTo ( 0 );
		UpdateScrollPos ();
		ChangeSelectedFile ( 0 );
		return true;
	case BA_MOVEEND:
		{
			int nFiles = GetNumFiles ();
			ScrollFilesTo ( nFiles );
			UpdateScrollPos ();
			ChangeSelectedFile ( nFiles );
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
		ShowHeaderMenu ( m_tDirBtnRect.right, m_tDirBtnRect.top );
		return true;
	case BA_DRIVE_LIST:
		ShowBookmarks ();
		return true;
	case BA_EXECUTE:
		m_bWasSelected = true;
		DoFileExecuteWork ( m_iCursorFile );
		m_bWasSelected = false;
		return true;
	case BA_PREVDIR:
		StepToPrevDir ();	
		return true;
	case BA_FL_TOGGLE:
		{
			const FileInfo_t & tInfo = GetFile ( GetCursorFile () );
			bool bMarked = !! ( tInfo.m_uFlags & FLAG_MARKED );
			if ( MarkFile ( GetCursorFile (), !bMarked ) )
				InvalidateHeader ();
		}
		return true;
	case BA_FL_TOGGLE_AND_MOVE:
		{
			const FileInfo_t & tInfo = GetFile ( GetCursorFile () );
			bool bMarked = !! ( tInfo.m_uFlags & FLAG_MARKED );
			if ( MarkFile ( GetCursorFile (), !bMarked ) )
				InvalidateHeader ();
			ScrollCursorTo ( GetCursorFile () + 1 );
		}
		return true;
	case BA_FILE_MENU:
		{
			Assert ( m_pFileView );
			RECT tRect;
			m_pFileView->GetFileRect ( GetCursorFile (), tRect );
			ShowFileMenu ( GetCursorFile (), ( tRect.left + tRect.right ) / 2, ( tRect.top + tRect.bottom ) / 2 );
		}
		return true;
	case BA_BM_ADD:
		FMAddBookmark ();
		return true;
	}

	return false;
}


void FilePanel_c::OnLButtonUp ( int iX, int iY )
{
	if ( ! IsActive () )
		FMActivatePanel ( this );

	if ( GetCapture () == m_hWnd )
		ReleaseCapture ();

	if ( m_iFirstMarkFile == -1 )
	{
		RECT tRect;
		GetFileViewRect ( tRect );
		if ( IsInRect ( iX, iY, tRect ) )
			ProcessFileClick ( iX, iY, tRect );
	}
	
	m_bJustActivated = false;
	m_iInitialMarkFile = -1;
	m_iFirstMarkFile = -1;
	m_iLastMarkFile = -1;
}


void FilePanel_c::OnStyleMove ( int iX, int iY )
{
	Assert ( m_pFileView );

	if ( GetCapture () == m_hWnd )
	{
		RECT tViewRect;
		GetFileViewRect ( tViewRect );
		int iFileUnderStyle = -1;
		FindOnScreen_e eFindResult = FindFileOnScreen ( iFileUnderStyle, iX, iY, tViewRect );

		// 1st try - try to capture the first file
		if ( m_iInitialMarkFile == -1 )
		{
			if ( iFileUnderStyle != -1 )
				m_iInitialMarkFile = iFileUnderStyle;

			return;
		}

		// 2nd try - see if we moved to another file
		if ( m_iFirstMarkFile == -1 && m_iInitialMarkFile != iFileUnderStyle )
		{
			if ( ! FMMarkMode () )
				MarkAllFiles ( false );

			ChangeSelectedFile ( m_iInitialMarkFile );
			m_iFirstMarkFile = m_iInitialMarkFile;
		}

		// 3rd try - actually perform markup
		if ( m_iFirstMarkFile != -1 )
		{
			MoveCursorTo ( iX, iY, tViewRect );
			m_eLastFindResult = eFindResult;

			if ( iFileUnderStyle != -1 )
			{
				// calculate if we need to clear selection from the first file
				bool bClearFirst = false;

				RECT tFileRect;
				if ( m_pFileView->GetFileRect ( m_iFirstMarkFile, tFileRect ) )
				{
					int iRectHeight = tFileRect.bottom - tFileRect.top;
					tFileRect.top 	 += ( int ) ( iRectHeight * 0.25f );
					tFileRect.bottom -= ( int ) ( iRectHeight * 0.25f );
					
					bClearFirst = IsInRect ( iX, iY, tFileRect );
				}

				MarkFilesTo ( iFileUnderStyle, bClearFirst );
			}
		}
	}
}


void FilePanel_c::OnVScroll ( int iPos, int iFlags, HWND hSlider )
{
	Assert ( m_pFileView );
	const int nFiles = GetNumFiles ();
	const int nViewFiles = m_pFileView->GetNumViewFiles ();

	switch ( iFlags )
	{
	case SB_LINEUP:
		ScrollFilesTo ( m_iFirstFile - 1 );
		break;
	case SB_LINEDOWN:
		ScrollFilesTo ( m_iFirstFile + 1 );
		break;
	case SB_PAGEUP:
		ScrollFilesTo ( m_iFirstFile - nViewFiles );
		break;
	case SB_PAGEDOWN:
		ScrollFilesTo ( m_iFirstFile + nViewFiles );
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		ScrollFilesTo ( iPos );
		break;
	case SB_TOP:
	case SB_BOTTOM:
	case SB_ENDSCROLL:
		{
			SCROLLINFO tScrollInfo;
			tScrollInfo.cbSize	= sizeof ( SCROLLINFO );
			tScrollInfo.fMask	= SIF_POS;
			tScrollInfo.nPos	= m_iFirstFile;
			SetScrollInfo ( m_hSlider, SB_CTL, &tScrollInfo, TRUE );
		}
		break;
	}
}


void FilePanel_c::OnTimer ()
{
	Assert ( m_pFileView );

	if ( GetCapture () == m_hWnd && m_iFirstMarkFile != -1 )
	{
		switch ( m_eLastFindResult )
		{
		case FIND_PANEL_TOP:
			ScrollFilesTo ( m_iFirstFile - 1 );
			ChangeSelectedFile ( m_iFirstFile );
			MarkFilesTo ( m_iFirstFile, false );
			UpdateScrollPos ();
			break;

		case FIND_PANEL_BOTTOM:
			ScrollFilesTo ( m_iFirstFile + 1 );
			{
				int iLastFile = m_iFirstFile + m_pFileView->GetNumViewFiles () - 1;
				int nFiles = GetNumFiles ();
				if ( iLastFile > nFiles - 1 )
					iLastFile = nFiles - 1;

				ChangeSelectedFile ( iLastFile );
				MarkFilesTo ( iLastFile, false );
			}
			UpdateScrollPos ();
			break;
		}
	}

	m_tTimeSinceRefresh += FM_TIMER_PERIOD_MS;

	bool bEnoughFiles = m_bRefreshAnyway || ( GetNumFiles () <= g_tConfig.refresh_max_files || ! g_tConfig.refresh_max_files );
	if ( m_bRefreshQueued && ( m_tTimeSinceRefresh / 1000.0f >= g_tConfig.refresh_period ) && bEnoughFiles )
		SoftRefresh ();
}

void FilePanel_c::OnFileChange ()
{
	QueueRefresh ();
}

void FilePanel_c::QueueRefresh ( bool bAnyway )
{
	m_bRefreshAnyway = bAnyway;
	m_bRefreshQueued = true;
}

bool FilePanel_c::OnCommand ( WPARAM wParam, LPARAM lParam )
{
	if ( Panel_c::OnCommand ( wParam, lParam ) )	
		return true;

	if ( (HWND) lParam == m_hBtnDirs && HIWORD ( wParam ) == BN_CLICKED )
	{
		POINT tShow;
		tShow.x = m_tDirBtnRect.left;
		tShow.y = m_tDirBtnRect.bottom;

		ClientToScreen ( m_hWnd, &tShow );

		Str_c sPath = GetDirectory ();
		if ( ShowDriveMenu ( tShow.x, tShow.y, m_hWnd, sPath ) )
			ProcessDriveMenuResults ( sPath );

		return true;
	}

	if ( (HWND) lParam == m_hBtnUp && HIWORD ( wParam ) == BN_CLICKED )
	{
		StepToPrevDir ();
		return true;
	}

	return false;
}


bool FilePanel_c::OnMeasureItem ( int iId, MEASUREITEMSTRUCT * pMeasureItem )
{
	if ( Panel_c::OnMeasureItem ( iId, pMeasureItem ) )	
		return true;

	if ( pMeasureItem->CtlType == ODT_MENU && pMeasureItem->itemID >= IDM_DRIVE_0 && pMeasureItem->itemID <= IDM_DRIVE_BM_EDIT )
	{
		// that's the drive menu
		SIZE tSize;
		if ( GetDriveMenuItemSize ( pMeasureItem->itemData, tSize ) )
		{
			pMeasureItem->itemWidth	 = tSize.cx;
			pMeasureItem->itemHeight = tSize.cy;
			return true;
		}

		return false;
	}
	
	return false;
}


bool FilePanel_c::OnDrawItem ( int iId, const DRAWITEMSTRUCT * pDrawItem )
{
	if ( Panel_c::OnDrawItem ( iId, pDrawItem ) )
		return true;

	// the dir button
	if ( pDrawItem->CtlType == ODT_BUTTON && pDrawItem->hwndItem == m_hBtnDirs )
	{
		DrawButton ( pDrawItem, m_tDirBtnRect, g_tResources.m_tBmpDirSize, g_tResources.m_hBmpBtnDir, g_tResources.m_hBmpBtnDirPressed );
		return true;
	}

	// the up button
	if ( pDrawItem->CtlType == ODT_BUTTON && pDrawItem->hwndItem == m_hBtnUp )
	{
		DrawButton ( pDrawItem, m_tUpBtnRect, g_tResources.m_tBmpUpSize, g_tResources.m_hBmpBtnUp, g_tResources.m_hBmpBtnUpPressed );
		return true;
	}

	// the drive menu
	if ( pDrawItem->CtlType == ODT_MENU && pDrawItem->itemID >= IDM_DRIVE_0 && pDrawItem->itemID <= IDM_DRIVE_BM_EDIT )
		return DriveMenuDrawItem ( pDrawItem );

	return false;
}

void FilePanel_c::InvalidateHeader ()
{
	Panel_c::InvalidateHeader ();

	InvalidateRect ( m_hBtnUp, NULL, FALSE  );
	InvalidateRect ( m_hBtnDirs, NULL, FALSE );
}

void FilePanel_c::UpdateAfterLoad ()
{
	Panel_c::UpdateAfterLoad ();

	if ( m_pConfigPath )
		SetDirectory ( *m_pConfigPath );
	
	if ( m_bMaximized && g_tConfig.pane_maximized_force )
		SetViewMode ( FileViewMode_e ( g_tConfig.pane_maximized_view ) );
	else
		SetViewMode ( FileViewMode_e ( *m_pConfigViewMode ) );
		
	if ( m_pConfigSortMode && m_pConfigSortReverse )
		SetSortMode ( SortMode_e ( *m_pConfigSortMode ), *m_pConfigSortReverse );

	SetVisibility ( g_tConfig.file_pane_show_hidden, g_tConfig.file_pane_show_system, g_tConfig.file_pane_show_rom );
}

void FilePanel_c::SetRect ( const RECT & tRect )
{
	Panel_c::SetRect ( tRect );

	Assert ( m_pFileView );
	
	RECT tFileViewRect;
	GetFileViewRect ( tFileViewRect );
	m_pFileView->SetViewRect ( tFileViewRect );

	// new slider position
	int iSliderLeft  = g_tConfig.pane_slider_left ? 0 : m_tRect.right - g_tConfig.pane_slider_size_x;
	SetWindowPos ( m_hSlider, NULL, iSliderLeft, tFileViewRect.top, g_tConfig.pane_slider_size_x, tFileViewRect.bottom - tFileViewRect.top - 1, IsSliderVisible () ? SWP_SHOWWINDOW  : SWP_HIDEWINDOW );

	m_iFirstFile = ClampFirstFile ( m_iFirstFile );
	ScrollCursorTo ( m_iCursorFile );
	UpdateScrollInfo ();

	// new button position
	int iHeight = GetHeaderViewHeight ();
	int iWidth	= g_tResources.m_tBmpDirSize.cx;

	m_tDirBtnRect.left	= m_tRect.left;
	m_tDirBtnRect.right	= m_tRect.left + iWidth;
	m_tDirBtnRect.top	= m_tRect.top;
	m_tDirBtnRect.bottom= m_tRect.top + iHeight;
	SetWindowPos ( m_hBtnDirs, m_hWnd, m_tDirBtnRect.left, m_tDirBtnRect.top, iWidth, iHeight, SWP_SHOWWINDOW );

	iWidth = g_tResources.m_tBmpUpSize.cx;

	RECT tLeftmost;
	GetLeftmostBtnRect ( tLeftmost );
	
	m_tUpBtnRect.right	= tLeftmost.left - 1;
	m_tUpBtnRect.left	= m_tUpBtnRect.right - iWidth;
	m_tUpBtnRect.top	= m_tRect.top;
	m_tUpBtnRect.bottom	= m_tRect.top + iHeight;
	SetWindowPos ( m_hBtnUp, m_hWnd, m_tUpBtnRect.left, m_tUpBtnRect.top, iWidth, iHeight, SWP_SHOWWINDOW );

	InvalidateRect ( m_hWnd, NULL, FALSE );
}

void FilePanel_c::Activate ( bool bActive )
{
	Panel_c::Activate ( bActive );

	if ( m_pFileView )
		m_pFileView->InvalidateCursor ( m_hWnd, FALSE );
}

void FilePanel_c::ChangeSelectedFile ( int iFileId )
{
	int iNewCursor = ClampCursorFile ( iFileId );
	if ( m_iCursorFile != iNewCursor )
	{
		Assert ( m_pFileView );
		m_pFileView->InvalidateCursor ( m_hWnd, TRUE );
		SetCursorFile ( iNewCursor );
		m_pFileView->InvalidateCursor ( m_hWnd, FALSE );
		InvalidateFileInfo ();
	}
}


void FilePanel_c::ExecuteFile ( const FileInfo_t & tInfo )
{
	// we don't execute directories
	if ( tInfo.m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		return;

	// empty ext can't be executed
	const wchar_t * pData = tInfo.m_tData.cFileName;
	while ( *pData && *pData != L'.' )
		++pData;
	
	if ( !*pData )
		return;

	FMExecuteFile ( GetDirectory (), tInfo.m_tData.cFileName );
}


void FilePanel_c::Event_MarkFile ( int nFile, bool bSelect )
{
	Assert ( m_pFileView );

	RECT tRect;
	if ( m_pFileView->GetFileRect ( nFile, tRect ) )
	{
		++tRect.bottom;
		++tRect.right;
		InvalidateRect ( m_hWnd, &tRect, TRUE );		// invalidate file rect in file view
	}
	
	FMSelectionChange_Event ();
}

void FilePanel_c::Event_BeforeSoftRefresh ()
{
	if ( GetCursorFile () != -1 )
		m_sFileAtCursor = GetFile ( GetCursorFile () ).m_tData.cFileName;
	else
		m_sFileAtCursor = L"";
}

void FilePanel_c::Event_AfterSoftRefresh ()
{
	if ( ! m_sFileAtCursor.Empty () )
		SetCursorAtFile ( m_sFileAtCursor );
}

void FilePanel_c::ResetTemporaryStates ()
{
	m_iInitialMarkFile = -1;
	m_iFirstMarkFile = -1;
	m_iLastMarkFile = -1;
	m_eLastFindResult = FIND_OUT_OF_PANEL;
}


void FilePanel_c::ScrollFilesTo ( int iFirstFile )
{
	Assert ( m_pFileView );

	int iNewFirstFile = iFirstFile;
	int iNewCursorFile = m_iCursorFile;

	iNewFirstFile = ClampFirstFile ( iNewFirstFile );

	if ( iNewFirstFile != m_iFirstFile )
	{
		m_iFirstFile = iNewFirstFile;
		InvalidateFiles ();

		iNewCursorFile = ClampCursorFile ( iNewCursorFile );

		if ( iNewCursorFile != m_iCursorFile  )
		{
			SetCursorFile ( iNewCursorFile );
			InvalidateFileInfo ();
		}
	}
}


void FilePanel_c::ScrollCursorTo ( int iCursorFile )
{
	if ( iCursorFile < m_iFirstFile )
	{
		ScrollFilesTo ( iCursorFile );
		ChangeSelectedFile ( iCursorFile );
		UpdateScrollPos ();
	}
	else
	{
		const int nViewFiles = m_pFileView->GetNumViewFiles ();
		if ( iCursorFile < m_pFileView->GetNumViewFiles () + m_iFirstFile )
			ChangeSelectedFile ( iCursorFile );
		else
		{
			ScrollFilesTo ( iCursorFile - nViewFiles + 1 );
			ChangeSelectedFile ( iCursorFile );
			UpdateScrollPos ();
		}
	}
}


int FilePanel_c::ClampCursorFile ( int iCursor ) const
{
	Assert ( m_pFileView );
	const int nViewFiles = m_pFileView->GetNumViewFiles ();
	int iRes = iCursor;

	int nFiles = GetNumFiles ();
	int nMinCursor = Min ( m_iFirstFile + nViewFiles - 1,  nFiles - 1 );

	// in case there are no files
	if ( nMinCursor < 0 )
		return nFiles > 0 ? 0 : -1;

	if ( iCursor > nMinCursor )
		iRes = nMinCursor;

	if ( iCursor < m_iFirstFile )
		iRes = m_iFirstFile;

	return iRes;
}


int FilePanel_c::ClampFirstFile ( int iFirst ) const
{
	Assert ( m_pFileView );
	const int iDelta = GetNumFiles () - m_pFileView->GetNumViewFiles ();
	int iNewFirstFile = iFirst;
	if ( iNewFirstFile > iDelta )
		iNewFirstFile = iDelta;

	if ( iNewFirstFile < 0 )
		iNewFirstFile = 0;

	return iNewFirstFile;
}


void FilePanel_c::MoveCursorTo ( int iX, int iY, const RECT & tViewRect )
{
	int iFileClicked;
	FindOnScreen_e eFindRes = FindFileOnScreen ( iFileClicked, iX, iY, tViewRect );
	if ( eFindRes != FIND_OUT_OF_PANEL )
		ChangeSelectedFile ( iFileClicked );
}


void FilePanel_c::DoFileExecuteWork ( int iFileClicked, bool bClickInPanel )
{
	const FileInfo_t & tInfo = GetFile ( iFileClicked );
	bool bMarkMode = FMMarkMode ();
	bool bMarked = !! ( tInfo.m_uFlags & FLAG_MARKED );
	
	if ( ! m_bWasSelected )
	{
		ChangeSelectedFile ( iFileClicked );
		if ( bMarkMode )
		{
			if ( MarkFile ( iFileClicked, ! bMarked ) )
				InvalidateHeader ();
		}
		else
		{
			if ( g_tConfig.fastnav && bClickInPanel )
			{
				if ( tInfo.m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
					ChangeDirectory ( tInfo );
				else
					ExecuteFile ( tInfo );
			}
		}
	}
	else
		if ( iFileClicked >= 0 )
		{
			// we cancel mark mode if we click "prev dir"
			if ( bMarkMode && ! ( tInfo.m_uFlags & FLAG_PREV_DIR ) )
			{
				if ( MarkFile ( iFileClicked, ! bMarked ) )
					InvalidateHeader ();
			}
			else
			{
				if ( bMarkMode && ( tInfo.m_uFlags & FLAG_PREV_DIR ) )
					FMSetMarkMode ( false );

				if ( bClickInPanel && ( ! m_bJustActivated || g_tConfig.fastnav ) )
				{
					if ( tInfo.m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
						ChangeDirectory ( tInfo );
					else
						ExecuteFile ( tInfo );
				}
			}
		}
}


void FilePanel_c::ProcessFileClick ( int iX, int iY, const RECT & tViewRect )
{
	// we clicked the file list
	int iFileClicked = -1;
	FindOnScreen_e eFindRes = FindFileOnScreen ( iFileClicked, iX, iY, tViewRect );
	if ( iFileClicked == -1 )
		return;

	if ( eFindRes != FIND_OUT_OF_PANEL && eFindRes != FIND_EMPTY_PANEL )
	{
		m_iFirstMarkFile = iFileClicked;
		DoFileExecuteWork ( iFileClicked, eFindRes != FIND_EMPTY_PANEL );
	}
}


void FilePanel_c::ChangeDirectory ( const FileInfo_t & tInfo )
{
	if ( tInfo.m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
	{
		if ( tInfo.m_uFlags & FLAG_PREV_DIR )
			StepToPrevDir ();
		else
		{
			StepToNextDir ( tInfo.m_tData.cFileName );
			Refresh ();
		}
	}
}

void FilePanel_c::ProcessDriveMenuResults ( const wchar_t * szPath )
{
	Str_c sPath ( szPath );

	RemoveSlash ( sPath );
	DWORD dwAttributes = GetFileAttributes ( sPath );
	if ( dwAttributes != 0xFFFFFFFF )
	{
		if ( ! ( dwAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
		{
			FMExecuteFile ( GetPath ( sPath ), GetName ( sPath ) );
			return;
		}
	}

	SetDirectory ( sPath );
	Refresh ();
}

int FilePanel_c::GetMinHeight () const
{
	int iHeight = g_tPanelFont.GetHeight () + GetFileInfoTextHeight () + 6 + g_tConfig.pane_slider_size_x * 2;
	return Max ( iHeight, Panel_c::GetMinHeight () );
}


int FilePanel_c::GetMinWidth () const
{
	int iWidth = g_tResources.m_tBmpDirSize.cx + g_tResources.m_tBmpUpSize.cx + g_tResources.m_tBmpMaxMidSize.cx + 4;
	return Max ( iWidth, Panel_c::GetMinWidth() );
}


void FilePanel_c::DrawPanelHeader ( HDC hDC )
{
	Panel_c::DrawPanelHeader ( hDC );

	const Str_c & sDir = GetDirectory ();
	RECT tHeader;
	GetHeaderViewRect ( tHeader );

	// selection size
	int nSelected = GetNumMarked ();
	RECT tMarked;
	tMarked.left = tHeader.right;
	int iMarkTextWidth = 0;

	// if there's anything marked, draw it!
	if ( nSelected )
	{
		static wchar_t szMarkedText [MARKEDTEXT_BUFFER_SIZE];
		static wchar_t szSize [FILESIZE_BUFFER_SIZE];
		FileSizeToString ( GetMarkedSize (), szSize, false );
		wsprintf ( szMarkedText, L"%s/%d", szSize, nSelected );

		int iSizeLen = wcslen (szMarkedText);

		RECT tCalcMarked;
		memset ( &tCalcMarked, 0, sizeof ( tCalcMarked ) );
		DrawText ( hDC, szMarkedText, iSizeLen, &tCalcMarked, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX );
		iMarkTextWidth = tCalcMarked.right - tCalcMarked.left;

		SetTextColor ( hDC, g_tColors.pane_font_marked_summary );
		
		int nLeft = tHeader.right - tHeader.left - iMarkTextWidth - 3;
		if ( nLeft > 0 )
		{
			tMarked = tHeader;
			tMarked.left = tMarked.right - ( iMarkTextWidth + 1 );
			if ( tMarked.left < m_tRect.left )
				tMarked.left = m_tRect.left;

			CollapseRect ( tMarked, 1 );
			--tMarked.left;
			DrawText ( hDC, szMarkedText, iSizeLen, &tMarked, DT_LEFT | DT_VCENTER | DT_NOCLIP | DT_SINGLELINE | DT_NOPREFIX ); //align right
		}
		else
		{
			tMarked = tHeader;
			// draw clipped size and a '|'
			CollapseRect ( tMarked, 1 );
			tMarked.right -= 2;
			HGDIOBJ hOldPen = SelectObject ( hDC, g_tResources.m_hOverflowPen );
			DrawText ( hDC, szMarkedText, iSizeLen, &tMarked, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
			MoveToEx ( hDC, tMarked.right + 1, tMarked.top, NULL );
			LineTo ( hDC, tMarked.right + 1, tMarked.bottom );
			SelectObject ( hDC, hOldPen );
		}
	}

	SetTextColor ( hDC, g_tColors.pane_font );

	// directory name
	tHeader.right = tMarked.left - 1;
	CollapseRect ( tHeader, 1 );

	// if there's space left, draw the directory name
	if ( ! nSelected || tHeader.right - tHeader.left > 0 )
	{
		RECT tCalcHeader;
		memset ( &tCalcHeader, 0, sizeof ( tCalcHeader ) );
		DrawText ( hDC, sDir, sDir.Length (), &tCalcHeader, DT_SINGLELINE | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX );

		if ( tCalcHeader.right - tCalcHeader.left > tHeader.right - tHeader.left )
		{
			HGDIOBJ hOldPen = SelectObject ( hDC, g_tResources.m_hOverflowPen );
			MoveToEx ( hDC, tHeader.left, tHeader.top, NULL );
			LineTo ( hDC, tHeader.left, tHeader.bottom );
			SelectObject ( hDC, hOldPen );
			++tHeader.left;
			DrawText ( hDC, sDir, sDir.Length (), &tHeader, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX ); //align right
		}
		else
			DrawText ( hDC, sDir, sDir.Length (), &tHeader, DT_LEFT | DT_VCENTER | DT_NOCLIP | DT_SINGLELINE | DT_NOPREFIX ); //align left
	}
}

void FilePanel_c::DrawTextInRect ( HDC hDC, RECT & tRect, const wchar_t * szText, DWORD uColor )
{
	if ( tRect.left >= tRect.right )
		return;

	int iTextLen = wcslen ( szText );

	RECT tCalcRect;
	memset ( &tCalcRect, 0, sizeof ( tCalcRect ) );
	DrawText ( hDC, szText, iTextLen, &tCalcRect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX );

	SetTextColor ( hDC, uColor );
	DrawText ( hDC, szText, iTextLen, &tRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
	tRect.right -= tCalcRect.right - tCalcRect.left;
}

void FilePanel_c::DrawFileInfo ( HDC hDC )
{
	static wchar_t szTime [TIMEDATE_BUFFER_SIZE];
	static wchar_t szDate [TIMEDATE_BUFFER_SIZE];
	static wchar_t szSize [FILESIZE_BUFFER_SIZE];

	szTime [0] = '\0';
	szDate [0] = '\0';
	szSize [0] = '\0';

	RECT tBottomRect;
	GetFileInfoRect ( tBottomRect );

	FillRect ( hDC, &tBottomRect, g_tResources.m_hBackgroundBrush );

	// status bar line
	MoveToEx ( hDC, tBottomRect.left, tBottomRect.top - 1, NULL );
	LineTo ( hDC, tBottomRect.right, tBottomRect.top - 1 );

	if ( m_iCursorFile < 0 )
		return;

	const FileInfo_t & tInfo = GetFile ( m_iCursorFile );

	CollapseRect ( tBottomRect, 1 );
	
	SYSTEMTIME tSysTime;
	FILETIME tLocalFileTime;
	FileTimeToLocalFileTime ( &tInfo.m_tData.ftLastWriteTime, &tLocalFileTime );
	FileTimeToSystemTime ( &tLocalFileTime, &tSysTime );
	GetTimeFormat ( LOCALE_SYSTEM_DEFAULT, TIME_NOSECONDS | TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT, &tSysTime, NULL, szTime, TIMEDATE_BUFFER_SIZE );
	GetDateFormat ( LOCALE_SYSTEM_DEFAULT, DATE_SHORTDATE, &tSysTime, NULL, szDate, TIMEDATE_BUFFER_SIZE );

	Str_c sText;

	// directory?
	if ( tInfo.m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		sText = Txt ( T_DIR_MARKER );
	else
	{
		FileSizeToString ( tInfo.m_tData.nFileSizeHigh, tInfo.m_tData.nFileSizeLow, szSize, false );
		sText = szSize;
	}

	// correct the rect in case of an icon
	RECT tStringRect = tBottomRect;
	if ( g_tConfig.pane_fileinfo_icon )
		tStringRect.left += GetSystemMetrics ( SM_CXSMICON ) + 1;

	RECT tRect;
	memset ( &tRect, 0, sizeof ( tRect ) );

	// time
	if ( g_tConfig.pane_fileinfo_time )
		DrawTextInRect ( hDC, tStringRect, szTime, g_tColors.pane_fileinfo_time );

	// date
	if ( g_tConfig.pane_fileinfo_date )
	{
		Str_c sTextToDraw = g_tConfig.pane_fileinfo_time ? Str_c ( szDate ) + L" " : szDate;
		DrawTextInRect ( hDC, tStringRect, sTextToDraw, g_tColors.pane_fileinfo_date );
	}

	// size
	sText = Str_c ( L" " ) + sText;
	if ( g_tConfig.pane_fileinfo_date || g_tConfig.pane_fileinfo_time )
		sText += L" ";

	DrawTextInRect ( hDC, tStringRect, sText, g_tColors.pane_fileinfo_size );

	int iSpaceLeft = tStringRect.right - tStringRect.left;

	// do we have any space left? :)
	if ( iSpaceLeft > 0 )
	{
		SetTextColor ( hDC, g_tColors.pane_font );

		int iFileNameLen = wcslen ( tInfo.m_tData.cFileName );
		memset ( &tRect, 0, sizeof ( tRect ) );
		DrawText ( hDC, tInfo.m_tData.cFileName, iFileNameLen, &tRect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX );

		if ( tRect.right - tRect.left <= iSpaceLeft )
			DrawText ( hDC, tInfo.m_tData.cFileName, iFileNameLen, &tStringRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOCLIP | DT_NOPREFIX ); 	// left-aligned filename
		else
		{
			// right-aligned & clipped filename
			tStringRect.right = tStringRect.left + iSpaceLeft;

			HGDIOBJ hOldPen = SelectObject ( hDC, g_tResources.m_hOverflowPen );
			MoveToEx ( hDC, tStringRect.left, tStringRect.top, NULL );
			LineTo ( hDC, tStringRect.left, tStringRect.bottom );
			SelectObject ( hDC, hOldPen );

			++tStringRect.left;
			DrawText ( hDC, tInfo.m_tData.cFileName, iFileNameLen, &tStringRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
		}
	}

	// update and draw a file icon if necessary
	if ( g_tConfig.pane_fileinfo_icon )
	{
		int iIconIndex = GetFileIconIndex ( m_iCursorFile );
		ImageList_Draw ( g_tResources.m_hSmallIconList, iIconIndex, hDC, tBottomRect.left, tBottomRect.top, ILD_TRANSPARENT );
	}
}


FindOnScreen_e FilePanel_c::FindFileOnScreen ( int & iResult, int iX, int iY, const RECT & tViewRect ) const
{
	Assert ( m_pFileView );

	iResult = -1;

	RECT tHeaderRect;
	GetHeaderViewRect ( tHeaderRect );

	if ( IsInRect ( iX, iY, tHeaderRect ) )
	{
		iResult = m_iFirstFile;
		return FIND_PANEL_TOP;
	}
	else
	{
		RECT tBottomRect;
		GetFileInfoRect ( tBottomRect );
		if ( IsInRect ( iX, iY, tBottomRect ) )
		{
			iResult = m_iFirstFile + m_pFileView->GetNumViewFiles () - 1;
			int nFiles = GetNumFiles ();
			if ( iResult > nFiles - 1 )
				iResult = nFiles - 1;

			return FIND_PANEL_BOTTOM;
		}
	}

	return m_pFileView->FindFileOnScreen ( iResult, iX, iY );
}


void FilePanel_c::GetFileViewRect ( RECT & tRect ) const
{
	tRect = m_tRect;

	if ( IsSliderVisible () )
	{
		if ( g_tConfig.pane_slider_left )
			tRect.left += g_tConfig.pane_slider_size_x;
		else
			tRect.right -= g_tConfig.pane_slider_size_x;
	}

	if ( m_tRect.top < m_tRect.bottom )
	{
		tRect.top		= m_tRect.top + GetHeaderViewHeight () + 1;
		tRect.bottom	= m_tRect.bottom - GetFileInfoTextHeight () - 2;
	}
}


void FilePanel_c::GetFileInfoRect ( RECT & tRect ) const
{
	tRect.left	 = m_tRect.left;
	tRect.right	 = m_tRect.right;
	tRect.top	 = m_tRect.bottom - GetFileInfoTextHeight () - 2;
	tRect.bottom = m_tRect.bottom;
}


int FilePanel_c::GetFileInfoTextHeight () const
{
	return g_tConfig.pane_fileinfo_icon ? Max ( GetSystemMetrics ( SM_CYSMICON ), g_tPanelFont.GetHeight () ) : g_tPanelFont.GetHeight ();
}


int FilePanel_c::GetHeaderViewHeight () const
{
	return Max ( Panel_c::GetHeaderViewHeight (), g_tResources.m_tBmpDirSize.cy, g_tResources.m_tBmpUpSize.cy );
}


void FilePanel_c::GetHeaderViewRect ( RECT & tRect ) const
{
	Panel_c::GetHeaderViewRect ( tRect );
	tRect.left = m_tDirBtnRect.right;
	tRect.right = m_tUpBtnRect.left;
}


void FilePanel_c::UpdateScrollInfo () const
{
	if ( IsSliderVisible () )
	{
		Assert ( m_pFileView );
		const int nNumFiles = GetNumFiles ();
		const int nViewFiles = 	m_pFileView->GetNumViewFiles (); 

		SCROLLINFO tScrollInfo;
		tScrollInfo.cbSize	= sizeof ( SCROLLINFO );
		tScrollInfo.fMask	= SIF_ALL;
		tScrollInfo.nMin	= 0;
		tScrollInfo.nMax	= nNumFiles <= nViewFiles ? 0 : nNumFiles - 1;
		tScrollInfo.nPage	= nViewFiles;
		tScrollInfo.nPos	= m_iFirstFile;
		tScrollInfo.nTrackPos= 0;
		SetScrollInfo ( m_hSlider, SB_CTL, &tScrollInfo, TRUE );

		if ( ! IsWindowVisible ( m_hSlider ) )
		{
			ShowWindow ( m_hSlider, SW_SHOW );
			InvalidateRect ( m_hSlider, NULL, TRUE );
		}
	}
	else
		ShowWindow ( m_hSlider, SW_HIDE );
}

void FilePanel_c::UpdateScrollPos () const
{
	SCROLLINFO tScrollInfo;
	tScrollInfo.cbSize	= sizeof ( SCROLLINFO );
	tScrollInfo.fMask	= SIF_POS;
	tScrollInfo.nPos	= m_iFirstFile;
	tScrollInfo.nTrackPos= 0;
	SetScrollInfo ( m_hSlider, SB_CTL, &tScrollInfo, TRUE );
}

void FilePanel_c::SetViewMode ( FileViewMode_e eMode )
{
	m_eViewMode = eMode;
	m_pFileView = m_dFileViews [eMode];

	Assert ( m_pFileView );
	RECT tRect;
	GetFileViewRect ( tRect );
	m_pFileView->SetViewRect ( tRect );
	m_pFileView->SetPanel ( this );

	bool bForceMaximized = m_bMaximized && g_tConfig.pane_maximized_force;
	if ( bForceMaximized )
		g_tConfig.pane_maximized_view = eMode;
	else
		if ( m_pConfigViewMode )
			*m_pConfigViewMode = eMode;
}

HWND FilePanel_c::GetSlider () const
{
	return m_hSlider;
}


int FilePanel_c::GetNumFiles () const
{
	return FileSorter_c::GetNumFiles ();
}


const FileInfo_t & FilePanel_c::GetFile ( int iFile ) const
{
	return FileSorter_c::GetFile ( iFile );
}


void FilePanel_c::MarkFile ( const Str_c & sFile, bool bMark )
{
	FileSorter_c::MarkFile ( sFile, bMark );
}


void FilePanel_c::MarkAllFiles ( bool bMark, bool bForceDirs )
{
	for ( int i = 0; i < GetNumFiles (); ++i )
	{
		const FileInfo_t & tInfo = GetFile ( i );
		bool bDoFile = true;
		if ( ! bForceDirs )
			bDoFile = ! ( tInfo.m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) || g_tConfig.filter_include_folders;

		if (  bDoFile )
			MarkFile ( i, bMark );
	}

	InvalidateHeader ();
}


void FilePanel_c::MarkFilter ( const Str_c & sFilter, bool bMark )
{
	FileSorter_c::MarkFilter ( sFilter, bMark );
}


void FilePanel_c::InvertMarkedFiles ()
{
	for ( int i = 0; i < GetNumFiles (); ++i )
	{
		const FileInfo_t & tInfo = GetFile ( i );
		bool bDir = !! ( tInfo.m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );
		if ( ! bDir || g_tConfig.filter_include_folders )
			MarkFile ( i, ! ( tInfo.m_uFlags & FLAG_MARKED ) );
	}

	InvalidateHeader ();
}


void FilePanel_c::StepToPrevDir ()
{
	Str_c sPrevDir;
	if ( FileSorter_c::StepToPrevDir ( &sPrevDir ) )
	{
		Refresh ();
		SetCursorAtFile ( sPrevDir );
	}
}

void FilePanel_c::ShowBookmarks ()
{
	POINT tShow;
	tShow.x = m_tDirBtnRect.left;
	tShow.y = m_tDirBtnRect.bottom;

	ClientToScreen ( m_hWnd, &tShow );

	Str_c sPath;
	if ( ShowDriveMenu ( tShow.x, tShow.y, m_hWnd, sPath ) )
		ProcessDriveMenuResults ( sPath );
}

const Str_c & FilePanel_c::GetDirectory () const
{
	return FileSorter_c::GetDirectory ();
}

int FilePanel_c::GetNumMarked () const
{
	return FileSorter_c::GetNumMarked ();
}


ULARGE_INTEGER FilePanel_c::GetMarkedSize () const
{
	return FileSorter_c::GetMarkedSize ();
}

int FilePanel_c::GetFirstFile () const
{
	return m_iFirstFile;
}

bool FilePanel_c::IsSliderVisible () const
{
	Assert ( m_pFileView );
	return m_pFileView->GetNumViewFiles () < GetNumFiles ();
}

int	FilePanel_c::GetCursorFile () const
{
	return m_iCursorFile;
}

void FilePanel_c::SetCursorFile ( int iCursor )
{
	m_iCursorFile = iCursor;
	FMCursorChange_Event ();
}

bool FilePanel_c::CanUseFile ( int iFile ) const
{
	if ( iFile == -1 || iFile >= GetNumFiles () )
		return false;

	const FileInfo_t & tInfo = GetFile ( iFile );

	return ! ( tInfo.m_uFlags & FLAG_PREV_DIR );
}

void FilePanel_c::Refresh ()
{
	FileSorter_c::Refresh ();
}


void FilePanel_c::SoftRefresh ()
{
	FileSorter_c::SoftRefresh ();
}


bool FilePanel_c::GenerateFileList ( FileList_t & tList )
{
	tList.m_sRootDir = GetDirectory ();
	tList.m_uSize = GetMarkedSize ();

	if ( GetNumMarked () && ! m_bSingleFileOperation )
	{
		for ( int i = 0; i < GetNumFiles (); ++i )
		{
			const FileInfo_t & tInfo = GetFile ( i );
			if ( tInfo.m_uFlags & FLAG_MARKED )
				tList.m_dFiles.Add ( &tInfo );
		}
	}
	else
	{
		if ( m_iCursorFile == -1 )
			return false;

		const FileInfo_t & tInfo = GetFile ( m_iCursorFile );
		if ( tInfo.m_uFlags & FLAG_PREV_DIR )
			return false;

		tList.m_dFiles.Add ( &tInfo );
	}

	return true;
}


void FilePanel_c::SetDirectory ( const wchar_t * szDir )
{
	FileSorter_c::SetDirectory ( szDir );
}


void FilePanel_c::SetCursorAtFile ( const wchar_t * szFile )
{
	for ( int i = 0; i < GetNumFiles (); ++i )
	{
		const FileInfo_t & tFileInfo = GetFile ( i );
		if ( ! wcscmp ( tFileInfo.m_tData.cFileName, szFile ) )
		{
			SetCursorFile ( i );
			break;
		}
	}

	if ( m_iCursorFile != -1 )
	{
		Assert ( m_pFileView );
		int nViewFiles = m_pFileView->GetNumViewFiles ();
		if ( m_iCursorFile >= m_iFirstFile + nViewFiles )
			m_iFirstFile = m_iCursorFile - nViewFiles + 1;
	}

	m_iFirstFile = ClampFirstFile ( m_iFirstFile );
	SetCursorFile ( ClampCursorFile ( m_iCursorFile ) );

	UpdateScrollPos ();
}

void FilePanel_c::SetVisibility ( bool bShowHidden, bool bShowSystem, bool bShowROM )
{
	FileSorter_c::SetVisibility ( bShowHidden, bShowSystem, bShowROM );
}

void FilePanel_c::MarkFilesTo ( int iFile, bool bClearFirst )
{
	Assert ( m_iFirstMarkFile != -1 && iFile != -1 );

	if ( m_iLastMarkFile == -1 )
	{
		// this is the first time we mark files
		for ( int i = Min ( iFile, m_iFirstMarkFile ); i <= Max ( iFile, m_iFirstMarkFile ); ++i )
			MarkFile ( i, true );
	}
	else
	{
		if ( m_iLastMarkFile != iFile )
		{
			int iFrom = Min ( m_iLastMarkFile, iFile );
			int iTo = Max ( m_iLastMarkFile, iFile );
			bool bMark;
			bool bNeedReverse = true;
			if ( iFile < m_iFirstMarkFile && m_iFirstMarkFile < m_iLastMarkFile )
				bMark = true;
			else
				if ( m_iLastMarkFile < m_iFirstMarkFile && m_iFirstMarkFile < iFile )
					bMark = false;
				else
				{
					bMark = fabs ( iFile - m_iFirstMarkFile ) > fabs ( m_iLastMarkFile - m_iFirstMarkFile );
					bNeedReverse = false;
				}

	 
			// this is NOT the first time we mark files
			for ( int i = iFrom; i <= iTo; ++i )
			{
				if ( !bMark && i == iFile )
					continue;

				if ( i == m_iFirstMarkFile )
				{
					if ( bNeedReverse )
						bMark = !bMark;
					else
						MarkFile ( i, bMark );
				}
				else
					MarkFile ( i, bMark );
			}
		}
	}

	bool bClearFirstFile = iFile == m_iFirstMarkFile && bClearFirst;
	if ( bClearFirstFile )
		MarkFile ( iFile, false );

	if ( m_iLastMarkFile != iFile || bClearFirstFile )
	{
		m_iLastMarkFile = iFile;
		InvalidateHeader ();
	}
}

void FilePanel_c::Config_Path ( Str_c * pPath )
{
	m_pConfigPath = pPath;
}

void FilePanel_c::Config_SortMode ( int * pMode )
{
	m_pConfigSortMode = pMode;
}


void FilePanel_c::Config_SortReverse ( bool * pReverse )
{
	m_pConfigSortReverse = pReverse;
}


void FilePanel_c::Config_ViewMode ( int * pMode )
{
	m_pConfigViewMode = pMode;
}


void FilePanel_c::Event_Refresh ()
{
	FileSorter_c::Event_Refresh ();

	SetCursorFile ( ClampCursorFile ( m_iCursorFile ) );
	m_iFirstFile = ClampFirstFile ( m_iFirstFile );

	InvalidateHeader ();
	InvalidateFileInfo ();

	InvalidateFilesArea ();
	UpdateScrollInfo ();

	FMFolderChange_Event ();

	m_bRefreshQueued = false;
	m_bRefreshAnyway = false;
	m_tTimeSinceRefresh = 0.0f;
}


void FilePanel_c::Event_Directory ()
{
	SetCursorFile ( 0 );
	m_iFirstFile	= 0;

	if ( m_pConfigPath )
		*m_pConfigPath = GetDirectory ();
}


void FilePanel_c::InvalidateFileInfo () const
{
	RECT tRect;
	GetFileInfoRect ( tRect );
	++tRect.top;
	InvalidateRect ( m_hWnd, & tRect, TRUE );
}


void FilePanel_c::InvalidateFiles () const
{
	Assert ( m_pFileView );
	RECT tColumnRect;
	for ( int i = 0; i < m_pFileView->GetNumColumns (); ++i )
	{
		m_pFileView->GetColumnViewRect ( tColumnRect, i );
		++tColumnRect.right;
		--tColumnRect.bottom;
		InvalidateRect ( m_hWnd, &tColumnRect, TRUE );
	}
}


void FilePanel_c::InvalidateFilesArea () const
{
	RECT tRect;
	GetFileViewRect ( tRect );
	InvalidateRect ( m_hWnd, &tRect, FALSE );
}

void FilePanel_c::CreateDrawObjects ()
{
	Panel_c::CreateDrawObjects ();

	CreateSlider ();
	CreateMenus ();

	m_hBtnDirs = CreateButton ();
	m_hBtnUp = CreateButton ();
}


void FilePanel_c::DestroyDrawObjects ()
{
	Panel_c::DestroyDrawObjects ();

	DestroyWindow ( m_hSlider );
	DestroyMenu ( m_hSortModeMenu );
	DestroyMenu ( m_hViewModeMenu );
	DestroyMenu ( m_hHeaderMenu );
	DestroyWindow ( m_hBtnDirs );
	DestroyWindow ( m_hBtnUp );
}

/*
WNDPROC g_pSliderProc = NULL;

static LRESULT CALLBACK SliderProc ( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if ( uMsg == WM_PAINT )
	{
		PAINTSTRUCT ps; 
		HDC hdc; 

		hdc = BeginPaint ( hWnd, &ps ); 
		EndPaint ( hWnd, &ps ); 

		return TRUE;
	}

	return CallWindowProc ( (WNDPROC)g_pSliderProc, hWnd, uMsg, wParam, lParam);	
}
*/

void FilePanel_c::CreateSlider ()
{
	m_hSlider = CreateWindow ( L"SCROLLBAR", L"", WS_VISIBLE | WS_CHILD | SBS_VERT, CW_USEDEFAULT, CW_USEDEFAULT, g_tConfig.pane_slider_size_x, CW_USEDEFAULT,
		m_hWnd, NULL, g_hAppInstance, NULL );

//	g_pSliderProc = (WNDPROC) SetWindowLong ( m_hSlider, GWL_WNDPROC, (LONG) SliderProc );
}


void FilePanel_c::CreateMenus ()
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
	AppendMenu ( m_hSortModeMenu, MF_STRING,	IDM_HEADER_SORT_GROUP,			Txt ( T_MNU_SORT_GROUP ) );
	AppendMenu ( m_hSortModeMenu, MF_SEPARATOR,	IDM_HEADER_SORT_SEPARATOR,	NULL );
	AppendMenu ( m_hSortModeMenu, MF_STRING,	IDM_HEADER_SORT_REVERSE,		Txt ( T_MNU_SORT_REVERSE ) );
	m_hViewModeMenu = CreatePopupMenu ();
	AppendMenu ( m_hViewModeMenu, MF_STRING,	IDM_HEADER_VIEW_ONE,			Txt ( T_MNU_VIEW_1C ) );
	AppendMenu ( m_hViewModeMenu, MF_STRING,	IDM_HEADER_VIEW_MID,			Txt ( T_MNU_VIEW_2C ) );
	AppendMenu ( m_hViewModeMenu, MF_STRING,	IDM_HEADER_VIEW_SHORT,			Txt ( T_MNU_VIEW_3C ) );
	AppendMenu ( m_hViewModeMenu, MF_STRING,	IDM_HEADER_VIEW_FULL,			Txt ( T_MNU_VIEW_D1 ) );
	AppendMenu ( m_hViewModeMenu, MF_STRING,	IDM_HEADER_VIEW_FULL_DATE,		Txt ( T_MNU_VIEW_D2 ) );
	AppendMenu ( m_hViewModeMenu, MF_STRING,	IDM_HEADER_VIEW_FULL_TIME,		Txt ( T_MNU_VIEW_D3 ) );
	m_hHeaderMenu = CreatePopupMenu ();
	AppendMenu ( m_hHeaderMenu, MF_STRING, 		IDM_HEADER_OPEN_OPPOSITE, 		Txt ( T_MNU_OPEN_2ND_PANE_FLD ) );
	AppendMenu ( m_hHeaderMenu, MF_STRING, 		IDM_HEADER_OPEN, 				Txt ( T_MNU_OPEN_DOT ) );
	AppendMenu ( m_hHeaderMenu, MF_SEPARATOR,	NULL, L"" );
	AppendMenu ( m_hHeaderMenu, MF_POPUP | MF_STRING, (UINT) m_hSortModeMenu, 	Txt ( T_MNU_HEADER_SORT ) );
	AppendMenu ( m_hHeaderMenu, MF_POPUP | MF_STRING, (UINT) m_hViewModeMenu, 	Txt ( T_MNU_HEADER_VIEW ) );
}


void FilePanel_c::ShowHeaderMenu ( int iX, int iY )
{
	if ( IsAnyPaneInMenu () || menu::IsInMenu () )
		return;

	Assert ( m_hHeaderMenu );
	POINT tShow;
	tShow.x = iX;
	tShow.y = iY;

	ClientToScreen ( m_hWnd, &tShow );

	// update sort mode menu
	CheckMenuRadioItem ( m_hSortModeMenu, IDM_HEADER_SORT_UNSORTED, IDM_HEADER_SORT_TIME_WRITE, GetSortMode () + IDM_HEADER_SORT_UNSORTED, MF_BYCOMMAND );
	CheckMenuItem ( m_hSortModeMenu, IDM_HEADER_SORT_REVERSE, MF_BYCOMMAND | ( GetSortReverse () ? MF_CHECKED : MF_UNCHECKED ) );

	// update view mode menu
	bool bForceMaximized = m_bMaximized && g_tConfig.pane_maximized_force;
	int iModeCheck = m_bMaximized && g_tConfig.pane_maximized_force ? g_tConfig.pane_maximized_view : m_eViewMode;
	CheckMenuRadioItem ( m_hViewModeMenu, IDM_HEADER_VIEW_ONE, IDM_HEADER_VIEW_FULL_TIME, iModeCheck + IDM_HEADER_VIEW_ONE, MF_BYCOMMAND );

	int iRes = TrackPopupMenu ( m_hHeaderMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, tShow.x, tShow.y, 0, m_hWnd, NULL );
	if ( !iRes || iRes == IDM_HEADER_SORT_SEPARATOR )
		return;

	SortMode_e eOldSortMode = GetSortMode ();
	SortMode_e eNewSortMode = eOldSortMode;

	bool bOldReverse = GetSortReverse ();
	bool bNewReverse = bOldReverse;

	FileViewMode_e eOldView = m_eViewMode;
	FileViewMode_e eNewView	= eOldView;

	switch ( iRes )
	{
	case IDM_HEADER_OPEN_OPPOSITE:
		SetDirectory ( FMGetPassivePanel ()->GetDirectory () );
		Refresh ();
		break;
	case IDM_HEADER_OPEN:
		FMOpenFolder ();
		break;
	case IDM_HEADER_SORT_UNSORTED:
		eNewSortMode = SORT_UNSORTED;
		break;
	case IDM_HEADER_SORT_NAME:
		eNewSortMode = SORT_NAME;
		break;
	case IDM_HEADER_SORT_EXTENSION:
		eNewSortMode = SORT_EXTENSION;
		break;
	case IDM_HEADER_SORT_SIZE:
		eNewSortMode = SORT_SIZE;
		break;
	case IDM_HEADER_SORT_TIME_CREATE:
		eNewSortMode = SORT_TIME_CREATE;
		break;
	case IDM_HEADER_SORT_TIME_ACCESS:
		eNewSortMode = SORT_TIME_ACCESS;
		break;
	case IDM_HEADER_SORT_TIME_WRITE:
		eNewSortMode = SORT_TIME_WRITE;
		break;
	case IDM_HEADER_SORT_GROUP:
		eNewSortMode = SORT_GROUP;
		break;
	case IDM_HEADER_SORT_REVERSE:
		bNewReverse = ! bOldReverse;
		break;
	case IDM_HEADER_VIEW_ONE:
		eNewView = FILE_VIEW_1C;
		break;
	case IDM_HEADER_VIEW_SHORT:
		eNewView = FILE_VIEW_3C;
		break;
	case IDM_HEADER_VIEW_MID:
		eNewView = FILE_VIEW_2C;
		break;
	case IDM_HEADER_VIEW_FULL:
		eNewView = FILE_VIEW_FULL;
		break;
	case IDM_HEADER_VIEW_FULL_DATE:
		eNewView = FILE_VIEW_FULL_DATE;
		break;
	case IDM_HEADER_VIEW_FULL_TIME:
		eNewView = FILE_VIEW_FULL_TIME;
		break;
	}

	if ( bNewReverse != bOldReverse || eNewSortMode != eOldSortMode )
	{
		SetSortMode ( eNewSortMode, bNewReverse );
		if ( m_pConfigSortMode )
			*m_pConfigSortMode = eNewSortMode;

		if ( m_pConfigSortReverse )
			*m_pConfigSortReverse = bNewReverse;

		Refresh ();
	}

	if ( ( bForceMaximized && eNewView != g_tConfig.pane_maximized_view ) || ( ! bForceMaximized && eNewView != eOldView ) )
	{
		SetViewMode ( eNewView );
		InvalidateRect ( m_hWnd, NULL, TRUE );
		UpdateScrollInfo ();
	}
}


void FilePanel_c::ShowFileMenu ( int iFile, int iX, int iY )
{
	if ( iFile == -1 )
		return;

	if ( GetFile ( iFile ).m_uFlags & FLAG_PREV_DIR )
		return;

	if ( IsAnyPaneInMenu () || menu::IsInMenu () )
		return;

	ChangeSelectedFile ( iFile );

	POINT tShow = {iX, iY};
	ClientToScreen ( m_hWnd, &tShow );

	m_bSingleFileOperation = true;

	bool bDir = false;
	bool bMarked = false;

	Str_c sFileName;

	const FileInfo_t & tInfo = GetFile ( iFile );
	if ( tInfo.m_uFlags & FLAG_MARKED )
	{
		m_bSingleFileOperation = false;
		bMarked = true;
	}

	if ( tInfo.m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		bDir = true;

	bool bOneFile = GetNumMarked () <= 1 || m_bSingleFileOperation;

	//////////////////////////////////////////////////////////////////////////
	HMENU hMenu = CreatePopupMenu ();
	AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_PROPERTIES, 	Txt ( T_MNU_PROPS ) );

	const OpenWithArray_t * pArray = NULL;

	if ( bOneFile && ! bDir )
	{
		HMENU hSubMenu = CreatePopupMenu ();

		AppendMenu ( hSubMenu, MF_STRING, IDM_FILE_MENU_OPEN, Txt ( T_MNU_OPEN ) );
		AppendMenu ( hSubMenu, MF_STRING, IDM_FILE_MENU_OPENWITH, Txt ( T_MNU_OPEN_WITH ) );

		if ( Str_c ( tInfo.m_tData.cFileName ).Ends ( L".exe" ) )
			AppendMenu ( hSubMenu, MF_STRING, IDM_FILE_MENU_RUNPARAMS, Txt ( T_MNU_RUN_DOT ) );

		pArray = g_tRecent.GetOpenWithFor ( GetExt ( tInfo.m_tData.cFileName ) );
		if ( pArray )
		{
			AppendMenu ( hSubMenu, MF_SEPARATOR, IDM_FILE_MENU_SEPARATOR, L"" );
			const RecentOpenWith_t * pOpenWith = NULL;
			for ( int i = pArray->Length () - 1; i >= 0 ; --i )
				AppendMenu ( hSubMenu, MF_STRING, IDM_FILE_MENU_OPEN_0 + i, (*pArray) [i].m_sName );
		}

		AppendMenu ( hMenu, MF_POPUP | MF_STRING, (UINT) hSubMenu, Txt ( T_MNU_OPEN_DOT ) );
	}

	if ( bOneFile && bDir )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_OPPOSITE, 	Txt ( T_MNU_OPEN_IN_2ND ) );

	AppendMenu ( hMenu, MF_SEPARATOR, IDM_FILE_MENU_SEPARATOR,	L"" );

	if ( bOneFile && ! bDir )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_VIEW, 		Txt ( T_MNU_VIEW ) );

	AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_COPYMOVE, 		Txt ( T_MNU_COPYMOVE ) );
	if ( bOneFile )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_RENAME, 	Txt ( T_MNU_RENAME ) );

	AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_DELETE, 		Txt ( T_MNU_DELETE ) );

	if ( bOneFile )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_SHORTCUT, 	Txt ( T_MNU_CREATE_SHORTCUT ) );

	AppendMenu ( hMenu, MF_SEPARATOR, IDM_FILE_MENU_SEPARATOR,	L"" );
	AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_COPYCLIP,		Txt ( T_MNU_COPYCLIP ) );
	AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_CUTCLIP,		Txt ( T_MNU_CUTCLIP ) );

	if ( ! clipboard::IsEmpty () )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_PASTECLIP,	Txt ( clipboard::IsMoveMode () ? T_MNU_PASTEMOVE : T_MNU_PASTECOPY ) );

	AppendMenu ( hMenu, MF_SEPARATOR, IDM_FILE_MENU_SEPARATOR,	L"" );

	HMENU hSelMenu = CreatePopupMenu ();
	if ( bMarked )
	{
		AppendMenu ( hSelMenu, MF_STRING, IDM_FILE_MENU_CLEAR, 		Txt ( T_MNU_CLEAR ) );
		AppendMenu ( hSelMenu, MF_STRING, IDM_FILE_MENU_CLEAR_ALL,	Txt ( T_MNU_CLEAR_SELECTION ) );
	}
	else
		AppendMenu ( hSelMenu, MF_STRING, IDM_FILE_MENU_MARK, 		Txt ( T_MNU_SELECT ) );

	AppendMenu ( hSelMenu, MF_STRING, IDM_FILE_MENU_MARK_ALL,		Txt ( T_MNU_SELECT_ALL ) );

	AppendMenu ( hMenu, MF_POPUP | MF_STRING, (UINT) hSelMenu,		Txt ( T_MNU_SELECTION ) );

	AppendMenu ( hMenu, MF_POPUP | MF_STRING, (UINT) menu::CreateSendMenu (), Txt ( T_MNU_SEND ) );

	HMENU hSubMenu = CreatePopupMenu ();
	AppendMenu ( hSubMenu, MF_STRING, IDM_FILE_MENU_ENCRYPT,	Txt ( T_MNU_ENCRYPT ) );
	AppendMenu ( hSubMenu, MF_STRING, IDM_FILE_MENU_DECRYPT,	Txt ( T_MNU_DECRYPT ) );

	AppendMenu ( hMenu, MF_POPUP | MF_STRING, (UINT) hSubMenu,	Txt ( T_MNU_ENCRYPTION ) );

	if ( bOneFile )
		AppendMenu ( hMenu, MF_STRING, IDM_FILE_MENU_BOOKMARK, 	Txt ( T_MNU_ADD_BM ) );

	//////////////////////////////////////////////////////////////////////////

	int iRes = TrackPopupMenu ( hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, tShow.x, tShow.y, 0, m_hWnd, NULL );
	DestroyMenu ( hMenu );
	if ( !iRes || iRes == IDM_FILE_MENU_SEPARATOR )
	{
		m_bSingleFileOperation = false;
		return;
	}

	switch ( iRes )
	{
		case IDM_FILE_MENU_OPEN:
			ExecuteFile ( tInfo );
			break;
		case IDM_FILE_MENU_OPENWITH:
			FMOpenWith ();
			break;
		case IDM_FILE_MENU_RUNPARAMS:
			FMRunParams ();
			break;
		case IDM_FILE_MENU_VIEW:
			FMViewFile ();
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
		case IDM_FILE_MENU_SEND_IR:
			FMSendIR ();
			break;
		case IDM_FILE_MENU_SEND_BT:
			FMSendBT ();
			break;
		case IDM_FILE_MENU_ENCRYPT:
			FMEncryptFiles ( true );
			break;
		case IDM_FILE_MENU_DECRYPT:
			FMEncryptFiles ( false );
			break;
		case IDM_FILE_MENU_BOOKMARK:
			ShowAddBMDialog ( GetDirectory () + tInfo.m_tData.cFileName );
			break;
		case IDM_FILE_MENU_MARK:
			if ( MarkFile ( m_iCursorFile, true ) )
				InvalidateHeader ();
			break;
		case IDM_FILE_MENU_MARK_ALL:
			MarkAllFiles ( true );
			break;
		case IDM_FILE_MENU_CLEAR:
			if ( MarkFile ( m_iCursorFile, false ) )
				InvalidateHeader ();
			break;
		case IDM_FILE_MENU_CLEAR_ALL:
			MarkAllFiles ( false );
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

				Str_c sFileName = GetDirectory () + tInfo.m_tData.cFileName;
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
			break;
	}

	m_bSingleFileOperation = false;
}


void FilePanel_c::ShowEmptyMenu ( int iX, int iY )
{
	if ( IsAnyPaneInMenu () || menu::IsInMenu () )
		return;

	POINT tShow = {iX, iY};
	ClientToScreen ( m_hWnd, &tShow );

	bool bMarked = GetNumMarked () > 0;
	bool bHaveFiles = GetNumFiles () > 0 && ! ( GetNumFiles () == 1 && ( GetFile ( 0 ).m_uFlags & FLAG_PREV_DIR ) );

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

	if ( ! clipboard::IsEmpty () )
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
		MarkAllFiles ( false );
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