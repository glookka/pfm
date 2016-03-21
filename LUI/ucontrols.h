#ifndef _ucontrols_
#define _ucontrols_

#include "LCore/cmain.h"
#include "LCore/cmap.h"

//////////////////////////////////////////////////////////////////////////
// base custom control
class CustomControl_c
{
public:
						CustomControl_c ();
	virtual				~CustomControl_c ();

	void				Create ( HWND hParent );
	void				Subclass ( HWND hControl );
	void				Show ( bool bShow );
	HWND				Handle () const;

	static bool			Init ();
	static void			Shutdown ();

protected:
	HWND				m_hWnd;
	HWND				m_hParent;

	virtual void		OnCreate () {}
	virtual void		OnSetCursor () {}
	virtual void		OnPaint ( HDC hDC, const RECT & Rect ) {}
	virtual bool		OnEraseBkgnd () { return false; }
	virtual void		OnLButtonDown ( int iX, int iY ) {}
	virtual void		OnLButtonUp ( int iX, int iY ) {}
	virtual void		OnMouseMove ( int iX, int iY ) {}

private:
	typedef Map_T <HWND, CustomControl_c *> WndProcs_t;
	static WndProcs_t 	m_WndProcs;
	WNDPROC				m_fnWndProc;				// for subclassed controls

	static LRESULT		WndProc ( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam );
};

//////////////////////////////////////////////////////////////////////////
// splitter control
class SplitterBar_c : public CustomControl_c
{
public:
						SplitterBar_c ();
						~SplitterBar_c ();

	void				SetMinMax ( int iMin, int iMax );
	void				SetRect ( const RECT & tRect );				// just set the rect
	void				GetRect ( RECT & tRect ) const;
	bool				SetRectAndFixup ( const RECT & tRect );		// set rect and fixup. return true if anything's changed

	void				SetColors ( DWORD uBorder, DWORD uFill, DWORD uPressed );
	void				CreateDrawObjects ();
	void				DestroyDrawObjects ();

protected:
	virtual void		OnCreate ();
	virtual void		OnPaint ( HDC hDC, const RECT & Rect );
	virtual bool		OnEraseBkgnd ();
	virtual void		OnLButtonDown ( int iX, int iY );
	virtual void		OnLButtonUp ( int iX, int iY );
	virtual void		OnMouseMove ( int iX, int iY );

	virtual void		Event_SplitterMoved () {}

private:
	DWORD				m_uBorderColor;
	DWORD				m_uFillColor;
	DWORD				m_uPressedColor;

	RECT				m_tRect;
	RECT				m_tClientRect;
	bool				m_bVertical;
	int					m_iOffset;
	int					m_iMin;
	int					m_iMax;

	HPEN				m_hBorderPen;
	HBRUSH				m_hPressedBrush;
	HBRUSH				m_hFillBrush;

	void				FixupRect ();
};

//////////////////////////////////////////////////////////////////////////
// hyperlink
class Hyperlink_c : public CustomControl_c
{
public:
						Hyperlink_c ( const Str_c & sText, const Str_c & sURL );

protected:
	virtual void		OnCreate ();
	virtual void		OnSetCursor ();
	virtual void		OnLButtonUp ( int iX, int iY );
	virtual void		OnLButtonDown ( int iX, int iY );
	virtual void		OnPaint ( HDC hDC, const RECT & Rect );

private:
	static HCURSOR		m_hLinkCursor;
	static COLORREF		m_tColor;

	Str_c				m_sText;
	Str_c				m_sURL;

	bool				Navigate ();
	void				Paint ( HDC hDC, RECT & tRect );
};

#endif
