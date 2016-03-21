#include "pch.h"

#include "LDialogs/dcrypt.h"
#include "LCore/clog.h"
#include "LSettings/sconfig.h"
#include "LSettings/slocal.h"
#include "LCrypt/cproviders.h"
#include "LFile/fcrypt.h"
#include "LPanel/pbase.h"
#include "LPanel/presources.h"
#include "LDialogs/derrors.h"
#include "LUI/udialogs.h"
#include "LDialogs/dcommon.h"

#include "aygshell.h"
#include "Resources/resource.h"

extern HWND g_hMainWindow;

const int MAX_PWD_CHARS = 128;

Panel_c *	g_pSourcePanel	= NULL;
Encrypter_c * g_pCrypter	= NULL;

const DWORD MY_UNIQUE_TIMER_CRYPT = 0xABCDEF05;

//////////////////////////////////////////////////////////////////////////
class CryptDlg_c
{
public:
	CryptDlg_c ( bool bEncrypt, FileList_t & tList )
		: m_bEncrypt	( bEncrypt )
		, m_pList		( &tList )
		, m_hAlgCombo	( NULL )
	{
	}

	void Init ( HWND hDlg )
	{
		DlgTxt ( hDlg, IDC_TITLE, m_bEncrypt ?  T_DLG_ENCRYPT_HEAD : T_DLG_DECRYPT_HEAD );
		if ( m_bEncrypt )
		{
			DlgTxt ( hDlg, IDC_ALG_TEXT, T_DLG_CRYPT_ALGO_HEAD );
			DlgTxt ( hDlg, IDC_CONF_PASSWORD, T_DLG_CONF_PASSWORD );
		}

		DlgTxt ( hDlg, IDC_PASSWORD, T_DLG_PASSWORD );
		DlgTxt ( hDlg, IDC_DELETE_CHECK, T_DLG_DEL_ORIGINAL );


		SendMessage ( GetDlgItem ( hDlg, IDC_TITLE ), WM_SETFONT, (WPARAM)g_tResources.m_hBoldSystemFont, TRUE );

		InitFullscreenDlg ( hDlg, IDM_OK_CANCEL, 0 );

		SendMessage ( GetDlgItem ( hDlg, IDC_PWD1 ), EM_LIMITTEXT, MAX_PWD_CHARS, 0 );
		SendMessage ( GetDlgItem ( hDlg, IDC_PWD2 ), EM_LIMITTEXT, MAX_PWD_CHARS, 0 );

		Str_c sText;
		Assert ( m_pList );

		if ( ! m_pList->OneFile () )
			sText = NewString ( Txt ( T_DLG_CRYPT_FILES ), m_pList->m_dFiles.Length () );
		else
		{
			const FileInfo_t * pInfo = m_pList->m_dFiles [0];
			Assert ( pInfo );
			sText = m_pList->m_sRootDir + pInfo->m_tData.cFileName;
		}

		AlignFileName ( GetDlgItem ( hDlg, IDC_FILENAME ), sText );

		// populate alg list
		m_hAlgCombo = GetDlgItem ( hDlg, IDC_ALG_COMBO );

		if ( m_bEncrypt )
		{
			const crypt::AlgList_t & dAlgs = crypt::GetAlgList ();

			wchar_t szAlgBuf [128];

			SendMessage ( m_hAlgCombo, CB_RESETCONTENT, 0, 0 );
			for ( int i = 0; i < dAlgs.Length (); ++i )
			{
				AnsiToUnicode ( dAlgs [i].m_tDesc.name, szAlgBuf );
				Str_c sAlgoName = szAlgBuf;
				Str_c sEntry = NewString ( Txt ( T_DLG_CRYPT_ALGO ), sAlgoName.ToUpper ().c_str (), dAlgs [i].m_iKeyLength * 8 );
				SendMessage ( m_hAlgCombo, CB_ADDSTRING, 0, (LPARAM) ( sEntry.c_str () ) );
			}

			SendMessage ( m_hAlgCombo, CB_SETCURSEL, g_tConfig.crypt_cipher, 0 );
		}

		CheckDlgButton ( hDlg, IDC_DELETE_CHECK, g_tConfig.crypt_delete ? BST_CHECKED : BST_UNCHECKED );
	}

	bool Close ( HWND hDlg, bool bOk )
	{
		if ( bOk )
		{
			wchar_t szPwd1 [MAX_PWD_CHARS + 1];
			GetDlgItemText ( hDlg, IDC_PWD1, szPwd1, MAX_PWD_CHARS );

			if ( m_bEncrypt )
			{
				wchar_t szPwd2 [MAX_PWD_CHARS + 1];
				GetDlgItemText ( hDlg, IDC_PWD2, szPwd2, MAX_PWD_CHARS );

				if ( wcsncmp ( szPwd1, szPwd2, MAX_PWD_CHARS ) != 0 )
				{
					ShowErrorDialog ( hDlg, Txt ( T_MSG_ERROR ), Txt ( T_MSG_PASSWORD_NO_MATCH ), IDD_ERROR_OK );
					return false;
				}
			}

			if ( szPwd1 [0] == L'\0' )
			{
				ShowErrorDialog ( hDlg, Txt ( T_MSG_ERROR ), Txt ( T_MSG_PASSWORD_EMPTY ), IDD_ERROR_OK );
				return false;
			}

			m_sPassword = szPwd1;
					
			int iCurSel = SendMessage ( m_hAlgCombo, CB_GETCURSEL, 0, 0 );
			if ( iCurSel == CB_ERR )
				iCurSel = 0;

			g_tConfig.crypt_cipher = iCurSel;
			g_tConfig.crypt_delete = IsDlgButtonChecked ( hDlg, IDC_DELETE_CHECK ) == BST_CHECKED;
		}

		CloseFullscreenDlg ();

		return true;
	}

	const Str_c & GetPassword ()
	{
		return m_sPassword;
	}

private:
	bool			m_bEncrypt;
	FileList_t *	m_pList;
	Str_c	m_sPassword;
	HWND			m_hAlgCombo;
};

CryptDlg_c * g_pCryptDlg = NULL;

static BOOL CALLBACK CryptDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	Assert ( g_pCryptDlg );

	switch ( Msg )
	{
	case WM_INITDIALOG:
		g_pCryptDlg->Init ( hDlg );
	    break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			if ( g_pCryptDlg->Close ( hDlg, LOWORD ( wParam ) == IDOK ) )
				EndDialog ( hDlg, LOWORD (wParam) );
			break;
		}
		break;

	case WM_HELP:
		Help ( L"Encryption" );
		return TRUE;
	}

	HandleActivate ( hDlg, Msg, wParam, lParam );

	return FALSE;
}

int ShowCryptDialog ( Panel_c * pSource, bool bEncrypt, FileList_t & tList, Str_c & sPassword )
{
	if ( ! crypt::LoadLibrary () )
	{
		ShowErrorDialog ( g_hMainWindow, Txt ( T_MSG_ERROR ), NewString ( Txt ( T_MSG_ERROR_LOADING ), L"Crypt.dll" ), IDD_ERROR_OK );
		return IDCANCEL;
	}

	Assert ( ! g_pCryptDlg );
	g_pCryptDlg = new CryptDlg_c ( bEncrypt, tList );
	int iRes = DialogBox ( ResourceInstance (), bEncrypt ? MAKEINTRESOURCE (IDD_ENCRYPT) : MAKEINTRESOURCE (IDD_DECRYPT), g_hMainWindow, CryptDlgProc );
	sPassword = g_pCryptDlg->GetPassword ();
	SafeDelete ( g_pCryptDlg );

	return iRes;
}


//////////////////////////////////////////////////////////////////////////
// Results
class CryptResultsDlg_c : public WindowResizer_c
{
public:
	CryptResultsDlg_c ( Encrypter_c * pEncrypter, bool bEncrypt )
		: m_hList		( NULL )
		, m_pEncrypter	( pEncrypter )
		, m_bEncrypt	( bEncrypt )
	{
	}

	void Init ( HWND hDlg )
	{
		DlgTxt ( hDlg, IDC_TITLE, T_DLG_CRYPT_ERRORS );

		SendMessage ( GetDlgItem ( hDlg, IDC_TITLE ), WM_SETFONT, (WPARAM)g_tResources.m_hBoldSystemFont, TRUE );

		m_hList = GetDlgItem ( hDlg, IDC_FILE_LIST );

		// init list
		ListView_SetExtendedListViewStyle ( m_hList, LVS_EX_FULLROWSELECT );

		LVCOLUMN tColumn;
		ZeroMemory ( &tColumn, sizeof ( tColumn ) );
		tColumn.mask = LVCF_TEXT | LVCF_SUBITEM;
		tColumn.iSubItem = 0;
		tColumn.pszText = (wchar_t *)Txt ( T_DLG_COLUMN_NAME );

		ListView_InsertColumn ( m_hList, tColumn.iSubItem,  &tColumn );

		++tColumn.iSubItem;
		tColumn.pszText = (wchar_t *)Txt ( T_DLG_COLUMN_ERROR );
		ListView_InsertColumn ( m_hList, tColumn.iSubItem,  &tColumn );

		ListView_SetImageList ( m_hList, g_tResources.m_hSmallIconList, LVSIL_SMALL );

		PopulateList ();

		InitFullscreenDlg ( hDlg, IDM_OK, SHCMBF_HIDESIPBUTTON );

		SetDlg ( hDlg );
		SetResizer ( m_hList );
		if ( HandleSizeChange ( 0, 0, 0, 0, true ) )
			Resize ();
	}

	void Shutdown ()
	{
		ListView_SetImageList ( m_hList, NULL, LVSIL_SMALL );
		CloseFullscreenDlg ();
	}

	void Resize ()
	{
		WindowResizer_c::Resize ();

		ListView_SetColumnWidth ( m_hList, 0, GetColumnWidthRelative ( 0.5f ) );
		ListView_SetColumnWidth ( m_hList, 1, GetColumnWidthRelative ( 0.5f ) );
	}

private:
	HWND					m_hList;
	Encrypter_c *			m_pEncrypter;
	bool					m_bEncrypt;

	void PopulateList ()
	{
		Assert ( m_pEncrypter );
		int nResults = m_pEncrypter->GetNumResults ();

		LVITEM tItem;
		ZeroMemory ( &tItem, sizeof ( tItem ) );
		tItem.mask = LVIF_TEXT | LVIF_IMAGE;

		Str_c sFileName;
		for ( int i = 0; i < nResults; ++i )
			if ( m_pEncrypter->GetResult ( i ) != Encrypter_c::ERROR_NONE )
			{
				const Str_c & sDir = m_pEncrypter->GetResultDir ( i );
				const WIN32_FIND_DATA & tData = m_pEncrypter->GetResultData ( i );

				// retrieve file type icon
				SHFILEINFO tShFileInfo;
				tShFileInfo.iIcon = -1;

				if ( sDir.Empty () )
					sFileName = tData.cFileName;
				else
					sFileName = sDir + L"\\" + tData.cFileName;

				SHGetFileInfo ( sFileName, 0, &tShFileInfo, sizeof (SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON );

				tItem.pszText = (wchar_t *)&tData.cFileName;
				tItem.iItem = i;
				tItem.iImage = tShFileInfo.iIcon;

				int iItem = ListView_InsertItem ( m_hList, &tItem );
				ListView_SetItemText( m_hList, iItem, 1, (wchar_t *)m_pEncrypter->GetResultString ( i ) );
			}
	}
};

static CryptResultsDlg_c * g_pCryptResultsDlg = NULL;

static BOOL CALLBACK ResultsDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	Assert ( g_pCryptResultsDlg );

	switch ( Msg )
	{
	case WM_INITDIALOG:
		g_pCryptResultsDlg->Init ( hDlg );
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			SipShowIM ( SIPF_OFF );
			g_pCryptResultsDlg->Shutdown ();
			EndDialog ( hDlg, LOWORD (wParam) );
			break;
		}
		break;

	case WM_HELP:
		Help ( L"Encryption" );
		return TRUE;
	}

	if ( HandleSizeChange ( hDlg, Msg, wParam, lParam ) )
		g_pCryptResultsDlg->Resize ();

	HandleActivate ( hDlg, Msg, wParam, lParam );

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CryptProgressDlg_c : public WindowProgress_c
{
public:
	CryptProgressDlg_c ( bool bEncrypt )
		: m_nOldFiles			( -1 )
		, m_nOldProcessed		( -1 )
		, m_bEncrypt			( bEncrypt )
		, m_hNumFiles			( NULL )
		, m_hNumProcessed		( NULL )
	{
	}

	void Init ( HWND hDlg )
	{
		DlgTxt ( hDlg, IDC_TOTAL_STATIC, T_DLG_ENC_TOTAL );
		DlgTxt ( hDlg, IDC_NUM_ENC_TEXT, m_bEncrypt ? T_DLG_ENCRYPTED : T_DLG_DECRYPTED );

		m_hNumFiles			= GetDlgItem ( hDlg, IDC_TOTAL );
		m_hNumProcessed		= GetDlgItem ( hDlg, IDC_PROCESSED );

		WindowProgress_c::Init ( hDlg, m_bEncrypt ? Txt ( T_DLG_ENC_FILES ) : Txt ( T_DLG_DEC_FILES ), g_pCrypter );

		InitFullscreenDlg ( hDlg, IDM_CANCEL, SHCMBF_HIDESIPBUTTON, true );

		SetTimer ( hDlg, MY_UNIQUE_TIMER_CRYPT, 0, NULL );
	}

	void UpdateProgress ()
	{
		if ( g_pCrypter->GetNameFlag () )
			AlignFileName ( m_hSourceText, g_pCrypter->GetSourceFileName () );

		wchar_t szRes [32];
		if ( g_pCrypter->GetNumFiles () != m_nOldFiles )
		{
			_itow ( g_pCrypter->GetNumFiles (), szRes, 10 );
			SetWindowText ( m_hNumFiles, szRes );
			m_nOldFiles = g_pCrypter->GetNumFiles ();
		}

		if ( g_pCrypter->GetNumProcessed () != m_nOldProcessed )
		{
			_itow ( g_pCrypter->GetNumProcessed  (), szRes, 10 );
			SetWindowText ( m_hNumProcessed, szRes );
			m_nOldProcessed = g_pCrypter->GetNumProcessed ();
		}

		if ( WindowProgress_c::UpdateProgress ( g_pCrypter ) )
			g_pCrypter->ResetChangeFlags ();
	}

	void Close ()
	{
		CloseFullscreenDlg ();
	}

private:
	bool	m_bEncrypt;
	HWND	m_hNumFiles;
	HWND	m_hNumProcessed;

	int		m_nOldFiles;
	int		m_nOldProcessed;
};

CryptProgressDlg_c * g_pCryptProgressDlg = NULL;


static BOOL CALLBACK CryptProgressDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	Assert ( g_pCryptProgressDlg && g_pCrypter );
	
	switch ( Msg )
	{
	case WM_INITDIALOG:
		g_pCryptProgressDlg->Init ( hDlg );
		g_pCrypter->SetWindow ( hDlg );
		break;

	case WM_TIMER:
		if ( wParam == MY_UNIQUE_TIMER_CRYPT )
		{
			if ( ! g_pCrypter->IsInDialog () )
			{
				if ( g_pCrypter->DoWork () )
					g_pCryptProgressDlg->UpdateProgress ();
				else
					PostMessage ( hDlg, WM_CLOSE, 0, 0 );
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			g_pCrypter->Cancel ();
			g_pCryptProgressDlg->Close ();
			EndDialog ( hDlg, LOWORD (wParam) );
			break;
		}
		break;

	case WM_HELP:
		Help ( L"Encryption" );
		return TRUE;
	}

	HandleActivate ( hDlg, Msg, wParam, lParam );

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void MarkCallback ( const Str_c & sFileName, bool bMark )
{	
	Assert ( g_pSourcePanel );
	g_pSourcePanel->MarkFile ( sFileName, bMark );
}

//////////////////////////////////////////////////////////////////////////
void ShowCryptProgressDlg ( Panel_c * pSource, bool bEncrypt, FileList_t & tList, const Str_c & sPassword )
{
	g_pSourcePanel = pSource;

	const crypt::AlgInfo_t & tInfo = crypt::GetAlgList () [g_tConfig.crypt_cipher];

	g_pCrypter = new Encrypter_c ( tList, bEncrypt, g_tConfig.crypt_delete, tInfo, sPassword, PrepareCallback, MarkCallback );
	DestroyPrepareDialog ();

	g_pCryptProgressDlg = new CryptProgressDlg_c ( bEncrypt );
	DialogBox ( ResourceInstance (), MAKEINTRESOURCE ( IDD_ENCRYPT_PROGRESS ), g_hMainWindow, CryptProgressDlgProc );
	SafeDelete ( g_pCryptProgressDlg );

	if ( g_pCrypter->GetNumResults () )
	{
		Assert ( ! g_pCryptResultsDlg );
		g_pCryptResultsDlg = new CryptResultsDlg_c ( g_pCrypter, bEncrypt );
		DialogBox ( ResourceInstance (), MAKEINTRESOURCE ( IDD_CRYPT_RESULTS ), g_hMainWindow, ResultsDlgProc );
		SafeDelete ( g_pCryptResultsDlg );
	}

	SafeDelete ( g_pCrypter );
}