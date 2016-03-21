#ifndef _shark_
#define _shark_

#define ROUNDS 6
#define ROUNDKEYS ROUNDS+1
#define ROOT 0x1f5

typedef unsigned char byte;
typedef unsigned long ulong;
struct ddword { union { ulong w[2]; byte  b[8]; }; };

struct diffusion
{
  byte log[256], alog[256], G[8][8], iG[8][8];
  byte mul(byte a, byte b ); /* multiply two elements of GF(2^m) */
  void transform(ddword &a);
  diffusion();
};

struct coder
{
  ddword buf;
  ddword roundkey[ROUNDKEYS];
  ddword cbox[8][256];
  byte sbox[256];
  void do_block();
};

struct shark
{
  coder enc,dec;
  shark( byte * key, unsigned key_length );
  ddword encryption( ddword plain );
  ddword decryption( ddword cipher );
};

#endif