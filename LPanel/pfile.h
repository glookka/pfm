#ifndef _pfile_
#define _pfile_

#include "LCore/cmain.h"
#include "LPanel/pbase.h"
#include "LPanel/pview.h"
#include "LFile/fsorter.h"


// file panel class
class FilePanel_c : public Panel_c, public FileSorter_c
{
	using FileSorter_c::MarkFile;
public:

					FilePanel_c ();
					~FilePanel_c ();

	virtual void	OnTimer ();								// timer event
	virtual void	OnFileChange ();						// file change event

	virtual void	QueueRefresh ( bool bAnyway = false );
	virtual void	UpdateAfterLoad ();						// update UI settings after load

	virtual void	SetRect ( const RECT & tRect );			// set the rect. update props
	virtual void	Activate ( bool bActive );				// activate this panel
	virtual int		GetMinHeight () const;					// min height for a panel. can't be less
	virtual int		GetMinWidth () const;					// min width for a panel. can't be less

	void			SetViewMode ( FileViewMode_e eMode );
	virtual HWND	GetSlider () const;

	virtual int		GetNumFiles () const;
	virtual const FileInfo_t & GetFile ( int iFile ) const;
	virtual void	MarkFile ( const Str_c & sFile, bool bMark );
	virtual void	MarkAllFiles ( bool bMark, bool bForceDirs = false );
	virtual void	MarkFilter ( const Str_c & sFilter, bool bMark );
	virtual void	InvertMarkedFiles ();
	virtual const Str_c & GetDirectory () const;
	virtual int		GetNumMarked () const;
	virtual ULARGE_INTEGER GetMarkedSize () const;
	virtual int		GetCursorFile () const;
	virtual void	Refresh ();
	virtual void	SoftRefresh ();
	virtual bool	GenerateFileList ( FileList_t & tList );
	virtual void 	SetDirectory ( const wchar_t * szDir );
	virtual void 	SetCursorAtFile ( const wchar_t * szFile );
	virtual void	SetVisibility ( bool bShowHidden, bool bShowSystem, bool bShowROM );
	virtual void	StepToPrevDir ();
	virtual void	ShowBookmarks ();

	int				GetFirstFile () const;

	/// config <-> panel links
	void			Config_Path ( Str_c * pPath );
	void			Config_SortMode ( int * pMode );
	void			Config_SortReverse ( bool * pReverse );
	void			Config_ViewMode ( int * pMode );
	
protected:
	virtual void	Event_Refresh ();						// file refresh occured
	virtual void	Event_Directory ();
	virtual void	Event_MarkFile ( int nFile, bool bSelect );
	virtual void	Event_BeforeSoftRefresh ();
	virtual void	Event_AfterSoftRefresh ();

	virtual void	ResetTemporaryStates ();

	virtual void	OnMaximize ();
	virtual void	OnPaint ( HDC hDC );					// process paint event
	virtual void	OnLButtonDown ( int iX, int iY );		// process "style down event"
	virtual void	OnLButtonUp ( int iX, int iY );
	virtual bool	OnKeyEvent ( int iBtn, btns::Event_e eEvent );
	virtual void	OnStyleMove ( int iX, int iY );			// stylus movement
	virtual void	OnVScroll ( int iPos, int iFlags, HWND hSlider );	// process slider events
	virtual bool	OnCommand ( WPARAM wParam, LPARAM lParam );
	virtual bool	OnMeasureItem ( int iId, MEASUREITEMSTRUCT * pMeasureItem );
	virtual bool	OnDrawItem ( int iId, const DRAWITEMSTRUCT * pDrawItem );

	virtual void	InvalidateHeader ();					// invalidates header

private:
	FileViewMode_e	m_eViewMode;
	FileView_c *	m_dFileViews [FILE_VIEW_TOTAL];
	FileView_c *	m_pFileView;							// current file view

	// config <-> panel links
	int *			m_pConfigViewMode;
	Str_c *	m_pConfigPath;
	int	*			m_pConfigSortMode;
	bool *			m_pConfigSortReverse;

	float			m_tTimeSinceRefresh;					// time since last refresh
	bool			m_bRefreshQueued;						// refresh queued
	bool			m_bRefreshAnyway;						// refresh regardless of number of files

	// resources
	HMENU			m_hSortModeMenu;						// sort mode menu handle
	HMENU			m_hViewModeMenu;						// view mode menu
	HMENU			m_hHeaderMenu;							// global header menu

	HWND			m_hSlider;								// the slider
	HWND			m_hBtnDirs;								// directory button
	HWND			m_hBtnUp;								// up button
	RECT			m_tDirBtnRect;							// dir button rect
	RECT			m_tUpBtnRect;							// up button rect

	int				m_iCursorFile;							// the file we're standing at
	int				m_iFirstFile;							// the first file we can see

	bool			m_bWasSelected;							// was file selected at mouse btn down event

	int				m_iInitialMarkFile;
	int				m_iFirstMarkFile;						// first drag file
	int				m_iLastMarkFile;						// last drag file

	bool			m_bJustActivated;
	bool			m_bSingleFileOperation;					// single file operation

	Str_c			m_sFileAtCursor;						// position to this file after soft refresh

	FindOnScreen_e	m_eLastFindResult;						// last file find result

	void 			DrawTextInRect ( HDC hDC, RECT & tRect, const wchar_t * szText, DWORD uColor );

	void			DrawPanelHeader ( HDC hDC );			// draws the header - dir, etc
	void			DrawFileInfo ( HDC hDC );				// draws the bottom - file info, etc

	FindOnScreen_e	FindFileOnScreen ( int & iResult, int iX, int iY, const RECT & tViewRect ) const; // returns the index of file under cursor
	void			UpdateScrollInfo () const;				// updates scroll info
	void			UpdateScrollPos () const;				// updates scroll position only

	void			InvalidateFileInfo () const;			// invalidates bottom file info
	void			InvalidateFiles () const;				// invalidate files
	void			InvalidateFilesArea () const;			// invalidate whole file area including separators
	// rect getters
	void			GetFileViewRect ( RECT & tRect ) const;	// client rect for drawing file names in a panel
	void			GetFileInfoRect ( RECT & tRect ) const;	// file info client rect
	int				GetFileInfoTextHeight () const;			// get max icon/font height

	virtual void	GetHeaderViewRect ( RECT & tRect ) const;	// header client rect
	virtual int		GetHeaderViewHeight () const;			// header height

	bool			IsSliderVisible () const;

	// moves a cursor to this click pos
	void			MoveCursorTo ( int iX, int iY, const RECT & tViewRect );
	
	// changes cursor pos. clamps cursor if out of screen
	void			ChangeSelectedFile ( int iFileId );		// change the selected file based on click

	// scroll cursor to the desired pos. scroll all files if needed
	void			ScrollCursorTo ( int iCursorFile );

	// scroll files to the desired first file pos. clamp cursor to stay on the panel
	void			ScrollFilesTo ( int iFirstFile );

	void 			DoFileExecuteWork ( int iFileClicked, bool bClickInPanel = true );	// do the work
	void			ProcessFileClick ( int iX, int iY, const RECT & tViewRect );
	void			ChangeDirectory ( const FileInfo_t & tInfo );	// change the directory based on click
	void			ProcessDriveMenuResults ( const wchar_t * szPath );

	void			ExecuteFile ( const FileInfo_t & tInfo ); // execute the file by assoc
	void			MarkFilesTo ( int iFile, bool bClearFirst );

	virtual bool	CanUseFile ( int iFile ) const;
	int				ClampCursorFile ( int iCursor ) const;	// clamps current cursor pos
	int				ClampFirstFile ( int iFirstFile ) const;// clams first file to avoid empty space

	void			SetCursorFile ( int iCursor );

	void			ShowHeaderMenu ( int iX, int iY );	// shows file sort mode selection menu
	void			ShowFileMenu ( int iFile, int iX, int iY );
	void			ShowEmptyMenu ( int iX, int iY );

	// resources
	void			CreateMenus ();							// creates popup menus
	void			CreateDrawObjects ();					// creates misc brushes/pens/etc
	void			CreateSlider ();						// create a slider
	void			DestroyDrawObjects ();					// destroy these misc objects
};


#endif