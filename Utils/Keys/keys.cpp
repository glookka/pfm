#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <vector>
#include <algorithm>
#include <map>

#include "../../PFM/crypt/tomcrypt.h"


#pragma warning( disable : 4018 4996 )

const int NUM_KEYS = 1024;
const int HASH_SIZE_BYTES = 16;
DWORD g_uStartSeed = 0;					// 0 as start seed
std::string g_sIncludeKeys;				// no include keys
std::string g_sExcludeKeys;				// no exclude keys

bool g_bLineBreaks = false;

std::vector < std::string > g_dKeys;
std::vector < std::string > g_dIncludeKeys;
std::vector < std::string > g_dExcludeKeys;


void GenerateRandomKey ( std::string & sKey )
{
	static const char g_szKeyMap [] = "T2QZ34GF6BRCDEW5H8JKL7MNP9SUVAXY";
	const int KEY_LEN = 14;
	char szKey [KEY_LEN + 1];
	int iKeyMapLen = (int) strlen ( g_szKeyMap );

	for ( int i = 0; i < KEY_LEN; ++i )
		szKey [i] = g_szKeyMap [ char ( float ( rand() ) / float ( RAND_MAX ) * iKeyMapLen )];

	szKey [4] = '-';
	szKey [9] = '-';
	szKey [KEY_LEN] = '\0';

	sKey = szKey;
}


void DumpData ()
{
	FILE * pFile = fopen ( "keys.inc", "wt" );
	FILE * pFileFormatted = fopen ( "keys_formatted.txt", "wt" );

	if ( ! pFile || ! pFileFormatted )
	{
		printf ( "Error writing results\n" );
		return;
	}

	unsigned char dHash [HASH_SIZE_BYTES];

	int nInLine = 0;
	for ( int i = 0; i < g_dKeys.size (); ++i  )
	{
		// raw key
		const std::string & sKey = g_dKeys [i];
		
		bool bWhite = false;
		for ( int j = 0; j < g_dIncludeKeys.size () && !bWhite; ++j )
			if ( sKey == g_dIncludeKeys [j] )
				bWhite = true;

		hash_state md;
		md2_init ( &md );
		md2_process ( &md, (BYTE *)sKey.c_str (), (ULONG)sKey.length () );
		md2_done ( &md, dHash );

		for ( int j = 0; j < HASH_SIZE_BYTES; ++j )
			fprintf ( pFile, "0x%X, ", dHash [j] );			

		if ( ++nInLine == 6 )
		{
			nInLine = 0;
			fprintf ( pFile, "\n" );
		}

		if ( !bWhite )
		{
			if ( g_bLineBreaks )
				fprintf ( pFileFormatted, "%s\n", sKey.c_str () );
			else
				fprintf ( pFileFormatted, "%s,", sKey.c_str () );
		}
	}

	fclose ( pFileFormatted );
	fclose ( pFile );
}



bool ReadKeyList ( const std::string & sFile, std::vector <std::string> & dKeys )
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
		int iLen = (int)strlen ( szKey );
		while ( iLen > 0 && ( szKey [iLen-1] == '\r' || szKey [iLen-1] == '\n' ) )
		{
			szKey [iLen-1] = '\0';
			--iLen;
		}

		if ( iLen > 0 )
			dKeys.push_back ( szKey );
	}
	
	fclose ( pFile );

	return true;
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

	if ( !bHaveSeed )
	{
		// randomize
		LARGE_INTEGER uLarge;
		QueryPerformanceCounter ( &uLarge);
		g_uStartSeed = uLarge.LowPart ^ uLarge.HighPart;
	}

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
		if ( !ReadKeyList ( g_sIncludeKeys, g_dIncludeKeys ) )
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
	if ( g_dKeys.size () > NUM_KEYS )
	{
		printf ( "ERROR! too many  keys in the white list\n" );
		return false;
	}

	for ( int i = 0; i < g_dIncludeKeys.size (); ++i )
		g_dKeys.push_back ( g_dIncludeKeys [i] );

	return true;
}


bool CheckKey ( const std::string & sKey )
{
	for ( int i = 0; i < g_dExcludeKeys.size (); ++i )
		if ( g_dExcludeKeys [i] == sKey )
			return false;

	for ( int i = 0; i < g_dKeys.size (); ++i )
		if ( g_dKeys [i] == sKey )
			return false;

	return true;
}

bool SortKeys ( const std::string & sKey1, std::string & sKey2 )
{
	static unsigned char dHash1 [HASH_SIZE_BYTES];
	static unsigned char dHash2 [HASH_SIZE_BYTES];

	hash_state md;
	md2_init ( &md );
	md2_process ( &md, (BYTE*)sKey1.c_str (), (ULONG)sKey1.size () );
	md2_done ( &md, dHash1 );

	md2_init ( &md );
	md2_process ( &md, (BYTE*)sKey2.c_str (), (ULONG)sKey2.size () );
	md2_done ( &md, dHash2 );

	const int HASH_SIZE_DWORDS = HASH_SIZE_BYTES / sizeof ( DWORD );
	DWORD uTmp1, uTmp2;
	for ( int i = 0; i < HASH_SIZE_DWORDS; ++i )
	{
		uTmp1 = ((DWORD *)dHash1)[i];
		uTmp2 = ((DWORD *)dHash2)[i];

		if ( uTmp1 < uTmp2 )
			return true;

		if ( uTmp1 > uTmp2 )
			return false;
	}

	return false;
}

int main ( int argc, const char * argv [] )
{
	if ( !ParseParams ( argc, argv ) )
		return 0;

	if ( !VerifyParams () )
		return 0;

	printf ( "Generating keys...\n" );

	if ( !PlaceWhiteKeys () )
		return 0;

	std::string sKey;

	while ( g_dKeys.size () < NUM_KEYS )
	{
		GenerateRandomKey ( sKey );
		if ( CheckKey ( sKey ) )
			g_dKeys.push_back ( sKey );
	}

	std::sort ( g_dKeys.begin (), g_dKeys.end (), SortKeys );

	printf ( "Dumping keys & hashes... " );
	DumpData ();
	printf ( "Done\n" );

	printf ( "Finished\n");

	return 0;
}