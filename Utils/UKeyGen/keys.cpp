#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <map>

#include "LCrypt/ckey.h"
#include "LCrypt/tomcrypt/tomcrypt.h"


#pragma warning( disable : 4018 4996 )

const int NUM_KEYS = 1024;
DWORD g_uStartSeed = 0;					// 0 as start seed
std::string g_sIncludeKeys;				// no include keys
std::string g_sExcludeKeys;				// no exclude keys

bool g_bLineBreaks = false;

std::vector < ULARGE_INTEGER > g_dKeys;
std::vector < ULARGE_INTEGER > g_dIncludeKeys;
std::vector < ULARGE_INTEGER > g_dExcludeKeys;


void GenerateRandomKey ( ULARGE_INTEGER & uKey )
{
	uKey.LowPart = ( (DWORD) rand () << 16 ) | ((DWORD) rand ());
	uKey.HighPart = ( (DWORD) rand () << 16 ) | ((DWORD) rand ());
	
	// truncate to 60 bits ( 12-letter key )
	uKey.HighPart &= 0xFFFFFFF;
}


void DumpData ()
{
	FILE * pFile = fopen ( "cmap.h", "wt" );
	FILE * pKeyFile = fopen ( "keys.txt", "wt" );
	FILE * pFileFormatted = fopen ( "keys_formatted.txt", "wt" );

	if ( ! pFile || ! pKeyFile || ! pFileFormatted )
	{
		printf ( "Error writing results\n" );
		return;
	}

	unsigned char dHash [reg::HASH_SIZE_BYTES];
	char szKey [128];

	int nInLine = 0;
	for ( int i = 0; i < g_dKeys.size (); ++i  )
	{
		// raw key
		ULARGE_INTEGER uKey = g_dKeys [i];
		
		bool bWhite = false;
		for ( int j = 0; j < g_dIncludeKeys.size () && ! bWhite; ++j )
			if ( uKey.QuadPart == g_dIncludeKeys [j].QuadPart )
				bWhite = true;

		fprintf ( pKeyFile, "%d: %X %X\n", i, uKey.HighPart, uKey.LowPart );
		
		// hash
		unsigned char * dBits = (unsigned char *) &uKey;

		hash_state md;
		md2_init ( &md );
		md2_process ( &md, dBits, reg::RAW_KEY_LEN );
		md2_done ( &md, dHash );

		for ( int j = 0; j < reg::HASH_SIZE_BYTES; ++j )
			fprintf ( pFile, "0x%X, ", dHash [j] );			

		if ( ++nInLine == 6 )
		{
			nInLine = 0;
			fprintf ( pFile, "\n" );
		}

		if ( ! bWhite )
		{
			// formatted key
			for ( int i = 0; i < reg::RAW_KEY_LEN; ++i )
				reg::InvertBits ( dBits [i] );

			reg::BitsToKey ( dBits, 60, szKey );
			std::string sKey = szKey;
			reg::FormatKey ( sKey, 4 );
			if ( g_bLineBreaks )
				fprintf ( pFileFormatted, "%s\n", sKey.c_str () );
			else
				fprintf ( pFileFormatted, "%s,", sKey.c_str () );
		}
	}

	fclose ( pFileFormatted );
	fclose ( pKeyFile );
	fclose ( pFile );
}

bool ParseParams ( int argc, const char * argv [] )
{
	bool bHaveSeed = false;

	int i = 1;
	while ( i < argc )
	{
		const char * szParam = argv [i];
		if ( !strcmp ( szParam, "-?" ) )
		{
			printf ( "params:\n" );
			printf ( "-? - print help\n" );
			printf ( "-s XXX - start seed (hex)\n" );
			printf ( "-l linebreaks instead of ',' \n" );
			printf ( "-w <file> - white list\n" );
			printf ( "-b <file> - black list\n" );
			return false;
		}

		if ( !strcmp ( szParam, "-s" ) )
		{
			bHaveSeed = true;
			sscanf ( argv [i+1], "%X", &g_uStartSeed );
			++i;
		}

		if ( !strcmp ( szParam, "-l" ) )
			g_bLineBreaks = true;

		if ( !strcmp ( szParam, "-w" ) )
		{
			g_sIncludeKeys = argv [i+1];
			++i;
		}

		if ( !strcmp ( szParam, "-b" ) )
		{
			g_sExcludeKeys = argv [i+1];
			++i;
		}

		++i;
	}

	if ( ! bHaveSeed )
	{
		// randomize
		LARGE_INTEGER uLarge;
		QueryPerformanceCounter ( &uLarge);
		g_uStartSeed = uLarge.LowPart ^ uLarge.HighPart;
	}

	return true;
}


bool ReadKeyList ( const std::string & sFile, std::vector <ULARGE_INTEGER> & dKeys )
{
	FILE * pFile = fopen ( sFile.c_str (), "rt" );
	if ( ! pFile )
	{
		printf ( "Can't open file [%s]\n", sFile.c_str () );
		return false;
	}

	char szKey [256];

	while ( ! feof ( pFile ) )
	{
		fgets ( szKey, 256, pFile );
		int iLen = strlen ( szKey );
		while ( iLen > 0 && ( szKey [iLen-1] == '\r' || szKey [iLen-1] == '\n' ) )
		{
			szKey [iLen-1] = '\0';
			--iLen;
		}

		unsigned char dBits [reg::RAW_KEY_LEN];
		std::string sKey = szKey;
		reg::UnformatKey ( sKey );
		int nBytes = reg::KeyToBits ( sKey.c_str (), (int)sKey.size (), dBits );

		// fucking hack/fix
		for ( int i = 0; i < nBytes; ++i )
			reg::InvertBits ( dBits [i] );

		ULARGE_INTEGER * pKey = (ULARGE_INTEGER *) &dBits;
		dKeys.push_back ( *pKey );
	}
	
	fclose ( pFile );

	return true;
}


bool VerifyParams ()
{
	srand ( g_uStartSeed );

	printf ( "Generation params:\n"	);
	printf ( "------------------\n"	);
	printf ( "Keys to generate: %d\n", NUM_KEYS );
	printf ( "Start seed: %X\n", g_uStartSeed );

	if ( ! g_sIncludeKeys.empty () )
		printf ( "White list: %s\n", g_sIncludeKeys.c_str () );

	if ( ! g_sExcludeKeys.empty () )
		printf ( "Black list: %s\n", g_sExcludeKeys.c_str () );

	printf ( "------------------\n"	);

	if ( ! g_sIncludeKeys.empty () )
	{
		printf ( "Reading WHITE list...  " );
		if ( ! ReadKeyList ( g_sIncludeKeys, g_dIncludeKeys ) )
		{
			printf ( "Failed\n" );
			return false;
		}

		printf ( "%d keys read\n", g_dIncludeKeys.size () );
		return true;
	}

	if ( ! g_sExcludeKeys.empty () )
	{
		printf ( "Reading BLACK list...  " );
		if ( ! ReadKeyList ( g_sExcludeKeys, g_dExcludeKeys ) )
		{
			printf ( "Failed\n" );
			return false;
		}

		printf ( "%d keys read\n", g_dExcludeKeys.size () );
		return true;
	}

	return true;
}

bool PlaceWhiteKeys ()
{
	for ( int i = 0; i < g_dIncludeKeys.size (); ++i )
		g_dKeys.push_back ( g_dIncludeKeys [i] );

	if ( g_dKeys.size () > NUM_KEYS )
	{
		printf ( "ERROR! too many white list keys\n" );
		return false;
	}

	return true;
}

bool CheckKey ( ULARGE_INTEGER uKey )
{
	for ( int i = 0; i < g_dExcludeKeys.size (); ++i )
		if ( g_dExcludeKeys [i].QuadPart == uKey.QuadPart )
			return false;

	for ( int i = 0; i < g_dKeys.size (); ++i )
		if ( g_dKeys [i].QuadPart == uKey.QuadPart )
			return false;

	return true;
}

bool SortKeys ( const ULARGE_INTEGER & uKey1, const ULARGE_INTEGER & uKey2 )
{
	unsigned char * dBits1 = (unsigned char *) &uKey1;
	unsigned char * dBits2 = (unsigned char *) &uKey2;

	static unsigned char dHash1 [reg::HASH_SIZE_BYTES];
	static unsigned char dHash2 [reg::HASH_SIZE_BYTES];

	hash_state md;
	md2_init ( &md );
	md2_process ( &md, dBits1, reg::RAW_KEY_LEN );
	md2_done ( &md, dHash1 );

	md2_init ( &md );
	md2_process ( &md, dBits2, reg::RAW_KEY_LEN );
	md2_done ( &md, dHash2 );

	DWORD uHash1Start = * (DWORD*)dHash1;
	DWORD uHash2Start = * (DWORD*)dHash2;

	return uHash1Start < uHash2Start;
}

int main ( int argc, const char * argv [] )
{
	if ( ! ParseParams ( argc, argv ) )
		return 0;

	if ( ! VerifyParams () )
		return 0;

	printf ( "Generating keys...\n" );

	if ( ! PlaceWhiteKeys () )
		return 0;

	ULARGE_INTEGER uKey;
	DWORD uStartTime = timeGetTime ();

	while ( g_dKeys.size () < NUM_KEYS )
	{
		GenerateRandomKey ( uKey );
		if ( CheckKey ( uKey ) )
			g_dKeys.push_back ( uKey );
	}

	std::sort ( g_dKeys.begin (), g_dKeys.end (), SortKeys );

	printf ( "Dumping keys & hashes... " );
	DumpData ();
	printf ( "Done\n" );

	printf ( "Finished\n");
	printf ( "Keys generated: %d\n", g_dKeys.size () );

	return 0;
}