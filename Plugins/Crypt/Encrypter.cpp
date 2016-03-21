#include "Encrypter.h"

#include "resource.h"

extern PluginStartupInfo_t g_PSI;
extern HANDLE g_hErrorHandler;

static const wchar_t g_szCryptExt [] = L".cf";
static const DWORD ENCRYPT_SIGNATURE_V0 = 0xCFCF0001;
static const DWORD ENCRYPT_SIGNATURE_V1 = 0xCFCF0002;

#define COMMON_ERROR(code) \
{\
	ErrResponse_e eErrResponse = g_PSI.m_fnErrShowErrorDlg ( m_hWnd, code, false, m_szSource, m_szDest );\
	switch ( eErrResponse )\
	{\
	case ER_SKIP:	return RES_SKIP;\
	case ER_CANCEL:	return RES_CANCEL;\
	case ER_OVER:	break;\
	default:		Assert ( "no such response"); return RES_ERROR;\
	}\
}


//////////////////////////////////////////////////////////////////////////

Encrypter_c::Encrypter_c ( const wchar_t * szRoot, PanelItem_t ** dItems, int nItems, bool bEncrypt, bool bDelete, const AlgInfo_t & tAlg, const wchar_t * szPassword, SlowOperationCallback_t SlowOp, MarkCallback_t MarkFile  )
	: m_bEncrypt		( bEncrypt )
	, m_bDelete			( bDelete )
	, m_bDestExists		( false )
	, m_pPlainBuffer	( NULL )
	, m_pEncBuffer		( NULL )
	, m_eState			( STATE_PREPARE )
	, m_hWnd			( NULL )
	, m_hSource			( NULL )
	, m_hDest			( NULL )
	, m_dwSourceAttrib	( 0 )
	, m_uSignature		( 0 )
	, m_nProcessedFiles	( 0 )
	, m_nTotalFiles		( 0 )
	, m_nEncryptedFiles	( 0 )
	, m_nSkippedFiles	( 0 )
	, m_nErrorFiles		( 0 )
	, m_fnMarkCallback	( MarkFile )
	, m_pAlg			( &tAlg )
{
	m_pPlainBuffer = new unsigned char [PLAIN_CHUNK_SIZE];
	m_pEncBuffer = new unsigned char [ENC_CHUNK_SIZE];
	Assert ( m_pPlainBuffer && m_pEncBuffer );

	wcsncpy ( m_szKey, szPassword, MAX_PWD_CHARS );

	g_PSI.m_fnErrSetOperation ( g_hErrorHandler );

	wcsncpy ( m_szRootDir, szRoot, MAX_PATH );

	int nFolders;
	DWORD dwAInc, dwAEx;

	g_PSI.m_fnCalcSize ( szRoot, dItems, nItems, m_uTotalSize, m_nTotalFiles, nFolders, dwAInc, dwAEx, SlowOp );

	m_hIterator = g_PSI.m_fnIteratorCreate ( ITERATOR_PANEL_CACHED );
	g_PSI.m_fnIteratorStart ( m_hIterator, szRoot, dItems, nItems, true );

	memset ( m_dKey, 0, sizeof ( m_dKey ) );

	PrepareKey ();
}


Encrypter_c::~Encrypter_c ()
{
	SafeDeleteArray ( m_pPlainBuffer );
	SafeDeleteArray ( m_pEncBuffer );
	g_PSI.m_fnIteratorDestroy ( m_hIterator );
}

bool Encrypter_c::IsInDialog () const
{
	return g_PSI.m_fnErrIsInDialog ();
}

void Encrypter_c::PrepareKey ()
{
	int iKeySize = m_pAlg->m_iKeyLength;
	int iGotKeyLength = wcslen ( m_szKey ) * sizeof ( wchar_t );
	const unsigned char * pKey = (unsigned char *) m_szKey;

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
	int iId = m_pAlg->m_iId;

	int iKeySize = m_pAlg->m_iKeyLength;
	m_pAlg->m_tDesc.keysize ( &iKeySize );
	if ( m_pAlg->m_tDesc.keysize ( &iKeySize ) != CRYPT_OK )
		return false;

	int iErr = ctr_start ( iId, m_IV, m_dKey, iKeySize, 0, CTR_COUNTER_LITTLE_ENDIAN, &m_CTR );
	Assert ( iErr == CRYPT_OK );
	if ( iErr != CRYPT_OK )
		return false;
	
	return true;
}

bool Encrypter_c::GenerateNextFileName ()
{
	Assert ( m_eState == STATE_PREPARE );

	bool bRes = false;

	while ( g_PSI.m_fnIteratorNext ( m_hIterator ) )
	{
		const IteratorInfo_t * pInfo = g_PSI.m_fnIteratorInfo ( m_hIterator );
		Assert ( pInfo );

		if ( pInfo->m_b2ndPassDir )
			MarkSourceOk ( pInfo->m_pFindData->cFileName );
		else
		{
			if ( pInfo->m_pFindData && !(pInfo->m_pFindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
			{
				wcsncpy ( m_szDir, pInfo->m_szFullPath, MAX_PATH );
				wcsncpy ( m_szSource, pInfo->m_szFullPath, MAX_PATH );
				wcsncat ( m_szSource, pInfo->m_pFindData->cFileName, MAX_PATH-wcslen ( m_szSource ) );
				wcsncpy ( m_szSourceFileName, pInfo->m_pFindData->cFileName, MAX_PATH );
				g_PSI.m_fnSplitPath ( m_szSourceFileName, NULL, m_szSourceName, m_szExt );

				if ( m_szExt [0] == L'.' )
					wcsncpy ( m_szExt, &(m_szExt [1]), MAX_PATH );

				if ( m_bEncrypt )
				{
					wcsncpy ( m_szDest, m_szDir, MAX_PATH );
					wcsncat ( m_szDest, m_szSourceName, MAX_PATH - wcslen ( m_szDest ) );
					wcsncat ( m_szDest, g_szCryptExt, MAX_PATH - wcslen ( m_szDest ) );
				}
				else
					wcsncpy ( m_szDest, m_szSource, MAX_PATH );

				m_bNameFlag = true;

				bRes = true;
				break;
			}
		}	
	}

	return bRes;
}

const wchar_t * Encrypter_c::GetDecryptedFileName ()
{
	int iIndex = -1;
	int iLength = wcslen ( m_szSourceName );
	for ( int i = iLength-1; i >= 0; --i )
		if ( m_szSourceName [i] == L'_' )
		{
			iIndex = i;
			break;
		}

	if ( iIndex == -1 )
	{
		wcsncpy ( m_szDecryptedFilename, m_szSourceName, MAX_PATH );
		wcsncat ( m_szDecryptedFilename, L".", MAX_PATH - wcslen ( m_szDecryptedFilename ) );
		wcsncat ( m_szDecryptedFilename, m_szExt, MAX_PATH - wcslen ( m_szDecryptedFilename ) );
	}
	else
	{
		const wchar_t * szSourceExt = &(m_szSourceName [iIndex+1]);
		if ( wcscmp ( szSourceExt, m_szExt ) )
		{
			wcsncpy ( m_szDecryptedFilename, m_szSourceName, MAX_PATH );
			wcsncat ( m_szDecryptedFilename, L".", MAX_PATH - wcslen ( m_szDecryptedFilename ) );
			wcsncat ( m_szDecryptedFilename, m_szExt, MAX_PATH - wcslen ( m_szDecryptedFilename ) );
		}
		else
		{
			wcsncpy ( m_szDecryptedFilename, m_szSourceName, iIndex );
			wcsncat ( m_szDecryptedFilename, L".", MAX_PATH - wcslen ( m_szDecryptedFilename ) );
			wcsncat ( m_szDecryptedFilename, m_szExt, MAX_PATH - wcslen ( m_szDecryptedFilename ) );
		}
	}

	return m_szDecryptedFilename;
}

void Encrypter_c::MoveToNextFile ( OperationResult_e eResult )
{
	MoveToNext ( m_uProcessedFile, m_uSourceSize );

	++m_nProcessedFiles;

	switch ( eResult )
	{
	case RES_ERROR:	m_nErrorFiles++;	break;
	case RES_SKIP:	m_nSkippedFiles++;	break;
	}

	m_eState = STATE_PREPARE;
}

OperationResult_e Encrypter_c::PrepareSource ()
{
	BOOL bResult = FALSE;
	ULARGE_INTEGER uAvailable, uTotal;

	// check if file exists. if not - just skip
	WIN32_FIND_DATA tFindData;
	HANDLE hFound = FindFirstFile ( m_szSource, &tFindData );
	bool bSourceExists = hFound != INVALID_HANDLE_VALUE;
	if ( bSourceExists )
		FindClose ( hFound );
	else
		return RES_SKIP;

	// get source attributes
	TRY_FUNC ( m_dwSourceAttrib, GetFileAttributes ( m_szSource ), 0xFFFFFFFF, m_szSource, m_szDest );

	Assert ( ! ( m_dwSourceAttrib & FILE_ATTRIBUTE_DIRECTORY ) );

	// open source file
	TRY_FUNC ( m_hSource, CreateFile ( m_szSource, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL ), INVALID_HANDLE_VALUE, m_szSource, m_szDest );

	DWORD uBytesRead = 0;
	DWORD uBytesToRead = sizeof ( DWORD );

	// check header version
	TRY_FUNC ( bResult, ReadFile ( m_hSource, (void *) m_pPlainBuffer, uBytesToRead, &uBytesRead, NULL ), FALSE, m_szSource, m_szDest );
	if ( uBytesRead != uBytesToRead )
		return RES_ERROR;

	m_uSignature = *((DWORD *) m_pPlainBuffer);

	if ( m_bEncrypt )
	{
		// set pointer to file start
		SetFilePointer ( m_hSource, 0, NULL, FILE_BEGIN );

		// warn if already encrypted
		if ( m_uSignature == ENCRYPT_SIGNATURE_V0 || m_uSignature == ENCRYPT_SIGNATURE_V1 )
			COMMON_ERROR ( EC_ALREADY_ENCRYPTED )
		else
			m_uSignature = ENCRYPT_SIGNATURE_V1;
	}
	else
	{
		if ( m_uSignature != ENCRYPT_SIGNATURE_V0 && m_uSignature != ENCRYPT_SIGNATURE_V1 )
			COMMON_ERROR ( EC_NOT_ENCRYPTED );

		// read filename
		if ( m_uSignature == ENCRYPT_SIGNATURE_V1 )
		{
			// read number of characters
			uBytesToRead = sizeof ( DWORD );
			TRY_FUNC ( bResult, ReadFile ( m_hSource, (void *) m_pPlainBuffer, uBytesToRead, &uBytesRead, NULL ), FALSE, m_szSource, m_szDest );
			DWORD uFilenameLength = *((DWORD *) m_pPlainBuffer);
			if ( uBytesToRead != uBytesRead || uFilenameLength > MAX_PATH )
				COMMON_ERROR ( EC_CORRUPT_HEADER );

			// read filename
			uBytesToRead = uFilenameLength*sizeof (wchar_t);
			TRY_FUNC ( bResult, ReadFile ( m_hSource, (void *) m_pPlainBuffer, uBytesToRead, &uBytesRead, NULL ), FALSE, m_szSource, m_szDest );

			if ( uBytesToRead != uBytesRead )
				COMMON_ERROR ( EC_CORRUPT_HEADER );

			// construct original file name
			wcsncpy ( m_szDest, m_szDir, MAX_PATH );
			wcsncat ( m_szDest, (wchar_t*)m_pPlainBuffer, uFilenameLength );
		}

		// in _V0 header this stuff will be generated later
	}

	// get file size
	TRY_FUNC ( m_uSourceSize.LowPart, GetFileSize ( m_hSource, &m_uSourceSize.HighPart ), 0xFFFFFFFF, m_szSource, m_szDest );

	wchar_t szPath [MAX_PATH];
	g_PSI.m_fnSplitPath ( m_szDest, szPath, NULL, NULL );

	// get free space
	TRY_FUNC ( bResult, GetDiskFreeSpaceEx ( szPath, &uAvailable, &uTotal, NULL ), FALSE, m_szSource, m_szDest );

	// check available space
	int iErrAction = IDRETRY;
	while ( m_uSourceSize.QuadPart > uAvailable.QuadPart && iErrAction == IDRETRY )
		iErrAction = g_PSI.m_fnErrShowErrorDlg ( m_hWnd, EC_NOT_ENOUGH_SPACE, false, m_szSource, m_szDest );

	if ( m_uSourceSize.QuadPart > uAvailable.QuadPart )
		return iErrAction == ER_CANCEL ? RES_CANCEL : RES_ERROR;

	return RES_OK;
}

OperationResult_e Encrypter_c::PrepareDest ()
{
	DWORD uResult;

	// try to find destination
	WIN32_FIND_DATA tFindData;
	HANDLE hFound = FindFirstFile ( m_szDest, &tFindData );
	m_bDestExists = hFound != INVALID_HANDLE_VALUE;
	if ( m_bDestExists )
	{
		FindClose ( hFound );

		if ( !g_Cfg.crypt_overwrite )
			COMMON_ERROR ( EC_DEST_EXISTS );
	}

	// generate temporary file name
	TRY_FUNC ( uResult, GetTempFileName ( m_szDir, L"PFM", 0, m_szTempDest ), 0, m_szSource, m_szDest );

	// open destination file
	TRY_FUNC ( m_hDest, CreateFile ( m_szTempDest, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, m_dwSourceAttrib, NULL ), INVALID_HANDLE_VALUE, m_szSource, m_szDest );

	return RES_OK;
}

OperationResult_e Encrypter_c::EncryptFirstChunk ()
{
	PrepareIV ();

	if ( ! PrepareCryptLib () )
		return RES_ERROR;

	// write non-encrypted header first
	unsigned char * pBufPtr = m_pPlainBuffer;

	*((DWORD *) pBufPtr) = ENCRYPT_SIGNATURE_V1;
	pBufPtr += sizeof (DWORD);

	// { _V1 stuff
	DWORD uNameLen = wcslen ( m_szSourceFileName );
	DWORD uNameLenAligned = uNameLen;
	if ( uNameLenAligned % 2 )
		uNameLenAligned++;

	*((DWORD *) pBufPtr) = uNameLenAligned;
	pBufPtr += sizeof (DWORD);

	wcscpy ( (wchar_t *)pBufPtr, m_szSourceFileName );
	pBufPtr += uNameLen * sizeof (wchar_t);
	if ( uNameLen != uNameLenAligned )
	{
		*((wchar_t *)pBufPtr) = 0;
		pBufPtr += sizeof (wchar_t);
	}
	// _V1 stuff }

	*((int *)pBufPtr )= m_pAlg->m_iId;
	pBufPtr += sizeof ( int );

	memcpy ( pBufPtr, m_IV, sizeof ( m_IV ) );
	pBufPtr += sizeof ( m_IV );

	unsigned char * pSize = pBufPtr;
	// leave space for header size
	pBufPtr += sizeof ( DWORD );

	// encrypted header
	unsigned char * pCryptPtr = pBufPtr;

	DWORD uKeyLen = wcslen ( m_szKey ) + 1;
	// last wchar_t may be garbage
	if ( uKeyLen % 2 )
		uKeyLen++;

	wmemcpy ( (wchar_t*)pCryptPtr, m_szKey, uKeyLen );
	pCryptPtr += uKeyLen*sizeof (wchar_t);

	// add pseudo-random noise to fill the password gap
	for ( DWORD i = 0; i < MAX_PWD_CHARS+2-uKeyLen; ++i )
	{
		*(wchar_t *)pCryptPtr = rand();
		pCryptPtr += sizeof ( wchar_t );
	}

	// encrypted header size
	DWORD uSize = pCryptPtr - pBufPtr;
	*((DWORD *)pSize ) = uSize;

	BOOL bResult;
	DWORD uBytesWritten = 0;
	TRY_FUNC ( bResult, WriteFile ( m_hDest, (void *) m_pPlainBuffer, pBufPtr - m_pPlainBuffer, &uBytesWritten, NULL ), FALSE, m_szSource, m_szDest );
	if ( uBytesWritten != pBufPtr - m_pPlainBuffer )
		COMMON_ERROR ( EC_WRITE );

	int iErr = ctr_encrypt ( pBufPtr, m_pEncBuffer, uSize, &m_CTR );
	if ( iErr != CRYPT_OK )
		COMMON_ERROR ( EC_ENCRYPT );

	TRY_FUNC ( bResult, WriteFile ( m_hDest, (void *) m_pEncBuffer, uSize, &uBytesWritten, NULL ), FALSE, m_szSource, m_szDest );
	if ( uBytesWritten != uSize )
		COMMON_ERROR ( EC_WRITE );

	return RES_OK;
}

OperationResult_e Encrypter_c::DecryptFirstChunk ()
{
	BOOL bResult;
	DWORD uBytesRead = 0;
	DWORD uBytesToRead = sizeof ( int ) + sizeof ( m_IV ) + sizeof ( DWORD );

	// read the header
	TRY_FUNC ( bResult, ReadFile ( m_hSource, (void *) m_pPlainBuffer, uBytesToRead, &uBytesRead, NULL ), FALSE, m_szSource, m_szDest );
	if ( uBytesRead != uBytesToRead )
		COMMON_ERROR ( EC_READ );

	unsigned char * pBufPtr = m_pPlainBuffer;

	int iAlgId = *((int *)pBufPtr);
	pBufPtr += sizeof ( int );

	if ( iAlgId < 0 || iAlgId >= GetNumAlgs () )
		COMMON_ERROR ( EC_CORRUPT_HEADER );

	m_pAlg = &(GetAlgList () [ iAlgId ]);

	memcpy ( m_IV, pBufPtr, sizeof ( m_IV ) );
	pBufPtr += sizeof ( m_IV );

	if ( ! PrepareCryptLib () )
		COMMON_ERROR ( EC_DECRYPT );

	DWORD uEncHeaderSize = *((DWORD *)pBufPtr );

	TRY_FUNC ( bResult, ReadFile ( m_hSource, (void *) m_pEncBuffer, uEncHeaderSize, &uBytesRead, NULL ), FALSE, m_szSource, m_szDest );
	if ( uBytesRead != uEncHeaderSize )
		COMMON_ERROR ( EC_READ );

	int iErr = ctr_decrypt ( m_pEncBuffer, m_pPlainBuffer, uEncHeaderSize, &m_CTR );
	if ( iErr != CRYPT_OK )
		COMMON_ERROR ( EC_DECRYPT );

	pBufPtr = m_pPlainBuffer;
	wchar_t dKeyBuf [MAX_PWD_CHARS+1];

	if ( m_uSignature == ENCRYPT_SIGNATURE_V1 )
	{
		// { _V1 stuff
		int nPwdChars = 0;
		while ( *(wchar_t*)pBufPtr && nPwdChars < MAX_PWD_CHARS )
		{
			dKeyBuf [nPwdChars++] = *(wchar_t*)pBufPtr;
			pBufPtr += sizeof (wchar_t);
		}

		dKeyBuf [nPwdChars] = L'\0';
		if ( wcscmp ( m_szKey, dKeyBuf ) )
			COMMON_ERROR ( EC_INVPASS );
		// _V1 stuff }
	}
	else
	{
		if ( *((DWORD *) pBufPtr) != m_uSignature )
			COMMON_ERROR ( EC_INVPASS );

		pBufPtr += sizeof ( DWORD );

		unsigned short uKeyLen = * (unsigned short *)pBufPtr;
		pBufPtr += sizeof ( unsigned short );

		if ( uKeyLen > MAX_PWD_CHARS * sizeof ( wchar_t ) )
			COMMON_ERROR ( EC_INVPASS );

		memcpy ( dKeyBuf, pBufPtr, uKeyLen );
		pBufPtr += uKeyLen;

		if ( wcscmp ( m_szKey, dKeyBuf ) )
			COMMON_ERROR ( EC_INVPASS );

		unsigned short uSize = * (unsigned short *)pBufPtr;
		pBufPtr += sizeof ( unsigned short );

		if ( uSize > MAX_PATH * sizeof ( wchar_t ) )
			COMMON_ERROR ( EC_CORRUPT_HEADER );

		wchar_t dExtBuf [MAX_PATH];
		memcpy ( dExtBuf, pBufPtr, uSize );
		pBufPtr += uSize;

		wcscpy ( m_szExt, dExtBuf );

		// generate dest file name 
		wcsncpy ( m_szDest, m_szDir, MAX_PATH );
		wcsncat ( m_szDest, GetDecryptedFileName (), MAX_PATH-wcslen (m_szDest) );
	}

	return RES_OK;
}


OperationResult_e Encrypter_c::EncryptDecryptNextChunk ()
{
	Assert ( m_eState == STATE_ENCRYPT );
	
	BOOL bResult;
	DWORD uBytesRead, uBytesWritten;

	DWORD uChunkSize = m_bEncrypt ? PLAIN_CHUNK_SIZE : ENC_CHUNK_SIZE;

	Assert ( m_pPlainBuffer && m_pEncBuffer );

	TRY_FUNC ( bResult, ReadFile ( m_hSource, (void *) ( m_bEncrypt ? m_pPlainBuffer : m_pEncBuffer ), uChunkSize, &uBytesRead, NULL ), FALSE, m_szSource, m_szDest );
	if ( uBytesRead )
	{
		int iErr = 0;

		if ( m_bEncrypt )
			iErr = ctr_encrypt( m_pPlainBuffer, m_pEncBuffer, uBytesRead, &m_CTR );
		else
            iErr = ctr_decrypt( m_pEncBuffer, m_pPlainBuffer, uBytesRead, &m_CTR );

		if ( iErr != CRYPT_OK )
			COMMON_ERROR ( m_bEncrypt ? EC_ENCRYPT : EC_DECRYPT );

		TRY_FUNC ( bResult, WriteFile ( m_hDest, (void *) ( m_bEncrypt ? m_pEncBuffer : m_pPlainBuffer ), uBytesRead, &uBytesWritten, NULL ), FALSE, m_szSource, m_szDest );

		if ( uBytesRead != uBytesWritten )
			COMMON_ERROR ( EC_WRITE );

		m_uProcessedFile.QuadPart += uBytesRead;
		m_uProcessedTotal.QuadPart += uBytesRead;
	}

	if ( uBytesRead < uChunkSize )
		return RES_FINISHED;

	return RES_OK;
}


OperationResult_e Encrypter_c::FinishFile ()
{
	DWORD dwBytes = 0;
	BOOL bResult = FALSE;

	if ( m_hSource )
	{
		CloseHandle ( m_hSource );
		m_hSource = NULL;
	}

	if ( m_hDest )
	{
		CloseHandle ( m_hDest );
		m_hDest = NULL;
	}

	if ( m_bDestExists )
	{
		DWORD dwDestAttrib;

		// get dest attributes
		TRY_FUNC ( dwDestAttrib, GetFileAttributes ( m_szDest ), 0xFFFFFFFF, m_szSource, m_szDest );

		DWORD dwAttribToSet = dwDestAttrib;
		dwAttribToSet &= ~ ( FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM );
		TRY_FUNC ( bResult, SetFileAttributes ( m_szDest, dwAttribToSet ), FALSE, m_szSource, m_szDest );
		TRY_FUNC ( bResult, DeleteFile  ( m_szDest ), FALSE, m_szSource, m_szDest );
	}

	TRY_FUNC ( bResult, MoveFile ( m_szTempDest, m_szDest ), FALSE, m_szSource, m_szDest );

	if ( m_bDelete )
		TRY_FUNC ( bResult, DeleteFile  ( m_szSource ), FALSE, m_szSource, m_szDest );

	m_nEncryptedFiles++;

	return RES_OK;
}

bool Encrypter_c::DoWork ()
{
	OperationResult_e eResult = RES_OK;
	OperationResult_e eFinalResult = RES_OK;

	switch ( m_eState )
	{
	case STATE_PREPARE:
		if ( !GenerateNextFileName () )
			return false;

		// generate file name, open files
		eResult = PrepareSource ();
		switch ( eResult )
		{
		case RES_SKIP:
		case RES_ERROR:
			Cleanup ();
			MoveToNextFile ( eResult );
			return true;
		case RES_CANCEL:
			Cleanup ();
			return false;
		}

		// for legacy files PrepareDest must be called AFTER DecryptFirstChunk
		if ( m_uSignature == ENCRYPT_SIGNATURE_V1 )
		{
			eResult = PrepareDest ();
			switch ( eResult )
			{
			case RES_SKIP:
			case RES_ERROR:
				Cleanup ();
				MoveToNextFile ( eResult );
				return true;
			case RES_CANCEL:
				Cleanup ();
				return false;
			}
		}

		// encrypt the very first chunk (signature, file name)
		eResult = m_bEncrypt ? EncryptFirstChunk () : DecryptFirstChunk ();
		switch ( eResult )
		{
		case RES_SKIP:	
		case RES_ERROR:
			Cleanup ();
			MoveToNextFile ( eResult );
			return true;

		case RES_CANCEL:
			Cleanup ();
			return false;
		}

		if ( m_uSignature == ENCRYPT_SIGNATURE_V0 )
		{
			eResult = PrepareDest ();
			switch ( eResult )
			{
			case RES_SKIP:
			case RES_ERROR:
				Cleanup ();
				MoveToNextFile ( eResult );
				return true;
			case RES_CANCEL:
				Cleanup ();
				return false;
			}
		}

		m_eState = STATE_ENCRYPT;
		break;

	case STATE_ENCRYPT:
		eResult = EncryptDecryptNextChunk ();
		UpdateProgress ();

		switch ( eResult )
		{
		case RES_SKIP:	
		case RES_ERROR:
			Cleanup ();
			MoveToNextFile ( eResult );
			return true;

		case RES_FINISHED:
			eFinalResult = FinishFile ();
			switch ( eFinalResult )
			{
			case RES_SKIP:
			case RES_ERROR:
				Cleanup ();
				MoveToNextFile ( eFinalResult );
				return true;

			case RES_CANCEL:
				Cleanup ();
				return false;
			}

			Cleanup ();
			MarkSourceOk ( m_szSourceFileName );
			MoveToNextFile ( eResult );
			return true;

		case RES_CANCEL:
			Cleanup ();
			return false;
		}
		break;
	}

	return true;
}

void Encrypter_c::Cancel ()
{
	Cleanup ();
	m_eState = STATE_CANCELLED;
}

void Encrypter_c::Cleanup ()
{
	ctr_done ( &m_CTR );

	if ( m_hSource )
	{
		CloseHandle ( m_hSource );
		m_hSource = NULL;
	}

	if ( m_hDest )
	{
		CloseHandle ( m_hDest );
		m_hDest = NULL;
	}

	DeleteFile ( m_szTempDest );
}

void Encrypter_c::MarkSourceOk ( const wchar_t * szName )
{
	if ( m_fnMarkCallback && g_PSI.m_fnIteratorInfo ( m_hIterator )->m_bRootDir )
		m_fnMarkCallback ( szName, false );
}