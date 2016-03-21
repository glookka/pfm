#include "pch.h"

#include "LComm/cmsbt.h"
#include "LCore/clog.h"
#include "LCore/ctimer.h"
#include "LComm/cirda.h"
#include "LComm/cbluetooth.h"

#include "initguid.h"
#include "bt_sdp.h"
#include "bthapi.h"

static const double SHUTDOWN_TIME = 2.0;


MsBT_c::MsBT_c ()
	: m_bInquiry	( false )
	, m_bExitFlag	( false )
	, m_hThread		( NULL )
	, m_uThreadId	( 0xFFFFFFFF )
{
}

MsBT_c::~MsBT_c ()
{
	if ( m_bInquiry )
	{
		m_bExitFlag = true;
		BOOL bResult = FALSE;
		DWORD dwExitCode = 0;
		do
		{
			Sleep ( 50 );
			bResult = GetExitCodeThread ( m_hThread, & dwExitCode ); 
		}
		while ( ! bResult || dwExitCode == STILL_ACTIVE );
	}

	CloseHandle ( m_hThread );
}

bool MsBT_c::StartInquiry ()
{
	Assert ( m_uThreadId == 0xFFFFFFFF );
	m_hThread = CreateThread ( NULL, 32768, MsBT_c::EnumThread, this, STACK_SIZE_PARAM_IS_A_RESERVATION, &m_uThreadId );
	if ( ! m_hThread )
		return false;

	m_bInquiry = true;

	return true;
}

void MsBT_c::StopInquiry ()
{
	m_bExitFlag = true;
}

void MsBT_c::OnDeviceResponded ( const wchar_t * szDeviceName, SOCKADDR_BTH * pAddress )
{
	BLUETOOTH_DEVICE_INFO tInfo;
	if ( szDeviceName )
		wcscpy ( tInfo.szName, szDeviceName );
	else
	{
		BYTE * pBAddr = (BYTE * ) & ( pAddress->btAddr );
		wsprintf ( tInfo.szName, L"%02X.%02X.%02X.%02X.%02X.%02X", pBAddr [0], pBAddr [1], pBAddr [2], pBAddr [3], pBAddr [4], pBAddr [5] );
	}

	tInfo.Address.ullLong = pAddress->btAddr;

	if ( m_hWnd )
		SendMessage( m_hWnd, MSBT_WM_DEVICEFOUNDMESSAGE, 0, (LPARAM) &tInfo );
}

void MsBT_c::OnInquiryComplete ( bool bSuccess )
{
	m_bInquiry = false;

	if ( m_hWnd )
		SendMessage ( m_hWnd, MSBT_WM_INQUIRYCOMPLETEMESSAGE, 0, 0 );
}


DWORD MsBT_c::EnumThread ( void * pParam )
{
	MsBT_c * pBT = (MsBT_c *) pParam;
	Assert ( pBT );

	WSAQUERYSET	tQuery;	
	memset ( &tQuery, 0, sizeof ( tQuery ) );
	tQuery.dwSize      = sizeof ( tQuery );
	tQuery.dwNameSpace = NS_BTH;

	HANDLE hLookup = 0;
	
	if ( WSALookupServiceBegin ( &tQuery, LUP_CONTAINERS, &hLookup ) != 0 )
	{
		pBT->OnInquiryComplete ( false );
		return 0;
	}

	int iResult = 0;

	// fixme: why is it that big?
	union {
		unsigned char dBuffer [4096];	// returned struct can be quite large 
		SOCKADDR_BTH		__unused;	// properly align buffer to BT_ADDR requirements
	};

	WSAQUERYSET * pResults = (WSAQUERYSET *) dBuffer;

	while ( true )
	{
		memset ( pResults, 0, sizeof (WSAQUERYSET) );
		pResults->dwSize      = sizeof ( WSAQUERYSET );
		pResults->dwNameSpace = NS_BTH;
		DWORD dwSize  = sizeof ( dBuffer );
		
		iResult = WSALookupServiceNext ( hLookup, LUP_RETURN_NAME | LUP_RETURN_ADDR, &dwSize, pResults );

		if ( iResult != 0 )
		{
			iResult = WSAGetLastError();
			break;
		}

		SOCKADDR_BTH * pAddress = (SOCKADDR_BTH *)pResults->lpcsaBuffer->RemoteAddr.lpSockaddr;

		pBT->OnDeviceResponded ( pResults->lpszServiceInstanceName, pAddress );

		Sleep ( 100 );

		if ( pBT->m_bExitFlag )
			break;
	}

	WSALookupServiceEnd ( hLookup );

	pBT->OnInquiryComplete ( iResult == WSA_E_NO_MORE );
	
	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////
MsBTObexClient_c::MsBTObexClient_c ( const BT_ADDR & tAddress )
	: m_bExitFlag	( false )
	, m_hEvent		( NULL )
	, m_hThread		( NULL )
	, m_uThreadId	( 0xFFFFFFFF )
	, m_uSize		( 0 )
	, m_uMaxPacket	( 0 )
	, m_uMaxData	( 0 )
	, m_uConnectionId( 0 )
	, m_tAddress	( tAddress )
{
	memset ( m_dBuffer, 0, sizeof ( m_dBuffer ) );
}


MsBTObexClient_c::~MsBTObexClient_c ()
{
	Close ();

	double fStartTime = g_Timer.GetTimeSec ();

	BOOL bResult = FALSE;
	DWORD dwExitCode = 0;
	do
	{
		Sleep ( 50 );
		bResult = GetExitCodeThread ( m_hThread, & dwExitCode ); 

		if ( m_tSocket != INVALID_SOCKET && g_Timer.GetTimeSec () - fStartTime >= SHUTDOWN_TIME )
		{
			closesocket ( m_tSocket );
			m_tSocket = INVALID_SOCKET;
		}
	}
	while ( ! bResult || dwExitCode == STILL_ACTIVE );

	CloseHandle ( m_hThread );
	CloseHandle ( m_hEvent );

	if ( m_tSocket != INVALID_SOCKET )
		closesocket ( m_tSocket );
}


bool MsBTObexClient_c::Open ()
{
	m_tSocket = socket ( AF_BT, SOCK_STREAM, BTHPROTO_RFCOMM );

	if ( m_tSocket == INVALID_SOCKET )
		return false;

	unsigned int uOptVal = 0; 
	setsockopt ( m_tSocket, SOL_RFCOMM, SO_BTH_SET_AUTHN_ENABLE, (const char*) &uOptVal, sizeof ( uOptVal ) );

	m_hEvent = CreateEvent ( NULL, FALSE, FALSE, NULL );
	if ( ! m_hEvent )
		return false;

	GUID FileTransferGUID = { 0xC47BECF9, 0x3C95, 0xD211, { 0x98, 0x4E, 0x52, 0x54, 0x00, 0xDC, 0x9E, 0x09 } };

	m_uSize = 10 + sizeof ( FileTransferGUID );

	m_dBuffer [0] = IR_OBEX_CONNECT;
	unsigned short uTmp = htons ( (unsigned short) m_uSize );
	m_dBuffer [1] = LOBYTE ( uTmp );
	m_dBuffer [2] = HIBYTE ( uTmp );
	m_dBuffer [3] = IR_OBEX_VERSION;
	m_dBuffer [4] = IR_OBEX_CONNECT_FLAGS;
	uTmp = htons ( (unsigned short) MAX_PACKET_SIZE );
	m_dBuffer [5] = LOBYTE ( uTmp );
	m_dBuffer [6] = HIBYTE ( uTmp );
	m_dBuffer [7] = IR_OBEX_TARGET;
	uTmp = htons ( sizeof ( FileTransferGUID ) + 3 );
	m_dBuffer [8] = LOBYTE ( uTmp );
	m_dBuffer [9] = HIBYTE ( uTmp );
	memcpy ( &(m_dBuffer [10]), &FileTransferGUID, sizeof ( FileTransferGUID ) );

	Assert ( m_uThreadId == 0xFFFFFFFF );
	m_hThread = CreateThread ( NULL, 16384, MsBTObexClient_c::Thread, this, STACK_SIZE_PARAM_IS_A_RESERVATION, &m_uThreadId );
	if ( ! m_hThread )
		return false;

	return true;
}

void MsBTObexClient_c::Close ()
{
	SetEvent ( m_hEvent );
	m_bExitFlag = true;
}

bool MsBTObexClient_c::PutHead ( Str_c sFileName, unsigned int uFileSize )
{
	int iFileNameLen = ( sFileName.Length () + 1 ) * sizeof ( wchar_t );

	m_uSize = iFileNameLen + 19;
	m_dBuffer [0] = IR_OBEX_PUT;
	unsigned short uTmp = htons ( (unsigned short)( m_uSize ) );
	m_dBuffer [1] = LOBYTE ( uTmp );
	m_dBuffer [2] = HIBYTE ( uTmp );
	m_dBuffer [3] = IR_CONNECTION_ID;
	DWORD uConnId = htonl ( m_uConnectionId );
	m_dBuffer [4] = LOBYTE ( LOWORD ( uConnId ) );
	m_dBuffer [5] = HIBYTE ( LOWORD ( uConnId ) );
	m_dBuffer [6] = LOBYTE ( HIWORD ( uConnId ) );
	m_dBuffer [7] = HIBYTE ( HIWORD ( uConnId ) );
	m_dBuffer [8] = IR_OBEX_NAME;
	uTmp = htons ( (unsigned short)( iFileNameLen + 3 ) );
	m_dBuffer [9] = LOBYTE ( uTmp );
	m_dBuffer [10] = HIBYTE ( uTmp );

	for ( int i = 0; i < iFileNameLen / 2; ++i )
	{
		unsigned short uToPut = htons ( (unsigned short) sFileName [i] );
		m_dBuffer [11 + 2*i    ] = LOBYTE ( uToPut );
		m_dBuffer [11 + 2*i + 1] = HIBYTE ( uToPut );
	}

	int iIndex = 11 + iFileNameLen;
	m_dBuffer [iIndex++] = IR_OBEX_LENGTH;
	DWORD uTmpLong = htonl ( uFileSize );
	m_dBuffer [iIndex++] = LOBYTE ( LOWORD ( uTmpLong ) );
	m_dBuffer [iIndex++] = HIBYTE ( LOWORD ( uTmpLong ) );
	m_dBuffer [iIndex++] = LOBYTE ( HIWORD ( uTmpLong ) );
	m_dBuffer [iIndex++] = HIBYTE ( HIWORD ( uTmpLong ) );
	m_dBuffer [iIndex++] = IR_OBEX_BODY;
	uTmp = htons ( 3 );
	m_dBuffer [iIndex++] = LOBYTE ( uTmp );
	m_dBuffer [iIndex++] = HIBYTE ( uTmp );

	SetEvent ( m_hEvent );
	
	return true;
}

bool MsBTObexClient_c::PutBody ( unsigned char * pBuffer, unsigned int uSize, bool bEOF )
{
	Assert ( uSize <= m_uMaxData );

	m_uSize = uSize + 11;

	m_dBuffer [0] = bEOF ? IR_OBEX_PUT_FINAL : IR_OBEX_PUT;
	unsigned short uTmp = htons((unsigned short)( m_uSize ));
	m_dBuffer [1] = LOBYTE ( uTmp );
	m_dBuffer [2] = HIBYTE ( uTmp );
	m_dBuffer [3] = IR_CONNECTION_ID;
	DWORD uConnId = htonl ( m_uConnectionId );
	m_dBuffer [4] = LOBYTE ( LOWORD ( uConnId ) );
	m_dBuffer [5] = HIBYTE ( LOWORD ( uConnId ) );
	m_dBuffer [6] = LOBYTE ( HIWORD ( uConnId ) );
	m_dBuffer [7] = HIBYTE ( HIWORD ( uConnId ) );
	m_dBuffer [8] = bEOF ? IR_OBEX_END_OF_BODY : IR_OBEX_BODY;
	uTmp = htons((unsigned short)( uSize + 3 ));
	m_dBuffer [9] = LOBYTE ( uTmp );
	m_dBuffer [10] = HIBYTE ( uTmp );
	memcpy ( &(m_dBuffer [11]), pBuffer, uSize );

	SetEvent ( m_hEvent );

	return true;
}

unsigned int MsBTObexClient_c::GetMaxDataSize () const
{
	return m_uMaxData;
}

void MsBTObexClient_c::OnOpen ( unsigned int uMaxPacketLen, DWORD uConnectionId, bool bOk )
{
	m_uMaxPacket = Max ( Min ( uMaxPacketLen, MAX_PACKET_SIZE ), MIN_PACKET_SIZE );
	m_uMaxData = m_uMaxPacket - 32;
	m_uConnectionId = uConnectionId;

	if ( m_hWnd )
		SendMessage( m_hWnd, MSBT_WM_ONOPENMESSAGE, m_uMaxData, bOk );
}

void MsBTObexClient_c::OnClose ()
{
	if ( m_hWnd )
		SendMessage( m_hWnd, MSBT_WM_ONCLOSEMESSAGE, 0, 0 );
}

void MsBTObexClient_c::OnPut ( int iResponse )
{
	if ( m_hWnd )
		SendMessage ( m_hWnd, MSBT_WM_ONPUTMESSAGE, 0, (LPARAM) iResponse );
}

DWORD MsBTObexClient_c::Thread ( void * pParam )
{
	MsBTObexClient_c * pObex = (MsBTObexClient_c *) pParam;
	Assert ( pObex );

	SOCKADDR_BTH tAddress;

	memset ( &tAddress, 0, sizeof ( tAddress ) );
	tAddress.addressFamily  = AF_BTH;
	tAddress.btAddr 		= pObex->m_tAddress;
	tAddress.serviceClassId = OBEXFileTransferServiceClass_UUID;

	bool bConnected = false;

	BYTE * pBAddr = (BYTE *) &tAddress.btAddr;

	Log ( L" -- connection started" );
	Log ( L"--connecting to:" );
	Log ( L"address: %02X.%02X.%02X.%02X.%02X.%02X", pBAddr [0], pBAddr [1], pBAddr [2], pBAddr [3], pBAddr [4], pBAddr [5] );

	for ( int i = 0; i < 5 && ! bConnected; ++i )
	{
		if ( connect ( pObex->m_tSocket, (SOCKADDR *)&tAddress, sizeof ( tAddress ) ) == SOCKET_ERROR )
		{
			Log ( L"connection error: %d", WSAGetLastError () );
			Sleep ( 500 );
		}
		else
			bConnected = true;
	}

	if ( bConnected )
		Log ( L"-- connected" );
	else
		Log ( L"-- not connected" );

	if ( ! bConnected )
	{
		pObex->OnOpen ( 0, 0, false );
		return 0;
	}

	if ( send ( pObex->m_tSocket, (const char*) pObex->m_dBuffer, pObex->m_uSize, 0 ) != pObex->m_uSize )
	{
		pObex->OnOpen ( 0, 0, false );
		return 0;
	}

	int iRes = recv ( pObex->m_tSocket, (char*) pObex->m_dBuffer, MAX_PACKET_SIZE, 0 );
	if ( iRes != SOCKET_ERROR && iRes != 0 )
	{
		if ( pObex->m_dBuffer [0] != IR_OBEX_SUCCESS )
			pObex->OnOpen ( 0, 0, false );
		else
		{
			DWORD uConnectionId = 0;
			if ( pObex->m_dBuffer [7] == IR_CONNECTION_ID )
				uConnectionId = ntohl ( *(DWORD *) & ( pObex->m_dBuffer [8] ) );

			unsigned short uPacket = ( ( (unsigned short) ( pObex->m_dBuffer [5] ) ) << 8 ) | ( (unsigned short) pObex->m_dBuffer [6] );
			pObex->OnOpen ( uPacket, uConnectionId, true );
		}
	}
	else 
	{
		pObex->OnOpen ( 0, 0, false );
		return 0;
	}

	while ( true )
	{
		WaitForSingleObject ( pObex->m_hEvent, INFINITE );

		if ( pObex->m_bExitFlag )
			return 0;

		if ( send ( pObex->m_tSocket, (const char*) pObex->m_dBuffer, pObex->m_uSize, 0 ) != pObex->m_uSize )
			pObex->OnPut ( 0 );
		else
		{
			int iRes = recv ( pObex->m_tSocket, (char*) pObex->m_dBuffer, pObex->m_uMaxPacket, 0 );
			if ( iRes == SOCKET_ERROR || iRes == 0 )
				pObex->OnPut ( 0 );
			else
				pObex->OnPut ( pObex->m_dBuffer [0] );
		}
	}

	return 0;
}