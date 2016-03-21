#include "pch.h"

#include "LFile/fsend.h"
#include "LComm/cbluetooth.h"
#include "LComm/cmsbt.h"

#include "Dlls\Resource\resource.h"

static const int WIDCOMM_RSP_CONTINUE	= 0x10;
static const int WIDCOMM_RSP_OK			= 0x20;

//////////////////////////////////////////////////////////////////////////
FileSend_c::FileSend_c ( unsigned int uMaxChunk, SelectedFileList_t & tList, SlowOperationCallback_t fnPrepareCallback )
	: m_pList		( &tList )
	, m_uBytesRead	( 0 )
	, m_uMaxChunk	( uMaxChunk )
	, m_eState 		( STATE_SEND_HEADER )
	, m_eError		( ERR_OK )
	, m_hWnd		( NULL )
{
	m_pBuffer = new unsigned char [m_uMaxChunk];

	int nFiles, nFolders;
	DWORD dwA1, dwA2;
	CalcTotalSize ( tList, m_uTotalSize, nFiles, nFolders, dwA1, dwA2, fnPrepareCallback );

	m_tFileIterator.IterateStart ( tList );
}

void FileSend_c::SetWindow ( HWND hWnd )
{
	m_hWnd = hWnd;
}

const wchar_t * FileSend_c::GetSourceFileName () const
{
	return m_sSourceFileName.c_str ();
}

FileSend_c::Error_e FileSend_c::GetError () const
{
	return m_eError;
}

bool FileSend_c::StartFile ()
{
	bool bFileOk = false;

	m_eError = ERR_READ;

	while ( ! bFileOk )
	{
		if ( ! GenerateNextFileName () )
		{
			m_eError = ERR_OK;
			return false;
		}

		OperationResult_e eResult = PrepareFile ();
		switch ( eResult )
		{
		case RES_ERROR:
			if ( m_hSource )
				CloseHandle ( m_hSource );

			MoveToNext ( m_uSourceSize, m_uSourceSize );
			break;

		case RES_CANCEL:
			Cancel ();
			m_eError = ERR_OK;
			return false;

		case RES_OK:
			bFileOk = true;
			break;
		}
	}

	if ( ! bFileOk )
		return false;

	m_eError = ERR_SEND;

	return SendFileName ();
}


bool FileSend_c::OnPut ( int iResponse )
{
	switch ( m_eState )
	{
	case STATE_SEND_HEADER:
		if ( iResponse != GetRspContinue () )
		{
			Cancel ();
			return false;
		}

		m_eState = STATE_SEND_BODY;
		if ( ! SendNextChunk () )
			return false;

		return true;

	case STATE_SEND_BODY:
		{
			bool bEOF = m_uBytesRead < m_uMaxChunk;
			if ( ( bEOF && iResponse != GetRspOk () ) ||  ( ! bEOF && iResponse != GetRspContinue () ) )
				return false;

			if ( ! bEOF )
			{
				if ( ! SendNextChunk () )
					return false;
			}
			else
			{
				MoveToNext ( m_uSourceSize, m_uSourceSize );
	
				if ( ! StartFile () )
				{
					UpdateProgress ();
					return false;
				}

				m_eState = STATE_SEND_HEADER;
			}
		}

		UpdateProgress ();
		return true;
	}

	return false;
}


OperationResult_e FileSend_c::PrepareFile ()
{
	BOOL bResult = FALSE;

	// open source file
	TRY_FUNC ( m_hSource, CreateFile ( m_sSourceFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL ), INVALID_HANDLE_VALUE, m_sSourceFileName, L"" );

	// get file size
	TRY_FUNC ( m_uSourceSize.LowPart, GetFileSize ( m_hSource, &m_uSourceSize.HighPart ), 0xFFFFFFFF, m_sSourceFileName, L"" );

	return RES_OK;
}


bool FileSend_c::SendNextChunk ()
{
	m_eError = ERR_READ;

	BOOL bResult = ReadFile ( m_hSource, (void *) m_pBuffer, m_uMaxChunk, &m_uBytesRead, NULL );
	if ( ! bResult )
	{
		Cancel ();
		return false;
	}

	m_eError = ERR_SEND;

	bool bEOF = m_uBytesRead < m_uMaxChunk;

	m_uProcessedFile.QuadPart += m_uBytesRead;
	m_uProcessedTotal.QuadPart += m_uBytesRead;

	if ( ! SendChunk ( m_pBuffer, m_uBytesRead, bEOF ) )
	{
		Cancel ();
		return false;
	}

	return true;
}


void FileSend_c::Cancel ()
{
	SendAbort ();

	if ( m_hSource )
		CloseHandle ( m_hSource );

	m_eState = STATE_ERROR;
}

bool FileSend_c::GenerateNextFileName ()
{
	while ( m_tFileIterator.IterateNext () )
	{
		const WIN32_FIND_DATA * pData = m_tFileIterator.GetData ();
		if ( pData && ! ( pData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) && ! m_tFileIterator.Is2ndPassDir () )
		{
			m_sSourceFileName = m_tFileIterator.GetFullName ();
			return true;
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
FileSendIrda_c::FileSendIrda_c ( IrdaObexClient_c * pOBEX, unsigned int uMaxChunk, SelectedFileList_t & tList, SlowOperationCallback_t fnPrepareCallback )
	: FileSend_c ( uMaxChunk, tList, fnPrepareCallback )
	, m_pOBEX ( pOBEX )
{
}

bool FileSendIrda_c::SendFileName ()
{
	Str_c sDir, sName, sExt;
	SplitPath ( m_sSourceFileName, sDir, sName, sExt );

	m_pOBEX->PutHead ( sName + sExt, m_uSourceSize.LowPart );
	return true;
}

bool FileSendIrda_c::SendChunk ( unsigned char * pBuffer, unsigned int uBytes, bool bEOF )
{
	m_pOBEX->PutBody ( pBuffer, uBytes, bEOF );
	return true;
}

void FileSendIrda_c::SendAbort ()
{
	// need an abort here?
}

int FileSendIrda_c::GetRspOk () const
{
	return IR_OBEX_SUCCESS;
}

int FileSendIrda_c::GetRspContinue () const
{
	return IR_OBEX_CONTINUE;
}


///////////////////////////////////////////////////////////////////////////////////////////
FileSendWidcomm_c::FileSendWidcomm_c ( unsigned int uMaxChunk, SelectedFileList_t & tList, SlowOperationCallback_t fnPrepareCallback )
	: FileSend_c ( uMaxChunk, tList, fnPrepareCallback )
{
}

bool FileSendWidcomm_c::SendFileName ()
{
	Str_c sDir, sName, sExt;
	SplitPath ( m_sSourceFileName, sDir, sName, sExt );

	return dll_WBT_ObexPutFileName ( (sName + sExt).c_str (), m_uSourceSize.LowPart );
}


bool FileSendWidcomm_c::SendChunk ( unsigned char * pBuffer, unsigned int uBytes, bool bEOF )
{
	return dll_WBT_ObexPutBody ( pBuffer, uBytes, bEOF );
}

void FileSendWidcomm_c::SendAbort ()
{
	dll_WBT_ObexAbort ();
}

int FileSendWidcomm_c::GetRspOk () const
{
	return WIDCOMM_RSP_OK;
}

int FileSendWidcomm_c::GetRspContinue () const
{
	return WIDCOMM_RSP_CONTINUE;
}

//////////////////////////////////////////////////////////////////////////
FileSendMsBT_c::FileSendMsBT_c ( MsBTObexClient_c * pOBEX, unsigned int uMaxChunk, SelectedFileList_t & tList, SlowOperationCallback_t fnPrepareCallback )
	: FileSend_c ( uMaxChunk, tList, fnPrepareCallback )
	, m_pOBEX ( pOBEX )
{
}

bool FileSendMsBT_c::SendFileName ()
{
	Str_c sDir, sName, sExt;
	SplitPath ( m_sSourceFileName, sDir, sName, sExt );

	m_pOBEX->PutHead ( sName + sExt, m_uSourceSize.LowPart );
	return true;
}

bool FileSendMsBT_c::SendChunk ( unsigned char * pBuffer, unsigned int uBytes, bool bEOF )
{
	m_pOBEX->PutBody ( pBuffer, uBytes, bEOF );
	return true;
}

void FileSendMsBT_c::SendAbort ()
{
	// need an abort here?
}

int FileSendMsBT_c::GetRspOk () const
{
	return IR_OBEX_SUCCESS;
}

int FileSendMsBT_c::GetRspContinue () const
{
	return IR_OBEX_CONTINUE;
}