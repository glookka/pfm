#ifndef _dialogs_errors_
#define _dialogs_errors_

#include "pfm/main.h"
#include "pfm/std.h"
#include "pfm/dlg_enums.inc"


int ShowErrorDialog ( HWND hParent, bool bWarning, const wchar_t * szText, int iDlgType );
int ShowMessageDialog ( HWND hParent, const wchar_t * szTitle, const wchar_t * szText, int iDlgType );

HANDLE		ErrCreateOperation		( const wchar_t * szOpName, int iWinErrorDlg, bool bTwoFile );
void		ErrDestroyOperation		( HANDLE hOperation );
void		ErrAddWinErrorHandler	( HANDLE hOperation, int iErrorCode, int iDlgType, const wchar_t * szErrorText = NULL );
void		ErrAddCustomErrorHandler( HANDLE hOperation, bool bTwoFile, int iErrorCode, int iDlgType, const wchar_t * szErrorText );
void		ErrSetOperation			( HANDLE hOperation );
ErrResponse_e ErrShowErrorDlg		( HWND hParent, int iError, bool bWinError, const wchar_t * szFile1, const wchar_t * szFile2 );
bool		ErrIsInDialog			();

#undef TRY_FUNC
#define TRY_FUNC(var,func,val,file1,file2) \
{\
	ErrResponse_e eErrResponse = ER_SKIP;\
	do\
	{\
		var = func;\
		if ( var == val )\
			eErrResponse = ErrShowErrorDlg ( m_hWnd, GetLastError (), true, file1, file2 );\
	}\
	while ( var == val && eErrResponse == ER_RETRY );\
	if ( var == val )\
		switch ( eErrResponse )\
		{\
		case ER_CANCEL:	return RES_CANCEL;\
		case ER_SKIP:	return RES_SKIP;\
		default:		return RES_ERROR;\
		}\
}\

#undef COMMON_ERROR
#define COMMON_ERROR(code,source,dest) \
{\
	ErrResponse_e eErrResponse = ErrShowErrorDlg ( m_hWnd, code, false, source, dest );\
	switch ( eErrResponse )\
	{\
	case ER_SKIP:	return RES_SKIP;\
	case ER_CANCEL:	return RES_CANCEL;\
	case ER_OVER:	break;\
	default:		Assert ( "no such response"); return RES_ERROR;\
	}\
}



#endif