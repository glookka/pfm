#ifndef _tga_
#define _tga_

#include "pfm/main.h"

/* orientation */
#define TGA_BOTTOM	0x0
#define TGA_TOP		0x1
#define	TGA_LEFT	0x0
#define	TGA_RIGHT	0x1

/* TGA image header */
struct TGAHeader_t
{
   	unsigned char	id_len;		/* image id length */
	unsigned char	map_t;		/* color map type */
	unsigned char	img_t;		/* image type */
	unsigned short	map_first;	/* index of first map entry */
	unsigned short	map_len;	/* number of entries in color map */
	unsigned char	map_entry;	/* bit-depth of a cmap entry */
	unsigned short	x;			/* x-coordinate */
	unsigned short	y;			/* y-coordinate */
	unsigned short	width;		/* width of image */
	unsigned short	height;		/* height of image */
	unsigned char	depth;		/* pixel-depth of image */
	unsigned char	alpha;      /* alpha bits */
	unsigned char	horz;	    /* horizontal orientation */
	unsigned char	vert;	    /* vertical orientation */
};


struct TGA_t
{
	TGAHeader_t		m_Header;
	unsigned char *	m_pImgData;
};


bool TGAReadImage ( unsigned char * pBuffer, unsigned int uLength, TGA_t & TGA );
void TGAFreeData ( TGA_t & TGA );

#endif