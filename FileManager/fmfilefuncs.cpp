#include "pch.h"

#include "Filemanager/fmfilefuncs.h"
#include "LCore/cfile.h"
#include "LSettings/srecent.h"
#include "LSettings/sconfig.h"
#include "LSettings/slocal.h"
#include "LDialogs/dmodule.h"
#include "LDialogs/dcommon.h"
#include "LDialogs/dfilter.h"
#include "LDialogs/dcopy.h"
#include "LDialogs/dcopy_progress.h"
#include "LDialogs/ddelete.h"
#include "LDialogs/ddelete_progress.h"
#include "LDialogs/dmkdir.h"
#include "LDialogs/doptions.h"
#include "LDialogs/dfind.h"
#include "LDialogs/dfind_progress.h"
#include "LDialogs/dfile_properties.h"
#include "LDialogs/dregister.h"
#include "LDialogs/derrors.h"
#include "LDialogs/dbookmarks.h"
#include "LDialogs/dcrypt.h"
#include "LDialogs/dshortcut.h"
#include "LDialogs/dapps.h"
#include "LDialogs/dsend.h"
#include "LFile/ferrors.h"
#include "LFile/fmkdir.h"
#include "LFile/fcards.h"
#include "LFile/fcopy.h"
#include "LFile/fapps.h"
#include "LFile/fmessages.h"
#include "LPanel/pfile.h"
#include "LComm/ccomm.h"
#include "Filemanager/filemanager.h"
#include "Resources/resource.h"

extern HWND g_hMainWindow;
extern HINSTANCE g_hAppInstance;


void FMMkDir ()
{
	Str_c sDir;
	if ( ShowMkDirDialog ( sDir ) )
	{
		if ( ! sDir.Empty () )
		{
			Panel_c * pPanel1 = FMGetActivePanel ();
			Panel_c * pPanel2 = FMGetPassivePanel ();

			if ( MakeDirectory ( sDir, pPanel1->GetDirectory () ) )
			{
				pPanel1->SoftRefresh ();

				if ( pPanel2->GetDirectory () == pPanel1->GetDirectory () )
					FMGetPassivePanel ()->SoftRefresh ();

				pPanel1->SetCursorAtFile ( sDir );
			}
		}
	}
}


void FMFindFiles ()
{
	FileSearchSettings_t tSettings;
	if ( ShowFindDialog ( tSettings ) )
	{
		FileList_t tList;
		Panel_c * pPanel = FMGetActivePanel ();
		Assert ( pPanel );
		pPanel->GenerateFileList ( tList );

		Str_c sDir, sFile;
		if ( DlgFindProgress ( tList, tSettings, sDir, sFile ) )
		{
			pPanel->SetDirectory ( sDir );
			pPanel->Refresh ();
			pPanel->SetCursorAtFile ( sFile );
		}
	}
}


void FMExit ()
{
	int iRes = IDOK;
	if ( g_tConfig.conf_exit )
		iRes = ShowErrorDialog ( g_hMainWindow, Txt ( T_MSG_EXIT_HEAD ), Txt ( T_MSG_EXIT_TEXT ), IDD_ERROR_YES_NO );

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


void FMExecuteFile ( const Str_c & sDir, const Str_c & sFile )
{
	Str_c sPath = sDir + sFile, sParams;

	SHELLEXECUTEINFO tExecuteInfo;
	memset ( &tExecuteInfo, 0, sizeof ( SHELLEXECUTEINFO ) );
	tExecuteInfo.cbSize	= sizeof ( SHELLEXECUTEINFO );
	tExecuteInfo.fMask		= SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
	tExecuteInfo.lpFile		= sPath;
	tExecuteInfo.nShow		= SW_SHOW;
	tExecuteInfo.hInstApp	= g_hAppInstance;

	const wchar_t * szExt = GetNameExtFast ( sFile );
	bool bLnk = wcscmp ( szExt, L"lnk" ) == 0;

	bool bDir, bRes = false;
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
		if ( ! ShellExecuteEx ( &tExecuteInfo ) )
		{
			Str_c sError = GetExecuteError ( GetLastError (), bLnk );
			if ( ! sError.Empty () )
				ShowErrorDialog ( g_hMainWindow, Txt ( T_MSG_ERROR ), sError, IDD_ERROR_OK );
		}
		else
			FMExecute_Event ();
	}
}


void FMCopyMoveFiles ()
{
	Panel_c * pPanel = FMGetActivePanel ();
	FileList_t tList;

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
	}
}


void FMRenameFiles ()
{
	Panel_c * pPanel = FMGetActivePanel ();
	FileList_t tList;

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
		}
}


void FMDeleteFiles ()
{
	Panel_c * pPanel = FMGetActivePanel ();
	FileList_t tList;

	if ( pPanel->GenerateFileList ( tList ) )
	{
		int iRes = ShowDeleteDialog ( tList );
		if ( iRes != IDCANCEL )
		{
			DeleteFiles ( tList );
			pPanel->SoftRefresh ();
			FMGetPassivePanel ()->SoftRefresh ();
		}
	}
}

void FMCreateShortcut ( const Str_c & sDest, bool bShowDialog )
{
	Panel_c * pPanel = FMGetActivePanel ();
	FileList_t tList;

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
		}
}
	
void FMEncryptFiles ( bool bEncrypt )
{
	Panel_c * pPanel = FMGetActivePanel ();
	FileList_t tList;

	if ( pPanel->GenerateFileList ( tList ) )
	{
		Str_c sPassword;
		int iRes = ShowCryptDialog ( pPanel, bEncrypt, tList, sPassword );
		if ( iRes != IDCANCEL )
		{
			ShowCryptProgressDlg ( pPanel, bEncrypt, tList, sPassword );
			pPanel->SoftRefresh ();
			FMGetPassivePanel ()->SoftRefresh ();
		}
	}
}


void FMFileProperties ()
{
	Panel_c * pPanel = FMGetActivePanel ();

	FileList_t tList;

	if ( pPanel->GenerateFileList ( tList ) )
	{
		// if there are any changes
		if ( ShowFilePropertiesDialog ( tList ) )
		{
			FMGetActivePanel ()->SoftRefresh ();
			FMGetPassivePanel ()->SoftRefresh ();
		}
	}
}

void FMOpenWith ()
{
	Panel_c * pPanel = FMGetActivePanel ();
	FileList_t tList;
	if ( pPanel->GenerateFileList ( tList ) )
		if ( tList.OneFile () && ! tList.IsDir () )
			ShowOpenWithDialog ( tList.m_sRootDir + tList.m_dFiles [0]->m_tData.cFileName );
}

void FMRunParams ()
{
	Panel_c * pPanel = FMGetActivePanel ();
	FileList_t tList;
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
		}
}

void FMViewFile ()
{
	Panel_c * pPanel = FMGetActivePanel ();

	FileList_t tList;

	if ( pPanel->GenerateFileList ( tList ) )
		if ( tList.OneFile () && ! tList.IsDir () )
		{
			Str_c sFileName = tList.m_sRootDir + tList.m_dFiles [0]->m_tData.cFileName;

			Str_c sParams = g_tConfig.ext_viewer_params;
			int iPos;
			while ( ( iPos = sParams.Find ( L"%1" ) ) != -1 )
				sParams = sParams.SubStr ( 0, iPos ) + sFileName + sParams.SubStr ( iPos + 2 );
			
			SHELLEXECUTEINFO tExecuteInfo;
			memset ( &tExecuteInfo, 0, sizeof ( SHELLEXECUTEINFO ) );
			tExecuteInfo.cbSize	= sizeof ( SHELLEXECUTEINFO );
			tExecuteInfo.fMask		 = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
			tExecuteInfo.lpFile		 = g_tConfig.ext_viewer;
			tExecuteInfo.lpParameters= sParams;
			tExecuteInfo.nShow		 = SW_SHOW;
			tExecuteInfo.hInstApp	 = g_hAppInstance;
			ShellExecuteEx ( &tExecuteInfo );
		}
}


void FMOpenInOpposite ()
{
	Panel_c * pPanel = FMGetActivePanel ();
	FileList_t tList;

	if ( pPanel->GenerateFileList ( tList ) )
		if ( ! tList.IsPrevDir () && tList.OneFile () && tList.IsDir () )
		{
			FMGetPassivePanel ()->SetDirectory ( ( FMGetActivePanel ()->GetDirectory () + tList.m_dFiles [0]->m_tData.cFileName ) );
			FMGetPassivePanel ()->Refresh ();
		}
}

void FMOpenFolder ()
{
	Panel_c * pPanel = FMGetActivePanel ();
	Str_c sDir;
	if ( ShowOpenDirDialog ( sDir ) )
	{
		pPanel->SetDirectory ( sDir );
		pPanel->Refresh ();
	}
}

void FMSendIR ()
{
	Panel_c * pPanel = FMGetActivePanel ();
	FileList_t tList;
	if ( pPanel->GenerateFileList ( tList ) )
		ShowIRDlg ( tList );
}

void FMSendBT ()
{
	if ( ! IsWidcommBTPresent () && ! IsMSBTPresent () )
		return;

	Panel_c * pPanel = FMGetActivePanel ();
	FileList_t tList;
	if ( pPanel->GenerateFileList ( tList ) )
		ShowBTDlg ( tList );
}

void FMAddBookmark ()
{
	Panel_c * pPanel = FMGetActivePanel ();
	FileList_t tList;
	if ( pPanel->GenerateFileList ( tList ) )
	{
		if ( ! tList.IsPrevDir () && tList.OneFile () )
			ShowAddBMDialog ( tList.m_sRootDir + tList.m_dFiles [0]->m_tData.cFileName );
	}
}