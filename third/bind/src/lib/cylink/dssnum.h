/*
 * Cylink Corporation � 1998
 * 
 * This software is licensed by Cylink to the Internet Software Consortium to
 * promote implementation of royalty free public key cryptography within IETF
 * standards.  Cylink wishes to expressly thank the contributions of Dr.
 * Martin Hellman, Whitfield Diffie, Ralph Merkle and Stanford University for
 * their contributions to Internet Security.  In accordance with the terms of
 * this license, ISC is authorized to distribute and sublicense this software
 * for the practice of IETF standards.  
 *
 * The software includes BigNum, written by Colin Plumb and licensed by Philip
 * R. Zimmermann for royalty free use and distribution with Cylink's
 * software.  Use of BigNum as a stand alone product or component is
 * specifically prohibited.
 *
 * Disclaimer of All Warranties. THIS SOFTWARE IS BEING PROVIDED "AS IS",
 * WITHOUT ANY EXPRESSED OR IMPLIED WARRANTY OF ANY KIND WHATSOEVER. IN
 * PARTICULAR, WITHOUT LIMITATION ON THE GENERALITY OF THE FOREGOING, CYLINK
 * MAKES NO REPRESENTATION OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
 * PURPOSE.
 *
 * Cylink or its representatives shall not be liable for tort, indirect,
 * special or consequential damages such as loss of profits or loss of
 * goodwill from the use or inability to use the software for any purpose or
 * for any reason whatsoever.
 *
 * EXPORT LAW: Export of the Foundations Suite may be subject to compliance
 * with the rules and regulations promulgated from time to time by the Bureau
 * of Export Administration, United States Department of Commerce, which
 * restrict the export and re-export of certain products and technical data.
 * If the export of the Foundations Suite is controlled under such rules and
 * regulations, then the Foundations Suite shall not be exported or
 * re-exported, directly or indirectly, (a) without all export or re-export
 * licenses and governmental approvals required by any applicable laws, or (b)
 * in violation of any applicable prohibition against the export or re-export
 * of any part of the Foundations Suite. All export licenses for software
 * containing the Foundations Suite are the sole responsibility of the licensee.
 */
 
/**********************************************************************\
*  FILENAME:  DSSNum.h   PRODUCT NAME: CRYPTOGRAPHIC TOOLKIT
*
*  DESCRIPTION:   Standard Cylink DSS numbers (P Q G) support.
*
*  USAGE:       File should be included to get access to
*               Cylink DSS numbers 
*
*
*      Copyright (c) Cylink Corporation 1995. All rights reserved.
*
*  REVISION  HISTORY:
*    10 APR 95  GKL    Initial release 
*
\**********************************************************************/
#ifndef DSSNUMBER_H    /* Prevent multiple inclusions */
#define DSSNUMBER_H

#include "ctk_endian.h"

#define LENGTH_MULT_TO_64

#define F0  0
#define F1  (F0  + 1)
#define F2  (F1  + 2)
#define F3  (F2  + 3)
#define F4  (F3  + 4)
#define F5  (F4  + 5)
#define F6  (F5  + 6)
#define F7  (F6  + 7)
#define F8  (F7  + 8)
#define F9  (F8  + 9)
#define F10 (F9  + 10)
#define F11 (F10 + 11)
#define F12 (F11 + 12)
#define F13 (F12 + 13)
#define F14 (F13 + 14)
#define F15 (F14 + 15)


#ifdef LENGTH_MULT_TO_64
#define LENGTH_STEP 8U
#define LAST_F F8
#endif

#ifdef LENGTH_MULT_TO_32
#define LENGTH_STEP 4U
#define LAST_F (F15 + 16)
#endif


#define DSS_LENS_NUMBER (int) (1 + ( DSS_LENGTH_MAX - DSS_LENGTH_MIN )/LENGTH_STEP)

#define DSS_LAST_INDEX  (DSS_LENGTH_MIN*DSS_LENS_NUMBER + LENGTH_STEP*LAST_F)/sizeof(ord)

#ifndef INITIALIZ_PQG

extern ushort DSS_NUM_INDEX[ DSS_LENS_NUMBER ];
extern ord DSS_Q_NUMBER[SHA_LENGTH/sizeof(ord)];
extern ord DSS_P_NUMBERS[DSS_LAST_INDEX];
extern ord DSS_G_NUMBERS[DSS_LAST_INDEX];

#else
ushort DSS_NUM_INDEX[ DSS_LENS_NUMBER ] =
{
   (DSS_LENGTH_MIN*0  + LENGTH_STEP*0  )/sizeof(ord),
   (DSS_LENGTH_MIN*1  + LENGTH_STEP*F0 )/sizeof(ord),
   (DSS_LENGTH_MIN*2  + LENGTH_STEP*F1 )/sizeof(ord),
   (DSS_LENGTH_MIN*3  + LENGTH_STEP*F2 )/sizeof(ord),
   (DSS_LENGTH_MIN*4  + LENGTH_STEP*F3 )/sizeof(ord),
   (DSS_LENGTH_MIN*5  + LENGTH_STEP*F4 )/sizeof(ord),
   (DSS_LENGTH_MIN*6  + LENGTH_STEP*F5 )/sizeof(ord),
   (DSS_LENGTH_MIN*7  + LENGTH_STEP*F6 )/sizeof(ord),
   (DSS_LENGTH_MIN*8  + LENGTH_STEP*F7 )/sizeof(ord)
#ifdef LENGTH_MULT_TO_32
   ,
   (DSS_LENGTH_MIN*9  + LENGTH_STEP*F8 )/sizeof(ord),
   (DSS_LENGTH_MIN*10 + LENGTH_STEP*F9 )/sizeof(ord),
   (DSS_LENGTH_MIN*11 + LENGTH_STEP*F10)/sizeof(ord),
   (DSS_LENGTH_MIN*12 + LENGTH_STEP*F11)/sizeof(ord),
   (DSS_LENGTH_MIN*13 + LENGTH_STEP*F12)/sizeof(ord),
   (DSS_LENGTH_MIN*14 + LENGTH_STEP*F13)/sizeof(ord),
   (DSS_LENGTH_MIN*15 + LENGTH_STEP*F14)/sizeof(ord),
   (DSS_LENGTH_MIN*16 + LENGTH_STEP*F15)/sizeof(ord)
#endif
};

ord DSS_Q_NUMBER[SHA_LENGTH/sizeof(ord)] =
#ifdef ORD_16
        {
          0x8fb7, 0x81f0, 0x6b9e, 0x122a, 0x4dc4, 0x03ca, 0xc896, 0x8d0e, 0xbe3b, 0xed4b
        };
#else
        {
                         0x81f08fb7L, 0x122a6b9eL, 0x03ca4dc4L, 0x8d0ec896L, 0xed4bbe3bL
        };
#endif


ord DSS_P_NUMBERS[DSS_LAST_INDEX] =
#ifdef ORD_16
        {
     /* dss_p length = 512*/
     0x92b7, 0xbd96, 0x9aab, 0x584c, 0xd617, 0xf2f0, 0xda85, 0xd370,
     0xcc82, 0x273e, 0x6b04, 0xc171, 0x5c32, 0xd3ff, 0x352e, 0xf2f8,
     0x4fc2, 0x20bc, 0xbdec, 0xe96e, 0x3503, 0x4d89, 0xe92d, 0xeb7d,
     0x9c1a, 0x7852, 0xfe2a, 0x0a25, 0x4446, 0xce18, 0x7829, 0x95d8,
#ifdef LENGTH_MULT_TO_32
     /* dss_p length = 544*/
     0x12db, 0x0d31, 0x9950, 0x16c9, 0x3045, 0x0acb, 0x3c46, 0x3c5c,
     0x6c94, 0x2458, 0x0736, 0x13da, 0xa0af, 0x790f, 0xb177, 0xa6ae,
     0xd111, 0x2bda, 0x697c, 0x49a2, 0xe3e5, 0x6f27, 0x02f5, 0x9bc9,
     0xd4da, 0x7d9b, 0x89c6, 0x63bc, 0x0b6d, 0xfe2d, 0xc7a1, 0x435f,
     0x7fad, 0x9e40,
#endif
     /* dss_p length = 576*/
     0x626b, 0x41b5, 0xd218, 0x25bf, 0x1825, 0x42b3, 0x9eb0, 0xc244,
     0x96ce, 0x22ac, 0x3ac6, 0x713f, 0x321d, 0x398c, 0x3022, 0xbc49,
     0xdd03, 0x52d5, 0x29f8, 0x2a57, 0x8fa1, 0x2173, 0xee7d, 0xb90d,
     0xcc64, 0x5fae, 0xaa81, 0xe3d0, 0x648f, 0xfa6e, 0xdc6c, 0x039f,
   0x2cd6, 0xc552, 0xe2dc, 0xebec,
#ifdef LENGTH_MULT_TO_32
     /* dss_p length = 608*/
     0x35f5, 0xb2af, 0x27d0, 0x398b, 0x493d, 0x6f2e, 0xbe27, 0xe5b5,
     0x972c, 0x3470, 0x9638, 0xe90e, 0xf7b7, 0x98ad, 0xd091, 0xb4ca,
     0x3f43, 0x5f58, 0xb6c5, 0xd014, 0x25ee, 0x414d, 0xe2c1, 0x6fd6,
     0xd737, 0x9be6, 0x66ca, 0xe241, 0x1897, 0xf740, 0xe5df, 0xceb0,
     0xdb38, 0xabc8, 0x8af8, 0xc58b, 0xc6a0, 0x9a04,
#endif
     /* dss_p length = 640*/
     0x4095, 0xf6bf, 0x7568, 0x4032, 0x5c0f, 0x64f2, 0x5aae, 0xb099,
     0x346f, 0x0e81, 0x357a, 0x7aeb, 0x0291, 0xfd0a, 0x8d54, 0xce80,
     0x0c05, 0xbea3, 0x889f, 0x8a1b, 0xf1c1, 0x6049, 0x214a, 0x8ec3,
     0xb926, 0xdde1, 0x61b4, 0x384e, 0x404b, 0xb6d7, 0x2e2d, 0x4584,
     0xae6a, 0xcc7a, 0x7bfa, 0x9eb0, 0x3a26, 0x3904, 0x8cac, 0xc036,
#ifdef LENGTH_MULT_TO_32
     /* dss_p length = 672*/
     0x0abb, 0xaea3, 0xc749, 0x757c, 0x3035, 0x29d0, 0x46bd, 0xd56c,
     0xf49d, 0xa355, 0x1297, 0x8dcb, 0x9802, 0xc58c, 0xf1ec, 0x8aac,
     0x55dd, 0xb107, 0x0140, 0x26d1, 0x8a5d, 0x8a90, 0xf33e, 0xde0b,
   0xb844, 0xb429, 0x4db2, 0x9806, 0xf629, 0x4c3b, 0xcd76, 0x2f23,
     0x6c68, 0x7bf4, 0x2a7e, 0xd982, 0xc89f, 0x16a2, 0xfe84, 0x953c,
     0x3e4a, 0x8de8,
#endif
     /* dss_p length = 704*/
     0x0113, 0x1535, 0xca0b, 0x8faf, 0xa327, 0x989c, 0x12c8, 0xe512,
     0x796f, 0x1229, 0x12bf, 0x62c8, 0xa50a, 0x2b99, 0xc93b, 0x450c,
     0x71ad, 0x0826, 0xf4c6, 0x3913, 0x9b6e, 0x96da, 0xa08d, 0xbc5e,
     0xd4d3, 0x7b26, 0xf9fd, 0xdd76, 0x4e82, 0x2f06, 0x1f96, 0xbf47,
     0xcadf, 0x3610, 0x917b, 0x4e94, 0xe2e8, 0x0eaf, 0x1cb9, 0x6b90,
     0xb917, 0x6d6f, 0x92b9, 0xb329,
#ifdef LENGTH_MULT_TO_32
     /* dss_p length = 736*/
     0x6dad, 0xd878, 0xc913, 0xe61e, 0x87d9, 0xe275, 0xad24, 0xf090,
     0xe12b, 0xfdfe, 0x39a3, 0xc9a1, 0x8330, 0x138a, 0x4bed, 0xc319,
     0x8094, 0x88ba, 0x4b80, 0x1325, 0x7852, 0x67e2, 0x715e, 0x7313,
     0x3b4e, 0x7727, 0x9870, 0x5c6e, 0xe0ce, 0xc67a, 0xd057, 0xf3c5,
     0x55ea, 0x98ba, 0x6ea0, 0xaee7, 0x31e8, 0x3cd6, 0x0e28, 0x42d1,
     0x8946, 0xc395, 0x34fc, 0x1b9d, 0x19d1, 0x86f9,
#endif
     /* dss_p length = 768*/
     0xe293, 0xea08, 0xcdd0, 0xf668, 0x8b2a, 0x06db, 0x7c71, 0xadb2,
     0x943d, 0x6721, 0x54f5, 0xbc44, 0xf7ca, 0x3345, 0x1bf6, 0x0b90,
     0xb475, 0xd3c4, 0xbdb6, 0x7caa, 0xa45f, 0xe9b4, 0x6e0e, 0x0c1a,
     0xb5c3, 0x9760, 0x851a, 0x53af, 0x96b7, 0x4979, 0xf162, 0xe951,
     0x6a54, 0xd020, 0x9ded, 0xdecb, 0xfc81, 0x7d74, 0x5e92, 0x2aee,
     0x0946, 0x0eb5, 0x3700, 0x9bce, 0x845a, 0xa7a3, 0x157a, 0x8618,
#ifdef LENGTH_MULT_TO_32
     /* dss_p length = 800*/
     0xb0a1, 0xac27, 0xec4e, 0xf623, 0x55e2, 0x2cb6, 0x288b, 0x4b28,
     0xd74e, 0xbbe6, 0xfdf7, 0xaecd, 0x5758, 0xe251, 0x9074, 0x1aee,
     0xe6ed, 0x4d5e, 0x01d9, 0x529e, 0xf736, 0x0091, 0x0212, 0xc725,
     0x60ce, 0x966f, 0x851d, 0x5c4b, 0x80de, 0x34df, 0x1c88, 0x0636,
     0xbb71, 0x32ce, 0x45cb, 0x86da, 0x4d0e, 0x13e0, 0x7d7d, 0x544b,
     0x1272, 0x747f, 0xd6a7, 0xfa3b, 0xcb86, 0x6b43, 0x66f5, 0xd012,
     0xfe7a, 0xa0cb,
#endif
     /* dss_p length = 832*/
     0xed2d, 0x84dd, 0xf274, 0xdd8f, 0xcd5f, 0x7928, 0xbfc6, 0xa522,
     0xe4e4, 0x50f0, 0xcddf, 0x5842, 0xbb29, 0x7c4d, 0xeef4, 0x6946,
     0x87f6, 0xe486, 0x61b5, 0xc1b6, 0xadef, 0x575a, 0x360f, 0x724f,
     0xcb29, 0x591b, 0x9a4c, 0xf7bc, 0x309e, 0xa348, 0x0ff5, 0x94f3,
     0x932f, 0x9dc6, 0x992c, 0xc6d7, 0x12f6, 0xc2d7, 0x38aa, 0x8c01,
     0x5dca, 0x74f4, 0xfcfd, 0x4cf4, 0x588a, 0x7837, 0x097e, 0xd2e2,
     0x6eac, 0x8869, 0xe92f, 0xec62,
#ifdef LENGTH_MULT_TO_32
     /* dss_p length = 864*/
     0x7c21, 0x931e, 0x553d, 0x00f2, 0xa860, 0x940f, 0x411e, 0x7d20,
     0xb168, 0x52a3, 0x69ee, 0x166e, 0xb9e6, 0x1b23, 0xd5e0, 0xef64,
     0x2850, 0x724c, 0xc1fe, 0xea73, 0xda8c, 0xe342, 0x1d7b, 0xd256,
     0x359b, 0x180f, 0xd7f5, 0xca77, 0x06ac, 0x2162, 0xa977, 0x78bb,
     0x5018, 0x5de2, 0x6061, 0xe217, 0x4d42, 0xfed1, 0x3929, 0xd50f,
     0xc946, 0x2433, 0x15f8, 0x361b, 0xbf51, 0x3ff0, 0x3efa, 0x3157,
     0x7350, 0x53a0, 0xd1a4, 0x261b, 0xde5b, 0x9236,
#endif
     /* dss_p length = 896*/
     0x8e21, 0x0364, 0x86d2, 0xe58c, 0x2274, 0x780a, 0x9ffc, 0x4951,
     0x3f99, 0xbc96, 0x9e60, 0x5a7e, 0x010a, 0x23d4, 0x54d1, 0x48e9,
     0xfd6b, 0x979c, 0x5202, 0x8af3, 0x5d87, 0xfe8f, 0x3e3c, 0x1e0c,
     0xe294, 0xcc52, 0xa8d6, 0x480e, 0xa898, 0xd4ce, 0x5949, 0xd341,
     0xe325, 0xf41b, 0x72d6, 0x6a90, 0x7a8b, 0x6f14, 0x3e2b, 0x6636,
     0x3748, 0x4eea, 0xa590, 0x03e4, 0x0524, 0x9c32, 0xeb53, 0x02af,
     0xca71, 0x6792, 0xd673, 0xedf8, 0x6448, 0x59c3, 0x7319, 0x883f,
#ifdef LENGTH_MULT_TO_32
     /* dss_p length = 928*/
     0xc17d, 0xa4e8, 0xea08, 0x8ff8, 0x2cb0, 0x73f5, 0xd7ba, 0xc027,
     0xb9c8, 0x989d, 0xe5fe, 0xe3a1, 0xf324, 0x39d3, 0xf142, 0x5ba8,
     0xe6cb, 0x3708, 0xa0b7, 0xed9a, 0xb6b3, 0xe314, 0xf80a, 0x6778,
     0x4d27, 0x7107, 0x6ee8, 0xb9c2, 0xc6af, 0xab70, 0x53be, 0xc445,
     0x7926, 0x34bb, 0x5191, 0xe11d, 0x67f1, 0xcc3a, 0x5354, 0xd34d,
     0xa1ac, 0x08ba, 0x32d4, 0x068a, 0x7c3b, 0x415f, 0xbce0, 0xb549,
     0x0ecf, 0x538f, 0xb5d0, 0x22a3, 0x1d47, 0xe837, 0x14c0, 0x69dd,
     0xa02d, 0xce91,
#endif
     /* dss_p length = 960*/
     0x1ff3, 0xf77e, 0xae7a, 0x5a28, 0x43b1, 0xe187, 0x2e04, 0xb3ea,
     0x7394, 0xa3e8, 0x4985, 0xc9bc, 0x7b7d, 0x1e1c, 0xd99c, 0x55f8,
     0x447b, 0x5704, 0xd758, 0xbed1, 0xe698, 0x2a5e, 0x5c19, 0xc206,
     0x6d38, 0x1f88, 0x2ea4, 0x5f15, 0xebfd, 0xd716, 0x8a2a, 0xf960,
     0x83ac, 0xc493, 0xb966, 0x8f13, 0x4778, 0x9682, 0x4712, 0x84cd,
     0xbb4e, 0xe567, 0x644f, 0x0780, 0x133d, 0x0b0a, 0xe42e, 0x06e8,
     0xa4cc, 0x3131, 0x39d9, 0xfaf8, 0x12a3, 0x5550, 0x43b1, 0xb8a9,
     0xb0eb, 0x0fad, 0x986f, 0xd427,
#ifdef LENGTH_MULT_TO_32
     /* dss_p length = 992*/
     0xbcfd, 0x363f, 0x050a, 0x18f0, 0x78d8, 0xcac9, 0x423a, 0xf31e,
     0x198f, 0xd0c3, 0xb319, 0x5d1b, 0x917a, 0xc0b0, 0x8d1b, 0x6b91,
     0xea69, 0xa9d1, 0x02ce, 0x5345, 0x2c80, 0x6992, 0x4dad, 0x7a28,
     0xbb75, 0xd46d, 0x0faa, 0x5612, 0x6878, 0x8a0e, 0x63b4, 0x46ab,
     0xaca0, 0x5381, 0xb90a, 0x3e70, 0x19c4, 0xb639, 0xf778, 0xe751,
     0x5974, 0xb726, 0x589f, 0x8679, 0xeb04, 0xc0b5, 0xdffd, 0x2b32,
     0x4b51, 0xc632, 0xd8c6, 0x9501, 0x4f94, 0x2026, 0x253a, 0x0d27,
     0x07bb, 0x0838, 0x2725, 0xa3eb, 0x3c4e, 0x89b6,
#endif
     /* dss_p length = 1024*/
     0xa141, 0xecf7, 0xc6d6, 0x867d, 0xefa3, 0x1cdd, 0x6c9d, 0x6ca5,
     0x2e2f, 0x68af, 0x90e1, 0x1d3e, 0xd75f, 0x0c2a, 0x844b, 0x36c7,
     0x3420, 0xfd1a, 0xb9ee, 0xf6e5, 0x5fde, 0xc8ed, 0x0c90, 0xd353,
     0x6faa, 0x80a4, 0x5bbd, 0xa59d, 0x9e72, 0x3223, 0x8f59, 0xf33d,
     0x10eb, 0xeccd, 0x184e, 0x3e2a, 0x4c07, 0x564b, 0x4c0a, 0x3263,
     0xc535, 0x8aeb, 0xf982, 0x2be2, 0xe475, 0x9b87, 0xcca0, 0x4d08,
     0x3adf, 0x0484, 0xd528, 0x7acd, 0x304d, 0x55a0, 0x70ae, 0x8298,
     0x6a7a, 0x2298, 0x15c0, 0x7a86, 0x7fc5, 0x654a, 0x14ad, 0xd0aa
        };
#else
        {
/* dss_p length = 512*/
          0xbd9692b7L, 0x584c9aabL, 0xf2f0d617L, 0xd370da85L,
     0x273ecc82L, 0xc1716b04L, 0xd3ff5c32L, 0xf2f8352eL,
     0x20bc4fc2L, 0xe96ebdecL, 0x4d893503L, 0xeb7de92dL,
     0x78529c1aL, 0x0a25fe2aL, 0xce184446L, 0x95d87829L,
#ifdef LENGTH_MULT_TO_32
     /* dss_p length = 544*/
         0x0d3112dbL, 0x16c99950L, 0x0acb3045L, 0x3c5c3c46L,
     0x24586c94L, 0x13da0736L, 0x790fa0afL, 0xa6aeb177L,
     0x2bdad111L, 0x49a2697cL, 0x6f27e3e5L, 0x9bc902f5L,
     0x7d9bd4daL, 0x63bc89c6L, 0xfe2d0b6dL, 0x435fc7a1L,
     0x9e407fadL,
#endif
      /* dss_p length = 576*/
         0x41b5626bL, 0x25bfd218L, 0x42b31825L, 0xc2449eb0L,
     0x22ac96ceL, 0x713f3ac6L, 0x398c321dL, 0xbc493022L,
     0x52d5dd03L, 0x2a5729f8L, 0x21738fa1L, 0xb90dee7dL,
     0x5faecc64L, 0xe3d0aa81L, 0xfa6e648fL, 0x039fdc6cL,
     0xc5522cd6L, 0xebece2dcL,
#ifdef LENGTH_MULT_TO_32
       /* dss_p length = 608*/
         0xb2af35f5L, 0x398b27d0L, 0x6f2e493dL, 0xe5b5be27L,
     0x3470972cL, 0xe90e9638L, 0x98adf7b7L, 0xb4cad091L,
     0x5f583f43L, 0xd014b6c5L, 0x414d25eeL, 0x6fd6e2c1L,
     0x9be6d737L, 0xe24166caL, 0xf7401897L, 0xceb0e5dfL,
     0xabc8db38L, 0xc58b8af8L, 0x9a04c6a0L,
#endif
    /* dss_p length = 640*/
         0xf6bf4095L, 0x40327568L, 0x64f25c0fL, 0xb0995aaeL,
     0x0e81346fL, 0x7aeb357aL, 0xfd0a0291L, 0xce808d54L,
     0xbea30c05L, 0x8a1b889fL, 0x6049f1c1L, 0x8ec3214aL,
     0xdde1b926L, 0x384e61b4L, 0xb6d7404bL, 0x45842e2dL,
     0xcc7aae6aL, 0x9eb07bfaL, 0x39043a26L, 0xc0368cacL,
#ifdef LENGTH_MULT_TO_32
     /* dss_p length = 672*/
         0xaea30abbL, 0x757cc749L, 0x29d03035L, 0xd56c46bdL,
     0xa355f49dL, 0x8dcb1297L, 0xc58c9802L, 0x8aacf1ecL,
     0xb10755ddL, 0x26d10140L, 0x8a908a5dL, 0xde0bf33eL,
     0xb429b844L, 0x98064db2L, 0x4c3bf629L, 0x2f23cd76L,
     0x7bf46c68L, 0xd9822a7eL, 0x16a2c89fL, 0x953cfe84L,
     0x8de83e4aL,
#endif
      /* dss_p length = 704*/
         0x15350113L, 0x8fafca0bL, 0x989ca327L, 0xe51212c8L,
     0x1229796fL, 0x62c812bfL, 0x2b99a50aL, 0x450cc93bL,
     0x082671adL, 0x3913f4c6L, 0x96da9b6eL, 0xbc5ea08dL,
     0x7b26d4d3L, 0xdd76f9fdL, 0x2f064e82L, 0xbf471f96L,
     0x3610cadfL, 0x4e94917bL, 0x0eafe2e8L, 0x6b901cb9L,
     0x6d6fb917L, 0xb32992b9L,
#ifdef LENGTH_MULT_TO_32
       /* dss_p length = 736*/
         0xd8786dadL, 0xe61ec913L, 0xe27587d9L, 0xf090ad24L,
     0xfdfee12bL, 0xc9a139a3L, 0x138a8330L, 0xc3194bedL,
     0x88ba8094L, 0x13254b80L, 0x67e27852L, 0x7313715eL,
     0x77273b4eL, 0x5c6e9870L, 0xc67ae0ceL, 0xf3c5d057L,
     0x98ba55eaL, 0xaee76ea0L, 0x3cd631e8L, 0x42d10e28L,
     0xc3958946L, 0x1b9d34fcL, 0x86f919d1L,
#endif
    /* dss_p length = 768*/
         0xea08e293L, 0xf668cdd0L, 0x06db8b2aL, 0xadb27c71L,
     0x6721943dL, 0xbc4454f5L, 0x3345f7caL, 0x0b901bf6L,
     0xd3c4b475L, 0x7caabdb6L, 0xe9b4a45fL, 0x0c1a6e0eL,
     0x9760b5c3L, 0x53af851aL, 0x497996b7L, 0xe951f162L,
     0xd0206a54L, 0xdecb9dedL, 0x7d74fc81L, 0x2aee5e92L,
     0x0eb50946L, 0x9bce3700L, 0xa7a3845aL, 0x8618157aL,
#ifdef LENGTH_MULT_TO_32
     /* dss_p length = 800*/
         0xac27b0a1L, 0xf623ec4eL, 0x2cb655e2L, 0x4b28288bL,
     0xbbe6d74eL, 0xaecdfdf7L, 0xe2515758L, 0x1aee9074L,
     0x4d5ee6edL, 0x529e01d9L, 0x0091f736L, 0xc7250212L,
     0x966f60ceL, 0x5c4b851dL, 0x34df80deL, 0x06361c88L,
     0x32cebb71L, 0x86da45cbL, 0x13e04d0eL, 0x544b7d7dL,
     0x747f1272L, 0xfa3bd6a7L, 0x6b43cb86L, 0xd01266f5L,
     0xa0cbfe7aL,
#endif
      /* dss_p length = 832*/
         0x84dded2dL, 0xdd8ff274L, 0x7928cd5fL, 0xa522bfc6L,
     0x50f0e4e4L, 0x5842cddfL, 0x7c4dbb29L, 0x6946eef4L,
     0xe48687f6L, 0xc1b661b5L, 0x575aadefL, 0x724f360fL,
     0x591bcb29L, 0xf7bc9a4cL, 0xa348309eL, 0x94f30ff5L,
     0x9dc6932fL, 0xc6d7992cL, 0xc2d712f6L, 0x8c0138aaL,
     0x74f45dcaL, 0x4cf4fcfdL, 0x7837588aL, 0xd2e2097eL,
     0x88696eacL, 0xec62e92fL,
#ifdef LENGTH_MULT_TO_32
     /* dss_p length = 864*/
     0x931e7c21L, 0x00f2553dL, 0x940fa860L, 0x7d20411eL,
     0x52a3b168L, 0x166e69eeL, 0x1b23b9e6L, 0xef64d5e0L,
     0x724c2850L, 0xea73c1feL, 0xe342da8cL, 0xd2561d7bL,
     0x180f359bL, 0xca77d7f5L, 0x216206acL, 0x78bba977L,
     0x5de25018L, 0xe2176061L, 0xfed14d42L, 0xd50f3929L,
     0x2433c946L, 0x361b15f8L, 0x3ff0bf51L, 0x31573efaL,
     0x53a07350L, 0x261bd1a4L, 0x9236de5bL,
#endif
     /* dss_p length = 896*/
     0x03648e21L, 0xe58c86d2L, 0x780a2274L, 0x49519ffcL,
     0xbc963f99L, 0x5a7e9e60L, 0x23d4010aL, 0x48e954d1L,
     0x979cfd6bL, 0x8af35202L, 0xfe8f5d87L, 0x1e0c3e3cL,
     0xcc52e294L, 0x480ea8d6L, 0xd4cea898L, 0xd3415949L,
     0xf41be325L, 0x6a9072d6L, 0x6f147a8bL, 0x66363e2bL,
     0x4eea3748L, 0x03e4a590L, 0x9c320524L, 0x02afeb53L,
     0x6792ca71L, 0xedf8d673L, 0x59c36448L, 0x883f7319L,
#ifdef LENGTH_MULT_TO_32
     /* dss_p length = 928*/
     0xa4e8c17dL, 0x8ff8ea08L, 0x73f52cb0L, 0xc027d7baL,
     0x989db9c8L, 0xe3a1e5feL, 0x39d3f324L, 0x5ba8f142L,
     0x3708e6cbL, 0xed9aa0b7L, 0xe314b6b3L, 0x6778f80aL,
     0x71074d27L, 0xb9c26ee8L, 0xab70c6afL, 0xc44553beL,
     0x34bb7926L, 0xe11d5191L, 0xcc3a67f1L, 0xd34d5354L,
     0x08baa1acL, 0x068a32d4L, 0x415f7c3bL, 0xb549bce0L,
     0x538f0ecfL, 0x22a3b5d0L, 0xe8371d47L, 0x69dd14c0L,
     0xce91a02dL,
#endif
     /* dss_p length = 960*/
     0xf77e1ff3L, 0x5a28ae7aL, 0xe18743b1L, 0xb3ea2e04L,
     0xa3e87394L, 0xc9bc4985L, 0x1e1c7b7dL, 0x55f8d99cL,
     0x5704447bL, 0xbed1d758L, 0x2a5ee698L, 0xc2065c19L,
     0x1f886d38L, 0x5f152ea4L, 0xd716ebfdL, 0xf9608a2aL,
     0xc49383acL, 0x8f13b966L, 0x96824778L, 0x84cd4712L,
     0xe567bb4eL, 0x0780644fL, 0x0b0a133dL, 0x06e8e42eL,
     0x3131a4ccL, 0xfaf839d9L, 0x555012a3L, 0xb8a943b1L,
     0x0fadb0ebL, 0xd427986fL,
#ifdef LENGTH_MULT_TO_32
     /* dss_p length = 992*/
     0x363fbcfdL, 0x18f0050aL, 0xcac978d8L, 0xf31e423aL,
     0xd0c3198fL, 0x5d1bb319L, 0xc0b0917aL, 0x6b918d1bL,
     0xa9d1ea69L, 0x534502ceL, 0x69922c80L, 0x7a284dadL,
     0xd46dbb75L, 0x56120faaL, 0x8a0e6878L, 0x46ab63b4L,
     0x5381aca0L, 0x3e70b90aL, 0xb63919c4L, 0xe751f778L,
     0xb7265974L, 0x8679589fL, 0xc0b5eb04L, 0x2b32dffdL,
     0xc6324b51L, 0x9501d8c6L, 0x20264f94L, 0x0d27253aL,
     0x083807bbL, 0xa3eb2725L, 0x89b63c4eL,
#endif
     /* dss_p length = 1024*/
     0xecf7a141L, 0x867dc6d6L, 0x1cddefa3L, 0x6ca56c9dL,
     0x68af2e2fL, 0x1d3e90e1L, 0x0c2ad75fL, 0x36c7844bL,
     0xfd1a3420L, 0xf6e5b9eeL, 0xc8ed5fdeL, 0xd3530c90L,
     0x80a46faaL, 0xa59d5bbdL, 0x32239e72L, 0xf33d8f59L,
     0xeccd10ebL, 0x3e2a184eL, 0x564b4c07L, 0x32634c0aL,
     0x8aebc535L, 0x2be2f982L, 0x9b87e475L, 0x4d08cca0L,
     0x04843adfL, 0x7acdd528L, 0x55a0304dL, 0x829870aeL,
       0x22986a7aL, 0x7a8615c0L, 0x654a7fc5L, 0xd0aa14adL
              };
#endif

ord DSS_G_NUMBERS[DSS_LAST_INDEX] =
#ifdef ORD_16
                {
      /* dss_g length = 512*/
          0x8b1a, 0xedfe, 0xef16, 0xdb26, 0xeae0, 0x1f1d, 0xaf3a, 0x3e30,
         0x2bd6, 0x25ec, 0xa451, 0x6255, 0xbc75, 0x499c, 0x4071, 0x5da2,
         0x1149, 0xc7fc, 0x1402, 0x9b69, 0xc168, 0xb0ea, 0xaf92, 0x6a33,
         0xf45f, 0xd93a, 0x75a6, 0x263c, 0xb820, 0x7eb1, 0x5f5b, 0x4bd7,
#ifdef LENGTH_MULT_TO_32
         /* dss_g length = 544*/
         0x2772, 0xe0e6, 0xce97, 0x605b, 0x4aa8, 0x2586, 0x095a, 0xb83f,
         0x8b01, 0xfe53, 0x9250, 0x74ef, 0x74d4, 0xd9fa, 0xb78b, 0xa714,
         0x106f, 0x03a6, 0xb406, 0xc549, 0xc44d, 0xd559, 0x8100, 0xfef4,
         0x34a4, 0x1f4c, 0x3c6b, 0x4ae0, 0xe770, 0x158b, 0x3f8d, 0xf73d,
         0x0cc5, 0x61b7,
#endif
     /* dss_g length = 576*/
     0x3dde, 0x64fd, 0x2b69, 0x03c3, 0xad1d, 0x1751, 0x11dc, 0xfe67,
     0x6379, 0x76de, 0xe333, 0x3b8f, 0x242f, 0x27d8, 0x5f33, 0x3597,
     0xb98c, 0x11dc, 0x718c, 0xe3b5, 0xa3d4, 0x58f2, 0x71fa, 0x2675,
     0x49f9, 0xf2c7, 0x510e, 0xa4e1, 0xeca4, 0x7c64, 0x243b, 0x78fc,
     0x2ce2, 0x7017, 0x0095, 0x23b6,
#ifdef LENGTH_MULT_TO_32
     /* dss_g length = 608*/
     0xdad8, 0x1e27, 0x41f5, 0xd536, 0x528f, 0x7047, 0x028a, 0x56f5,
     0xe55f, 0xe20d, 0x612c, 0x520f, 0xebfd, 0x8c86, 0x0924, 0x562a,
     0x2185, 0xe5ac, 0xa113, 0x4b8c, 0x87da, 0xfa4c, 0x8788, 0x9d8d,
     0x41c0, 0x9d25, 0x9c77, 0xff33, 0x6861, 0xd10a, 0xc734, 0x8454,
     0xf803, 0x55ce, 0xfeac, 0x6580, 0x6cd2, 0x482b,
#endif
     /* dss_g length = 640*/
         0x4ff7, 0x2829, 0x8ab0, 0xa0bd, 0x7b1c, 0xf544, 0xe633, 0x6e7b,
     0x9824, 0x494a, 0xfb7f, 0xc8ad, 0x45b6, 0x956e, 0x0574, 0x5b0d,
     0xd40c, 0xf807, 0x7372, 0x56fd, 0xdd12, 0x8960, 0x255c, 0x019e,
     0xfc39, 0x06b3, 0x9f8e, 0x4cd9, 0xe714, 0x77f6, 0x76f7, 0xb573,
     0x010e, 0x9b52, 0x04d2, 0xe269, 0xd4bb, 0xbdec, 0x089d, 0x7f88,
#ifdef LENGTH_MULT_TO_32
     /* dss_g length = 672*/
     0x3d36, 0xb22d, 0x2144, 0xfda0, 0x1d4a, 0xc144, 0xef8f, 0x70e8,
     0xfd0f, 0xcbf9, 0x7433, 0xbf29, 0x1657, 0x757b, 0x011d, 0x5c7e,
     0xa0d3, 0xec5a, 0xd45d, 0xacd6, 0x136a, 0x9454, 0x61fd, 0xc3f6,
     0x3758, 0x5c89, 0xe4df, 0xd3c9, 0x6f99, 0xa113, 0x0992, 0x16b2,
     0xf92f, 0xfb67, 0x3f34, 0x57e6, 0xb224, 0xdfd1, 0x43c4, 0x639a,
     0xccf5, 0x86c4,
#endif
     /* dss_g length = 704*/
     0x1621, 0x8313, 0x216e, 0xcb3b, 0xde00, 0xa11b, 0xf27f, 0xd5d4,
     0x6d2e, 0xc870, 0x1c47, 0x2e21, 0x780b, 0x1db6, 0x8adf, 0xe5c6,
     0x837d, 0x7dba, 0x9c8c, 0x28b5, 0xd309, 0xf3fa, 0x6c65, 0xe37f,
     0x6a1e, 0x2601, 0xbb92, 0x56aa, 0x9c0e, 0x9db1, 0xb782, 0x642b,
     0x6cc9, 0xb9fb, 0x6a3c, 0x97b9, 0xf1a8, 0xd8a2, 0xfe6b, 0xcb93,
       0x59ee, 0x32a4, 0x0aa1, 0x58ad,
#ifdef LENGTH_MULT_TO_32
     /* dss_g length = 736*/
     0x648c, 0xa15c, 0x27b2, 0x8137, 0xefb3, 0x0e81, 0x258e, 0xabe0,
     0x9f2f, 0x6c67, 0xbed6, 0xd201, 0x3647, 0xbe9a, 0x0091, 0xba9a,
     0x4df1, 0xdc8c, 0x5b15, 0x5a37, 0xb5b1, 0x50f8, 0xdfe6, 0x081b,
     0x48d7, 0xd40b, 0x7c51, 0x0417, 0x97c7, 0x2565, 0xf960, 0x89b2,
     0xa1f0, 0x7aac, 0xed6f, 0x20fe, 0x1d98, 0x0eee, 0x48b8, 0xb062,
     0x70e7, 0xa3f3, 0xbe3f, 0x9183, 0x76ae, 0x6cbc,
#endif
     /* dss_g length = 768*/
     0x6216, 0x4b90, 0xc254, 0x7ab6, 0x7a04, 0xf90f, 0x42dd, 0x58c7,
     0xd015, 0x904d, 0xfbf7, 0x624a, 0x5010, 0x627f, 0x696c, 0x1a32,
     0xe0bc, 0xcdfd, 0xe32f, 0xb081, 0x1377, 0x1913, 0x5f96, 0x86c6,
     0x864a, 0x8429, 0x4bb9, 0xd0c6, 0x3361, 0xbc7d, 0xbd8d, 0xa3b2,
     0x47d5, 0x5086, 0xed0b, 0x3bb6, 0xdba6, 0x6f2c, 0x707a, 0x434b,
     0xd4cc, 0x7b10, 0x8ef0, 0x3466, 0x4737, 0x8f27, 0x3399, 0x3716,
#ifdef LENGTH_MULT_TO_32
     /* dss_g length = 800*/
     0x99a7, 0xa90c, 0xdf8e, 0xba50, 0x83b5, 0x7ea0, 0x1d8a, 0xe5bb,
   0x34c9, 0x8c4d, 0xb151, 0x3aba, 0xee2f, 0x76c8, 0xeebf, 0x3db9,
     0xc1e0, 0x2b9a, 0x774f, 0xb476, 0x9b93, 0x53b5, 0xc008, 0xed2c,
     0x0ad4, 0x8af4, 0xc0e6, 0x0e98, 0x2d7b, 0xdb37, 0x7b59, 0x8a31,
     0x0667, 0x1225, 0xb882, 0x0355, 0x58ba, 0xf079, 0x80d7, 0x8033,
     0x54bc, 0xf9cd, 0x461e, 0xc70a, 0x9170, 0x1dba, 0xc447, 0xd3e5,
     0xaf18, 0x04c4,
#endif
     /* dss_g length = 832*/
     0xd1b5, 0x3d20, 0xfbdb, 0xa0b6, 0x0505, 0x4e88, 0xa781, 0x7c65,
     0xd381, 0x5b6e, 0xfd1e, 0xbb71, 0xe085, 0x855d, 0x6d0b, 0x650a,
     0xa248, 0x82d0, 0xd4dd, 0x7ea0, 0x16d1, 0x6937, 0x2cc2, 0x2dec,
     0x5e07, 0x97b4, 0x0c5a, 0xcf21, 0x0299, 0x9b96, 0x4acf, 0xc732,
     0xfbcf, 0xeefb, 0x0032, 0x40bb, 0xc86e, 0xeacb, 0xae2b, 0x8adb,
     0x39aa, 0xbb47, 0xaf3a, 0xfd36, 0xf859, 0x97fc, 0x5535, 0x0d3d,
     0x627a, 0x3f62, 0x1f90, 0x5490,
#ifdef LENGTH_MULT_TO_32
     /* dss_g length = 864*/
     0xbf83, 0x8b52, 0xc2b1, 0x0808, 0x4a3e, 0x6ccb, 0x8aa8, 0xcfdd,
     0xb046, 0x4948, 0xcdf4, 0x9881, 0x13d8, 0x85b0, 0xa22a, 0x3786,
     0xe9db, 0xcb8a, 0x89ad, 0x5e27, 0xd925, 0x1fcb, 0x3855, 0x4afd,
       0x7f67, 0x83be, 0xb092, 0xe061, 0x6703, 0xf21f, 0x403e, 0x4c6a,
     0xa8bd, 0x536a, 0x89b6, 0xb25e, 0xe165, 0xd259, 0xb765, 0xd7f3,
     0xc474, 0xd8bc, 0x617d, 0x1a0a, 0xa054, 0x8c28, 0x9fb0, 0x9595,
     0x6f2d, 0x6254, 0xe1c2, 0xb450, 0xef81, 0x277f,
#endif
     /* dss_g length = 896*/
     0x7490, 0x2aea, 0xd005, 0xa3cc, 0x9211, 0x235a, 0x7b6d, 0xacec,
     0xfca5, 0x5407, 0x8515, 0x5bc1, 0x8b2a, 0x9388, 0x8ff9, 0xed56,
     0xf4e9, 0xf31e, 0x7e05, 0x172a, 0x6e2a, 0x8165, 0x7a24, 0x975b,
     0x1f6e, 0xd4d1, 0x8232, 0xd6ff, 0x2363, 0x4072, 0xa1f1, 0xd18b,
     0xe574, 0xb032, 0xc330, 0x81b6, 0x6033, 0x07d5, 0x5107, 0xb7cc,
     0x2c10, 0xbd8e, 0xc6a3, 0xe731, 0xfcd6, 0xe567, 0x3440, 0xbcde,
     0x1976, 0xdbb3, 0xd15a, 0x4e39, 0xc282, 0x4b0b, 0x82b3, 0x286b,
#ifdef LENGTH_MULT_TO_32
     /* dss_g length = 928*/
     0x9f66, 0xd0e5, 0xd7d2, 0xe875, 0x0707, 0x2dca, 0xa5a2, 0x266d,
     0x80c2, 0x6d20, 0x69b3, 0x759c, 0x497f, 0x74b4, 0x0f56, 0x4b63,
     0xa294, 0x4995, 0x3eda, 0xf5c4, 0x7951, 0x3a3d, 0x22e9, 0x617f,
     0x1906, 0xd45d, 0x047f, 0x2726, 0x27be, 0x660e, 0xc1d3, 0xa588,
     0xceb2, 0xe300, 0xca06, 0x8620, 0x1110, 0x76ae, 0xf7b8, 0xe368,
       0x89a7, 0x2ded, 0x2756, 0x3ed6, 0x5449, 0x4bd8, 0xb73d, 0xb406,
     0x25ec, 0x32f7, 0x989c, 0x6623, 0x6b1f, 0xf7f0, 0x4807, 0x0954,
     0x51e6, 0x97d0,
#endif
     /* dss_g length = 960*/
     0x4118, 0xe87c, 0x809a, 0x1287, 0x2a0f, 0x51d6, 0x7fc8, 0xdad8,
     0x8dc6, 0xddce, 0xd27c, 0x898a, 0x96a9, 0xaee1, 0xe44c, 0xd322,
     0x9d58, 0x3a34, 0xcebd, 0x322a, 0x5b48, 0x9c21, 0x6d04, 0x2d77,
     0x41f1, 0x5123, 0xaa5f, 0x2192, 0x8175, 0x5cac, 0xf547, 0xdc08,
     0xb0d9, 0x4f11, 0x85c9, 0xb5bf, 0x7147, 0x9fbf, 0x3c20, 0x7f7a,
     0xdd8a, 0xa163, 0x0ae4, 0xfcb2, 0x8251, 0x8162, 0xb96e, 0x84f9,
     0xd652, 0x94ba, 0xbb90, 0x9559, 0xac51, 0x6418, 0xdb54, 0x4739,
     0x3ec0, 0x9b7c, 0xae96, 0x3e14,
#ifdef LENGTH_MULT_TO_32
     /* dss_g length = 992*/
     0x4eff, 0x399a, 0x5da9, 0x1e5a, 0x9040, 0x4bad, 0xc85d, 0x5b9a,
     0x825b, 0x5464, 0x7538, 0xe2ea, 0xb957, 0x5def, 0xbfa6, 0x4916,
     0x89be, 0x93fe, 0xe5da, 0x919f, 0x791c, 0xe7a8, 0xef86, 0xc186,
     0xbf63, 0xa881, 0x1e38, 0x9abe, 0x8b58, 0x70b4, 0x9dfd, 0x0719,
     0x85e5, 0xc60a, 0xab36, 0xe07d, 0x0c49, 0xd6fe, 0xfdb4, 0x6619,
       0xe55d, 0x997e, 0x5f38, 0x824f, 0x47ff, 0xf800, 0xb137, 0x839d,
     0x2dae, 0xe0db, 0x2a74, 0x2f58, 0x6fb3, 0x1bab, 0x62ed, 0x0e02,
     0xbef2, 0xd7f1, 0xe566, 0xa9ae, 0xbc35, 0x6357,
#endif
     /* dss_g length = 1024*/
     0x914d, 0x76b2, 0xfee4, 0x9cf7, 0xa136, 0x07a1, 0x3808, 0x1857,
     0x8f96, 0x870c, 0x1f3c, 0x59f2, 0x6f01, 0x4d4d, 0x580e, 0xafc6,
     0x99af, 0xff1b, 0xc019, 0x6c1f, 0xe449, 0x1698, 0x4787, 0x0aa2,
     0x6e68, 0x768d, 0x5ff0, 0x27dc, 0xca9e, 0x630f, 0x01ae, 0x9981,
     0x688d, 0xf278, 0x7f3d, 0x17ad, 0x48aa, 0x65d6, 0x181f, 0x1802,
     0x647c, 0x9ef5, 0x7081, 0x5843, 0x1f0d, 0x9364, 0xebe9, 0x6330,
     0x033a, 0x5d2a, 0xb68a, 0x5639, 0xfa7c, 0xd77e, 0x4bdc, 0x4a62,
     0x5c6d, 0xfed5, 0xa8aa, 0x5eff, 0x1138, 0xae4a, 0xe993, 0x3a73
        };
#else
        {
     /* dss_g length = 512*/
     0xedfe8b1aL, 0xdb26ef16L, 0x1f1deae0L, 0x3e30af3aL,
   0x25ec2bd6L, 0x6255a451L, 0x499cbc75L, 0x5da24071L,
     0xc7fc1149L, 0x9b691402L, 0xb0eac168L, 0x6a33af92L,
     0xd93af45fL, 0x263c75a6L, 0x7eb1b820L, 0x4bd75f5bL,
#ifdef LENGTH_MULT_TO_32
     /* dss_g length = 544*/
     0xe0e62772L, 0x605bce97L, 0x25864aa8L, 0xb83f095aL,
     0xfe538b01L, 0x74ef9250L, 0xd9fa74d4L, 0xa714b78bL,
     0x03a6106fL, 0xc549b406L, 0xd559c44dL, 0xfef48100L,
     0x1f4c34a4L, 0x4ae03c6bL, 0x158be770L, 0xf73d3f8dL,
     0x61b70cc5L,
#endif
     /* dss_g length = 576*/
     0x64fd3ddeL, 0x03c32b69L, 0x1751ad1dL, 0xfe6711dcL,
     0x76de6379L, 0x3b8fe333L, 0x27d8242fL, 0x35975f33L,
     0x11dcb98cL, 0xe3b5718cL, 0x58f2a3d4L, 0x267571faL,
     0xf2c749f9L, 0xa4e1510eL, 0x7c64eca4L, 0x78fc243bL,
     0x70172ce2L, 0x23b60095L,
#ifdef LENGTH_MULT_TO_32
     /* dss_g length = 608*/
     0x1e27dad8L, 0xd53641f5L, 0x7047528fL, 0x56f5028aL,
     0xe20de55fL, 0x520f612cL, 0x8c86ebfdL, 0x562a0924L,
     0xe5ac2185L, 0x4b8ca113L, 0xfa4c87daL, 0x9d8d8788L,
     0x9d2541c0L, 0xff339c77L, 0xd10a6861L, 0x8454c734L,
     0x55cef803L, 0x6580feacL, 0x482b6cd2L,
#endif
     /* dss_g length = 640*/
     0x28294ff7L, 0xa0bd8ab0L, 0xf5447b1cL, 0x6e7be633L,
     0x494a9824L, 0xc8adfb7fL, 0x956e45b6L, 0x5b0d0574L,
     0xf807d40cL, 0x56fd7372L, 0x8960dd12L, 0x019e255cL,
     0x06b3fc39L, 0x4cd99f8eL, 0x77f6e714L, 0xb57376f7L,
     0x9b52010eL, 0xe26904d2L, 0xbdecd4bbL, 0x7f88089dL,
#ifdef LENGTH_MULT_TO_32
     /* dss_g length = 672*/
     0xb22d3d36L, 0xfda02144L, 0xc1441d4aL, 0x70e8ef8fL,
     0xcbf9fd0fL, 0xbf297433L, 0x757b1657L, 0x5c7e011dL,
     0xec5aa0d3L, 0xacd6d45dL, 0x9454136aL, 0xc3f661fdL,
     0x5c893758L, 0xd3c9e4dfL, 0xa1136f99L, 0x16b20992L,
     0xfb67f92fL, 0x57e63f34L, 0xdfd1b224L, 0x639a43c4L,
     0x86c4ccf5L,
#endif
     /* dss_g length = 704*/
     0x83131621L, 0xcb3b216eL, 0xa11bde00L, 0xd5d4f27fL,
     0xc8706d2eL, 0x2e211c47L, 0x1db6780bL, 0xe5c68adfL,
     0x7dba837dL, 0x28b59c8cL, 0xf3fad309L, 0xe37f6c65L,
     0x26016a1eL, 0x56aabb92L, 0x9db19c0eL, 0x642bb782L,
     0xb9fb6cc9L, 0x97b96a3cL, 0xd8a2f1a8L, 0xcb93fe6bL,
     0x32a459eeL, 0x58ad0aa1L,
#ifdef LENGTH_MULT_TO_32
     /* dss_g length = 736*/
     0xa15c648cL, 0x813727b2L, 0x0e81efb3L, 0xabe0258eL,
     0x6c679f2fL, 0xd201bed6L, 0xbe9a3647L, 0xba9a0091L,
     0xdc8c4df1L, 0x5a375b15L, 0x50f8b5b1L, 0x081bdfe6L,
     0xd40b48d7L, 0x04177c51L, 0x256597c7L, 0x89b2f960L,
     0x7aaca1f0L, 0x20feed6fL, 0x0eee1d98L, 0xb06248b8L,
     0xa3f370e7L, 0x9183be3fL, 0x6cbc76aeL,
#endif
     /* dss_g length = 768*/
     0x4b906216L, 0x7ab6c254L, 0xf90f7a04L, 0x58c742ddL,
     0x904dd015L, 0x624afbf7L, 0x627f5010L, 0x1a32696cL,
     0xcdfde0bcL, 0xb081e32fL, 0x19131377L, 0x86c65f96L,
     0x8429864aL, 0xd0c64bb9L, 0xbc7d3361L, 0xa3b2bd8dL,
     0x508647d5L, 0x3bb6ed0bL, 0x6f2cdba6L, 0x434b707aL,
     0x7b10d4ccL, 0x34668ef0L, 0x8f274737L, 0x37163399L,
#ifdef LENGTH_MULT_TO_32
     /* dss_g length = 800*/
     0xa90c99a7L, 0xba50df8eL, 0x7ea083b5L, 0xe5bb1d8aL,
     0x8c4d34c9L, 0x3abab151L, 0x76c8ee2fL, 0x3db9eebfL,
     0x2b9ac1e0L, 0xb476774fL, 0x53b59b93L, 0xed2cc008L,
     0x8af40ad4L, 0x0e98c0e6L, 0xdb372d7bL, 0x8a317b59L,
     0x12250667L, 0x0355b882L, 0xf07958baL, 0x803380d7L,
     0xf9cd54bcL, 0xc70a461eL, 0x1dba9170L, 0xd3e5c447L,
     0x04c4af18L,
#endif
     /* dss_g length = 832*/
     0x3d20d1b5L, 0xa0b6fbdbL, 0x4e880505L, 0x7c65a781L,
     0x5b6ed381L, 0xbb71fd1eL, 0x855de085L, 0x650a6d0bL,
     0x82d0a248L, 0x7ea0d4ddL, 0x693716d1L, 0x2dec2cc2L,
          0x97b45e07L, 0xcf210c5aL, 0x9b960299L, 0xc7324acfL,
     0xeefbfbcfL, 0x40bb0032L, 0xeacbc86eL, 0x8adbae2bL,
     0xbb4739aaL, 0xfd36af3aL, 0x97fcf859L, 0x0d3d5535L,
     0x3f62627aL, 0x54901f90L,
#ifdef LENGTH_MULT_TO_32
     /* dss_g length = 864*/
     0x8b52bf83L, 0x0808c2b1L, 0x6ccb4a3eL, 0xcfdd8aa8L,
     0x4948b046L, 0x9881cdf4L, 0x85b013d8L, 0x3786a22aL,
     0xcb8ae9dbL, 0x5e2789adL, 0x1fcbd925L, 0x4afd3855L,
     0x83be7f67L, 0xe061b092L, 0xf21f6703L, 0x4c6a403eL,
     0x536aa8bdL, 0xb25e89b6L, 0xd259e165L, 0xd7f3b765L,
     0xd8bcc474L, 0x1a0a617dL, 0x8c28a054L, 0x95959fb0L,
     0x62546f2dL, 0xb450e1c2L, 0x277fef81L,
#endif
     /* dss_g length = 896*/
     0x2aea7490L, 0xa3ccd005L, 0x235a9211L, 0xacec7b6dL,
     0x5407fca5L, 0x5bc18515L, 0x93888b2aL, 0xed568ff9L,
     0xf31ef4e9L, 0x172a7e05L, 0x81656e2aL, 0x975b7a24L,
     0xd4d11f6eL, 0xd6ff8232L, 0x40722363L, 0xd18ba1f1L,
     0xb032e574L, 0x81b6c330L, 0x07d56033L, 0xb7cc5107L,
     0xbd8e2c10L, 0xe731c6a3L, 0xe567fcd6L, 0xbcde3440L,
     0xdbb31976L, 0x4e39d15aL, 0x4b0bc282L, 0x286b82b3L,
#ifdef LENGTH_MULT_TO_32
     /* dss_g length = 928*/
     0xd0e59f66L, 0xe875d7d2L, 0x2dca0707L, 0x266da5a2L,
     0x6d2080c2L, 0x759c69b3L, 0x74b4497fL, 0x4b630f56L,
     0x4995a294L, 0xf5c43edaL, 0x3a3d7951L, 0x617f22e9L,
     0xd45d1906L, 0x2726047fL, 0x660e27beL, 0xa588c1d3L,
     0xe300ceb2L, 0x8620ca06L, 0x76ae1110L, 0xe368f7b8L,
          0x2ded89a7L, 0x3ed62756L, 0x4bd85449L, 0xb406b73dL,
     0x32f725ecL, 0x6623989cL, 0xf7f06b1fL, 0x09544807L,
     0x97d051e6L,
#endif
     /* dss_g length = 960*/
     0xe87c4118L, 0x1287809aL, 0x51d62a0fL, 0xdad87fc8L,
     0xddce8dc6L, 0x898ad27cL, 0xaee196a9L, 0xd322e44cL,
     0x3a349d58L, 0x322acebdL, 0x9c215b48L, 0x2d776d04L,
     0x512341f1L, 0x2192aa5fL, 0x5cac8175L, 0xdc08f547L,
     0x4f11b0d9L, 0xb5bf85c9L, 0x9fbf7147L, 0x7f7a3c20L,
     0xa163dd8aL, 0xfcb20ae4L, 0x81628251L, 0x84f9b96eL,
     0x94bad652L, 0x9559bb90L, 0x6418ac51L, 0x4739db54L,
     0x9b7c3ec0L, 0x3e14ae96L,
#ifdef LENGTH_MULT_TO_32
     /* dss_g length = 992*/
     0x399a4effL, 0x1e5a5da9L, 0x4bad9040L, 0x5b9ac85dL,
     0x5464825bL, 0xe2ea7538L, 0x5defb957L, 0x4916bfa6L,
     0x93fe89beL, 0x919fe5daL, 0xe7a8791cL, 0xc186ef86L,
        0xa881bf63L, 0x9abe1e38L, 0x70b48b58L, 0x07199dfdL,
     0xc60a85e5L, 0xe07dab36L, 0xd6fe0c49L, 0x6619fdb4L,
     0x997ee55dL, 0x824f5f38L, 0xf80047ffL, 0x839db137L,
     0xe0db2daeL, 0x2f582a74L, 0x1bab6fb3L, 0x0e0262edL,
     0xd7f1bef2L, 0xa9aee566L, 0x6357bc35L,
#endif
     /* dss_g length = 1024*/
     0x76b2914dL, 0x9cf7fee4L, 0x07a1a136L, 0x18573808L,
     0x870c8f96L, 0x59f21f3cL, 0x4d4d6f01L, 0xafc6580eL,
     0xff1b99afL, 0x6c1fc019L, 0x1698e449L, 0x0aa24787L,
     0x768d6e68L, 0x27dc5ff0L, 0x630fca9eL, 0x998101aeL,
     0xf278688dL, 0x17ad7f3dL, 0x65d648aaL, 0x1802181fL,
     0x9ef5647cL, 0x58437081L, 0x93641f0dL, 0x6330ebe9L,
     0x5d2a033aL, 0x5639b68aL, 0xd77efa7cL, 0x4a624bdcL,
       0xfed55c6dL, 0x5effa8aaL, 0xae4a1138L, 0x3a73e993L
        };
#endif

#endif  /*INITIALIZ_PQG*/

#endif /*DSSNUMBER_H*/
