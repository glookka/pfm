#include "pch.h"

#include "pfm/dialogs/errors.h"
#include "pfm/resources.h"
#include "pfm/gui.h"
#include "pfm/config.h"

#include "Dlls/Resource/resource.h"
#include "aygshell.h"

class MessageDlg_c : public Dialog_Moving_c
{
public:
	MessageDlg_c ( const wchar_t * szTitle, const wchar_t * szText )
		: Dialog_Moving_c ( L"", IDM_OK_CANCEL )
		, m_sTitle	( szTitle )
		, m_sText	( szText )
	{
	}

	virtual void OnInit ()
	{
		Dialog_Moving_c::OnInit ();

		Bold ( IDC_TITLE );

		ItemTxt ( IDC_TITLE,	m_sTitle );
		ItemTxt ( IDC_TEXT,		m_sText );

		if ( GetDlgId () == IDD_ERROR_YES_NO )
		{
			HWND hTB = GetToolbar ();
			SetToolbarText ( hTB, IDOK, 	Txt ( T_TBAR_YES ) );
			SetToolbarText ( hTB, IDCANCEL, Txt ( T_TBAR_NO ) );

			Loc ( IDOK, T_TBAR_YES );
			Loc ( IDCANCEL, T_TBAR_NO );
		}
		else
		{
			Loc ( IDOK, T_TBAR_OK );
			Loc ( IDCANCEL, T_TBAR_CANCEL );
		}
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Moving_c::OnCommand ( iItem, iNotify );

		switch ( iItem )
		{
		case IDOK:
		case IDCANCEL:
			Close ( iItem );
			break;
		}
	}

private:
	Str_c	m_sTitle;
	Str_c	m_sText;
};

int DlgTypeToId ( int iDlgId )
{
	switch ( iDlgId )
	{
	case DLG_ERROR_OK:		return IDD_ERROR_OK;
	case DLG_ERROR_YES_NO:	return IDD_ERROR_YES_NO;
	}

	Assert ( 0 && "DlgTypeToId: dlg type not found" );

	return iDlgId;
}

int ShowErrorDialog ( HWND hParent, bool bWarning, const wchar_t * szText, int iDlgType )
{
	return ShowMessageDialog ( hParent, bWarning ? Txt ( T_MSG_WARNING ) : Txt ( T_MSG_ERROR ), szText, iDlgType );
}

int ShowMessageDialog ( HWND hParent, const wchar_t * szTitle, const wchar_t * szText, int iDlgType )
{
	MessageDlg_c Dlg ( szTitle, szText );
	return Dlg.Run ( DlgTypeToId (iDlgType), hParent );
}

///////////////////////////////////////////////////////////////////////////////////////////
class ErrorDlg_c : public Dialog_Moving_c
{
public:
	ErrorDlg_c ( const wchar_t * szOpName, const wchar_t * szError, 
		const wchar_t * szFile1, const wchar_t * szFile2, bool bTwoFiles, ErrorDlgType_e eType )
		: Dialog_Moving_c	( L"", IDM_CANCEL )
		, m_sOpName		( szOpName )
		, m_sError		( szError )
		, m_sFile1		( szFile1 )
		, m_sFile2		( szFile2 )
		, m_bTwoFiles	( bTwoFiles )
		, m_eDlgType	( eType )
	{
	}

	virtual void OnInit ()
	{
		Dialog_Moving_c::OnInit ();

		Bold ( IDC_OPERATION );
		ItemTxt ( IDC_OPERATION, m_sOpName );
		ItemTxt ( IDC_ERROR, m_sError );

		Loc ( IDC_TO,		T_DLG_TO );
		Loc ( IDCANCEL,		T_TBAR_CANCEL );
		Loc ( IDRETRY,		T_DLG_RETRY );
		Loc ( IDIGNORE,		T_DLG_SKIP );
		Loc ( IDC_SKIP_ALL, T_DLG_SKIP_ALL );
		Loc ( IDC_OVER,		T_DLG_OVERWRITE );
		Loc ( IDC_OVER_ALL, T_DLG_OVERWRITE_ALL );

		AlignFileName ( Item ( IDC_FILE1 ), m_sFile1 );
		if ( m_bTwoFiles )
			AlignFileName ( Item ( IDC_FILE2 ), m_sFile2 );
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Moving_c::OnCommand ( iItem, iNotify );

		switch ( iItem )
		{
		case IDIGNORE:
		case IDC_SKIP_ALL:
		case IDC_OVER:
		case IDC_OVER_ALL:
		case IDRETRY:
		case IDCANCEL:
			Close ( iItem );
			break;
		}
	}

private:
	Str_c			m_sOpName;
	Str_c			m_sOpBtn;
	Str_c			m_sError;
	Str_c			m_sFile1;
	Str_c			m_sFile2;
	bool			m_bTwoFiles;
	ErrorDlgType_e	m_eDlgType;
};

//////////////////////////////////////////////////////////////////////////

class OperationError_c
{
public:
	int				m_iErrorCode;
	bool			m_bWinError;

					OperationError_c ( int iErrorCode, bool bWinError, ErrorDlgType_e eType, bool bTwoFile, const wchar_t * szErrorText, bool bStoreText );

	ErrResponse_e	ShowErrorDialog ( HWND hParent, const wchar_t * szOpName, const wchar_t * szFile1, const wchar_t * szFile2 );
	void			Reset ();

private:
	int				m_iDialog;
	ErrorDlgType_e	m_eType;
	bool			m_bTwoFile;
	Str_c			m_sErrorText;
	const wchar_t *	m_szErrorText;
	bool			m_bAuto;
	int				m_iAutoResult;

	const wchar_t *	GetErrorText ();
};

OperationError_c::OperationError_c ( int iErrorCode, bool bWinError, ErrorDlgType_e eType, bool bTwoFile, const wchar_t * szErrorText, bool bStoreText )
	: m_iDialog		( -1 )
	, m_iErrorCode	( iErrorCode )
	, m_bWinError	( bWinError )
	, m_eType		( eType )
	, m_bTwoFile	( bTwoFile )
	, m_szErrorText	( NULL )
	, m_bAuto		( false )
	, m_iAutoResult	( -1 )
{
	if ( bStoreText )
		m_sErrorText = szErrorText;
	else
		m_szErrorText = szErrorText;

	switch ( m_eType )
	{
	case EB_C:
		m_iDialog = m_bTwoFile ? IDD_ERR_TWO_C : IDD_ERR_ONE_C;
		break;
	case EB_R_C:
		m_iDialog = m_bTwoFile ? IDD_ERR_TWO_R_C : IDD_ERR_ONE_R_C;
		break;
	case EB_S_SA_C:
		m_iDialog = m_bTwoFile ? IDD_ERR_TWO_S_SA_C : IDD_ERR_ONE_S_SA_C;
		break;
	case EB_R_S_SA_C:
		m_iDialog = m_bTwoFile ? IDD_ERR_TWO_R_S_SA_C : IDD_ERR_ONE_R_S_SA_C;
		break;
	case EB_O_OA_S_SA_C:
		m_iDialog = m_bTwoFile ? IDD_ERR_TWO_O_OA_S_SA_C : IDD_ERR_ONE_O_OA_S_SA_C;
		break;
	}

	Assert ( m_iDialog != -1 );
}

static ErrResponse_e BtnToErrorResponse ( int iBtn )
{
	switch ( iBtn )
	{
	case IDIGNORE:	return ER_SKIP;
	case IDC_OVER:	return ER_OVER;
	case IDRETRY:	return ER_RETRY;
	case IDCANCEL:	return ER_CANCEL;
	}

	Assert ( 0 && "Unknown error dialog button code" );
	return ER_CANCEL;
}

ErrResponse_e OperationError_c::ShowErrorDialog ( HWND hParent, const wchar_t * szOpName, const wchar_t * szFile1, const wchar_t * szFile2 )
{
	if ( m_bAuto )
		return BtnToErrorResponse ( m_iAutoResult );

	ErrorDlg_c Dlg ( szOpName, GetErrorText (), szFile1, szFile2, m_bTwoFile, m_eType );
	int iRes = Dlg.Run ( m_iDialog, hParent );

	if ( iRes == IDC_SKIP_ALL )
	{
		m_bAuto = true;
		m_iAutoResult = iRes = IDIGNORE;
	}

	if ( iRes == IDC_OVER_ALL )
	{
		m_bAuto = true;
		m_iAutoResult = iRes = IDC_OVER;
	}

	return BtnToErrorResponse ( iRes );
}

void OperationError_c::Reset ()
{
	m_bAuto = false;
	m_iAutoResult = -1;
}

const wchar_t *	OperationError_c::GetErrorText ()
{
	if ( m_szErrorText )
		return m_szErrorText;
	else if ( !m_sErrorText.Empty () )
		return m_sErrorText;
	else if ( m_bWinError )
		m_sErrorText = GetWinErrorText ( m_iErrorCode );

	return m_sErrorText;
}

static const int NUM_LOCALIZED_ERRORS = 2;
static int g_dLocalizedErrors [NUM_LOCALIZED_ERRORS][2] =
{
	   { ERROR_SHARING_VIOLATION,	T_ERR_SHARING }
	  ,{ ERROR_LOCK_VIOLATION,		T_ERR_LOCK }
};

//////////////////////////////////////////////////////////////////////////
class OperationErrorContainer_c
{
public:
					OperationErrorContainer_c ( const wchar_t * szOpName, ErrorDlgType_e eWinErrorDlg, bool bTwoFile );
					~OperationErrorContainer_c ();

	void			RegisterWinError ( int iErrorCode, ErrorDlgType_e eDlgType, const wchar_t * szErrorText = NULL );
	void			RegisterCustomError ( ErrorCustom_e eErrorCode, bool bTwoFile, ErrorDlgType_e eDlgType, const wchar_t * szErrorText );

	ErrResponse_e	ShowErrorDialog ( HWND hWnd, int iError, bool bWinError, const wchar_t * szFile1, const wchar_t * szFile2 );

	void			Reset ();
	static bool		IsInDialog ();

private:
	Array_T <OperationError_c *>	m_dErrors;
	Str_c			m_sOpName;
	Str_c			m_sOpBtn;
	ErrorDlgType_e	m_eWinErrorDlg;
	bool			m_bTwoFile;

	static bool		m_bInDialog;
};

bool OperationErrorContainer_c::m_bInDialog	= false;

OperationErrorContainer_c::OperationErrorContainer_c ( const wchar_t * szOpName, ErrorDlgType_e eWinErrorDlg, bool bTwoFile )
	: m_sOpName		( szOpName )
	, m_eWinErrorDlg( eWinErrorDlg )
	, m_bTwoFile	( bTwoFile )
{
}

OperationErrorContainer_c::~OperationErrorContainer_c ()
{
	for ( int i = 0; i < m_dErrors.Length (); ++i )
		delete m_dErrors [i];
}

void OperationErrorContainer_c::RegisterWinError ( int iErrorCode, ErrorDlgType_e eDlgType, const wchar_t * szErrorText )
{
	m_dErrors.Add ( new OperationError_c ( iErrorCode, true, eDlgType, m_bTwoFile, szErrorText, true ) );
}

void OperationErrorContainer_c::RegisterCustomError ( ErrorCustom_e eErrorCode, bool bTwoFile, ErrorDlgType_e eDlgType, const wchar_t * szErrorText )
{
	m_dErrors.Add ( new OperationError_c ( eErrorCode, false, eDlgType, bTwoFile, szErrorText, false ) );
}

void OperationErrorContainer_c::Reset ()
{
	for ( int i = 0; i < m_dErrors.Length (); ++i )
		m_dErrors [i]->Reset ();
}

ErrResponse_e OperationErrorContainer_c::ShowErrorDialog ( HWND hWnd, int iError, bool bWinError, const wchar_t * szFile1, const wchar_t * szFile2 )
{
	for ( int i = 0; i < m_dErrors.Length (); ++i )
		if ( m_dErrors [i]->m_iErrorCode == iError && m_dErrors [i]->m_bWinError == bWinError )
		{
			m_bInDialog = true;
			ErrResponse_e eRes = m_dErrors [i]->ShowErrorDialog ( hWnd, m_sOpName, szFile1, szFile2 );
			m_bInDialog = false;

			return eRes;
		}

	// common windows error?
	if ( bWinError )
	{
		// do we have localized text for this error?
		int iLocalizedText = -1;
		for ( int i = 0; i < NUM_LOCALIZED_ERRORS; i++ )
			if ( g_dLocalizedErrors [i][0] == iError )
			{
				iLocalizedText = g_dLocalizedErrors [i][1];
				break;
			}

			OperationError_c CommonError ( iError, true, m_eWinErrorDlg, m_bTwoFile, iLocalizedText == -1 ? NULL : Txt ( iLocalizedText ), false );

		m_bInDialog = true;
		ErrResponse_e eRes = CommonError.ShowErrorDialog ( hWnd, m_sOpName, szFile1, szFile2 );
		m_bInDialog = false;

		return eRes;
	}

	// unknown error
	Log ( L"ShowErrorDialog: unknown error [%d, %d]", iError, bWinError );
	OperationError_c UnknownError ( EC_UNKNOWN, false, EB_R_C, m_bTwoFile, Txt ( T_ERR_UNKNOWN ), false );

	m_bInDialog = true;
	ErrResponse_e eRes = UnknownError.ShowErrorDialog ( hWnd, m_sOpName, szFile1, szFile2 );
	m_bInDialog = false;

	return eRes;
}

bool OperationErrorContainer_c::IsInDialog ()
{
	return m_bInDialog;
}


//////////////////////////////////////////////////////////////////////////
// error api access
static OperationErrorContainer_c * g_pErrorContainer = NULL;

HANDLE ErrCreateOperation ( const wchar_t * szOpName, int iWinErrorDlg, bool bTwoFile )
{
	OperationErrorContainer_c * pContainer = new OperationErrorContainer_c ( szOpName, (ErrorDlgType_e)iWinErrorDlg, bTwoFile );
	return (HANDLE)pContainer;
}

void ErrDestroyOperation ( HANDLE hOperation )
{
	if ( hOperation == NULL || hOperation == INVALID_HANDLE_VALUE )
		return;

	delete ((OperationErrorContainer_c *) hOperation);
}

void ErrAddWinErrorHandler ( HANDLE hOperation, int iErrorCode, int iDlgType, const wchar_t * szErrorText )
{
	if ( hOperation == NULL || hOperation == INVALID_HANDLE_VALUE )
		return;

	OperationErrorContainer_c * pContainer = (OperationErrorContainer_c *)hOperation;
	pContainer->RegisterWinError ( iErrorCode, (ErrorDlgType_e)iDlgType, szErrorText );
}

void ErrAddCustomErrorHandler( HANDLE hOperation, bool bTwoFile, int iErrorCode, int iDlgType, const wchar_t * szErrorText )
{
	if ( hOperation == NULL || hOperation == INVALID_HANDLE_VALUE )
		return;

	OperationErrorContainer_c * pContainer = (OperationErrorContainer_c *)hOperation;
	pContainer->RegisterCustomError ( (ErrorCustom_e)iErrorCode, bTwoFile, (ErrorDlgType_e)iDlgType, szErrorText );
}

void ErrSetOperation ( HANDLE hOperation )
{
	if ( hOperation == NULL || hOperation == INVALID_HANDLE_VALUE )
	{
		g_pErrorContainer = NULL;
		return;
	}

	OperationErrorContainer_c * pContainer = (OperationErrorContainer_c *)hOperation;
	pContainer->Reset ();

	g_pErrorContainer = pContainer;
}

ErrResponse_e ErrShowErrorDlg ( HWND hParent, int iError, bool bWinError, const wchar_t * szFile1, const wchar_t * szFile2 )
{
	if ( !g_pErrorContainer )
		return ER_CANCEL;

	return g_pErrorContainer->ShowErrorDialog ( hParent, iError, bWinError, szFile1, szFile2 );
}

bool ErrIsInDialog ()
{
	return OperationErrorContainer_c::IsInDialog ();
}