#include "pch.h"

#include "LDialogs/ddelete_progress.h"
#include "LCore/clog.h"
#include "LCore/ctimer.h"
#include "LSettings/sconfig.h"
#include "LFile/fdelete.h"
#include "LFile/fmisc.h"
#include "LPanel/presources.h"
#include "LUI/udialogs.h"
#include "LDialogs/dcommon.h"
#include "LSettings/slocal.h"

#include "Resources/resource.h"
#include "aygshell.h"

extern HWND g_hMainWindow;
const DWORD MY_UNIQUE_TIMER_DELETE = 0xABCDEF01;

class DeleteProgressDlg_c : public Dialog_Moving_c
{
public:
	DeleteProgressDlg_c ( FileList_t & List )
		: Dialog_Moving_c	( L"Delete", IDM_CANCEL )
		, m_Deleter			( List )
		, m_fLastUpdateTime	( 0.0 )
	{
	}

	virtual void OnInit ()
	{
		Dialog_Moving_c::OnInit ();

		Bold ( IDC_TITLE );

		Loc ( IDC_TITLE, T_DLG_DELETE_PROGRESS );
		Loc ( IDCANCEL, T_TBAR_CANCEL );

		SetTimer ( m_hWnd, MY_UNIQUE_TIMER_DELETE, 0, NULL );
		ItemTxt ( IDC_NUM_FILES, NewString ( Txt ( T_DLG_DELETED_FILES ), 0 ) );

		m_Deleter.SetWindow ( m_hWnd );
	}

	virtual void OnTimer ( DWORD uTimer )
	{
		Dialog_Moving_c::OnTimer ( uTimer );

		if ( uTimer != MY_UNIQUE_TIMER_DELETE || m_Deleter.IsInDialog () )
			return;

		const double UPDATE_INTERVAL = 0.1;

		for ( int i = 0; i < FileDelete_c::FILES_IN_PASS; ++i )
			if ( m_Deleter.PrepareFileName () )
			{
				if ( m_fLastUpdateTime == 0.0 )
				{
					m_fLastUpdateTime = g_Timer.GetTimeSec ();
					UpdateNames ();
				}
				else
				{
					double fCurTime = g_Timer.GetTimeSec ();
					if ( fCurTime - m_fLastUpdateTime >= UPDATE_INTERVAL )
					{
						m_fLastUpdateTime = fCurTime;
						UpdateNames ();
					}
				}
				
				if ( ! m_Deleter.DeleteNext () )
				{
					Close ( IDCANCEL );
					break;
				}
			}
			else
			{
				UpdateNames ();
				Close ( IDOK );
				break;
			}
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Moving_c::OnCommand ( iItem, iNotify );

		if ( iItem == IDCANCEL )
			Close ( iItem );
	}

private:
	FileList_t *	m_pList;
	FileDelete_c 	m_Deleter;
	double			m_fLastUpdateTime;

	void UpdateNames ()
	{
		AlignFileName ( Item ( IDC_FILENAME ), m_Deleter.GetFileToDelete () );
		ItemTxt ( IDC_NUM_FILES, NewString ( Txt ( T_DLG_DELETED_FILES ), m_Deleter.GetNumDeletedFiles () ) );
	}
};

///////////////////////////////////////////////////////////////////////////////////////////
// wrapper funcs
void DeleteFiles ( FileList_t & List )
{
	DeleteProgressDlg_c Dlg ( List );
	Dlg.Run ( IDD_DELETE_PROGRESS, g_hMainWindow );
}