#include "pch.h"

#include "LDialogs/dshortcut.h"
#include "LCore/clog.h"
#include "LSettings/sconfig.h"
#include "LSettings/slocal.h"
#include "LPanel/presources.h"
#include "LUI/udialogs.h"
#include "LSettings/srecent.h"
#include "LDialogs/dcommon.h"
#include "LDialogs/dtree.h"

#include "aygshell.h"
#include "Resources/resource.h"

extern HWND g_hMainWindow;

class ShortcutDlg_c : public Dialog_Fullscreen_c
{
public:
	ShortcutDlg_c ( Str_c & sName, Str_c & sParams, Str_c & sDest )
		: Dialog_Fullscreen_c ( L"Shortcut", IDM_OK_CANCEL )
		, m_sName	( sName )
		, m_sParams ( sParams )
		, m_sDest	( sDest )
		, m_hCombo	( NULL )
	{
	}

	virtual void OnInit ()
	{
		Dialog_Fullscreen_c::OnInit ();

		Bold ( IDC_TITLE );

		Loc ( IDC_TITLE,		T_DLG_SHORT_HEAD );
		Loc ( IDC_NAME_STATIC,	T_DLG_NAME_HEAD );
		Loc ( IDC_OBJECT_STATIC,T_DLG_SHORT_OBJECT );
		Loc ( IDC_DEST_STATIC,	T_DLG_SHORT_DEST );
		Loc	( IDC_TREE_OBJ,		T_CMN_TREE );
		Loc	( IDC_TREE_DEST,	T_CMN_TREE );

		ItemTxt ( IDC_NAME, m_sName );
		ItemTxt ( IDC_PARAMS, m_sParams );
		
		m_hCombo = Item ( IDC_DEST_COMBO );
		PopulateList ();
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Fullscreen_c::OnCommand ( iItem, iNotify );

		switch ( iItem )
		{
		case IDOK:
		case IDCANCEL:
			m_sName = GetItemTxt ( IDC_NAME );
			m_sParams = GetItemTxt ( IDC_PARAMS );
			m_sDest = GetItemTxt ( IDC_DEST_COMBO );
			Close ( iItem );
			break;
		case IDC_TREE_OBJ:
			{
				m_sParams = GetItemTxt ( IDC_PARAMS );
				Str_c sParams = m_sParams;
				while ( sParams.Begins ( L"\"" ) )
					sParams.Erase ( 0, 1 );

				while ( sParams.Ends ( L"\"" ) )
					sParams.Chop ( 1 );

				if ( ShowFileTreeDlg ( m_hWnd, sParams, true ) )
				{
					m_sParams = Str_c ( L"\"" ) + sParams + L"\"";
					SetEditTextFocused ( Item ( IDC_PARAMS ), m_sParams );
				}
			}
			break;
		case IDC_TREE_DEST:
			m_sDest = GetItemTxt ( IDC_DEST_COMBO );
			if ( ShowDirTreeDlg ( m_hWnd, m_sDest ) )
				SetComboTextFocused ( Item ( IDC_DEST_COMBO ), m_sDest );
			break;
		}
	}

	const Str_c & Name () const
	{
		return m_sName;
	}

	const Str_c & Params () const
	{
		return m_sParams;
	}

	const Str_c & Dest () const
	{
		return m_sDest;
	}

private:
	HWND	m_hCombo;
	Str_c	m_sName;
	Str_c	m_sParams;
	Str_c	m_sDest;

	void PopulateList ()
	{
		for ( int i = 0; i < g_tRecent.GetNumShortcuts (); ++i )
			SendMessage ( m_hCombo, CB_ADDSTRING, 0, (LPARAM) ( g_tRecent.GetShortcut ( i ).c_str () ) );

		SendMessage ( m_hCombo, CB_INSERTSTRING, 0, (LPARAM) ( m_sDest.c_str () ) );
		SetWindowText ( m_hCombo, m_sDest );
	}
};


bool ShowShortcutDialog ( Str_c & sName, Str_c & sParams, Str_c & sDest )
{
	ShortcutDlg_c Dlg ( sName, sParams, sDest );
	
	if ( Dlg.Run ( IDD_SHORTCUT, g_hMainWindow ) == IDOK )
	{
		sName = Dlg.Name ();
		sParams = Dlg.Params ();
		sDest = Dlg.Dest ();
		return true;
	}

	return false;
}

void SetupInitialShortcuts ()
{
	int dFolders [] =
	{
		 CSIDL_PERSONAL
		,CSIDL_PROGRAMS
		,CSIDL_STARTUP
		,CSIDL_WINDOWS
	};

	wchar_t szPath [MAX_PATH];

	int nFolders = sizeof ( dFolders ) / sizeof ( int );

	for ( int i = 0; i < nFolders; ++i )
		if ( SHGetSpecialFolderPath ( g_hMainWindow, szPath, dFolders [i], TRUE ) )
		{
			g_tRecent.AddShortcut ( szPath );
			if ( dFolders [i] == CSIDL_PROGRAMS )
			{
				Str_c sPath = szPath;
				int iPos = sPath.RFind ( L'\\' );
				if ( iPos != -1 && iPos != 0 )
				{
					sPath.Chop ( sPath.Length () - iPos );
					g_tRecent.AddShortcut ( sPath );
				}
			}
		}
}