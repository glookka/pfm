#include "pch.h"

#include "LCore/cfile.h"
#include "LCore/clog.h"

#include "projects.h"

const int MAX_FILENAME_LENGTH = 255;

///////////////////////////////////////////////////////////////////////////////////////////
// helpers
Str_c GetExt ( const wchar_t * szFileName )
{
	Str_c sFileName ( szFileName );

	int i = sFileName.RFind ( L'.' );

	sFileName = sFileName.SubStr ( i + 1, sFileName.Length () - i - 1 );

	return sFileName;
}


Str_c GetPath ( const wchar_t * szFileName )
{
	Str_c sFileName ( szFileName );
	int iRes = sFileName.RFind ( L'\\' );
	if ( iRes == -1 )
		return L"\\";

	sFileName = sFileName.SubStr ( 0, iRes + 1 );

	return sFileName;
}


Str_c GetName ( const wchar_t * szPath )
{
	Str_c sFileName ( szPath );
	int iRes = sFileName.RFind ( L'\\' );
	if ( iRes != -1 )
		sFileName = sFileName.SubStr ( iRes + 1 );

	return sFileName;
}


Str_c GetExecutablePath ()
{
	wchar_t szFileName [MAX_FILENAME_LENGTH];

	GetModuleFileName ( NULL, szFileName, MAX_FILENAME_LENGTH );
	return GetPath ( szFileName );
}


const wchar_t * GetNameExtFast ( const wchar_t * szNameExt, wchar_t * szName )
{
	int iLen = wcslen ( szNameExt );
	int iToCopy = iLen;
	const wchar_t * szExt = NULL;

	for ( int i = iLen - 1; i; --i )
		if ( szNameExt [i] == L'.' )
		{
			szExt = & ( szNameExt [i + 1] );
			iToCopy = i;
			break;
		}

	if ( ! szExt )
		szExt = & ( szNameExt [iLen] );

	if ( szName )
	{
		wcsncpy ( szName, szNameExt, iToCopy );
		szName [iToCopy] = L'\0';
	}

	return szExt;
}


void SplitPath ( const Str_c & sFileName, Str_c & sDir, Str_c & sName, Str_c & sExt )
{
	int iDirPos = sFileName.RFind (  L'\\' );
	if ( iDirPos != -1 )
		sDir = sFileName.SubStr ( 0, iDirPos + 1 );
	else
		sDir = L"";

	int iDotPos = sFileName.RFind ( L'.' );
	if ( iDotPos != -1 && iDotPos > iDirPos )
	{
		sExt = sFileName.SubStr ( iDotPos, sFileName.Length () - iDotPos + 1 );

		if ( iDirPos != -1 )
			sName = sFileName.SubStr ( iDirPos + 1, iDotPos - iDirPos - 1 );
		else
			sName = sFileName.SubStr ( 0, iDotPos - iDirPos - 1 );
	}
	else
	{
		sExt = L"";

		if ( iDirPos != -1 )
			sName = sFileName.SubStr ( iDirPos + 1, sFileName.Length () - iDirPos + 1 );
		else
			sName = sFileName;
	}
}

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

bool IsStorageCard ( const Str_c & sFile )
{
	WIN32_FIND_DATA tData;
	HANDLE hCard = FindFirstFlashCard ( &tData );
	int iCard = 0;
	if ( hCard != INVALID_HANDLE_VALUE )
	{
		do 
		{
			if ( sFile == tData.cFileName )
				return true;
		}
		while ( FindNextFlashCard ( hCard, &tData ) );
	}

	return false;
}


Str_c & AppendSlash ( Str_c & sStr )
{
	int iPos = sStr.RFind ( L'\\' );
	if ( iPos != sStr.Length () - 1 || iPos == -1 )
		sStr += L"\\";

	return sStr;
}

void RemoveSlash ( Str_c & sStr )
{
	int iPos = sStr.RFind ( L'\\' );
	if ( iPos != -1 && iPos == sStr.Length () - 1 )
		sStr.Chop ( 1 );
}

bool EndsInSlash ( const Str_c & sStr )
{
	return sStr.RFind ( L'\\' ) == sStr.Length () - 1;
}

void CutToSlash ( Str_c & sStr )
{
	int nPos = sStr.Find ( L'\\' );

	if ( nPos != -1 )
		sStr.Erase ( 0, nPos + 1 );
}

Str_c RemoveSlashes ( const Str_c & sStr )
{
	Str_c sResult = sStr;

	RemoveSlash ( sResult );

	while ( sResult.Length () > 0 && sResult [0] == L'\\' )
		sResult.Erase ( 0, 1 );
	
	return sResult;
}

void ReplaceSlashes ( Str_c & sPath )
{
	int iPos = 0;
	for ( int i = 0; i < sPath.Length (); ++i )
	{
		if ( sPath [i] == L'/' )
			sPath [i] = L'\\';
	}
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

	Str_c sPathNoQuotes = sPath;
	sPathNoQuotes.Strip ( L'\"' );
	DWORD dwAttributes = GetFileAttributes ( sPathNoQuotes );
	if ( dwAttributes == 0xFFFFFFFF )
	{
		bDir = false;
		return true;
	}

	bDir = !!( dwAttributes & FILE_ATTRIBUTE_DIRECTORY );
	return true;
}