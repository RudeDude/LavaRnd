/*
 * s100_internal - internal details of the subtractive 100 shuffle generator
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: s100_internal.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


#if !defined(__LAVARND_S100_INTERNAL_H__)
#  define __LAVARND_S100_INTERNAL_H__


/*
 * fundamental s100 constants
 *
 * S100_PTR_OFFSET - 1st and 2nd pointer offset
 *
 * Don't change this value!  The algorithm depends on this EXACT value.
 */
#  define S100_PTR_OFFSET (37)


/*
 * fundamental subtractive 100 shuffle defines
 *
 * SHUF_MASK - mask for shuffle table slot selection
 * S100_OUTPUT_CYCLE - output values (of used data) in a spin cycle
 * NEXT_SPIN - output values tossed (not used) in a spin cycle
 *
 * These constants are used by the s100.c code to implement the
 * subtractive 100 shuffle algorithm.
 *
 * Don't change this value!  The algorithm depends on this EXACT values.
 */
#  define S100_SHUF_MASK (SHUF_SIZE-1)


/*
 * subtractive 100 reseed frequency
 *
 * S100_RESEED_CYCLES - number of spin cycles before a reseed is recommended
 */
#  define S100_RESEED_CYCLES (SPIN_CYCLE*SPIN_CYCLE)


#endif /* __LAVARND_S100_INTERNAL_H__ */
