#include "pch.h"

#include "LFile/fdelete.h"
#include "LCore/clog.h"
#include "LFile/ferrors.h"
#include "LPanel/pbase.h"

#include "Resources/resource.h"


FileDelete_c::FileDelete_c ( FileList_t & tList )
	: m_hWnd 	( NULL )
	, m_nFiles 	( 0 )
	, m_tList	( tList )
{
	g_tErrorList.Reset ( OM_DELETE );
	m_tIterator.IterateStart ( tList );
}


bool FileDelete_c::PrepareFileName ()
{
	while ( m_tIterator.IterateNext () )
	{
		const WIN32_FIND_DATA * pData = m_tIterator.GetData ();
		if ( pData )
		{
			if ( pData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				if ( m_tIterator.Is2ndPassDir () )
					return true;
			}
			else
				return true;

		}
	}

	return false;
}


bool FileDelete_c::DeleteNext ()
{
	const WIN32_FIND_DATA * pData = m_tIterator.GetData ();
	if ( pData )
	{
		Str_c sFileName = m_tIterator.GetSourceDir () + m_tIterator.GetFileName ();
		if ( m_tIterator.Is2ndPassDir () && ( pData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
		{
			if ( TryToDeleteDir ( sFileName ) == RES_CANCEL )
				return false;

			++m_nFiles;
		}
		else
		{
			if ( ! m_tIterator.Is2ndPassDir () && ! ( pData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
			{
				if ( TryToDeleteFile ( sFileName ) == RES_CANCEL )
					return false;

				++m_nFiles;
			}
		}
	}

	return true;
}

bool FileDelete_c::IsInDialog  () const
{
	return g_tErrorList.IsInDialog ();
}


void FileDelete_c::SetWindow ( HWND hWnd )
{
	m_hWnd = hWnd;
}


int FileDelete_c::GetNumDeletedFiles () const
{
	return m_nFiles;
}


Str_c FileDelete_c::GetFileToDelete () const
{
	return m_tIterator.GetFileName ();
}


OperationResult_e FileDelete_c::TryToDeleteDir ( const Str_c & sFileName )
{
	BOOL bResult = FALSE;
	TRY_FUNC ( bResult, RemoveDirectory ( sFileName ), FALSE, sFileName, L"" );
	return RES_OK;
}


OperationResult_e FileDelete_c::TryToDeleteFile ( const Str_c & sFileName )
{
	BOOL bResult = FALSE;
	DWORD dwAttrib = 0;

	TRY_FUNC ( dwAttrib, GetFileAttributes ( sFileName ), 0xFFFFFFFF, sFileName, L"" );
	DWORD dwAttribToSet = dwAttrib;

	// test for read-only-ness
	if ( dwAttrib & FILE_ATTRIBUTE_READONLY )
	{
		int iErrAction = g_tErrorList.ShowErrorDialog ( m_hWnd, EC_DEST_READONLY, false, sFileName );

		if ( iErrAction != IDC_OVER )
			return iErrAction == IDCANCEL ? RES_CANCEL : RES_ERROR;
	}

	dwAttribToSet &= ~ ( FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM );

	TRY_FUNC ( bResult, SetFileAttributes ( sFileName, dwAttribToSet ), FALSE, sFileName, L"" );
	TRY_FUNC ( bResult, DeleteFile  ( sFileName ), FALSE, sFileName, L"" );

	return RES_OK;
}