#include "pch.h"

#include "LFile/fcrypt.h"
#include "LCore/clog.h"
#include "LCore/cfile.h"
#include "LSettings/sconfig.h"
#include "LSettings/slocal.h"

#include "Resources/resource.h"

static const wchar_t g_szCryptExt [] = L".cf";
static const DWORD ENCRYPT_SIGNATURE = 0xCFCF0001;
static const unsigned short MAX_KEY_LEN = 512;
static const unsigned short MAX_EXT_SIZE = 260;

//////////////////////////////////////////////////////////////////////////

Encrypter_c::Encrypter_c ( FileList_t & tList, bool bEncrypt, bool bDelete, const crypt::AlgInfo_t & tAlg, const Str_c & sKey,
						   SlowOperationCallback_t fnPrepareCallback, MarkCallback_t fnMarkCallback )
	: m_bEncrypt		( bEncrypt )
	, m_bDelete			( bDelete )
	, m_pPlainBuffer	( NULL )
	, m_pEncBuffer		( NULL )
	, m_eState			( STATE_INITIAL )
	, m_sKey			( sKey )
	, m_hWnd			( NULL )
	, m_hSource			( NULL )
	, m_hDest			( NULL )
	, m_dwSourceAttrib	( 0 )
	, m_eErrorCode		( ERROR_NONE )
	, m_nFiles			( 0 )
	, m_nProcessed		( 0 )
	, m_fnMarkCallback	( fnMarkCallback )
{
	m_pList = &tList;
	m_pAlg = &tAlg;

	m_pPlainBuffer = new unsigned char [PLAIN_CHUNK_SIZE];
	m_pEncBuffer = new unsigned char [ENC_CHUNK_SIZE];
	Assert ( m_pPlainBuffer && m_pEncBuffer );

	g_tErrorList.Reset ( OM_CRYPT );

	int nFiles, nFolders;
	DWORD dwA1, dwA2;
	CalcTotalSize ( tList, m_uTotalSize, nFiles, nFolders, dwA1, dwA2, fnPrepareCallback );

	m_tFileIterator.IterateStart ( tList );

	memset ( m_dKey, 0, sizeof ( m_dKey ) );
}


Encrypter_c::~Encrypter_c ()
{
	SafeDeleteArray ( m_pPlainBuffer );
	SafeDeleteArray ( m_pEncBuffer );
}

bool Encrypter_c::IsInDialog () const
{
	return g_tErrorList.IsInDialog ();
}

void Encrypter_c::PrepareKey ()
{
	int iKeySize = m_pAlg->m_iKeyLength;
	int iGotKeyLength = m_sKey.Length () * sizeof ( wchar_t );
	const unsigned char * pKey = (unsigned char *) m_sKey.c_str ();

	int iKeyCounter = 0;
	for ( int i = 0; i < iGotKeyLength; ++i )
	{
		m_dKey [iKeyCounter] ^= pKey [i];

		if ( ++iKeyCounter >= iKeySize )
			iKeyCounter = 0;
	}
		
}

void Encrypter_c::PrepareIV ()
{
	for ( int i = 0; i < MAX_BLOCK_LENGTH / 2; ++i )
		( (unsigned short * )m_IV ) [i] = rand ();
}

bool Encrypter_c::PrepareCryptLib ()
{
	Assert ( m_pAlg );

	int iId = m_pAlg->m_iId;

	int iKeySize = m_pAlg->m_iKeyLength;
	m_pAlg->m_tDesc.keysize ( &iKeySize );
	if ( m_pAlg->m_tDesc.keysize ( &iKeySize ) != CRYPT_OK )
		return false;

	int iErr = crypt::dll_ctr_start ( iId, m_IV, m_dKey, iKeySize, 0, CTR_COUNTER_LITTLE_ENDIAN, &m_CTR );
	Assert ( iErr == CRYPT_OK );
	if ( iErr != CRYPT_OK )
		return false;
	
	return true;
}

bool Encrypter_c::GenerateNextFileName ()
{
	Assert ( m_eState == STATE_PREPARE_SOURCE );

	bool bRes = false;

	Str_c sExt;
	while ( m_tFileIterator.IterateNext () )
	{
		if ( ! m_tFileIterator.Is2ndPassDir () )
		{
			const WIN32_FIND_DATA *	pData = m_tFileIterator.GetData ();
			if ( pData && ! ( pData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
			{
				m_sSource = m_pList->m_sRootDir + m_tFileIterator.GetFileName ();
				m_sSourceFileName = m_tFileIterator.GetData ()->cFileName;

				SplitPath ( m_sSource, m_sDir, m_sSourceName, sExt );
			
				if ( ! sExt.Empty () && sExt [0] == L'.' )
					m_sExt = sExt.SubStr ( 1, sExt.Length () - 1 );

				if ( m_bEncrypt )
					m_sDest = m_sDir + m_sSourceName + L"_" + m_sExt + g_szCryptExt;
				else
					m_sDest = m_sSource;

				m_bNameFlag = true;
				m_eErrorCode = ERROR_NONE;

				m_nFiles++;

				bRes = true;
				break;
			}
		}
		else
			MarkSourceOk ( m_tFileIterator.GetData ()->cFileName );
	}

	return bRes;
}

Str_c Encrypter_c::GetDecryptedFileName () const
{
	int iIndex = m_sSourceName.RFind ( L'_' );
	if ( iIndex == -1 )
		return m_sSourceName + L"." + m_sExt;
	else
	{
		Str_c sSourceExt = iIndex +1 < m_sSourceName.Length () ? m_sSourceName.SubStr ( iIndex + 1 ) : L"";

		if ( sSourceExt != m_sExt )
			return m_sSourceName + L"." + m_sExt;
		else
			return m_sSourceName.SubStr ( 0, iIndex ) + L"." + m_sExt;
	}
}

void Encrypter_c::MoveToNextFile ()
{
	if ( m_eErrorCode != ERROR_NONE )
	{
		Error_t tError;
		tError.m_iIndex = m_tFileIterator.GetIndex ();
		tError.m_eError = m_eErrorCode;

		m_dErrors.Add ( tError );
	}

	MoveToNext ( m_uProcessedFile, m_uSourceSize );

	m_eState = STATE_PREPARE_SOURCE;
}

OperationResult_e Encrypter_c::PrepareSource ()
{
	BOOL bResult = FALSE;
	ULARGE_INTEGER uAvailable, uTotal;

	// get source attributes
	TRY_FUNC ( m_dwSourceAttrib, GetFileAttributes ( m_sSource ), 0xFFFFFFFF, m_sSource, m_sDest );

	Assert ( ! ( m_dwSourceAttrib & FILE_ATTRIBUTE_DIRECTORY ) );

	// open source file
	TRY_FUNC ( m_hSource, CreateFile ( m_sSource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL ), INVALID_HANDLE_VALUE, m_sSource, m_sDest );

	// get file size
	TRY_FUNC ( m_uSourceSize.LowPart, GetFileSize ( m_hSource, &m_uSourceSize.HighPart ), 0xFFFFFFFF, m_sSource, m_sDest );

	// get free space
	TRY_FUNC ( bResult, GetDiskFreeSpaceEx ( GetPath ( m_sDest ), &uAvailable, &uTotal, NULL ), FALSE, m_sSource, m_sDest );

	// check available space
	int iErrAction = IDC_RETRY;
	while ( m_uSourceSize.QuadPart > uAvailable.QuadPart && iErrAction == IDC_RETRY  )
		iErrAction = g_tErrorList.ShowErrorDialog ( m_hWnd, EC_NOT_ENOUGH_SPACE, false, m_sSource, m_sDest );

	if ( m_uSourceSize.QuadPart > uAvailable.QuadPart )
		return iErrAction == IDCANCEL ? RES_CANCEL : RES_ERROR;

	return RES_OK;
}

OperationResult_e Encrypter_c::PrepareDest ()
{
	BOOL bResult;

	// try to find destination
	WIN32_FIND_DATA tFindData;
	HANDLE hFound = FindFirstFile ( m_sDest, &tFindData );
	bool bDestExists = hFound != INVALID_HANDLE_VALUE;
	if ( bDestExists )
	{
		FindClose ( hFound );

		if ( g_tConfig.conf_crypt_over )
		{
			int iErrAction = g_tErrorList.ShowErrorDialog ( m_hWnd, EC_DEST_EXISTS, false, m_sSource, m_sDest );

			if ( iErrAction != IDC_OVER )
				return iErrAction == IDCANCEL ? RES_CANCEL : RES_ERROR;
		}

		DWORD dwDestAttrib;

		// get dest attributes
		TRY_FUNC ( dwDestAttrib, GetFileAttributes ( m_sDest ), 0xFFFFFFFF, m_sSource, m_sDest );

		DWORD dwAttribToSet = dwDestAttrib;
		dwAttribToSet &= ~ ( FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM );
		TRY_FUNC ( bResult, SetFileAttributes ( m_sDest, dwAttribToSet ), FALSE, m_sSource, m_sDest );
	}

	// open destination file
	TRY_FUNC ( m_hDest, CreateFile ( m_sDest, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, m_dwSourceAttrib, NULL ), INVALID_HANDLE_VALUE, m_sSource, m_sDest );

	return RES_OK;
}

bool Encrypter_c::DoWork ()
{
	OperationResult_e eResult = RES_OK;
	OperationResult_e eResultFin = RES_OK;

	switch ( m_eState )
	{
	case STATE_INITIAL:
		PrepareKey ();
		m_eState = STATE_PREPARE_SOURCE;
		break;

	case STATE_PREPARE_SOURCE:
		if ( ! GenerateNextFileName () )
			return false;

		// generate file name, open files
		eResult = PrepareSource ();
		RememberError ( eResult );
		switch ( eResult )
		{
		case RES_ERROR:
			MoveToNextFile ();
			break;
		case RES_CANCEL:
			Cleanup ( false );
			break;
		default:
			m_eState = m_bEncrypt ? STATE_PREPARE_DEST : STATE_CRYPT_FIRST;
			break;
    	}
		break;

	case STATE_PREPARE_DEST:
		eResult = PrepareDest ();
		RememberError ( eResult );
		switch ( eResult )
		{
		case RES_ERROR:
			MoveToNextFile ();
			break;
		case RES_CANCEL:
			Cleanup ( false );
			break;
		default:
			m_eState = m_bEncrypt ? STATE_CRYPT_FIRST : STATE_ENCRYPT;
			break;
		}
		break;

	case STATE_CRYPT_FIRST:
		// encrypt the very first chunk (signature, file name)
		eResult = m_bEncrypt ? EncryptFirstChunk () : DecryptFirstChunk ();
		RememberError ( eResult );
		switch ( eResult )
		{
		case RES_ERROR:
			Cleanup ( m_bEncrypt );
			MoveToNextFile ();
			break;
		case RES_CANCEL:
			Cleanup ( m_bEncrypt );
			break;
		default:
			m_eState = m_bEncrypt ? STATE_ENCRYPT : STATE_PREPARE_DEST;
			break;
	   	}
		break;

	case STATE_ENCRYPT:
		eResult = EncryptNextChunk ();
		RememberError ( eResult );
		switch ( eResult )
		{
		case RES_ERROR:
			Cleanup ( true );
			MoveToNextFile ();
			break;
		case RES_FINISHED:
			eResultFin = FinishFile ();
			switch ( eResultFin )
			{
			case RES_ERROR:
				Cleanup ( true );
				MoveToNextFile ();
				break;
			case RES_CANCEL:
				Cleanup ( true );
				break;
			default:
				Cleanup ( false );
				MarkSourceOk ( m_sSourceFileName );
				MoveToNextFile ();
				break;
			}
			break;
		case RES_CANCEL:
			Cleanup ( true );
			break;
		}
		break;
	}

	return eResult != RES_CANCEL && eResultFin != RES_CANCEL;
}

OperationResult_e Encrypter_c::EncryptFirstChunk ()
{
	PrepareIV ();

	if ( ! PrepareCryptLib () )
		return RES_ERROR;

	// write non-encrypted header first
	unsigned char * pBufPtr = m_pPlainBuffer;

	*((DWORD *) pBufPtr) = ENCRYPT_SIGNATURE;
	pBufPtr += sizeof ( DWORD );

	*((int *)pBufPtr )= m_pAlg->m_iId;
	pBufPtr += sizeof ( int );

	memcpy ( pBufPtr, m_IV, sizeof ( m_IV ) );
	pBufPtr += sizeof ( m_IV );

	unsigned char * pSize = pBufPtr;
	pBufPtr += sizeof ( DWORD );

	// encrypted header
	unsigned char * pCryptPtr = pBufPtr;

	*((DWORD *) pCryptPtr) = ENCRYPT_SIGNATURE;
	pCryptPtr += sizeof ( DWORD );

	unsigned short uKeyLen = ( m_sKey.Length () + 1 ) * sizeof ( wchar_t );
	* (unsigned short *)pCryptPtr = uKeyLen;
	pCryptPtr += sizeof ( unsigned short );

	memcpy ( pCryptPtr, m_sKey.c_str (), uKeyLen );
	pCryptPtr += uKeyLen;

	unsigned short uSize = ( m_sExt.Length () + 1 ) * sizeof ( wchar_t );

	* (unsigned short *)pCryptPtr = uSize;
	pCryptPtr += sizeof ( unsigned short );

	memcpy ( pCryptPtr, m_sExt.c_str (), uSize );
	pCryptPtr += uSize;

	uSize = pCryptPtr - pBufPtr;

	// encrypted header size
	*((DWORD *)pSize ) = uSize;

	BOOL bResult;
	DWORD uBytesWritten = 0;
	TRY_FUNC ( bResult, WriteFile ( m_hDest, (void *) m_pPlainBuffer, pBufPtr - m_pPlainBuffer, &uBytesWritten, NULL ), FALSE, m_sSource, m_sDest );

	if ( uBytesWritten != pBufPtr - m_pPlainBuffer ) 
		return RES_ERROR;

	int iErr = crypt::dll_ctr_encrypt ( pBufPtr, m_pEncBuffer, uSize, &m_CTR );
	if ( iErr != CRYPT_OK )
		return RES_ERROR;

	TRY_FUNC ( bResult, WriteFile ( m_hDest, (void *) m_pEncBuffer, uSize, &uBytesWritten, NULL ), FALSE, m_sSource, m_sDest );

	if ( uBytesWritten != uSize ) 
		return RES_ERROR;

	return RES_OK;
}


OperationResult_e Encrypter_c::DecryptFirstChunk ()
{
	BOOL bResult;
	DWORD uBytesRead = 0;
	DWORD uBytesToRead = sizeof ( DWORD ) * 2 + sizeof ( int ) + sizeof ( m_IV );

	// read the header
	TRY_FUNC ( bResult, ReadFile ( m_hSource, (void *) m_pPlainBuffer, uBytesToRead, &uBytesRead, NULL ), FALSE, m_sSource, m_sDest );
	if ( uBytesRead != uBytesToRead )
		return RES_ERROR;

	unsigned char * pBufPtr = m_pPlainBuffer;

	if ( *((DWORD *) pBufPtr) != ENCRYPT_SIGNATURE )
	{
		m_eErrorCode = ERROR_NOT_ENCRYPTED;
		return RES_ERROR;
	}

	pBufPtr += sizeof ( DWORD );

	int iAlgId = *((int *)pBufPtr );
	pBufPtr += sizeof ( int );

	m_pAlg = crypt::GetAlg ( iAlgId );

	if ( ! m_pAlg )
	{
		m_eErrorCode = ERROR_CORRUPT_HEADER;
		return RES_ERROR;
	}

	memcpy ( m_IV, pBufPtr, sizeof ( m_IV ) );
	pBufPtr += sizeof ( m_IV );

	if ( ! PrepareCryptLib () )
		return RES_ERROR;

	DWORD uEncHeaderSize = *((DWORD *)pBufPtr );

	TRY_FUNC ( bResult, ReadFile ( m_hSource, (void *) m_pEncBuffer, uEncHeaderSize, &uBytesRead, NULL ), FALSE, m_sSource, m_sDest );

	if ( uBytesRead != uEncHeaderSize )
		return RES_ERROR;

	int iErr = crypt::dll_ctr_decrypt ( m_pEncBuffer, m_pPlainBuffer, uEncHeaderSize, &m_CTR );
	if ( iErr != CRYPT_OK )
	{
		m_eErrorCode = ERROR_DECRYPT;
		return RES_ERROR;
	}

	pBufPtr = m_pPlainBuffer;

	if ( *((DWORD *) pBufPtr) != ENCRYPT_SIGNATURE )
	{
		m_eErrorCode = ERROR_INVALID_PWD;
		return RES_ERROR;
	}

	pBufPtr += sizeof ( DWORD );

	unsigned short uKeyLen = * (unsigned short *)pBufPtr;
	pBufPtr += sizeof ( unsigned short );

	if ( uKeyLen > MAX_KEY_LEN * sizeof ( wchar_t ) )
	{
		m_eErrorCode = ERROR_INVALID_PWD;
		return RES_ERROR;
	}

	wchar_t dKeyBuf [MAX_KEY_LEN];
	memcpy ( dKeyBuf, pBufPtr, uKeyLen );
	pBufPtr += uKeyLen;

	if ( m_sKey != dKeyBuf )
	{
		m_eErrorCode = ERROR_INVALID_PWD;
		return RES_ERROR;
	}

	unsigned short uSize = * (unsigned short *)pBufPtr;
	pBufPtr += sizeof ( unsigned short );

	if ( uSize > MAX_EXT_SIZE * sizeof ( wchar_t ) )
		return RES_ERROR;

	wchar_t dExtBuf [MAX_EXT_SIZE];
	memcpy ( dExtBuf, pBufPtr, uSize );
	pBufPtr += uSize;

	m_sExt = dExtBuf;

	// generate dest file name 
	m_sDest = m_sDir + GetDecryptedFileName ();

	return RES_OK;
}


OperationResult_e Encrypter_c::EncryptNextChunk ()
{
	Assert ( m_eState == STATE_ENCRYPT );
	
	BOOL bResult;
	DWORD uBytesRead, uBytesWritten;

	DWORD uChunkSize = m_bEncrypt ? PLAIN_CHUNK_SIZE : ENC_CHUNK_SIZE;

	Assert ( m_pPlainBuffer && m_pEncBuffer );

	TRY_FUNC ( bResult, ReadFile ( m_hSource, (void *) ( m_bEncrypt ? m_pPlainBuffer : m_pEncBuffer ), uChunkSize, &uBytesRead, NULL ), FALSE, m_sSource, m_sDest );
	if ( uBytesRead )
	{
		int iErr = 0;

		if ( m_bEncrypt )
			iErr = crypt::dll_ctr_encrypt( m_pPlainBuffer, m_pEncBuffer, uBytesRead, &m_CTR );
		else
            iErr = crypt::dll_ctr_decrypt( m_pEncBuffer, m_pPlainBuffer, uBytesRead, &m_CTR );

		if ( iErr != CRYPT_OK )
			return RES_ERROR;

		TRY_FUNC ( bResult, WriteFile ( m_hDest, (void *) ( m_bEncrypt ? m_pEncBuffer : m_pPlainBuffer ), uBytesRead, &uBytesWritten, NULL ), FALSE, m_sSource, m_sDest );

		if ( uBytesRead != uBytesWritten )
			return RES_ERROR;

		m_uProcessedFile.QuadPart += uBytesRead;
		m_uProcessedTotal.QuadPart += uBytesRead;

		UpdateProgress ();
	}

	if ( uBytesRead < uChunkSize )
		return RES_FINISHED;

	return RES_OK;
}


OperationResult_e Encrypter_c::FinishFile ()
{
	DWORD dwBytes = 0;
	BOOL bResult = FALSE;

	++m_nProcessed;

	if ( m_bDelete )
	{
		if ( m_hSource )
			CloseHandle ( m_hSource );

		TRY_FUNC ( bResult, DeleteFile  ( m_sSource ), FALSE, m_sSource, m_sDest );

		m_eErrorCode = ERROR_NONE;
	}

	return RES_OK;
}

void Encrypter_c::Cancel ()
{
	switch ( m_eState )
	{
	case STATE_PREPARE_SOURCE:
	case STATE_PREPARE_DEST:
		Cleanup ( false );
		break;
	case STATE_CRYPT_FIRST:
	case STATE_ENCRYPT:
		Cleanup ( true );
		break;
	}

	m_eState = STATE_CANCELLED;
}

void Encrypter_c::SetWindow ( HWND hWnd )
{
	m_hWnd = hWnd;
}

int Encrypter_c::GetNumFiles ()
{
	return m_nFiles;
}

int Encrypter_c::GetNumProcessed ()
{
	return m_nProcessed;
}

int	Encrypter_c::GetNumResults () const
{
	return m_dErrors.Length ();
}

Encrypter_c::ErrorCode_e Encrypter_c::GetResult ( int iId ) const
{
	return m_dErrors [iId].m_eError;
}

const wchar_t * Encrypter_c::GetResultString ( int iId ) const
{
	int iTxt = m_dErrors [iId].m_eError + T_CRYPT_ERR_OK;

	Assert ( iTxt <= T_CRYPT_ERR_INV_PASS );
	return Txt ( iTxt );
}

const Str_c Encrypter_c::GetResultDir ( int iId  ) const
{
	return m_pList->m_sRootDir + m_tFileIterator.GetCachedResult ( m_dErrors [iId].m_iIndex ).m_sDir;
}

const WIN32_FIND_DATA & Encrypter_c::GetResultData ( int iId ) const
{
	return m_tFileIterator.GetCachedResult ( m_dErrors [iId].m_iIndex ).m_tData;
}

const wchar_t *	Encrypter_c::GetSourceFileName () const
{
	return m_sSource;
}

void Encrypter_c::Cleanup ( bool bError )
{
	crypt::dll_ctr_done ( &m_CTR );

	if ( m_hSource )
		CloseHandle ( m_hSource );

	if ( m_hDest )
		CloseHandle ( m_hDest );

	if ( bError )
		DeleteFile ( m_sDest );
}

void Encrypter_c::MarkSourceOk ( const Str_c & sName )
{
	if ( m_fnMarkCallback && m_tFileIterator.IsRootDir () )
		m_fnMarkCallback ( sName, false );
}

void Encrypter_c::RememberError ( OperationResult_e eResult )
{
	if ( m_eErrorCode == ERROR_NONE && eResult != RES_OK && eResult != RES_FINISHED )
		m_eErrorCode = ERROR_COMMON;
}