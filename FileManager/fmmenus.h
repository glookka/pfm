#ifndef _fmmenus_
#define _fmmenus_

#include "LCore/cmain.h"
#include "LCore/cstr.h"

namespace menu
{
	bool OnCommand ( int iCommand );
	void OnNotify ( int iControlId, NMHDR * pInfo );

	bool OnInitMenuPopup ( HMENU hMenu );
	void OnEnterMenu ();
	void OnExitMenu ();

	bool IsInMenu ();

	void RecreateMenuBar ();
	void ShowSecondToolbar ( bool bShow );
	void UpdateToolbar ();
	bool Is2ndBarVisible ();
	void HideWM5Toolbar ();
	bool IsToolbarHidden ();
	void GetToolbarRect ( RECT & tRect );
	void CheckMarkMode ( bool bMark );
	void CheckFastNav ( bool bFastNav );

	bool HandleMiscMenu ( int iCommand );
	bool HandleCryptMenu ( int iCommand );
	bool HandleSelectionMenu ( int iCommand );
	bool HandleSendMenu ( int iCommand );

	HMENU CreateSendMenu ();

	bool Init ();
	void Shutdown ();
}

#endif