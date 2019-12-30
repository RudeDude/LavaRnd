/*
 * random - high level random number interface without libc overrides
 *
 * This is a high level interface to random number services.  These
 * functions do not conflict with random number services found in libc.
 * See random_libc.c for libc replacement functions.
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: random.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


#include <stdio.h>

#include "LavaRnd/lavaerr.h"
#include "LavaRnd/fetchlava.h"
#include "LavaRnd/random.h"
#include "LavaRnd/lava_callback.h"
#include "LavaRnd/lava_debug.h"

#if defined(DMALLOC)
#  include <dmalloc.h>
#endif


/*
 * default compiled in callback
 *
 * The lava_callback is the default callback that is used by this high level
 * random number interface.  It is typically initialized by one of the
 * libLava_XYZ.c that is linked into a liblava_XYZ.a library.
 *
 * The default compiled in callback may be overridden by calling the
 * set_lava_callback() function below.
 */
extern lavaback lava_callback;


/*
 * set_lava_callback - change the default callback for random.c & random_libc.c
 *
 * given:
 *      callback        call to set, or NULL to just return
 *
 * returns:
 *      previous callback
 *
 * NOTE: Call with a NULL ptr to just inquire about the callback.
 */
lavaback
set_lava_callback(lavaback callback)
{
    lavaback old = lava_callback;	/* previous callback */

    /*
     * set callback if non-NULL
     */
    if (callback != NULL) {
	lava_callback = callback;
    }

    /*
     * return previous callback
     */
    return old;
}


/*
 * random8 - return an 8 bit random value
 */
u_int8_t
random8(void)
{
    u_int8_t value;	/* random return value */

#if defined(LAVA_DEBUG)
    int ret;	/* return code */
    int amount;	/* amount of data returned */
#endif /* LAVA_DEBUG */

    /*
     * fetch the value
     */
    lastop_errno = LAVAERR_OK;
#if defined(LAVA_DEBUG)
    ret = raw_random(&value, sizeof(value), lava_callback, &amount, FALSE);
    if (ret == 0) {
	LAVA_DEBUG_E("random8", lastop_errno);
    } else if (ret < 0) {
	LAVA_DEBUG_E("random8", ret);
    } else if (amount != sizeof(value)) {
	LAVA_DEBUG_E("random8", LAVAERR_PARTIAL);
    }
#else /* LAVA_DEBUG */
    (void)raw_random(&value, sizeof(value), lava_callback, NULL, FALSE);
#endif /* LAVA_DEBUG */

    /*
     * return random value
     */
    return value;
}


/*
 * random16 - return an 16 bit random value
 */
u_int16_t
random16(void)
{
    u_int16_t value;	/* random return value */

#if defined(LAVA_DEBUG)
    int ret;	/* return code */
    int amount;	/* amount of data returned */
#endif /* LAVA_DEBUG */

    /*
     * fetch the value
     */
    lastop_errno = LAVAERR_OK;
#if defined(LAVA_DEBUG)
    ret = raw_random((u_int8_t *) & value, sizeof(value), lava_callback,
		     &amount, FALSE);
    if (ret == 0) {
	LAVA_DEBUG_E("random16", lastop_errno);
    } else if (ret < 0) {
	LAVA_DEBUG_E("random16", ret);
    } else if (amount != sizeof(value)) {
	LAVA_DEBUG_E("random16", LAVAERR_PARTIAL);
    }
#else /* LAVA_DEBUG */
    (void)raw_random((u_int8_t *) & value, sizeof(value), lava_callback,
		     NULL, FALSE);
#endif /* LAVA_DEBUG */

    /*
     * return random value
     */
    return value;
}


/*
 * random32 - return an 32 bit random value
 */
u_int32_t
random32(void)
{
    u_int32_t value;	/* random return value */

#if defined(LAVA_DEBUG)
    int ret;	/* return code */
    int amount;	/* amount of data returned */
#endif /* LAVA_DEBUG */

    /*
     * fetch the value
     */
    lastop_errno = LAVAERR_OK;
#if defined(LAVA_DEBUG)
    ret = raw_random((u_int8_t *) & value, sizeof(value), lava_callback,
		     &amount, FALSE);
    if (ret == 0) {
	LAVA_DEBUG_E("random32", lastop_errno);
    } else if (ret < 0) {
	LAVA_DEBUG_E("random32", ret);
    } else if (amount != sizeof(value)) {
	LAVA_DEBUG_E("random32", LAVAERR_PARTIAL);
    }
#else /* LAVA_DEBUG */
    (void)raw_random((u_int8_t *) & value, sizeof(value), lava_callback,
		     NULL, FALSE);
#endif /* LAVA_DEBUG */

    /*
     * return random value
     */
    return value;
}


/*
 * random64 - return an 64 bit random value
 */
u_int64_t
random64(void)
{
    u_int64_t value;	/* random return value */

#if defined(LAVA_DEBUG)
    int ret;	/* return code */
    int amount;	/* amount of data returned */
#endif /* LAVA_DEBUG */

    /*
     * fetch the value
     */
    lastop_errno = LAVAERR_OK;
#if defined(LAVA_DEBUG)
    ret = raw_random((u_int8_t *) & value, sizeof(value), lava_callback,
		     &amount, FALSE);
    if (ret == 0) {
	LAVA_DEBUG_E("random64", lastop_errno);
    } else if (ret < 0) {
	LAVA_DEBUG_E("random64", ret);
    } else if (amount != sizeof(value)) {
	LAVA_DEBUG_E("random64", LAVAERR_PARTIAL);
    }
#else /* LAVA_DEBUG */
    (void)raw_random((u_int8_t *) & value, sizeof(value), lava_callback,
		     NULL, FALSE);
#endif /* LAVA_DEBUG */

    /*
     * return random value
     */
    return value;
}


/*
 * randomcpy - fill a buffer with random data
 *
 * given:
 *      ptr     where to place random data
 *      len     octets to copy
 *
 * returns:
 *      number of octets returned, or < 0 ==> error, 0 ==> no data returned
 */
int
randomcpy(u_int8_t * ptr, int len)
{
    int ret;	/* return code */
    int amount;	/* amount of data returned */

    /*
     * firewall
     */
    lastop_errno = LAVAERR_OK;
    if (ptr == NULL || len <= 0) {
	lastop_errno = LAVAERR_BADARG;
	return LAVAERR_BADARG;
    }

    /*
     * fetch the value
     */
    ret = raw_random(ptr, len, lava_callback, &amount, TRUE);

    /*
     * return count or error
     */
    if (ret < 0) {
    	lastop_errno = ret;
	return ret;
    } else if (ret == 0 && lastop_errno != LAVAERR_OK) {
	return lastop_errno;
    }
    return amount;
}


/*
 * drandom - returns random double in the open interval [0.0, 1.0)
 *
 * NOTE: We do not need to touch lastop_errno because random64()
 *       will do it for us.
 */
double
drandom(void)
{
    /*
     * Return a double value >=0 and < 1.0
     */
    return ((double)(random64() & BITS_53) / DPOW_53);
}


/*
 * dcrandom - returns random double in the closed interval [0.0, 1.0]
 *
 * NOTE: We do not need to touch lastop_errno because random64()
 *       will do it for us.
 */
double
dcrandom(void)
{
    /*
     * Return a double value >=0 and <= 1.0
     */
    return ((double)(random64() & BITS_53) / DBITS_53);
}


/*
 * ldrandom - returns random long double in the open interval [0.0, 1.0)
 *
 * NOTE: We do not need to touch lastop_errno because random64()
 *       will do it for us.
 */
long double
ldrandom(void)
{
    /*
     * Return a long double value >=0 and < 1.0
     */
    return ((long double)random64() / LDPOW_64);
}


/*
 * ldcrandom - returns random double in the closed interval [0.0, 1.0]
 *
 * NOTE: We do not need to touch lastop_errno because random64()
 *       will do it for us.
 */
long double
ldcrandom(void)
{
    /*
     * Return a long double value >=0 and <= 1.0
     */
    return ((long double)random64() / LDBITS_64);
}


/*
 * random_val - roll a die and return a value from [0,n)  (32 bit max)
 *
 * We return a value that is >= 0 and < beyond.
 *
 * The calculation is performed carefully to avoid bias results.
 * The naive programmer would do (random32() % largest), which
 * produces biased results when 'largest' is not a power of 2.
 *
 * given:
 *      beyond          smallest integer>0 NOT to return
 *
 * returns:
 *      a random number in the interval [0, largest)
 */
u_int32_t
random_val(u_int32_t beyond)
{
    u_int32_t slice;	/* slice of [0,2^32) to use */
    u_int32_t answer;	/* the 'die' face */
    int tmp_errno;	/* temp holder of tmp_errno */

    /*
     * firewall - deal with 1 and 0 sided dice
     */
    tmp_errno = LAVAERR_OK;
    if (beyond <= 1) {
	return (u_int32_t) 0;
    }

    /*
     * roll the dice avoiding a non-existent side number
     */
    if (beyond <= BITS_8) {
	slice = BITS_8 / beyond;
	while ((answer = random8() / slice) >= beyond) {
	    tmp_errno = (lavarnd_errno < 0) ? lavarnd_errno : tmp_errno;
	}
    } else if (beyond <= BITS_16) {
	slice = BITS_16 / beyond;
	while ((answer = random16() / slice) >= beyond) {
	    tmp_errno = (lavarnd_errno < 0) ? lavarnd_errno : tmp_errno;
	}
    } else {
	slice = BITS_32 / beyond;
	while ((answer = random32() / slice) >= beyond) {
	    tmp_errno = (lavarnd_errno < 0) ? lavarnd_errno : tmp_errno;
	}
    }
    lavarnd_errno = tmp_errno;
    return answer;
}


/*
 * random_lval - roll a die and return a value from [0,n)  (64 bit max)
 *
 * We return a value that is >= 0 and < beyond.
 *
 * The calculation is performed carefully to avoid bias results.
 * The naive programmer would do (random64() % largest), which
 * produces biased results when 'largest' is not a power of 2.
 *
 * given:
 *      beyond          smallest integer>0 NOT to return
 *
 * returns:
 *      a random number in the interval [0, largest)
 */
u_int64_t
random_lval(u_int64_t beyond)
{
    u_int64_t slice;	/* slice of [0,2^64) to use */
    u_int64_t answer;	/* the ``die'' face */
    int tmp_errno;	/* temp holder of tmp_errno */

    /*
     * firewall - deal with 1 and 0 sided dice
     */
    tmp_errno = LAVAERR_OK;
    if (beyond <= 1) {
	return (u_int64_t) 0;
    }

    /*
     * roll the dice avoiding a non-existent side number
     */
    if (beyond <= BITS_8) {
	slice = BITS_8 / beyond;
	while ((answer = random8() / slice) >= beyond) {
	    tmp_errno = (lavarnd_errno < 0) ? lavarnd_errno : tmp_errno;
	}
    } else if (beyond <= BITS_16) {
	slice = BITS_16 / beyond;
	while ((answer = random16() / slice) >= beyond) {
	    tmp_errno = (lavarnd_errno < 0) ? lavarnd_errno : tmp_errno;
	}
    } else if (beyond <= BITS_32) {
	slice = BITS_32 / beyond;
	while ((answer = random32() / slice) >= beyond) {
	    tmp_errno = (lavarnd_errno < 0) ? lavarnd_errno : tmp_errno;
	}
    } else {
	slice = BITS_64 / beyond;
	while ((answer = random64() / slice) >= beyond) {
	    tmp_errno = (lavarnd_errno < 0) ? lavarnd_errno : tmp_errno;
	}
    }
    lavarnd_errno = tmp_errno;
    return answer;
}
