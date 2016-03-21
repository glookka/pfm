#include "pch.h"
#include "protection.h"

#include "crypt/shark.h"
#include "crypt/tomcrypt.h"

namespace protection
{
	const int		KEY_LEN			= 14;
	const int		KEY_LEN_WITH_SIG= 24;

	const DWORD		DATE_SIG		= 0xDADE;
	const DWORD		KEY_SIG			= 0xBABE;
	const DWORD		KEY_SIGR		= 0xFCBA;

	// encryption in registry
	const int		KEY_STORE_LEN	= 8;
	unsigned char	KEY_STORE_KEY [KEY_STORE_LEN] = { 0x4E, 0x0C, 0xE3, 0x38, 0x57, 0xC4, 0x7E, 0x06 };

	const int		HASH_SIZE_BYTES = 16;

	int				interval	   = 0;
	const int		trial_period   = 14;
	int				days_left	   = trial_period;

	bool			rlang		   = false;
	bool			registered     = false;
	bool			expired        = false;
	bool			nag_shown	   = false;
	Str_c			key;

	const unsigned char g_dHashTable [] =
	{
#if FM_BUILD_HANDANGO
	#include "hashes/keys_handango.inc"
#endif

#if FM_BUILD_POCKETGEAR
	#include "hashes/keys_pocketgear.inc"
#endif

#if FM_BUILD_CLICKAPPS
	#include "hashes/keys_clickapps.inc"
#endif

#if FM_BUILD_POCKETLAND
	#include "hashes/keys_pocketland.inc"
#endif

#if FM_BUILD_POCKETSELECT
	#include "hashes/keys_pocketselect.inc"
#endif

#if FM_BUILD_PDATOPSOFT
	#include "hashes/keys_pdatopsoft.inc"
#endif
	};
	

	Str_c DecString ( int iIndex, const unsigned short * dBuf, int iStringLen )
	{
		wchar_t szBuf [32];

		const unsigned short uXorKey = 0x0811;

		for ( int i = 0; i < iStringLen; ++i )
		{
			unsigned short uChar = dBuf [iIndex*iStringLen + i];
			if ( uChar )
				szBuf [i] = uChar ^ uXorKey;
			else
			{
				szBuf [i] = L'\0';
				break;
			}
		}

		return szBuf;
	}

	bool CheckRus ()
	{
		const unsigned short dDays [] =
		{ 
			0xC23, 0xC2F, 0xC50, 0xC2B, 0xC51, 0xC24, 0xC50, 0xC24, 0xC2C, 0xC5D, 0xC24, 0x0,
			0xC2E, 0xC2F, 0xC2C, 0xC24, 0xC25, 0xC24, 0xC2A, 0xC5D, 0xC2C, 0xC29, 0xC2B, 0x0,
			0xC23, 0xC53, 0xC2F, 0xC51, 0xC2C, 0xC29, 0xC2B, 0x000, 0x000, 0x000, 0x000, 0x0,
			0xC50, 0xC51, 0xC24, 0xC25, 0xC21, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x0,
			0xC56, 0xC24, 0xC53, 0xC23, 0xC24, 0xC51, 0xC22, 0x000, 0x000, 0x000, 0x000, 0x0,
			0xC2E, 0xC5E, 0xC53, 0xC2C, 0xC29, 0xC57, 0xC21, 0x000, 0x000, 0x000, 0x000, 0x0,
			0xC50, 0xC52, 0xC20, 0xC20, 0xC2F, 0xC53, 0xC21, 0x000, 0x000, 0x000, 0x000, 0x0
		};

		const unsigned short dMonths [] =
		{
			0xC5E, 0xC2C, 0xC23, 0xC21, 0xC51, 0xC5D, 0x000, 0x000, 0x0,
			0xC55, 0xC24, 0xC23, 0xC51, 0xC21, 0xC2A, 0xC5D, 0x000, 0x0,
			0xC2D, 0xC21, 0xC51, 0xC53, 0x000, 0x000, 0x000, 0x000, 0x0,
			0xC21, 0xC2E, 0xC51, 0xC24, 0xC2A, 0xC5D, 0x000, 0x000, 0x0,
			0xC2D, 0xC21, 0xC28, 0x000, 0x000, 0x000, 0x000, 0x000, 0x0,
			0xC29, 0xC5F, 0xC2C, 0xC5D, 0x000, 0x000, 0x000, 0x000, 0x0,
			0xC29, 0xC5F, 0xC2A, 0xC5D, 0x000, 0x000, 0x000, 0x000, 0x0,
			0xC21, 0xC23, 0xC22, 0xC52, 0xC50, 0xC53, 0x000, 0x000, 0x0,
			0xC50, 0xC24, 0xC2C, 0xC53, 0xC5E, 0xC20, 0xC51, 0xC5D, 0x0,
			0xC2F, 0xC2B, 0xC53, 0xC5E, 0xC20, 0xC51, 0xC5D, 0x000, 0x0,
			0xC2C, 0xC2F, 0xC5E, 0xC20, 0xC51, 0xC5D, 0x000, 0x000, 0x0,
			0xC25, 0xC24, 0xC2B, 0xC21, 0xC20, 0xC51, 0xC5D, 0x000, 0x0
		};


		SYSTEMTIME tTime;
		GetLocalTime ( &tTime );

		Str_c sMonth = DecString ( tTime.wMonth - 1, dMonths, 9 );
		Str_c sDay = DecString ( tTime.wDayOfWeek, dDays, 12 );

		Str_c sKeyToCheck = key;
		sKeyToCheck.ToLower ();

		if ( sKeyToCheck.Find ( sMonth ) == -1 )
			return false;

		if ( sKeyToCheck.Find ( sDay ) == -1 )
			return false;

		return true;
	}


	void Encrypt ( unsigned char * dData, int iDataSize, unsigned char * dKey, int iKeySize )
	{
		Assert ( iDataSize % 8 == 0 );

		shark * pShark = new shark ( dKey, iKeySize );

		ddword tRes;
		int nBlocks = iDataSize / 8;

		for( int i = 0; i < nBlocks; i++)
		{
			ddword tData;
			tData.w [0] = (( long * ) dData )[i*2];
			tData.w [1] = (( long * ) dData )[i*2 + 1];

			tRes = pShark->encryption ( tData );
			(( long * ) dData )[i*2]	= tRes.w [0];
			(( long * ) dData )[i*2 + 1]= tRes.w [1];
		}

		delete pShark;
	}

	void Decrypt ( unsigned char * dCode, int iCodeSize, unsigned char * dKey, int iKeySize )
	{
		Assert ( iCodeSize % 8 == 0 );

		shark * pShark = new shark ( dKey, iKeySize );

		ddword tRes;
		int nBlocks = iCodeSize / 8;

		for( int i = 0; i < nBlocks; i++)
		{
			ddword tCode;
			tCode.w [0] = (( long * ) dCode )[i*2];
			tCode.w [1] = (( long * ) dCode )[i*2 + 1];

			tRes = pShark->decryption ( tCode );
			(( long * ) dCode )[i*2]	= tRes.w [0];
			(( long * ) dCode )[i*2 + 1]= tRes.w [1];
		}

		delete pShark;
	}


	int HashCompare ( const BYTE * pHash1, const BYTE * pHash2 )
	{
		const int HASH_SIZE_DWORDS = HASH_SIZE_BYTES / sizeof ( DWORD );
		DWORD uTmp1, uTmp2;
		for ( int i = 0; i < HASH_SIZE_DWORDS; ++i )
		{
			uTmp1 = ((DWORD *)pHash1)[i];
			uTmp2 = ((DWORD *)pHash2)[i];

			if ( uTmp1 < uTmp2 )
				return -1;

			if ( uTmp1 > uTmp2 )
				return 1;
		}

		return 0;
	}


	int FindHash ( const BYTE * pKeyHash )
	{
		int nTotal = sizeof ( g_dHashTable ) / HASH_SIZE_BYTES;

		int low = 0; 
		int high = nTotal;

		while ( true )
		{
			int b = low + ( high - low ) / 2;

			if ( b == high )
				return ( b < nTotal && HashCompare ( pKeyHash, &(g_dHashTable [b*HASH_SIZE_BYTES]) ) == 0 ) ? b : -1;

			int iCompRes = HashCompare ( pKeyHash, &(g_dHashTable [b*HASH_SIZE_BYTES]) );

			if ( iCompRes > 0 )
				low = b + 1;
			else
				if ( iCompRes < 0 )
					high = b;
				else
					high = low = b;
		}
	}


	bool CheckKey ()
	{
		HKEY hKey;

		// get signature hash from registry
		if ( RegOpenKeyEx ( HKEY_CURRENT_USER, L"\\Software\\PFM\\Cache", 0, 0, &hKey ) != ERROR_SUCCESS )
			return false;

		DWORD dwType = REG_BINARY;
		DWORD dwSize = 256;
		unsigned char dKeyCrypted [256];
		memset ( dKeyCrypted, 0, dwSize );
		LONG iRes = RegQueryValueEx ( hKey, L"Icons", 0, &dwType, (PBYTE)dKeyCrypted, &dwSize );
		RegCloseKey ( hKey );

		if  ( iRes != ERROR_SUCCESS )
			return false;

		if ( dwSize != KEY_LEN_WITH_SIG )
			return false;

		Decrypt ( dKeyCrypted, KEY_LEN_WITH_SIG, KEY_STORE_KEY, KEY_STORE_LEN );

		if ( *(DWORD *) dKeyCrypted == KEY_SIGR )
		{
			rlang = true;
			return true;
		}

		if ( *(DWORD *) dKeyCrypted != KEY_SIG )
			return false;

		BYTE * pDecryptedKey = (BYTE  *)( & ( dKeyCrypted [4] ) );
		pDecryptedKey [KEY_LEN] = '\0';

		// check the key
		unsigned char dHash [HASH_SIZE_BYTES];

		int iHR;
		hash_state md;
		md2_init ( &md );
		iHR = md2_process ( &md, pDecryptedKey, KEY_LEN );
		Assert ( iHR == CRYPT_OK );
		iHR = md2_done ( &md, dHash );
		Assert ( iHR == CRYPT_OK );

		return FindHash ( dHash ) != -1;
	}


	bool CheckDate ()
	{
		HKEY hKey;
		if ( RegOpenKeyEx ( HKEY_CURRENT_USER, L"\\Software\\PFM\\Cache", 0, 0, &hKey ) != ERROR_SUCCESS )
			return true;

		DWORD dwType = REG_BINARY;
		DWORD dwSize = 256;
		unsigned char dDateCrypted [256];
		memset ( dDateCrypted, 0, dwSize );
		if ( RegQueryValueEx ( hKey, L"Icons", 0, &dwType, (PBYTE)dDateCrypted, &dwSize ) != ERROR_SUCCESS )
			return true;

		if ( dwSize != KEY_LEN_WITH_SIG )
			return true;

		Decrypt ( dDateCrypted, KEY_LEN_WITH_SIG, KEY_STORE_KEY, KEY_STORE_LEN );
		if ( *(DWORD *)dDateCrypted != DATE_SIG )
			return true;

		SYSTEMTIME tRegTime = *(SYSTEMTIME * )( ((DWORD *)dDateCrypted) + 1 );
		SYSTEMTIME tCurTime;
		FILETIME tRegFTime, tCurFTime;
		GetSystemTime ( &tCurTime );
		
		SystemTimeToFileTime ( &tRegTime, &tRegFTime );
		SystemTimeToFileTime ( &tCurTime, &tCurFTime );

		ULARGE_INTEGER tRegTimeAsInt, tCurTimeAsInt, tTimeDiff;
		memcpy ( &tRegTimeAsInt, &tRegFTime, sizeof ( tRegFTime ) );
		memcpy ( &tCurTimeAsInt, &tCurFTime, sizeof ( tCurFTime ) );

		// something's fucked up
		if ( tCurTimeAsInt.QuadPart < tRegTimeAsInt.QuadPart )
			return true;
			
		// convert 100-nanosecond intervals to seconds
		tTimeDiff.QuadPart = ( tCurTimeAsInt.QuadPart - tRegTimeAsInt.QuadPart ) / 10000000;

		// more than our trial?
		if ( tTimeDiff.QuadPart >= trial_period*86400 )
			return false;

		days_left = trial_period - int ( tTimeDiff.QuadPart / 86400 );
		if ( days_left <= 0 )
			days_left = trial_period;

		return true;
	}


	void Init ()
	{
		interval = rand () % 20 + 40;
	}


	void Shutdown ()
	{
		DWORD uKeySig = KEY_SIG;

		if ( CheckRus () )
			uKeySig = KEY_SIGR;
		else
			if ( key.Length () != KEY_LEN )
				return;

		HKEY hKey;
		DWORD dwDisposition;

		if ( RegCreateKeyEx ( HKEY_CURRENT_USER, L"\\Software\\PFM\\Cache", 0, NULL, 0, 0, NULL, &hKey, &dwDisposition ) != ERROR_SUCCESS )
			return;

		unsigned char dKey [256];
		*(DWORD *)dKey = uKeySig;

		if ( uKeySig != KEY_SIGR )
			WideCharToMultiByte ( CP_ACP, 0, key, -1, ((char *)dKey ) + 4, 256, NULL, NULL );

		Encrypt ( dKey, KEY_LEN_WITH_SIG, KEY_STORE_KEY, KEY_STORE_LEN );

		RegSetValueEx ( hKey, L"Icons", 0, REG_BINARY, dKey, KEY_LEN_WITH_SIG );
		RegCloseKey ( hKey );
	}
}