#include "pch.h"

#include "LDialogs/dfile_properties.h"
#include "pfm/config.h"
#include "pfm/resources.h"
#include "pfm/gui.h"
#include "pfm/dialogs/tree.h"

#include "aygshell.h"
#include "Dlls/Resource/resource.h"
#include "shellapi.h"

extern HWND g_hMainWindow;

static HWND g_hPropsWnd = NULL;

static const DWORD VALID_ATTRIB = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_INROM;

static void SlowCallback ()
{
	SetCursor ( LoadCursor ( NULL, IDC_WAIT ) );
}

static void InitFileIcon ( HWND hDlg, SelectedFileList_t &  tList )
{
	bool bEmptyIcon = true;

	SHFILEINFO tShFileInfo;
	tShFileInfo.iIcon = -1;
	const WIN32_FIND_DATA & tData = tList.m_dFiles [0]->m_FindData;
	HIMAGELIST hList = (HIMAGELIST) SHGetFileInfo ( tList.m_sRootDir + tData.cFileName,
		tData.dwFileAttributes, &tShFileInfo, sizeof (SHFILEINFO), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_ICON | SHGFI_LARGEICON );

	if ( tShFileInfo.iIcon != -1 )
	{
		HICON hIcon = ImageList_GetIcon ( hList, tShFileInfo.iIcon, ILD_NORMAL );
		if ( hIcon )
		{
			SendMessage ( GetDlgItem ( hDlg, IDC_FILE_ICON ), STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon );
			bEmptyIcon = false;
		}
	}

	if ( bEmptyIcon )
	{
		HICON hIcon = ImageList_GetIcon ( hList, 1, ILD_NORMAL );
		SendMessage ( GetDlgItem ( hDlg, IDC_FILE_ICON ), STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon );
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
// common props dlg
class PropsDlg_c
{
public:
	PropsDlg_c ( SelectedFileList_t & tList, const ULARGE_INTEGER & uSize, int nFiles, int nFolders, DWORD dwIncAttrib, DWORD dwExAttrib )
		: m_tList			( tList )
		, m_uSize			( uSize )
		, m_nFiles			( nFiles )
		, m_nFolders		( nFolders )
		, m_hTimeAccess		( NULL )
		, m_hDateAccess		( NULL )
		, m_hTimeCreate		( NULL )
		, m_hDateCreate		( NULL )
		, m_hTimeWrite		( NULL )
		, m_hDateWrite		( NULL )
		, m_bUseSubdirs		( false )
		, m_bChangeAttributes( false )
		, m_bChangeTime		( false )
		, m_hMultiIcon		( NULL )
		, m_dwIncAttrib		( dwIncAttrib )
		, m_dwExAttrib		( dwExAttrib )
	{
		Assert ( m_tList.m_dFiles.Length() > 0 );

		memset ( &m_tTimeCreate, 0, sizeof ( m_tTimeCreate ) );
		memset ( &m_tTimeAccess, 0, sizeof ( m_tTimeAccess ) );
		memset ( &m_tTimeWrite, 0, sizeof ( m_tTimeWrite ) );

		m_dwIncAttrib &= VALID_ATTRIB;
		m_dwExAttrib &= VALID_ATTRIB;
	}


	~PropsDlg_c ()
	{
		if ( m_hMultiIcon )
			DestroyIcon ( m_hMultiIcon );
	}


	void InitCommonPage ( HWND hDlg )
	{
		DlgTxt ( hDlg, IDC_LOCATION_STATIC, T_DLG_LOCATION );
		DlgTxt ( hDlg, IDC_SIZE_STATIC, T_DLG_SIZE );
		DlgTxt ( hDlg, IDC_CONTAINS_STATIC, T_DLG_CONTAINS );

		InitIcon ( hDlg );

		if ( m_tList.OneFile () )
		{
			const WIN32_FIND_DATA & tData = m_tList.m_dFiles [0]->m_FindData;

			SHFILEINFO tShFileInfo;
			SHGetFileInfo ( m_tList.m_sRootDir + tData.cFileName, tData.dwFileAttributes, &tShFileInfo, sizeof (SHFILEINFO), SHGFI_TYPENAME );
			SetDlgItemText ( hDlg, IDC_TYPE, tShFileInfo.szTypeName );
			SetDlgItemText ( hDlg, IDC_FILENAME_STATIC, tData.cFileName );
			SetDlgItemText ( hDlg, IDC_LOCATION, m_tList.m_sRootDir + tData.cFileName );
		}
		else
		{
			wchar_t szBuf [128];
			wsprintf ( szBuf, Txt ( T_DLG_FILESFOLDERS ), m_nFiles, m_nFolders );
			SetDlgItemText ( hDlg, IDC_TYPE, Txt ( T_DLG_MULTIFILES ) );
			SetDlgItemText ( hDlg, IDC_FILENAME_STATIC, szBuf );
			SetDlgItemText ( hDlg, IDC_LOCATION, m_tList.m_sRootDir );
		}

		SendMessage ( GetDlgItem ( hDlg, IDC_FILENAME_STATIC ), WM_SETFONT, (WPARAM)g_tResources.m_hBoldSystemFont, TRUE );

		const WIN32_FIND_DATA & tData = m_tList.m_dFiles [0]->m_FindData;
		
		wchar_t szSize [FILESIZE_BUFFER_SIZE*2 + 64];
		wchar_t szSize1 [FILESIZE_BUFFER_SIZE];
		wchar_t szSize2 [FILESIZE_BUFFER_SIZE];
		if ( tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			wsprintf ( szSize, Txt ( T_DLG_SIZE_DETAILED ), FileSizeToStringUL ( m_uSize, szSize1, true ), FileSizeToStringByte ( m_uSize, szSize2 ) );
			SetDlgItemText ( hDlg, IDC_FILE_SIZE, szSize );

			wsprintf ( szSize, Txt ( T_DLG_FILESFOLDERS ), m_nFiles, m_nFolders - 1 );
			SetDlgItemText ( hDlg, IDC_CONTAINS, szSize );
		}
		else
		{
			ShowWindow ( GetDlgItem ( hDlg, IDC_CONTAINS_STATIC ), SW_HIDE );
			ShowWindow ( GetDlgItem ( hDlg, IDC_CONTAINS ), SW_HIDE );
			wsprintf ( szSize, Txt ( T_DLG_SIZE_DETAILED ), FileSizeToStringUL ( m_uSize, szSize1, true ), FileSizeToStringByte ( m_uSize, szSize2 ) );
			SetDlgItemText ( hDlg, IDC_FILE_SIZE, szSize );
		}
	}


	void InitAttributesPage ( HWND hDlg )
	{
		DlgTxt ( hDlg, IDC_FIND_ARCHIVE, T_DLG_ATTR_ARCHIVE );
		DlgTxt ( hDlg, IDC_FIND_READONLY, T_DLG_ATTR_READONLY );
		DlgTxt ( hDlg, IDC_FIND_SYSTEM, T_DLG_ATTR_SYSTEM );
		DlgTxt ( hDlg, IDC_FIND_HIDDEN, T_DLG_ATTR_HIDDEN );
		DlgTxt ( hDlg, IDC_FIND_ROM, T_DLG_ATTR_ROM );
		DlgTxt ( hDlg, IDC_ORIGINAL, T_DLG_PROPS_ORIGINAL );
		DlgTxt ( hDlg, IDC_CURRENT, T_DLG_PROPS_CURRENT );
		DlgTxt ( hDlg, IDC_FIND_ATTRIBUTES_CHECK, T_DLG_PROPS_APPLYTOSUB );

		DlgTxt ( hDlg, IDC_CREATE_STATIC, T_DLG_PROPS_CREATED );
		DlgTxt ( hDlg, IDC_ACCESS_STATIC, T_DLG_PROPS_ACCESSED );
		DlgTxt ( hDlg, IDC_MODIFY_STATIC, T_DLG_PROPS_WRITTEN  );

		// time/date
		m_hTimeAccess	= GetDlgItem ( hDlg, IDC_TIME_ACCESS );
		m_hDateAccess	= GetDlgItem ( hDlg, IDC_DATE_ACCESS );
		m_hTimeCreate	= GetDlgItem ( hDlg, IDC_TIME_CREATE );
		m_hDateCreate	= GetDlgItem ( hDlg, IDC_DATE_CREATE );
		m_hTimeWrite	= GetDlgItem ( hDlg, IDC_TIME_WRITE );
		m_hDateWrite	= GetDlgItem ( hDlg, IDC_DATE_WRITE );

		BOOL bEnableTimeDate = m_tList.m_dFiles.Length () <= 1;

		EnableWindow ( m_hTimeAccess, bEnableTimeDate );
		EnableWindow ( m_hDateAccess, bEnableTimeDate );
		EnableWindow ( m_hTimeCreate, bEnableTimeDate );
		EnableWindow ( m_hDateCreate, bEnableTimeDate );
		EnableWindow ( m_hTimeWrite, bEnableTimeDate );
		EnableWindow ( m_hDateWrite, bEnableTimeDate );

		EnableWindow ( GetDlgItem ( hDlg, IDC_ORIGINAL ), bEnableTimeDate );
		EnableWindow ( GetDlgItem ( hDlg, IDC_CURRENT ), bEnableTimeDate );

		if ( ! bEnableTimeDate )
		{
			GetLocalTime ( &m_tTimeAccess );
			m_tTimeCreate = m_tTimeWrite = m_tTimeAccess;

			DateTime_SetSystemtime ( m_hDateAccess, GDT_VALID, &m_tTimeAccess );
			DateTime_SetSystemtime ( m_hTimeAccess, GDT_VALID, &m_tTimeAccess );
			DateTime_SetSystemtime ( m_hDateCreate, GDT_VALID, &m_tTimeAccess );
			DateTime_SetSystemtime ( m_hTimeCreate, GDT_VALID, &m_tTimeAccess );
			DateTime_SetSystemtime ( m_hDateWrite, GDT_VALID,  &m_tTimeAccess );
			DateTime_SetSystemtime ( m_hTimeWrite, GDT_VALID,  &m_tTimeAccess );
		}
		else
		{
			const WIN32_FIND_DATA & tData = m_tList.m_dFiles [0]->m_FindData;
			FILETIME tLocalTime;

			FileTimeToLocalFileTime ( &tData.ftCreationTime, &tLocalTime );
			FileTimeToSystemTime ( &tLocalTime, &m_tTimeCreate );

			FileTimeToLocalFileTime ( &tData.ftLastAccessTime, &tLocalTime );
			FileTimeToSystemTime ( &tLocalTime, &m_tTimeAccess );
			
			FileTimeToLocalFileTime ( &tData.ftLastWriteTime, &tLocalTime );
			FileTimeToSystemTime ( &tLocalTime, &m_tTimeWrite );

			DateTime_SetSystemtime ( m_hTimeAccess, GDT_VALID, &m_tTimeAccess );
			DateTime_SetSystemtime ( m_hDateAccess, GDT_VALID, &m_tTimeAccess );

			DateTime_SetSystemtime ( m_hTimeCreate, GDT_VALID, &m_tTimeCreate );
			DateTime_SetSystemtime ( m_hDateCreate, GDT_VALID, &m_tTimeCreate );

			DateTime_SetSystemtime ( m_hTimeWrite, GDT_VALID, &m_tTimeWrite );
			DateTime_SetSystemtime ( m_hDateWrite, GDT_VALID, &m_tTimeWrite );
		}

		// attributes
		bool bEnable3State = m_nFiles > 1;

		EnableWindow ( GetDlgItem ( hDlg, IDC_FIND_ATTRIBUTES_CHECK ), ( m_nFiles + m_nFolders > 1 ) ? TRUE : FALSE );
		for ( int i = 0; i <= IDC_FIND_ROM - IDC_FIND_ARCHIVE; ++i )
		{
			HWND hItem = GetDlgItem ( hDlg, i + IDC_FIND_ARCHIVE );
			
			int iStyle = bEnable3State ? BS_AUTO3STATE : BS_AUTOCHECKBOX;
			SendMessage ( hItem, BM_SETSTYLE, iStyle, FALSE );

			int iCheck = BST_INDETERMINATE;

			if ( m_dwIncAttrib & g_dAttributes [i] )
			{
				if ( ! ( m_dwExAttrib & g_dAttributes [i] ) )
					iCheck = BST_CHECKED;
			}
			else
				if (  m_dwExAttrib & g_dAttributes [i] )
					iCheck = BST_UNCHECKED;

			CheckDlgButton ( hDlg, i + IDC_FIND_ARCHIVE, iCheck );
		}
	}

	void InitShortcutPage ( HWND hDlg )
	{
		DlgTxt ( hDlg, IDC_TARGET_STATIC,	T_DLG_PROPS_TARGET );
		DlgTxt ( hDlg, IDC_TREE,			T_CMN_TREE );

		Str_c sPath = m_tList.m_sRootDir + m_tList.m_dFiles [0]->m_FindData.cFileName, sParams;
		bool bDir;
		if ( DecomposeLnk ( sPath, sParams, bDir ) )
		{
			m_sInitialTarget = sPath;
			if ( ! sParams.Empty () )
			{
				m_sInitialTarget += L" ";
				m_sInitialTarget += sParams;
			}

			SetEditTextFocused ( GetDlgItem ( hDlg, IDC_TARGET ), m_sInitialTarget );
		}
	}

	void CloseShortcutPage ( HWND hDlg )
	{
		wchar_t szBuf [MAX_PATH];
		GetDlgItemText ( hDlg, IDC_TARGET, szBuf, MAX_PATH );
		if ( szBuf != m_sInitialTarget )
		{
			Str_c sPath = m_tList.m_sRootDir + m_tList.m_dFiles [0]->m_FindData.cFileName;
			DeleteFile ( sPath );
			CreateShortcut ( sPath, szBuf );
		}
	}

	void ShowShortcutTree ( HWND hDlg )
	{
		Str_c sPath;
		if ( FileTreeDlg ( hDlg, sPath, true ) )
		{
			RemoveSlash ( sPath );
			SetEditTextFocused ( GetDlgItem ( hDlg, IDC_TARGET ), Str_c ( L"\"" ) + sPath +  L"\"" );
		}
	}

	void CloseAttributesPage ( HWND hDlg )
	{
		if ( m_tList.m_dFiles.Length () <= 1 )
		{
			// time/date
			SYSTEMTIME tTime;
			if ( GetTime ( m_hTimeCreate, m_hDateCreate, tTime ) )
			{
				if ( memcmp ( &tTime, &m_tTimeCreate, sizeof ( SYSTEMTIME ) ) )
					m_bChangeTime = true;

				m_tTimeCreate = tTime;
			}

			if ( GetTime ( m_hTimeAccess, m_hDateAccess, tTime ) )
			{
				if ( memcmp ( &tTime, &m_tTimeAccess, sizeof ( SYSTEMTIME ) ) )
					m_bChangeTime = true;

				m_tTimeAccess = tTime;
			}

			if ( GetTime ( m_hTimeWrite, m_hDateWrite, tTime ) )
			{
				if ( memcmp ( &tTime, &m_tTimeWrite, sizeof ( SYSTEMTIME ) ) )
					m_bChangeTime = true;

				m_tTimeWrite = tTime;
			}
		}

		// attributes
		m_bUseSubdirs = IsDlgButtonChecked ( hDlg, IDC_FIND_ATTRIBUTES_CHECK ) == BST_CHECKED;

		DWORD dwIncAttribToSet = 0, dwExAttribToSet = 0;

		for ( int i = 0; i <= IDC_FIND_ROM - IDC_FIND_ARCHIVE; ++i )
		{
			int iCheck = IsDlgButtonChecked ( hDlg, i + IDC_FIND_ARCHIVE );
			switch ( iCheck )
			{
			case BST_CHECKED:
				dwIncAttribToSet |= g_dAttributes [i];
				break;

			case BST_UNCHECKED:
				dwExAttribToSet |= g_dAttributes [i];
				break;

			case BST_INDETERMINATE:
				dwIncAttribToSet |= g_dAttributes [i];
				dwExAttribToSet |= g_dAttributes [i];
				break;
			}
		}

		if ( m_dwIncAttrib != dwIncAttribToSet || m_dwExAttrib != dwExAttribToSet )
		{
			m_dwIncAttrib = dwIncAttribToSet;
			m_dwExAttrib = dwExAttribToSet;
			m_bChangeAttributes = true;
		}
	}


	void SetOriginalTime ()
	{
		DateTime_SetSystemtime ( m_hTimeCreate, GDT_VALID, &m_tTimeCreate );
		DateTime_SetSystemtime ( m_hDateCreate, GDT_VALID, &m_tTimeCreate );
		DateTime_SetSystemtime ( m_hTimeAccess, GDT_VALID, &m_tTimeAccess );
		DateTime_SetSystemtime ( m_hDateAccess, GDT_VALID, &m_tTimeAccess );
		DateTime_SetSystemtime ( m_hTimeWrite, GDT_VALID, &m_tTimeWrite );
		DateTime_SetSystemtime ( m_hDateWrite, GDT_VALID, &m_tTimeWrite );
	}

	void SetCurrentTime ()
	{
		SYSTEMTIME tTime;
		GetLocalTime ( &tTime );

		DateTime_SetSystemtime ( m_hTimeCreate, GDT_VALID, &tTime );
		DateTime_SetSystemtime ( m_hDateCreate, GDT_VALID, &tTime );
		DateTime_SetSystemtime ( m_hTimeAccess, GDT_VALID, &tTime );
		DateTime_SetSystemtime ( m_hDateAccess, GDT_VALID, &tTime );
		DateTime_SetSystemtime ( m_hTimeWrite, GDT_VALID, &tTime );
		DateTime_SetSystemtime ( m_hDateWrite, GDT_VALID, &tTime );
	}


	bool ApplyChanges ()
	{
		if ( ! m_bChangeTime && ! m_bChangeAttributes )
			return false;

		DWORD uFlags = 0;
		if ( m_bChangeTime )
			uFlags |= FILE_PROPERTY_TIME;

		if ( m_bChangeAttributes )
			uFlags |= FILE_PROPERTY_ATTRIBUTES;

		if ( m_bUseSubdirs )
			uFlags |= FILE_PROPERTY_RECURSIVE;

		FilePropertyCommand_t tCommand;
		tCommand.m_tTimeCreate	= m_tTimeCreate;
		tCommand.m_tTimeAccess	= m_tTimeAccess;
		tCommand.m_tTimeWrite	= m_tTimeWrite;
		tCommand.m_dwIncAttrib	= m_dwIncAttrib;
		tCommand.m_dwExAttrib	= m_dwExAttrib;
		tCommand.m_dwFlags		= uFlags;

		FileChangeProperties ( g_hMainWindow, m_tList, tCommand, SlowCallback );
		SetCursor ( LoadCursor ( NULL, IDC_ARROW ) );

		return true;
	}

private:
	SelectedFileList_t &	m_tList;
	ULARGE_INTEGER	m_uSize;
	DWORD			m_dwIncAttrib;
	DWORD			m_dwExAttrib;
	int				m_nFiles;
	int				m_nFolders;

	HWND			m_hTimeAccess;
	HWND			m_hDateAccess;
	HWND			m_hTimeCreate;
	HWND			m_hDateCreate;
	HWND			m_hTimeWrite;
	HWND			m_hDateWrite;

	SYSTEMTIME		m_tTimeCreate;
	SYSTEMTIME		m_tTimeAccess;
	SYSTEMTIME		m_tTimeWrite;

	bool			m_bUseSubdirs;
	bool			m_bChangeAttributes;
	bool			m_bChangeTime;

	HICON			m_hMultiIcon;

	Str_c			m_sInitialTarget;

	void InitIcon ( HWND hDlg )
	{
		if ( m_tList.m_dFiles.Length () == 1 )
			InitFileIcon ( hDlg, m_tList );
		else
		{
			m_hMultiIcon = LoadIcon ( ResourceInstance (), MAKEINTRESOURCE ( IDI_MULTI ) );
			SendMessage ( GetDlgItem ( hDlg, IDC_FILE_ICON ), STM_SETIMAGE, IMAGE_ICON, (LPARAM) m_hMultiIcon );
		}
	}

	bool GetTime ( HWND hTime, HWND hDate, SYSTEMTIME & tResult )
	{
		SYSTEMTIME tSystemTime;
		if ( DateTime_GetSystemtime ( hTime, &tSystemTime ) != GDT_VALID )
			return false;

		if ( DateTime_GetSystemtime ( hDate, &tResult ) != GDT_VALID )
			return false;

		tResult.wMilliseconds	= tSystemTime.wMilliseconds;
		tResult.wSecond			= tSystemTime.wSecond;
		tResult.wMinute			= tSystemTime.wMinute;
		tResult.wHour			= tSystemTime.wHour;
		
		return true;
	}
};


static PropsDlg_c * g_pPropsDlg = NULL;


///////////////////////////////////////////////////////////////////////////////////////////
// card props dlg
class CardPropsDlg_c
{
public:
	CardPropsDlg_c ( SelectedFileList_t & tList, const ULARGE_INTEGER & uSize, int nFiles, int nFolders )
		: m_tList			( tList )
		, m_uSize			( uSize )
		, m_nFiles			( nFiles )
		, m_nFolders		( nFolders )
	{
		Assert ( tList.m_dFiles.Length () == 1 );
	}


	void Init ( HWND hDlg )
	{
		DlgTxt ( hDlg, IDC_USED_SPACE_STATIC, T_DLG_USED_SPACE );
		DlgTxt ( hDlg, IDC_TOTAL_SPACE_STATIC, T_DLG_TOTAL_SPACE );
		DlgTxt ( hDlg, IDC_FREE_SPACE_STATIC, T_DLG_FREE_SPACE );
		DlgTxt ( hDlg, IDC_CONTAINS_STATIC, T_DLG_CONTAINS );

		InitFullscreenDlg ( hDlg, IDM_OK, SHCMBF_HIDESIPBUTTON );

		HWND hFilename = GetDlgItem ( hDlg, IDC_FILENAME );
		SendMessage ( hFilename, WM_SETFONT, (WPARAM)g_tResources.m_hBoldSystemFont, TRUE );

		const WIN32_FIND_DATA & tData = m_tList.m_dFiles [0]->m_FindData;

		SHFILEINFO tShFileInfo;
		SHGetFileInfo ( m_tList.m_sRootDir + tData.cFileName, tData.dwFileAttributes, &tShFileInfo, sizeof (SHFILEINFO), SHGFI_TYPENAME );
		SetDlgItemText ( hDlg, IDC_TYPE, tShFileInfo.szTypeName );

		AlignFileName ( hFilename, tData.cFileName );
		InitFileIcon ( hDlg, m_tList );

		wchar_t szSize [FILESIZE_BUFFER_SIZE*2 + 128];
		wchar_t szSize1 [FILESIZE_BUFFER_SIZE];
		wchar_t szSize2 [FILESIZE_BUFFER_SIZE];
		wsprintf ( szSize, Txt ( T_DLG_SIZE_DETAILED ), FileSizeToStringUL ( m_uSize, szSize1, true ), FileSizeToStringByte ( m_uSize, szSize2 ) );
		SetDlgItemText ( hDlg, IDC_FILE_SIZE, szSize );

		wsprintf ( szSize, Txt ( T_DLG_FILESFOLDERS ), m_nFiles, m_nFolders - 1 );
		SetDlgItemText ( hDlg, IDC_CONTAINS, szSize );

		ULARGE_INTEGER uTotal;
		ULARGE_INTEGER uFree;
		uTotal.QuadPart = uFree.QuadPart = 0;
		
		GetDiskFreeSpaceEx ( m_tList.m_sRootDir + tData.cFileName, &uFree, &uTotal, NULL );

		wsprintf ( szSize, Txt ( T_DLG_SIZE_DETAILED ), FileSizeToStringUL ( uTotal, szSize1, true ), FileSizeToStringByte ( uTotal, szSize2 ) );
		SetDlgItemText ( hDlg, IDC_TOTAL_SPACE, szSize );

		wsprintf ( szSize, Txt ( T_DLG_SIZE_DETAILED ), FileSizeToStringUL ( uFree, szSize1, true ), FileSizeToStringByte ( uFree, szSize2 ) );
		SetDlgItemText ( hDlg, IDC_FREE_SPACE, szSize );
	}

	void Close ()
	{
		CloseFullscreenDlg ();
	}

private:
	SelectedFileList_t &	m_tList;
	ULARGE_INTEGER	m_uSize;
	int				m_nFiles;
	int				m_nFolders;
};


static CardPropsDlg_c * g_pCardPropsDlg = NULL;



static BOOL CALLBACK DlgProc0 ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	Assert ( g_pPropsDlg );
	static int iIcon = 0;

	switch ( Msg )
	{
	case WM_INITDIALOG:
		g_pPropsDlg->InitCommonPage ( hDlg );
	    break;

	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"Props" );
				break;
			case PSN_APPLY:
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
	Assert ( g_pPropsDlg );

	switch ( Msg )
	{
	case WM_INITDIALOG:
		g_pPropsDlg->InitAttributesPage ( hDlg );
	    break;

	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"Props" );
				break;

			case PSN_APPLY:
				g_pPropsDlg->CloseAttributesPage ( hDlg );
				break;
			}
		}
		break;

	case WM_COMMAND:
		{
			int iId = (int)LOWORD(wParam);
			int iSubMsg = HIWORD(wParam);

			switch ( iSubMsg )
			{
			case BN_CLICKED:
				if ( iId == IDC_ORIGINAL )
					g_pPropsDlg->SetOriginalTime ();

				if ( iId == IDC_CURRENT )
					g_pPropsDlg->SetCurrentTime ();
				break;
			}
		}
		break;
	}

	return FALSE;
}

static BOOL CALLBACK DlgProcShortcut ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	Assert ( g_pPropsDlg );

	switch ( Msg )
	{
	case WM_INITDIALOG:
		g_pPropsDlg->InitShortcutPage ( hDlg );
		break;

	case WM_NOTIFY:
		{
			NMHDR * pInfo = (NMHDR *) lParam;
			switch ( pInfo->code )
			{
			case PSN_HELP:
				Help ( L"Props" );
				break;

			case PSN_APPLY:
				g_pPropsDlg->CloseShortcutPage ( hDlg );
				break;
			}
		}
		break;

	case WM_COMMAND:
		{
			switch ( (int)LOWORD(wParam) )
			{
			case IDC_TREE:
				g_pPropsDlg->ShowShortcutTree ( hDlg );
				break;
			}
		}
		break;
	}

	return FALSE;
}



static BOOL CALLBACK CardDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	Assert ( g_pCardPropsDlg );

	switch ( Msg )
	{
	case WM_INITDIALOG:
		g_pCardPropsDlg->Init ( hDlg );
	    break;

	case WM_COMMAND:
		{
			int iId = (int)LOWORD(wParam);
			if ( iId == IDOK || iId == IDCANCEL )
			{
				g_pCardPropsDlg->Close ();
				EndDialog ( hDlg, LOWORD (wParam) );
			}
		}
		break;

	case WM_HELP:
		Help ( L"Props" );
		return TRUE;
	}

	HandleActivate ( hDlg, Msg, wParam, lParam );

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
			g_hPropsWnd = hDlg;
			InitFullscreenDlg ( hDlg, IDM_OK_CANCEL, 0 );
			break;
	}
	
	return 0;
}


static bool ShowCommonFilesDlg ( SelectedFileList_t & tList, bool bShortcut )
{
	ULARGE_INTEGER uSize;
	int nFiles, nFolders;

	DWORD dwIncAttrib = 0;
	DWORD dwExAttrib = 0;

	CalcTotalSize ( tList, uSize, nFiles, nFolders, dwIncAttrib, dwExAttrib, SlowCallback );
	SetCursor ( LoadCursor ( NULL, IDC_ARROW ) );

	const int nPages = bShortcut ? 3 : 2;
	PROPSHEETPAGE dPages [3];
    PROPSHEETHEADER tHeader;

	SetupPage ( dPages [0], Txt ( T_DLG_TAB_PROPS ),	IDD_FILE_PROPERTIES0, 	DlgProc0 );	
	SetupPage ( dPages [1], Txt ( T_DLG_TAB_TIMEATTR ),	IDD_FILE_PROPERTIES1,	DlgProc1 );
	if ( bShortcut )
		SetupPage ( dPages [2], Txt ( T_DLG_TAB_SHORTCUT ), IDD_FILE_SHORTCUT,	DlgProcShortcut );

    tHeader.dwSize 		= sizeof(PROPSHEETHEADER);
    tHeader.dwFlags 	= PSH_MAXIMIZE | PSH_PROPSHEETPAGE | PSH_USECALLBACK;
    tHeader.hwndParent 	= g_hMainWindow;
    tHeader.hInstance 	= ResourceInstance ();
    tHeader.nPages 		= nPages;
    tHeader.nStartPage 	= 0;
    tHeader.ppsp 		= (LPCPROPSHEETPAGE) &dPages;
    tHeader.pfnCallback = PropSheetProc;
 
	g_pPropsDlg = new PropsDlg_c ( tList, uSize, nFiles, nFolders, dwIncAttrib, dwExAttrib );
	int iRes = PropertySheet ( &tHeader );

	bool bChanged = false;
	if ( iRes == 1 )
		bChanged = g_pPropsDlg->ApplyChanges ();

	SafeDelete ( g_pPropsDlg );

	return bChanged;	
}


static void ShowCardDlg ( SelectedFileList_t & tList )
{
 	ULARGE_INTEGER uSize;
	int nFiles, nFolders;
	DWORD dwA1, dwA2;
	CalcTotalSize ( tList, uSize, nFiles, nFolders, dwA1, dwA2, SlowCallback );
	SetCursor ( LoadCursor ( NULL, IDC_ARROW ) );

	g_pCardPropsDlg = new CardPropsDlg_c ( tList, uSize, nFiles, nFolders );
	int iRes = DialogBox ( ResourceInstance (), MAKEINTRESOURCE (IDD_FILE_PROPERTIES_CARD), g_hMainWindow, CardDlgProc );
	SafeDelete ( g_pCardPropsDlg );
}


bool ShowFilePropertiesDialog ( SelectedFileList_t & tList )
{
	if ( cfg->fullscreen )
		SHFullScreen ( g_hMainWindow, SHFS_SHOWTASKBAR );

	if ( tList.OneFile () && ( tList.m_dFiles [0]->m_FindData.dwFileAttributes & FILE_ATTRIBUTE_CARD ) )
		ShowCardDlg ( tList );
	else
	{
		bool bLnk = tList.OneFile () && Str_c ( tList.m_dFiles [0]->m_FindData.cFileName ).Ends ( L".lnk" );
		ShowCommonFilesDlg ( tList, bLnk );
	}

	return false;
}