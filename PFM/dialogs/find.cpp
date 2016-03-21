#include "pch.h"
#include "find.h"

#include "pfm/config.h"
#include "pfm/gui.h"
#include "pfm/resources.h"

#include "aygshell.h"
#include "Dlls/Resource/resource.h"

extern HWND g_hMainWindow;

#pragma warning( disable : 4512 )		// we dont need assignment operators

class FindDlg_c
{
public:
	FindDlg_c ( FileSearchSettings_t & tSettings )
		: m_tSettings		( tSettings )
		, m_hMaskList		( NULL )
		, m_hTextList		( NULL )
		, m_hCaseCheck		( NULL )
		, m_hWholeCheck		( NULL )
		, m_hFindSizeCombo1	( NULL )
		, m_hFindSizeCombo2	( NULL )
		, m_hTimeTypeCombo	( NULL )
		, m_hOlderCombo		( NULL )
		, m_hPicker1		( NULL )
		, m_hPicker2		( NULL )
	{	
	}


	void InitCommonPage ( HWND hDlg )
	{
		DlgTxt ( hDlg, IDC_MASK_STATIC, T_DLG_FIND_MASK );
		DlgTxt ( hDlg, IDC_FIND_TEXT_CHECK, T_DLG_FIND_TEXT );
		DlgTxt ( hDlg, IDC_FIND_CASE_CHECK, T_DLG_FIND_CASE );
		DlgTxt ( hDlg, IDC_FIND_WHOLE_CHECK, T_DLG_FIND_WHOLE );
		DlgTxt ( hDlg, IDC_AREA_STATIC, T_DLG_FIND_AREA );
		DlgTxt ( hDlg, IDC_FIND_ROOT, T_DLG_FIND_ROOT );
		DlgTxt ( hDlg, IDC_FIND_CURFOLDER, T_DLG_FIND_CURFOLDER );
		DlgTxt ( hDlg, IDC_FIND_SELFOLDER, T_DLG_FIND_SELECTED );
		DlgTxt ( hDlg, IDC_SEARCH_SUBFOLDERS, T_DLG_FIND_SUBFOLDERS );
		DlgTxt ( hDlg, IDC_SEARCH_FOR_FOLDERS, T_DLG_FIND_FOLDERS );

		m_hMaskList = GetDlgItem ( hDlg, IDC_FIND_MASK );
		m_hTextList = GetDlgItem ( hDlg, IDC_FIND_TEXT );
		m_hCaseCheck= GetDlgItem ( hDlg, IDC_FIND_CASE_CHECK );
		m_hWholeCheck= GetDlgItem ( hDlg, IDC_FIND_WHOLE_CHECK );

		CheckDlgButton ( hDlg, IDC_FIND_TEXT_CHECK, cfg->find_text_check ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton ( hDlg, IDC_FIND_CASE_CHECK, cfg->find_case_check ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton ( hDlg, IDC_FIND_WHOLE_CHECK, cfg->find_whole_check ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton ( hDlg, IDC_SEARCH_FOR_FOLDERS, cfg->find_folders ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton ( hDlg, IDC_SEARCH_SUBFOLDERS, cfg->find_recursive ? BST_CHECKED : BST_UNCHECKED );

		CheckRadioButton ( hDlg, IDC_FIND_CURFOLDER, IDC_FIND_SELFOLDER, cfg->find_area + IDC_FIND_CURFOLDER );
		EnableTextControls ( hDlg, cfg->find_text_check );

		PopulateMaskList ();
		PopulateSearchText ();
	}


	void InitSizeDatePage ( HWND hDlg )
	{
		DlgTxt ( hDlg, IDC_SIZE_STATIC, T_DLG_FIND_SIZE );
		DlgTxt ( hDlg, IDC_FIND_SIZE_GREATER_CHK, T_DLG_FIND_GREATER );
		DlgTxt ( hDlg, IDC_FIND_SIZE_LESS_CHK, T_DLG_FIND_LESS );
		DlgTxt ( hDlg, IDC_TIMEDATE_STATIC, T_DLG_FIND_TIMEDATE );
		DlgTxt ( hDlg, IDC_FIND_OLDER_CHECK, T_DLG_FIND_OLDER );
		DlgTxt ( hDlg, IDC_FIND_DATE_CHECK, T_DLG_FIND_TIME );
		DlgTxt ( hDlg, IDC_BETWEEN_STATIC, T_DLG_FIND_BETWEEN );

		const int NUM_SIZES = 3;
		const wchar_t * dSizes [NUM_SIZES] = { Txt ( T_BYTES ), Txt ( T_KBYTES ), Txt ( T_MBYTES ) };

		const int NUM_DATES = 5;
		const wchar_t * dDates [NUM_DATES] = { Txt ( T_MINUTES ), Txt ( T_HOURS ), Txt ( T_DAYS ), Txt ( T_MONTHS ),  Txt ( T_YEARS ) };

		const int NUM_TYPES = 3;
		const wchar_t * dTypes [NUM_TYPES] = { Txt ( T_MNU_SORT_TIME_CREATE ), Txt ( T_MNU_SORT_TIME_ACCESS ), Txt ( T_MNU_SORT_TIME_WRITE ) };

		// size
		m_hFindSizeCombo1 = GetDlgItem ( hDlg, IDC_FIND_SIZE_COMBO_1 );
		m_hFindSizeCombo2 = GetDlgItem ( hDlg, IDC_FIND_SIZE_COMBO_2 );

		for ( int i = 0; i < NUM_SIZES; ++i )
		{
			SendMessage ( m_hFindSizeCombo1, CB_ADDSTRING, 0, (LPARAM)( dSizes [i] ) );
			SendMessage ( m_hFindSizeCombo2, CB_ADDSTRING, 0, (LPARAM)( dSizes [i] ) );
		}

		SendMessage ( m_hFindSizeCombo1, CB_SETCURSEL, 1, 0 );
		SendMessage ( m_hFindSizeCombo2, CB_SETCURSEL, 1, 0 );

		SetDlgItemInt ( hDlg, IDC_FIND_SIZE1, 0, TRUE );
		SetDlgItemInt ( hDlg, IDC_FIND_SIZE2, 0, TRUE );

		EnableSizeControls ( hDlg, false, false );

		// type
		m_hTimeTypeCombo = GetDlgItem ( hDlg, IDC_FIND_DATE_TYPE );
		for ( int i = 0; i < NUM_TYPES; ++i )
			SendMessage ( m_hTimeTypeCombo, CB_ADDSTRING, 0, (LPARAM)( dTypes [i] ) );

		SendMessage ( m_hTimeTypeCombo, CB_SETCURSEL, 2, 0 );

		// older
		m_hOlderCombo = GetDlgItem ( hDlg, IDC_FIND_DATE_OLDER_COMBO );

		for ( int i = 0; i < NUM_DATES; ++i )
			SendMessage ( m_hOlderCombo, CB_ADDSTRING, 0, (LPARAM)( dDates [i] ) );

		SendMessage ( m_hOlderCombo, CB_SETCURSEL, 2, 0 );

		SetDlgItemInt ( hDlg, IDC_FIND_DATE_OLDER, 0, TRUE );

		EnableOlderControls ( hDlg, false );

		// date
		m_hPicker1 = GetDlgItem ( hDlg, IDC_DATETIMEPICKER1 );
		m_hPicker2 = GetDlgItem ( hDlg, IDC_DATETIMEPICKER2 );

		EnableDateControls ( false );
	}

	
	void InitAttributesPage ( HWND hDlg )
	{
		DlgTxt ( hDlg, IDC_FIND_ATTRIBUTES_CHECK, T_DLG_FIND_ATTRIB );
		DlgTxt ( hDlg, IDC_FIND_ARCHIVE, T_DLG_ATTR_ARCHIVE );
		DlgTxt ( hDlg, IDC_FIND_READONLY, T_DLG_ATTR_READONLY );
		DlgTxt ( hDlg, IDC_FIND_HIDDEN, T_DLG_ATTR_HIDDEN );
		DlgTxt ( hDlg, IDC_FIND_SYSTEM, T_DLG_ATTR_SYSTEM );
		DlgTxt ( hDlg, IDC_FIND_ROM, T_DLG_ATTR_ROM );

		for ( int i = 0; i < IDC_FIND_ROM - IDC_FIND_ARCHIVE + 1; ++i )
			CheckDlgButton ( hDlg, i + IDC_FIND_ARCHIVE, BST_INDETERMINATE );

		EnableAttrControls ( hDlg, false );
	}


	void UpdateEnabledControls0 ( HWND hDlg )
	{
		cfg->find_text_check = IsDlgButtonChecked ( hDlg, IDC_FIND_TEXT_CHECK ) == BST_CHECKED;
		EnableTextControls ( hDlg, cfg->find_text_check );
	}


	void UpdateEnabledControls1 ( HWND hDlg )
	{
		EnableSizeControls ( hDlg, IsDlgButtonChecked ( hDlg, IDC_FIND_SIZE_GREATER_CHK ) == BST_CHECKED
								 , IsDlgButtonChecked ( hDlg, IDC_FIND_SIZE_LESS_CHK ) == BST_CHECKED );

		EnableOlderControls ( hDlg, IsDlgButtonChecked ( hDlg, IDC_FIND_OLDER_CHECK ) == BST_CHECKED );
		EnableDateControls ( IsDlgButtonChecked ( hDlg, IDC_FIND_DATE_CHECK ) == BST_CHECKED );
	}


	void UpdateEnabledControls2 ( HWND hDlg )
	{
		EnableAttrControls ( hDlg, IsDlgButtonChecked ( hDlg, IDC_FIND_ATTRIBUTES_CHECK ) == BST_CHECKED );
	}


	void CloseCommonPage ( HWND hDlg )
	{
		cfg->find_text_check	= IsDlgButtonChecked ( hDlg, IDC_FIND_TEXT_CHECK ) == BST_CHECKED;
		cfg->find_case_check 	= IsDlgButtonChecked ( hDlg, IDC_FIND_CASE_CHECK ) == BST_CHECKED;
		cfg->find_whole_check 	= IsDlgButtonChecked ( hDlg, IDC_FIND_WHOLE_CHECK ) == BST_CHECKED;
		cfg->find_folders		= IsDlgButtonChecked ( hDlg, IDC_SEARCH_FOR_FOLDERS ) == BST_CHECKED;
		cfg->find_recursive	= IsDlgButtonChecked ( hDlg, IDC_SEARCH_SUBFOLDERS ) == BST_CHECKED;

		cfg->find_area	= 0;
		for ( int i = IDC_FIND_CURFOLDER; i <= IDC_FIND_SELFOLDER; ++i )
			if ( IsDlgButtonChecked ( hDlg, i ) == BST_CHECKED )
			{
				cfg->find_area = i - IDC_FIND_CURFOLDER;
				break;
			}

		wchar_t szBuf [256];
		GetWindowText ( m_hMaskList, szBuf, 256 );
		m_tSettings.m_sFilter = szBuf;
		GetWindowText ( m_hTextList, szBuf, 256 );
		m_tSettings.m_sContains = szBuf;

		m_tSettings.m_uSearchFlags |= cfg->find_text_check ? SEARCH_STRING : 0;
		m_tSettings.m_uSearchFlags |= cfg->find_case_check ? SEARCH_MATCH_CASE : 0;
		m_tSettings.m_uSearchFlags |= cfg->find_whole_check ? SEARCH_WHOLE_WORD : 0;
		m_tSettings.m_uSearchFlags |= cfg->find_folders ? SEARCH_FOLDERS : 0;
		m_tSettings.m_uSearchFlags |= cfg->find_recursive ? SEARCH_RECURSIVE : 0;
		m_tSettings.m_eArea = SearchArea_e ( cfg->find_area );

		g_tRecent.AddFindMask ( m_tSettings.m_sFilter );

		if ( cfg->find_text_check )
			g_tRecent.AddFindText ( m_tSettings.m_sContains );
	}


	void CloseSizeDatePage ( HWND hDlg )
	{
		bool bOlderChecked = IsDlgButtonChecked ( hDlg, IDC_FIND_OLDER_CHECK ) == BST_CHECKED;
		bool bDateChecked = IsDlgButtonChecked ( hDlg, IDC_FIND_DATE_CHECK ) == BST_CHECKED;
		bool bSizeGreaterChecked = IsDlgButtonChecked ( hDlg, IDC_FIND_SIZE_GREATER_CHK ) == BST_CHECKED;
		bool bSizeLessChecked = IsDlgButtonChecked ( hDlg, IDC_FIND_SIZE_LESS_CHK ) == BST_CHECKED;

		int iCombo = SendMessage ( m_hTimeTypeCombo, CB_GETCURSEL, 0, 0 );
		Assert ( iCombo != CB_ERR );
		m_tSettings.m_eTimeType = SearchTime_e ( iCombo );

		// date stuff
		FILETIME tOlderTime;
		memset ( &tOlderTime, 0, sizeof ( tOlderTime ) );

		if ( bOlderChecked )
		{
			m_tSettings.m_uSearchFlags |= SEARCH_DATE_1;

			int iSelected = SendMessage ( m_hOlderCombo, CB_GETCURSEL, 0, 0 );
			Assert ( iSelected != CB_ERR );

			int iAmount = GetDlgItemInt ( hDlg, IDC_FIND_DATE_OLDER, NULL, FALSE );

			GenerateSearchTime ( tOlderTime, iAmount, iSelected );
		}

		if ( bDateChecked )
		{
			m_tSettings.m_uSearchFlags |= SEARCH_DATE_1 | SEARCH_DATE_2;

			SYSTEMTIME tTime1, tTime2;
			DateTime_GetSystemtime ( GetDlgItem ( hDlg, IDC_DATETIMEPICKER1 ), & tTime1 );
			DateTime_GetSystemtime ( GetDlgItem ( hDlg, IDC_DATETIMEPICKER2 ), & tTime2 );

			SystemTimeToFileTime( &tTime1, &m_tSettings.m_tTime1 );
			SystemTimeToFileTime( &tTime2, &m_tSettings.m_tTime2 );

			if ( bOlderChecked && CompareFileTime ( &tOlderTime, &m_tSettings.m_tTime2 ) > 0 )
				m_tSettings.m_tTime1 = tOlderTime;
		}
		else
			if ( bOlderChecked )
				m_tSettings.m_tTime1 = tOlderTime;

		if ( bSizeGreaterChecked || bSizeLessChecked )
		{
			m_tSettings.m_uSearchFlags |= bSizeGreaterChecked ? SEARCH_SIZE_GREATER : 0;
			m_tSettings.m_uSearchFlags |= bSizeLessChecked ? SEARCH_SIZE_LESS : 0;

			int iCombo1 = SendMessage ( m_hFindSizeCombo1, CB_GETCURSEL, 0, 0 );
			int iCombo2 = SendMessage ( m_hFindSizeCombo2, CB_GETCURSEL, 0, 0 );
			Assert ( iCombo1 != CB_ERR && iCombo2 != CB_ERR );

			m_tSettings.m_tSize1.QuadPart = GetDlgItemInt ( hDlg, IDC_FIND_SIZE1, NULL, TRUE );
			m_tSettings.m_tSize2.QuadPart = GetDlgItemInt ( hDlg, IDC_FIND_SIZE2, NULL, TRUE );

			FixupSize ( m_tSettings.m_tSize1, iCombo1 );
			FixupSize ( m_tSettings.m_tSize2, iCombo2 );
		}
    }

	void CloseAttributesPage ( HWND hDlg )
	{
		if ( IsDlgButtonChecked ( hDlg, IDC_FIND_ATTRIBUTES_CHECK ) == BST_CHECKED )
		{
			m_tSettings.m_uSearchFlags |= SEARCH_ATTRIBUTES;
			for ( int i = 0; i < IDC_FIND_ROM - IDC_FIND_ARCHIVE + 1; ++i )
			{
				int iCheck = IsDlgButtonChecked ( hDlg, i + IDC_FIND_ARCHIVE );
				if ( iCheck == BST_CHECKED )
					m_tSettings.m_uIncludeAttributes |= g_dAttributes [i];
				
				if ( iCheck == BST_UNCHECKED )
					m_tSettings.m_uExcludeAttributes |= g_dAttributes [i];
			}
		}
	}

private:
	FileSearchSettings_t & m_tSettings;
	HWND		m_hMaskList;
	HWND		m_hTextList;
	HWND		m_hCaseCheck;
	HWND		m_hWholeCheck;
	HWND		m_hFindSizeCombo1;
	HWND		m_hFindSizeCombo2;
	HWND		m_hTimeTypeCombo;
	HWND		m_hOlderCombo;
	HWND		m_hPicker1;
	HWND		m_hPicker2;


	void EnableTextControls ( HWND hDlg, bool bEnable )
	{
		BOOL bEW = bEnable ? 1 : 0;
		EnableWindow ( m_hTextList, bEW );
		EnableWindow ( m_hCaseCheck, bEW );
		EnableWindow ( m_hWholeCheck, bEW );

		EnableWindow ( GetDlgItem ( hDlg, IDC_SEARCH_FOR_FOLDERS ), ! bEW );
		if ( bEnable )
			CheckDlgButton ( hDlg, IDC_SEARCH_FOR_FOLDERS, BST_UNCHECKED );
	}


	void EnableSizeControls ( HWND hDlg, bool bEnable1, bool bEnable2 )
	{
		BOOL bEW1 = bEnable1 ? 1 : 0;
		BOOL bEW2 = bEnable2 ? 1 : 0;
		EnableWindow ( m_hFindSizeCombo1, bEW1 );
		EnableWindow ( m_hFindSizeCombo2, bEW2 );
		EnableWindow ( GetDlgItem ( hDlg, IDC_FIND_SIZE1 ), bEW1 );
		EnableWindow ( GetDlgItem ( hDlg, IDC_FIND_SIZE2 ), bEW2 );
	}


	void EnableOlderControls ( HWND hDlg, bool bEnable )
	{
		BOOL bEW = bEnable ? 1 : 0;
		EnableWindow ( m_hOlderCombo, bEW );
		EnableWindow ( GetDlgItem ( hDlg, IDC_FIND_DATE_OLDER ), bEW );
	}


	void EnableDateControls ( bool bEnable )
	{
		BOOL bEW = bEnable ? 1 : 0;
		EnableWindow ( m_hPicker1, bEW );
		EnableWindow ( m_hPicker2, bEW );
	}

	void EnableAttrControls ( HWND hDlg, bool bEnable )
	{
		BOOL bEW = bEnable ? 1 : 0;

		for ( int i = 0; i < IDC_FIND_ROM - IDC_FIND_ARCHIVE + 1; ++i )
			EnableWindow ( GetDlgItem ( hDlg, i + IDC_FIND_ARCHIVE ), bEW );
	}

	void PopulateMaskList ()
	{
		int nFindMasks = g_tRecent.GetNumFindMasks ();
		for ( int i = nFindMasks - 1; i >= 0 ; --i )
			SendMessage ( m_hMaskList, CB_ADDSTRING, 0, (LPARAM) (g_tRecent.GetFindMask ( i ).c_str () ) );

		SendMessage ( m_hMaskList, CB_SETCURSEL, 0, 0 );
	}


	void PopulateSearchText ()
	{
		SendMessage ( m_hTextList, CB_RESETCONTENT, 0, 0 );
		for ( int i = g_tRecent.GetNumFindTexts () - 1; i >= 0 ; --i )
			SendMessage ( m_hTextList, CB_ADDSTRING, 0, (LPARAM) (g_tRecent.GetFindText ( i ).c_str () ) );

		SendMessage ( m_hTextList, CB_SETCURSEL, 0, 0 );
	}


	void FixupSize ( ULARGE_INTEGER & tSize, int iCombo )
	{
		int iShift = 0;
		switch ( iCombo )
		{
			case 1:
				iShift = 10;
				break;
			case 2:
				iShift = 20;
				break;
		}
		tSize.QuadPart <<= iShift;
	}


	void GenerateSearchTime ( FILETIME & tTime, int iAmount, int iType )
	{
		SYSTEMTIME tSysTime;
		GetLocalTime ( &tSysTime );
		SystemTimeToFileTime ( &tSysTime, &tTime );

		ULARGE_INTEGER tTimeAsInt;
		memcpy ( &tTimeAsInt, &tTime, sizeof ( tTime ) );

		ULARGE_INTEGER tTimeSub;
		tTimeSub.QuadPart = iAmount * 10000000;	// in 100-nanosecond intervals

		switch ( iType )
		{
			case 0:	//min
				tTimeSub.QuadPart *= 60;
				break;

			case 1: //hours
				tTimeSub.QuadPart *= 3600;
				break;

			case 2: //days
				tTimeSub.QuadPart *= 86400;
				break;
			
			case 3: //months
				tTimeSub.QuadPart *= 2592000;
				break;

			case 4: //years
				tTimeSub.QuadPart *= 31536000;
				break;

			default:
				tTimeSub.QuadPart = 0;
				break;
		}

		tTimeAsInt.QuadPart -= tTimeSub.QuadPart;
		memcpy ( &tTime, &tTimeAsInt, sizeof ( tTime ) );
	}
};

#pragma warning( default : 4512 )

static FindDlg_c * g_pFindDlg = NULL;


static BOOL CALLBACK DlgProc0 ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	Assert ( g_pFindDlg );

	switch ( Msg )
	{
	case WM_INITDIALOG:
		g_pFindDlg->InitCommonPage ( hDlg );
	    break;

    case WM_COMMAND:
		if ( HIWORD(wParam) == BN_CLICKED )
		{
			if ( LOWORD(wParam) == IDC_FIND_TEXT_CHECK )
				g_pFindDlg->UpdateEnabledControls0 ( hDlg );
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"FileSearch" );
				break;
			case PSN_APPLY:
				g_pFindDlg->CloseCommonPage ( hDlg );
				CloseFullscreenDlg ();
				break;
			case PSN_RESET:
				CloseFullscreenDlg ();
				break;
			}
		}
		break;
	}

	return FALSE;
}


static BOOL CALLBACK DlgProc1 ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	Assert ( g_pFindDlg );

	switch ( Msg )
	{
	case WM_INITDIALOG:
		g_pFindDlg->InitSizeDatePage ( hDlg );
	    break;

	case WM_COMMAND:
		{
			int iId = (int)LOWORD(wParam);

			switch ( HIWORD(wParam) )
			{
			case BN_CLICKED:
				if (   iId == IDC_FIND_SIZE_GREATER_CHK || iId == IDC_FIND_SIZE_LESS_CHK || iId == IDC_FIND_OLDER_CHECK
					|| iId == IDC_FIND_DATE_CHECK )
					g_pFindDlg->UpdateEnabledControls1 ( hDlg );
				break;
			}
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"FileSearch" );
				break;
			case PSN_APPLY:
				g_pFindDlg->CloseSizeDatePage ( hDlg );
				break;
			}
		}

		break;
	}

	return FALSE;
}


static BOOL CALLBACK DlgProc2 ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	Assert ( g_pFindDlg );

	switch ( Msg )
	{
	case WM_INITDIALOG:
		g_pFindDlg->InitAttributesPage ( hDlg );
	    break;

	case WM_COMMAND:
		{
			int iId = (int)LOWORD(wParam);

			switch ( HIWORD(wParam) )
			{
			case BN_CLICKED:
				if ( iId == IDC_FIND_ATTRIBUTES_CHECK )
					g_pFindDlg->UpdateEnabledControls2 ( hDlg );
				break;
			}
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"FileSearch" );
				break;
			case PSN_APPLY:
				g_pFindDlg->CloseAttributesPage ( hDlg );
				break;
			}
		}
		break;
	}

	return FALSE;
}


static void SetupPage ( PROPSHEETPAGE & tPage, const wchar_t * szTitle, int iResource, DLGPROC fnDlgProc )
{
	ZeroMemory ( &tPage, sizeof ( tPage ) );
    tPage.dwSize 		= sizeof(PROPSHEETPAGE);
    tPage.dwFlags 		= PSP_DEFAULT | PSP_USETITLE;
    tPage.hInstance		= ResourceInstance ();
    tPage.pszTemplate	= MAKEINTRESOURCE(iResource);
    tPage.pszIcon		= NULL;
    tPage.pfnDlgProc 	= fnDlgProc;
	tPage.pszTitle 		= szTitle;
    tPage.lParam 		= 0;
    tPage.pfnCallback 	= NULL;
}


static int CALLBACK PropSheetProc ( HWND hDlg, UINT uMsg, LPARAM lParam )
{
	switch ( uMsg )
	{
		case PSCB_GETVERSION:
			return COMCTL32_VERSION;
		case PSCB_INITIALIZED:
			InitFullscreenDlg ( hDlg, IDM_OK_CANCEL, 0 );
			break;
	}
	
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
bool FindRequestDlg ( FileSearchSettings_t & tSettings )
{
	const int nPages = 3;
	PROPSHEETPAGE dPages [nPages];
    PROPSHEETHEADER tHeader;
	
	SetupPage ( dPages [0], Txt ( T_DLG_TAB_FIND ),			IDD_FIND_FILES_1, 	DlgProc0 );	
	SetupPage ( dPages [1], Txt ( T_DLG_TAB_SIZEDATE ),		IDD_FIND_FILES_2,	DlgProc1 );
	SetupPage ( dPages [2], Txt ( T_DLG_TAB_ATTRIBUTES ),	IDD_FIND_FILES_3, 	DlgProc2 );

    tHeader.dwSize 		= sizeof(PROPSHEETHEADER);
    tHeader.dwFlags 	= PSH_MAXIMIZE | PSH_PROPSHEETPAGE | PSH_USECALLBACK;
    tHeader.hwndParent 	= g_hMainWindow;
    tHeader.hInstance 	= ResourceInstance ();
    tHeader.nPages 		= nPages;
    tHeader.nStartPage 	= 0;
    tHeader.ppsp 		= (LPCPROPSHEETPAGE) &dPages;
    tHeader.pfnCallback = PropSheetProc;
 
	g_pFindDlg = new FindDlg_c ( tSettings );
	int iRes = PropertySheet ( &tHeader );
	SafeDelete ( g_pFindDlg );

	return iRes == 1;
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

const DWORD MY_UNIQUE_TIMER_FIND = 0xABCDEF02;

class FindProgressDlg_c : public Dialog_Resizer_c
{
public:
	Str_c			m_sDirToMoveTo;
	Str_c			m_sFileToMoveTo;

	FindProgressDlg_c ( const SelectedFileList_t & tList, const FileSearchSettings_t & tSettings )
		: Dialog_Resizer_c ( L"FileSearch", IDM_CANCEL, SHCMBF_HIDESIPBUTTON )
		, m_uTimer 		( 0 )
		, m_bSearchFinished ( false )
		, m_nLastFiles	( 0 )
		, m_pFileFinder	( NULL )
	{
		m_pFileFinder = new FileSearch_c ( tList, tSettings );
	}

	virtual ~FindProgressDlg_c ()
	{
		SafeDelete ( m_pFileFinder );
	}

	virtual void OnInit ()
	{
		Dialog_Resizer_c::OnInit ();

		Loc ( IDC_FIND_NUMFILES, T_DLG_SEARCHING );

		HWND hList = Item ( IDC_FIND_FILE_LIST );
		SendMessage ( hList, WM_SETFONT, (WPARAM)g_tResources.m_hSmallSystemFont, TRUE );
		ListView_SetExtendedListViewStyle ( hList, LVS_EX_FULLROWSELECT );

		LVCOLUMN tColumn;
		ZeroMemory ( &tColumn, sizeof ( tColumn ) );
		tColumn.mask = LVCF_TEXT | LVCF_SUBITEM;
		tColumn.iSubItem = 0;
		tColumn.pszText = (wchar_t *)Txt ( T_DLG_COLUMN_NAME );

		ListView_InsertColumn ( hList, tColumn.iSubItem,  &tColumn );
		++tColumn.iSubItem;
		tColumn.pszText = (wchar_t *)Txt ( T_DLG_COLUMN_PATH );
		ListView_InsertColumn ( hList, tColumn.iSubItem,  &tColumn );

		ListView_SetImageList ( hList, g_tResources.m_hSmallIconList, LVSIL_SMALL );

		UpdateProgress ( true );

		m_uTimer = SetTimer ( m_hWnd, MY_UNIQUE_TIMER_FIND, 0, NULL );

		SetResizer ( hList );
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Resizer_c::OnCommand ( iItem, iNotify );

		switch ( iItem )
		{
		case IDCANCEL:
			if ( m_bSearchFinished )
				Close ( iItem );
			else
				SearchFinished ( true );
			break;

		case IDOK:
			if ( MoveToSelected () )
				Close ( iItem );
			break;
		}
	}

	virtual void OnClose ()
	{
		ListView_SetImageList ( Item ( IDC_FIND_FILE_LIST ), NULL, LVSIL_SMALL );
	}

	virtual void OnSettingsChange ()
	{
		HWND hList = Item ( IDC_FIND_FILE_LIST );
		ListView_SetColumnWidth ( hList, 0, GetColumnWidthRelative ( 0.48f ) );
		ListView_SetColumnWidth ( hList, 1, GetColumnWidthRelative ( 0.52f ) );
		Dialog_Resizer_c::OnSettingsChange ();
	}

	virtual void OnTimer ( DWORD uTimer )
	{
		if ( uTimer == MY_UNIQUE_TIMER_FIND && ! m_bSearchFinished )
		{
			const double FIND_CHUNK_TIME = 0.1;
			double fStart = g_Timer.GetTimeSec ();

			while ( g_Timer.GetTimeSec () - fStart <= FIND_CHUNK_TIME )
				if ( !m_pFileFinder->SearchNext () )
				{
					SearchFinished ( false );
					break;
				}

			UpdateProgress ();
		}
	}

private:
	UINT			m_uTimer;
	bool			m_bSearchFinished;
	int				m_nLastFiles;
	FileSearch_c *	m_pFileFinder;

	void UpdateProgress ( bool bForce = false )
	{
		int nFiles = m_pFileFinder->GetNumFiles ();
		if ( nFiles && ( m_nLastFiles != nFiles || bForce ) )
		{	
			HWND hList = Item ( IDC_FIND_FILE_LIST );

			LVITEM tItem;
			ZeroMemory ( &tItem, sizeof ( tItem ) );
			tItem.mask = LVIF_TEXT | LVIF_IMAGE;

			for ( int i = 0; i < nFiles - m_nLastFiles; ++i )
			{
				const WIN32_FIND_DATA & tData = m_pFileFinder->GetData ( m_nLastFiles + i );
				Str_c sDir = m_pFileFinder->GetDirectory ( m_nLastFiles + i );

				// retrieve file type icon
				SHFILEINFO tShFileInfo;
				tShFileInfo.iIcon = -1;

				SHGetFileInfo ( sDir + tData.cFileName, 0, &tShFileInfo, sizeof (SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON );

				tItem.pszText = (wchar_t *) &tData.cFileName;
				tItem.iItem = m_nLastFiles + i;
				tItem.iImage = tShFileInfo.iIcon;

				int iItem = ListView_InsertItem ( hList, &tItem );
				ListView_SetItemText( hList, iItem, 1, (wchar_t *)( sDir.c_str () ) );
			}

			m_nLastFiles = nFiles;

			if ( !m_bSearchFinished )
				ItemTxt ( IDC_FIND_NUMFILES, NewString ( Txt ( T_DLG_SEARCH_RESULTS ), nFiles ) );
		}
	}

	void SearchFinished ( bool bCancelled )
	{
		m_bSearchFinished = true;
		KillTimer ( m_hWnd, m_uTimer );

		HWND hTB = CreateToolbar ( m_hWnd, IDM_OK_CANCEL, SHCMBF_HIDESIPBUTTON );
		SetToolbarText ( hTB, IDOK, Txt ( T_TBAR_GOTO ) );
		ItemTxt ( IDC_FIND_NUMFILES, NewString ( Txt ( bCancelled ? T_DLG_SEARCH_CANCELLED : T_DLG_SEARCH_FINISHED ), m_pFileFinder->GetNumFiles () ) );
	}

	bool MoveToSelected ()
	{
		int iSelected = ListView_GetSelectionMark ( Item ( IDC_FIND_FILE_LIST ) );
		if ( iSelected == -1 )
			return false;

		m_sFileToMoveTo = m_pFileFinder->GetData ( iSelected ).cFileName;
		m_sDirToMoveTo	= m_pFileFinder->GetDirectory ( iSelected );
		return true;
	}
};


///////////////////////////////////////////////////////////////////////////////////////////
bool FindProgressDlg ( const SelectedFileList_t & tList, const FileSearchSettings_t & tSettings, Str_c & sDir, Str_c & sFile )
{
	FindProgressDlg_c Dlg ( tList, tSettings );
	if ( Dlg.Run ( IDD_FIND_RESULTS, g_hMainWindow ) != IDOK )
		return false;

	sDir  = Dlg.m_sDirToMoveTo;
	sFile = Dlg.m_sFileToMoveTo;
	return !sFile.Empty () || !sDir.Empty ();
}