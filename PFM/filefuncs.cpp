#include "pch.h"
#include "filefuncs.h"

#include "config.h"

#include "LDialogs/dcopy.h"
#include "LDialogs/dcopy_progress.h"
#include "LDialogs/doptions.h"
#include "LDialogs/dfile_properties.h"
#include "LDialogs/dsend.h"
#include "LFile/fcopy.h"
#include "LFile/fapps.h"
#include "LFile/fmessages.h"
#include "LComm/ccomm.h"

#include "pfm.h"
#include "plugins.h"
#include "dialogs/apps.h"
#include "dialogs/delete.h"
#include "dialogs/find.h"
#include "dialogs/bookmarks.h"
#include "dialogs/filter.h"
#include "dialogs/dmkdir.h"
#include "dialogs/register.h"
#include "dialogs/shortcut.h"
#include "dialogs/errors.h"

#include "Dlls/Resource/resource.h"

extern HWND g_hMainWindow;
extern HINSTANCE g_hAppInstance;


void FMMkDir ()
{
	Str_c sDir;
	if ( ShowMkDirDialog ( sDir ) )
	{
		if ( ! sDir.Empty () )
		{
			// FIXME
/*			Panel_c * pPanel1 = FMGetActivePanel ();
			Panel_c * pPanel2 = FMGetPassivePanel ();

			if ( MakeDirectory ( sDir, pPanel1->GetDirectory () ) )
			{
				pPanel1->SoftRefresh ();

				if ( pPanel2->GetDirectory () == pPanel1->GetDirectory () )
					FMGetPassivePanel ()->SoftRefresh ();

				pPanel1->SetCursorAtFile ( sDir );
			}*/
		}
	}
}


void FMFindFiles ()
{
	Panel_c * pPanel = FMGetActivePanel ();
	const SelectedFileList_t * pList = pPanel->GetSelectedItems ();

	if ( !pList )
		return;

	bool bHandlesFind = g_PluginManager.HandlesFind ( pPanel->GetActivePlugin () );
	bool bHavePlugin = pPanel->GetActivePlugin () != INVALID_HANDLE_VALUE;

	if ( !bHavePlugin )
	{
		FileSearchSettings_t tSettings;
		if ( FindRequestDlg ( tSettings ) )
		{
			const SelectedFileList_t * pList = pPanel->GetSelectedItems ();
			Str_c sDir, sFile;
			if ( FindProgressDlg ( *pList, tSettings, sDir, sFile ) )
			{
				pPanel->SetDirectory ( sDir );
				pPanel->Refresh ();
				pPanel->SetCursorAtItem ( sFile );
			}
		}
	}
	else
		if ( bHandlesFind )
		{
			PanelItem_t ** ppFiles = pList->m_dFiles.Empty () ? NULL : (PanelItem_t **)&(pList->m_dFiles [0]);
			g_PluginManager.FindFiles ( pPanel->GetActivePlugin (), ppFiles, pList->m_dFiles.Length () );
		}
}


void FMExit ()
{
	int iRes = IDOK;
	if ( cfg->conf_exit )
		iRes = ShowMessageDialog ( g_hMainWindow, Txt ( T_MSG_EXIT_HEAD ), Txt ( T_MSG_EXIT_TEXT ), DLG_ERROR_YES_NO );

	if ( iRes == IDOK )
		PostQuitMessage ( 0 );
}


void FMSelectFilter ( bool bMark )
{
	Str_c sFilter;

	if ( ShowSetFilterDialog ( sFilter, bMark ) )
	{
		Panel_c * pPanel = FMGetActivePanel ();
		Assert ( pPanel );
		pPanel->MarkFilter ( sFilter, bMark );
		g_tRecent.AddFilter ( sFilter );
	}
}


void FMExecuteFile ( const Str_c & sFile )
{
	Str_c sParams;

	SHELLEXECUTEINFO tExecuteInfo;
	memset ( &tExecuteInfo, 0, sizeof ( SHELLEXECUTEINFO ) );
	tExecuteInfo.cbSize	= sizeof ( SHELLEXECUTEINFO );
	tExecuteInfo.fMask		= SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
	tExecuteInfo.lpFile		= sFile;
	tExecuteInfo.nShow		= SW_SHOW;
	tExecuteInfo.hInstApp	= g_hAppInstance;

	const wchar_t * szExt = GetNameExtFast ( sFile );
	bool bLnk = wcscmp ( szExt, L"lnk" ) == 0;
	bool bDir = false, bRes = false;

	Str_c sPath = sFile;
	if ( bLnk )
		bRes = DecomposeLnk ( sPath, sParams, bDir );
	
	if ( bLnk && bRes && bDir )
	{
		sPath.Strip ( L'\"' );
		FMGetActivePanel ()->SetDirectory ( sPath );
		FMGetActivePanel ()->Refresh ();
	}
	else
	{
		tExecuteInfo.lpParameters = sParams;
		if ( !ShellExecuteEx ( &tExecuteInfo ) )
		{
			Str_c sError = GetExecuteError ( GetLastError (), bLnk );
			if ( !sError.Empty () )
				ShowErrorDialog ( g_hMainWindow, false, sError, DLG_ERROR_OK );
		}
		else
			FMExecute_Event ();
	}
}


void FMCopyMoveFiles ()
{
	// FIXME
	/*	
	Panel_c * pPanel = FMGetActivePanel ();
	SelectedFileList_t tList;

	if ( ! pPanel->GenerateFileList ( tList ) )
		return;

	Str_c sDest = FMGetPassivePanel ()->GetDirectory ();
	int iRes = ShowCopyMoveDialog ( sDest, tList );
	switch ( iRes )
	{
	case IDC_COPY:
		DlgCopyFiles ( pPanel, sDest, tList );
		g_tRecent.AddCopyMove ( sDest );
		FMGetPanel1 ()->SoftRefresh ();
		FMGetPanel2 ()->SoftRefresh ();
		break;

	case IDC_MOVE:
		DlgMoveFiles ( pPanel, sDest, tList );
		g_tRecent.AddCopyMove ( sDest );
		FMGetPanel1 ()->SoftRefresh ();
		FMGetPanel2 ()->SoftRefresh ();
		break;

	case IDC_COPY_SHORTCUT:
		FMCreateShortcut ( sDest, false );
		break;
	}*/
}


void FMRenameFiles ()
{
	/*
	Panel_c * pPanel = FMGetActivePanel ();
	SelectedFileList_t tList;

	if ( pPanel->GenerateFileList ( tList ) )
		if ( tList.m_dFiles.Length () == 1 )
		{
			Str_c sDestName;
			if ( ShowRenameFilesDialog ( sDestName, tList ) == IDOK )
			{
				DlgMoveFiles ( pPanel, sDestName, tList );
				pPanel->SoftRefresh ();
				FMGetPassivePanel ()->SoftRefresh ();
				g_tRecent.AddRename ( sDestName );
				pPanel->SetCursorAtFile ( sDestName );
			}
		}*/
}


void FMDeleteFiles ()
{
	Panel_c * pPanel = FMGetActivePanel ();
	const SelectedFileList_t * pList = pPanel->GetSelectedItems ();

	if ( !pList )
		return;

	bool bRefresh = false;

	bool bHandlesDelete = g_PluginManager.HandlesDelete ( pPanel->GetActivePlugin () );
	bool bHandlesRealNames = g_PluginManager.HandlesRealNames ( pPanel->GetActivePlugin () );

	if ( bHandlesDelete )
	{
		if ( g_PluginManager.DeleteFiles ( pPanel->GetActivePlugin (), (PanelItem_t **)&(pList->m_dFiles [0]), pList->m_dFiles.Length () ) )
			bRefresh = true;
	}
	else
		if ( bHandlesRealNames )
		{
			if ( DeleteRequestDlg ( *pList ) )
			{
				DeleteProgressDlg ( *pList );
				bRefresh = true;
			}
		}

	if ( bRefresh )
	{
		pPanel->SoftRefresh ();
		FMGetPassivePanel ()->SoftRefresh ();
	}
}


void FMCreateShortcut ( const Str_c & sDest, bool bShowDialog )
{
	/*
	Panel_c * pPanel = FMGetActivePanel ();
	SelectedFileList_t tList;

	if ( pPanel->GenerateFileList ( tList ) )
		if ( tList.m_dFiles.Length () == 1 )
		{
			wchar_t szName [MAX_PATH];

			GetNameExtFast ( tList.m_dFiles [0]->m_tData.cFileName, szName );
			Str_c sName;
			if ( szName )
				sName = szName;

			Str_c sParams = Str_c ( L"\"" ) + tList.m_sRootDir + tList.m_dFiles [0]->m_tData.cFileName + L"\"";
			Str_c sDestDir = sDest;
			bool bCreate = true;
			if ( bShowDialog )
				bCreate = ShowShortcutDialog ( sName, sParams, sDestDir );

			if ( bCreate )
			{
				g_tRecent.AddShortcut ( sDestDir );
				PrepareDestDir ( sDestDir, tList.m_sRootDir );
				AppendSlash ( sDestDir );
				if ( CreateShortcut ( sDestDir + sName + L".lnk", sParams ) )
				{
					pPanel->SoftRefresh ();
					FMGetPassivePanel ()->SoftRefresh ();
				}
			}
		}*/
}


void FMFileProperties ()
{
	/*
	Panel_c * pPanel = FMGetActivePanel ();

	SelectedFileList_t tList;

	if ( pPanel->GenerateFileList ( tList ) )
	{
		// if there are any changes
		if ( ShowFilePropertiesDialog ( tList ) )
		{
			FMGetActivePanel ()->SoftRefresh ();
			FMGetPassivePanel ()->SoftRefresh ();
		}
	}*/
}

void FMOpenWith ()
{
	Panel_c * pPanel = FMGetActivePanel ();
	const SelectedFileList_t * pList = pPanel->GetSelectedItems ();
	if ( pList && pList->OneFile () && !pList->IsDir () )
		OpenWithDlg ( pList->m_sRootDir + pList->m_dFiles [0]->m_FindData.cFileName );
}

void FMRunParams ()
{
/*	Panel_c * pPanel = FMGetActivePanel ();
	SelectedFileList_t tList;
	if ( pPanel->GenerateFileList ( tList ) )
		if ( tList.OneFile () && ! tList.IsDir () && Str_c ( tList.m_dFiles [0]->m_tData.cFileName ).Ends ( L".exe" ) )
		{
			Str_c sExe ( tList.m_sRootDir + tList.m_dFiles [0]->m_tData.cFileName ), sParams;
			if ( ShowRunParamsDialog ( sExe, sParams ) )
			{
				SHELLEXECUTEINFO tExecuteInfo;
				memset ( &tExecuteInfo, 0, sizeof ( SHELLEXECUTEINFO ) );
				tExecuteInfo.cbSize	= sizeof ( SHELLEXECUTEINFO );
				tExecuteInfo.fMask		= SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
				tExecuteInfo.lpFile		= sExe;
				tExecuteInfo.lpParameters = sParams;
				tExecuteInfo.nShow		= SW_SHOW;
				tExecuteInfo.hInstApp	= g_hAppInstance;
				ShellExecuteEx ( & tExecuteInfo );
			}
		}*/
}

void FMViewFile ()
{
/*	Panel_c * pPanel = FMGetActivePanel ();

	SelectedFileList_t tList;

	if ( pPanel->GenerateFileList ( tList ) )
		if ( tList.OneFile () && ! tList.IsDir () )
		{
			Str_c sFileName = tList.m_sRootDir + tList.m_dFiles [0]->m_tData.cFileName;

			Str_c sParams = cfg->ext_viewer_params;
			int iPos;
			while ( ( iPos = sParams.Find ( L"%1" ) ) != -1 )
				sParams = sParams.SubStr ( 0, iPos ) + sFileName + sParams.SubStr ( iPos + 2 );
			
			SHELLEXECUTEINFO tExecuteInfo;
			memset ( &tExecuteInfo, 0, sizeof ( SHELLEXECUTEINFO ) );
			tExecuteInfo.cbSize	= sizeof ( SHELLEXECUTEINFO );
			tExecuteInfo.fMask		 = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
			tExecuteInfo.lpFile		 = cfg->ext_viewer;
			tExecuteInfo.lpParameters= sParams;
			tExecuteInfo.nShow		 = SW_SHOW;
			tExecuteInfo.hInstApp	 = g_hAppInstance;
			ShellExecuteEx ( &tExecuteInfo );
		}*/
}


void FMOpenInOpposite ()
{
	Panel_c * pPanel = FMGetActivePanel ();
	const SelectedFileList_t * pList = pPanel->GetSelectedItems ();

	if ( pList && !pList->IsPrevDir () && pList->OneFile () && pList->IsDir () )
	{
		FMGetPassivePanel ()->SetDirectory ( pPanel->GetDirectory () + pList->m_dFiles [0]->m_FindData.cFileName );
		FMGetPassivePanel ()->Refresh ();
	}
}

void FMOpenFolder ()
{
/*	Panel_c * pPanel = FMGetActivePanel ();
	Str_c sDir;
	if ( ShowOpenDirDialog ( sDir ) )
	{
		pPanel->SetDirectory ( sDir );
		pPanel->Refresh ();
	}*/
}

void FMSendIR ()
{
/*	Panel_c * pPanel = FMGetActivePanel ();
	SelectedFileList_t tList;
	if ( pPanel->GenerateFileList ( tList ) )
		ShowIRDlg ( tList );*/
}

void FMSendBT ()
{
	if ( ! IsWidcommBTPresent () && ! IsMSBTPresent () )
		return;

/*	Panel_c * pPanel = FMGetActivePanel ();
	SelectedFileList_t tList;
	if ( pPanel->GenerateFileList ( tList ) )
		ShowBTDlg ( tList );*/
}

void FMAddBookmark ()
{
	/*
	Panel_c * pPanel = FMGetActivePanel ();
	SelectedFileList_t tList;
	if ( pPanel->GenerateFileList ( tList ) )
	{
		if ( ! tList.IsPrevDir () && tList.OneFile () )
			ShowAddBMDialog ( tList.m_sRootDir + tList.m_dFiles [0]->m_tData.cFileName );
	}*/
}