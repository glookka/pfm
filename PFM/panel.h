#ifndef _panel_
#define _panel_

#include "main.h"
#include "filelist.h"
#include "system.h"


struct ViewMode_t : public PanelView_t
{
	bool			m_bSeparateExt;
	bool			m_bShowIcons;
};


struct ViewModes_t
{
	ViewMode_t				m_dViewModes [VM_TOTAL];


							ViewModes_t ();

	void					Reset ();
	const PanelColumn_t &	GetColumn ( ViewMode_e eMode, int iColumn ) const;
};


enum FindOnScreen_e
{
	 FIND_OUT_OF_PANEL
	,FIND_EMPTY_PANEL
	,FIND_PANEL_TOP
	,FIND_PANEL_BOTTOM
	,FIND_FILE_FOUND
};


// a panel with columns
// displays files
// handles user action
class Panel_c : public FileList_c
{
public:	
	struct Button_t
	{
		HWND		m_hButton;

					Button_t ();

		void		SetPosition ( HWND hParent, int iX, int iY, bool bVisible );
		void		GetPosition ( int & iX, int & iY );
		void		Draw ( const DRAWITEMSTRUCT * pDrawItem, HBITMAP hBmp, HBITMAP hBmpPressed, bool bActive );
	};

	struct ColumnSize_t
	{
		int				m_iMaxTextWidth;	// max text width
		int				m_iWidth;
		int				m_iLeft;			// start x for this column
		int				m_iRight;			// end x for this column

						ColumnSize_t ();
	};


					Panel_c  ();
					~Panel_c  ();

	static bool		Init ();							// register window class data
	static void		Shutdown ();						// cleanup

	static bool		IsLayoutVertical ();

	int				GetMinHeight () const;
	int				GetMinWidth () const;

	void			SetRect		( const RECT & tRect );
	void			Activate	( bool bActive );		// activate this panel
	bool			IsActive	() const;
	void			Show		( bool bShow );			// show or hide
	bool			IsVisible	() const;
	void			Maximize	( bool bMaximize );
	bool			IsMaximized () const;
	void			SetViewMode ( ViewMode_e eViewMode );
	ViewMode_e		GetViewMode () const;

	// handlers
	bool			OnKeyEvent ( int iBtn, btns::Event_e eEvent );
	void			OnTimer ();
	void			OnFileChange ();

	bool			IsInMenu () const;

	// rect invalidaters
	void			InvalidateHeader ();				// invalidates header
	void			InvalidateFooter ();				// invalidates footer
	void			InvalidateCenter ();				// invalidates center
	void			InvalidateCursor ();				// invalidates cursor
	void			InvalidateAll ();					// invalidates all

	void			RedrawWindow ();					// redraws whole window

	void			QueueRefresh ( bool bAnyway = false );
	void			Refresh ();							// performs refresh, updates cached stuff
	void			SoftRefresh ();						// performs refresh, updates cached stuff
	void			Resort ();							// do not refresh, just re-sort items

	void			DoFileExecute ( int iFile, bool bKeyboard );
	int				GetCursorItem () const;
	const SelectedFileList_t * GetSelectedItems ();
	void			SetDirectory ( const wchar_t * szDir );
	void			StepToPrevDir ();
	void			SetCursorAtItem ( const wchar_t * szName );
	virtual bool	MarkFileById ( int iFile, bool bMark );
	void			MarkAllFiles ( bool bMark, bool bForceDirs = false );
	void			MarkFilter ( const wchar_t * szFilter, bool bMark );
	void			InvertMarkedFiles ();

protected:
	HWND			m_hWnd;
	RECT 			m_Rect;							// client rect
	bool			m_bMaximized;
	bool			m_bVisible;

	bool			IsClickAndHoldDetected ( int iX, int iY ) const;

	void			ResetTemporaryStates ();	// reset all temporary states like drag'n'drop, selection etc

	void			OnPaint ( HDC hDC );
	void			OnLButtonDown ( int iX, int iY );
	void			OnLButtonDblClk ( int iX, int iY );
	void			OnLButtonUp ( int iX, int iY );
	void			OnMouseMove ( int iX, int iY );
	bool			OnCommand ( WPARAM wParam, LPARAM lParam );
	bool			OnMeasureItem ( int iId, MEASUREITEMSTRUCT * pMeasureItem );
	bool			OnDrawItem ( int iId, const DRAWITEMSTRUCT * pDrawItem );
	void			OnVScroll ( int iPos, int iFlags, HWND hSlider );

	HDC				BeginPaint ( HDC hDC);
	void			EndPaint ( HDC hDC, const PAINTSTRUCT & PS );

	void			CreateButton ( Button_t & Button );
	void			DrawButton ( const DRAWITEMSTRUCT * pDrawItem, const RECT & tBtnRect, const SIZE & tBmpSize, HBITMAP hBmp, HBITMAP hBmpPressed ) const;

	void			CreateDrawObjects ();
	void			DestroyDrawObjects ();

	void			DrawCursor ( HDC hDC );

	void			DrawHeader ( HDC hDC );
	void			DrawCenter ( HDC hDC );
	void			DrawFooter ( HDC hDC );

	void			UpdateScrollInfo ();
	void			UpdateScrollPos ();

	// rect getters
	int				GetHeaderHeight () const;
	void			GetHeaderRect ( RECT & Rect ) const;		// header rect
	void			GetHeaderViewRect ( RECT & Rect ) const;	// header client rect
	int				GetFooterHeight () const;
	void			GetFooterRect ( RECT & Rect ) const;
	void			GetCenterRect ( RECT & Rect ) const;
	bool			GetItemTextRect ( int nItem, RECT & Rect ) const;
	bool			GetCursorRect ( RECT & Rect ) const;

	int				GetMaxItemsInView () const;
	int				GetMaxItemsInColumn () const;
	int				GetItemHeight () const;			// item height without spacers between lines
	int				GetRowHeight () const;			// full row heights with spacers

	// filesystem events
	void			Event_PluginOpened ( HANDLE hPlugin );
	void			Event_PluginClosed ();

private:
	Button_t		m_BtnMax;
	Button_t		m_BtnBookmarks;
	Button_t		m_BtnSwitch;
	Button_t		m_BtnUp;

	RECT			m_MarkedRect;
	POINT			m_LastMouse;

	HWND			m_hSlider;

	HDC				m_hMemDC;
	HBITMAP			m_hMemBitmap;
	SIZE			m_BmpSize;

	HIMAGELIST		m_hImageList;
	HIMAGELIST		m_hCachedImageList;				// image list before plugin

	HMENU			m_hSortModeMenu;
	HMENU			m_hViewModeMenu;
	HMENU			m_hHeaderMenu;

	// cursor stuff (both are in global units)
	int				m_iFirstItem;
	int				m_iCursorItem;

	float			m_fTimeSinceRefresh;
	bool			m_bRefreshAnyway;
	bool			m_bRefreshQueued;

	int				m_iLastMarkFile;
	bool			m_bMarkOnDrag;

	SelectedFileList_t m_SelectedList;

	bool			m_bSliderVisible;				// cached slider visibility
	bool			m_bActive;
	bool			m_bInMenu;
	bool			m_bFirstDrawAfterRefresh;		// first draw after refresh flag

	static WNDPROC	m_pOldBtnProc;

	ViewModes_t		m_ViewModes;
	ViewMode_e		m_eViewMode;
	ViewMode_e		m_eCachedViewMode;
	ViewMode_e		m_eViewModeBeforeMax;
	ColumnSize_t	m_dColumnSizes [MAX_COLUMNS];

	bool			m_bFilenameColMode;				// special 2 or more filename column mode

	void			EnterMenu ();
	void			ExitMenu ();

	void			CreateMenus ();
	void			ShowHeaderMenu ( int iX, int iY );
	void			ShowEmptyMenu ( int iX, int iY );
	void			ShowRecentMenu ( int iX, int iY );
	void			ShowFileMenu ( int iFile, int iX, int iY );
	void			ShowBookmarks ();
	void			ProcessDriveMenuResults ( const wchar_t * szPath );

	FindOnScreen_e	FindFile ( int & iResult, int iX, int iY ) const;

	void			MarkFilesTo ( int iFile );
	void			AfterRefresh ();

	void			SetCursorItem ( int iCursorItem );
	void			SetFirstItem ( int iFirstItem );
	int				ClampCursorItem ( int iCursor ) const;
	int				ClampFirstItem ( int iFirst ) const;
	void			ScrollItemsTo ( int iFirstFile );
	void			ScrollCursorTo ( int iCursorItem );
	void			SetCursorItemInvalidate ( int iCursor );

	static const wchar_t * FormatSize ( wchar_t * szBuf, DWORD uSizeHigh, DWORD uSizeLow, bool bDirectory );
	static const wchar_t * FormatTime ( wchar_t * szBuf, const FILETIME & Time );
	static const wchar_t * FormatDate ( wchar_t * szBuf, const FILETIME & Date );

	static const wchar_t * FormatTimeCreation	( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatTimeAccess		( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatTimeWrite		( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatDateCreation	( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatDateAccess		( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatDateWrite		( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatSize			( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatSizePacked		( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatText0			( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatText1			( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatText2			( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatText3			( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatText4			( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatText5			( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatText6			( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatText7			( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatText8			( wchar_t * szBuf, const PanelItem_t & Item );
	static const wchar_t * FormatText9			( wchar_t * szBuf, const PanelItem_t & Item );
	
	typedef const wchar_t * (* fnFormat) ( wchar_t *, const PanelItem_t & );

	void			GetMaxTimeDateWidth ( HDC hDC, int & iTimeWidth, int & iDateWidth ) const;
	int				GetMaxSizeWidth ( HDC hDC ) const;
	int				CalcMaxStringWidth ( HDC hDC, const wchar_t * szString ) const;
	int				CalcTextWidth ( HDC hDC, const wchar_t * szString ) const;

	void			InvalidateButtons ();
	void			RecalcColumnWidths ( HDC hDC );
	fnFormat		GetFormatFunc ( ColumnType_e eType ) const;
	void			DrawColumn ( int iId, HDC hDC, int iStartItem, int nItems, const RECT & DrawRect );
	void			DrawFileNames ( HDC hDC, int iStartItem, int nItems, const RECT & DrawRect );
	void			DrawColText ( HDC hDC, int iStartItem, int nItems, const RECT & DrawRect, bool bAlignRight, bool bClip, fnFormat Format );
	void			DrawTextInRect ( HDC hDC, RECT & tRect, const wchar_t * szText, DWORD uColor );

	static LRESULT CALLBACK WndProc ( HWND hWnd, UINT Msg, UINT wParam, LONG lParam );
	static LRESULT CALLBACK BtnProc ( HWND hWnd, UINT Msg, UINT wParam, LONG lParam );
};

// init the panels
void Init_Panels ();

// shutdown the panels
void Shutdown_Panels ();

#endif