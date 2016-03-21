#include "pch.h"

#include "cbluetooth.h"
#include "LCore/cos.h"

HMODULE g_hWidcommLib = NULL;

fnWBT_CreateStack			dll_WBT_CreateStack			= NULL;
fnWBT_DestroyStack			dll_WBT_DestroyStack		= NULL;
fnWBT_SetWindow				dll_WBT_SetWindow			= NULL;
fnWBT_StartInquiry			dll_WBT_StartInquiry		= NULL;
fnWBT_StopInquiry			dll_WBT_StopInquiry			= NULL;
fnWBT_StartDiscovery		dll_WBT_StartDiscovery		= NULL;
fnWBT_ReadDiscoveryRecords	dll_WBT_ReadDiscoveryRecords= NULL;
fnWBT_PrepareSCN			dll_WBT_PrepareSCN			= NULL;
fnWBT_CreateObexClient		dll_WBT_CreateObexClient	= NULL;
fnWBT_DestroyObexClient		dll_WBT_DestroyObexClient	= NULL;
fnWBT_ObextOpen				dll_WBT_ObextOpen			= NULL;
fnWBT_ObexPutFileName		dll_WBT_ObexPutFileName		= NULL;
fnWBT_ObexPutBody			dll_WBT_ObexPutBody			= NULL;
fnWBT_ObexAbort				dll_WBT_ObexAbort			= NULL;
fnWBT_Shutdown				dll_WBT_Shutdown			= NULL;


bool Init_WidcommLibrary ()
{
	if ( ! g_hWidcommLib )
	{
		bool b2003 = GetOSVersion () == WINCE_2003 || GetOSVersion () == WINCE_2003SE;
		g_hWidcommLib = LoadLibrary ( b2003 ? L"WidcommBT.dll" : L"WidcommBT5.dll" );
		if ( ! g_hWidcommLib )
			return false;
	}

	(FARPROC &)dll_WBT_CreateStack			= GetProcAddress ( g_hWidcommLib, L"WBT_CreateStack" );
	(FARPROC &)dll_WBT_DestroyStack			= GetProcAddress ( g_hWidcommLib, L"WBT_DestroyStack" );
	(FARPROC &)dll_WBT_SetWindow			= GetProcAddress ( g_hWidcommLib, L"WBT_SetWindow" );
	(FARPROC &)dll_WBT_StartInquiry			= GetProcAddress ( g_hWidcommLib, L"WBT_StartInquiry" );
	(FARPROC &)dll_WBT_StopInquiry			= GetProcAddress ( g_hWidcommLib, L"WBT_StopInquiry" );
	(FARPROC &)dll_WBT_StartDiscovery		= GetProcAddress ( g_hWidcommLib, L"WBT_StartDiscovery" );
	(FARPROC &)dll_WBT_ReadDiscoveryRecords	= GetProcAddress ( g_hWidcommLib, L"WBT_ReadDiscoveryRecords" );
	(FARPROC &)dll_WBT_PrepareSCN			= GetProcAddress ( g_hWidcommLib, L"WBT_PrepareSCN" );
	(FARPROC &)dll_WBT_CreateObexClient		= GetProcAddress ( g_hWidcommLib, L"WBT_CreateObexClient" );
	(FARPROC &)dll_WBT_DestroyObexClient	= GetProcAddress ( g_hWidcommLib, L"WBT_DestroyObexClient" );
	(FARPROC &)dll_WBT_ObextOpen			= GetProcAddress ( g_hWidcommLib, L"WBT_ObextOpen" );
	(FARPROC &)dll_WBT_ObexPutFileName		= GetProcAddress ( g_hWidcommLib, L"WBT_ObexPutFileName" );
	(FARPROC &)dll_WBT_ObexPutBody			= GetProcAddress ( g_hWidcommLib, L"WBT_ObexPutBody" );
	(FARPROC &)dll_WBT_ObexAbort			= GetProcAddress ( g_hWidcommLib, L"WBT_ObexAbort" );
	(FARPROC &)dll_WBT_Shutdown				= GetProcAddress ( g_hWidcommLib, L"WBT_Shutdown" );

	if ( ! dll_WBT_CreateStack || ! dll_WBT_DestroyStack || ! dll_WBT_SetWindow || ! dll_WBT_StartInquiry || ! dll_WBT_StopInquiry
		|| ! dll_WBT_StartDiscovery	|| ! dll_WBT_ReadDiscoveryRecords || ! dll_WBT_PrepareSCN || ! dll_WBT_CreateObexClient
		|| ! dll_WBT_DestroyObexClient || ! dll_WBT_ObextOpen || ! dll_WBT_ObexPutFileName || ! dll_WBT_ObexPutBody
		|| ! dll_WBT_ObexAbort || ! dll_WBT_Shutdown )
	{
		return false;
	}

	return true;	
}

void Shutdown_WidcommLibrary ()
{
	if ( g_hWidcommLib )
	{
		if ( dll_WBT_Shutdown )
			dll_WBT_Shutdown ();

		FreeLibrary ( g_hWidcommLib );
		g_hWidcommLib = NULL;
	}
}
