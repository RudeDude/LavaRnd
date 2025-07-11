/*
 * s100 - subtractive 100 shuffle generator
 *
 * @(#) $Revision: 10.2 $
 * @(#) $Id: s100.h,v 10.2 2003/11/09 22:37:03 lavarnd Exp $
 *
 * Copyright 1995,1999,2003 by Landon Curt Noll.  All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright, this permission notice, and the
 * disclaimer below appear in all of the following:
 *
 *      * supporting documentation
 *      * source copies
 *      * source works derived from this source
 *      * binaries derived from this source or from derived source
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Under source code control:	1995/01/07 09:45:25
 * File existed as early as:	1994
 *
 * chongo <was here> /\oo/\	http://www.isthe.com/chongo/
 * Share and enjoy!  :-)	http://www.isthe.com/chongo/tech/comp/calc/
 *
 * NOTE: This code was "borrowed from Calc".
 */


#if !defined(__LAVARND_S100_H__)
#  define __LAVARND_S100_H__

#  include <sys/types.h>
#  include "sha1.h"
#  include "lavaquality.h"


/*
 * subtractive 100 shuffle level
 *
 * SHUF_POWER - power of 2 size of the shuffle table
 *
 * This constant controls the size of the shuffle table.  It must be
 * at least 1 and must be < 32.
 *
 * Small values do not provide sufficient mixing.  The value should be
 * at least 4.
 *
 * The value of 7 is recommended because 2^7 == 120 ~= S100 == 100.
 *
 * Values beyond 10 are excessive.
 */
#  define SHUF_POWER (7)

/*
 * fundamental shuffle defines
 *
 * SHUF_SIZE - size of shuffle table
 *
 * The s100.c code depends on SHUF_SIZE being a power of 2 that is > 1.
 */
#  define SHUF_SIZE (1<<SHUF_POWER)

/*
 * fundamental s100 constants
 *
 * S100 - subtractive 100 slot count
 * SPIN_CYCLE - total cycles in a spin
 *
 * Don't change these values!  The algorithm depends on these EXACT values.
 */
#  define S100 (100)
#  define SPIN_CYCLE (1009)


/*
 * subtractive 100 shuffle sizes
 *
 * S100_U64 - length of struct s100_seed in u_int64_t's
 * S100_U32 - length of struct s100_seed in u_int32_t's
 * SEED_SHA1 - length of struct s100_seed in SHA-1 size's rounded up
 * SEED_U32 - SEED_SHA1 as expressed in multiples of u_int32_t's
 * S100_BUF - size of the s100 output buffer in u_int8_t's
 */
#  define S100_U64 (S100+SHUF_SIZE+1)
#  define S100_U32 (2*S100_U64)
#  define SEED_SHA1 ((S100_U32+SHA_DIGESTLONG-1)/SHA_DIGESTLONG)
#  define SEED_U32 (SEED_SHA1*SHA_DIGESTLONG)
#  define S100_BUF (S100*(int)sizeof(u_int64_t))


/*
 * s100 - subtractive 100 shuffle generator
 *
 * This defines the full state of a subtractive 100 shuffle generator.
 * Needless to say, there is nothing here that one should needs to change.
 */
struct s100_seed {
    u_int64_t prevout;	/* previous subtractive 100 output */
    u_int64_t shuf[SHUF_SIZE];	/* shuffle table slots */
    u_int64_t slot[S100];	/* subtractive 100 table slots */
};
struct s100_state {
    u_int64_t prevout;	/* previous subtractive 100 output */
    u_int64_t shuf[SHUF_SIZE];	/* shuffle table slots */
    u_int64_t slot[SPIN_CYCLE - S100];	/* full 1009 cycle slot buffer */
};
struct s_s100 {
    int seeded;	/* 1 ==> state has setup and seeded */
    int nextspin;	/* spin cycles to next reload */
    int seed_len;	/* seed length not counting default seed */
    int rndbuf_len;	/* output buffer octets at/after nxt_rnd */
    lavaqual bufqual;	/* rndbuf quality if rndbuf_len>0 */
    u_int8_t *nxt_rnd;	/* next output buffer write point or NULL */
    union {
	struct s100_seed seed;	/* s100 shuffle generator seed */
	struct s100_state state;	/* s100 shuffle generator seed */
	u_int32_t overlay[SEED_U32];	/* seed extended to SHA-1 multiple */
    } s;
    u_int8_t rndbuf[S100_BUF];	/* s100 output buffer */
};
typedef struct s_s100 s100shuf;


/*
 * external functions
 */
extern int s100_load_size(void);
extern void s100_load(s100shuf *s100, u_int8_t * buf, int len);
extern void s100_unload(s100shuf *s100);
extern int s100_turn(s100shuf *s100, u_int64_t * ptr);
extern int s100_randomcpy(s100shuf *s100, u_int8_t * buf, int len);
extern int s100_loadleft(s100shuf *s100);
extern lavaqual s100_quality(s100shuf *s100);


#endif /* __LAVARND_S100_H__ */
