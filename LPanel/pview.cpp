#include "pch.h"

#include "LPanel/pview.h"
#include "LPanel/presources.h"
#include "LCore/cfile.h"
#include "LFile/fmisc.h"
#include "LFile/fcolorgroups.h"
#include "LSettings/sconfig.h"
#include "LSettings/slocal.h"
#include "LPanel/pfile.h"
#include "LCore/clog.h"

#include "shellapi.h"

///////////////////////////////////////////////////////////////////////////////////////////
// the base file view
FileView_c::FileView_c ( int nColumns, const bool * pDrawFileIcon, const bool * pSeparateExt )
	: m_pDrawFileIcon	( pDrawFileIcon )
	, m_pSeparateExt	( pSeparateExt )
	, m_nColumns		( nColumns )
	, m_pPanel			( NULL )
{
	Assert ( m_pDrawFileIcon && m_pSeparateExt );
	m_tRect.left = m_tRect.right = m_tRect.top = m_tRect.bottom = 0;
}


void FileView_c::SetViewRect ( const RECT & tRect )
{
	m_tRect = tRect;
}


void FileView_c::SetPanel ( FilePanel_c * pPanel )
{
	m_pPanel = pPanel;
}


int	FileView_c::GetTextHeight () const
{
	int iIconHeight = *m_pDrawFileIcon ? GetSystemMetrics ( SM_CYSMICON ) : 0;
	return Max ( g_tPanelFont.GetHeight (), iIconHeight );
}

bool FileView_c::GetMarkerFlag () const
{
	return g_tConfig.pane_dir_marker;
}


int	FileView_c::GetNumColumnFiles () const
{
	return ( m_tRect.bottom - m_tRect.top - SPACE_BETWEEN_LINES - 1 ) / ( GetTextHeight () + SPACE_BETWEEN_LINES );
}


int FileView_c::GetNumColumns () const
{
	return m_nColumns;
}


void FileView_c::DrawCursor ( HDC hDC ) const
{
	Assert ( m_pPanel );

	RECT tRect;
	if ( ! GetFileRect ( m_pPanel->GetCursorFile (), tRect ) )
		return;

	CollapseRect ( tRect, -1 );

	if ( g_tConfig.pane_cursor_bar )
	{
		++tRect.bottom;
		++tRect.right;
		FillRect ( hDC, &tRect, g_tResources.GetCursorBrush ( m_pPanel->IsActive () ) );
	}
	else
	{
		HGDIOBJ hOldPen = SelectObject ( hDC, g_tResources.GetCursorPen ( m_pPanel->IsActive () ) );

		MoveToEx ( hDC, tRect.left + 1, tRect.top, NULL );
		LineTo ( hDC, tRect.right, tRect.top );

		MoveToEx ( hDC, tRect.right, tRect.top + 1, NULL );
		LineTo ( hDC, tRect.right, tRect.bottom );

		MoveToEx ( hDC, tRect.right - 1, tRect.bottom, NULL );
		LineTo ( hDC, tRect.left, tRect.bottom );

		MoveToEx ( hDC, tRect.left, tRect.bottom - 1, NULL );
		LineTo ( hDC, tRect.left, tRect.top );
		SelectObject ( hDC, hOldPen );
	}
}


void FileView_c::InvalidateCursor ( HWND hWnd, BOOL bErase ) const
{
	Assert ( m_pPanel );

	int iCursorFile = m_pPanel->GetCursorFile ();
	if ( iCursorFile < 0 )
		return;

	RECT tCR;
	GetFileRect ( iCursorFile, tCR );
	CollapseRect ( tCR, -1 );

	if ( g_tConfig.pane_cursor_bar )
	{
		++tCR.bottom;
		++tCR.right;
		InvalidateRect ( hWnd, &tCR, bErase );
	}
	else
	{
		RECT tInvRect;
		tInvRect.top	= tCR.top;
		tInvRect.bottom	= tCR.top + 1;
		tInvRect.left	= tCR.left;
		tInvRect.right	= tCR.right;
		InvalidateRect ( hWnd, &tInvRect, bErase );

		tInvRect.top	= tCR.top;
		tInvRect.bottom	= tCR.bottom;
		tInvRect.left	= tCR.right;
		tInvRect.right	= tCR.right + 1;
		InvalidateRect ( hWnd, &tInvRect, bErase );

		tInvRect.top	= tCR.bottom;
		tInvRect.bottom	= tCR.bottom + 1;
		tInvRect.left	= tCR.left;
		tInvRect.right	= tCR.right;
		InvalidateRect ( hWnd, &tInvRect, bErase );

		tInvRect.top	= tCR.top;
		tInvRect.bottom	= tCR.bottom;
		tInvRect.left	= tCR.left;
		tInvRect.right	= tCR.left + 1;
		InvalidateRect ( hWnd, &tInvRect, bErase );
	}
}


int FileView_c::DrawFileNames ( HDC hDC, int iStartFile, const RECT & tDrawRect )
{
	Assert ( m_pPanel );
	const int iIconWidth = GetSystemMetrics ( SM_CXSMICON );
	const int iIconHeight = GetSystemMetrics ( SM_CYSMICON );

	int iCurFile = iStartFile;
	int nFiles = m_pPanel->GetNumFiles ();
	int iSymbolHeight = GetTextHeight ();
	int iCursorFile = m_pPanel->GetCursorFile ();

	RECT tCalcRect;
	RECT tStringRect;

	bool bDrawDirMarker = GetMarkerFlag ();

	static const Str_c sDirMarker = Txt ( T_DIR_RIGHT );

	HGDIOBJ hOldPen = SelectObject ( hDC, g_tResources.m_hOverflowPen );

	static wchar_t szName [MAX_PATH];

	int nColumnFiles = GetNumColumnFiles();
	for ( iCurFile = iStartFile; iCurFile < nFiles && iCurFile - iStartFile < nColumnFiles; ++iCurFile )
	{
		const FileInfo_t & tInfo = m_pPanel->GetFile ( iCurFile );
		const wchar_t * szExt = L"";

		if ( tInfo.m_uFlags	& FLAG_PREV_DIR )
			wcscpy ( szName, L".." );
		else
			szExt = GetNameExtFast ( tInfo.m_tData.cFileName, szName );

		int iLen = wcslen ( tInfo.m_tData.cFileName );
		int iNameLen = wcslen ( szName );
		int iExtLen = wcslen ( szExt );

		// 1 for space between lines and 1 for space before 1st line
		tStringRect.top		= tDrawRect.top + ( iCurFile - iStartFile ) * ( iSymbolHeight + SPACE_BETWEEN_LINES ) + SPACE_BETWEEN_LINES;
		tStringRect.bottom	= tStringRect.top + iSymbolHeight;
		tStringRect.left	= tDrawRect.left + 1;
		tStringRect.right	= tDrawRect.right - 1;

		// draw icon
		if ( *m_pDrawFileIcon && tStringRect.right - tStringRect.left >= iIconWidth )
		{
			int iIconIndex = m_pPanel->GetFileIconIndex ( iCurFile );
			if ( iIconIndex != -1 )
			{
				int iIconY = ( tStringRect.bottom - tStringRect.top - iIconHeight ) / 2 + tStringRect.top;
				ImageList_Draw ( g_tResources.m_hSmallIconList, iIconIndex, hDC, tStringRect.left, iIconY, ILD_TRANSPARENT  );
			}
			tStringRect.left += iIconWidth + 1;
		}

		SetTextColor ( hDC, CGGetFileColor ( tInfo, iCurFile == iCursorFile ) );

		bool bDirectory = !! ( tInfo.m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );
		bool bDrawClipLine = false;

		// calc the filename's precise width
		memset ( &tCalcRect, 0, sizeof ( tCalcRect ) );
		DrawText ( hDC, tInfo.m_tData.cFileName, iLen, &tCalcRect, DT_NOCLIP | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX );

		int iNameWidth = tCalcRect.right - tCalcRect.left;
		if ( iNameWidth > tStringRect.right - tStringRect.left )
			bDrawClipLine = true;

		if ( bDrawClipLine )
		{
			if ( g_tConfig.pane_long_cut == 0 && ! bDirectory )
			{
				// draw the ext aligned right
				DrawText ( hDC, szExt, iExtLen, &tStringRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );

				memset ( &tCalcRect, 0, sizeof ( tCalcRect ) );
				DrawText ( hDC, szExt, iExtLen, &tCalcRect, DT_NOCLIP | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX );

				tStringRect.right -= tCalcRect.right - tCalcRect.left + 3;

				if ( tStringRect.right > tStringRect.left )
					DrawText ( hDC, szName, iNameLen, &tStringRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
			}
			else
			{
				// draw a clipped whole name
				if ( bDirectory && bDrawDirMarker )
				{
					memset ( &tCalcRect, 0, sizeof ( tCalcRect ) );
					DrawText ( hDC, sDirMarker, sDirMarker.Length (), &tCalcRect, DT_NOCLIP | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX );

					DrawText ( hDC, sDirMarker, sDirMarker.Length (), &tStringRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
					tStringRect.right -= tCalcRect.right - tCalcRect.left + 2;
				}

				if ( tStringRect.right > tStringRect.left )
					DrawText ( hDC, tInfo.m_tData.cFileName, iLen, &tStringRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
			}

			if ( tStringRect.right > tStringRect.left )
			{
				MoveToEx ( hDC, tStringRect.right + 1, tStringRect.top, NULL );
				LineTo ( hDC, tStringRect.right + 1, tStringRect.bottom );
			}
		}
		else
		{
			if ( !*m_pSeparateExt || bDirectory )
			{
				if ( bDirectory && bDrawDirMarker )
				{
					memset ( &tCalcRect, 0, sizeof ( tCalcRect ) );
					DrawText ( hDC, sDirMarker, sDirMarker.Length (), &tCalcRect, DT_NOCLIP | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX );

					DrawText ( hDC, sDirMarker, sDirMarker.Length (), &tStringRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
					tStringRect.right -= tCalcRect.right - tCalcRect.left + 2;
				}

				bool bClipNow = tStringRect.right - tStringRect.left < iNameWidth;

				if ( tStringRect.right > tStringRect.left )
				{
					DrawText ( hDC, tInfo.m_tData.cFileName, iLen, &tStringRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | ( bClipNow ? 0 : DT_NOCLIP ) );
					if ( bClipNow )
					{
						MoveToEx ( hDC, tStringRect.right + 1, tStringRect.top, NULL );
						LineTo ( hDC, tStringRect.right + 1, tStringRect.bottom );
					}
				}
			}
			else
			{
				// draw the ext aligned right
				DrawText ( hDC, szExt, iExtLen, &tStringRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );

				memset ( &tCalcRect, 0, sizeof ( tCalcRect ) );
				DrawText ( hDC, szExt, iExtLen, &tCalcRect, DT_NOCLIP | DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX );

				tStringRect.right -= tCalcRect.right - tCalcRect.left;
			
				if ( tStringRect.right > tStringRect.left )
					DrawText ( hDC, szName, iNameLen, &tStringRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
			}
		}
	}

	SelectObject ( hDC, hOldPen );

	return iCurFile;
}


bool FileView_c::GetFileRect ( int nFile, RECT & tRect ) const
{
	Assert ( m_pPanel );

	if ( nFile < 0 || nFile > m_pPanel->GetNumFiles () )
		return false;

	int iOffset = nFile - m_pPanel->GetFirstFile ();
	if ( iOffset < 0 )
		return false;

	int nColumnFiles = GetNumColumnFiles ();
	if ( iOffset >= nColumnFiles * m_nColumns )
		return false;

	int nCol = iOffset / nColumnFiles;
	int nRow = iOffset % nColumnFiles;

	RECT tColumnRect;
	GetColumnViewRect ( tColumnRect, nCol );

	tRect.top	 = m_tRect.top + nRow * ( GetTextHeight () + SPACE_BETWEEN_LINES ) + SPACE_BETWEEN_LINES;
	tRect.bottom = tRect.top + GetTextHeight () - 1;
	tRect.left	 = tColumnRect.left + 1;
	tRect.right	 = tColumnRect.right - 1;

	return true;
}


int	FileView_c::GetNumViewFiles () const
{
	return GetNumColumnFiles () * m_nColumns;
}


FindOnScreen_e FileView_c::FindFileOnScreen ( int & iResult, int iX, int iY ) const
{
	Assert ( m_pPanel );

	if ( iY < m_tRect.top || iY > m_tRect.bottom )
		return FIND_OUT_OF_PANEL;

	RECT tRect;
	GetColumnViewRect ( tRect, 0 );
	const int nColumnFiles = GetNumColumnFiles ();
	int iRow = ( iY - m_tRect.top - SPACE_BETWEEN_LINES ) / ( GetTextHeight () + SPACE_BETWEEN_LINES  );
	iRow = Clamp ( iRow, 0, nColumnFiles - 1 );
	int iColumn = iX / ( tRect.right - tRect.left + 1 );
	iResult = m_pPanel->GetFirstFile () + nColumnFiles * iColumn + iRow;

	int iLastFile = m_pPanel->GetNumFiles () - 1;
	if ( iResult > iLastFile )
	{
		iResult = iLastFile;
		return FIND_EMPTY_PANEL;
	}

	return FIND_FILE_FOUND;
}


void FileView_c::GetColumnViewRect ( RECT & tColumnRect, int nColumn ) const
{
	Assert ( m_pPanel );
	Assert ( nColumn >= 0 && nColumn < m_nColumns );

	int iWidth = ( m_tRect.right - m_tRect.left ) / m_nColumns;
	int iColWidth = nColumn * iWidth;
	tColumnRect.top		= m_tRect.top;
	tColumnRect.bottom	= m_tRect.bottom;
	tColumnRect.left	= m_tRect.left + iColWidth + ( nColumn ? 1 : 0 );
	tColumnRect.right	= m_tRect.left + iColWidth + iWidth - 1;	// 1 for separator
}


FileView_c * FileView_c::Create ( FileViewMode_e eMode )
{
	switch ( eMode )
	{
		case FILE_VIEW_1C:
			return new FileView1C_c;

		case FILE_VIEW_3C:
			return new FileView3C_c;

		case FILE_VIEW_2C:
			return new FileView2C_c;

		case FILE_VIEW_FULL:
			return new FileViewFull_c ( false, false );

		case FILE_VIEW_FULL_DATE:
			return new FileViewFull_c ( false, true );

		case FILE_VIEW_FULL_TIME:
			return new FileViewFull_c ( true, true );
	}

	return NULL;
}


///////////////////////////////////////////////////////////////////////////////////////////
// short file view
FileViewShortCommon_c::FileViewShortCommon_c ( int nColumns, const bool * pDrawFileIcon, const bool * pSeparateExt )
	: FileView_c ( nColumns, pDrawFileIcon, pSeparateExt )
{
}


void FileViewShortCommon_c::Draw ( HDC hDC )
{
	Assert ( m_pPanel );

	int iStartFile = m_pPanel->GetFirstFile ();
	int nFiles = m_pPanel->GetNumFiles ();

	int iColumnWidth = 0;
	RECT tColumnRect;
	static RECT dColumnRects [MAX_FILE_COLUMNS];
	for ( int i = 0; i < m_nColumns; ++i )
	{
		GetColumnViewRect ( tColumnRect, i );
		dColumnRects [i] = tColumnRect;

		// background
		++tColumnRect.right;
		--tColumnRect.bottom;
		FillRect ( hDC, &tColumnRect, g_tResources.m_hBackgroundBrush );
		--tColumnRect.right;
		++tColumnRect.bottom;
	}

	bool bDrawFileNames = iStartFile < nFiles;
	if ( bDrawFileNames )
		DrawCursor ( hDC );

	for ( int i = 0; i < m_nColumns; ++i )
	{
		tColumnRect = dColumnRects [i];
		iColumnWidth += tColumnRect.right - tColumnRect.left + 1;

		if ( i < m_nColumns - 1 )
		{
			++iColumnWidth;
			MoveToEx ( hDC, tColumnRect.right + 1, tColumnRect.top, NULL );
			LineTo ( hDC, tColumnRect.right + 1, tColumnRect.bottom );
		}

		if ( bDrawFileNames )
			iStartFile = DrawFileNames ( hDC, iStartFile, tColumnRect );
	}

	int iFillWidth = m_tRect.right - m_tRect.left - iColumnWidth;

	if ( iFillWidth > 0 )
	{
		RECT tFillRect;
		tFillRect.right = m_tRect.right;
		tFillRect.left = tFillRect.right - iFillWidth;
		tFillRect.top = m_tRect.top;
		tFillRect.bottom = m_tRect.bottom - 1;
		FillRect ( hDC, &tFillRect, g_tResources.m_hBackgroundBrush );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
// short file view
FileView1C_c::FileView1C_c ()
	: FileViewShortCommon_c ( 1, & g_tConfig.pane_1c_file_icon, & g_tConfig.pane_1c_separate_ext )
{
}


///////////////////////////////////////////////////////////////////////////////////////////
// middle file view
FileView2C_c::FileView2C_c ()
	: FileViewShortCommon_c ( 2, & g_tConfig.pane_2c_file_icon, & g_tConfig.pane_2c_separate_ext )
{
}


///////////////////////////////////////////////////////////////////////////////////////////
// short file view
FileView3C_c::FileView3C_c ()
: FileViewShortCommon_c ( 3, & g_tConfig.pane_3c_file_icon, & g_tConfig.pane_3c_separate_ext )
{
}


///////////////////////////////////////////////////////////////////////////////////////////
// full file view
FileViewFull_c::FileViewFull_c ( bool bShowTime, bool bShowDate )
	: FileView_c ( 1, &g_tConfig.pane_full_file_icon, &g_tConfig.pane_full_separate_ext )
	, m_bShowTime ( bShowTime )
	, m_bShowDate ( bShowDate )
{
}


int FileViewFull_c::CalcMaxStringWidth ( HDC hDC, const wchar_t * szString ) const
{
	Assert ( szString );

	const wchar_t * szStart = szString;
	int iWidth = 0;
	while ( *szStart )
	{
		ABC tABC;
		if ( iswdigit ( *szStart ) )
			tABC = g_tPanelFont.GetMaxDigitWidthABC ();
		else
			GetCharABCWidths ( hDC, *szStart, *szStart, &tABC );

		iWidth += tABC.abcA + tABC.abcB + tABC.abcC;

		// not the last?
//		if ( *( szStart + 1 ) )
//		 iWidth += tABC.abcC;

		++szStart;
	}

	return iWidth;
}


void FileViewFull_c::GetMaxTimeDateWidth ( HDC hDC, int & iTimeWidth, int & iDateWidth ) const
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


int FileViewFull_c::GetMaxSizeWidth ( HDC hDC ) const
{
	return Max ( CalcMaxStringWidth ( hDC, GetMaxByteString () ),
				 CalcMaxStringWidth ( hDC, GetMaxKByteString () ),
				 CalcMaxStringWidth ( hDC, GetMaxMByteString () ) );
}


void FileViewFull_c::Draw ( HDC hDC )
{
	Assert ( m_pPanel );
	int iStartFile = m_pPanel->GetFirstFile ();

	RECT tColumnRect;
	GetColumnViewRect ( tColumnRect, 0 );

	// background
	--tColumnRect.bottom;
	++tColumnRect.right;
	FillRect ( hDC, &tColumnRect, g_tResources.m_hBackgroundBrush );
	++tColumnRect.bottom;
	--tColumnRect.right;
	
	HGDIOBJ hOldPen = SelectObject ( hDC, g_tResources.m_hFullSeparatorPen );

	int iDateWidth = 0, iTimeWidth = 0;
	GetMaxTimeDateWidth ( hDC, iTimeWidth, iDateWidth );
	int iSizeWidth = GetMaxSizeWidth ( hDC );

	DrawCursor ( hDC );

	DrawFileInfo ( hDC, iStartFile, tColumnRect, iSizeWidth, m_bShowDate ? iDateWidth : 0, m_bShowTime ? iTimeWidth : 0 );
	int iTotalWidth = iSizeWidth + 3;
	
	if ( m_bShowDate )
		iTotalWidth += iDateWidth + 3;

	if ( m_bShowTime )
		iTotalWidth += iTimeWidth + 3;

	if ( tColumnRect.right - tColumnRect.left > iTotalWidth )
	{
		tColumnRect.right -= iTotalWidth + 1;
		DrawFileNames ( hDC, iStartFile, tColumnRect );
	}

	SelectObject ( hDC, hOldPen );
}


void FileViewFull_c::DrawFileInfo ( HDC hDC, int iStartFile, RECT & tRect, int iSizeWidth, int iDateWidth, int iTimeWidth )
{
	Assert ( m_pPanel);

	static wchar_t szTime [TIMEDATE_BUFFER_SIZE];
	static wchar_t szDate [TIMEDATE_BUFFER_SIZE];
	static wchar_t szSize [FILESIZE_BUFFER_SIZE];

	const int nFiles = m_pPanel->GetNumFiles ();
	const int iCursorFile = m_pPanel->GetCursorFile ();
	const int iSymbolHeight = GetTextHeight ();
	const int nColumnFiles = GetNumColumnFiles ();

	SYSTEMTIME tSysTime;
	FILETIME tLocalFileTime;

	// initial rect setup
	RECT tTimeRect, tDateRect, tSizeRect;

	tTimeRect.right = tRect.right;
	tTimeRect.left = tTimeRect.right - iTimeWidth;

	tDateRect.right = tTimeRect.left - ( m_bShowTime ? 3 : 0 );
	tDateRect.left = tDateRect.right - iDateWidth;

	tSizeRect.right = tDateRect.left - ( m_bShowDate ? 3 : 0 );
	tSizeRect.left  = tSizeRect.right - iSizeWidth;

	// draw the lines
	int iLineX = tSizeRect.left - 2;
	if ( iLineX >= tRect.left  )
	{
		MoveToEx ( hDC, iLineX, tRect.top, NULL );
		LineTo ( hDC, iLineX, tRect.bottom - 1 );
	}

	if ( m_bShowDate )
	{
		iLineX = tDateRect.left - 2;
		if ( iLineX >= tRect.left )
		{
			MoveToEx ( hDC, iLineX, tRect.top, NULL );
			LineTo ( hDC, iLineX, tRect.bottom - 1 );
		}
	}

	if ( m_bShowTime )
	{
		iLineX = tTimeRect.left - 2;
		if ( iLineX > tRect.left )
		{
			MoveToEx ( hDC, iLineX, tRect.top, NULL );
			LineTo ( hDC, iLineX, tRect.bottom - 1 );
		}
	}

	for ( int iCurFile = iStartFile; iCurFile < nFiles && iCurFile - iStartFile < nColumnFiles; ++iCurFile )
	{
		const FileInfo_t & tInfo = m_pPanel->GetFile ( iCurFile );
		const wchar_t * szExt = L"";

		tSizeRect.top = tDateRect.top = tTimeRect.top = tRect.top + ( iCurFile - iStartFile ) * ( iSymbolHeight + SPACE_BETWEEN_LINES ) + SPACE_BETWEEN_LINES;
		tSizeRect.bottom = tDateRect.bottom = tTimeRect.bottom = tSizeRect.top + iSymbolHeight;

		SetTextColor ( hDC, CGGetFileColor ( tInfo, iCurFile == iCursorFile ) );

		FileTimeToLocalFileTime ( &tInfo.m_tData.ftLastWriteTime, &tLocalFileTime );
		FileTimeToSystemTime ( &tLocalFileTime, &tSysTime );

		// draw time
		if ( m_bShowTime && tTimeRect.right > tRect.left )
		{
			GetTimeFormat ( LOCALE_SYSTEM_DEFAULT, TIME_NOSECONDS | TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT, &tSysTime, NULL, szTime, TIMEDATE_BUFFER_SIZE );
			DrawText ( hDC, szTime, wcslen ( szTime ), &tTimeRect, DT_NOCLIP | DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
		}

		 // draw date
		if ( m_bShowDate && tDateRect.right > tRect.left )
		{
			GetDateFormat ( LOCALE_SYSTEM_DEFAULT, DATE_SHORTDATE, &tSysTime, NULL, szDate, TIMEDATE_BUFFER_SIZE );
			DrawText ( hDC, szDate, wcslen ( szDate ), &tDateRect, DT_NOCLIP | DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
		}

		// draw size
		if ( tSizeRect.right > tRect.left )
		{
			if ( tInfo.m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				wsprintf ( szSize, Txt ( T_DIR_MARKER ) );
			else
				FileSizeToString ( tInfo.m_tData.nFileSizeHigh, tInfo.m_tData.nFileSizeLow, szSize, false );

			DrawText ( hDC, szSize, wcslen ( szSize ), &tSizeRect, DT_NOCLIP | DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX );
		}
	}
}

bool FileViewFull_c::GetMarkerFlag () const
{
	return false;
}