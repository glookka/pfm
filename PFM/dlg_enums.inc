#ifndef _dlg_enums_
#define _dlg_enums_


enum
{
	 DLG_ERROR_OK
	,DLG_ERROR_YES_NO
};

enum ErrorDlgType_e
{
	 EB_C
	,EB_R_C
	,EB_S_SA_C
	,EB_R_S_SA_C
	,EB_O_OA_S_SA_C
};

enum ErrResponse_e
{
	 ER_CANCEL
	,ER_RETRY
	,ER_SKIP
	,ER_OVER
};

enum ErrorCustom_e
{
	 EC_COPY_ONTO_ITSELF
	,EC_MOVE_ONTO_ITSELF
	,EC_NOT_ENOUGH_SPACE
	,EC_DEST_EXISTS
	,EC_DEST_READONLY
	,EC_UNKNOWN
	,EC_LAST
};

enum OperationResult_e
{
	 RES_OK
	,RES_ERROR
	,RES_SKIP
	,RES_CANCEL
	,RES_FINISHED
};

enum
{
	 TOOLBAR_OK
	,TOOLBAR_CANCEL
	,TOOLBAR_OK_CANCEL
};

#endif