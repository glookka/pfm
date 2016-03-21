#ifndef _slocal_
#define _slocal_

#include "LCore/cmain.h"

enum LocString_e
{
#include "slocal.inc"
	T_TOTAL
};

const wchar_t * Txt ( int iString );
void DlgTxt ( HWND hDlg, int iItemId, int iString );

bool Init_Lang ( const wchar_t * szFileName );
void Shutdown_Lang ();

HMODULE ResourceInstance ();

bool Init_Resources ( const wchar_t * szFileName );
void Shutdown_Resources ();

#endif