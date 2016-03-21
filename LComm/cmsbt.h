#ifndef _cmsbt_
#define _cmsbt_

#include "LCore/cmain.h"
#include "LCore/cstr.h"
#include "LComm/ccomm.h"
#include "LComm/cbtdefs.h"

#include "ws2bth.h"

const UINT MSBT_WM_DEVICEFOUNDMESSAGE =		RegisterWindowMessage(_T("MSBT_WM_DEVICEFOUNDMESSAGE"));
const UINT MSBT_WM_INQUIRYCOMPLETEMESSAGE =	RegisterWindowMessage(_T("MSBT_WM_INQUIRYCOMPLETEMESSAGE"));
const UINT MSBT_WM_ONOPENMESSAGE =			RegisterWindowMessage(_T("MSBT_WM_ONOPENMESSAGE"));
const UINT MSBT_WM_ONCLOSEMESSAGE =			RegisterWindowMessage(_T("MSBT_WM_ONCLOSEMESSAGE"));
const UINT MSBT_WM_ONABORTMESSAGE =			RegisterWindowMessage(_T("MSBT_WM_ONABORTMESSAGE"));
const UINT MSBT_WM_ONPUTMESSAGE =			RegisterWindowMessage(_T("MSBT_WM_ONPUTMESSAGE"));

class MsBT_c : public WindowNotifier_c
{
public:
					MsBT_c ();
					~MsBT_c ();

	bool			StartInquiry ();
	void			StopInquiry ();

	virtual void 	OnInquiryComplete ( bool bSuccess );
    virtual void 	OnDeviceResponded ( const wchar_t * szDeviceName, SOCKADDR_BTH * pAddress );

private:
	bool			m_bInquiry;
	bool			m_bExitFlag;
	HANDLE			m_hThread;
	DWORD			m_uThreadId;

	static DWORD	EnumThread ( void * pParam );
};

class MsBTObexClient_c : public WindowNotifier_c
{
public:
	static const int MAX_PACKET_SIZE = 0x0FD7;
	static const int MIN_PACKET_SIZE = 256;

					MsBTObexClient_c ( const BT_ADDR & tAddress );
					~MsBTObexClient_c ();

	bool			Open ();
	void			Close ();
	bool			PutHead ( Str_c sFileName, unsigned int uFileSize );
	bool			PutBody ( unsigned char * pBuffer, unsigned int uSize, bool bEOF );

	unsigned int	GetMaxDataSize () const;

protected:
	void			OnOpen ( unsigned int uMaxPacketLen, DWORD uConnectionId, bool bOk );
	void			OnClose ();
	void			OnPut ( int iResponse );

private:
	bool			m_bExitFlag;
	HANDLE			m_hEvent;
	HANDLE			m_hThread;
	DWORD			m_uThreadId;
	SOCKET			m_tSocket;
	BT_ADDR			m_tAddress;
	
	unsigned char	m_dBuffer [MAX_PACKET_SIZE];
	unsigned int	m_uSize;

	unsigned int	m_uMaxPacket;
	unsigned int	m_uMaxData;
	DWORD			m_uConnectionId;

	static DWORD	Thread ( void * pParam );
};

#endif