A /* MD5C.C - RSA Data Security, Inc., MD5 message-digest algorithm   */   B /* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All rights reserved.  A License to copy and use this software is granted provided that it @ is identified as the "RSA Data Security, Inc. MD5 Message-DigestB Algorithm" in all material mentioning or referencing this software or this function.   A License is also granted to make and use derivative works provided < that such works are identified as "derived from the RSA Data< Security, Inc. MD5 Message-Digest Algorithm" in all material+ mentioning or referencing the derived work.   B RSA Data Security, Inc. makes no representations concerning either? the merchantability of this software or the suitability of this ; software for any particular purpose. It is provided "as is" 0 without express or implied warranty of any kind.  @ These notices must be retained in any copies of any part of this documentation and/or software.  */    #include "global.h"  #include "md5.h"  & /* Constants for MD5Transform routine.  */          #define S11 7  #define S12 12 #define S13 17 #define S14 22 #define S21 5  #define S22 9  #define S23 14 #define S24 20 #define S31 4  #define S32 11 #define S33 16 #define S34 23 #define S41 6  #define S42 10 #define S43 15 #define S44 21  F static void MD5Transform PROTO_LIST ((UINT4 [4], unsigned char [64])); static void Encode PROTO_LIST -   ((unsigned char *, UINT4 *, unsigned int));  static void Decode PROTO_LIST -   ((UINT4 *, unsigned char *, unsigned int)); E static void MD5_memcpy PROTO_LIST ((POINTER, POINTER, unsigned int)); A static void MD5_memset PROTO_LIST ((POINTER, int, unsigned int));   $ static unsigned char PADDING[64] = {F   0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,F   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,9   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  };  ) /* F, G, H and I are basic MD5 functions.   */ / #define F(x, y, z) (((x) & (y)) | ((~x) & (z))) / #define G(x, y, z) (((x) & (z)) | ((y) & (~z))) $ #define H(x, y, z) ((x) ^ (y) ^ (z))' #define I(x, y, z) ((y) ^ ((x) | (~z)))   % /* ROTATE_LEFT rotates x left n bits.   */ < #define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))  @ /* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.< Rotation is separate from addition to prevent recomputation.  */ $ #define FF(a, b, c, d, x, s, ac) { \0  (a) += F ((b), (c), (d)) + (x) + (UINT4)(ac); \   (a) = ROTATE_LEFT ((a), (s)); \  (a) += (b); \   } $ #define GG(a, b, c, d, x, s, ac) { \0  (a) += G ((b), (c), (d)) + (x) + (UINT4)(ac); \   (a) = ROTATE_LEFT ((a), (s)); \  (a) += (b); \   } $ #define HH(a, b, c, d, x, s, ac) { \0  (a) += H ((b), (c), (d)) + (x) + (UINT4)(ac); \   (a) = ROTATE_LEFT ((a), (s)); \  (a) += (b); \   } $ #define II(a, b, c, d, x, s, ac) { \0  (a) += I ((b), (c), (d)) + (x) + (UINT4)(ac); \   (a) = ROTATE_LEFT ((a), (s)); \  (a) += (b); \   }   F /* MD5 initialization. Begins an MD5 operation, writing a new context.  */  void MD5Init (context)F MD5_CTX *context;                                        /* context */ { ,   context->count[0] = context->count[1] = 0;)   /* Load magic initialization constants.  */!   context->state[0] = 0x67452301; !   context->state[1] = 0xefcdab89; !   context->state[2] = 0x98badcfe; !   context->state[3] = 0x10325476;  }   > /* MD5 block update operation. Continues an MD5 message-digest?   operation, processing another message block, and updating the 
   context.  */ ) void MD5Update (context, input, inputLen) F MD5_CTX *context;                                        /* context */F unsigned char *input;                                /* input block */F unsigned int inputLen;                     /* length of input block */ { !   unsigned int i, index, partLen;   &   /* Compute number of bytes mod 64 */:   index = (unsigned int)((context->count[0] >> 3) & 0x3F);     /* Update number of bits */ 3   if ((context->count[0] += ((UINT4)inputLen << 3))     < ((UINT4)inputLen << 3))   context->count[1]++;/   context->count[1] += ((UINT4)inputLen >> 29);      partLen = 64 - index;   )   /* Transform as many times as possible.  */   if (inputLen >= partLen) {  MD5_memcpy ?    ((POINTER)&context->buffer[index], (POINTER)input, partLen); 0  MD5Transform (context->state, context->buffer);  .  for (i = partLen; i + 63 < inputLen; i += 64),    MD5Transform (context->state, &input[i]);    index = 0;    }    else  i = 0;      /* Buffer remaining input */   MD5_memcpy7  ((POINTER)&context->buffer[index], (POINTER)&input[i],    inputLen-i); }   F /* MD5 finalization. Ends an MD5 message-digest operation, writing the/   the message digest and zeroizing the context.   */  void MD5Final (digest, context) F unsigned char digest[16];                         /* message digest */E MD5_CTX *context;                                       /* context */  {    unsigned char bits[8];   unsigned int index, padLen;      /* Save number of bits */ #   Encode (bits, context->count, 8);      /* Pad out to 56 mod 64. */:   index = (unsigned int)((context->count[0] >> 3) & 0x3f);7   padLen = (index < 56) ? (56 - index) : (120 - index); '   MD5Update (context, PADDING, padLen);   &   /* Append length (before padding) */   MD5Update (context, bits, 8);      /* Store state in digest */ &   Encode (digest, context->state, 16);  #   /* Zeroize sensitive information.  */6   MD5_memset ((POINTER)context, 0, sizeof (*context)); }   = /* MD5 basic transformation. Transforms state based on block.   */ ' static void MD5Transform (state, block)  UINT4 state[4];  unsigned char block[64]; { F   UINT4 a = state[0], b = state[1], c = state[2], d = state[3], x[16];     Decode (x, block, 64);     /* Round 1 */ 2   FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */2   FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */2   FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */2   FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */2   FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */2   FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */2   FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */2   FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */2   FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */3   FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */ 3   FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */ 3   FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */ 3   FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */ 3   FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */ 3   FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */ 3   FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */     /* Round 2 */3   GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */ 3   GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */ 3   GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */ 3   GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */ 3   GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */ 3   GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */ 3   GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */ 3   GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */ 3   GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */ 3   GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */ 3   GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */ 3   GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */ 3   GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */ 3   GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */ 3   GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */ 3   GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */      /* Round 3 */ 3   HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */ 3   HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */ 3   HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */ 3   HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */ 3   HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */ 3   HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */ 3   HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */ 3   HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */ 3   HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */ 3   HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */ 3   HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */ 3   HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */ 3   HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */ 3   HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */ 3   HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */ 3   HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */      /* Round 4 */ 3   II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */ 3   II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */ 3   II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */ 3   II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */ 3   II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */ 3   II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */ 3   II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */ 3   II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */ 3   II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */ 3   II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */ 3   II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */ 3   II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */ 3   II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */ 3   II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */ 3   II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */ 3   II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */      state[0] += a;   state[1] += b;   state[2] += c;   state[3] += d;  #   /* Zeroize sensitive information.    */)   MD5_memset ((POINTER)x, 0, sizeof (x));  }   D /* Encodes input (UINT4) into output (unsigned char). Assumes len is   a multiple of 4.  */ ' static void Encode (output, input, len)  unsigned char *output; UINT4 *input;  unsigned int len;  {    unsigned int i, j;  ,   for (i = 0, j = 0; j < len; i++, j += 4) {.  output[j] = (unsigned char)(input[i] & 0xff);7  output[j+1] = (unsigned char)((input[i] >> 8) & 0xff); 8  output[j+2] = (unsigned char)((input[i] >> 16) & 0xff);8  output[j+3] = (unsigned char)((input[i] >> 24) & 0xff);   }  }   D /* Decodes input (unsigned char) into output (UINT4). Assumes len is   a multiple of 4.  */ ' static void Decode (output, input, len)  UINT4 *output; unsigned char *input;  unsigned int len;  {    unsigned int i, j;  *   for (i = 0, j = 0; j < len; i++, j += 4)=  output[i] = ((UINT4)input[j]) | (((UINT4)input[j+1]) << 8) | =    (((UINT4)input[j+2]) << 16) | (((UINT4)input[j+3]) << 24);  }   = /* Note: Replace "for loop" with standard memcpy if possible.   */   + static void MD5_memcpy (output, input, len)  POINTER output;  POINTER input; unsigned int len;  {    unsigned int i;      for (i = 0; i < len; i++)     output[i] = input[i]; }   = /* Note: Replace "for loop" with standard memset if possible.   */ + static void MD5_memset (output, value, len)  POINTER output; 
 int value; unsigned int len;  {    unsigned int i;      for (i = 0; i < len; i++) #  ((char *)output)[i] = (char)value;  }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               