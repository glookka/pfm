#include "pch.h"

#include "LPanel/presources.h"
#include "LCore/cstr.h"
#include "LCore/clog.h"
#include "LCore/cos.h"
#include "LSettings/sconfig.h"
#include "LSettings/scolors.h"
#include "LSettings/slocal.h"

#include "aygshell.h"
#include "Resources/resource.h"

#define DEL_OBJ(obj) if ( obj ) DeleteObject (obj);
#define DEL_DC(obj) if ( obj ) DeleteDC (obj);

PanelResources_c	g_tResources;
PanelFont_c			g_tPanelFont;

///////////////////////////////////////////////////////////////////////////////////////////
// the panel font
PanelFont_c::PanelFont_c ()
	: m_hFont 		( NULL )
{
	memset ( &m_tMetric, 0, sizeof (m_tMetric) );
	memset ( &m_tMaxABC, 0, sizeof (m_tMaxABC) );
}


bool PanelFont_c::Init ()
{
	LOGFONT tFont;
	tFont.lfHeight			= g_tConfig.font_height;
	tFont.lfWidth			= 0;
	tFont.lfEscapement		= 0;
	tFont.lfOrientation		= 0;
	tFont.lfWeight			= g_tConfig.font_bold ? FW_BOLD : FW_NORMAL;
	tFont.lfItalic			= FALSE;
	tFont.lfUnderline		= FALSE;
	tFont.lfStrikeOut		= FALSE;
	tFont.lfCharSet			= DEFAULT_CHARSET;
	tFont.lfOutPrecision	= OUT_DEFAULT_PRECIS;
   	tFont.lfClipPrecision	= CLIP_DEFAULT_PRECIS;
	tFont.lfQuality			= g_tConfig.font_clear_type ? CLEARTYPE_QUALITY : DEFAULT_QUALITY;
	tFont.lfPitchAndFamily	= DEFAULT_PITCH;
	wcscpy ( tFont.lfFaceName, g_tConfig.font_name );

	m_hFont = CreateFontIndirect ( & tFont );

	GetMetrics ();

	return !!m_hFont;
}


void PanelFont_c::GetMetrics ()
{
	const int NUM_DIGITS = L'9' - L'0' + 1;
	ABC dABC [NUM_DIGITS];

	if ( m_hFont )
	{
		HDC hDC = g_tResources.m_hBitmapDC;
		HGDIOBJ hOldObj = SelectObject ( hDC, m_hFont );
		GetTextMetrics ( hDC, &m_tMetric );
		GetCharABCWidths ( hDC, L'0', L'9', dABC );
		
		int iMax = 0;
		for ( int i = 1; i < NUM_DIGITS; ++i )
			if ( dABC [i].abcA + dABC [i].abcB + dABC [i].abcC > dABC [iMax].abcA + dABC [iMax].abcB + dABC [iMax].abcC )
				iMax = i;

		m_tMaxABC = dABC [iMax];

		SelectObject ( hDC, hOldObj );
	}
}


ABC PanelFont_c::GetMaxDigitWidthABC ()
{
	return m_tMaxABC;
}


void PanelFont_c::Shutdown ()
{
	if ( m_hFont )
		DeleteObject ( m_hFont );
}


int	PanelFont_c::GetHeight () const
{
	return m_tMetric.tmHeight ? m_tMetric.tmHeight : g_tConfig.font_height;
}


int PanelFont_c::GetMaxCharWidth () const
{
	return m_tMetric.tmMaxCharWidth;
}


HFONT PanelFont_c::GetHandle () const
{
	return m_hFont;
}


///////////////////////////////////////////////////////////////////////////////////////////
// common panel resources
PanelResources_c::PanelResources_c ()
	: m_hActiveHdrBrush		( NULL )
	, m_hPassiveHdrBrush	( NULL )
	, m_hBackgroundBrush	( NULL )
	, m_hCursorBrush		( NULL )
	, m_hCursorBrushIA		( NULL )
	, m_hBorderPen			( NULL )
	, m_hOverflowPen		( NULL )
	, m_hCursorPen			( NULL )
	, m_hCursorPenIA		( NULL )
	, m_hFullSeparatorPen	( NULL )
	, m_hBmpBtnMax			( NULL )
	, m_hBmpBtnMaxPressed	( NULL )
	, m_hBmpBtnMid			( NULL )
	, m_hBmpBtnMidPressed	( NULL )
	, m_hBmpBtnDir			( NULL )
	, m_hBmpBtnDirPressed	( NULL )
	, m_hBmpBtnUp			( NULL )
	, m_hBmpBtnUpPressed	( NULL )
	, m_hBmpChangeTo1		( NULL )
	, m_hBmpChangeTo1Pressed( NULL )
	, m_hBmpChangeTo2		( NULL )
	, m_hBmpChangeTo2Pressed( NULL )
	, m_hBoldSystemFont		( NULL )
	, m_hSystemFont			( NULL )
	, m_hSmallSystemFont	( NULL )
	, m_hBitmapDC			( NULL )
	, m_hSmallIconList		( NULL )
	, m_iScreenWidth		( 240 )
	, m_iScreenHeight		( 320 )
{
	m_tBmpMaxMidSize.cx = m_tBmpMaxMidSize.cy = 0;
	m_tBmpDirSize.cx = m_tBmpDirSize.cy = 0;
	m_tBmpUpSize.cx = m_tBmpUpSize.cy = 0;
	m_tBmpChangeToSize.cx = m_tBmpChangeToSize.cy = 0;
}

HBRUSH PanelResources_c::GetCursorBrush ( bool bActive )
{
	return bActive ? m_hCursorBrush : m_hCursorBrushIA;
}

HPEN PanelResources_c::GetCursorPen ( bool bActive )
{
	return bActive ? m_hCursorPen : m_hCursorPenIA;
}


void PanelResources_c::Init ()
{
	m_hActiveHdrBrush	= CreateSolidBrush ( g_tColors.pane_header_active );
	m_hPassiveHdrBrush	= CreateSolidBrush ( g_tColors.pane_header_inactive );
	m_hBackgroundBrush	= CreateSolidBrush ( g_tColors.pane_back );
	m_hCursorBrush		= CreateSolidBrush ( g_tColors.pane_cursor_bar );
	m_hCursorBrushIA	= CreateSolidBrush ( g_tColors.pane_cursor_bar_inactive );

	m_hBorderPen	= CreatePen ( PS_SOLID, 1, g_tColors.pane_separators );
	m_hOverflowPen	= CreatePen ( PS_SOLID, 1, g_tColors.pane_overflow );
	m_hCursorPen	= CreatePen ( PS_DASH, 1, g_tColors.pane_cursor_rect );
	m_hCursorPenIA	= CreatePen ( PS_DASH, 1, g_tColors.pane_cursor_rect_inactive );

	m_hFullSeparatorPen	 = CreatePen ( PS_SOLID, 1, g_tColors.pane_full_separator );

	m_hBmpBtnMax = (HBITMAP) LoadImage ( ResourceInstance (), MAKEINTRESOURCE ( 	IsVGAScreen () ? IDB_MAXSIZE_VGA : IDB_MAXSIZE ), IMAGE_BITMAP, 0, 0, 0 );
	m_hBmpBtnMaxPressed	= (HBITMAP) LoadImage ( ResourceInstance (), MAKEINTRESOURCE ( 	IsVGAScreen () ? IDB_MAXSIZE_PRESSED_VGA : IDB_MAXSIZE_PRESSED ), IMAGE_BITMAP, 0, 0, 0 );

	m_hBmpBtnMid = (HBITMAP) LoadImage ( ResourceInstance (), MAKEINTRESOURCE ( IsVGAScreen () ? IDB_MIDSIZE_VGA : IDB_MIDSIZE ), IMAGE_BITMAP, 0, 0, 0 );
	m_hBmpBtnMidPressed	= (HBITMAP) LoadImage ( ResourceInstance (), MAKEINTRESOURCE ( 	IsVGAScreen () ? IDB_MIDSIZE_PRESSED_VGA : IDB_MIDSIZE_PRESSED ), IMAGE_BITMAP, 0, 0, 0 );

	m_hBmpBtnDir = (HBITMAP) LoadImage ( ResourceInstance (), MAKEINTRESOURCE ( IsVGAScreen () ? IDB_DIRS_VGA : IDB_DIRS ), IMAGE_BITMAP, 0, 0, 0 );
	m_hBmpBtnDirPressed = (HBITMAP) LoadImage ( ResourceInstance (), MAKEINTRESOURCE ( IsVGAScreen () ? IDB_DIRS_PRESSED_VGA : IDB_DIRS_PRESSED ), IMAGE_BITMAP, 0, 0, 0 );

	m_hBmpBtnUp = (HBITMAP) LoadImage ( ResourceInstance (), MAKEINTRESOURCE ( IsVGAScreen () ? IDB_UP_VGA : IDB_UP ), IMAGE_BITMAP, 0, 0, 0 );
	m_hBmpBtnUpPressed = (HBITMAP) LoadImage ( ResourceInstance (), MAKEINTRESOURCE ( IsVGAScreen () ? IDB_UP_PRESSED_VGA : IDB_UP_PRESSED ), IMAGE_BITMAP, 0, 0, 0 );

	m_hBmpChangeTo1 = (HBITMAP) LoadImage ( ResourceInstance (), MAKEINTRESOURCE ( IsVGAScreen () ? IDB_CHANGE_TO1_VGA : IDB_CHANGE_TO1 ), IMAGE_BITMAP, 0, 0, 0 );
	m_hBmpChangeTo1Pressed = (HBITMAP) LoadImage ( ResourceInstance (), MAKEINTRESOURCE ( IsVGAScreen () ? IDB_CHANGE_TO1_PRESSED_VGA : IDB_CHANGE_TO1_PRESSED ), IMAGE_BITMAP, 0, 0, 0 );

	m_hBmpChangeTo2 = (HBITMAP) LoadImage ( ResourceInstance (), MAKEINTRESOURCE ( IsVGAScreen () ? IDB_CHANGE_TO2_VGA : IDB_CHANGE_TO2 ), IMAGE_BITMAP, 0, 0, 0 );
	m_hBmpChangeTo2Pressed = (HBITMAP) LoadImage ( ResourceInstance (), MAKEINTRESOURCE ( IsVGAScreen () ? IDB_CHANGE_TO2_PRESSED_VGA : IDB_CHANGE_TO2_PRESSED ), IMAGE_BITMAP, 0, 0, 0 );

	BITMAP tBmp;
	GetObject ( m_hBmpBtnMax, sizeof ( BITMAP ), (void *) &tBmp);
	m_tBmpMaxMidSize.cx = tBmp.bmWidth;
	m_tBmpMaxMidSize.cy = tBmp.bmHeight;

	GetObject ( m_hBmpBtnDir, sizeof ( BITMAP ), (void *) &tBmp );
	m_tBmpDirSize.cx = tBmp.bmWidth;
	m_tBmpDirSize.cy = tBmp.bmHeight;

	GetObject ( m_hBmpBtnUp, sizeof ( BITMAP ), (void *) &tBmp );
	m_tBmpUpSize.cx = tBmp.bmWidth;
	m_tBmpUpSize.cy = tBmp.bmHeight;

	GetObject ( m_hBmpChangeTo1, sizeof ( BITMAP ), (void *) &tBmp );
	m_tBmpChangeToSize.cx = tBmp.bmWidth;
	m_tBmpChangeToSize.cy = tBmp.bmHeight;

	m_hBitmapDC = CreateCompatibleDC ( NULL );

	LOGFONT tFont;

	int iFontSizePixel = IsVGAScreen () ? 26 : 13;
	int iSmallFontSize = IsVGAScreen () ? 24 : 12;

	GetObject ( (HFONT)GetStockObject ( SYSTEM_FONT ), sizeof (LOGFONT), &tFont );
	tFont.lfHeight = iFontSizePixel;
	tFont.lfWeight = FW_BOLD;
	m_hBoldSystemFont = CreateFontIndirect ( &tFont );

	tFont.lfWeight = FW_NORMAL;
	m_hSystemFont = CreateFontIndirect ( &tFont );

	tFont.lfHeight = iSmallFontSize;
	m_hSmallSystemFont = CreateFontIndirect ( &tFont );

	SHFILEINFO tShFileInfo;
	tShFileInfo.iIcon = -1;
	m_hSmallIconList = (HIMAGELIST)SHGetFileInfo ( L" ", FILE_ATTRIBUTE_DIRECTORY, &tShFileInfo, sizeof (SHFILEINFO), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON );
}


void PanelResources_c::Shutdown ()
{
	DEL_OBJ ( m_hActiveHdrBrush );
	DEL_OBJ ( m_hPassiveHdrBrush );
	DEL_OBJ ( m_hPassiveHdrBrush );
	DEL_OBJ ( m_hBackgroundBrush );
	DEL_OBJ ( m_hCursorBrush );
	DEL_OBJ ( m_hCursorBrushIA );
	DEL_OBJ ( m_hBorderPen );
	DEL_OBJ ( m_hOverflowPen );
	DEL_OBJ ( m_hFullSeparatorPen );
	DEL_OBJ ( m_hCursorPen );
	DEL_OBJ ( m_hCursorPenIA );
	DEL_OBJ ( m_hBmpBtnMax );
	DEL_OBJ ( m_hBmpBtnMaxPressed );
	DEL_OBJ ( m_hBmpBtnMid );
	DEL_OBJ ( m_hBmpBtnMidPressed );
	DEL_OBJ ( m_hBmpBtnDir );
	DEL_OBJ ( m_hBmpBtnDirPressed );
	DEL_OBJ ( m_hBmpBtnUp );
	DEL_OBJ ( m_hBmpBtnUpPressed );

	DEL_OBJ ( m_hBmpChangeTo1 );
	DEL_OBJ ( m_hBmpChangeTo1Pressed );
	DEL_OBJ ( m_hBmpChangeTo2 );
	DEL_OBJ ( m_hBmpChangeTo2Pressed );

	DEL_OBJ ( m_hBoldSystemFont );
	DEL_OBJ ( m_hSystemFont );
	DEL_OBJ ( m_hSmallSystemFont );
	DEL_DC ( m_hBitmapDC );
}