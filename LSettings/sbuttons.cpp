#include "pch.h"

#include "LSettings/sbuttons.h"
#include "LCore/cos.h"
#include "LParser/pcommander.h"
#include "LSettings/slocal.h"

#include "winuserm.h"

Buttons_c g_tButtons;

CMD_BEGIN ( btn, 7 )
{
	BtnAction_e eAction = (BtnAction_e) tArgs.GetInt ( 0 );
	g_tButtons.SetButton ( eAction, 0, tArgs.GetInt ( 1 ), tArgs.GetInt ( 2 ) != 0 );
	g_tButtons.SetButton ( eAction, 1, tArgs.GetInt ( 3 ), tArgs.GetInt ( 4 ) != 0 );
	g_tButtons.SetButton ( eAction, 2, tArgs.GetInt ( 5 ), tArgs.GetInt ( 6 ) != 0 );
}
CMD_END


///////////////////////////////////////////////////////////////
// colors
Buttons_c::Buttons_c ()
	: m_bChangeFlag	( false )
{
	Assert ( T_BTN_NAME_LAST - T_BTN_NAME_1 + 1 == BA_TOTAL );

	for ( int i = 0; i < BA_TOTAL; ++i )
		for ( int j = 0; j < BTNS_PER_ACTION; ++j )
	{
		m_dCfgBtns [i][j].m_iKey = -1;
		m_dCfgBtns [i][j].m_bLong = false;
	}
}

void Buttons_c::Load ( const Str_c & sFileName )
{
	Commander_c::Get ()->ExecuteFile ( sFileName );
	m_sFileName = sFileName;

	m_bChangeFlag = false;
}

void Buttons_c::Save ()
{
	if ( ! m_bChangeFlag )
		return;

	FILE * pFile = _wfopen ( m_sFileName, L"wb" );
	if ( ! pFile )
	{
		Log ( L"Can't save buttons to [%s]", m_sFileName.c_str () );
		return;
	}

	unsigned short uSign = 0xFEFF;
	fwrite ( &uSign, sizeof ( uSign ), 1, pFile );

	for ( int i = 0; i < BA_TOTAL; ++i )
		fwprintf ( pFile, L"btn %d %d %d %d %d %d %d;\n", i
			,m_dCfgBtns [i][0].m_iKey, m_dCfgBtns [i][0].m_bLong ? 1 : 0
			,m_dCfgBtns [i][1].m_iKey, m_dCfgBtns [i][1].m_bLong ? 1 : 0
			,m_dCfgBtns [i][2].m_iKey, m_dCfgBtns [i][2].m_bLong ? 1 : 0 );

	fclose ( pFile );
}

void Buttons_c::Init ()
{
	CMD_REG ( btn );
}

void Buttons_c::Shutdown ()
{
}

void Buttons_c::SetButton ( BtnAction_e eAction, int iBtnIndex, int iKey, bool bLong )
{
	if ( eAction < 0 || eAction >= BA_TOTAL || iBtnIndex < 0 || iBtnIndex >= BTNS_PER_ACTION )
		return;

	m_dCfgBtns [eAction][iBtnIndex].m_iKey = iKey;
	m_dCfgBtns [eAction][iBtnIndex].m_bLong = bLong;

	m_bChangeFlag = true;
}

void Buttons_c::SetLong ( BtnAction_e eAction, int iBtnIndex, bool bLong )
{
	if ( eAction < 0 || eAction >= BA_TOTAL || iBtnIndex < 0 || iBtnIndex >= BTNS_PER_ACTION )
		return;
	
	m_dCfgBtns [eAction][iBtnIndex].m_bLong = bLong;

	m_bChangeFlag = true;
}

const wchar_t *	Buttons_c::GetLitName ( BtnAction_e eAction ) const
{
	if ( eAction < 0 || eAction >= BA_TOTAL )
		return NULL;

	return Txt ( eAction + T_BTN_NAME_1 );
}

int	Buttons_c::GetKey ( BtnAction_e eAction, int iBtnIndex ) const
{
	if ( eAction < 0 || eAction >= BA_TOTAL || iBtnIndex < 0 || iBtnIndex >= BTNS_PER_ACTION )
		return -1;

	return m_dCfgBtns [eAction][iBtnIndex].m_iKey;
}

bool Buttons_c::GetLong ( BtnAction_e eAction, int iBtnIndex ) const
{
	if ( eAction < 0 || eAction >= BA_TOTAL || iBtnIndex < 0 || iBtnIndex >= BTNS_PER_ACTION )
		return false;

	return m_dCfgBtns [eAction][iBtnIndex].m_bLong;
}

#define VK_TSPEAKERPHONE_TOGGLE VK_F16
#define VK_VOICEDIAL			VK_F24
#define VK_KEYLOCK				VK_F22

Str_c Buttons_c::GetKeyLitName ( int iKey ) const
{
	if ( iKey == -1 )
		return Txt ( T_DLG_BTN_NOT_ASSIGNED );

	switch ( iKey )
	{
	case VK_TSOFT1:			return L"Soft key 1";
	case VK_TSOFT2:			return L"Soft key 2";
	case VK_TTALK:			return L"Talk";
	case VK_TEND:			return L"End";
	case VK_THOME:			return L"Home";
	case VK_TBACK:			return L"Back";
	case VK_TRECORD:		return L"Record";
	case VK_TFLIP:			return L"Flip";
	case VK_TPOWER:			return L"Power";
	case VK_TVOLUMEUP:		return L"Volume up";
	case VK_TVOLUMEDOWN:	return L"Volume down";
	case VK_TSPEAKERPHONE_TOGGLE: return L"Speakerphone toggle";
	case VK_TUP:			return L"Up";
	case VK_TDOWN:			return L"Down";
	case VK_TLEFT:			return L"Left";
	case VK_TRIGHT:			return L"Right";
	case VK_T0:				return L"0";
	case VK_T1:				return L"1";
	case VK_T2:				return L"2";
	case VK_T3:				return L"3";
	case VK_T4:				return L"4";
	case VK_T5:				return L"5";
	case VK_T6:				return L"6";
	case VK_T7:				return L"7";
	case VK_T8:				return L"8";
	case VK_T9:				return L"9";
	case VK_TSTAR:			return L"*";
	case VK_TPOUND:			return L"#";
	case VK_SYMBOL:			return L"SYM";
	case VK_ACTION:			return L"Action";
	case VK_VOICEDIAL:		return L"Voice dial";
	case VK_KEYLOCK:		return L"Key lock";
	case VK_APP1:			return L"Application 1";
	case VK_APP2:			return L"Application 2";
	case VK_APP3:			return L"Application 3";
	case VK_APP4:			return L"Application 4";
	case VK_APP5:			return L"Application 5";
	case VK_APP6:			return L"Application 6";
	}

	return NewString ( L"Button #%d", iKey );
}

BtnAction_e	Buttons_c::GetAction ( int iKey, btns::Event_e eEvent )
{
	if ( eEvent == btns::EVENT_NONE )
		return BA_NONE;

	bool bLongEvent = eEvent == btns::EVENT_LONGPRESS;

	for ( int i = 0; i < BA_TOTAL; ++i )
		for ( int j = 0; j < BTNS_PER_ACTION; ++j )
			if ( m_dCfgBtns [i][j].m_iKey == iKey && m_dCfgBtns [i][j].m_bLong == bLongEvent )
				return BtnAction_e ( i );

	return BA_NONE;
}

static bool ShouldRegisterKey ( int iKey )
{
	return iKey >= 0 && iKey != VK_TSOFT1 && iKey != VK_TSOFT2;
}

void Buttons_c::RegisterHotKeys ( HWND hWnd )
{
	for ( int i = 0; i < BA_TOTAL; ++i )
		for ( int j = 0; j < BTNS_PER_ACTION; ++j )
		{
			int iKey = m_dCfgBtns [i][j].m_iKey;
			if ( ShouldRegisterKey ( iKey ) )
			{
				UndergisterFunc ( MOD_WIN, iKey );
				RegisterHotKey ( hWnd, iKey, MOD_WIN, iKey );
			}
		}
}

void Buttons_c::UnregisterHotKeys ( HWND hWnd )
{
	for ( int i = 0; i < BA_TOTAL; ++i )
		for ( int j = 0; j < BTNS_PER_ACTION; ++j )
		{
			int iKey = m_dCfgBtns [i][j].m_iKey;
			if ( ShouldRegisterKey ( iKey ) )
				UnregisterHotKey ( hWnd, iKey );
		}
}