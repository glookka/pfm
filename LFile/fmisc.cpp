#include "pch.h"

#include "LCore/clog.h"
#include "LCore/cfile.h"
#include "LCore/ctimer.h"
#include "LFile/ferrors.h"
#include "LFile/fmisc.h"
#include "LFile/fmkdir.h"
#include "LSettings/slocal.h"

#include "Resources/resource.h"
#include "shlobj.h"

extern HWND g_hMainWindow;

static DWORD g_dwValidAttributes = FILE_ATTRIBUTE_ARCHIVE
								| FILE_ATTRIBUTE_ARCHIVE
								| FILE_ATTRIBUTE_HIDDEN
								| FILE_ATTRIBUTE_NORMAL
								| FILE_ATTRIBUTE_READONLY
								| FILE_ATTRIBUTE_SYSTEM
								| FILE_ATTRIBUTE_TEMPORARY;


DWORD g_dAttributes [] = 
{
	 FILE_ATTRIBUTE_ARCHIVE
	,FILE_ATTRIBUTE_READONLY
	,FILE_ATTRIBUTE_HIDDEN
	,FILE_ATTRIBUTE_SYSTEM
	,FILE_ATTRIBUTE_INROM
};

const double SLOW_OPERATION_TIME = 1.0;


const wchar_t * GetMaxByteString ()
{
	return Txt ( T_MAX_BYTE_STRING );
}


const wchar_t * GetMaxKByteString ()
{
	return Txt ( T_MAX_KBYTE_STRING );
}


const wchar_t * GetMaxMByteString ()
{
	return Txt ( T_MAX_MBYTE_STRING );
}


const wchar_t * FileSizeToString ( DWORD uSizeHigh, DWORD uSizeLow, wchar_t * szSize, bool bShowBytes )
{
	if ( uSizeHigh )
	{
		ULARGE_INTEGER uSize;
		uSize.HighPart = uSizeHigh;
		uSize.LowPart = uSizeLow;
		float fSize = float ( uSize.QuadPart >> 20 ) / 1024.0f;

		// > 3G -> 'g'
		wsprintf ( szSize, Txt ( T_SIZE_GBYTES ), fSize );
	}
	else
	{
		if ( uSizeLow < 524288UL ) // 0 <= size < 512k -> ''
			wsprintf ( szSize, bShowBytes ? Txt ( T_SIZE_BYTES ) : L"%d", uSizeLow );
		else
			if ( uSizeLow < 5242880UL )	// 512k <= size < 5M -> 'k'
				wsprintf ( szSize, Txt ( T_SIZE_KBYTES ), uSizeLow >> 10 );
			else
				if ( uSizeLow < 1073741824UL ) // 5M <= size < 1G -> 'm'
				{
					float fSize = float ( uSizeLow >> 10 );
					wsprintf ( szSize, Txt ( T_SIZE_MBYTES ), fSize / 1024.0f );
				}
				else // 1G <= size < 3G -> 'g'
				{
					float fSize = float ( uSizeLow >> 20 );
					wsprintf ( szSize, Txt ( T_SIZE_GBYTES ), fSize / 1024.0f );
				}
	}

	return szSize;
}


const wchar_t * FileSizeToString ( const ULARGE_INTEGER & uLargeSize, wchar_t * szSize, bool bShowBytes )
{
	return FileSizeToString ( uLargeSize.HighPart, uLargeSize.LowPart, szSize, bShowBytes );
}


const wchar_t * FileSizeToStringByte ( const ULARGE_INTEGER & uLargeSize, wchar_t * szSize )
{
	ULARGE_INTEGER uSize = uLargeSize;
	ULARGE_INTEGER uTmp = uLargeSize;
	ULARGE_INTEGER uDivisor;
	uDivisor.QuadPart = 1;

	while ( uTmp.QuadPart >= 1000 )
	{
		uTmp.QuadPart /= 1000;
		uDivisor.QuadPart *= 1000;
	}

	*szSize = L'\0';
	wchar_t szBuf [16];

	bool bFirstPass = true;
	while ( uDivisor.QuadPart )
	{
		int iPart = int ( ( uSize.QuadPart / uDivisor.QuadPart ) % 1000 );
		uDivisor.QuadPart /= 1000;

		if ( bFirstPass )
		{
			wsprintf ( szBuf, L"%d", iPart );
			bFirstPass = false;
		}
		else
			wsprintf ( szBuf, L"%03d", iPart );

		wcscat ( szSize, szBuf );

		if ( uDivisor.QuadPart )
			wcscat ( szSize, L" " );
	}

	return szSize;
}


void AlignFileName ( HWND hWnd, const Str_c & sText )
{
	if ( ! hWnd )
		return;

	HDC hDC = GetDC ( hWnd );
	HFONT hFont = (HFONT) SendMessage ( hWnd, WM_GETFONT, 0, 0 );
	SelectObject ( hDC, hFont );

	RECT tTextRect = { 0, 0, 0, 0 }, tWindowRect;

	GetWindowRect ( hWnd, &tWindowRect );
	DrawText ( hDC, sText, sText.Length (), &tTextRect, DT_LEFT | DT_TOP | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX );

	if ( tTextRect.right - tTextRect.left > tWindowRect.right - tWindowRect.left )
	{
		TEXTMETRIC tMetric;
		GetTextMetrics ( hDC, &tMetric );

		int nCharsToFit = ( tWindowRect.right - tWindowRect.left ) / tMetric.tmAveCharWidth;
		
		int iStartChar = sText.Length () - nCharsToFit;
		if ( iStartChar < 0 )
			iStartChar = 0;

		Str_c sTextToDraw = Str_c ( L"..." ) + sText.SubStr ( iStartChar, nCharsToFit );
	
		while ( ( sTextToDraw.Length () > 3 ) && ( tTextRect.right - tTextRect.left >= tWindowRect.right - tWindowRect.left ) )
		{
			sTextToDraw.Erase ( 3, 1 );
			DrawText ( hDC, sTextToDraw, sTextToDraw.Length (), &tTextRect, DT_LEFT | DT_TOP | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX );
		}

		SetWindowText ( hWnd, sTextToDraw );
	}
	else
		SetWindowText ( hWnd, sText );
}


void AlignFileName ( HDC hDC, const RECT & tRect, const Str_c & sText )
{
	RECT tTextRect = { 0, 0, 0, 0 };
	RECT tDrawRect = tRect;

	DrawText ( hDC, sText, sText.Length (), &tTextRect, DT_LEFT | DT_TOP | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX );
	
	if ( tTextRect.right - tTextRect.left > tRect.right - tRect.left )
	{
		TEXTMETRIC tMetric;
		GetTextMetrics ( hDC, &tMetric );

		int nCharsToFit = ( tRect.right - tRect.left ) / tMetric.tmAveCharWidth;

		int iStartChar = sText.Length () - nCharsToFit;
		if ( iStartChar < 0 )
			iStartChar = 0;

		Str_c sTextToDraw = Str_c ( L"..." ) + sText.SubStr ( iStartChar, nCharsToFit );
	
		while ( ( sTextToDraw.Length () > 3 ) && ( tTextRect.right - tTextRect.left > tRect.right - tRect.left ) )
		{
			sTextToDraw.Erase ( 3, 2 );
			DrawText ( hDC, sTextToDraw, sTextToDraw.Length (), &tTextRect, DT_LEFT | DT_TOP | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX );
		}

		DrawText ( hDC, sTextToDraw, sTextToDraw.Length (), &tDrawRect, DT_LEFT | DT_TOP | DT_NOCLIP | DT_NOPREFIX );
	}
	else
		DrawText ( hDC, sText, sText.Length (), &tDrawRect, DT_LEFT | DT_TOP | DT_NOCLIP | DT_NOPREFIX );
}


void CalcTotalSize ( FileList_t & tList, ULARGE_INTEGER & uSize, int & nFiles, int & nFolders, DWORD & dwIncAttrib, DWORD & dwExAttrib, SlowOperationCallback_t fnCallback )
{
	nFiles = 0;
	nFolders = 0;

	uSize.QuadPart = 0;

	dwIncAttrib = dwExAttrib = 0;

	double tTime = g_Timer.GetTimeSec ();
	bool bSignalled = false;

	FileIteratorPanel_c tIterator;
	tIterator.IterateStart ( tList );

	while ( tIterator.IterateNext () )
	{
		if ( fnCallback && !bSignalled && ( g_Timer.GetTimeSec () - tTime >= SLOW_OPERATION_TIME ) )
		{
			fnCallback ();
			bSignalled = true;
		}

		if ( ! tIterator.Is2ndPassDir () )
		{
			const WIN32_FIND_DATA * pFindData = tIterator.GetData ();
			if ( pFindData )
			{
				dwIncAttrib |= pFindData->dwFileAttributes;
				dwExAttrib |= ~pFindData->dwFileAttributes;
				if  ( ! ( pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
				{
					ULARGE_INTEGER uFileSize;
					uFileSize.HighPart = pFindData->nFileSizeHigh;
					uFileSize.LowPart = pFindData->nFileSizeLow;
					uSize.QuadPart += uFileSize.QuadPart;

					++nFiles;
				}
				else
					++nFolders;
			}
		}
	}
}


static OperationResult_e TryToSetAttributes ( HWND hWnd, const Str_c & sFileName, const WIN32_FIND_DATA * pData, const FilePropertyCommand_t & tCommand )
{
	DWORD dwAttribToSet = pData->dwFileAttributes;
	Str_c sName ( sFileName );
	HWND m_hWnd = hWnd;
	BOOL bRes;

	// combine source attributes with our masks
	dwAttribToSet = ( dwAttribToSet | tCommand.m_dwIncAttrib ) & ( ~tCommand.m_dwExAttrib );
	dwAttribToSet &= g_dwValidAttributes;

	if ( tCommand.m_dwFlags & FILE_PROPERTY_ATTRIBUTES )
		TRY_FUNC ( bRes, SetFileAttributes ( sName, dwAttribToSet ), FALSE, sName, L"" );

	if ( tCommand.m_dwFlags & FILE_PROPERTY_TIME && ! ( pData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
	{
		HANDLE hFile = NULL;

		TRY_FUNC ( hFile, CreateFile ( sName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ), INVALID_HANDLE_VALUE, sFileName, L"" );

		FILETIME tTimeCreate, tTimeAccess, tTimeWrite;
		SystemTimeToFileTime ( &tCommand.m_tTimeCreate, &tTimeCreate );
		LocalFileTimeToFileTime ( &tTimeCreate, &tTimeCreate );

		SystemTimeToFileTime ( &tCommand.m_tTimeAccess, &tTimeAccess );
		LocalFileTimeToFileTime ( &tTimeAccess, &tTimeAccess );

		SystemTimeToFileTime ( &tCommand.m_tTimeWrite, &tTimeWrite );
		LocalFileTimeToFileTime ( &tTimeWrite, &tTimeWrite );

		TRY_FUNC ( bRes, SetFileTime ( hFile, &tTimeCreate, &tTimeAccess, &tTimeWrite ), FALSE, sFileName, L"" );
		CloseHandle ( hFile );
	}

	return RES_OK;
}


void FileChangeProperties ( HWND hWnd, FileList_t & tList, const FilePropertyCommand_t & tCommand, SlowOperationCallback_t fnCallback )
{
	FileIteratorPanel_c tIterator;
	tIterator.IterateStart ( tList, !! ( tCommand.m_dwFlags & FILE_PROPERTY_RECURSIVE ) );

	g_tErrorList.Reset ( OM_PROPS );

	bool bSignalled = false;

	double tTime = g_Timer.GetTimeSec ();
	while ( tIterator.IterateNext () )
	{
		if ( fnCallback && !bSignalled && ( g_Timer.GetTimeSec () - tTime >= SLOW_OPERATION_TIME ) )
		{
			fnCallback ();
			bSignalled = true;
		}

		if ( ! tIterator.Is2ndPassDir () )
		{
			const WIN32_FIND_DATA * pFindData = tIterator.GetData ();
			if ( pFindData )
			{
				OperationResult_e eRes = TryToSetAttributes ( hWnd, tList.m_sRootDir + tIterator.GetFileName (), pFindData, tCommand );
	 			if ( eRes == RES_CANCEL )
					break;	
			}
		}
	}
}

void PrepareDestDir ( Str_c & sDest, const Str_c & sRootDir )
{
	ReplaceSlashes( sDest );

	// dest may be not fully entered. try to append
	if ( sDest.Find ( L'\\' ) != 0 )
	{
		Str_c sTmp ( sRootDir );
		AppendSlash ( sTmp );
		sDest = sTmp + sDest;
	}
}

static OperationResult_e TryToCreateShortcut ( const Str_c & sDest, const Str_c & sParams )
{
	MakeDirectory ( GetPath ( sDest ) );

	g_tErrorList.Reset ( OM_SHORTCUT );
	
	BOOL bRes = FALSE;
	HWND m_hWnd = g_hMainWindow;

	// try to find destination
	WIN32_FIND_DATA tFindData;
	HANDLE hFound = FindFirstFile ( sDest, &tFindData );
	bool bDestExists = hFound != INVALID_HANDLE_VALUE;
	if ( bDestExists )
	{
		FindClose ( hFound );

		int iErrAction = g_tErrorList.ShowErrorDialog ( m_hWnd, EC_DEST_EXISTS, false, sDest, L"" );

		if ( iErrAction != IDC_OVER )
			return iErrAction == IDCANCEL ? RES_CANCEL : RES_ERROR;

		DWORD dwDestAttrib;

		// get dest attributes
		TRY_FUNC ( dwDestAttrib, GetFileAttributes ( sDest ), 0xFFFFFFFF, sDest, L"" );

		DWORD dwAttribToSet = dwDestAttrib;
		dwAttribToSet &= ~ ( FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM );
		TRY_FUNC ( bRes, SetFileAttributes ( sDest, dwAttribToSet ), FALSE, sDest, L"" );

		TRY_FUNC ( bRes, DeleteFile ( sDest ), FALSE, sDest, L"" );
	}

	wchar_t * szDest = (wchar_t *) sDest.c_str ();
	wchar_t * szParams = (wchar_t *) sParams.c_str ();

	TRY_FUNC ( bRes, (BOOL)SHCreateShortcut ( szDest, szParams ), FALSE, sDest, L"" );

	return RES_OK;
}

bool CreateShortcut ( const Str_c & sDest, const Str_c & sParams )
{
	OperationResult_e eRes = TryToCreateShortcut ( sDest, sParams );
	return eRes != RES_ERROR && eRes != RES_CANCEL;
}

bool HasSubDirs ( const Str_c & sPath )
{
	Str_c sMask = sPath;
	AppendSlash ( sMask );
	sMask += L"*";

	WIN32_FIND_DATA tFindData;
	HANDLE hFound = FindFirstFile ( sMask, &tFindData );
	bool bSearching = hFound != INVALID_HANDLE_VALUE;
	bool bHasDirs = false;
	
	while ( bSearching && ! bHasDirs )
	{
		if ( tFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			bHasDirs = true;

		bSearching = !!FindNextFile ( hFound, &tFindData );
	}

	FindClose ( hFound );
	
	return bHasDirs;
}