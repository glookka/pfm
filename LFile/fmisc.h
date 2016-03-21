#ifndef _fmisc_
#define _fmisc_

#include "LCore/cmain.h"
#include "LFile/fiterator.h"

enum
{
	 TIMEDATE_BUFFER_SIZE	= 128
	,FILESIZE_BUFFER_SIZE	= 64
	,PATH_BUFFER_SIZE		= 256
};

enum FilePropertyFlags_e
{
	 FILE_PROPERTY_TIME 		= 1 << 0
	,FILE_PROPERTY_ATTRIBUTES 	= 1 << 1
	,FILE_PROPERTY_RECURSIVE	= 1 << 2
};


struct FilePropertyCommand_t
{
	SYSTEMTIME	m_tTimeCreate;
	SYSTEMTIME	m_tTimeAccess;
	SYSTEMTIME	m_tTimeWrite;

	DWORD		m_dwIncAttrib;
	DWORD		m_dwExAttrib;
	DWORD 		m_dwFlags;
};


extern DWORD g_dAttributes [];

typedef void ( * SlowOperationCallback_t ) ();
typedef void ( * MarkCallback_t ) ( const Str_c &, bool );

// for max size width calculation
const wchar_t * GetMaxByteString ();
const wchar_t * GetMaxKByteString ();
const wchar_t * GetMaxMByteString ();

// make a string out of a file's size
const wchar_t * FileSizeToString ( DWORD uSizeHigh, DWORD uSizeLow, wchar_t * szSize, bool bShowBytes );
// overloaded version
const wchar_t * FileSizeToString ( const ULARGE_INTEGER & uLargeSize, wchar_t * szSize, bool bShowBytes );
// bytes. every 3 digits are separated by spaces
const wchar_t * FileSizeToStringByte ( const ULARGE_INTEGER & uLargeSize, wchar_t * szSize );
// alignes and clips file name in a static control
void AlignFileName ( HWND hWnd, const Str_c & sText );
// alignes, clips and draws a file name
void AlignFileName ( HDC hDC, const RECT & tRect, const Str_c & sText );
// calculate file size + collect attributes
void CalcTotalSize ( FileList_t & tList, ULARGE_INTEGER & uSize, int & nFiles, int & nFolders, DWORD & dwIncAttrib, DWORD & dwExAttrib, SlowOperationCallback_t fnCallback = NULL );
// change the properties of a file tree
void FileChangeProperties ( HWND hWnd, FileList_t & tList, const FilePropertyCommand_t & tCommand, SlowOperationCallback_t fnCallback = NULL );
// append source 
void PrepareDestDir ( Str_c & sDest, const Str_c & sRootDir );
// create a shortcut
bool CreateShortcut ( const Str_c & sDest, const Str_c & sParams );
// has subdirs?
bool HasSubDirs ( const Str_c & sPath );

#endif
