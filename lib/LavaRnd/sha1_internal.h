/*
 * This section was 'borrowed' from the Digest-MD5-2.09 module.
 * In particular it is based on the file Digest-MD5-2.09/SHA1/SHA1.xs.
 *
 * Why is it here?  Well:
 *
 *      1) For speed we do not want to jump in/out of the C/Perl
 *         interface and carry the turned buffers for very long
 *         in main memory.
 *
 *      2) Landon Curt Noll has some SHA-1 performance improvements
 *         which are not found in the original source code.
 *
 *      3) To make some minor changes to the code such as to remove
 *         a function that was only used by the Digest::SHA1 object
 *         constructor which we do not need.
 *
 * In all other respects the code is the same.  In particular this
 * section of code implements the standard SHA-1 algorithm as well
 * as the Digest::SHA1 module, only a little faster.
 */

/*
 * NIST Secure Hash Algorithm
 * heavily modified by Uwe Hollerbach <uh@alumni.caltech edu>
 * from Peter C. Gutmann's implementation as found in
 * Applied Cryptography by Bruce Schneier
 * Further modifications to include the "UNRAVEL" stuff, below and
 * additional mods by chongo.
 */

/* This code is in the public domain */


#if !defined(__LAVARND_SHA1_INTERNAL_H__)
#  define __LAVARND_SHA1_INTERNAL_H__

/* UNRAVEL should be fastest & biggest */
/* UNROLL_LOOPS should be just as big, but slightly slower */
/* both undefined should be smallest and slowest */

#  define SHA_VERSION 1
#  define UNRAVEL
#  undef UNROLL_LOOPS

/*
 * The SHS1 f()-functions.  The f1 and f3 functions can be optimized
 * to save one boolean operation each - thanks to Rich Schroeppel,
 * rcs at cs dot arizona dot edu for discovering this.
 *
 * f1: ((x&y) | (~x&z)) == (z ^ (x&(y^z)))
 * f3: ((x&y) | (x&z) | (y&z)) == ((x&y) | (z&(x|y)))
 */
#  define f1(x,y,z)	(z ^ (x&(y^z)))	/* Rounds  0-19 */
#  define f2(x,y,z)	(x^y^z)	/* Rounds 20-39 */
#  define f3(x,y,z)	((x&y) | (z&(x|y)))	/* Rounds 40-59 */
#  define f4(x,y,z)	(x^y^z)	/* Rounds 60-79 */

/* SHA constants */
#  define CONST1		0x5a827999L
#  define CONST2		0x6ed9eba1L
#  define CONST3		0x8f1bbcdcL
#  define CONST4		0xca62c1d6L

/* truncate to 32 bits -- should be a null op on 32-bit machines */
#  define T32(x)	((x) & 0xffffffffL)

/* 32-bit rotate */
#  define R32(x,n)	T32(((x << n) | (x >> (32 - n))))

/* the generic case, for when the overall rotation is not unraveled */
#  define FG(n)	\
    T = T32(R32(A,5) + f##n(B,C,D) + E + *WP++ + CONST##n);	\
    E = D; D = C; C = R32(B,30); B = A; A = T

/* specific cases, for when the overall rotation is unraveled */
#  define FA(n)	\
    T = T32(R32(A,5) + f##n(B,C,D) + E + *WP++ + CONST##n); B = R32(B,30)

#  define FB(n)	\
    E = T32(R32(T,5) + f##n(A,B,C) + D + *WP++ + CONST##n); A = R32(A,30)

#  define FC(n)	\
    D = T32(R32(E,5) + f##n(T,A,B) + C + *WP++ + CONST##n); T = R32(T,30)

#  define FD(n)	\
    C = T32(R32(D,5) + f##n(E,T,A) + B + *WP++ + CONST##n); E = R32(E,30)

#  define FE(n)	\
    B = T32(R32(C,5) + f##n(D,E,T) + A + *WP++ + CONST##n); D = R32(D,30)

#  define FT(n)	\
    A = T32(R32(B,5) + f##n(C,D,E) + T + *WP++ + CONST##n); C = R32(C,30)


#endif /* __LAVARND_SHA1_INTERNAL_H__ */
