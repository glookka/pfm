#ifndef _pview_
#define _pview_

#include "LCore/cmain.h"


enum FindOnScreen_e
{
	 FIND_OUT_OF_PANEL
	,FIND_EMPTY_PANEL
	,FIND_PANEL_TOP
	,FIND_PANEL_BOTTOM
	,FIND_FILE_FOUND
};


enum FileViewMode_e
{
	 FILE_VIEW_1C
	,FILE_VIEW_2C
	,FILE_VIEW_3C
	,FILE_VIEW_FULL
	,FILE_VIEW_FULL_DATE
	,FILE_VIEW_FULL_TIME
	,FILE_VIEW_TOTAL
};


enum
{
	 SPACE_BETWEEN_LINES	= 1
	,MAX_FILE_COLUMNS		= 3
};


class FilePanel_c;


//
// common file view
//
class FileView_c
{
public:
						FileView_c ( int nColumns, const bool * pDrawFileIcon, const bool * pSeparateExt );

	void				SetViewRect ( const RECT & tRect );
	void				SetPanel ( FilePanel_c * pPanel );

	virtual void		Draw ( HDC hDC ) = 0;

	FindOnScreen_e		FindFileOnScreen ( int & iResult, int iX, int iY ) const;
	bool				GetFileRect ( int nFile, RECT & tRect ) const;
	void				GetColumnViewRect ( RECT & tColumnRect, int nColumn ) const;

	int					GetNumColumns () const;
	int					GetNumViewFiles () const;
	int					GetNumColumnFiles () const;

	void 				InvalidateCursor ( HWND hWnd, BOOL bErase ) const;

	static FileView_c *	Create ( FileViewMode_e eMode );

protected:
	RECT				m_tRect;
	const bool *		m_pDrawFileIcon;
	const bool *		m_pSeparateExt;
	FilePanel_c *		m_pPanel;

	void				DrawCursor ( HDC hDC ) const;
	int					DrawFileNames ( HDC hDC, int iStartFile, const RECT & tDrawRect );
	int					GetTextHeight () const;

	virtual bool		GetMarkerFlag () const;

protected:
	int					m_nColumns;
};

//
// common short file view
//
class FileViewShortCommon_c : public FileView_c
{
public:
						FileViewShortCommon_c ( int nColumns, const bool * pDrawFileIcon, const bool * pSeparateExt );

	virtual void		Draw ( HDC hDC );
};


//
//  1-column file view
//
class FileView1C_c : public FileViewShortCommon_c
{
public:
						FileView1C_c ();
};

//
// 2-column file view
//
class FileView2C_c : public FileViewShortCommon_c
{
public:
						FileView2C_c ();
};


//
// 3-column file view
//
class FileView3C_c : public FileViewShortCommon_c
{
public:
						FileView3C_c ();
};


//
// Full file view
//
class FileViewFull_c : public FileView_c
{
public:
						FileViewFull_c ( bool bShowTime, bool bShowDate );

	virtual void		Draw ( HDC hDC );

private:
	bool				m_bShowTime;
	bool				m_bShowDate;

	void				DrawFileInfo ( HDC hDC, int iStartFile, RECT & tRect, int iSizeWidth, int iDateWidth, int iTimeWidth );

	void				GetMaxTimeDateWidth ( HDC hDC, int & iTimeWidth, int & iDateWidth ) const;
	int					GetMaxSizeWidth ( HDC hDC ) const;

	int					CalcMaxStringWidth ( HDC hDC, const wchar_t * szString ) const;

	virtual bool		GetMarkerFlag () const;
};

#endif