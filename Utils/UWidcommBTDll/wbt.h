#ifndef _wbt_
#define _wbt_

const UINT WIDCOMM_WM_DEVICEFOUNDMESSAGE =		RegisterWindowMessage(_T("WIDCOMM_WM_DEVICEFOUNDMESSAGE"));
const UINT WIDCOMM_WM_INQUIRYCOMPLETEMESSAGE =	RegisterWindowMessage(_T("WIDCOMM_WM_INQUIRYCOMPLETEMESSAGE"));
const UINT WIDCOMM_WM_DISCOVERYCOMPLETEMESSAGE =RegisterWindowMessage(_T("WIDCOMM_WM_DISCOVERYCOMPLETEMESSAGE"));
const UINT WIDCOMM_WM_ONOPENMESSAGE =			RegisterWindowMessage(_T("WIDCOMM_WM_ONOPENMESSAGE"));
const UINT WIDCOMM_WM_ONCLOSEMESSAGE =			RegisterWindowMessage(_T("WIDCOMM_WM_ONCLOSEMESSAGE"));
const UINT WIDCOMM_WM_ONABORTMESSAGE =			RegisterWindowMessage(_T("WIDCOMM_WM_ONABORTMESSAGE"));
const UINT WIDCOMM_WM_ONPUTMESSAGE =			RegisterWindowMessage(_T("WIDCOMM_WM_ONPUTMESSAGE"));
const UINT WIDCOMM_WM_ONGETMESSAGE =			RegisterWindowMessage(_T("WIDCOMM_WM_ONGETMESSAGE"));
const UINT WIDCOMM_WM_ONSETPATHMESSAGE =		RegisterWindowMessage(_T("WIDCOMM_WM_ONSETPATHMESSAGE"));

bool	WBT_CreateStack ();
void	WBT_SetWindow ( HWND hWnd );
void	WBT_DestroyStack ();
bool	WBT_StartInquiry ();
void	WBT_StopInquiry ();
bool	WBT_StartDiscovery ( BYTE * pAddress );
int		WBT_ReadDiscoveryRecords ( BYTE * pAddress );
bool	WBT_PrepareSCN ( BYTE & uSCN, int iService, BYTE uSecurity, bool bServer );

bool	WBT_CreateObexClient ( HWND hWnd );
void	WBT_DestroyObexClient ();

bool	WBT_ObextOpen ( BYTE uSCN, BYTE * pAddress );
bool	WBT_ObexPutFileName ( const wchar_t * szName, unsigned int uLength );
bool	WBT_ObexPutBody ( unsigned char * pBuffer, unsigned int uBytes, bool bEOF );
void	WBT_ObexAbort ();

void	WBT_Shutdown ();

#endif