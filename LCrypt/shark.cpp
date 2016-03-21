#include "shark.h"

ddword operator ^ ( ddword x, ddword y )
{
  x.w[0] ^= y.w[0];
  x.w[1] ^= y.w[1];
  return x;
}

byte diffusion::mul(byte a, byte b ) /* multiply two elements of GF(2^m */
{
   if (a && b) return alog[(log[a] + log[b])%255];
   else return 0;
}

diffusion::diffusion()
{
   unsigned i, j = 1;
   for(i = 0; i < 256; i++)
   {
     alog[i] = j;
     j *= 2;
     if (j & 0x100) j ^= ROOT;
   }
   log[0] = 0;
   log[1] = 0; // N.B.
   for(i = 1; i < 255; i++)
     log[alog[i]] = i;

   byte g[9], c[9];
   byte A[8][16];

   /* diffusion box G
    * g(x) = (x - 2)*(x - 4)*...*(x - 2^{8})
    * the rows of G are the coefficients of
    * x^{i} mod g(x), i = 2n - 1, 2n - 2, ..., n
    * iG = inv(G)
    * A = [G,I]
    * Gauss elemination on A to produce [I,iG]
    */
   g[0] = 2;
   g[1] = 1;
   for(i = 2; i < 9; i++) g[i] = 0;
   for(i = 2; i <= 8; i++)
   {
      for(j = 8; j > 0; j--)
      {
         if (g[j]) g[j] = g[j - 1] ^ alog[(i + log[g[j]])%255];
         else g[j] = g[j - 1];
      }
      g[0] = alog[(i + log[g[0]])%255];
   }

   for(j = 0; j < 8; j++) c[j] = 0;
   c[8] = 1;
   for(i = 0; i < 8; i++)
   {
     if (c[8])
       for(byte u = 1; u <= 8; u++)
         c[8 - u] ^= mul(c[8],g[8 - u]);
     c[8] = 0;
     for(j = 0; j < 8; j++)
       G[7 - i][7 - j] = c[j];
     for(j = 8; j > 0; j--)
       c[j] = c[j - 1];
     c[0] = 0;
   }

   for(i = 0; i < 8; i++)
   {
      for(j = 0; j < 8; j++) A[i][j] = G[i][j];
      for(j = 8; j < 16; j++) A[i][j] = 0;
      A[i][i+8] = 1;
   }
   for(i = 0; i < 8; i++)
   {
      byte pivot = A[i][i];
      if (pivot == 0)
      {
         unsigned t = i + 1;
         while (A[t][i] == 0) if (t < 8) t+=1;
         for(j = 0; j < 16; j++)
         {
            byte tmp = A[i][j];
            A[i][j] = A[t][j];
            A[t][j] = tmp;
         }
         pivot = A[i][i];
      }
      for(j = 0; j < 16; j++)
      {
         if (A[i][j])
            A[i][j] = alog[(255 + log[A[i][j]] - log[pivot])%255];
      }
      for(unsigned t = 0; t < 8; t++)
      {
         if (i != t)
         {
            for(j = i+1; j < 16; j++)
               A[t][j] ^= mul(A[i][j],A[t][i]);
            A[t][i] = 0;
         }
      }
   }
   for(i = 0; i < 8; i++)
      for(j = 0; j < 8; j++) iG[i][j] = A[i][j+8];

}

void diffusion::transform(ddword &a)
{
   unsigned i,j;
   ddword k = a;
   for(i = 0; i < 8; i++)
   {
      byte sum = 0;
      for(j = 0; j < 8; j++) sum ^= mul(iG[i][j],k.b[7-j]);
      a.b[7-i] = sum;
   }
}

void coder::do_block()
{
   ulong t, w0, w1=buf.w[1];
   for (unsigned r=0;r<ROUNDS-1;r+=1)
   {
     unsigned ix;
     t = w1 ^ roundkey[r].w[1];
     ix = (unsigned)t % 256; w0  = cbox[3][ix].w[0]; w1  = cbox[3][ix].w[1]; t /= 256;
     ix = (unsigned)t % 256; w0 ^= cbox[2][ix].w[0]; w1 ^= cbox[2][ix].w[1]; t /= 256;
     ix = (unsigned)t % 256; w0 ^= cbox[1][ix].w[0]; w1 ^= cbox[1][ix].w[1]; t /= 256;
     ix = (unsigned)t      ; w0 ^= cbox[0][ix].w[0]; w1 ^= cbox[0][ix].w[1];
     t  = buf.w[0] ^ roundkey[r].w[0];
     ix = (unsigned)t % 256; w0 ^= cbox[7][ix].w[0]; w1 ^= cbox[7][ix].w[1]; t /= 256;
     ix = (unsigned)t % 256; w0 ^= cbox[6][ix].w[0]; w1 ^= cbox[6][ix].w[1]; t /= 256;
     ix = (unsigned)t % 256; w0 ^= cbox[5][ix].w[0]; w1 ^= cbox[5][ix].w[1]; t /= 256;
     ix = (unsigned)t      ; w0 ^= cbox[4][ix].w[0]; w1 ^= cbox[4][ix].w[1];
     buf.w[0] = w0;
   }

   t = w1 ^ roundkey[ROUNDS-1].w[1];
   w1  = sbox[t%256];       t /= 256;
   w1 |= sbox[t%256] <<  8; t /= 256;
   w1 |= sbox[t%256] << 16; t /= 256;
   w1 |= sbox[t    ] << 24;
   buf.w[1] = w1 ^ roundkey[ROUNDS].w[1];

   t = buf.w[0]^ roundkey[ROUNDS-1].w[0];
   w0  = sbox[t%256];       t /= 256;
   w0 |= sbox[t%256] <<  8; t /= 256;
   w0 |= sbox[t%256] << 16; t /= 256;
   w0 |= sbox[t    ] << 24;
   buf.w[0] = w0 ^ roundkey[ROUNDS].w[0];
}

ddword shark::encryption( ddword plain )
{
  enc.buf = plain;
  enc.do_block();
  return enc.buf;
}

ddword shark::decryption( ddword cipher )
{
  dec.buf = cipher;
  dec.do_block();
  return dec.buf;
}

shark::shark( byte * key, unsigned key_length )
{
  diffusion diff;
  {  // initialise boxes
     byte trans[9] = { 0xd6, 0x7b, 0x3d, 0x1f,0x0f, 0x05, 0x03, 0x01, 0xb1 };
     unsigned i, j, k ;
     /* the substitution box based on F^{-1}(x + affine transform of the output */
     enc.sbox[0] = 0;
     enc.sbox[1] = 1;
     for(i = 2; i < 256; i++ ) enc.sbox[i] = diff.alog[255 - diff.log[i]];

     for(i = 0; i < 256; i++)
     {
        byte in = enc.sbox[i];
        enc.sbox[i] = 0;
        for(unsigned t = 0; t < 8; t++)
        {
           byte u = in & trans[t];
           enc.sbox[i] ^= ((1 & (u ^ (u >> 1) ^ (u >> 2) ^ (u >> 3)
                    ^ (u >> 4) ^ (u >> 5) ^ (u >> 6) ^ (u >> 7)))
                     << (7 - t));
        }
        enc.sbox[i] ^= trans[8];
     }

     for(i = 0; i < 256; i++) dec.sbox[enc.sbox[i]] = i;

     for(j = 0; j < 8; j++)
        for(k = 0; k < 256; k++)
           for(i = 0; i < 8; i++)
           {
              enc.cbox[j][k].b[7-i] = diff.mul(enc.sbox[k],diff.G[i][j]);
              dec.cbox[j][k].b[7-i] = diff.mul(dec.sbox[k],diff.iG[i][j]);
           }
  }

  { // initialise roundkeys
    ddword a[ROUNDKEYS];
    unsigned i, j, r;

    for(r = 0; r <= ROUNDS; r++) enc.roundkey[r] = enc.cbox[0][r];
    diff.transform(enc.roundkey[ROUNDS]);

    i = 0;
    for(r = 0; r < ROUNDKEYS; r++)
       for(j = 0; j < 8; j++)
          a[r].b[7-j] = key[(i++)%key_length];

    ddword temp[ROUNDKEYS];
    ddword zero; zero.w[0] = 0; zero.w[1] = 0;
    temp[0] = a[0] ^ encryption( zero );
    for(r = 1; r < ROUNDKEYS; r+=1 )
      temp[r] = a[r] ^ encryption(temp[r-1]);
    diff.transform(temp[ROUNDS]);

    for (r = 0; r < ROUNDKEYS; r+=1 )
    {
      enc.roundkey[r] = temp[r];
      dec.roundkey[ROUNDS-r] = temp[r];
    }
    for(r = 1; r < ROUNDS; r++)
      diff.transform(dec.roundkey[r]);
  }
}