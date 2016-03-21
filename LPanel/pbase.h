#ifndef _pbase_
#define _pbase_

#include "LCore/cmain.h"
#include "LCore/cbuttons.h"
#include "LFile/fsorter.h"
#include "LFile/fiterator.h"

// misc panel types
enum PanelType_e
{
	 PANEL_FILES
};


// common panel view
class Panel_c
{
public:	
					Panel_c  ();
	virtual			~Panel_c  ();

	static bool		Init ();						// register window class data
	static void		Shutdown ();					// cleanup

	static bool		IsAnyPaneInMenu ();
	static bool		IsLayoutVertical ();
	static Panel_c * CreatePanel ( PanelType_e eType, bool bTop );	// creates a panel of a given type

	virtual void	UpdateAfterLoad ();				// perform update after init
	virtual HWND	GetSlider () const { return NULL; }

	virtual int		GetMinHeight () const;
	virtual int		GetMinWidth () const;

	virtual void	SetRect ( const RECT & tRect );
	virtual void	Activate ( bool bActive );			// activate this panel
	bool			IsActive () const;
	void			Show ( bool bShow );				// show or hide
	void			Maximize ( bool bMaximize );
	bool			IsMaximized () const;

	// public events
	virtual void	OnTimer () {}
	virtual void	OnFileChange () {}

	// file-specific functions. called from menu bar
	// and the menu bar doesn't care about panel content
	virtual void	QueueRefresh ( bool bAnyway = false ) {}
	virtual int		GetNumFiles () const = 0;
	virtual const FileInfo_t & GetFile ( int iFile ) const = 0;
	virtual void	MarkFile ( const Str_c & sFile, bool bMark ) = 0;
	virtual void	MarkAllFiles ( bool bMark, bool bForceDirs = false ) = 0;
	virtual void	MarkFilter ( const Str_c & sFilter, bool bMark ) = 0;
	virtual void	InvertMarkedFiles () = 0;
	virtual const Str_c & GetDirectory () const = 0;
	virtual int		GetNumMarked () const = 0;
	virtual ULARGE_INTEGER GetMarkedSize () const = 0;
	virtual int		GetCursorFile () const = 0;
	virtual bool	CanUseFile ( int iFile ) const = 0;
	virtual void	Refresh () = 0;
	virtual void	SoftRefresh () = 0;
	virtual bool	GenerateFileList ( FileList_t & tList ) = 0;
	virtual void 	SetDirectory ( const wchar_t * szDir ) = 0;
	virtual void 	SetCursorAtFile ( const wchar_t * szFile ) = 0;
	virtual void	SetVisibility ( bool bShowHidden, bool bShowSystem, bool bShowROM ) = 0;
	virtual void	StepToPrevDir () = 0;
	virtual void	ShowBookmarks () = 0;

	void			Config_Maximized ( bool * pMaximized );

	// called from main message queue
	virtual bool	OnKeyEvent ( int iBtn, btns::Event_e eEvent ) { return false; }

	void			RedrawWindow ();						// redraws whole window

protected:
	HWND			m_hWnd;
	RECT 			m_tRect;					// client rect
	bool			m_bMaximized;

	bool			IsClickAndHoldDetected ( int iX, int iY ) const;

	virtual void	ResetTemporaryStates () {}	// reset all temporary states like drag'n'drop, selection etc

	virtual void	GetLeftmostBtnRect ( RECT & tRect );

	virtual void	OnMaximize () {}
	virtual void	OnPaint ( HDC hDC );
	virtual void	OnLButtonDown ( int iX, int iY );
	virtual void	OnLButtonUp ( int iX, int iY ) {}
	virtual void	OnStyleMove ( int iX, int iY ) {}
	virtual void	OnVScroll ( int iPos, int iFlags, HWND hSlider ) {}
	virtual bool	OnCommand ( WPARAM wParam, LPARAM lParam );
	virtual bool	OnMeasureItem ( int iId, MEASUREITEMSTRUCT * pMeasureItem ) { return false; }
	virtual bool	OnDrawItem ( int iId, const DRAWITEMSTRUCT * pDrawItem );

	HDC				BeginPaint ( HDC hDC);
	void			EndPaint ( HDC hDC, const PAINTSTRUCT & PS );

	HWND			CreateButton ();
	void			DrawButton ( const DRAWITEMSTRUCT * pDrawItem, const RECT & tBtnRect, const SIZE & tBmpSize, HBITMAP hBmp, HBITMAP hBmpPressed ) const;

	virtual void	CreateDrawObjects ();
	virtual void	DestroyDrawObjects ();

	virtual void	DrawPanelHeader ( HDC hDC );			// panel header. just a filled rect and a bottom line

	// rect getters
	virtual int		GetHeaderViewHeight () const;				// header height
	void			GetHeaderRect ( RECT & tRect ) const;	// header rect
	virtual void	GetHeaderViewRect ( RECT & tRect ) const;	// header client rect

	// rect invalidaters
	virtual void	InvalidateHeader ();					// invalidates header

private:
	HWND			m_hBtnMax;
	HWND			m_hBtnChangeTo;

	HDC				m_hMemDC;
	HBITMAP			m_hMemBitmap;
	SIZE			m_BmpSize;

	RECT			m_tMaxMidBtnRect;			// max/mid button rect
	RECT			m_tChangeToBtnRect;			// max/mid button rect

	bool			m_bActive;
	bool *			m_pConfigMaximized;

	bool			m_bInMenu;

	static WNDPROC	m_pOldBtnProc;
	static Array_T <Panel_c *> m_dPanels;			// list of all panels

	void			EnterMenu ();
	void			ExitMenu ();
	bool			IsInMenu () const;

	static LRESULT CALLBACK WndProc ( HWND hWnd, UINT Msg, UINT wParam, LONG lParam );
	static LRESULT CALLBACK BtnProc ( HWND hWnd, UINT Msg, UINT wParam, LONG lParam );
};

// init the panels
void Init_Panels ();

// shutdown the panels
void Shutdown_Panels ();


#endif
