#include "pch.h"

#include "LDialogs/derrors.h"
#include "LCore/clog.h"
#include "LPanel/presources.h"
#include "LUI/udialogs.h"
#include "LDialogs/dcommon.h"
#include "LSettings/slocal.h"

#include "Resources/resource.h"
#include "aygshell.h"


class ErrorMessageDialog_c
{
public:
	ErrorMessageDialog_c ( int iDlg, const wchar_t * szTitle, const wchar_t * szText )
		: m_sTitle	( szTitle )
		, m_sText	( szText )
		, m_iDlg	( iDlg )
	{
	}

	void Init ( HWND hDlg )
	{
		HWND hTB = CreateToolbar ( hDlg, IDM_OK_CANCEL, 0, m_iDlg != IDD_ERROR_YES_NO );
		if ( m_iDlg == IDD_ERROR_YES_NO )
		{
			SetToolbarText ( hTB, IDOK, 	Txt ( T_TBAR_YES ) );
			SetToolbarText ( hTB, IDCANCEL, Txt ( T_TBAR_NO ) );

			DlgTxt ( hDlg, IDOK, T_TBAR_YES );
			DlgTxt ( hDlg, IDCANCEL, T_TBAR_NO );
		}
		else
		{
			DlgTxt ( hDlg, IDOK, T_TBAR_OK );
			DlgTxt ( hDlg, IDCANCEL, T_TBAR_CANCEL );
		}

		SendMessage ( GetDlgItem ( hDlg, IDC_TITLE ), WM_SETFONT, (WPARAM)g_tResources.m_hBoldSystemFont, TRUE );
		SetDlgItemText ( hDlg, IDC_TITLE, m_sTitle );
		SetDlgItemText ( hDlg, IDC_TEXT, m_sText );
	}

private:
	Str_c 	m_sTitle;
	Str_c 	m_sText;
	int		m_iDlg;
};


static ErrorMessageDialog_c * g_pErrorDialog = NULL;


static BOOL CALLBACK DlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	Assert ( g_pErrorDialog );

	switch ( Msg )
	{
	case WM_INITDIALOG:
		g_pErrorDialog->Init ( hDlg );
	    break;

    case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog ( hDlg, LOWORD (wParam) );
            break;
		}
		break;
	}

	DWORD dwRes = HandleDlgColor ( hDlg, Msg, wParam, lParam );
	if ( dwRes )
		return dwRes;

	return MovingWindowProc ( hDlg, Msg, wParam, lParam );
}

int ShowErrorDialog ( HWND hParent, const wchar_t * szTitle, const wchar_t * szText, int iDlgType )
{
	Assert ( szTitle && szText && ! g_pErrorDialog);

	g_pErrorDialog = new ErrorMessageDialog_c ( iDlgType, szTitle, szText );
	int iRes = DialogBox ( ResourceInstance (), MAKEINTRESOURCE (iDlgType), hParent, DlgProc );
	SafeDelete ( g_pErrorDialog );
		
	return iRes;
}