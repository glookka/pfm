#ifndef _dcommon_
#define _dcommon_

#include "LCore/cmain.h"
#include "LCore/carray.h"
#include "LCore/cmap.h"
#include "LFile/fcommon.h"
#include "LFile/fmisc.h"

class WindowResizer_c
{
public:
			WindowResizer_c ();

	void	SetDlg ( HWND hDlg );

	void	SetResizer ( HWND hWnd );
	void	AddStayer ( HWND hWnd );

	void	Resize ();

private:
	HWND	m_hDlg;
	HWND	m_hResizer;
	int		m_iSpacing;
	int		m_iHorSpacing;
	Array_T <HWND> m_dStayers;
};

class WindowProgress_c
{
public:
			WindowProgress_c ();

	void	Init ( HWND hDlg, const wchar_t * szHeader, FileProgress_c * pProgress  );
	bool 	UpdateProgress ( FileProgress_c * pProgress );
	void 	Setup ( FileProgress_c * pProgress );

protected:
	HWND 	m_hSourceText;
	HWND 	m_hDestText;

private:
	HWND 	m_hTotalProgressBar;
	HWND 	m_hFileProgressBar;
	HWND 	m_hTotalProgressText;
	HWND 	m_hFileProgressText;
	HWND 	m_hTotalText;
	HWND 	m_hFileText;
	
	double	m_fLastTime;
	wchar_t	m_szTotalSize [FILESIZE_BUFFER_SIZE];
};

//////////////////////////////////////////////////////////////////////////
// helper funcs
BOOL MovingWindowProc ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
DWORD HandleDlgColor ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam  );

bool HandleSizeChange ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, bool bForced = false );
bool HandleTabbedSizeChange ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam, bool bForced = false );
void HandleActivate ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

int GetColumnWidthRelative ( float fWidth );

void SetComboTextFocused ( HWND hCombo, const wchar_t * szText );
void SetEditTextFocused ( HWND hCombo, const wchar_t * szText );

// "preparing files..."
void PrepareCallback ();
void DestroyPrepareDialog ();

#endif