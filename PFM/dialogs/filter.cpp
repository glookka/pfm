#include "pch.h"
#include "filter.h"

#include "pfm/config.h"
#include "pfm/resources.h"
#include "pfm/gui.h"

#include "aygshell.h"
#include "Dlls/Resource/resource.h"

extern HWND g_hMainWindow;

//////////////////////////////////////////////////////////////////////////
class FilterDlg_c : public Dialog_Moving_c
{
public:
	FilterDlg_c ( bool bMark )
		: Dialog_Moving_c ( L"Toolbar", IDM_OK_CANCEL )
		, m_bMark ( bMark )
	{
	}

	virtual void OnInit ()
	{
		Dialog_Moving_c::OnInit ();

		Loc ( IDC_FOLDERS_CHECK, T_DLG_SELECT_DIRS );
		Loc ( IDC_FILTER_HEADER, m_bMark ? T_DLG_SELECT_FILES : T_DLG_CLEAR_FILES );
		Bold ( IDC_FILTER_HEADER );
		CheckBtn ( IDC_FOLDERS_CHECK, cfg->filter_include_dirs );

		HWND hList = Item ( IDC_FILTER_COMBO );
		SendMessage ( hList, CB_RESETCONTENT, 0, 0 );

		int nFilters = g_tRecent.GetNumFilters ();
		for ( int i = nFilters - 1; i >= 0 ; --i )
			SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM) (g_tRecent.GetFilter ( i ).c_str () ) );

		if ( !nFilters )
			SendMessage ( hList, CB_ADDSTRING, 0, (LPARAM) L"*" );

		SendMessage ( hList, CB_SETCURSEL, 0, 0 );
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Moving_c::OnCommand ( iItem, iNotify );

		switch ( iItem )
		{
		case IDOK:
			m_sFilter = GetItemTxt ( IDC_FILTER_COMBO );
			cfg->filter_include_dirs = IsChecked ( IDC_FOLDERS_CHECK );
		case IDCANCEL:
			Close ( iItem );
			break;
		}
	}

	const Str_c & Filter () const
	{
		return m_sFilter;
	}

private:
	bool	m_bMark;
	Str_c	m_sFilter;
};


bool ShowSetFilterDialog ( Str_c & sFilter, bool bMark )
{
	FilterDlg_c Dlg ( bMark );
	if ( Dlg.Run ( IDD_SET_FILTER, g_hMainWindow ) == IDOK )
	{
		sFilter = Dlg.Filter ();
		return true;
	}

	return false;
}