#include "pch.h"
#include "delete.h"

#include "pfm/config.h"
#include "pfm/gui.h"

#include "LFile/fdelete.h"

#include "Dlls/Resource/resource.h"


extern HWND g_hMainWindow;

class DeleteDlg_c : public Dialog_Moving_c
{
public:
	DeleteDlg_c ( const SelectedFileList_t & List )
		: Dialog_Moving_c	( L"Delete", IDM_OK_CANCEL )
		, m_pList			( &List )
	{
	}

	virtual void OnInit ()
	{
		Dialog_Moving_c::OnInit ();

		const int nFiles = m_pList->m_dFiles.Length ();
		Assert ( nFiles > 0 );

		Str_c sText, sFile;
		if ( nFiles > 1 )
			sText = NewString ( Txt ( T_DLG_DELETE_N_FILES ), nFiles );
		else
		{
			const PanelItem_t * pInfo = m_pList->m_dFiles [0];
			Assert ( pInfo );

			sText = Txt ( ( pInfo->m_FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ? T_DLG_DELETE_FOLDER : T_DLG_DELETE_FILE );
			sFile = m_pList->m_sRootDir + pInfo->m_FindData.cFileName;
		}

		Bold ( IDC_TITLE );
		ItemTxt ( IDC_TITLE, sText );
		AlignFileName ( Item ( IDC_DELETE_FILE ), sFile );
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Moving_c::OnCommand ( iItem, iNotify );

		Close ( iItem );
	}

private:
	const SelectedFileList_t * m_pList;
};

//////////////////////////////////////////////////////////////////////////

extern HWND g_hMainWindow;
const DWORD MY_UNIQUE_TIMER_DELETE = 0xABCDEF01;

class DeleteProgressDlg_c : public Dialog_Moving_c
{
public:
	DeleteProgressDlg_c ( const SelectedFileList_t & List )
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

				if ( !m_Deleter.DeleteNext () )
				{
					Close ( IDCANCEL );
					break;
				}
			}
			else
			{
				UpdateNames ();
				RedrawWindow ( m_hWnd, NULL, NULL, RDW_ERASENOW  | RDW_UPDATENOW | RDW_INVALIDATE );
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
	SelectedFileList_t *	m_pList;
	FileDelete_c 			m_Deleter;
	double					m_fLastUpdateTime;

	void UpdateNames ()
	{
		AlignFileName ( Item ( IDC_FILENAME ), m_Deleter.GetFileToDelete () );
		ItemTxt ( IDC_NUM_FILES, NewString ( Txt ( T_DLG_DELETED_FILES ), m_Deleter.GetNumDeletedFiles () ) );
	}
};

///////////////////////////////////////////////////////////////////////////////////////////
bool DeleteRequestDlg ( const SelectedFileList_t & List )
{
	DeleteDlg_c Dlg ( List );
	return Dlg.Run ( IDD_DELETE_FILES, g_hMainWindow ) == IDOK;
}

void DeleteProgressDlg ( const SelectedFileList_t & List )
{
	DeleteProgressDlg_c Dlg ( List );
	Dlg.Run ( IDD_DELETE_PROGRESS, g_hMainWindow );
}