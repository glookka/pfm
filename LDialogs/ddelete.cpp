#include "pch.h"

#include "LDialogs/ddelete.h"
#include "LCore/clog.h"
#include "LDialogs/dcommon.h"
#include "LSettings/slocal.h"
#include "LUI/udialogs.h"

#include "Resources/resource.h"

extern HWND g_hMainWindow;

class DeleteDlg_c : public Dialog_Moving_c
{
public:
	DeleteDlg_c ( FileList_t & List )
		: Dialog_Moving_c	( L"Delete", IDM_OK_CANCEL )
		, m_pList			( & List )
	{
	}

	virtual void OnInit ()
	{
		Dialog_Moving_c::OnInit ();

		Loc ( IDOK,		T_TBAR_OK );
		Loc ( IDCANCEL, T_TBAR_CANCEL );

		const int nFiles = m_pList->m_dFiles.Length ();
		Assert ( nFiles > 0 );

		Str_c sText, sFile;
		if ( nFiles > 1 )
			sText = NewString ( Txt ( T_DLG_DELETE_N_FILES ), nFiles );
		else
		{
			const FileInfo_t * pInfo = m_pList->m_dFiles [0];
			Assert ( pInfo );

			sText = Txt ( ( pInfo->m_tData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ? T_DLG_DELETE_FOLDER : T_DLG_DELETE_FILE );
			sFile = m_pList->m_sRootDir + pInfo->m_tData.cFileName;
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
	FileList_t * m_pList;
};


int ShowDeleteDialog ( FileList_t & List )
{
	DeleteDlg_c Dlg ( List );
	return Dlg.Run ( IDD_DELETE_FILES, g_hMainWindow );
}