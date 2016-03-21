#include "pch.h"

#include "LDialogs/dsend.h"
#include "LComm/cirda.h"
#include "LComm/ccomm.h"
#include "LComm/cbluetooth.h"
#include "LComm/cmsbt.h"
#include "pfm/config.h"
#include "LFile/fsend.h"
#include "pfm/gui.h"
#include "pfm/dialogs/errors.h"
#include "pfm/resources.h"
#include "Utils/UWidcommBTDll/wbt.h"

#include "aygshell.h"
#include "Dlls/Resource/resource.h"

extern HWND g_hMainWindow;

enum SendType_e
{
	 SEND_IRDA
	,SEND_BT_WIDCOMM
	,SEND_BT_MS
};

Irda_c * g_pIrda 	= NULL;
MsBT_c * g_pMSBT	= NULL;
SendType_e g_eType 	= SEND_IRDA;
/*
class FindDeviceDlg_c : public WindowResizer_c
{
public:
	FindDeviceDlg_c ()
		: m_hList		( NULL )
		, m_hToolbar	( NULL )
		, m_hOldCursor	( NULL )
		, m_bInquiry	( true )
		, m_uBDA		( 0 )
		, m_uSCN		( 0 )
		, m_iSelected	( -1 )
	{	
	}

	void Init ( HWND hDlg )
	{
		switch ( g_eType )
		{
		case SEND_IRDA:
			Assert ( g_pIrda );
			g_pIrda->SetWindow ( hDlg );
			g_pIrda->StartInquiry ();
			break;

		case SEND_BT_WIDCOMM:
			dll_WBT_CreateStack ();
			dll_WBT_SetWindow ( hDlg );
			dll_WBT_StartInquiry ();
			break;

		case SEND_BT_MS:
			Assert ( g_pMSBT );
			g_pMSBT->SetWindow ( hDlg );
			g_pMSBT->StartInquiry ();
			break;
		}

		m_hToolbar = InitFullscreenDlg ( hDlg, IDM_OK_CANCEL, SHCMBF_HIDESIPBUTTON );

		SetToolbarText ( m_hToolbar, IDOK, Txt ( T_TBAR_SEND ) );

		m_hList = GetDlgItem ( hDlg, IDC_LIST );
		HWND hTitle = GetDlgItem ( hDlg, IDC_TITLE );
		SendMessage ( hTitle, WM_SETFONT, (WPARAM)g_tResources.m_hBoldSystemFont, TRUE );

		LVCOLUMN tColumn;
		ZeroMemory ( &tColumn, sizeof ( tColumn ) );
		tColumn.mask = LVCF_TEXT;
		ListView_InsertColumn ( m_hList, tColumn.iSubItem, &tColumn );

		ListView_SetExtendedListViewStyle ( m_hList, LVS_EX_FULLROWSELECT );
		
		SetWindowText ( hTitle, Txt ( g_eType == SEND_IRDA ? T_DLG_IR_TITLE : T_DLG_BT_TITLE ) );

		DlgTxt ( hDlg, IDC_SEARCH, T_DLG_SEARCHING_DEVICES );

		SetDlg ( hDlg );
		SetResizer ( m_hList );
		if ( HandleSizeChange ( 0, 0, 0, 0, true ) )
			Resize ();
	}

	// ir device responded 
	void OnDeviceResponded ( HWND hDlg, IRDA_DEVICE_INFO * pDevInfo, int iAttributeId )
	{
		Assert ( pDevInfo );
		BLUETOOTH_DEVICE_INFO tInfo;
		memset ( &tInfo, 0, sizeof ( tInfo ) );
		for ( int i = 0; i < 4; ++i )
			tInfo.Address.rgBytes [i] = pDevInfo->irdaDeviceID [i];

		tInfo.ulClassofDevice = iAttributeId;
		AnsiToUnicode ( pDevInfo->irdaDeviceName, tInfo.szName );
		AddDevice ( hDlg, tInfo );
	}

	// bt device responded
	void OnDeviceResponded ( HWND hDlg, BLUETOOTH_DEVICE_INFO * pDevInfo )
	{
		Assert ( pDevInfo );
		AddDevice ( hDlg, *pDevInfo );
	}

	void OnInquiryComplete ( HWND hDlg )
	{
		m_bInquiry = false;
		DlgTxt ( hDlg, IDC_SEARCH, T_DLG_DEV_SEARCH_FINISHED );
	}

	void StartDiscovery ( HWND hDlg )
	{
		int iSelected = ListView_GetSelectionMark ( m_hList );
		if ( iSelected == -1 )
			return;

		m_uBDA = m_dDevices [iSelected].Address.ullLong;

		m_iSelected = iSelected;

		switch ( g_eType )
		{
		case SEND_IRDA:
		case SEND_BT_MS:
			EndDialog ( hDlg, IDOK );
			Close ();
			break;

		case SEND_BT_WIDCOMM:
			{
				if ( ! dll_WBT_StartDiscovery ( (BYTE*)&m_uBDA ) )
					ShowErrorDialog ( hDlg, Txt ( T_MSG_ERROR ), Txt ( T_DLG_BT_ERR_CONN ), IDD_ERROR_OK );
				else
					ShowDiscoveryProgress ();
			}
			break;
		}
	}

	void OnDiscoveryComplete ( HWND hDlg )
	{
		HideDiscoveryProgress ();

		const int MAX_SERVICES = 8;

		int nRecords = dll_WBT_ReadDiscoveryRecords ( (BYTE *)&m_uBDA );
		if ( ! nRecords )
			ShowErrorDialog ( hDlg, Txt ( T_MSG_ERROR ), Txt ( T_DLG_BT_ERR_CONN ), IDD_ERROR_OK );
		else
		{
			if ( ! dll_WBT_PrepareSCN ( m_uSCN, 0, 0, false ) )
			{
				ShowErrorDialog ( hDlg, Txt ( T_MSG_ERROR ), Txt ( T_DLG_BT_ERR_CONN ), IDD_ERROR_OK );
				return;
			}

			if ( m_bInquiry )
			{
				dll_WBT_StopInquiry ();
				m_bInquiry = false;
			}

			EndDialog ( hDlg, IDOK );
			Close ();
		}
	}

	void AddDevice ( HWND hDlg, BLUETOOTH_DEVICE_INFO & tDevInfo )
	{
		int iIndex = -1;
		for ( int i = 0; i < m_dDevices.Length () && iIndex == -1; ++i )
			if ( m_dDevices [i].Address.ullLong == tDevInfo.Address.ullLong )
			{
				m_dDevices [i] = tDevInfo;
				iIndex = i;
			}

			if ( iIndex != -1 )
			{
				ListView_SetItemText ( m_hList, iIndex, 0, tDevInfo.szName );
			}
			else
			{
				LVITEM tItem;
				ZeroMemory ( &tItem, sizeof ( tItem ) );
				tItem.mask = LVIF_TEXT;
				tItem.pszText = tDevInfo.szName;
				tItem.iItem = m_dDevices.Length ();
				ListView_InsertItem	 ( m_hList, &tItem );

				m_dDevices.Add ( tDevInfo );
			}

	}

	void Resize ()
	{
		WindowResizer_c::Resize ();
		ListView_SetColumnWidth ( m_hList, 0, GetColumnWidthRelative ( 1.0f ) );
	}

	const BLUETOOTH_DEVICE_INFO & GetDeviceInfo () const
	{
		Assert ( m_iSelected != -1 );
		return m_dDevices [m_iSelected];
	}

	unsigned char GetSCN () const
	{
		return m_uSCN;
	}

	void Close ()
	{
		HideDiscoveryProgress ();

		if ( m_bInquiry  )
		{
			switch ( g_eType )
			{
			case SEND_IRDA:
				Assert ( g_pIrda );
				g_pIrda->StopInquiry ();
				break;

			case SEND_BT_WIDCOMM:
				dll_WBT_StopInquiry ();
				break;

			case SEND_BT_MS:
				Assert ( g_pMSBT );
				g_pMSBT->StopInquiry ();
				break;
			}
		}

		CloseFullscreenDlg ();
	}

private:
	HCURSOR			m_hOldCursor;
	HWND			m_hToolbar;
	int				m_iSelected;
	bool			m_bInquiry;
	ULONGLONG		m_uBDA;
	unsigned char	m_uSCN;
	Array_T	< BLUETOOTH_DEVICE_INFO > m_dDevices;
	HWND			m_hList;

	void ShowDiscoveryProgress ()
	{
		EnableToolbarButton ( m_hToolbar, IDOK, false );
		m_hOldCursor = SetCursor ( LoadCursor ( NULL, IDC_WAIT ) );
	}

	void HideDiscoveryProgress ()
	{
		SetCursor ( m_hOldCursor );
		m_hOldCursor = NULL;
		EnableToolbarButton ( m_hToolbar, IDOK, true );
	}
};


static FindDeviceDlg_c * g_pFindDeviceDlg = NULL;
*/

static BOOL CALLBACK FindDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
/*	Assert ( g_pFindDeviceDlg );

	if ( Msg == WIDCOMM_WM_DEVICEFOUNDMESSAGE || Msg == MSBT_WM_DEVICEFOUNDMESSAGE )
		g_pFindDeviceDlg->OnDeviceResponded ( hDlg, (BLUETOOTH_DEVICE_INFO *)lParam );
	else if ( Msg == WIDCOMM_WM_INQUIRYCOMPLETEMESSAGE || Msg == IRDA_WM_INQUIRYCOMPLETEMESSAGE || Msg == MSBT_WM_INQUIRYCOMPLETEMESSAGE )
		g_pFindDeviceDlg->OnInquiryComplete ( hDlg );
	else if ( Msg == WIDCOMM_WM_DISCOVERYCOMPLETEMESSAGE )
		g_pFindDeviceDlg->OnDiscoveryComplete ( hDlg );
	else if ( Msg == IRDA_WM_DEVICEFOUNDMESSAGE )
		g_pFindDeviceDlg->OnDeviceResponded ( hDlg, (IRDA_DEVICE_INFO *) lParam, wParam );
*/	
	switch ( Msg )
	{
	case WM_INITDIALOG:
//		g_pFindDeviceDlg->Init ( hDlg );
		SetCursor ( LoadCursor ( NULL, IDC_ARROW ) );
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
//			g_pFindDeviceDlg->StartDiscovery ( hDlg );
			break;

		case IDCANCEL:
			SipShowIM ( SIPF_OFF );
//			g_pFindDeviceDlg->Close ();
			EndDialog ( hDlg, LOWORD (wParam) );
			break;
		}
		break;

	case WM_HELP:
		Help ( L"FileSend" );
		return TRUE;
	}

/*	if ( HandleSizeChange ( hDlg, Msg, wParam, lParam ) )
		g_pFindDeviceDlg->Resize ();*/

	HandleActivate ( hDlg, Msg, wParam, lParam );

	return FALSE;
}


//////////////////////////////////////////////////////////////////////////
class FileSendProgressDlg_c : public WindowProgress_c
{
public:
	FileSendProgressDlg_c ( SelectedFileList_t & tList, const BLUETOOTH_DEVICE_INFO & tInfo, unsigned char uSCN = 0 )
		: m_hDlg 	( NULL )
		, m_tInfo	( tInfo )
		, m_pSender	( NULL )
		, m_pIRObex	( NULL )
		, m_pMSBTObex( NULL )
		, m_uSCN	( uSCN )
		, m_tList	( tList )
		, m_bCancelled	( false )
	{
	}

	~FileSendProgressDlg_c ()
	{
		SafeDelete ( m_pSender );

		switch ( g_eType )
		{
		case SEND_IRDA:
			SafeDelete ( m_pIRObex );
			break;

		case SEND_BT_WIDCOMM:
			dll_WBT_DestroyObexClient ();
			break;

		case SEND_BT_MS:
			SafeDelete ( m_pMSBTObex );
			break;
		}
	}

	void Init ( HWND hDlg )
	{
		m_hDlg = hDlg;
		WindowProgress_c::Init ( hDlg, Txt ( g_eType == SEND_IRDA ? T_DLG_IR_SENDING : T_DLG_BT_SENDING ), NULL );
    	InitFullscreenDlg ( hDlg, IDM_CANCEL, SHCMBF_HIDESIPBUTTON, true );
	}

	bool Open ()
	{
		switch ( g_eType )
		{
		case SEND_IRDA:
			Assert ( g_pIrda );
			m_pIRObex = new IrdaObexClient_c ( g_pIrda->GetSocket (), m_tInfo.Address.rgBytes, int ( m_tInfo.ulClassofDevice ) );
			m_pIRObex->SetWindow ( m_hDlg );

			if ( ! m_pIRObex->Open () )
			{
				ShowError ();
				Close ();
				return false;
			}
			break;

		case SEND_BT_WIDCOMM:
			if ( ! dll_WBT_CreateObexClient ( m_hDlg ) )
			{
				Close ();
				ShowErrorDialog ( g_hMainWindow, false, Txt ( T_DLG_BT_ERR_CONN ), IDD_ERROR_OK );
				return false;
			}
			
			if ( ! dll_WBT_ObextOpen ( m_uSCN, m_tInfo.Address.rgBytes ) )
			{
				Close ();
				ShowErrorDialog ( g_hMainWindow, false, Txt ( T_DLG_BT_ERR_CONN ), IDD_ERROR_OK );
				return false;
			}
			break;

		case SEND_BT_MS:
			m_pMSBTObex = new MsBTObexClient_c ( m_tInfo.Address.ullLong );
			m_pMSBTObex->SetWindow ( m_hDlg );

			if ( ! m_pMSBTObex->Open () )
			{
				ShowError ();
				Close ();
				return false;
			}
			break;
		}

		return true;
	}

	void OnOpen ( unsigned int uPacket, bool bOk )
	{
		if ( bOk )
		{
			Assert ( ! m_pSender );
			switch  ( g_eType )
			{
			case SEND_IRDA:
				Assert ( m_pIRObex );
				m_pSender = new FileSendIrda_c ( m_pIRObex, m_pIRObex->GetMaxDataSize (), m_tList, PrepareCallback );
				break;
			case SEND_BT_WIDCOMM:
				m_pSender = new FileSendWidcomm_c ( uPacket, m_tList, PrepareCallback );
				m_pSender->SetWindow ( m_hDlg );
				break;
			case SEND_BT_MS:
				m_pSender = new FileSendMsBT_c ( m_pMSBTObex, uPacket, m_tList, PrepareCallback );
				m_pSender->SetWindow ( m_hDlg );
				break;
			}

			m_pSender->SetWindow ( m_hDlg );
			WindowProgress_c::Setup ( m_pSender );

			if ( ! m_pSender->StartFile () )
			{
				ShowError ();
				Close ();
			}
		}
		else
		{
			ShowErrorDialog ( m_hDlg, false, Txt ( g_eType == SEND_IRDA ? T_DLG_IR_ERR_CONN : T_DLG_BT_ERR_CONN ), IDD_ERROR_OK );
			Close ();
		}
	}

	void OnClose ()
	{
		ShowErrorDialog ( m_hDlg, false, Txt ( T_DLG_BT_CONN_CLOSED ), IDD_ERROR_OK );
		Close ();
	}

	void OnPut ( int iResponse )
	{
		if ( ! m_bCancelled )
		{
			Assert ( m_pSender );
			if ( ! m_pSender->OnPut ( iResponse ) )
			{
				ShowError ();
				Close ();
			}
			else
				UpdateProgress ();
		}
	}

	void ShowError ()
	{
		if ( m_pSender )
		{
			switch ( m_pSender->GetError () )
			{
			case FileSend_c::ERR_SEND:
				ShowErrorDialog ( m_hDlg, false, Txt ( g_eType == SEND_IRDA ? T_DLG_IR_ERR_SEND : T_DLG_BT_ERR_SEND ), IDD_ERROR_OK );
				break;
			case FileSend_c::ERR_READ:
				ShowErrorDialog ( m_hDlg, false, Txt ( T_DLG_ERR_READ ), IDD_ERROR_OK );
				break;
			}
		}
	}

	void Cancel ()
	{
		Close ();

		if ( m_pSender )
			m_pSender->Cancel ();

		m_bCancelled  = true;
	}

private:
	HWND					m_hDlg;
	BLUETOOTH_DEVICE_INFO	m_tInfo;
	unsigned char			m_uSCN;
	FileSend_c * 			m_pSender;
	IrdaObexClient_c *		m_pIRObex;
	MsBTObexClient_c *		m_pMSBTObex;
	SelectedFileList_t &			m_tList;
	bool					m_bCancelled;

	void Close ()
	{
		CloseFullscreenDlg ();
		EndDialog ( m_hDlg, IDCANCEL );
	}

	void UpdateProgress ()
	{
		if ( m_pSender->GetNameFlag () )
			AlignFileName ( m_hSourceText, m_pSender->GetSourceFileName () );

		WindowProgress_c::UpdateProgress ( m_pSender );
	}
};


static FileSendProgressDlg_c * g_pFileSendProgressDlg = NULL;


static BOOL CALLBACK SendDlgProc ( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam )
{  
	Assert ( g_pFileSendProgressDlg );

	if ( Msg == WIDCOMM_WM_ONOPENMESSAGE || Msg == IRDA_WM_ONOPENMESSAGE || Msg == MSBT_WM_ONOPENMESSAGE )
		g_pFileSendProgressDlg->OnOpen ( (unsigned int) wParam, !!lParam );
	else if ( Msg == WIDCOMM_WM_ONCLOSEMESSAGE || Msg == IRDA_WM_ONCLOSEMESSAGE || Msg == MSBT_WM_ONCLOSEMESSAGE )
		g_pFileSendProgressDlg->OnClose ();
	else if ( Msg == WIDCOMM_WM_ONPUTMESSAGE || Msg == IRDA_WM_ONPUTMESSAGE || Msg == MSBT_WM_ONPUTMESSAGE )
		g_pFileSendProgressDlg->OnPut ( lParam );

	switch ( Msg )
	{
	case WM_INITDIALOG:
		g_pFileSendProgressDlg->Init ( hDlg );
		if ( ! g_pFileSendProgressDlg->Open () )
			return FALSE;
		break;

	case WM_COMMAND:
		switch ( LOWORD ( wParam ) )
		{
		case IDCANCEL:
			g_pFileSendProgressDlg->Cancel ();
			break;
		}
		break;

	case WM_HELP:
		Help ( L"FileSend" );
		return TRUE;
	}

	HandleActivate ( hDlg, Msg, wParam, lParam );

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////

void ShowIRDlg ( SelectedFileList_t & tList )
{
//	Assert ( ! g_pFindDeviceDlg && ! g_pIrda && ! g_pFileSendProgressDlg );

	if ( ! Init_Sockets () )
	{
		ShowErrorDialog ( g_hMainWindow, false, Txt ( T_DLG_IR_ERR_INIT ), IDD_ERROR_OK );
		return;
	}

	g_eType = SEND_IRDA;

	g_pIrda = new Irda_c;
//	g_pFindDeviceDlg = new FindDeviceDlg_c;

	int iDlgRes = DialogBox ( ResourceInstance (), MAKEINTRESOURCE ( IDD_FIND_DEVICE ), g_hMainWindow, FindDlgProc );

	if ( iDlgRes != IDOK )
	{
		SafeDelete ( g_pIrda );
//		SafeDelete ( g_pFindDeviceDlg );
		return;
	}

//	g_pFileSendProgressDlg = new FileSendProgressDlg_c ( tList, g_pFindDeviceDlg->GetDeviceInfo () );

//	SafeDelete ( g_pFindDeviceDlg );

	DialogBox ( ResourceInstance (), MAKEINTRESOURCE ( IDD_SEND_PROGRESS ), g_hMainWindow, SendDlgProc );

	SafeDelete ( g_pFileSendProgressDlg );
	SafeDelete ( g_pIrda );
}


void ShowBTDlg ( SelectedFileList_t & tList )
{
//	Assert ( ! g_pFindDeviceDlg && ! g_pFileSendProgressDlg && ! g_pMSBT );

	SetCursor ( LoadCursor ( NULL, IDC_WAIT ) );
	
	if ( IsWidcommBTPresent () )
	{
		if ( ! Init_WidcommBT () )
		{
			SetCursor ( LoadCursor ( NULL, IDC_ARROW ) );
			ShowErrorDialog ( g_hMainWindow, false, Txt ( T_DLG_BT_ERR_INIT_W ), IDD_ERROR_OK );
			return;
		}

		g_eType = SEND_BT_WIDCOMM;
	}
	else
		if ( IsMSBTPresent () )
		{
			if ( ! Init_MsBT () )
			{
				SetCursor ( LoadCursor ( NULL, IDC_ARROW ) );
				ShowErrorDialog ( g_hMainWindow, false, Txt ( T_DLG_BT_ERR_INIT_W ), IDD_ERROR_OK );
				return;
			}

			g_pMSBT = new MsBT_c;

			g_eType = SEND_BT_MS;
		}
		else
			return;

//	g_pFindDeviceDlg = new FindDeviceDlg_c;

	int iDlgRes = DialogBox ( ResourceInstance (), MAKEINTRESOURCE ( IDD_FIND_DEVICE ), g_hMainWindow, FindDlgProc );

	if ( iDlgRes != IDOK )
	{
		switch ( g_eType )
		{
		case SEND_BT_WIDCOMM:
			dll_WBT_DestroyStack ();
			break;

		case SEND_BT_MS:
			SafeDelete ( g_pMSBT );
			Shutdown_MsBT ();
			break;
		}

//		SafeDelete ( g_pFindDeviceDlg );
		return;
	}

	// show send dialog in here
//	g_pFileSendProgressDlg = new FileSendProgressDlg_c ( tList, g_pFindDeviceDlg->GetDeviceInfo (), g_pFindDeviceDlg->GetSCN () );

//	SafeDelete ( g_pFindDeviceDlg );

	DialogBox ( ResourceInstance (), MAKEINTRESOURCE ( IDD_SEND_PROGRESS ), g_hMainWindow, SendDlgProc );

	SafeDelete ( g_pFileSendProgressDlg );

	switch ( g_eType )
	{
	case SEND_BT_WIDCOMM:
		dll_WBT_DestroyStack ();
		break;

	case SEND_BT_MS:
		SafeDelete ( g_pMSBT );
		Shutdown_MsBT ();
		break;
	}
}