#include "pch.h"

#include "LDialogs/dcopy_progress.h"
#include "LCore/clog.h"
#include "LCore/cfile.h"
#include "LFile/fcopy.h"
#include "LSettings/sconfig.h"
#include "LSettings/slocal.h"
#include "LPanel/presources.h"
#include "LDialogs/dcopy.h"
#include "LUI/udialogs.h"
#include "LDialogs/dcommon.h"
#include "LDialogs/derrors.h"

#include "Resources/resource.h"
#include "aygshell.h"

extern HWND g_hMainWindow;

static FileCopier_c * g_pFileCopier = NULL;
static FileMover_c * g_pFileMover = NULL;

static Panel_c * g_pSourcePanel = NULL;

const DWORD MY_UNIQUE_TIMER_COPY = 0xABCDEF00;
const DWORD MY_UNIQUE_TIMER_MOVE = 0xABCDEF04;

class FileCopyProgressDlg_c : public WindowProgress_c
{
public:

	void Init ( HWND hDlg )
	{
		DlgTxt ( hDlg, IDC_FROM, T_DLG_FROM );
		DlgTxt ( hDlg, IDC_TO, T_DLG_TO );

		WindowProgress_c::Init ( hDlg, g_pFileCopier->GetCopyMode () == FileCopier_c::MODE_COPY ? Txt ( T_DLG_COPYING ) : Txt ( T_DLG_MOVING ), g_pFileCopier );
		
		InitFullscreenDlg ( hDlg, IDM_CANCEL, SHCMBF_HIDESIPBUTTON, true );

		SetTimer ( hDlg, MY_UNIQUE_TIMER_COPY, 0, NULL );
	}


	void UpdateProgress ()
	{
		if ( g_pFileCopier->GetNameFlag () )
		{
			const Str_c & sSource = g_pFileCopier->GetSourceFileName ();
			const Str_c & sDest = g_pFileCopier->GetDestFileName ();

			AlignFileName ( m_hSourceText, sSource );
			AlignFileName ( m_hDestText, sDest );
		}

		if ( WindowProgress_c::UpdateProgress ( g_pFileCopier ) )
			g_pFileCopier->ResetChangeFlags ();
	}

	void Close ()
	{
		CloseFullscreenDlg ();
	}
};


static FileCopyProgressDlg_c * g_pFileCopyProgressDlg = NULL;


static BOOL CALLBACK DlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	Assert ( g_pFileCopyProgressDlg && g_pFileCopier );

	switch ( Msg )
	{
	case WM_INITDIALOG:
		g_pFileCopyProgressDlg->Init ( hDlg );
		g_pFileCopier->SetWindow ( hDlg );
		break;

	case WM_TIMER:
		if ( wParam == MY_UNIQUE_TIMER_COPY )
		{
			if ( ! g_pFileCopier->IsInDialog () )
			{
				if ( g_pFileCopier->DoCopyWork () )
					g_pFileCopyProgressDlg->UpdateProgress ();
				else
					PostMessage ( hDlg, WM_CLOSE, 0, 0 );
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			g_pFileCopyProgressDlg->Close ();
			g_pFileCopier->Cancel ();
			EndDialog ( hDlg, LOWORD (wParam) );
			break;
		}
		break;

	case WM_HELP:
		Help ( L"CopyMove" );
		return TRUE;
	}

	HandleActivate ( hDlg, Msg, wParam, lParam );

	return FALSE;
}


static BOOL CALLBACK MoveDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	switch ( Msg )
	{
	case WM_INITDIALOG:
		{
			DlgTxt ( hDlg, IDC_TEXT, T_DLG_MOVING );
			DlgTxt ( hDlg, IDCANCEL, T_TBAR_CANCEL );
			SetTimer ( hDlg, MY_UNIQUE_TIMER_MOVE, 0, NULL );
			g_pFileMover->SetWindow ( hDlg );
		}
		break;

	case WM_TIMER:
		if ( wParam == MY_UNIQUE_TIMER_MOVE )
		{
			if ( ! g_pFileMover->IsInDialog () )
				if ( ! g_pFileMover->MoveNext () )
					EndDialog ( hDlg, LOWORD (wParam) );
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog ( hDlg, IDCANCEL );
			break;
		}
		break;

	case WM_HELP:
		Help ( L"CopyMove" );
		return TRUE;
	}

	return FALSE;
}



///////////////////////////////////////////////////////////////////////////////////////////
// "preparing..." dialog stuff
static void MarkCallback ( const Str_c & sFileName, bool bMark )
{	
	if ( g_pSourcePanel )
		g_pSourcePanel->MarkFile ( sFileName, bMark );
}


///////////////////////////////////////////////////////////////////////////////////////////
// wrapper funcs
void DlgCopyFiles ( Panel_c * pSource, const wchar_t * szDest, FileList_t & tList )
{
	g_pSourcePanel = pSource;
	Assert ( ! g_pFileCopier );

	Str_c sDest = szDest;
	if ( sDest.Empty () )
	{
		ShowErrorDialog ( g_hMainWindow, Txt ( T_MSG_WARNING ), Txt ( T_MSG_WRONG_DEST ), IDD_ERROR_OK );
		return;
	}

	PrepareDestDir ( sDest, tList.m_sRootDir );

	g_pFileCopier = new FileCopier_c ( sDest, tList, false, PrepareCallback, MarkCallback );

	DestroyPrepareDialog ();

	g_pFileCopyProgressDlg = new FileCopyProgressDlg_c;
	DialogBox ( ResourceInstance (), MAKEINTRESOURCE ( IDD_COPY_PROGRESS ), g_hMainWindow, DlgProc );
	SafeDelete ( g_pFileCopyProgressDlg );
	SafeDelete ( g_pFileCopier );
}

void DlgMoveFiles ( Panel_c * pSource, const wchar_t * szDest, FileList_t & tList )
{
	g_pSourcePanel = pSource;
	Assert ( ! g_pFileCopier );
	Str_c sDest = szDest;

	if ( sDest.Empty () )
	{
		ShowErrorDialog ( g_hMainWindow, Txt ( T_MSG_WARNING ), Txt ( T_MSG_WRONG_DEST ), IDD_ERROR_OK );
		return;
	}

	PrepareDestDir ( sDest, tList.m_sRootDir );

	if ( AreFilesOnDifferentVolumes ( sDest, tList.m_sRootDir ) )
	{
		g_pFileCopier = new FileCopier_c ( sDest, tList, true, PrepareCallback, MarkCallback );

		DestroyPrepareDialog ();

		g_pFileCopyProgressDlg = new FileCopyProgressDlg_c;
		DialogBox ( ResourceInstance (), MAKEINTRESOURCE ( IDD_COPY_PROGRESS ), g_hMainWindow, DlgProc );
		SafeDelete ( g_pFileCopyProgressDlg );
		SafeDelete ( g_pFileCopier );
	}
	else
	{
		g_pFileMover = new FileMover_c ( sDest, tList, MarkCallback );
		DialogBox ( ResourceInstance (), MAKEINTRESOURCE ( IDD_WAITBOX_CANCEL ), g_hMainWindow, MoveDlgProc );
		SafeDelete ( g_pFileMover );
	}
}