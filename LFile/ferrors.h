#ifndef _ferrors_
#define _ferrors_

#include "LCore/cmain.h"
#include "LCore/cstr.h"
#include "LCore/carray.h"

// operation results
enum OperationResult_e
{
	 RES_OK
	,RES_ERROR
	,RES_CANCEL
	,RES_FINISHED
};


// custom error types
enum ErrorCustom_e
{
	 EC_COPY_ONTO_ITSELF
	,EC_MOVE_ONTO_ITSELF
	,EC_NOT_ENOUGH_SPACE
	,EC_DEST_EXISTS
	,EC_DEST_READONLY
	,EC_UNKNOWN
};

enum OperationMode_e
{
	 OM_COPY
	,OM_MOVE
	,OM_RENAME
	,OM_MKDIR
	,OM_DELETE
	,OM_PROPS
	,OM_CRYPT
	,OM_SHORTCUT
	,OM_SEND
	,OM_TOTAL
};


#undef TRY_FUNC
#define TRY_FUNC(var,func,val,file1,file2) \
{\
	int iErrAction = IDC_SKIP;\
	do\
	{\
		var = func;\
		if ( var == val )\
			iErrAction = g_tErrorList.ShowErrorDialog ( m_hWnd, GetLastError (), true, file1, file2 );\
	}\
	while ( var == val && iErrAction == IDC_RETRY );\
	if ( var == val )\
		return iErrAction == IDCANCEL ? RES_CANCEL : RES_ERROR;\
}\


class ErrorDialog_c;

//
// error registry
//
class OperationErrorList_c
{
public:
							OperationErrorList_c ();

	void 					SetOperationMode ( OperationMode_e eMode );
	void					Reset ( OperationMode_e eMode );
	OperationMode_e			GetMode () const;

	void					Init ();
	void					Shutdown ();

	int						ShowErrorDialog ( HWND hWnd, int iError, bool bWinError, const Str_c & sFile1 );
	int						ShowErrorDialog ( HWND hWnd, int iError, bool bWinError, const Str_c & sFile1, const Str_c & sFile2 );
	bool					IsInDialog () const;

private:
	Array_T < ErrorDialog_c * >	m_dErrors;
	OperationMode_e			m_eOperationMode;

	void					RegisterWinError ( int iErrorCode );
};

extern OperationErrorList_c g_tErrorList;

#endif