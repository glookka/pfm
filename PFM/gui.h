#ifndef _gui_
#define _gui_

#include "main.h"
#include "std.h"

#include "progress.h"

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


//////////////////////////////////////////////////////////////////////////
// base dialog class
class Dialog_c
{
public:
						Dialog_c ( const wchar_t * szHelp );
	virtual				~Dialog_c ();

	int					Run ( int iResource, HWND hParent );

	virtual void		OnInit ();
	virtual void		PostInit () {}
	virtual void		OnClose () {}
	virtual void		OnHelp ();
	virtual void		OnSettingsChange () {}
	virtual bool		OnNotify ( int iControlId, NMHDR * pInfo ) { return false; }
	virtual void		OnContextMenu ( int iX, int iY ) {}

	virtual bool		OnLButtonDown ( int iX, int iY ) { return false; }
	virtual bool		OnMouseMove ( int iX, int iY )  { return false; }
	virtual bool		OnLButtonUp ()  { return false; }

	virtual void		OnCommand ( int iItem, int iNotify ) {}
	virtual void		OnTimer ( DWORD uTimer ) {}

protected:
	HWND				m_hWnd;
	HWND				m_hToolbar;

	HWND				Item ( int iItemId );
	void				ItemTxt ( int iItemId, const wchar_t * szText );
	Str_c				GetItemTxt ( int iItemId );
	void				CheckBtn ( int iItemId, bool bCheck );
	bool				IsChecked ( int iItemId ) const;
	void				Loc ( int iItemId, int iString );
	void				Bold ( int iItemId );
	int					GetDlgId () const;
	HWND				GetToolbar () const;

	virtual void		Close ( int iItem );

private:
	typedef Map_T <HWND, Dialog_c *> WndProcs_t;
	static WndProcs_t 	m_WndProcs;
	static Dialog_c *	m_pLastCreatedDialog;

	static BOOL CALLBACK DialogProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );

	Str_c				m_sHelp;
	int					m_iDialogId;
};

//////////////////////////////////////////////////////////////////////////
// fullscreen dialog
class Dialog_Fullscreen_c : public Dialog_c
{
public:
					Dialog_Fullscreen_c ( const wchar_t * szHelp, int iToolbar, DWORD uBarFlags = 0, bool bHideDone = false );

	virtual void	OnInit ();

protected:
	virtual void	Close ( int iItem );

private:
	int				m_iToolbar;
	DWORD			m_uBarFlags;
	bool			m_bHideDone;
};

//////////////////////////////////////////////////////////////////////////
// resizer dialog
class Dialog_Resizer_c : public Dialog_Fullscreen_c
{
public:
					Dialog_Resizer_c ( const wchar_t * szHelp, int iToolbar, DWORD uBarFlags = 0, bool bHideDone = false );

	virtual void	PostInit ();
	virtual void	OnSettingsChange ();

protected:
	void			SetResizer ( HWND hWnd );
	void			AddStayer ( HWND hWnd );

private:
	HWND			m_hResizer;
	bool			m_bForcedResize;
	Array_T <HWND>	m_dStayers;

	void			Resize ();
};

//////////////////////////////////////////////////////////////////////////
// moving dialog
class Dialog_Moving_c : public Dialog_c
{
public:
					Dialog_Moving_c ( const wchar_t * szHelp, int iToolbar );

	virtual void	OnInit ();
	virtual bool	OnLButtonDown ( int iX, int iY );
	virtual bool	OnMouseMove ( int iX, int iY );
	virtual bool	OnLButtonUp ();

private:
	int				m_iToolbar;
	bool			m_bDefaultNames;
	RECT			m_DlgRect;
	POINT			m_MouseInDlg;
};

//////////////////////////////////////////////////////////////////////////
// func-based GUI helpers
void Help ( const wchar_t * szHelp );
HWND CreateToolbar ( HWND hParent, int iId, DWORD uFlags = 0 );
HWND CreateToolbar ( HWND hParent, HINSTANCE hInstance, int iId, DWORD uFlags );
void SetToolbarText ( HWND hToolbar, int iId, const wchar_t * szOkText );
void EnableToolbarButton ( HWND hToolbar, int iId, bool bEnable );

HWND InitFullscreenDlg ( HWND hDlg, int iToolbar, DWORD uBarFlags, bool bHideDone = false );
bool IsInDialog ();
void ActivateFullscreenDlg ();

bool HandleTabbedSizeChange ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, bool bForced = false );

int GetColumnWidthRelative ( float fWidth );

void SetComboTextFocused ( HWND hCombo, const wchar_t * szText );
void SetEditTextFocused ( HWND hCombo, const wchar_t * szText );

// "preparing files..."
void PrepareCallback ();
void DestroyPrepareDialog ();

//////////////////////////////////////////////////////////////////////////
// used by plugins

// fullscreen dialogs
bool InitFullscreenDlg ( HWND hDlg, bool bHideDone );
void CloseFullscreenDlg ();
void HandleActivate ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

// moving dialogs
BOOL MovingWindowProc ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

// resizers
bool HandleSizeChange ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, bool bForced );
void DoResize ( HWND hDlg, HWND hResizer, HWND * dStayers, int nStayers );


#endif
