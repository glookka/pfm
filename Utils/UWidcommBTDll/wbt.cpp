#include "pch.h"

#include "wbt.h"
#include "widcommbt.h"

static WidcommStack_c * g_pBTStack = NULL;
static WidcommObexClient_c * g_pBTObex = NULL;

static CObexHeaders * g_pHeadersOpen = NULL;
static CObexHeaders * g_pHeadersHead = NULL;
static CObexHeaders * g_pHeadersBody = NULL;
static CObexHeaders * g_pHeadersAbort = NULL;

const int MAX_SERVICES = 8;
int g_nRecords;
CSdpDiscoveryRec g_dRecords [MAX_SERVICES];

static void CreateHeader ( CObexHeaders * & pHeader )
{
	if ( pHeader )
		delete pHeader;

	pHeader = new CObexHeaders;
}


bool WBT_CreateStack ()
{
	if ( g_pBTStack )
		return false;

	g_pBTStack = new WidcommStack_c ();

	return !!g_pBTStack;
}

void WBT_SetWindow ( HWND hWnd )
{
	if ( g_pBTStack )
		g_pBTStack->SetWindow ( hWnd );
}

void WBT_DestroyStack ()
{
	delete g_pBTStack;
	g_pBTStack = NULL;
}

bool WBT_StartInquiry ()
{
	if ( ! g_pBTStack )
		return false;

	return !! g_pBTStack->StartInquiry ();
}

void WBT_StopInquiry ()
{
	if ( ! g_pBTStack )
		return;

	g_pBTStack->StopInquiry ();
}

bool WBT_StartDiscovery ( BYTE * pAddress )
{
	if ( ! g_pBTStack )
		return false;

	g_nRecords = 0;
	GUID guid = CBtIf::guid_SERVCLASS_OBEX_FILE_TRANSFER;
	return !! g_pBTStack->StartDiscovery ( pAddress, &guid );
}

int	WBT_ReadDiscoveryRecords ( BYTE * pAddress )
{
	if ( ! g_pBTStack )
		return 0;

	GUID guid = CBtIf::guid_SERVCLASS_OBEX_FILE_TRANSFER;
	g_nRecords = g_pBTStack->ReadDiscoveryRecords ( pAddress, MAX_SERVICES, g_dRecords, &guid );
	return g_nRecords;
}

bool WBT_PrepareSCN ( BYTE & uSCN, int iService, BYTE uSecurity, bool bServer )
{
	if ( iService < 0 || iService >= g_nRecords )
		return false;

	if ( ! g_dRecords [iService].FindRFCommScn ( &uSCN ) )
		return false;

	GUID guid = CBtIf::guid_SERVCLASS_OBEX_FILE_TRANSFER;

	CRfCommIf RfCommIf;	

	if ( ! RfCommIf.AssignScnValue ( &guid, uSCN ) )
		return false;

	if ( ! RfCommIf.SetSecurityLevel ( (wchar_t * )( g_dRecords [iService].m_service_name ), uSecurity, bServer ? TRUE : FALSE ) )
		return false;

	return true;
}

bool WBT_CreateObexClient ( HWND hWnd )
{
	if ( g_pBTObex )
		return false;

	g_pBTObex = new WidcommObexClient_c;
	if ( g_pBTObex )
		g_pBTObex->SetWindow ( hWnd );

	return !!g_pBTObex;
}

void WBT_DestroyObexClient ()
{
	delete g_pBTObex;
	g_pBTObex = NULL;
}

bool WBT_ObextOpen ( BYTE uSCN, BYTE * pAddress )
{
	if ( ! g_pBTObex )
		return false;

	CreateHeader ( g_pHeadersOpen );

	GUID FileTransferGUID = { 0xC47BECF9, 0x3C95, 0xD211, { 0x98, 0x4E, 0x52, 0x54, 0x00, 0xDC, 0x9E, 0x09 } };
	g_pHeadersOpen->AddTarget ( (UINT8 *) &FileTransferGUID, sizeof ( FileTransferGUID ) );
				
	return g_pBTObex->Open ( uSCN, pAddress, g_pHeadersOpen ) == OBEX_SUCCESS;
}

bool WBT_ObexPutFileName ( const wchar_t * szName, unsigned int uLength )
{
	if ( ! g_pBTObex )
		return false;

	CreateHeader ( g_pHeadersHead );
	if ( ! g_pHeadersHead->SetName ( (UINT16 *) szName ) )
		return false;

	g_pHeadersHead->SetLength ( uLength );
	g_pHeadersHead->SetBody ( NULL, 0, FALSE );

	return g_pBTObex->Put ( g_pHeadersHead, FALSE ) == OBEX_SUCCESS;	
}

bool WBT_ObexPutBody ( unsigned char * pBuffer, unsigned int uBytes, bool bEOF )
{
	if ( ! g_pBTObex )
		return false;

	CreateHeader ( g_pHeadersBody );

	if ( ! g_pHeadersBody->SetBody ( pBuffer, uBytes, bEOF ? TRUE : FALSE ) )
		return false;

	return g_pBTObex->Put ( g_pHeadersBody, bEOF ? TRUE : FALSE ) == OBEX_SUCCESS;
}

void WBT_ObexAbort ()
{
	if ( ! g_pBTObex )
		return;

	g_pBTObex->Abort ( g_pHeadersAbort );
}

void WBT_Shutdown ()
{
	delete g_pHeadersOpen;
	delete g_pHeadersHead;
	delete g_pHeadersBody;
	delete g_pHeadersAbort;

	g_pHeadersOpen = g_pHeadersHead = g_pHeadersBody = g_pHeadersAbort = NULL;
	
	WIDCOMMSDK_ShutDown ();
}