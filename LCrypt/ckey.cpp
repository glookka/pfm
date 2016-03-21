#include "pch.h"

#include "LCrypt/ckey.h"

namespace reg
{
    const char g_szKeyMap [] = "T2QZ34GF6BRCDEW5H8JKL7MNP9SUVAXY";

	const int BITS_TO_CUT = 5;

	char ToLetter ( unsigned char iCode )
	{
		// iCode : [0 .. 31]
		return g_szKeyMap [iCode];
	}


	unsigned char ToBits ( char iChar )
	{
		int iCharToCheck = toupper ( iChar );
		for ( int i = 0; i < 32; ++i )
			if ( g_szKeyMap [i] == iCharToCheck )
			return i;
				
		return -1;
	}


	int BitsToKey ( const unsigned char * dBits, int nBits, char * dKey )
	{
		// truncate output to 100 bits and encode to 20 chars
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


	int KeyToBits ( const char * dKey, int iLen, unsigned char * dBits )
	{
		// 20 chars -> 100 bits
		const unsigned char iMask = ( 1 << BITS_TO_CUT ) - 1;
		int nBitsUsed = 0;
		int iByte = 0;

		if ( iLen > 0 )
			dBits [0] = 0;

		for ( int i = 0; i < iLen; ++i )
		{	
			unsigned char iResult = ToBits ( dKey [i] );
			int iOffset = 8 - nBitsUsed;
	
			if ( iOffset >= BITS_TO_CUT )
			{
				dBits [iByte] |= iResult << ( iOffset - BITS_TO_CUT );
				nBitsUsed += BITS_TO_CUT;
			}
			else
			{
				nBitsUsed = ( BITS_TO_CUT - iOffset );
				dBits [iByte] |= iResult >> nBitsUsed;
				++iByte;
				dBits [iByte] = ( iResult << ( 8 - nBitsUsed ) ) & 255;
			}
		}

		return iByte + 1;
	}

/*	void FormatKey ( StrAnsi_c & sKey, int nGroupLetters )
	{
		int nInserts = sKey.Length () / nGroupLetters - 1;
		for ( int i = 0; i < nInserts; ++i )
			sKey.Insert ( "-", ( i + 1 ) * nGroupLetters + i );
	}*/

	void UnformatKey ( Str_c & sKey )
	{
		int iPos = 0;
		while ( ( iPos = sKey.Find ( L'-' ) ) != -1 ) 
			sKey.Erase ( iPos, 1 );
	}

  	unsigned int HashToIndex ( unsigned char * dHash, int nBytes )
	{
		int nDWORDs = nBytes / 4;
		DWORD * dDWORDHash = (DWORD *) dHash;
		DWORD uResult = 0;

		for ( int i = 0; i < nDWORDs; ++i )
			uResult ^= dDWORDHash [i];

		return uResult & ( BIT_TABLE_SIZE - 1 );
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
}
