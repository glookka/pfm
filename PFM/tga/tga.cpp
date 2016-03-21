#include "tga.h"

#define ADVBUF(val) pBuffer+=val; uLength-=val;

void TGAFreeData ( TGA_t & TGA )
{
	// 15/16 bit color map entries
	if ( TGA.m_Header.img_t > 8 && TGA.m_Header.img_t < 12 )
	{
		delete [] TGA.m_pImgData;
		TGA.m_pImgData = 0;
	}
}


bool TGAReadHeader ( unsigned char *& pBuffer, unsigned int & uLength, TGA_t & TGA )
{
	const int TGA_HEADER_SIZE = 18;

	if ( uLength < TGA_HEADER_SIZE )
		return 0;

	TGA.m_Header.id_len 	= pBuffer[0];
	TGA.m_Header.map_t 		= pBuffer[1];
	TGA.m_Header.img_t 		= pBuffer[2];
	TGA.m_Header.map_first 	= pBuffer[3] + pBuffer[4] * 256;
	TGA.m_Header.map_len 	= pBuffer[5] + pBuffer[6] * 256;
	TGA.m_Header.map_entry	= pBuffer[7];
	TGA.m_Header.x 			= pBuffer[8] + pBuffer[9] * 256;
	TGA.m_Header.y 			= pBuffer[10] + pBuffer[11] * 256;
	TGA.m_Header.width 		= pBuffer[12] + pBuffer[13] * 256;
	TGA.m_Header.height 	= pBuffer[14] + pBuffer[15] * 256;
	TGA.m_Header.depth 		= pBuffer[16];
	TGA.m_Header.alpha		= pBuffer[17] & 0x0f;
	TGA.m_Header.horz	    = (pBuffer[17] & 0x10) ? TGA_TOP : TGA_BOTTOM;
	TGA.m_Header.vert	    = (pBuffer[17] & 0x20) ? TGA_RIGHT : TGA_LEFT;

	if ( TGA.m_Header.map_t )
		return false;

	if ( TGA.m_Header.depth != 24 && TGA.m_Header.depth != 32 )
		return false;

	ADVBUF ( TGA_HEADER_SIZE );
	return true;
}


inline bool TGAReadRLE ( unsigned char *& pBuffer, unsigned int & uLength, unsigned char * pImageData, TGA_t & TGA )
{
	unsigned char uBytesPP = TGA.m_Header.depth / 8;
	unsigned char * pResPtr = pImageData;
	unsigned char uRepeat, uHead, uDirect;

	unsigned int uDirectSize;
	unsigned char uByte0, uByte1, uByte2, uByte3;

	unsigned int x = 0;

	while ( x < TGA.m_Header.width )
	{
		if ( uLength == 0 )
			return false;

		uHead = *pBuffer;
		ADVBUF ( 1 );

		if ( uHead >= 128 )
		{
			uRepeat = uHead - 127;
			if ( uLength < uBytesPP )
				return false;

			if ( uBytesPP == 4 )
			{
				uByte0 = pBuffer [0];
				uByte1 = pBuffer [1];
				uByte2 = pBuffer [2];
				uByte3 = pBuffer [3];

				for ( unsigned int i = 0; i < uRepeat; i++ )
				{
					pResPtr [0] = uByte0;
					pResPtr [1] = uByte1;
					pResPtr [2] = uByte2;
					pResPtr [3] = uByte2;
					pResPtr += 4;
				}
			}
			else
			{
				uByte0 = pBuffer [0];
				uByte1 = pBuffer [1];
				uByte2 = pBuffer [2];

				for ( unsigned int i = 0; i < uRepeat; i++ )
				{
					pResPtr [0] = uByte0;
					pResPtr [1] = uByte1;
					pResPtr [2] = uByte2;
					pResPtr += 3;
				}
			}

			ADVBUF ( uBytesPP );
			x += uRepeat;
		}
		else
		{
			uDirect = uHead + 1;
			uDirectSize = ((unsigned int )uDirect * (unsigned int )uBytesPP );
			if ( uLength < uDirectSize )
				return false;

			memcpy ( pResPtr, pBuffer, uDirectSize );
			ADVBUF ( uDirectSize );
			pResPtr += uDirectSize;
			x += uDirect;
		}
	}

	return true;
}


inline unsigned int TGAReadScanlines ( unsigned char *& pBuffer, unsigned int & uLength, TGA_t & TGA )
{	
	unsigned int uScanlineSize = TGA.m_Header.width * TGA.m_Header.depth / 8;

	// RLE?
	if ( TGA.m_Header.img_t > 8 && TGA.m_Header.img_t < 12 )
	{
		TGA.m_pImgData = new unsigned char [uScanlineSize*TGA.m_Header.height];

		for ( int i = 0; i < TGA.m_Header.height; ++i )
		{
			unsigned char * pReadAddress = TGA.m_pImgData;
			if ( TGA.m_Header.horz == TGA_BOTTOM )
				pReadAddress += ( TGA.m_Header.height - i - 1 ) * uScanlineSize;
			else
				pReadAddress += i*uScanlineSize;

			if ( !TGAReadRLE ( pBuffer, uLength, pReadAddress, TGA ) ) 
				return false;
		}
	}
	else
		TGA.m_pImgData = pBuffer;

	return true;
}


bool TGAReadImage ( unsigned char * pBuffer, unsigned int uLength, TGA_t & TGA )
{
	if ( !pBuffer || !uLength )
		return false;

	unsigned char * pReadPtr = pBuffer;
	unsigned int uLengthLeft = uLength;
	if ( !TGAReadHeader ( pReadPtr, uLengthLeft, TGA ) )
		return false;

	if ( uLengthLeft < TGA.m_Header.id_len )
		return false;

	pReadPtr += TGA.m_Header.id_len;
	uLengthLeft -= TGA.m_Header.id_len;

	if ( !TGAReadScanlines ( pReadPtr, uLengthLeft, TGA ) )
		return false;

	return true;
}