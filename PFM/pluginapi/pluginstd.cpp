#include "pluginstd.h"

extern PluginStartupInfo_t g_PSI;
extern HINSTANCE g_hInstance;

static wchar_t * g_szEmptyString = L"";

#ifdef _DEBUG
void Warning ( const wchar_t * szFormat, ... )
{
	static wchar_t szStr [256];
	va_list argptr;

	va_start ( argptr, szFormat );
	vswprintf ( szStr, szFormat, argptr );
	va_end	 ( argptr );

	MessageBox ( NULL, szStr, L"Warning!", MB_OK );
}
#endif

const wchar_t *	LocalStr ( int iString )
{
	int nStrings = g_PSI.m_nStrings;
	if ( iString < 0 || iString >= nStrings )
		return g_szEmptyString;

	return g_PSI.m_dStrings [iString];

}

void DlgTxt ( HWND hDlg, int iControl, int iString )
{
	SetDlgItemText ( hDlg, iControl, LocalStr ( iString ) );
}

const wchar_t * Txt ( int iString )
{
	return LocalStr ( iString );
}

void SetToolbarText ( HWND hToolbar, int iId, const wchar_t * szText )
{
	if ( ! hToolbar )
		return;

	TBBUTTONINFO tBtnInfo;
	tBtnInfo.cbSize		= sizeof ( tBtnInfo );
	tBtnInfo.dwMask		= TBIF_TEXT;
	tBtnInfo.pszText	= (wchar_t *)szText;
	SendMessage ( hToolbar, TB_SETBUTTONINFO, iId, (LPARAM) &tBtnInfo );
}

int GetColumnWidthRelative ( float fWidth )
{
	int iMul = g_PSI.m_bVGA ? 2 : 1;
	return int ( float ( GetDeviceCaps ( GetDC ( NULL ), HORZRES ) - 20 * iMul ) * fWidth );
}


void AnsiToUnicode ( const char * szStr, wchar_t * szResult )
{
	MultiByteToWideChar ( CP_ACP, 0, szStr, -1, szResult, 1024 );
}