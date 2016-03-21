#include "pch.h"

#include "LSettings/slocal.h"
#include "LCore/clog.h"
#include "LCore/carray.h"
#include "LParser/pparser.h"

static Array_T < wchar_t * > g_dStrings;
static HMODULE g_hResourceModule = NULL;

const wchar_t * Txt ( int iString )
{
	return g_dStrings [iString];
}

void DlgTxt ( HWND hDlg, int iItemId, int iString )
{
	SetDlgItemText ( hDlg, iItemId, Txt ( iString ) );
}

void CopyConvert ( const wchar_t * szSource, wchar_t * szDest, int nChars )
{
	int iSource = 0;
	int iDest  = 0;
	while ( iSource < nChars )
	{
		if ( szSource [iSource] == L'\\' && szSource [iSource+1] == L'n' )
		{
			szDest [iDest] = L'\n';
			iSource += 2;
		}
		else
		{
			szDest [iDest] = szSource [iSource];
			++iSource;
		}

		++iDest;
	}

	szDest [iDest] = L'\0';
}

bool Init_Lang ( const wchar_t * szFileName )
{
	const int MAX_BUFFER_SIZE = 102400;

	HANDLE hFile = CreateFile ( szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
	if ( hFile == INVALID_HANDLE_VALUE )
		return false;

	ULARGE_INTEGER uSize;
	uSize.LowPart = GetFileSize ( hFile, &uSize.HighPart );

	if ( uSize.LowPart == 0xFFFFFFFF )
		return false;

	int iSize = (int) uSize.QuadPart;

	if ( iSize > MAX_BUFFER_SIZE )
		return false;

	if ( ( iSize % 2 ) != 0  )
		return false;

	int iBufferSize = iSize / 2;
	wchar_t * pBuffer = new wchar_t [iBufferSize + 1];

	DWORD uSizeRead = 0;
	ReadFile ( hFile, pBuffer, iSize, &uSizeRead, NULL );
	CloseHandle ( hFile );

	if ( uSizeRead != iSize )
	{
		SafeDeleteArray ( pBuffer );
		return false;
	}

	pBuffer [iBufferSize] = L'\0';

	wchar_t * pStart = pBuffer;
	while ( *pStart )
	{
		while ( *pStart && _X_iswspace ( *pStart ) )
			++pStart;

		wchar_t * pLineStart = pStart;
		while ( *pStart && *pStart != L'\n' )
			++pStart;

		int nChars = pStart - pLineStart - ( *pStart ? 1 : 0 );
		wchar_t * szAllocated = new wchar_t [nChars + 1];
		CopyConvert ( pLineStart, szAllocated, nChars );
		g_dStrings.Add ( szAllocated );
	}

	SafeDeleteArray ( pBuffer );

	if ( g_dStrings.Length () != T_TOTAL )
		return false;

	return true;
}

void Shutdown_Lang ()
{
	for ( int i = 0; i < g_dStrings.Length (); ++i )
		SafeDeleteArray ( g_dStrings [i] );

	g_dStrings.Clear ();
}

bool Init_Resources ( const wchar_t * szFileName )
{
	g_hResourceModule = LoadLibraryEx ( szFileName, NULL, LOAD_LIBRARY_AS_DATAFILE );
	return !!g_hResourceModule;
}

void Shutdown_Resources ()
{
	if ( g_hResourceModule )
		::FreeLibrary ( g_hResourceModule );
}

HMODULE ResourceInstance ()
{
	return g_hResourceModule;
}