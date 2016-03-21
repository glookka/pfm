#include "pch.h"

#include "widcommbt.h"

///////////////////////////////////////////////////////////////////////////////////////////
BTWindowNotifier_c::BTWindowNotifier_c ()
	: m_hWnd 	( NULL )
{
}

void BTWindowNotifier_c::SetWindow ( HWND hWnd )
{
	m_hWnd = hWnd;
}


///////////////////////////////////////////////////////////////////////////////////////////

void WidcommStack_c::OnDeviceResponded ( BD_ADDR bda, DEV_CLASS devClass, BD_NAME bdName, BOOL bConnected )
{
	BLUETOOTH_DEVICE_INFO tDevInfo;
	memset (&tDevInfo, 0, sizeof ( tDevInfo ) );

	tDevInfo.dwSize		= sizeof ( BLUETOOTH_DEVICE_INFO );
	tDevInfo.fConnected	= bConnected;

	memcpy ( tDevInfo.Address.rgBytes, bda, BD_ADDR_LEN );

	// Set the device class
	tDevInfo.ulClassofDevice = ((ULONG)devClass[0] << 16) + ((ULONG)devClass[1] << 8) + devClass[2];
	
	if ( ! bdName [0] )
		wsprintf ( tDevInfo.szName, L"%02X.%02X.%02X.%02X.%02X.%02X", tDevInfo.Address.rgBytes [0], tDevInfo.Address.rgBytes [1],
			tDevInfo.Address.rgBytes [2], tDevInfo.Address.rgBytes [3], tDevInfo.Address.rgBytes [4], tDevInfo.Address.rgBytes [5] );
	else
		MultiByteToWideChar( CP_UTF8, 0, (const char*)bdName, -1, tDevInfo.szName, BD_NAME_LEN );
		

	if ( m_hWnd )
		SendMessage( m_hWnd, WIDCOMM_WM_DEVICEFOUNDMESSAGE, 0, (LPARAM)&tDevInfo );
}


void WidcommStack_c::OnInquiryComplete ( BOOL bSuccess, short nResponses )
{
	if ( m_hWnd )
		SendMessage ( m_hWnd, WIDCOMM_WM_INQUIRYCOMPLETEMESSAGE, 0, MAKELPARAM ( bSuccess, nResponses ) );
}


void WidcommStack_c::OnDiscoveryComplete ()
{
	if ( m_hWnd )
		SendMessage ( m_hWnd, WIDCOMM_WM_DISCOVERYCOMPLETEMESSAGE, 0, 0 );
}


///////////////////////////////////////////////////////////////////////////////////////////

void WidcommObexClient_c::OnOpen ( CObexHeaders *p_confirm, UINT16 tx_mtu, tOBEX_ERRORS code, tOBEX_RESPONSE_CODE response )
{
	if ( m_hWnd )
		SendMessage ( m_hWnd, WIDCOMM_WM_ONOPENMESSAGE, (WPARAM) tx_mtu, (LPARAM) ( response == OBEX_RSP_OK ) );
}


void WidcommObexClient_c::OnClose ( CObexHeaders *p_confirm, tOBEX_ERRORS code, tOBEX_RESPONSE_CODE response )
{
	if ( m_hWnd )
		SendMessage ( m_hWnd, WIDCOMM_WM_ONCLOSEMESSAGE, (WPARAM) code, (LPARAM) response );
}

void WidcommObexClient_c::OnPut ( CObexHeaders *p_confirm, tOBEX_ERRORS code, tOBEX_RESPONSE_CODE response )
{
	if ( m_hWnd )
		SendMessage ( m_hWnd, WIDCOMM_WM_ONPUTMESSAGE, 0, (LPARAM) response );
}

///////////////////////////////////////////////////////////////////////////////////////////

WidcommObexClient_c * CreateWidcommObexClient ()
{
	return new WidcommObexClient_c;
}

void DestroyWidcommObexClient ( WidcommObexClient_c * pClient )
{
	delete pClient;
}

void ShutdownWidcommBT ()
{
	WIDCOMMSDK_ShutDown ();
}