#ifndef _filemisc_
#define _filemisc_

#include "main.h"
#include "iterator.h"

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

class ErrorHandlers_t
{
public:
	HANDLE		m_hMkDir;
	HANDLE		m_hSetProps;
	HANDLE		m_hShortcut;
	HANDLE		m_hDelete;
	HANDLE		m_hMove;
	HANDLE		m_hRename;
	HANDLE		m_hCopy;

				ErrorHandlers_t ();
				~ErrorHandlers_t ();
};

extern ErrorHandlers_t g_ErrorHandlers;


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

// check if .lnk is a folder or a file
bool DecomposeLnk ( Str_c & sPath, Str_c & sParams, bool & bDir );
// are files on different drives/cards/volumes?
bool AreFilesOnDifferentVolumes ( const Str_c & sFile1, const Str_c & sFile2 );
// make a string out of a file's size
const wchar_t * FileSizeToString ( DWORD uSizeHigh, DWORD uSizeLow, wchar_t * szSize, bool bShowBytes );
// overloaded version
const wchar_t * FileSizeToStringUL ( const ULARGE_INTEGER & uLargeSize, wchar_t * szSize, bool bShowBytes );
// bytes. every 3 digits are separated by spaces
const wchar_t * FileSizeToStringByte ( const ULARGE_INTEGER & uLargeSize, wchar_t * szSize );
// alignes and clips file name in a static control
void AlignFileName ( HWND hWnd, const wchar_t * szText );
// alignes, clips and draws a file name
void AlignFileNameRect ( HDC hDC, const RECT & tRect, const Str_c & sText );
// calculate file size + collect attributes
void CalcTotalSize ( SelectedFileList_t & tList, ULARGE_INTEGER & uSize, int & nFiles, int & nFolders, DWORD & dwIncAttrib, DWORD & dwExAttrib, SlowOperationCallback_t fnCallback = NULL );
// append source 
void PrepareDestDir ( Str_c & sDest, const Str_c & sRootDir );
// has subdirs?
bool HasSubDirs ( const Str_c & sPath );

// change the properties of a file tree
void FileChangeProperties ( HWND hWnd, SelectedFileList_t & tList, const FilePropertyCommand_t & tCommand, SlowOperationCallback_t fnCallback = NULL );
// create a shortcut
bool CreateShortcut ( const Str_c & sDest, const Str_c & sParams );
// create a folder
bool MakeDirectory ( const Str_c & sDir );
//bool MakeDirectory ( const Str_c & sDir, const Str_c & sRootDir );

#endif