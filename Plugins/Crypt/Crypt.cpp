#include "Crypt.h"

#include "encrypter.h"

PluginStartupInfo_t g_PSI;
HINSTANCE g_hInstance = NULL;

const DWORD MY_UNIQUE_TIMER_CRYPT = 0xABCDEF05;
const DWORD MAX_PWD_LEN = 512;

Config_t		g_Cfg;
HANDLE			g_hCfg			= INVALID_HANDLE_VALUE;
AlgInfo_t		g_dAlgs [TAB_SIZE];
int				g_nAlgs			= 0;
Encrypter_c *	g_pCrypter		= NULL;
HANDLE			g_hErrorHandler = NULL;

//////////////////////////////////////////////////////////////////////////
void Help ( const wchar_t * szSection )
{
	wchar_t szBuffer [128];
	wsprintf ( szBuffer, L"file:PFM_crypt.htm%s", szSection );
	CreateProcess ( L"peghelp.exe", szBuffer, NULL, NULL, FALSE, 0, NULL, NULL, NULL, NULL ); 
}


//////////////////////////////////////////////////////////////////////////
Config_t::Config_t ()
	: crypt_cipher		( 0 )
	, crypt_delete		( 0 )
	, crypt_overwrite	( 0 )
{
}

//////////////////////////////////////////////////////////////////////////
class CryptDlg_c
{
public:
	wchar_t m_szPassword [MAX_PWD_LEN];

	CryptDlg_c ( const wchar_t * szRoot, PanelItem_t ** dItems, int nItems, bool bEncrypt )
		: m_bEncrypt	( bEncrypt )
		, m_ppItems		( dItems )
		, m_nItems		( nItems )
		, m_szRoot		( szRoot )
		, m_hDlg		( NULL )
		, m_uTimer		( 0 )
	{
	}

	void Init ( HWND hDlg )
	{
		m_hDlg = hDlg;

		DlgTxt ( m_hDlg, IDC_TITLE, m_bEncrypt ?  T_DLG_ENCRYPT_HEAD : T_DLG_DECRYPT_HEAD );

		if ( m_bEncrypt )
		{
			DlgTxt ( m_hDlg, IDC_ALG_TEXT, T_DLG_CRYPT_ALGO_HEAD );
			DlgTxt ( m_hDlg, IDC_CONF_PASSWORD, T_DLG_CONF_PASSWORD );
		}

		DlgTxt ( m_hDlg, IDC_PASSWORD,			T_DLG_PASSWORD );
		DlgTxt ( m_hDlg, IDC_DELETE_CHECK,		T_DLG_DEL_ORIGINAL );
		DlgTxt ( m_hDlg, IDC_SHOW_PWD,			T_DLG_SHOW_PWD );
		DlgTxt ( m_hDlg, IDC_OVERWRITE_CHECK,	T_DLG_OVERWRITE );

		SendMessage ( GetDlgItem ( hDlg, IDC_TITLE ), WM_SETFONT, (WPARAM)g_PSI.m_hBoldFont, TRUE );
		g_PSI.m_fnDlgInitFullscreen ( m_hDlg, false );
		g_PSI.m_fnCreateToolbar ( m_hDlg, TOOLBAR_OK_CANCEL, 0 );

		m_uTimer = SetTimer ( m_hDlg, MY_UNIQUE_TIMER_CRYPT, 0, NULL );

		SendMessage ( GetDlgItem ( m_hDlg, IDC_PWD1 ), EM_LIMITTEXT, MAX_PWD_CHARS, 0 );
		SendMessage ( GetDlgItem ( m_hDlg, IDC_PWD2 ), EM_LIMITTEXT, MAX_PWD_CHARS, 0 );

		Assert ( m_ppItems );
		const int MAX_TXT = 256;
		wchar_t szText [MAX_TXT];

		if ( m_nItems == 1 )
		{
			Assert ( m_ppItems );
			wcsncpy ( szText, m_szRoot, MAX_TXT );
			wcsncat ( szText, m_ppItems[0]->m_FindData.cFileName, MAX_TXT-wcslen(szText) );
		}
		else
			wsprintf ( szText, Txt ( T_DLG_CRYPT_FILES ), m_nItems );

		g_PSI.m_fnAlignText ( GetDlgItem ( m_hDlg, IDC_FILENAME ), szText );

		// populate alg list
		HWND hAlgCombo = GetDlgItem ( hDlg, IDC_ALG_COMBO );

		if ( m_bEncrypt )
		{
			const AlgInfo_t * dAlgs = GetAlgList ();

			wchar_t szAlgBuf [128];
			wchar_t szEntry [256];

			SendMessage ( hAlgCombo, CB_RESETCONTENT, 0, 0 );
			for ( int i = 0; i < GetNumAlgs (); ++i )
			{
				AnsiToUnicode ( dAlgs [i].m_tDesc.name, szAlgBuf );
				wcsupr ( szAlgBuf );
				wsprintf ( szEntry, Txt ( T_DLG_CRYPT_ALGO ), szAlgBuf, dAlgs [i].m_iKeyLength * 8 );
				SendMessage ( hAlgCombo, CB_ADDSTRING, 0, (LPARAM) szEntry );
			}

			SendMessage ( hAlgCombo, CB_SETCURSEL, g_Cfg.crypt_cipher, 0 );
		}

		CheckDlgButton ( hDlg, IDC_DELETE_CHECK,	g_Cfg.crypt_delete ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton ( hDlg, IDC_OVERWRITE_CHECK,	g_Cfg.crypt_overwrite ? BST_CHECKED : BST_UNCHECKED );
	}

	bool Close ( bool bOk )
	{
		KillTimer ( m_hDlg, m_uTimer );
		
		if ( bOk )
		{
			wchar_t szPwd1 [MAX_PWD_CHARS + 1];
			GetDlgItemText ( m_hDlg, IDC_PWD1, szPwd1, MAX_PWD_CHARS );

			if ( m_bEncrypt )
			{
				wchar_t szPwd2 [MAX_PWD_CHARS + 1];
				GetDlgItemText ( m_hDlg, IDC_PWD2, szPwd2, MAX_PWD_CHARS );

				bool bShowPwd = IsDlgButtonChecked ( m_hDlg, IDC_SHOW_PWD )	== BST_CHECKED;

				if ( !bShowPwd && wcsncmp ( szPwd1, szPwd2, MAX_PWD_CHARS ) != 0 )
				{
					g_PSI.m_fnDialogError ( m_hDlg, false, Txt ( T_MSG_PASSWORD_NO_MATCH ), DLG_ERROR_OK );
					return false;
				}
			}

			if ( szPwd1 [0] == L'\0' )
			{
				g_PSI.m_fnDialogError ( m_hDlg, false, Txt ( T_MSG_PASSWORD_EMPTY ), DLG_ERROR_OK );
				return false;
			}

			wcscpy ( m_szPassword, szPwd1 );

			int iCurSel = SendMessage ( GetDlgItem ( m_hDlg, IDC_ALG_COMBO ), CB_GETCURSEL, 0, 0 );
			if ( iCurSel == CB_ERR )
				iCurSel = 0;

			g_Cfg.crypt_cipher = iCurSel;
			g_Cfg.crypt_delete		= IsDlgButtonChecked ( m_hDlg, IDC_DELETE_CHECK )	== BST_CHECKED;
			g_Cfg.crypt_overwrite	= IsDlgButtonChecked ( m_hDlg, IDC_OVERWRITE_CHECK )== BST_CHECKED;
		}

		g_PSI.m_fnDlgCloseFullscreen ();

		return true;
	}

	void ShowPwd ()
	{
		HWND hPwd1 = GetDlgItem ( m_hDlg, IDC_PWD1 );
		HWND hPwd2 = GetDlgItem ( m_hDlg, IDC_PWD2 );
		LONG iFlags = GetWindowLong ( hPwd1, GWL_STYLE );
		if ( iFlags & ES_PASSWORD )
		{
			SetWindowLong ( hPwd1, iFlags & ~ES_PASSWORD, GWL_STYLE );
			SendMessage ( hPwd1, EM_SETPASSWORDCHAR, 0, 0 );
			SetFocus ( hPwd1 );
			SendMessage ( hPwd2, EM_SETREADONLY, TRUE, 0 );
		}
		else
		{
			SetWindowLong ( hPwd1, iFlags | ES_PASSWORD, GWL_STYLE );
			SendMessage ( hPwd1, EM_SETPASSWORDCHAR, (WPARAM)'*', 0 );
			SetFocus ( hPwd1 );
			SendMessage ( hPwd2, EM_SETREADONLY, FALSE, 0 );
		}

		SetWindowText ( hPwd2, L"" );
	}

private:
	bool				m_bEncrypt;
	const wchar_t *		m_szRoot;
	PanelItem_t **		m_ppItems;
	int					m_nItems;
	HWND				m_hDlg;
	DWORD				m_uTimer;
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
			if ( g_pCryptDlg->Close ( LOWORD ( wParam ) == IDOK ) )
				EndDialog ( hDlg, LOWORD (wParam) );
			break;
		case IDC_SHOW_PWD:
			g_pCryptDlg->ShowPwd ();
			break;
		}
		break;

	case WM_HELP:
		Help ( L"Main_Contents" );
		return TRUE;
	}

	g_PSI.m_fnDlgHandleActivate ( hDlg, Msg, wParam, lParam );

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
// Results
static BOOL CALLBACK CryptResultsDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
		DlgTxt ( hDlg, IDC_TITLE, g_pCrypter->IsEncryption () ?  T_DLG_ENCRYPT_RESULTS_HEAD : T_DLG_DECRYPT_RESULTS_HEAD );
		SendMessage ( GetDlgItem ( hDlg, IDC_TITLE ), WM_SETFONT, (WPARAM)g_PSI.m_hBoldFont, TRUE );

		DlgTxt ( hDlg, IDC_TOTAL_STATIC,		T_DLG_ENC_TOTAL );
		DlgTxt ( hDlg, IDC_ENCRYPTED_STATIC,	g_pCrypter->IsEncryption () ? T_DLG_ENCRYPTED : T_DLG_DECRYPTED );
		DlgTxt ( hDlg, IDC_SKIPPED_STATIC,		T_DLG_SKIPPED );
		DlgTxt ( hDlg, IDC_ERRORS_STATIC,		T_DLG_ERRORS );
		DlgTxt ( hDlg, IDOK,					T_OK );

		SetDlgItemInt ( hDlg, IDC_TOTAL,		g_pCrypter->GetNumTotal (),		TRUE );
		SetDlgItemInt ( hDlg, IDC_ENCRYPTED,	g_pCrypter->GetNumEncrypted (),	TRUE );
		SetDlgItemInt ( hDlg, IDC_SKIPPED,		g_pCrypter->GetNumSkipped (),	TRUE );
		SetDlgItemInt ( hDlg, IDC_ERRORS,		g_pCrypter->GetNumErrors (),	TRUE );

		g_PSI.m_fnCreateToolbar ( hDlg, TOOLBAR_OK, SHCMBF_HIDESIPBUTTON );
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog ( hDlg, LOWORD (wParam) );
			break;
		}
		break;

	case WM_HELP:
		Help ( L"Main_Contents" );
		return TRUE;
	}

	return g_PSI.m_fnDlgMoving ( hDlg, Msg, wParam, lParam );;
}


//////////////////////////////////////////////////////////////////////////
class CryptProgressDlg_c : public WindowProgress_c
{
public:
	CryptProgressDlg_c ( bool bEncrypt )
	  : WindowProgress_c( Txt ( T_DLG_PROGRESS_SIZE ) )
	  , m_nOldTotal		( -1 )
	  , m_nOldProcessed	( -1 )
	  , m_nOldEncrypted ( -1 )
	  , m_bEncrypt		( bEncrypt )
	  , m_hTotalFiles		( NULL )
	  , m_hProcessedFiles	( NULL )
	  , m_hEncryptedFiles	( NULL )
	{
	}

	void Init ( HWND hDlg )
	{
		DlgTxt ( hDlg, IDC_TOTAL_STATIC,		T_DLG_ENC_TOTAL );
		DlgTxt ( hDlg, IDC_PROCESSED_STATIC,	T_DLG_PROCESSED );
		DlgTxt ( hDlg, IDC_ENCRYPTED_STATIC,	m_bEncrypt ? T_DLG_ENCRYPTED : T_DLG_DECRYPTED );

		m_hTotalFiles		= GetDlgItem ( hDlg, IDC_TOTAL );
		m_hProcessedFiles	= GetDlgItem ( hDlg, IDC_PROCESSED );
		m_hEncryptedFiles	= GetDlgItem ( hDlg, IDC_ENCRYPTED );

		WindowProgress_c::SetItems ( GetDlgItem ( hDlg, IDC_HEADER ), GetDlgItem ( hDlg, IDC_TOTAL_PROGRESS ), GetDlgItem ( hDlg, IDC_FILE_PROGRESS ),
			GetDlgItem ( hDlg, IDC_SOURCE_TEXT ), NULL, GetDlgItem ( hDlg, IDC_TOTAL_PERCENT ),
			GetDlgItem ( hDlg, IDC_FILE_PERCENT ), GetDlgItem ( hDlg, IDC_TOTAL_SIZE ), GetDlgItem ( hDlg, IDC_FILE_SIZE ) );

		WindowProgress_c::Init ( hDlg, m_bEncrypt ? Txt (T_DLG_ENC_FILES) : Txt (T_DLG_DEC_FILES), Txt (T_DLG_PROGRESS_SIZE), g_pCrypter );

		g_PSI.m_fnDlgInitFullscreen ( hDlg, true );
		g_PSI.m_fnCreateToolbar ( hDlg, TOOLBAR_CANCEL, SHCMBF_HIDESIPBUTTON );

		SetTimer ( hDlg, MY_UNIQUE_TIMER_CRYPT, 0, NULL );
	}

	void UpdateProgress ()
	{
		if ( g_pCrypter->GetNameFlag () )
			g_PSI.m_fnAlignText ( m_hSourceText, g_pCrypter->GetSourceFileName () );

		wchar_t szRes [32];
		if ( g_pCrypter->GetNumProcessed () != m_nOldProcessed )
		{
			_itow ( g_pCrypter->GetNumProcessed (), szRes, 10 );
			SetWindowText ( m_hProcessedFiles, szRes );
			m_nOldProcessed = g_pCrypter->GetNumProcessed ();
		}

		if ( g_pCrypter->GetNumEncrypted () != m_nOldEncrypted )
		{
			_itow ( g_pCrypter->GetNumEncrypted (), szRes, 10 );
			SetWindowText ( m_hEncryptedFiles, szRes );
			m_nOldEncrypted = g_pCrypter->GetNumEncrypted ();
		}

		if ( g_pCrypter->GetNumTotal () != m_nOldTotal )
		{
			_itow ( g_pCrypter->GetNumTotal (), szRes, 10 );
			SetWindowText ( m_hTotalFiles, szRes );
			m_nOldTotal = g_pCrypter->GetNumTotal ();
		}

		if ( WindowProgress_c::UpdateProgress ( g_pCrypter ) )
			g_pCrypter->ResetChangeFlags ();
	}

	void Close ()
	{
		g_PSI.m_fnDlgCloseFullscreen ();
	}

private:
	int		m_nOldTotal;
	int		m_nOldProcessed;
	int		m_nOldEncrypted;
	bool	m_bEncrypt;
	HWND	m_hTotalFiles;
	HWND	m_hProcessedFiles;
	HWND	m_hEncryptedFiles;
};

static CryptProgressDlg_c * g_pCryptProgressDlg = NULL;


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
			bool bFinished = false;
			const float CRYPT_CHUNK_TIME = 0.1f;
			float fStart = g_PSI.m_fnTimeSec ();

			while ( g_PSI.m_fnTimeSec () - fStart <= CRYPT_CHUNK_TIME )
			{
				if ( !g_pCrypter->IsInDialog () )
					if ( !g_pCrypter->DoWork () )
					{
						PostMessage ( hDlg, WM_CLOSE, 0, 0 );
						break;
					}

				g_pCryptProgressDlg->UpdateProgress ();
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

	g_PSI.m_fnDlgHandleActivate ( hDlg, Msg, wParam, lParam );

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
static void MarkCallback ( const wchar_t * szFileName, bool bMark )
{
	g_PSI.m_fnPanelMarkFile ( szFileName, bMark );
}


int ShowCryptDialog ( const wchar_t * szRoot, PanelItem_t ** dItems, int nItems, bool bEncrypt, wchar_t * szPassword )
{
	g_pCryptDlg = new CryptDlg_c ( szRoot, dItems, nItems, bEncrypt );
	int iRes = DialogBox ( g_hInstance, bEncrypt ? MAKEINTRESOURCE (IDD_ENCRYPT) : MAKEINTRESOURCE (IDD_DECRYPT), g_PSI.m_hMainWindow, CryptDlgProc );
	wcsncpy ( szPassword, g_pCryptDlg->m_szPassword, MAX_PWD_LEN );
	SafeDelete ( g_pCryptDlg );

	return iRes;
}

//////////////////////////////////////////////////////////////////////////
void ShowCryptProgressDlg ( const wchar_t * szRoot, PanelItem_t ** dItems, int nItems, bool bEncrypt, const wchar_t * szPassword )
{
	const AlgInfo_t & tAlg = GetAlgList () [g_Cfg.crypt_cipher];

	g_pCrypter = new Encrypter_c ( szRoot, dItems, nItems, bEncrypt, !!g_Cfg.crypt_delete, tAlg, szPassword, g_PSI.m_fnCreateWaitWnd, MarkCallback );
	g_PSI.m_fnDestroyWaitWnd ();

	g_pCryptProgressDlg = new CryptProgressDlg_c ( bEncrypt );
	DialogBox ( g_hInstance, MAKEINTRESOURCE ( IDD_ENCRYPT_PROGRESS ), g_PSI.m_hMainWindow, CryptProgressDlgProc );
	SafeDelete ( g_pCryptProgressDlg );

	DialogBox ( g_hInstance, MAKEINTRESOURCE ( IDD_CRYPT_RESULTS ), g_PSI.m_hMainWindow, CryptResultsDlgProc );
	SafeDelete ( g_pCrypter );
}


void HandleMenu ( int iSubmenuId, const wchar_t * szRoot, PanelItem_t ** dItems, int nItems )
{
	wchar_t szPassword[MAX_PWD_LEN];
	bool bEncrypt = iSubmenuId == 0;
	int iRes = ShowCryptDialog ( szRoot, dItems, nItems, bEncrypt, szPassword );
	if ( iRes != IDCANCEL )
	{
		ShowCryptProgressDlg ( szRoot, dItems, nItems, bEncrypt, szPassword );
		g_PSI.m_fnPanelSoftRefreshAll ();
	}
}


void SetStartupInfo ( const PluginStartupInfo_t & Info )
{
	g_PSI = Info;

	g_hCfg = g_PSI.m_fnCfgCreate ();
	g_PSI.m_fnCfgInt ( g_hCfg, L"crypt_cipher",		&g_Cfg.crypt_cipher );
	g_PSI.m_fnCfgInt ( g_hCfg, L"crypt_delete",		&g_Cfg.crypt_delete );
	g_PSI.m_fnCfgInt ( g_hCfg, L"crypt_overwrite",	&g_Cfg.crypt_overwrite );

	wchar_t szConfig [MAX_PATH];
	wsprintf ( szConfig, L"%scrypt.ini", g_PSI.m_szRoot );
	g_PSI.m_fnCfgLoad ( g_hCfg, szConfig );

	g_hErrorHandler = g_PSI.m_fnErrCreateOperation ( Txt (T_OP_HEAD_CRYPT), EB_R_S_SA_C, true );
	g_PSI.m_fnErrAddCustomErrorHandler ( g_hErrorHandler, true,		EC_DEST_EXISTS,			EB_O_OA_S_SA_C, Txt ( T_ERR_DEST_EXISTS ) );
	g_PSI.m_fnErrAddCustomErrorHandler ( g_hErrorHandler, false,	EC_ALREADY_ENCRYPTED,	EB_S_SA_C,		Txt ( T_ERR_ALREADY_ENCRYPTED ) );
	g_PSI.m_fnErrAddCustomErrorHandler ( g_hErrorHandler, false,	EC_NOT_ENCRYPTED,		EB_S_SA_C,		Txt ( T_ERR_NOT_ENCRYPTED ) );
	g_PSI.m_fnErrAddCustomErrorHandler ( g_hErrorHandler, false,	EC_INVPASS,				EB_S_SA_C,		Txt ( T_ERR_INVPASS ) );
	g_PSI.m_fnErrAddCustomErrorHandler ( g_hErrorHandler, true,		EC_READ,				EB_S_SA_C,		Txt ( T_ERR_READ ) );
	g_PSI.m_fnErrAddCustomErrorHandler ( g_hErrorHandler, true,		EC_WRITE,				EB_S_SA_C,		Txt ( T_ERR_WRITE ) );
	g_PSI.m_fnErrAddCustomErrorHandler ( g_hErrorHandler, true,		EC_ENCRYPT,				EB_S_SA_C,		Txt ( T_ERR_ENCRYPT ) );
	g_PSI.m_fnErrAddCustomErrorHandler ( g_hErrorHandler, true,		EC_DECRYPT,				EB_S_SA_C,		Txt ( T_ERR_DECRYPT ) );	
}

static int GetForcedKeyLen ( int idx )
{
	switch ( idx )
	{
	case 0: return 32;	//aes
	case 1:	return 32;	//blowfish
	case 2:	return 32;	//twofish
	case 3:	return 8;	//des
	case 4:	return 24;	//des3
	case 5:	return 16;	//cast5
	case 6:	return 16;	//rc2
	case 7:	return 16;	//rc5
	case 8:	return 16;	//rc6
	}

	return 0;
}

void EnumerateAlgs ()
{
	g_nAlgs = 0;
	for ( int i = 0; i < TAB_SIZE; ++i )
		if ( cipher_is_valid ( i ) == CRYPT_OK )
		{
			g_dAlgs [g_nAlgs].m_tDesc = cipher_descriptor [i];
			g_dAlgs [g_nAlgs].m_iKeyLength = GetForcedKeyLen ( i );
			g_dAlgs [g_nAlgs].m_iId = i;
			g_nAlgs++;
		}
}

const AlgInfo_t * GetAlgList ()
{
	return g_dAlgs;
}

int GetNumAlgs ()
{
	return g_nAlgs;
}

void ExitPFM ()
{
	g_PSI.m_fnCfgSave ( g_hCfg );
	g_PSI.m_fnCfgDestroy ( g_hCfg );

	g_PSI.m_fnErrDestroyOperation ( g_hErrorHandler );
}

BOOL WINAPI DllMain ( HANDLE hinstDLL, DWORD dwReason, LPVOID lpvReserved )
{
	g_hInstance = (HINSTANCE)hinstDLL;

	LARGE_INTEGER iTime;
	QueryPerformanceCounter ( &iTime );
	srand ( iTime.LowPart );

	// register all available ciphers
	register_cipher (&aes_desc);
	register_cipher (&blowfish_desc);
	register_cipher (&twofish_desc);
	register_cipher (&des_desc);
	register_cipher (&des3_desc);
	register_cipher (&cast5_desc);
	register_cipher (&rc2_desc);
	register_cipher (&rc5_desc);
	register_cipher (&rc6_desc);

	EnumerateAlgs ();

	return TRUE;
}