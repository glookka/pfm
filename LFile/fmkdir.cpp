#include "pch.h"

#include "LFile/fmkdir.h"
#include "LCore/clog.h"
#include "LCore/cfile.h"
#include "LFile/ferrors.h"
#include "LFile/fmisc.h"

#include "Resources/resource.h"

extern HWND g_hMainWindow;

static OperationResult_e TryToMakeDir ( const Str_c & sFileName )
{
	BOOL bResult = FALSE;
	HWND m_hWnd = g_hMainWindow;
	TRY_FUNC ( bResult, CreateDirectory ( sFileName, NULL ), FALSE, sFileName, L"" );
	return RES_OK;
}

bool MakeDirectory ( const Str_c & sDir )
{
	g_tErrorList.Reset ( OM_MKDIR );

	int iOffset = 1;
	int iPos = 0;
	OperationResult_e eRes = RES_OK;
	Str_c sSourceDir = sDir;
	Str_c sDirToCreate;

	RemoveSlash ( sSourceDir );

	while ( ( iPos = sSourceDir.Find ( L'\\', iOffset ) ) != -1 )
	{
		sDirToCreate = sSourceDir.SubStr ( 0, iPos );

		DWORD dwAttributes = GetFileAttributes ( sDirToCreate );
		if ( dwAttributes == 0xFFFFFFFF )
		{
			eRes = TryToMakeDir ( sDirToCreate );
			if ( eRes != RES_OK  )
				return false;
		}
		iOffset = iPos + 1;
	}

	if ( ! sSourceDir.Empty () && GetFileAttributes ( sSourceDir ) == 0xFFFFFFFF )
		if ( TryToMakeDir ( sSourceDir ) != RES_OK )
			return false;

	return true;
}

bool MakeDirectory ( const Str_c & sDir, const Str_c & sRootDir )
{
	Str_c sDirToCreate = sDir;
	PrepareDestDir ( sDirToCreate, sRootDir );
	RemoveSlash ( sDirToCreate );
	return MakeDirectory ( sDirToCreate );
}