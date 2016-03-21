#include <stdio.h>

void Print ( unsigned short uWord )
{
	static int nPrinted = 0;

	if ( nPrinted == 9 )
	{
		printf ( "\n\t" );
		nPrinted = 0;
	}

	printf ( "0x%04X, ", uWord );
	nPrinted++;
}


int main ( int argc, char ** argv )
{
	if ( argc < 2 )
		return 0;

	const int iSize = 1024*1024;
	unsigned char * pBuffer = new unsigned char [iSize];

	FILE * pFile = fopen ( argv [1], "rb" );
	if ( !pFile )
		return 0;

	int iRead = fread ( pBuffer, 1, iSize, pFile );
	if ( !iRead )
		return 0;

	printf ( "??? RCDATA\nBEGIN\n\t" );

	int nWords = iRead / 2;
	for ( int i = 0; i < nWords; ++i )
	{
		unsigned short uWord = ((unsigned short *)pBuffer)[i];
		Print ( uWord );
	}

	if ( iRead % 2 )
		Print ( pBuffer [iSize - 1] );

	fclose ( pFile );
	delete [] pBuffer;

	printf ( "\nEND\n" );

	return 0;
}