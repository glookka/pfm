#include "pch.h"

#include "LFile/fclipboard.h"
#include "LDialogs/dcopy_progress.h"
#include "FileManager/filemanager.h"

namespace clipboard
{
	FileList_t				g_Files;
	Array_T <FileInfo_t>	g_Infos;
	bool					g_bMove;

	void Copy ( Panel_c * pPanel )
	{
		Assert ( pPanel );
		g_Files.Reset ();
		g_Infos.Reset ();

		if ( pPanel->GenerateFileList ( g_Files ) )
		{
			for ( int i = 0; i < g_Files.m_dFiles.Length (); ++i )
				g_Infos.Add ( *g_Files.m_dFiles [i] );

			for ( int i = 0; i < g_Files.m_dFiles.Length (); ++i )
				g_Files.m_dFiles [i] = &(g_Infos [i]);
		}
		
		g_bMove = false;
	}

	void Cut ( Panel_c * pPanel )
	{
		Copy ( pPanel );
		g_bMove = true;
	}

	void Paste ( const wchar_t * szDestDir )
	{
		if ( IsEmpty () )
			return;

		if ( g_bMove )
			DlgMoveFiles ( NULL, szDestDir, g_Files );
		else
			DlgCopyFiles ( NULL, szDestDir, g_Files );

		FMGetPanel1 ()->SoftRefresh ();
		FMGetPanel2 ()->SoftRefresh ();
	}

	bool IsEmpty ()
	{
		return g_Infos.Empty ();
	}

	bool IsMoveMode ()
	{
		return g_bMove;
	}
}