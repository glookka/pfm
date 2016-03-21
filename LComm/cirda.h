#ifndef _irda_
#define _irda_

#include "LCore/cmain.h"
#include "LCore/cstr.h"
#include "LComm/ccomm.h"

#include "af_irda.h"

const unsigned char IR_OBEX_CONTINUE		= 0x90;
const unsigned char IR_OBEX_CONNECT 		= 0x80;
const unsigned char IR_OBEX_VERSION 		= 0x10;
const unsigned char IR_OBEX_CONNECT_FLAGS 	= 0x00;
const unsigned char IR_CONNECTION_ID		= 0xCB;
const unsigned char IR_OBEX_PUT 			= 0x02;
const unsigned char IR_OBEX_PUT_FINAL 		= 0x82;
const unsigned char IR_OBEX_NAME 			= 0x01;
const unsigned char IR_OBEX_LENGTH 			= 0xC3;
const unsigned char IR_OBEX_TARGET			= 0x46;
const unsigned char IR_OBEX_BODY 			= 0x48;
const unsigned char IR_OBEX_END_OF_BODY 	= 0x49;
const unsigned char IR_OBEX_SUCCESS 		= 0xA0;

const UINT IRDA_WM_DEVICEFOUNDMESSAGE =		RegisterWindowMessage(_T("IRDA_WM_DEVICEFOUNDMESSAGE"));
const UINT IRDA_WM_INQUIRYCOMPLETEMESSAGE =	RegisterWindowMessage(_T("IRDA_WM_INQUIRYCOMPLETEMESSAGE"));
const UINT IRDA_WM_ONOPENMESSAGE =			RegisterWindowMessage(_T("IRDA_WM_ONOPENMESSAGE"));
const UINT IRDA_WM_ONCLOSEMESSAGE =			RegisterWindowMessage(_T("IRDA_WM_ONCLOSEMESSAGE"));
const UINT IRDA_WM_ONABORTMESSAGE =			RegisterWindowMessage(_T("IRDA_WM_ONABORTMESSAGE"));
const UINT IRDA_WM_ONPUTMESSAGE =			RegisterWindowMessage(_T("IRDA_WM_ONPUTMESSAGE"));

class Irda_c : public WindowNotifier_c
{
public:
					Irda_c ();
					~Irda_c ();

	bool			StartInquiry ();
	void			StopInquiry ();

	virtual void 	OnInquiryComplete ();
    virtual void 	OnDeviceResponded ( IRDA_DEVICE_INFO & tDevInfo, int iIrdaAttribute );

	SOCKET &		GetSocket ();

private:
	static const int MAX_NUM_DEVICES = 8;
	static const int DEVLIST_SIZE = sizeof (DEVICELIST) + ( sizeof (IRDA_DEVICE_INFO) * ( MAX_NUM_DEVICES - 1 ) );

	bool			m_bInquiry;
	bool			m_bExitFlag;
	HANDLE			m_hThread;
	DWORD			m_uThreadId;
	SOCKET			m_tSocket;

	unsigned char	m_dDevListBuff [DEVLIST_SIZE];

	static DWORD	EnumThread ( void * pParam );
};

class IrdaObexClient_c : public WindowNotifier_c
{
public:
	static const int MAX_PACKET_SIZE = 16384;
	static const int MIN_PACKET_SIZE = 256;

					IrdaObexClient_c ( SOCKET & tSocket, BYTE * pDeviceId, int iIrdaAttribute );
					~IrdaObexClient_c ();

	bool			Open ();
	void			Close ();
	bool			PutHead ( Str_c sFileName, unsigned int uFileSize );
	bool			PutBody ( unsigned char * pBuffer, unsigned int uSize, bool bEOF );

	unsigned int	GetMaxDataSize () const;

protected:
	void			OnOpen ( unsigned int uMaxPacketLen, bool bOk );
	void			OnClose ();
	void			OnPut ( int iResponse );

private:
	bool			m_bExitFlag;
	HANDLE			m_hEvent;
	HANDLE			m_hThread;
	DWORD			m_uThreadId;
	SOCKET			m_tSocket;
	int				m_iIrdaAttrib;

	unsigned char	m_dBuffer [MAX_PACKET_SIZE];
	unsigned int	m_uSize;

	unsigned int	m_uMaxPacket;
	unsigned int	m_uMaxData;

	unsigned char	m_dDeviceId [4];

	static DWORD	Thread ( void * pParam );
};

#endif