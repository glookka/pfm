#include "pch.h"

#include "LFile/fmessages.h"
#include "pfm/system.h"
#include "pfm/config.h"

Str_c GetExecuteError ( int iMsg, bool bShowAlways )
{
	switch ( iMsg )
	{
	case SE_ERR_FNF:			return Txt ( T_EXEC_FILE_NOT_FOUND );
	case SE_ERR_PNF: 			return Txt ( T_EXEC_PATH_NOT_FOUND );
	case SE_ERR_ACCESSDENIED:	return Txt ( T_EXEC_ACCESS_DENIED );
	case SE_ERR_OOM:			return Txt ( T_EXEC_OUT_OF_MEMORY );
	case SE_ERR_DLLNOTFOUND:	return Txt ( T_EXEC_NO_DLL );
	case SE_ERR_SHARE:			return Txt ( T_EXEC_CANT_SHARE );
	case SE_ERR_DDETIMEOUT:		return Txt ( T_EXEC_DDE_TIMEOUT );
	case SE_ERR_DDEFAIL:		return Txt ( T_EXEC_DDE_FAILED );
	case SE_ERR_DDEBUSY:		return Txt ( T_EXEC_DDE_BUSY );
	}

	if ( bShowAlways )
		return GetWinErrorText ( iMsg );

	return L"";
}