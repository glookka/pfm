#include "pch.h"

#include "LDialogs/dapps.h"
#include "LCore/clog.h"
#include "LFile/fapps.h"
#include "LCore/cfile.h"
#include "LPanel/presources.h"
#include "LDialogs/dcommon.h"
#include "LDialogs/dtree.h"
#include "LUI/udialogs.h"
#include "LSettings/slocal.h"
#include "LSettings/srecent.h"

#include "commdlg.h"
#include "aygshell.h"
#include "Resources/resource.h"

extern HINSTANCE g_hAppInstance;
extern HWND g_hMainWindow;

class OpenWithDlg_c : public Dialog_Resizer_c
{
public:
	OpenWithDlg_c ()
		: Dialog_Resizer_c ( L"Associate", IDM_OK_CANCEL, SHCMBF_HIDESIPBUTTON )
		, m_bNoSource ( true )
		, m_hList		( NULL )
		, m_iNewApp		( -1 )
		, m_iOldApp		( -1 )
		, m_bAlways		( false )
		, m_bQuotes		( false )
	{
	}

	OpenWithDlg_c ( const Str_c & sFileName )
		: Dialog_Resizer_c ( L"Associate", IDM_OK_CANCEL, SHCMBF_HIDESIPBUTTON )
		, m_bNoSource	( false )
		, m_hList		( NULL )
		, m_sFileName	( sFileName )
		, m_iNewApp		( -1 )
		, m_iOldApp		( -1 )
		, m_bAlways		( false )
		, m_bQuotes		( false )
	{
	}


	virtual void OnInit ()
	{
		Dialog_Resizer_c::OnInit ();

		Loc ( IDC_BROWSE, T_DLG_BROWSE );

		SetCursor ( LoadCursor ( NULL, IDC_WAIT ) );

		if ( m_bNoSource )
		{
			Bold ( IDC_TITLE );
			Loc ( IDC_TITLE, T_DLG_APP_CHOOSE );
			
			if ( ! apps::EnumApps () )
				return;
		}
		else
		{
			Loc ( IDC_TITLE, T_DLG_APP_CHOOSE_FOR );
			Loc ( IDC_ALWAYS, T_DLG_APP_ALWAYS );
			Loc ( IDC_QUOTES, T_DLG_APP_QUOTES );

			if ( ! apps::EnumAppsFor ( m_sFileName ) )
				return;

			m_iOldApp = apps::GetAppFor ( m_sFileName );

			AlignFileName ( Item ( IDC_FILENAME ), m_sFileName );

			SHFILEINFO tShFileInfo;
			tShFileInfo.iIcon = -1;
			HIMAGELIST hImageList = (HIMAGELIST) SHGetFileInfo ( m_sFileName, 0, &tShFileInfo, sizeof (SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_LARGEICON );
			if ( hImageList )
			{
				HICON hIcon = ImageList_GetIcon ( hImageList, tShFileInfo.iIcon, ILD_NORMAL );
				if ( hIcon )
					SendMessage ( Item ( IDC_ICON_APP ), STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon );
			}
		}

		SetCursor ( LoadCursor ( NULL, IDC_ARROW ) );

		m_hList = Item ( IDC_LIST );

		// init list
		ListView_SetImageList ( m_hList, g_tResources.m_hSmallIconList, LVSIL_SMALL );

		LVCOLUMN tColumn;
		ZeroMemory ( &tColumn, sizeof ( tColumn ) );
		tColumn.mask = LVCF_TEXT;

		ListView_InsertColumn ( m_hList, tColumn.iSubItem,  &tColumn );

		PopulateList ();

		SetResizer ( m_hList );
		AddStayer ( Item ( IDC_ALWAYS ) );
		AddStayer ( Item ( IDC_QUOTES ) );
		AddStayer ( Item ( IDC_BROWSE ) );
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Resizer_c::OnCommand ( iItem, iNotify );

		switch ( iItem )
		{
		case IDC_BROWSE:
			Browse ();
			break;

		case IDOK:
		case IDCANCEL:
			if ( CanClose ( iItem == IDOK ) )
				Close ( iItem );
			break;
		}
	}

	virtual void OnClose ()
	{
		ListView_SetImageList ( m_hList, NULL, LVSIL_SMALL );
	}

	virtual void OnSettingsChange ()
	{
		ListView_SetColumnWidth ( m_hList, 0, GetColumnWidthRelative ( 1.0f ) );
		Dialog_Resizer_c::OnSettingsChange ();
	}


	int GetOldApp () const
	{
		return m_iOldApp;
	}


	int GetNewApp () const
	{
		return m_iNewApp;
	}


	bool OpenAlways () const
	{
		return m_bAlways;
	}


	bool Quotes () const
	{
		return m_bQuotes;
	}

private:
	HWND			m_hList;
	Str_c			m_sFileName;
	int				m_iNewApp;
	int				m_iOldApp;
	bool			m_bAlways;
	bool			m_bNoSource;
	bool			m_bQuotes;

	void PopulateList ()
	{
		ListView_DeleteAllItems ( m_hList );

		LVITEM tItem;
		memset ( &tItem, 0, sizeof ( tItem ) );

		int nRecommended = 0;

		for ( int i = 0; i < apps::GetNumApps (); ++i )
		{
			const apps::AppInfo_t & tAppInfo = apps::GetApp ( i );
			if ( tAppInfo.m_bRecommended )
			{
				if ( ! nRecommended )
				{
					tItem.mask		= LVIF_TEXT | LVIF_PARAM;
					tItem.pszText	= (wchar_t *)Txt ( T_DLG_RCMD_PROGRAM );
					tItem.lParam	= -1;
					ListView_InsertItem ( m_hList, &tItem );
				}

				tItem.mask		= LVIF_TEXT | LVIF_IMAGE | LVIF_INDENT | LVIF_PARAM;
				tItem.pszText	= (wchar_t *) tAppInfo.m_sName.c_str ();
				tItem.iImage	= tAppInfo.m_iIcon;
				tItem.iItem		= ++nRecommended;
				tItem.iIndent	= 1;
				tItem.lParam	= i;
				ListView_InsertItem ( m_hList, &tItem );
			}
		}

		if ( nRecommended > 0 )
		{
			tItem.mask		= LVIF_TEXT | LVIF_PARAM;
			tItem.pszText	= (wchar_t *)Txt ( T_DLG_OTHER_PROGRAMS );
			tItem.iItem		= ++nRecommended;
			tItem.lParam	= -1;
			ListView_InsertItem ( m_hList, &tItem );
		}

		tItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_INDENT | LVIF_PARAM;
		for ( int i = 0; i < apps::GetNumApps (); ++i )
		{
			const apps::AppInfo_t & tAppInfo = apps::GetApp ( i );
			if ( ! tAppInfo.m_bRecommended )
			{
				tItem.pszText	= (wchar_t *) tAppInfo.m_sName.c_str ();
				tItem.iImage	= tAppInfo.m_iIcon;
				tItem.iItem		= i + nRecommended + 1;
				tItem.iIndent	= nRecommended > 0 ? 1 : 0;
				tItem.lParam	= i;
				ListView_InsertItem ( m_hList, &tItem );
			}
		}
	}


	bool CanClose ( bool bOk )
	{
		m_bAlways = IsChecked ( IDC_ALWAYS );
		m_bQuotes = IsChecked ( IDC_QUOTES );

		if ( bOk )
		{
			int iSelected = ListView_GetSelectionMark ( m_hList );
			if ( iSelected == -1 )
				return false;

			LVITEM tItem;
			memset ( &tItem, 0, sizeof ( tItem ) );
			tItem.iItem = iSelected;
			tItem.mask = LVIF_PARAM;
			if ( ! ListView_GetItem ( m_hList, &tItem ) )
				return false;

			if ( tItem.lParam == -1 )
				return false;

			m_iNewApp = tItem.lParam;
		}

		return true;
	}

	void Browse ()
	{
		Str_c sFile;
		TreeFilters_t dFilters;
		dFilters.Add ( TreeFilterEntry_t ( Txt ( T_FILTER_APP ), L"exe" ) );

		if ( ShowFileTreeDlg ( m_hWnd, sFile, false, &dFilters ) )
		{
			// add to apps
			int iApp = apps::FindAppByFilename ( sFile);
			int iAppInList = -1;

			if ( iApp == -1 )
			{
				Str_c sDir, sName, sExt;
				SplitPath ( sFile, sDir, sName, sExt );

				apps::AppInfo_t tApp;
				tApp.m_sName 		= sName + sExt;
				tApp.m_sFileName 	= sFile;
				tApp.m_bBuiltIn 	= false;
				tApp.LoadIcon ();

				int iApp = apps::AddApp ( tApp );

				int nItems = ListView_GetItemCount ( m_hList );

				LVITEM tItem;
				memset ( &tItem, 0, sizeof ( tItem ) );
				tItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_INDENT | LVIF_PARAM;
				tItem.pszText = (wchar_t *) tApp.m_sName.c_str ();
				tItem.lParam = iApp;
				tItem.iImage = tApp.m_iIcon;
				tItem.iItem = nItems;
				tItem.iIndent = ( m_iOldApp != -1 ) ? 1 : 0;
				ListView_InsertItem ( m_hList, &tItem );

				iAppInList = nItems;
			}
			else
			{
				const apps::AppInfo_t & tAppInfo = apps::GetApp ( iApp );
				LVFINDINFO tFindInfo;
				memset ( &tFindInfo, 0, sizeof ( LVFINDINFO ) );
				tFindInfo.flags = LVFI_PARAM;
				tFindInfo.lParam = iApp;
				iAppInList = ListView_FindItem ( m_hList, 0, &tFindInfo );
			}
			
			ListView_SetItemState ( m_hList, iAppInList, LVIS_SELECTED, LVIS_SELECTED );
			ListView_SetSelectionMark ( m_hList, iAppInList );
			ListView_EnsureVisible ( m_hList, iAppInList, FALSE );
		}
	}
};


void ShowOpenWithDialog ( const Str_c & sFileName )
{
	OpenWithDlg_c OpenWithDlg ( sFileName );
	
	if ( OpenWithDlg.Run ( IDD_APPS, g_hMainWindow ) == IDOK )
	{
		int iNewApp = OpenWithDlg.GetNewApp ();
		Assert ( iNewApp != -1 );
		
		const apps::AppInfo_t & tAppInfo = apps::GetApp ( iNewApp );

		Str_c sNameToExec = Str_c ( L"\"" ) + sFileName + L"\"";

		SHELLEXECUTEINFO tExecuteInfo;
		memset ( &tExecuteInfo, 0, sizeof ( SHELLEXECUTEINFO ) );
		tExecuteInfo.cbSize	= sizeof ( SHELLEXECUTEINFO );
		tExecuteInfo.fMask		 = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
		tExecuteInfo.lpFile		 = tAppInfo.m_sFileName;
		tExecuteInfo.lpParameters= OpenWithDlg.Quotes () ? sNameToExec : sFileName;
		tExecuteInfo.nShow		 = SW_SHOW;
		tExecuteInfo.hInstApp	 = g_hAppInstance;
		ShellExecuteEx ( &tExecuteInfo );

		g_tRecent.AddOpenWith ( GetExt ( sFileName ), tAppInfo.m_sName, tAppInfo.m_sFileName, OpenWithDlg.Quotes () );

		if ( OpenWithDlg.OpenAlways () )
			apps::Associate ( sFileName, iNewApp, OpenWithDlg.Quotes () );
	}
}

//////////////////////////////////////////////////////////////////////////
class RunParamsDlg_c : public Dialog_Moving_c
{
public:
	RunParamsDlg_c ( const wchar_t * szPath, const wchar_t * szParams )
		: Dialog_Moving_c ( L"RunParams", IDM_OK_CANCEL )
		, m_sPath	( szPath )
		, m_sParams	( szParams )
	{
	}

	virtual void OnInit ()
	{
		Dialog_Moving_c::OnInit ();

		Bold ( IDC_TITLE );

		Loc ( IDC_TITLE,			T_DLG_RUN_TITLE );
		Loc ( IDC_EXE_STATIC,		T_DLG_RUN_EXECUTABLE );
		Loc ( IDC_PARAMS_STATIC,	T_DLG_RUN_PARAMS );

		ItemTxt ( IDC_EXE,			m_sPath );
		ItemTxt ( IDC_PARAMS,		m_sParams );

		Loc ( IDOK,		T_TBAR_OK );
		Loc ( IDCANCEL, T_TBAR_CANCEL );
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Moving_c::OnCommand ( iItem, iNotify );

		switch ( iItem )
		{
		case IDOK:
			m_sPath = GetItemTxt ( IDC_EXE );
			m_sParams = GetItemTxt ( IDC_PARAMS );
		case IDCANCEL:
			Close ( iItem );
			break;
		}
	}

	const Str_c & Path () const
	{
		return m_sPath;
	}

	const Str_c & Params () const
	{
		return m_sParams;
	}

protected:
	Str_c	m_sPath;
	Str_c	m_sParams;
};


//////////////////////////////////////////////////////////////////////////

int ShowSelectAppDialog ( HWND hParent )
{
	int iApp = -1;
	OpenWithDlg_c OpenWithDlg;

	if ( OpenWithDlg.Run ( IDD_APP_SELECT, hParent ) == IDOK )
		iApp = OpenWithDlg.GetNewApp ();

	return iApp;
}

bool ShowRunParamsDialog ( Str_c & sPath, Str_c & sParams )
{
	RunParamsDlg_c Dlg ( sPath, sParams );
	if ( Dlg.Run ( IDD_RUN_PARAMS, g_hMainWindow ) == IDOK )
	{
		sPath = Dlg.Path ();
		sParams = Dlg.Params ();
		return true;
	}

	return false;
}