#include "pch.h"

#include "filemisc.h"
//#include "LFile/fmkdir.h"
#include "config.h"
#include "dialogs/errors.h"

#include "Dlls/Resource/resource.h"
#include "shlobj.h"
#include "projects.h"

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

ErrorHandlers_t g_ErrorHandlers;

const double SLOW_OPERATION_TIME = 1.0;


bool AreFilesOnDifferentVolumes ( const Str_c & sFile1, const Str_c & sFile2 )
{
	Str_c sFileToTest1 = sFile1;
	sFileToTest1.ToLower ();

	Str_c sFileToTest2 = sFile2;
	sFileToTest2.ToLower ();
	
	WIN32_FIND_DATA tData;
	HANDLE hCard = FindFirstFlashCard ( &tData );
	int iFile1Card = -1;
	int iFile2Card = -1;
	int iCard = 0;

	wchar_t szFileNameToTest [ MAX_PATH ];
	if ( hCard != INVALID_HANDLE_VALUE )
	{
		do 
		{
			wcscpy ( szFileNameToTest, tData.cFileName );
			_wcslwr ( szFileNameToTest );

			int iFindRes1 = sFileToTest1.Find ( szFileNameToTest );
			if ( iFile1Card == -1 && iFindRes1 != -1 && iFindRes1 <= 1 )
				iFile1Card = iCard;

			int iFindRes2 = sFileToTest2.Find ( szFileNameToTest );
			if ( iFile2Card == -1 && iFindRes2 != -1 && iFindRes2 <= 1 )
				iFile2Card = iCard;

			++iCard;
		}
		while ( FindNextFlashCard ( hCard, &tData ) && ( iFile1Card == -1 || iFile2Card == -1 ) );
	}

	return iFile1Card != iFile2Card;
}


bool DecomposeLnk ( Str_c & sPath, Str_c & sParams, bool & bDir )
{
	wchar_t szTarget [MAX_PATH];
	if ( ! SHGetShortcutTarget ( sPath, szTarget, MAX_PATH ) )
		return false;

	sPath = szTarget;
	sPath.RTrim ();

	if ( sPath.Begins ( L"\"" ) )
	{
		int iPos = sPath.Find ( L'\"', 1 );
		if ( iPos == -1 )
			return false;

		if ( iPos + 1 < sPath.Length () )
			sParams = sPath.SubStr ( iPos + 1 );

		sPath.Chop ( sPath.Length () - iPos - 1 );
	}
	else
	{
		int iPos = sPath.Find ( L' ' );
		if ( iPos != -1 )
		{
			sParams = sPath.SubStr ( iPos );
			sPath.Chop ( sPath.Length () - iPos );
		}
	}

	sParams.LTrim ();

	sPath.Strip ( L'\"' );
	DWORD dwAttributes = GetFileAttributes ( sPath );
	if ( dwAttributes == 0xFFFFFFFF )
	{
		bDir = false;
		return true;
	}

	bDir = !!( dwAttributes & FILE_ATTRIBUTE_DIRECTORY );
	return true;
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
		wsprintf ( szSize, L"%.1f ", fSize );
		wcscat ( szSize, Txt ( T_SIZE_GBYTES ) );
	}
	else
	{
		if ( uSizeLow < 524288UL ) // 0 <= size < 512k -> ''
		{
			wsprintf ( szSize, bShowBytes ? L"%d " : L"%d", uSizeLow );
			if ( bShowBytes )
				wcscat ( szSize, Txt ( T_SIZE_BYTES ) ); 
		}
		else
			if ( uSizeLow < 5242880UL )	// 512k <= size < 5M -> 'k'
			{
				wsprintf ( szSize, L"%d ", uSizeLow >> 10 );
				wcscat ( szSize, Txt ( T_SIZE_KBYTES ) );
			}
			else
				if ( uSizeLow < 1048471000UL ) // 5M <= size < 1G -> 'm'
				{
					wsprintf ( szSize, L"%.1f ", float ( uSizeLow >> 10 ) / 1024.0f );
					wcscat ( szSize, Txt ( T_SIZE_MBYTES ) );
				}
				else // 1G <= size < 3G -> 'g'
				{
					wsprintf ( szSize, L"%.1f ", float ( uSizeLow >> 20 ) / 1024.0f );
					wcscat ( szSize, Txt ( T_SIZE_GBYTES ) );
				}
	}

	return szSize;
}


const wchar_t * FileSizeToStringUL ( const ULARGE_INTEGER & uLargeSize, wchar_t * szSize, bool bShowBytes )
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


void AlignFileName ( HWND hWnd, const wchar_t * szText )
{
	if ( ! hWnd )
		return;

	HDC hDC = GetDC ( hWnd );
	HFONT hFont = (HFONT) SendMessage ( hWnd, WM_GETFONT, 0, 0 );
	SelectObject ( hDC, hFont );

	RECT tTextRect = { 0, 0, 0, 0 }, tWindowRect;

	int iTextLen = wcslen ( szText );

	GetWindowRect ( hWnd, &tWindowRect );
	DrawText ( hDC, szText, iTextLen, &tTextRect, DT_LEFT | DT_TOP | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX );

	if ( tTextRect.right - tTextRect.left > tWindowRect.right - tWindowRect.left )
	{
		TEXTMETRIC tMetric;
		GetTextMetrics ( hDC, &tMetric );

		int nCharsToFit = ( tWindowRect.right - tWindowRect.left ) / tMetric.tmAveCharWidth;
		
		int iStartChar = iTextLen - nCharsToFit;
		if ( iStartChar < 0 )
			iStartChar = 0;

		Str_c sTextToDraw = szText;
		sTextToDraw = Str_c ( L"..." ) + sTextToDraw.SubStr ( iStartChar, nCharsToFit );
	
		while ( ( sTextToDraw.Length () > 3 ) && ( tTextRect.right - tTextRect.left >= tWindowRect.right - tWindowRect.left ) )
		{
			sTextToDraw.Erase ( 3, 1 );
			DrawText ( hDC, sTextToDraw, sTextToDraw.Length (), &tTextRect, DT_LEFT | DT_TOP | DT_NOCLIP | DT_CALCRECT | DT_NOPREFIX );
		}

		SetWindowText ( hWnd, sTextToDraw );
	}
	else
		SetWindowText ( hWnd, szText );
}


void AlignFileNameRect ( HDC hDC, const RECT & tRect, const Str_c & sText )
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


void CalcTotalSize ( SelectedFileList_t & tList, ULARGE_INTEGER & uSize, int & nFiles, int & nFolders, DWORD & dwIncAttrib, DWORD & dwExAttrib, SlowOperationCallback_t fnCallback )
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

//////////////////////////////////////////////////////////////////////////
ErrorHandlers_t::ErrorHandlers_t ()
{
	m_hMkDir	= ErrCreateOperation ( Txt (T_OP_HEAD_MKDIR),		EB_R_C,			false );
	m_hSetProps	= ErrCreateOperation ( Txt (T_OP_HEAD_PROPS),		EB_R_S_SA_C,	false );
	m_hShortcut	= ErrCreateOperation ( Txt (T_OP_HEAD_SHORTCUT),	EB_R_C,			false );
	m_hDelete	= ErrCreateOperation ( Txt (T_OP_HEAD_DELETE),		EB_R_S_SA_C,	false );
	m_hMove		= ErrCreateOperation ( Txt (T_OP_HEAD_MOVE),		EB_R_S_SA_C,	true );
	m_hCopy		= ErrCreateOperation ( Txt (T_OP_HEAD_COPY),		EB_R_S_SA_C,	true );
	m_hRename	= ErrCreateOperation ( Txt (T_OP_HEAD_RENAME),		EB_R_S_SA_C,	false );

	ErrAddCustomErrorHandler( m_hShortcut,	false,	EC_DEST_EXISTS,				EB_O_OA_S_SA_C,	Txt ( T_ERR_DEST_EXISTS ) );

	ErrAddCustomErrorHandler( m_hMove,		true,	EC_MOVE_ONTO_ITSELF,		EB_C,			Txt ( T_ERR_MOVE_ONTO_ITSELF ) );
	ErrAddCustomErrorHandler( m_hMove,		true,	EC_NOT_ENOUGH_SPACE,		EB_C,			Txt ( T_ERR_NOT_ENOUGH_SPACE ) );
	ErrAddCustomErrorHandler( m_hMove,		true,	EC_DEST_EXISTS,				EB_O_OA_S_SA_C,	Txt ( T_ERR_DEST_EXISTS ) );
	ErrAddCustomErrorHandler( m_hMove,		true,	EC_DEST_READONLY,			EB_O_OA_S_SA_C,	Txt ( T_ERR_DEST_READONLY ) );

	ErrAddCustomErrorHandler( m_hCopy,		true,	EC_COPY_ONTO_ITSELF,		EB_C,			Txt ( T_ERR_COPY_ONTO_ITSELF ) );
	ErrAddCustomErrorHandler( m_hCopy,		true,	EC_NOT_ENOUGH_SPACE,		EB_C,			Txt ( T_ERR_NOT_ENOUGH_SPACE ) );
	ErrAddCustomErrorHandler( m_hCopy,		true,	EC_DEST_EXISTS,				EB_O_OA_S_SA_C,	Txt ( T_ERR_DEST_EXISTS ) );
	ErrAddCustomErrorHandler( m_hCopy,		true,	EC_DEST_READONLY,			EB_O_OA_S_SA_C,	Txt ( T_ERR_DEST_READONLY ) );

	ErrAddCustomErrorHandler( m_hRename,	false,	EC_NOT_ENOUGH_SPACE,		EB_C,			Txt ( T_ERR_NOT_ENOUGH_SPACE ) );
	ErrAddCustomErrorHandler( m_hRename,	false,	EC_DEST_EXISTS,				EB_O_OA_S_SA_C,	Txt ( T_ERR_DEST_EXISTS ) );
	ErrAddCustomErrorHandler( m_hRename,	false,	EC_DEST_READONLY,			EB_O_OA_S_SA_C,	Txt ( T_ERR_DEST_READONLY ) );
}

ErrorHandlers_t::~ErrorHandlers_t ()
{
	ErrDestroyOperation ( m_hMkDir );
	ErrDestroyOperation ( m_hSetProps );
	ErrDestroyOperation ( m_hShortcut );
	ErrDestroyOperation ( m_hDelete );
	ErrDestroyOperation ( m_hMove );
	ErrDestroyOperation ( m_hRename );
	ErrDestroyOperation ( m_hCopy );
}


//////////////////////////////////////////////////////////////////////////
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

void FileChangeProperties ( HWND hWnd, SelectedFileList_t & tList, const FilePropertyCommand_t & tCommand, SlowOperationCallback_t fnCallback )
{
	FileIteratorPanel_c tIterator;
	tIterator.IterateStart ( tList, !! ( tCommand.m_dwFlags & FILE_PROPERTY_RECURSIVE ) );

	ErrSetOperation ( g_ErrorHandlers.m_hSetProps );

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
				OperationResult_e eRes = TryToSetAttributes ( hWnd, tIterator.GetFullName (), pFindData, tCommand );
				if ( eRes == RES_CANCEL )
					break;	
			}
		}
	}
}

static OperationResult_e TryToCreateShortcut ( const Str_c & sDest, const Str_c & sParams )
{
	MakeDirectory ( GetPath ( sDest ) );

	ErrSetOperation ( g_ErrorHandlers.m_hShortcut );

	BOOL bRes = FALSE;
	HWND m_hWnd = g_hMainWindow;

	// try to find destination
	WIN32_FIND_DATA tFindData;
	HANDLE hFound = FindFirstFile ( sDest, &tFindData );
	bool bDestExists = hFound != INVALID_HANDLE_VALUE;
	if ( bDestExists )
	{
		FindClose ( hFound );

		ErrResponse_e eErrResponse = ErrShowErrorDlg ( m_hWnd, EC_DEST_EXISTS, false, sDest, NULL );

		if ( eErrResponse != ER_OVER )
			return eErrResponse == ER_CANCEL ? RES_CANCEL : RES_ERROR;

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

static OperationResult_e TryToMakeDir ( const Str_c & sFileName )
{
	BOOL bResult = FALSE;
	HWND m_hWnd = g_hMainWindow;
	TRY_FUNC ( bResult, CreateDirectory ( sFileName, NULL ), FALSE, sFileName, L"" );
	return RES_OK;
}

bool MakeDirectory ( const Str_c & sDir )
{
	ErrSetOperation ( g_ErrorHandlers.m_hMkDir );

	int iOffset = 1;
	int iPos = 0;
	OperationResult_e eRes = RES_OK;
	Str_c sSourceDir = sDir;
	Str_c sDirToCreate;

	RemoveSlash ( sSourceDir );

	while ( ( iPos = sSourceDir.Find ( L'\\', iOffset ) ) != -1 )
	{
		sDirToCreate = sSourceDir.SubStr ( 0, iPos );

		DWORD dwAttributes = GetFileAttributes ( sDirToCreate );
		if ( dwAttributes == 0xFFFFFFFF )
		{
			eRes = TryToMakeDir ( sDirToCreate );
			if ( eRes != RES_OK  )
				return false;
		}
		iOffset = iPos + 1;
	}

	if ( ! sSourceDir.Empty () && GetFileAttributes ( sSourceDir ) == 0xFFFFFFFF )
		if ( TryToMakeDir ( sSourceDir ) != RES_OK )
			return false;

	return true;
}

/*bool MakeDirectory ( const Str_c & sDir, const Str_c & sRootDir )
{
	Str_c sDirToCreate = sDir;
	PrepareDestDir ( sDirToCreate, sRootDir );
	RemoveSlash ( sDirToCreate );
	return MakeDirectory ( sDirToCreate );
}*/