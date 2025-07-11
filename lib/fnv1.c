/*
 * fnv1_hash - perform a Fowler/Noll/Vo-1a 64 bit hash
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: fnv1.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
 *
 * Copyright (c) 2000-2003 by Landon Curt Noll and Simon Cooper.
 * All Rights Reserved.
 *
 * This is open software; you can redistribute it and/or modify it under
 * the terms of the version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 *
 * This software is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General
 * Public License for more details.
 *
 * The file COPYING contains important info about Licenses and Copyrights.
 * Please read the COPYING file for details about this open software.
 *
 * A copy of version 2.1 of the GNU Lesser General Public License is
 * distributed with calc under the filename COPYING-LGPL.  You should have
 * received a copy with calc; if not, write to Free Software Foundation, Inc.
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 *
 * For more information on LavaRnd: http://www.LavaRnd.org
 *
 * Share and enjoy! :-)
 */


#include "LavaRnd/fnv1.h"

#if defined(DMALLOC)
#  include <dmalloc.h>
#endif


/*
 * FNV-1a initial basis
 *
 * We start the hash at a non-zero value at the beginning so that
 * hashing blocks of data with all 0 bits do not map onto the same
 * 0 hash value.  The virgin value that we use below is the hash value
 * that we would get from following 32 ASCII characters:
 *
 *              chongo <Landon Curt Noll> /\../\
 *
 * Note that the \'s above are not back-slashing escape characters.
 * They are literal ASCII  backslash 0x5c characters.
 *
 * The effect of this virgin initial value is the same as starting
 * with 0 and pre-pending those 32 characters onto the data being
 * hashed.
 *
 * Yes, even with this non-zero virgin value there is a set of data
 * that will result in a zero hash value.  Worse, appending any
 * about of zero bytes will continue to produce a zero hash value.
 * But that would happen with any initial value so long as the
 * hash of the initial was the `inverse' of the virgin prefix string.
 *
 * But then again for any hash function, there exists sets of data
 * which that the hash of every member is the same value.  That is
 * life with many to few mapping functions.  All we do here is to
 * prevent sets whose members consist of 0 or more bytes of 0's from
 * being such an awkward set.
 *
 * And yes, someone can figure out what the magic 'inverse' of the
 * 32 ASCII character are ... but this hash function is NOT intended
 * to be a cryptographic hash function, just a fast and reasonably
 * good hash function.
 */
#define FNV1_64_BASIS ((u_int64_t)(0xcbf29ce484222325ULL))

/*
 * FNV prime multiplier
 *
 * See:
 *
 *      http://www.isthe.com/chongo/tech/comp/fnv/
 *
 * for details on the FNV hash algorithm,
 */
#define FNV_PRIME ((u_int64_t)1099511628211ULL)


/*
 * fnv1_hash - perform a Fowler/Noll/Vo-1a 64 bit hash
 *
 * input:
 *      buf     - start of buffer to hash
 *      len     - length of buffer in octets
 *      hval    - the hash value to modify
 *
 * returns:
 *      64 bit hash as a static u_int64_t structure
 */
u_int64_t
fnv1_hash(u_int8_t * buf, u_int32_t len)
{
    u_int64_t hval;	/* current hash value */
    u_int8_t *buf_end = buf + len;	/* beyond end of hash area */

    /*
     * FNV-1a - Fowler/Noll/Vo-1a 64 bit hash
     *
     * The basis of this hash algorithm was taken from an idea sent
     * as reviewer comments to the IEEE POSIX P1003.2 committee by:
     *
     *      Phong Vo (http://www.research.att.com/info/kpv/)
     *      Glenn Fowler (http://www.research.att.com/~gsf/)
     *
     * In a subsequent ballot round:
     *
     *      Landon Curt Noll (http://www.isthe.com/chongo/)
     *
     * improved on their algorithm.  Some people tried this hash
     * and found that it worked rather well.  In an EMail message
     * to Landon, they named it ``Fowler/Noll/Vo'' or the FNV hash.
     *
     * FNV hashes are architected to be fast while maintaining a low
     * collision rate. The FNV speed allows one to quickly hash lots
     * of data while maintaining a reasonable collision rate.  See:
     *
     *      http://www.isthe.com/chongo/tech/comp/fnv/
     *
     * for more details as well as other forms of the FNV hash.
     */
    /* hash each octet of the buffer */
    for (hval = FNV1_64_BASIS; buf < buf_end; ++buf) {

	/* xor the bottom with the current octet */
	hval ^= (u_int64_t) (*buf);

	/* multiply by the FNV magic prime mod 2^64 using 64 bit longs */
#if !defined(NO_FNV_OPTIMIZATION)
	hval *= FNV_PRIME;
#else
	hval += (hval << 1) + (hval << 4) + (hval << 5) +
	  (hval << 7) + (hval << 8) + (hval << 40);
#endif
    }

    /* return our hash value */
    return hval;
}


/*
 * fnv_seq - FNV sequence number
 *
 * We will return some value within the closed interval [0.0, 1.0].  The
 * value returned is a non-random value based on the 64 bit FNV-1 hash of
 * current and previous values supplied to this function.  We convert
 * the 64 bit FNV hash value into a double that is >= 0.0 and <= 1.0.
 *
 * It is important to understand that the fnv_seq() return value is
 * NOT random.   It will typically be well distributed over its range.
 *
 * For example in the following code:
 *
 *      u_int32_t count, i;
 *      double seq;
 *
 *      for (i=0; i < count; ++i) {
 *          seq = fnv_seq();
 *      }
 *
 * will find that seq takes on values well distributed over the
 * closed range [0.0, 1.0].
 *
 * Why is this useful?  If one wants to generate a predictable and
 * deterministic sequence of nearly uniformly distributed values,
 * fnv_seq() will do a reasonable job.
 *
 * How well distributed is fnv_seq()?  Look at the above for-loop.  If
 * we break up the closed range [0.0, 1.0] into 101 equal-sized slots and
 * apply a 100-degree-of-freedom Chi^2 test.  For a 'count' of 2^32-1:
 *
 *      Chi^2 with 100 degrees of freedom: 73.34171
 *      Chi^2 estimation of the null hypothesis is between 97.5% and 99%
 *
 * Again, there is no claim on the randomness of the fnv_seq() values.
 * So what good are these sequence values?  They provide a reasonable
 * and fair coverage of values >= 0.0 and <= 1.0.
 *
 * return:
 *      a value >= 0.0 and <= 1.0 based on the on-going hash of term values
 */
double
fnv_seq(void)
{
    static u_int64_t state = FNV1_64_BASIS;	/* FNV hash value */
    static u_int32_t term = 0;	/* term values to FNV hash */

    /*
     * hash all of the octets of the value argument
     */
    state ^= (u_int64_t) (term & 0xff);
    state *= FNV_PRIME;
    state ^= (u_int64_t) ((term >> 8) & 0xff);
    state *= FNV_PRIME;
    state ^= (u_int64_t) ((term >> 16) & 0xff);
    state *= FNV_PRIME;
    state ^= (u_int64_t) ((term >> 24) & 0xff);
    state *= FNV_PRIME;

    /*
     * prep for next call
     */
    ++term;

    /*
     * return a number over the closed internal [0.0, 1.0]
     *
     * We divide by 2^64-1 so that the state value will map into
     * the closed internal [0.0, 1.0].  We only use the bottom 52
     * bits of the FNV hash value because an IEEE double typically
     * only has 52 bits of precision in the mantissa.
     */
    return (double)(state & ((1ULL << 52) - 1ULL)) / (double)((1ULL << 52) -
							      1ULL);
}
