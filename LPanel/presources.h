#ifndef _presources_
#define _presources_

#include "LCore/cmain.h"


// simple font wrapper
class PanelFont_c
{
public:
			PanelFont_c ();

	bool	Init ();
	void 	Shutdown ();
	ABC		GetMaxDigitWidthABC ();

	int		GetHeight () const;
	int 	GetMaxCharWidth () const;
	HFONT	GetHandle () const;

private:
	ABC			m_tMaxABC;
	HFONT 		m_hFont;
	TEXTMETRIC	m_tMetric;

	void		GetMetrics ();
};


// common resources used by panels
class PanelResources_c
{
public:
	HBRUSH			m_hActiveHdrBrush;						// active header brush
	HBRUSH			m_hPassiveHdrBrush;						// passive header brush
	HBRUSH			m_hBackgroundBrush;						// background brush
	HBRUSH			m_hCursorBrush;							// cursor brush
	HBRUSH			m_hCursorBrushIA;							// cursor brush
	HPEN			m_hBorderPen;							// panel border pen
	HPEN			m_hOverflowPen;							// file overflow pen
	HPEN			m_hCursorPen;							// cursor pen
	HPEN			m_hCursorPenIA;							// cursor pen
	HPEN			m_hFullSeparatorPen;					// full panel mode separator pen
	HBITMAP			m_hBmpBtnMax;							// border button bitmaps
	HBITMAP			m_hBmpBtnMaxPressed;					// border button bitmaps
	HBITMAP			m_hBmpBtnMid;							// border button bitmaps
	HBITMAP			m_hBmpBtnMidPressed;					// border button bitmaps
	HBITMAP			m_hBmpBtnDir;							// dir button bitmap
	HBITMAP			m_hBmpBtnDirPressed;					
	HBITMAP			m_hBmpBtnUp;
	HBITMAP			m_hBmpBtnUpPressed;
	HBITMAP			m_hBmpChangeTo1;
	HBITMAP			m_hBmpChangeTo1Pressed;
	HBITMAP			m_hBmpChangeTo2;
	HBITMAP			m_hBmpChangeTo2Pressed;
	HFONT			m_hBoldSystemFont;						// bold system font
	HFONT			m_hSystemFont;							// bold system font
	HFONT			m_hSmallSystemFont;							// bold system font
	SIZE			m_tBmpMaxMidSize;						// max btn image size
	SIZE			m_tBmpDirSize;							// dir btn image size
	SIZE			m_tBmpUpSize;							// dir btn image size
	SIZE			m_tBmpChangeToSize;
	HDC				m_hBitmapDC;							// DC for BitBlts
	HIMAGELIST		m_hSmallIconList;						// small file icon list

	int				m_iScreenWidth;
	int				m_iScreenHeight;


					PanelResources_c ();

	HBRUSH			GetCursorBrush ( bool bActive );
	HPEN			GetCursorPen ( bool bActive );

	void			Init ();
	void			Shutdown ();
};


extern PanelResources_c	g_tResources;
extern PanelFont_c		g_tPanelFont;

#endif