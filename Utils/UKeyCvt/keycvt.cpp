#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>


const int 	BITS_TO_CUT		= 5;
const char	g_szKeyMap [] = "T2QZ34GF6BRCDEW5H8JKL7MNP9SUVAXY";


char ToLetter ( unsigned char iCode )
{
	// iCode : [0 .. 31]
	return g_szKeyMap [iCode];
}

void FormatKey ( std::string & sKey, int nGroupLetters )
{
	int nInserts = sKey.size () / nGroupLetters - 1;
	for ( int i = 0; i < nInserts; ++i )
		sKey.insert ( ( i + 1 ) * nGroupLetters + i, "-" );
}

int BitsToKey ( const unsigned char * dBits, int nBits, char * dKey )
{
	const unsigned char iMask = ( 1 << BITS_TO_CUT ) - 1;
	int nBitsUsed = 0;
	int iByte = 0;

	int i = 0;
	int nTotalBits = 0;
	while ( nTotalBits < nBits )
	{	
		unsigned char iResult = 0;
		int iOffset = 8 - nBitsUsed;

		if ( iOffset >= BITS_TO_CUT )
		{
			iResult = ( dBits [iByte] >> ( iOffset - BITS_TO_CUT ) ) & iMask;
			nBitsUsed += BITS_TO_CUT;
		}
		else
		{
			nBitsUsed = ( BITS_TO_CUT - iOffset );
			iResult = ( dBits [iByte] << nBitsUsed ) & iMask;
			++iByte;
			iResult |= ( dBits [iByte] >> ( 8 - nBitsUsed ) ) & iMask;
		}

		nTotalBits += BITS_TO_CUT;

		dKey [i++] = ToLetter ( iResult );
	}

	dKey [i] = 0;

	return i;
}

void InvertBits ( unsigned char & uByte )
{
	unsigned uBB = uByte;
	uByte = 0;

	uByte |= ( uBB >> 7 ) & 1;
	uByte |= ( ( uBB >> 6 ) & 1 ) << 1;
	uByte |= ( ( uBB >> 5 ) & 1 ) << 2;
	uByte |= ( ( uBB >> 4 ) & 1 ) << 3;
	uByte |= ( ( uBB >> 3 ) & 1 ) << 4;
	uByte |= ( ( uBB >> 2 ) & 1 ) << 5;
	uByte |= ( ( uBB >> 1 ) & 1 ) << 6;
	uByte |= ( uBB & 1 ) << 7;
}

int main ( int argc, const char * argv [] )
{
	if ( argc != 2 )
	{
		printf ( "Args error\n" );
		return 0;
	}

	FILE * pFile = fopen ( "keys.txt", "rt" );
	FILE * pFileOut = fopen ( "keys_formatted.txt", "wt" );

	if ( ! pFile || ! pFileOut )
	{
		printf ( "File error\n" );
		return 0;
	}

	int iKey = 0;
	ULARGE_INTEGER uKey;
	uKey.QuadPart = 0;

	unsigned char * dKey = (unsigned char *)&uKey;
	char szKey [255];
	std::string sKey;

	int nKeys = 0;

	int nLineKeys = 0;
	while ( ! feof ( pFile ) )
	{
		fscanf ( pFile, "%d: %X %X\n", &iKey, &uKey.HighPart, &uKey.LowPart );

		++nKeys;

		// fucking hack for unusable BitsToKey func
		for ( int i = 0; i < 8; ++i )
			InvertBits ( dKey [i] );

		BitsToKey ( dKey, 60, szKey );
		sKey = szKey;
		FormatKey ( sKey, 4 );
		fprintf ( pFileOut, "%s,", sKey.c_str () );

		if ( ++nLineKeys == 7 )
		{
			nLineKeys = 0;
	//		fprintf ( pFileOut, "\n" );
		}
	}

	fclose ( pFileOut );
	fclose ( pFile );
	
	return 0;
}