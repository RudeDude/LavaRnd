/*
 * tryrnd - try the random and random_libc interfaces
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: tryrnd.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <stdlib.h>
#include <string.h>

#include "LavaRnd/random.h"
#include "LavaRnd/random_libc.h"
#include "LavaRnd/lava_debug.h"
#include "LavaRnd/cleanup.h"

#define SMALL_BUF 25
#define DIE_ROLLS 10
#define SMALL_DIE 10
#define MED_DIE 10000
#define LARGE_DIE 1000000000000LL

char *program;	/* our name */

static void errno_chk(int exitval, char *func);


int
main(int argc, char *argv[])
{
#if defined(LAVA_DEBUG)
    char *debug_env = NULL;	/* $LAVA_DEBUG */
#endif /* LAVA_DEBUG */
    unsigned short int xsubi[3] = { 19937, 21701, 23209 };	/* fake seed arg */
    unsigned short int param[7] = { 0, 1, 2, 3, 4, 5, 6 };	/* fake seed arg */
    u_int8_t i8;	/* 8 bit unsigned int */
    u_int16_t i16;	/* 16 bit unsigned int */
    u_int32_t i32;	/* 32 bit unsigned int */
    u_int64_t i64;	/* 64 bit unsigned int */
    double dval;	/* double */
    long double ldval;	/* long double */
    int ret;	/* generic function return value */
    u_int8_t sbuf[SMALL_BUF];	/* small buffer */
    long int li;	/* long int value */
    int i;	/* int value */

    /*
     * parse args
     */
    program = argv[0];
    if (argc != 1) {
	fprintf(stderr, "usage: %s\n", program);
	exit(1);
    }

    /*
     * clear error code
     */
    lavarnd_errno = LAVAERR_OK;

    /*
     * setup for debugging
     */
#if defined(LAVA_DEBUG)
    ret = lava_init_debug();
    if (ret < 0) {
	fprintf(stderr, "%s: LavaRnd debug failed to initialize: %d\n",
		program, ret);
	exit(2);
    } else if (ret == 0) {
	debug_env = getenv("LAVA_DEBUG");
	if (debug_env == NULL) {
#  if 0
	    fprintf(stderr, "%s: $LAVA_DEBUG not found\n\n", program);
#  endif
	} else if (strlen(debug_env) <= 0) {
#  if 0
	    fprintf(stderr, "%s: $LAVA_DEBUG is empty\n\n", program);
#  endif
	} else {
	    fprintf(stderr,
		    "%s: LavaRnd libs not compiled with -DLAVA_DEBUG\n\n",
		    program);
	}
    } else {
	debug_env = getenv("LAVA_DEBUG");
	if (debug_env == NULL) {
	    fprintf(stderr,
		    "%s: LavaRnd debug initialized without $LAVA_DEBUG!\n",
		    program);
	    exit(3);
	}
	printf("debugging enabled: $LAVA_DEBUG is %s\n\n", debug_env);
	fflush(stdout);
    }
#else /* LAVA_DEBUG */
    printf("debugging disabled, compiled without -DLAVA_DEBUG\n\n");
    fflush(stdout);
#endif /* LAVA_DEBUG */

    /*
     * call random functions that return fixed sized values
     */
    i8 = random8();
    errno_chk(4, "random8");
    printf("random8: 0x%02x (%d)\n", i8, i8);
    i16 = random16();
    errno_chk(5, "random16");
    printf("random16: 0x%04x (%d)\n", i16, i16);
    i32 = random32();
    errno_chk(6, "random32");
    printf("random32: 0x%08x (%d)\n", i32, i32);
    i64 = random64();
    errno_chk(7, "random64");
    printf("random64: 0x%016llx (%lld)\n", i64, i64);

    /*
     * ask for a chunk of random data
     */
    ret = randomcpy(sbuf, sizeof(sbuf));
    errno_chk(8, "randomcpy");
    if (ret > 0) {
	printf("randomcpy: returned %d octets\n", ret);
	printf("randomcpy: data: 0x");
	for (i = 0; i < ret; ++i) {
	    printf("%02x", sbuf[i]);
	}
	fputc('\n', stdout);
    } else {
	printf("randomcpy: error return: %d\n", ret);
    }
    fputc('\n', stdout);

    /*
     * ask for floating point values
     */
    dval = drandom();
    errno_chk(9, "drandom");
    printf("drandom: %.16f\n", dval);
    dval = dcrandom();
    errno_chk(10, "dcrandom");
    printf("dcrandom: %.16f\n", dval);
    ldval = drandom();
    errno_chk(11, "drandom");
    printf("ldrandom: %.32Lf\n", ldval);
    ldval = dcrandom();
    errno_chk(12, "dcrandom");
    printf("ldcrandom: %.32Lf\n", ldval);
    fputc('\n', stdout);

    /*
     * ask for some dice values
     */
    printf("random_val: [0,%d): ", SMALL_DIE);
    for (i = 0; i < DIE_ROLLS; ++i) {
	i32 = random_val(SMALL_DIE);
	errno_chk(13, "random_val");
	printf(" %d", i32);
    }
    fputc('\n', stdout);
    printf("random_val: [0,%d): ", MED_DIE);
    for (i = 0; i < DIE_ROLLS; ++i) {
	i32 = random_val(MED_DIE);
	errno_chk(14, "random_val");
	printf(" %04d", i32);
    }
    fputc('\n', stdout);
    printf("random_lval: [0,%lld): ", LARGE_DIE);
    for (i = 0; i < DIE_ROLLS / 2 - 2; ++i) {
	i64 = random_lval(LARGE_DIE);
	errno_chk(15, "random_lval");
	printf(" %012lld", i64);
    }
    fputc('\n', stdout);
    fputc('\n', stdout);

    /*
     * fake libc seed calls
     */
    printf("about to call srand()\n");
    srand(23209);
    errno_chk(16, "srand");
    printf("about to call srandom()\n");
    srandom(23209);
    errno_chk(17, "srandom");
    printf("about to call initstate()\n");
    (void)initstate(23209, NULL, 128);
    errno_chk(18, "initstate");
    printf("about to call setstate()\n");
    (void)setstate(NULL);
    errno_chk(19, "setstate");
    printf("about to call srand48()\n");
    srand48(23209);
    errno_chk(20, "srand48");
    printf("about to call seed48()\n");
    (void)seed48(xsubi);
    errno_chk(21, "seed48");
    printf("about to call lcong48()\n");
    (void)lcong48(param);
    errno_chk(22, "lcong48");
    fputc('\n', stdout);

    /*
     * libc-like interface  calls
     */
    i = rand();
    errno_chk(23, "rand");
    printf("rand: %d\n", i);
    li = random();
    errno_chk(24, "random");
    printf("random: %ld\n", li);
    dval = drand48();
    errno_chk(25, "drand48");
    printf("drand48: %.16f\n", dval);
    dval = erand48(xsubi);
    errno_chk(26, "erand48");
    printf("erand48: %.16f\n", dval);
    li = lrand48();
    errno_chk(27, "lrand48");
    printf("lrand48: %ld\n", li);
    li = nrand48(xsubi);
    errno_chk(28, "nrand48");
    printf("nrand48: %ld\n", li);
    li = mrand48();
    errno_chk(29, "mrand48");
    printf("mrand48: %ld\n", li);
    li = jrand48(xsubi);
    errno_chk(20, "jrand48");
    printf("jrand48: %ld\n", li);

    /*
     * cleanup - so things like valgrind will not complain about memory leaks
     */
    lava_dormant();

    /*
     * all done
     */
    return 0;
}


/*
 * errno_chk - check on the lavarnd_errno and lastop_errno values
 *
 * given:
 *      exitval         what to exit with if a LavaRnd error is found
 *      funcname        name of the LavaRnd function that was just called
 *
 * Does not return if either lavarnd_errno or lastop_errno != LAVAERR_OK.
 */
static void
errno_chk(int exitval, char *func)
{
    /*
     * firewall
     */
    if (func == NULL) {
	func = "((NULL_PTR))";
    }

    /*
     * ensure that lavarnd_errno is the same as lastop_errno
     */
    if (lavarnd_errno != lastop_errno) {
	fprintf(stderr,
		"%s: %s lavarnd_errno: %d (%s) != lastop_errno: %d (%s)",
		program, func, lavarnd_errno, lava_err_name(lavarnd_errno),
		lastop_errno, lava_err_name(lastop_errno));
    }

    /*
     * check lavarnd_errno
     */
    if (lavarnd_errno != LAVAERR_OK) {
	fprintf(stderr,
		"%s: %s lavarnd_errno: %d (%s)",
		program, func, lavarnd_errno, lava_err_name(lavarnd_errno));
    }

    /*
     * exit if a problem, return otherwise
     */
    if (lavarnd_errno != LAVAERR_OK || lavarnd_errno != lastop_errno) {
	lava_dormant();
	exit(exitval);
    }
    return;
}
