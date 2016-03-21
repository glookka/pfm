#include "pch.h"

#include "LDialogs/dmkdir.h"
#include "LSettings/sconfig.h"
#include "LFile/fmisc.h"
#include "LUI/udialogs.h"
#include "LDialogs/dcommon.h"
#include "LDialogs/derrors.h"
#include "LDialogs/dtree.h"
#include "LPanel/presources.h"
#include "LSettings/srecent.h"
#include "LSettings/slocal.h"

#include "aygshell.h"
#include "Resources/resource.h"

extern HWND g_hMainWindow;

class MkDirDlg_c : public Dialog_Moving_c
{
public:
	MkDirDlg_c ( const wchar_t * szHelp )
		: Dialog_Moving_c ( szHelp, IDM_OK_CANCEL )
	{
	}

	virtual void OnInit ()
	{
		Dialog_Moving_c::OnInit ();

		Loc ( IDC_TITLE,T_DLG_MKDIR_HEAD );
		Loc ( IDOK,		T_TBAR_OK );
		Loc ( IDCANCEL, T_TBAR_CANCEL );

		Bold ( IDC_TITLE );

		HWND hDirCombo = Item ( IDC_FOLDER_COMBO );
		PopulateDirList ( hDirCombo );
		SetFocus ( hDirCombo );
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Moving_c::OnCommand ( iItem, iNotify );

		switch ( iItem )
		{
		case IDOK:
			m_sDir = GetItemTxt ( IDC_FOLDER_COMBO );
		case IDCANCEL:
			Close ( iItem );
			break;
		}
	}

	const Str_c & Dir () const
	{
		return m_sDir;
	}

protected:
	Str_c m_sDir;

	virtual void PopulateDirList ( HWND hList )
	{
		SendMessage ( hList, CB_RESETCONTENT, 0, 0 );
		for ( int i = g_tRecent.GetNumDirs () - 1; i >= 0 ; --i )
			SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM) (g_tRecent.GetDir ( i ).c_str () ) );
	}
};

//////////////////////////////////////////////////////////////////////////
class OpenDirDlg_c : public MkDirDlg_c
{
public:
	OpenDirDlg_c ( const wchar_t * szHelp )
		: MkDirDlg_c ( szHelp )
	{
	}

	virtual void OnInit ()
	{
		MkDirDlg_c::OnInit ();

		Loc ( IDC_TITLE,	T_DLG_OPEN_TITLE );
		Loc	( IDC_TREE,		T_CMN_TREE );
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		MkDirDlg_c::OnCommand ( iItem, iNotify );

		if ( iItem == IDC_TREE )
		{
			m_sDir = GetItemTxt ( IDC_FOLDER_COMBO );
			if ( ShowDirTreeDlg ( m_hWnd, m_sDir ) )
				SetComboTextFocused ( Item ( IDC_FOLDER_COMBO ), m_sDir );
		}
	}

protected:
	virtual void PopulateDirList ( HWND hList )
	{
		SendMessage ( hList, CB_RESETCONTENT, 0, 0 );
		for ( int i = g_tRecent.GetNumOpenDirs () - 1; i >= 0 ; --i )
			SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM) (g_tRecent.GetOpenDir ( i ).c_str () ) );
	}
};

bool ShowMkDirDialog ( Str_c & sDir )
{
	MkDirDlg_c Dlg ( L"CreateFolder" );
	if ( Dlg.Run ( IDD_MKDIR, g_hMainWindow ) == IDOK )
	{
		sDir = Dlg.Dir ();

		if ( sDir.Empty () )
		{
			ShowErrorDialog ( g_hMainWindow, Txt ( T_MSG_WARNING ), Txt ( T_MSG_WRONG_FOLDER ), IDD_ERROR_OK );
			return false;
		}
		else
		{
			g_tRecent.AddDir ( sDir );
			return true;
		}
	}

	return false;
}

bool ShowOpenDirDialog ( Str_c & sDir )
{
	OpenDirDlg_c Dlg ( L"Overview" );
	if ( Dlg.Run ( IDD_OPENDIR, g_hMainWindow ) == IDOK )
	{
		sDir = Dlg.Dir ();

		if ( sDir.Empty () )
		{
			ShowErrorDialog ( g_hMainWindow, Txt ( T_MSG_WARNING ), Txt ( T_MSG_WRONG_FOLDER ), IDD_ERROR_OK );
			return false;
		}
		else
		{
			g_tRecent.AddOpenDir ( sDir );
			return true;
		}
	}

	return false;
}