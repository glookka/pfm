#ifndef _cbluetooth_
#define _cbluetooth_

#include "pfm/main.h"

typedef bool	(*fnWBT_CreateStack) ();
typedef void	(*fnWBT_DestroyStack) ();
typedef void	(*fnWBT_SetWindow) ( HWND );
typedef bool	(*fnWBT_StartInquiry) ();
typedef void	(*fnWBT_StopInquiry) ();
typedef bool	(*fnWBT_StartDiscovery) ( BYTE * );
typedef int		(*fnWBT_ReadDiscoveryRecords) ( BYTE * );
typedef bool	(*fnWBT_PrepareSCN) ( BYTE &, int, BYTE, bool );
typedef bool	(*fnWBT_CreateObexClient) ( HWND hWnd );
typedef void	(*fnWBT_DestroyObexClient) ();
typedef bool	(*fnWBT_ObextOpen) ( BYTE uSCN, BYTE * pAddress );
typedef bool	(*fnWBT_ObexPutFileName) ( const wchar_t *, unsigned int );
typedef bool	(*fnWBT_ObexPutBody) ( unsigned char *, unsigned int, bool );
typedef void	(*fnWBT_ObexAbort) ();
typedef void	(*fnWBT_Shutdown) ();

extern fnWBT_CreateStack			dll_WBT_CreateStack;
extern fnWBT_DestroyStack			dll_WBT_DestroyStack;
extern fnWBT_SetWindow				dll_WBT_SetWindow;
extern fnWBT_StartInquiry			dll_WBT_StartInquiry;
extern fnWBT_StopInquiry			dll_WBT_StopInquiry;
extern fnWBT_StartDiscovery			dll_WBT_StartDiscovery;
extern fnWBT_ReadDiscoveryRecords	dll_WBT_ReadDiscoveryRecords;
extern fnWBT_PrepareSCN				dll_WBT_PrepareSCN;
extern fnWBT_CreateObexClient		dll_WBT_CreateObexClient;
extern fnWBT_DestroyObexClient		dll_WBT_DestroyObexClient;
extern fnWBT_ObextOpen				dll_WBT_ObextOpen;
extern fnWBT_ObexPutFileName		dll_WBT_ObexPutFileName;
extern fnWBT_ObexPutBody			dll_WBT_ObexPutBody;
extern fnWBT_ObexAbort				dll_WBT_ObexAbort;
extern fnWBT_Shutdown				dll_WBT_Shutdown;

bool Init_WidcommLibrary ();
void Shutdown_WidcommLibrary ();

#endif