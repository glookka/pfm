#ifndef _LSettings_buttons_
#define _LSettings_buttons_

#include "LCore/cmain.h"
#include "LCore/cbuttons.h"
#include "LParser/pvar_loader.h"

enum BtnAction_e
{
	 BA_NONE = -1
	,BA_EXIT = 0
	,BA_MINIMIZE
	,BA_MOVEUP
	,BA_MOVEDOWN
	,BA_MOVELEFT
	,BA_MOVERIGHT
	,BA_MOVEHOME
	,BA_MOVEEND
	,BA_PREVDIR
	,BA_EXECUTE
	,BA_OPENWITH
	,BA_SEND_BT
	,BA_SEND_IR
	,BA_OPEN_IN_OPPOSITE
	,BA_SAME_AS_OPPOSITE
	,BA_SWITCH_PANES
	,BA_MAXIMIZE_PANE
	,BA_REFRESH_PANES
	,BA_DRIVE_LIST
	,BA_FILE_MENU
	,BA_HEADER_MENU
	,BA_TGL_FULLSCREEN
	,BA_TGL_FASTSEL
	,BA_TGL_FASTNAV
	,BA_TGL_2NDBAR
	,BA_OP_VIEW
	,BA_OP_COPYMOVE
	,BA_OP_RENAME
	,BA_OP_DELETE
	,BA_OP_SHORTCUT
	,BA_OP_MKDIR
	,BA_OP_CLIPCOPY
	,BA_OP_CLIPCUT
	,BA_OP_CLIPPASTE
	,BA_OP_ENCRYPT
	,BA_OP_DECRYPT
	,BA_OP_PROPS
	,BA_OP_SEARCH
	,BA_FL_SELECT_ALL
	,BA_FL_SELECT_NONE
	,BA_FL_INVERT
	,BA_FL_SELECT_FILTER
	,BA_FL_CLEAR_FILTER
	,BA_FL_TOGGLE
	,BA_FL_TOGGLE_AND_MOVE
	,BA_BM_ADD
	,BA_OPTIONS
	,BA_TOTAL
};

class Buttons_c
{
public:
					Buttons_c ();

	void			Load ( const Str_c & sFileName );
	void			Save ();

	void			Init ();
	void			Shutdown ();

	void			SetButton ( BtnAction_e eAction, int iBtnIndex, int iKey, bool bLong );
	void			SetLong ( BtnAction_e eAction, int iBtnIndex, bool bLong );

	const wchar_t *	GetLitName ( BtnAction_e eAction ) const;
	int				GetKey ( BtnAction_e eAction, int iBtnIndex ) const;
	bool			GetLong ( BtnAction_e eAction, int iBtnIndex ) const;

	Str_c			GetKeyLitName ( int iKey ) const;

	BtnAction_e		GetAction ( int iKey, btns::Event_e eEvent );

	void			RegisterHotKeys ( HWND hWnd );
	void			UnregisterHotKeys ( HWND hWnd );

private:
	bool			m_bChangeFlag;
	Str_c			m_sFileName;

	struct ConfigBtn_c
	{
		int		m_iKey;
		bool	m_bLong;
	};

	static const int BTNS_PER_ACTION = 3;

	ConfigBtn_c		m_dCfgBtns [BA_TOTAL][BTNS_PER_ACTION];
};

extern Buttons_c g_tButtons;

#endif