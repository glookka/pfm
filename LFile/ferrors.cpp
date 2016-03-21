#include "pch.h"

#include "LFile/ferrors.h"
#include "LCore/clog.h"
#include "LCore/cos.h"
#include "LFile/fmisc.h"
#include "LPanel/presources.h"
#include "LUI/udialogs.h"
#include "LDialogs/dcommon.h"
#include "LSettings/slocal.h"

#include "Resources/resource.h"
#include "aygshell.h"

extern HWND g_hMainWindow;

OperationErrorList_c g_tErrorList;
static ErrorDialog_c * g_pCurrentError = NULL;

static BOOL CALLBACK ErrDialogProc ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

enum ErrorBtns_e
{
	 EB_C
	,EB_R_C
	,EB_R_S_SA_C
	,EB_O_OA_S_SA_C
};


///////////////////////////////////////////////////////////////////////////////////////////
class ErrorDialog_c
{
public:
	ErrorDialog_c ( int iErrorCode, bool bWinError,ErrorBtns_e eBtnCopy, ErrorBtns_e eBtnMove, ErrorBtns_e eBtnRename, ErrorBtns_e eBtnMkdir, ErrorBtns_e eBtnDelete,
					ErrorBtns_e eBtnProps, ErrorBtns_e eBtnCrypt, ErrorBtns_e eBtnShort, ErrorBtns_e eBtnSend, const wchar_t * szErrorDesc )
		: m_bAuto 			( false )
		, m_iAutoResult		( -1 )
		, m_eOperationMode	( OM_COPY )
		, m_sDesc			( szErrorDesc )
		, m_iErrorCode		( iErrorCode )
		, m_bWinError		( bWinError )
		, m_hWndParent		( NULL )
	{
		m_dErrorBtns [OM_COPY] 	= eBtnCopy;
		m_dErrorBtns [OM_MOVE] 	= eBtnMove;
		m_dErrorBtns [OM_RENAME]= eBtnRename;
		m_dErrorBtns [OM_MKDIR] = eBtnMkdir;
		m_dErrorBtns [OM_DELETE]= eBtnDelete;
		m_dErrorBtns [OM_PROPS] = eBtnProps;
		m_dErrorBtns [OM_CRYPT] = eBtnCrypt;
		m_dErrorBtns [OM_SHORTCUT] = eBtnShort;
		m_dErrorBtns [OM_SEND] = eBtnSend;
	}

	int ShowError ( HWND hParent, OperationMode_e eMode, const Str_c & sFile1, const Str_c & sFile2 )
	{
		m_hWndParent = hParent;

		if ( m_bAuto )
			return m_iAutoResult;

		m_sFile1 = sFile1;
		m_sFile2 = sFile2;
		int iRes = ShowDialog ( hParent, eMode );

		if ( iRes == IDC_SKIP_ALL )
		{
			m_bAuto = true;
			m_iAutoResult = iRes = IDC_SKIP;
		}

		if ( iRes == IDC_OVER_ALL )
		{
			m_bAuto = true;
			m_iAutoResult = iRes = IDC_OVER;
		}

		return iRes;
	}

	void Reset ()
	{
		m_hWndParent = NULL;
		m_bAuto = false;
		m_iAutoResult = -1;
		m_eOperationMode = OM_COPY;
		m_sFile1 = L"";
		m_sFile2 = L"";
	}

	void InitDlg ( HWND hDlg ) const
	{
		DlgTxt ( hDlg, IDC_TO, T_DLG_TO );
		DlgTxt ( hDlg, IDCANCEL, T_TBAR_CANCEL );
		DlgTxt ( hDlg, IDC_RETRY, T_DLG_RETRY );
		DlgTxt ( hDlg, IDC_SKIP, T_DLG_SKIP );
		DlgTxt ( hDlg, IDC_SKIP_ALL, T_DLG_SKIP_ALL );

		CreateToolbar ( hDlg, IDM_CANCEL );

		SetDlgItemText ( hDlg, IDC_OPERATION, Txt ( T_OP_HEAD_COPY + m_eOperationMode ) );
		SendMessage ( GetDlgItem ( hDlg, IDC_OPERATION ), WM_SETFONT, (WPARAM)g_tResources.m_hBoldSystemFont, TRUE );

		AlignFileName ( GetDlgItem ( hDlg, IDC_FILE1 ), m_sFile1 );
		if ( IsTwoFileDlg () )
			AlignFileName ( GetDlgItem ( hDlg, IDC_FILE2 ), m_sFile2 );
		
		SetDlgItemText ( hDlg, IDC_ERROR, m_sDesc );

		// set proper button name
		if ( m_dErrorBtns [m_eOperationMode] == EB_O_OA_S_SA_C )
		{
			SetDlgItemText ( hDlg, IDC_OVER, Txt ( m_eOperationMode + T_OP_BTN_COPY ) );
			SetDlgItemText ( hDlg, IDC_OVER_ALL, Txt ( T_OP_ALL ) );
		}
	}

	int GetCode () const
	{
		return m_iErrorCode;
	}

	bool GetWinFlag () const
	{
		return m_bWinError;
	}

	HWND GetParentWnd () const
	{
		return m_hWndParent;
	}

protected:
	int	ShowDialog ( HWND hParent, OperationMode_e eMode )
	{
		ErrorBtns_e eBtns = m_dErrorBtns [eMode];
		m_eOperationMode = eMode;

		// choosing the appropriate dialog
		int iDialog = -1;
		if ( IsTwoFileDlg () )
		{
			switch ( eBtns )
			{
				case EB_C:
					iDialog = IDD_ERR_TWO_C;
					break;
				case EB_R_C:
					iDialog =  IDD_ERR_TWO_R_C;
					break;
				case EB_R_S_SA_C:
					iDialog = IDD_ERR_TWO_R_S_SA_C;
					break;
				case EB_O_OA_S_SA_C:
					iDialog = IDD_ERR_TWO_O_OA_S_SA_C;
					break;
			}
		}
		else
		{
			switch ( eBtns )
			{
				case EB_C:
					iDialog = IDD_ERR_ONE_C;
					break;
				case EB_R_C:
					iDialog =  IDD_ERR_ONE_R_C;
					break;
				case EB_R_S_SA_C:
					iDialog = IDD_ERR_ONE_R_S_SA_C;
					break;
				case EB_O_OA_S_SA_C:
					iDialog = IDD_ERR_ONE_O_OA_S_SA_C;
					break;
			}
		}

		Assert ( iDialog != -1 );

		return DialogBox ( ResourceInstance (), MAKEINTRESOURCE (iDialog), hParent, ErrDialogProc );
	}

	bool IsTwoFileDlg () const
	{
		return m_eOperationMode == OM_COPY || m_eOperationMode == OM_MOVE || m_eOperationMode == OM_CRYPT;
	}

private:
	bool			m_bAuto;
	int				m_iAutoResult;
	OperationMode_e	m_eOperationMode;

	HWND			m_hWndParent;

	int				m_iErrorCode;
	bool			m_bWinError;

	ErrorBtns_e		m_dErrorBtns [OM_TOTAL];
	Str_c			m_sDesc;

	Str_c			m_sFile1;
	Str_c			m_sFile2;
};

///////////////////////////////////////////////////////////////////////////////////////////
// a window proc for error dialogs
static BOOL CALLBACK ErrDialogProc ( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	Assert ( g_pCurrentError );

	switch ( uMsg )
	{
	case WM_INITDIALOG:
		g_pCurrentError->InitDlg ( hDlg );
		return TRUE;

	case WM_COMMAND:
		switch ( LOWORD ( wParam ) )
		{
		case IDC_SKIP:
		case IDC_SKIP_ALL:
		case IDC_OVER:
		case IDC_OVER_ALL:
		case IDC_RETRY:
		case IDCANCEL:
			EndDialog ( hDlg, LOWORD (wParam) );
			return TRUE;
		}
	}

	return MovingWindowProc ( hDlg, uMsg, wParam, lParam );
}

///////////////////////////////////////////////////////////////////////////////////////////
OperationErrorList_c::OperationErrorList_c ()
	: m_eOperationMode ( OM_COPY )
{
}

void OperationErrorList_c::Init ()
{
	RegisterWinError ( ERROR_FILE_NOT_FOUND );
	RegisterWinError ( ERROR_PATH_NOT_FOUND );
	RegisterWinError ( ERROR_TOO_MANY_OPEN_FILES );
	RegisterWinError ( ERROR_ACCESS_DENIED );
	RegisterWinError ( ERROR_INVALID_PARAMETER );
	RegisterWinError ( ERROR_NOT_ENOUGH_MEMORY );
	RegisterWinError ( ERROR_WRITE_PROTECT );
	RegisterWinError ( ERROR_NOT_READY );
	RegisterWinError ( ERROR_NOT_DOS_DISK );
	RegisterWinError ( ERROR_SECTOR_NOT_FOUND );
	RegisterWinError ( ERROR_WRITE_FAULT );
	RegisterWinError ( ERROR_READ_FAULT );
	RegisterWinError ( ERROR_GEN_FAILURE );
	RegisterWinError ( ERROR_DRIVE_LOCKED );
	RegisterWinError ( ERROR_INVALID_NAME );
	RegisterWinError ( ERROR_ALREADY_EXISTS );
	RegisterWinError ( ERROR_DISK_FULL );
	RegisterWinError ( ERROR_DIR_NOT_EMPTY );

	m_dErrors.Add ( new ErrorDialog_c ( ERROR_SHARING_VIOLATION, true,EB_R_S_SA_C,	EB_R_S_SA_C,	EB_R_S_SA_C,	EB_R_C,	EB_R_S_SA_C,	EB_R_S_SA_C,	EB_R_S_SA_C,	EB_R_C,			EB_R_S_SA_C,	Txt ( T_ERR_SHARING ) ) );
	m_dErrors.Add ( new ErrorDialog_c ( ERROR_LOCK_VIOLATION, true,	EB_R_S_SA_C,	EB_R_S_SA_C,	EB_R_S_SA_C,	EB_R_C,	EB_R_S_SA_C,	EB_R_S_SA_C,	EB_R_S_SA_C,	EB_R_C,			EB_R_S_SA_C,	Txt ( T_ERR_LOCK ) ) );
	                                                                                                                                                                                                                        
	m_dErrors.Add ( new ErrorDialog_c ( EC_COPY_ONTO_ITSELF, false, EB_C, 			EB_C, 			EB_C,			EB_R_C, EB_R_S_SA_C, 	EB_R_S_SA_C, 	EB_R_S_SA_C,	EB_C,			EB_C,			Txt ( T_ERR_COPY_ONTO_ITSELF ) ) );
	m_dErrors.Add ( new ErrorDialog_c ( EC_MOVE_ONTO_ITSELF, false, EB_C, 			EB_C, 			EB_C,			EB_R_C, EB_R_S_SA_C, 	EB_R_S_SA_C, 	EB_R_S_SA_C,	EB_C,			EB_C,			Txt ( T_ERR_MOVE_ONTO_ITSELF ) ) );
	m_dErrors.Add ( new ErrorDialog_c ( EC_NOT_ENOUGH_SPACE, false, EB_R_S_SA_C, 	EB_C,			EB_C,			EB_R_C, EB_R_S_SA_C, 	EB_R_S_SA_C, 	EB_R_S_SA_C,	EB_R_C,			EB_R_S_SA_C,	Txt ( T_ERR_NOT_ENOUGH_SPACE ) ) );
	m_dErrors.Add ( new ErrorDialog_c ( EC_DEST_EXISTS, false, 		EB_O_OA_S_SA_C, EB_O_OA_S_SA_C, EB_O_OA_S_SA_C,	EB_R_C, EB_R_S_SA_C, 	EB_R_S_SA_C, 	EB_O_OA_S_SA_C,	EB_O_OA_S_SA_C,	EB_R_S_SA_C,	Txt ( T_ERR_DEST_EXISTS ) ) );
	m_dErrors.Add ( new ErrorDialog_c ( EC_DEST_READONLY, false, 	EB_O_OA_S_SA_C, EB_O_OA_S_SA_C, EB_O_OA_S_SA_C,	EB_R_C, EB_O_OA_S_SA_C, EB_O_OA_S_SA_C, EB_O_OA_S_SA_C,	EB_O_OA_S_SA_C,	EB_O_OA_S_SA_C,	Txt ( T_ERR_DEST_READONLY ) ) );
	m_dErrors.Add ( new ErrorDialog_c ( EC_UNKNOWN, false, 			EB_R_C, 		EB_R_C, 		EB_R_C,			EB_R_C, EB_R_C, 	 	EB_R_C, 	  	EB_R_C,			EB_R_C,			EB_R_C,			Txt ( T_ERR_UNKNOWN	) ) );
}

void OperationErrorList_c::RegisterWinError ( int iErrorCode )
{
	Str_c sError = GetWinErrorText ( iErrorCode );
	m_dErrors.Add ( new ErrorDialog_c ( iErrorCode, true, EB_R_S_SA_C, EB_R_S_SA_C, EB_R_S_SA_C, EB_R_C, EB_R_S_SA_C, EB_R_S_SA_C, EB_R_S_SA_C, EB_R_C, EB_R_S_SA_C, sError ) );
}

void OperationErrorList_c::Shutdown ()
{
	for ( int i = 0; i < m_dErrors.Length (); ++i )
		SafeDelete ( m_dErrors [i] );
}

bool OperationErrorList_c::IsInDialog () const
{
	return !!g_pCurrentError;
}

int	OperationErrorList_c::ShowErrorDialog ( HWND hWnd, int iError, bool bWinError, const Str_c & sFile1 )
{
	return ShowErrorDialog ( hWnd, iError, bWinError, sFile1, L"" );
}

int OperationErrorList_c::ShowErrorDialog ( HWND hWnd, int iError, bool bWinError, const Str_c & sFile1, const Str_c & sFile2 )
{
	for ( int i = 0; i < m_dErrors.Length (); ++i )
		if ( m_dErrors [i]->GetCode () == iError &&
			m_dErrors [i]->GetWinFlag () == bWinError )
		{
			g_pCurrentError = m_dErrors [i];
			int iRes = g_pCurrentError->ShowError ( hWnd, m_eOperationMode, sFile1, sFile2 );
			g_pCurrentError = NULL;

			return iRes;
		}

	// unknown error
	g_pCurrentError = m_dErrors [m_dErrors.Length () - 1];
	Log ( L"ShowErrorDialog: unknown error [%d, %d]", iError, bWinError );
	int iRes = g_pCurrentError->ShowError ( hWnd, m_eOperationMode, sFile1, sFile2 );
	g_pCurrentError = NULL;
	return iRes;
}

void OperationErrorList_c::SetOperationMode ( OperationMode_e eMode )
{
	m_eOperationMode = eMode;
}

void OperationErrorList_c::Reset ( OperationMode_e eMode )
{
	SetOperationMode ( eMode );
	
	for ( int i = 0; i < m_dErrors.Length (); ++i )
		m_dErrors [i]->Reset ();
}

OperationMode_e	OperationErrorList_c::GetMode () const
{
	return m_eOperationMode;
}