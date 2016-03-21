#include "pch.h"

#include "LComm/cirda.h"
#include "LCore/clog.h"
#include "LCore/ctimer.h"

const double SHUTDOWN_TIME = 2.0;


Irda_c::Irda_c ()
	: m_bInquiry	( false )
	, m_bExitFlag	( false )
	, m_hThread		( NULL )
	, m_uThreadId	( 0xFFFFFFFF )
	, m_tSocket		( INVALID_SOCKET )
{
	memset ( m_dDevListBuff, 0, sizeof ( m_dDevListBuff ) ) ;
}

Irda_c::~Irda_c ()
{
	double fStartTime = g_Timer.GetTimeSec ();
	// wait for the thread to terminate
	if ( m_bInquiry )
	{
		m_bExitFlag = true;
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
	}

	CloseHandle ( m_hThread );

	if ( m_tSocket != INVALID_SOCKET )
		closesocket ( m_tSocket );
}

bool Irda_c::StartInquiry ()
{
	m_tSocket = socket ( AF_IRDA, SOCK_STREAM , 0 );
	if ( m_tSocket == INVALID_SOCKET)
		return false;

	Assert ( m_uThreadId == 0xFFFFFFFF );
	m_hThread = CreateThread ( NULL, 16384, Irda_c::EnumThread, this, STACK_SIZE_PARAM_IS_A_RESERVATION, &m_uThreadId );
	if ( ! m_hThread )
		return false;

	m_bInquiry = true;

	return true;
}

void Irda_c::StopInquiry ()
{
	m_bExitFlag = true;
}

void Irda_c::OnDeviceResponded ( IRDA_DEVICE_INFO & tDevInfo, int iIrdaAttribute )
{
	if ( m_hWnd )
		SendMessage( m_hWnd, IRDA_WM_DEVICEFOUNDMESSAGE, (WPARAM) iIrdaAttribute, (LPARAM)&tDevInfo );
}

void Irda_c::OnInquiryComplete ()
{
	m_bInquiry = false;

	if ( m_hWnd )
		SendMessage ( m_hWnd, IRDA_WM_INQUIRYCOMPLETEMESSAGE, 0, 0 );
}

SOCKET & Irda_c::GetSocket ()
{
	return m_tSocket;
}

DWORD Irda_c::EnumThread ( void * pParam )
{
	const char szClass [] = "OBEX";
	const char szAttribute [] = "IrDA:TinyTP:LsapSel";
	Irda_c * pIrda = (Irda_c *) pParam;

	while ( true )
	{
		int iListSize = DEVLIST_SIZE;
		int iRes = getsockopt ( pIrda->m_tSocket, SOL_IRLMP, IRLMP_ENUMDEVICES, (char *) pIrda->m_dDevListBuff, &iListSize );

		if ( pIrda->m_bExitFlag )
			break;

		if ( iRes != SOCKET_ERROR )
		{
			DEVICELIST * pDevList = (DEVICELIST * )pIrda->m_dDevListBuff;
			for ( int i = 0; i < pDevList->numDevice; ++i )
			{
				IAS_QUERY tIASQuery;

				int iQueryLen = sizeof ( tIASQuery );

				// Query IAS database
				memcpy ( &tIASQuery.irdaDeviceID, pDevList->Device [i].irdaDeviceID, 4 );
				memcpy ( &tIASQuery.irdaClassName, szClass, strlen ( szClass ) + 1 );
				memcpy ( &tIASQuery.irdaAttribName, szAttribute, strlen ( szAttribute ) + 1 );

				if ( getsockopt ( pIrda->GetSocket (), SOL_IRLMP, IRLMP_IAS_QUERY, (char*) &tIASQuery, &iQueryLen ) == SOCKET_ERROR )
					break;

				if ( pIrda->m_bExitFlag )
					break;

			    if ( tIASQuery.irdaAttribType == IAS_ATTRIB_NO_CLASS )
					break;

			    if ( tIASQuery.irdaAttribType == IAS_ATTRIB_NO_ATTRIB )
					break;

				pIrda->OnDeviceResponded ( pDevList->Device [i], tIASQuery.irdaAttribute.irdaAttribInt );
			}
		}

		if ( pIrda->m_bExitFlag )
			break;

		Sleep ( 1000 );
	}

	pIrda->OnInquiryComplete ();

	return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////
IrdaObexClient_c::IrdaObexClient_c ( SOCKET & tSocket, BYTE * pDeviceId, int iIrdaAttribute )
	: m_tSocket		( tSocket )
	, m_iIrdaAttrib ( iIrdaAttribute )
	, m_bExitFlag	( false )
	, m_hEvent		( NULL )
	, m_hThread		( NULL )
	, m_uThreadId	( 0xFFFFFFFF )
	, m_uSize		( 0 )
	, m_uMaxPacket	( 0 )
	, m_uMaxData	( 0 )
{
	memcpy ( m_dDeviceId, pDeviceId, 4 );
	memset ( m_dBuffer, 0, sizeof ( m_dBuffer ) );
}


IrdaObexClient_c::~IrdaObexClient_c ()
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
}


bool IrdaObexClient_c::Open ()
{
	m_hEvent = CreateEvent ( NULL, FALSE, FALSE, NULL );
	if ( ! m_hEvent )
		return false;

	if ( m_tSocket == INVALID_SOCKET )
		return false;

	m_uSize = 7;

	m_dBuffer [0] = IR_OBEX_CONNECT;
	unsigned short uTmp = htons ( (unsigned short) m_uSize );
	m_dBuffer [1] = LOBYTE ( uTmp );
	m_dBuffer [2] = HIBYTE ( uTmp );
	m_dBuffer [3] = IR_OBEX_VERSION;
	m_dBuffer [4] = IR_OBEX_CONNECT_FLAGS;
	uTmp = htons ( (unsigned short) MAX_PACKET_SIZE );
	m_dBuffer [5] = LOBYTE ( uTmp );
	m_dBuffer [6] = HIBYTE ( uTmp );

	Assert ( m_uThreadId == 0xFFFFFFFF );
	m_hThread = CreateThread ( NULL, 16384, IrdaObexClient_c::Thread, this, STACK_SIZE_PARAM_IS_A_RESERVATION, &m_uThreadId );
	if ( ! m_hThread )
		return false;

	return true;
}

void IrdaObexClient_c::Close ()
{
	SetEvent ( m_hEvent );
	m_bExitFlag = true;
}

bool IrdaObexClient_c::PutHead ( Str_c sFileName, unsigned int uFileSize )
{
	int iFileNameLen = ( sFileName.Length () + 1 ) * sizeof ( wchar_t );

	m_uSize = iFileNameLen + 11;
	m_dBuffer [0] = IR_OBEX_PUT;
	unsigned short uTmp = htons ( (unsigned short)( m_uSize ) );
	m_dBuffer [1] = LOBYTE ( uTmp );
	m_dBuffer [2] = HIBYTE ( uTmp );
	m_dBuffer [3] = IR_OBEX_NAME;
	uTmp = htons ( (unsigned short)( iFileNameLen + 3 ) );
	m_dBuffer [4] = LOBYTE ( uTmp );
	m_dBuffer [5] = HIBYTE ( uTmp );

	for ( int i = 0; i < iFileNameLen / 2; ++i )
	{
		unsigned short uToPut = htons ( (unsigned short) sFileName [i] );
		m_dBuffer [6 + 2*i    ] = LOBYTE ( uToPut );
		m_dBuffer [6 + 2*i + 1] = HIBYTE ( uToPut );
	}

	m_dBuffer [6 + iFileNameLen + 0] = IR_OBEX_LENGTH;
	DWORD uTmpLong = htonl ( uFileSize );
	m_dBuffer [6 + iFileNameLen + 1] = LOBYTE ( LOWORD ( uTmpLong ) );
	m_dBuffer [6 + iFileNameLen + 2] = HIBYTE ( LOWORD ( uTmpLong ) );
	m_dBuffer [6 + iFileNameLen + 3] = LOBYTE ( HIWORD ( uTmpLong ) );
	m_dBuffer [6 + iFileNameLen + 4] = HIBYTE ( HIWORD ( uTmpLong ) );

	SetEvent ( m_hEvent );
	
	return true;
}

bool IrdaObexClient_c::PutBody ( unsigned char * pBuffer, unsigned int uSize, bool bEOF )
{
	Assert ( uSize <= m_uMaxData );

	m_uSize = uSize + 6;

	m_dBuffer [0] = bEOF ? IR_OBEX_PUT_FINAL : IR_OBEX_PUT;
	unsigned short uTmp = htons((unsigned short)( m_uSize ));
	m_dBuffer [1] = LOBYTE ( uTmp );
	m_dBuffer [2] = HIBYTE ( uTmp );
	m_dBuffer [3] = bEOF ? IR_OBEX_END_OF_BODY : IR_OBEX_BODY;
	uTmp = htons((unsigned short)( uSize + 3 ));
	m_dBuffer [4] = LOBYTE ( uTmp );
	m_dBuffer [5] = HIBYTE ( uTmp );
	memcpy ( &(m_dBuffer [6]), pBuffer, uSize );

	SetEvent ( m_hEvent );

	return true;
}

unsigned int IrdaObexClient_c::GetMaxDataSize () const
{
	return m_uMaxData;
}

void IrdaObexClient_c::OnOpen ( unsigned int uMaxPacketLen, bool bOk )
{
	m_uMaxPacket = Max ( Min ( uMaxPacketLen, MAX_PACKET_SIZE ), MIN_PACKET_SIZE );
	m_uMaxData = m_uMaxPacket - 32;

	if ( m_hWnd )
		SendMessage( m_hWnd, IRDA_WM_ONOPENMESSAGE, 0, bOk );
}

void IrdaObexClient_c::OnClose ()
{
	if ( m_hWnd )
		SendMessage( m_hWnd, IRDA_WM_ONCLOSEMESSAGE, 0, 0 );
}

void IrdaObexClient_c::OnPut ( int iResponse )
{
	if ( m_hWnd )
		SendMessage ( m_hWnd, IRDA_WM_ONPUTMESSAGE, 0, (LPARAM) iResponse );
}

DWORD IrdaObexClient_c::Thread ( void * pParam )
{
	IrdaObexClient_c * pObex = (IrdaObexClient_c *) pParam;
	Assert ( pObex );

	SOCKADDR_IRDA tAddress;
	memset ( &tAddress, 0, sizeof ( tAddress ) );
	tAddress.irdaAddressFamily = AF_IRDA;
	sprintf ( tAddress.irdaServiceName, "LSAP-SEL%d", pObex->m_iIrdaAttrib );
	memcpy ( &tAddress.irdaDeviceID, pObex->m_dDeviceId, 4 );

	bool bConnected = false;

	// 10 connect retries
	for ( int i = 0; i < 10 && ! bConnected; ++i )
	{
		if ( connect ( pObex->m_tSocket, (const sockaddr *) &tAddress, sizeof ( tAddress ) ) == SOCKET_ERROR )
			Sleep ( 100 );
		else
			bConnected = true;
	}

	if ( ! bConnected )
	{
		pObex->OnOpen ( 0, false );
		return 0;
	}

	if ( send ( pObex->m_tSocket, (const char*) pObex->m_dBuffer, pObex->m_uSize, 0 ) != pObex->m_uSize )
	{
		pObex->OnOpen ( 0, false );
		return 0;
	}

	int iRes = recv ( pObex->m_tSocket, (char*) pObex->m_dBuffer, MAX_PACKET_SIZE, 0 );
	if ( iRes != SOCKET_ERROR && iRes != 0 )
	{
		unsigned short uPacket = ( ( (unsigned short) ( pObex->m_dBuffer [5] ) ) << 8 ) | ( (unsigned short) pObex->m_dBuffer [6] );
		pObex->OnOpen ( uPacket, true );
	}
	else 
	{
		pObex->OnOpen ( 0, false );
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