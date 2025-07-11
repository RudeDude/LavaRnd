/*
 * pool - lavapool operations
 *
 * @(#) $Revision: 10.2 $
 * @(#) $Id: pool.c,v 10.2 2003/11/09 22:37:03 lavarnd Exp $
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
#include <malloc.h>
#include <string.h>

#include "LavaRnd/lavaerr.h"
#include "LavaRnd/rawio.h"
#include "LavaRnd/sha1.h"
#include "LavaRnd/lavarnd.h"

#include "pool.h"
#include "dbg.h"
#include "cfg_lavapool.h"

#if defined(DMALLOC)
#  include <dmalloc.h>
#endif

#define MAX_ALPHA 8.0		/* highest allowed alpha rate to use */
#define NORM_ALPHA 1.0		/* normal alpha rate to use */


/*
 * lavapool - where cryptographically strong LavaRnd data resides
 */
static u_int8_t *pool = NULL;	/* random daemon pool */
static int32_t poollen = 0;	/* lavapool data in pool, .. pool[poollen-1] */
static int32_t maxlen = 0;	/* allocated length of pool */


/*
 * init_pool - initialize the lavapool
 *
 * given:
 *      size    size of the lavapool in octets
 */
void
init_pool(u_int32_t size)
{
    /*
     * round the pool size up to the next SHA_DIGESTSIZE
     */
    size = SHA_DIGESTSIZE * ((size - 1 + SHA_DIGESTSIZE) / SHA_DIGESTSIZE);
    dbg(2, "init_pool", "pool size will be: %d", size);

    /*
     * create the pool
     */
    pool = (u_int8_t *) malloc(size);
    if (pool == NULL) {
	fatal(7, "init_pool", "unable to allocated a pool of %u octets", size);
	/*NOTREACHED*/
    }
    maxlen = size;
    poollen = 0;
    return;
}


/*
 * fill_pool_from_fd - perform a lavapool filling operation from an open file
 *
 * given:
 *      fd      descriptor containing LavaRnd data
 *
 * returns:
 *      chars added or <0 ==> error
 */
int
fill_pool_from_fd(int fd)
{
    u_int32_t need;	/* max amount of LavaRnd data needed */
    u_int32_t len;	/* amount of LavaRnd to try to read */
    int ret;	/* function return call */

    /*
     * firewall
     */
    if (fd < 0) {
	warn("fill_pool_from_fd", "invalid descriptor: %d", fd);
	return LAVAERR_BADARG;
    }

    /*
     * find a need ...
     */
    if (poollen >= maxlen) {
	dbg(5, "fill_pool_from_fd", "pool is too full: %d", poollen);
	return 0;
    }
    need = maxlen - poollen;

    /*
     * ... and fill it
     */
    len = ((need > MAXPOOL_IO) ? MAXPOOL_IO : need);
    dbg(5, "fill_pool_from_fd", "pool level: %d, need: %d, len: %d",
	poollen, need, len);
    ret = nilblock_read(fd, pool + poollen, len, FALSE);
    if (ret > 0) {
	poollen += ret;
    }
    dbg(5, "fill_pool_from_fd", "nilblock_read on %d: %d, pool level: %d",
	fd, ret, poollen);
    return ret;
}


/*
 * fill_pool_from_chaos - lavapool filling by LavaRnd processing chaos data
 *
 * usage:
 *      buf     buffer of chaos data
 *      buflen  length of chaos buffer
 *
 * returns:
 *      >0 ==> chars added, 0 ==> pool is too full, or <0 ==> error
 */
int
fill_pool_from_chaos(void *buf, int buflen)
{
    double factor;	/* pool fill rate factor */
    double rate;	/* alpha filling rate */
    int addlen;	/* amount of data added to the pool, <0 ==> error */

    /*
     * firewall
     */
    if (buf == NULL || buflen < 0) {
	return LAVAERR_BADARG;
    }
    if (buflen == 0) {
	/* nothing to do, not an error */
	return 0;
    }

    /*
     * determine the output rate
     */
    factor = pool_rate_factor();
    if (factor < 0.0) {
	dbg(5, "fill_pool_from_chaos", "pool is too full: %d", poollen);
	return 0;
    }
    rate = MAX_ALPHA * factor + NORM_ALPHA * (1.0 - factor);

    /*
     * add to the pool if there is room
     */
    dbg(3, "fill_pool_from_chaos", "factor: %.3f, rate: %.3f", factor, rate);
    addlen = lavarnd(cfg_lavapool.prefix, buf, buflen, rate, pool + poollen,
		     maxlen - poollen);
    if (addlen < 0) {
	warn("fill_pool_from_chaos",
	     "lavarnd(%d,buf,%d,%.3f,pool+%d,%d) error: %d",
	     cfg_lavapool.prefix, buflen, rate, poollen, maxlen - poollen,
	     addlen);
	return addlen;
    }
    poollen += addlen;
    dbg(3, "fill_pool_from_chaos", "added: %d, poollen: %d", addlen, poollen);
    return addlen;
}


/*
 * drain_pool - perform a lavapool draining operation
 *
 * given:
 *      cnt     amount of data requested
 *
 * returns:
 *      chars drained
 */
int
drain_pool(u_int8_t * buf, int cnt)
{
    int cpycnt;	/* LavaRnd copy length */

    /*
     * firewall
     */
    if (buf == NULL) {
	fatal(11, "drain_pool", "NULL arg");
	/*NOTREACHED*/
    }
    if (cnt <= 0) {
	warn("drain_pool", "bogus drain length: %d", cnt);
	return 0;
    }

    /*
     * determine how much we can copy
     */
    cpycnt = (cnt <= poollen) ? cnt : poollen;
    if (cpycnt <= 0) {
	return 0;
    }

    /*
     * copy out data
     */
    memcpy(buf, pool + poollen - cpycnt, cpycnt);

    /*
     * perform accounting
     */
    poollen -= cpycnt;
    dbg(3, "drain_pool", "drained %d octets, pool now has %d", cpycnt,
	poollen);
    return cpycnt;
}


/*
 * pool_level - return the amount of data in the pool
 *
 * returns:
 *      0 ==> pool is empty, >0 ==> number of octets in the pool
 */
u_int32_t
pool_level(void)
{
    return ((poollen > maxlen) ? maxlen : poollen);
}


/*
 * pool_frac - return the faction of the pool
 *
 * returns:
 *      faction of the pool level: 0.0 ==> empty, 1.0 ==> full
 */
double
pool_frac(void)
{
    return ((double)pool_level() / (double)maxlen);
}


/*
 * pool_rate_factor - determine the suggested alpha rate for a driver
 *
 * returns:
 *      1.0 ==> fill at highest speed,
 *      >0.0 ==> fill at a faster than normal speed,
 *      0.0 ==> fill at normal speed,
 *      -1.0 ==> pool is full, do not fill
 */
double
pool_rate_factor(void)
{
    if (poollen > maxlen - SHA_DIGESTSIZE) {
	/* pool is too full to fill */
	return -1.0;
    } else if (poollen > cfg_lavapool.slowpool) {
	/* fill at the normal rate */
	return 0.0;
    } else if (poollen > cfg_lavapool.fastpool &&
	       cfg_lavapool.slowpool > cfg_lavapool.fastpool) {
	/* fill between fastest and normal rate */
	return (double)(cfg_lavapool.slowpool - poollen) /
	  (double)(cfg_lavapool.slowpool - cfg_lavapool.fastpool);
    } else {
	/* fill at the fastest rate */
	return 1.0;
    }
}


/*
 * free_pool - close down and free the lavapool
 */
void
free_pool(void)
{
    if (pool != NULL) {
	free(pool);
	pool = NULL;
    }
    poollen = 0;
    maxlen = 0;
}
