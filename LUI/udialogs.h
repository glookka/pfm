#ifndef _udialogs_
#define _udialogs_

#include "LCore/cmain.h"
#include "LCore/cmap.h"

//////////////////////////////////////////////////////////////////////////
// base dialog class
class Dialog_c
{
public:
						Dialog_c ( const wchar_t * szHelp );
	virtual				~Dialog_c ();

	int					Run ( int iResource, HWND hParent );

	virtual void		OnInit () {}
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

	HWND				Item ( int iItemId );
	void				ItemTxt ( int iItemId, const wchar_t * szText );
	Str_c				GetItemTxt ( int iItemId );
	void				CheckBtn ( int iItemId, bool bCheck );
	bool				IsChecked ( int iItemId ) const;
	void				Loc ( int iItemId, int iString );
	void				Bold ( int iItemId );

	virtual void		Close ( int iItem );

private:
	typedef Map_T <HWND, Dialog_c *> WndProcs_t;
	static WndProcs_t 	m_WndProcs;
	static Dialog_c *	m_pLastCreatedDialog;

	static BOOL CALLBACK DialogProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );

	Str_c				m_sHelp;
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
	int				m_iSpacing;
	int				m_iHorSpacing;
	bool			m_bForcedResize;
	Array_T <HWND>	m_dStayers;

	void			Resize ();
};

//////////////////////////////////////////////////////////////////////////
// moving dialog
class Dialog_Moving_c : public Dialog_c
{
public:
					Dialog_Moving_c ( const wchar_t * szHelp, int iToolbar, bool bDefaultNames = true );

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
void Help ( const wchar_t * szHelp );
HWND CreateToolbar ( HWND hParent, int iId, DWORD uFlags = 0, bool bDefaultNames = true );
void SetToolbarText ( HWND hToolbar, int iId, const wchar_t * szOkText );
void EnableToolbarButton ( HWND hToolbar, int iId, bool bEnable );

HWND InitFullscreenDlg ( HWND hDlg, int iToolbar, DWORD uBarFlags, bool bHideDone = false );
void CloseFullscreenDlg ();
bool IsInDialog ();
void ActivateFullscreenDlg ();


#endif
