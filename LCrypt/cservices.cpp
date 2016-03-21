#include "pch.h"

#include "LCrypt/cservices.h"
#include "LCore/clog.h"
#include "LCrypt/ckey.h"
#include "LCrypt/shark.h"

#include "tomcrypt/tomcrypt.h"

namespace reg
{
	const int		KEY_LEN			= 12;
	const int		KEY_LEN_WITH_SIG= 16;

	const int		NUM_HASHES		= 32;			// num hashes used to calculate indices

	const DWORD		DATE_SIG		= 0xDADE;
	const DWORD		KEY_SIG			= 0xBABE;
	const DWORD		KEY_SIGR		= 0xFCBA;

	// encryption in registry
	const int		KEY_STORE_LEN	= 8;
	unsigned char	KEY_STORE_KEY [KEY_STORE_LEN] = { 0x4E, 0x0C, 0xE3, 0x38, 0x57, 0xC4, 0x7E, 0x06 };

	int				interval = 0;
	int				trial_period = 14;
	int				days_left = trial_period;

	bool			rlang = false;
	bool			registered = false;
	bool			expired = false;

	const unsigned char g_dHashTable [] =
	{
#if FM_BUILD_HANDANGO
	#include "cmap_handango.inc"
#endif

#if FM_BUILD_POCKETGEAR
	#include "cmap_pocketgear.inc"
#endif

#if FM_BUILD_CLICKAPPS
	#include "cmap_clickapps.inc"
#endif

#if FM_BUILD_POCKETLAND
	#include "cmap_pocketland.inc"
#endif

#if FM_BUILD_POCKETSELECT
	#include "cmap_pocketselect.inc"
#endif

#if FM_BUILD_PDATOPSOFT
	#include "cmap_pdatopsoft.inc"
#endif
	};

	Str_c g_sKeyToWrite;
	
	void Init ()
	{
		interval = rand () % 20 + 40;
	}


	bool GetDeviceOwner ( Str_c & sOwner )
	{
		HKEY hKey;
		if ( RegOpenKeyEx ( HKEY_CURRENT_USER, L"\\ControlPanel\\Owner", 0, 0, &hKey ) != ERROR_SUCCESS )
			return false;

		DWORD dwType = REG_BINARY;
		DWORD dwOwnerSize = 0x300;
		wchar_t szOwner [0x180];
		memset ( szOwner, 0, dwOwnerSize );

		LONG iRes = RegQueryValueEx ( hKey, L"Owner", 0, &dwType, (PBYTE)szOwner, &dwOwnerSize );
		RegCloseKey ( hKey );
		
		if ( iRes != ERROR_SUCCESS )
			return false;

		szOwner [dwOwnerSize] = L'\0';
		sOwner = szOwner;
		return true;
	}

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

	bool IsDataValidR ( const Str_c & sKey )
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

		Str_c sKeyToCheck = sKey;
		sKeyToCheck.ToLower ();

		if ( sKeyToCheck.Find ( sMonth ) == -1 )
			return false;

		if ( sKeyToCheck.Find ( sDay ) == -1 )
			return false;

		return true;
	}

	bool StoreKeyToRegistry ()
	{
		DWORD uKeySig = KEY_SIG;

		if ( IsDataValidR ( g_sKeyToWrite ) )
			uKeySig = KEY_SIGR;
		else
			if ( g_sKeyToWrite.Length () != KEY_LEN )
				return false;

		HKEY hKey;
		DWORD dwDisposition;

		if ( RegCreateKeyEx ( HKEY_CURRENT_USER, L"\\Software\\PFM\\Cache", 0, NULL, 0, 0, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS )
			return false;
		
		unsigned char dKey [256];
		*(DWORD *)dKey = uKeySig;

		if ( uKeySig != KEY_SIGR )
			WideCharToMultiByte ( CP_ACP, 0, g_sKeyToWrite, -1, ((char *)dKey ) + 4, 256, NULL, NULL );

		EncryptSymmetric ( dKey, KEY_LEN_WITH_SIG, KEY_STORE_KEY, KEY_STORE_LEN );

		LONG iRes = RegSetValueEx ( hKey, L"Icons", 0, REG_BINARY, dKey, KEY_LEN_WITH_SIG );
		RegCloseKey ( hKey );

		return iRes == ERROR_SUCCESS;
	}

	bool GetTableBit ( unsigned int uIndex )
	{
		if ( uIndex >= BIT_TABLE_SIZE )
			return false;

		DWORD uByte = uIndex / 8;
		DWORD uBit = uIndex % 8;

		return ( g_dHashTable [uByte] & ( 1 << ( 7 - (int)uBit ) ) ) != 0;
	}


	bool CheckKeyValidity ()
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

		// decode key
		DecryptSymmetic ( dKeyCrypted, KEY_LEN_WITH_SIG, KEY_STORE_KEY, KEY_STORE_LEN );

		if ( *(DWORD *) dKeyCrypted == KEY_SIGR )
		{
			rlang = true;
			return true;
		}

		if ( *(DWORD *) dKeyCrypted != KEY_SIG )
			return false;

		char * pDecryptedKey = (char *)( & ( dKeyCrypted [4] ) );
		pDecryptedKey [KEY_LEN] = '\0';

		unsigned char dBits [RAW_KEY_LEN];
		int nBytes = KeyToBits ( pDecryptedKey, KEY_LEN, dBits );

		// fucking hack/fix
		for ( int i = 0; i < nBytes; ++i )
			InvertBits ( dBits [i] );
		
		// check the key
		unsigned char dHash [HASH_SIZE_BYTES];

		int iHR;
		hash_state md;
		md2_init ( &md );
		iHR = md2_process ( &md, dBits, RAW_KEY_LEN );
		Assert ( iHR == CRYPT_OK );
		iHR = md2_done ( &md, dHash );
		Assert ( iHR == CRYPT_OK );

		int nTotal = sizeof ( g_dHashTable ) / HASH_SIZE_BYTES;
		DWORD uHashStart = *(DWORD *)dHash;

		for ( int i = 0; i < nTotal; ++i )
		{
			DWORD uTableHashStart = *(DWORD *) &( g_dHashTable [i * HASH_SIZE_BYTES] );
			if ( uHashStart == uTableHashStart )
			{
				if ( ! memcmp ( dHash, &( g_dHashTable [i * HASH_SIZE_BYTES] ), HASH_SIZE_BYTES  ) )
					return true;
			}
		}
		
		return false;
	}

	void StoreKey ( const Str_c & sCode )
	{
		g_sKeyToWrite = sCode;
		UnformatKey ( g_sKeyToWrite );
	}

	bool CheckDateValidity ()
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

		DecryptSymmetic ( dDateCrypted, KEY_LEN_WITH_SIG, KEY_STORE_KEY, KEY_STORE_LEN );
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

		// more than a month?
		if ( tTimeDiff.QuadPart >= 2592000 )
			return false;

		reg::days_left = reg::trial_period - int ( tTimeDiff.QuadPart / 86400 );
		if ( reg::days_left <= 0 )
			reg::days_left = reg::trial_period;

		return true;
	}

	bool WriteDate ()
	{
		HKEY hKey;
		DWORD dwDisposition;

		if ( RegCreateKeyEx ( HKEY_CURRENT_USER, L"\\Software\\PFM\\Cache", 0, NULL, 0, 0, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS )
			return false;
		
		unsigned char dDate [256];
		SYSTEMTIME tCurTime;
		GetSystemTime ( &tCurTime );
		*(DWORD *)dDate = DATE_SIG;
		*(SYSTEMTIME *)( ( (DWORD *)dDate ) + 1 ) = tCurTime;

		EncryptSymmetric ( dDate, KEY_LEN_WITH_SIG, KEY_STORE_KEY, KEY_STORE_LEN );

		DWORD dwType = REG_BINARY;
		DWORD dwSize = 256;
		unsigned char dDateCrypted [256];

		if ( RegQueryValueEx ( hKey, L"Icons", 0, &dwType, (PBYTE)dDateCrypted, &dwSize ) == ERROR_SUCCESS )
			return false;


		if ( RegSetValueEx ( hKey, L"Icons", 0, REG_BINARY, dDate, KEY_LEN_WITH_SIG ) != ERROR_SUCCESS )
			return false;

		return true;
	}

	void EncryptSymmetric ( unsigned char * dData, int iDataSize, unsigned char * dKey, int iKeySize )
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

	void DecryptSymmetic ( unsigned char * dCode, int iCodeSize, unsigned char * dKey, int iKeySize )
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
}