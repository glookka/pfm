#ifndef _widcommbt_
#define _widcommbt_

#include "wbt.h"

#include "BtIfClasses.h"
#include "BtIfObexHeaders.h" 
#include "../../LComm/cbtdefs.h"

class BTWindowNotifier_c
{
public:
					BTWindowNotifier_c ();

	void			SetWindow ( HWND hWnd );

protected:
	HWND			m_hWnd;
};

class WidcommStack_c : public CBtIf, public BTWindowNotifier_c
{
protected:
    virtual void 	OnDeviceResponded ( BD_ADDR bda, DEV_CLASS devClass, BD_NAME bdName, BOOL bConnected );
    virtual void 	OnDiscoveryComplete ();
	virtual void 	OnInquiryComplete ( BOOL bSuccess, short nResponses );
};


class WidcommObexClient_c : public CObexClient, public BTWindowNotifier_c
{
protected:
	virtual void 	OnOpen ( CObexHeaders *p_confirm, UINT16 tx_mtu, tOBEX_ERRORS code, tOBEX_RESPONSE_CODE response );
	virtual void 	OnClose ( CObexHeaders *p_confirm, tOBEX_ERRORS code, tOBEX_RESPONSE_CODE response );
	virtual void 	OnPut ( CObexHeaders *p_confirm, tOBEX_ERRORS code, tOBEX_RESPONSE_CODE response );
};

#endif
