#include "pch.h"
#include "register.h"

#include "pfm/config.h"
#include "pfm/resources.h"
#include "pfm/gui.h"
#include "pfm/protection.h"
#include "pfm/pfm.h"
#include "pfm/dialogs/errors.h"

#include "aygshell.h"
#include "pfm/resource/resource.h"
#include "Dlls/Resource/resource.h"

extern HINSTANCE g_hAppInstance;
extern HWND g_hMainWindow;

#define RUREG_HEADER1 L"�����������"
#define RUREG_HEADER2 L"������������� �����������."
#define RUREG_TEXT L"�� ������ ���������� �������� �������, ���������� ����� ����� �� ��������:\n\n WebMoney: R143261032891 Z435769869984\n ������.������: 41001137416812"

Str_c GetPurchaseName ()
{
#if FM_BUILD_HANDANGO
	return L"www.handango.com";
#endif
#if FM_BUILD_POCKETGEAR
	return L"www.pocketgear.com";
#endif
#if FM_BUILD_CLICKAPPS
	return L"www.clickapps.com";
#endif
#if FM_BUILD_POCKETLAND
	return L"www.pocketland.de";
#endif
#if FM_BUILD_POCKETSELECT
	return L"www.pocketselect.com";
#endif
#if FM_BUILD_PDATOPSOFT
	return L"www.pdatopsoft.com";
#endif
}

Str_c GetPurchaseLink ()
{
	#if FM_BUILD_HANDANGO
		return L"https://minibrand.handango.com/minibrand/basket.jsp?addItem=203549&siteId=2106&bNav=1&continueUrl=http://www.pocketfilemanager.com";
	#endif
	#if FM_BUILD_POCKETGEAR
		return L"https://www.pocketgear.com/software_detail.asp?id=24263";
	#endif
	#if FM_BUILD_CLICKAPPS
		return L"http://www.clickapps.com/moreinfo.htm?pid=8251&section=PPC";
	#endif
	#if FM_BUILD_POCKETLAND
		return L"http://pocketland.de/product.php?prod_id=39735";
	#endif
	#if FM_BUILD_POCKETSELECT
		return L"http://www.pocketselect.com/product.php?productid=133075";
	#endif
	#if FM_BUILD_PDATOPSOFT
		return L"http://www.pdatopsoft.com/PocketPC/Pocket-File-Manager-1.3";
	#endif
}

//////////////////////////////////////////////////////////////////////////
class AboutDlg_c : public Dialog_Fullscreen_c
{
public:
	AboutDlg_c ()
		: Dialog_Fullscreen_c ( L"Registering", IDM_OK )
		, m_HLink1 ( L"Rinfix.com", L"http://www.rinfix.com" )
		, m_HLink2 ( L"www.pocketfilemanager.com", L"http://www.pocketfilemanager.com" )
		, m_HLink3 ( GetPurchaseName (), GetPurchaseLink () )
		, m_HLink4 ( L"support@pocketfilemanager.com", L"mailto:support@pocketfilemanager.com" )
	{
	}

	virtual void OnInit ()
	{
		Dialog_Fullscreen_c::OnInit ();

		Loc ( IDC_HOMEPAGE_STATIC,	T_DLG_ABOUT_HOME );
		Loc ( IDC_PURCHASE_STATIC,	T_DLG_ABOUT_PURCHASE );
		Loc ( IDC_SUPPORT_STATIC,	T_DLG_ABOUT_SUPPORT );
		Loc ( IDC_REGISTER,			T_DLG_ABOUT_REGISTER );

		Bold ( IDC_COMPANY );

		// application icon
		HICON hIcon = LoadIcon ( g_hAppInstance, MAKEINTRESOURCE ( IDI_MAIN_ICON ) );
		if ( hIcon )
			SendMessage ( Item ( IDC_ICON_APP ), STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon );

		if ( ! protection::rlang )
			Bold ( IDC_VERSION );

		ItemTxt ( IDC_VERSION_NUMBER, FMGetVersion () );

		bool bRegistered = protection::registered;

		Str_c sRegText;
		if ( bRegistered )
		{
			if ( protection::rlang )
			{
				sRegText  = RUREG_HEADER2;
				sRegText += L"\n";
				sRegText += RUREG_TEXT;
			}
			else
				sRegText = Txt ( T_DLG_REGGED );
		}
		else
			sRegText = Txt ( T_DLG_UNREGGED );

		ItemTxt ( IDC_VERSION, sRegText );

		int bHA = bRegistered ? SW_HIDE : SW_SHOW;
		ShowWindow ( Item ( IDC_REGISTER ), bHA );

		m_HLink1.Subclass ( Item ( IDC_LINK ) );
		m_HLink2.Subclass ( Item ( IDC_LINK2 ) );
		m_HLink3.Subclass ( Item ( IDC_LINK3 ) );
		m_HLink4.Subclass ( Item ( IDC_LINK4 ) );
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Fullscreen_c::OnCommand ( iItem, iNotify );

		switch ( iItem )
		{
			case IDC_REGISTER:
				ShowRegisterDialog ( m_hWnd );
			case IDOK:
				Close ( iItem );
				break;
		}
	} 

private:
	Hyperlink_c		m_HLink1;
	Hyperlink_c 	m_HLink2;
	Hyperlink_c 	m_HLink3;
	Hyperlink_c 	m_HLink4;
};

//////////////////////////////////////////////////////////////////////////

class RegisterDlg_c : public Dialog_Fullscreen_c
{
public:
	RegisterDlg_c ()
		: Dialog_Fullscreen_c ( L"Registering", IDM_OK_CANCEL, SHCMBF_EMPTYBAR, true )
		, m_HLink ( GetPurchaseName (), GetPurchaseLink () )
	{
	}

	virtual void OnInit ()
	{
		Dialog_Fullscreen_c::OnInit ();

		Bold ( IDC_DAYS_LEFT );

		Loc ( IDC_PURCHASE_STATIC,	T_DLG_ABOUT_PURCHASE );
		Loc ( IDC_NAG_STATIC,		T_DLG_REG_NAG );
		Loc ( IDC_PRODUCT_KEY_TEXT,	T_DLG_REG_KEY );
		Loc ( IDC_REGISTER,			T_DLG_REG_REGISTER );
		Loc ( IDC_LATER,			T_DLG_REG_LATER );

		Loc ( IDC_LATER,			protection::expired ? T_DLG_REG_EXIT : T_DLG_REG_LATER );

		m_HLink.Subclass ( Item ( IDC_LINK ) );

		Str_c sText;
		if ( protection::expired )
			sText = Txt ( T_DLG_TRIAL_EXPIRED );
		else                      
			sText = NewString ( Txt ( T_DLG_TRIAL_LEFT ), protection::days_left );

		ItemTxt ( IDC_DAYS_LEFT, sText );

		// notify that the nag has been shown
		protection::nag_shown = true;
	}

	virtual void OnCommand ( int iItem, int iNotify )
	{
		Dialog_Fullscreen_c::OnCommand ( iItem, iNotify );

		switch ( iItem )
		{
		case IDC_REGISTER:
			if ( Register () )
				PostQuitMessage ( 0 );
			break;

		case IDC_LATER:
			if ( protection::expired )
				PostQuitMessage ( 0 );
			else
				Close ( iItem );
			break;
		}
	}


private:
	Hyperlink_c 	m_HLink;

	bool Register ()
	{
		Str_c sKey = GetItemTxt ( IDC_KEY_CODE );
		if ( ! sKey.Empty () )
		{
			protection::key = sKey;
			if ( protection::CheckRus () )
				MessageBox ( m_hWnd, RUREG_TEXT, RUREG_HEADER1, MB_OK );

			MessageBox ( m_hWnd, Txt ( T_DLG_REG_RESTART ), Txt ( T_MSG_ATTENTION ), MB_OK | MB_ICONINFORMATION );
			return true;
		}

		return false;
	}
};

void ShowAboutDialog ()
{
	AboutDlg_c Dlg;
	Dlg.Run ( IDD_ABOUT, g_hMainWindow );
}

void ShowRegisterDialog ( HWND hParent )
{
	RegisterDlg_c Dlg;
	Dlg.Run ( IDD_REGISTER, hParent );
}