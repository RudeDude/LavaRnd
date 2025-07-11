/*
 * chk_lavarnd - validity check the core of the LavaRnd algorithm
 *
 * usage:
 * 	chk_lavarnd [-v level]
 *
 * 	level		debug level
 *
 * If everything is OK, this program will exit 0.  It will exit non-zero
 * if there is some sort of problem.
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: chk_lavarnd.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <stdarg.h>
#include <string.h>

#include "LavaRnd/lavarnd.h"
#include "LavaRnd/sysstuff.h"

#if defined(DMALLOC)
#include <dmalloc.h>
#endif


/*
 * static declarations
 */
static char *program;		/* our name */
static int dbg_lvl = 1;		/* debug level, 0 => none */
static void *x_malloc(int len);
static void x_free(void *buf);
static void dbg(int level, char *fmt, ...);
static void fatal(int code, char *fmt, ...);


/*
 * static test buffers
 */
static char alpha[] = "abcdefghijklmnopqrstuvwxyz";
static char cap_alpha[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static int turn_len;		/* length of the turned buffer */
static u_int8_t *turn;		/* turned buffer */
static double rate_set[10] = {
    4.0, 3.0, 2.0, 1.5, 1.0, 0.75, 0.6666, 0.5, 0.3333, 0.25
};
static int nway350000_set[10] = {
    149, 131, 107, 91, 73, 65, 61, 53, 43, 37
};
static int out350000_set[10] = {
    2980, 2620, 2140, 1820, 1460, 1300, 1220, 1060, 860, 740
};
static char input[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMN"
		      "OPQRSTUVWXYZ0123456789abcdefghijklmnopqr"
		      "stuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012345"
		      "67890abcdefghijklmnopqrstuvwxyzABCDEFGHI"
		      "JKLMNOPQRSTUVWXYZ0123456789abcdefghijklm";
static char turned_input[] = "afkpuzEJOTY38dinsxCHMRW16afkpuzEJOTY38di"
			     "bglqvAFKPUZ49ejotyDINSX27bglqvAFKPUZ49ej"
			     "chmrwBGLQV05afkpuzEJOTY38chmrwBGLQV05afk"
			     "dinsxCHMRW16bglqvAFKPUZ49dinsxCHMRW16bgl"
			     "ejotyDINSX27chmrwBGLQV050ejotyDINSX27chm";
static u_int32_t *output;	/* lavarnd output */
static int output_len;		/* length of output */
static int output_test[] = {	/* what lavarnd should output without salt */
    0x40289614, 0x0bc61dfc, 0xdd9c9a71, 0x75bcac7a, 0xdcc624f3,
    0x5c7c01ae, 0x666268a3, 0x8fe53256, 0xd531dde0, 0xaec6b332,
    0xd0af8a07, 0x0de7c6ce, 0xe0a53eba, 0x8955d934, 0xf1c13793,
    0xad137b94, 0xd8f90cbd, 0x49cadeda, 0xac203edd, 0x951bc8b8,
    0x40841902, 0xa0ee5eea, 0x33bc7539, 0xc8fb3a07, 0xa780d4e4
};

#define MAX_TRIAL 10	/* maximum number of times to try salted lavarnd */


int
main(int argc, char *argv[])
{
    extern char *optarg;	/* option argument */
    extern int optind;		/* argv index of the next arg */
    void *turn_ret;		/* return of turn */
    int trial;			/* lavarnd salting trial */
    int i;

    /*
     * parse args
     */
    program = argv[0];
    while ((i = getopt(argc, argv, "v:")) != -1) {
	switch (i) {
	case 'v':
	    dbg_lvl = atoi(optarg);
	    break;
	default:
	    fatal(1, "usage: %s [-v debug_level]", program);
	    /*NOTREACHED*/
	    break;
	}
    }
    argv += optind;
    argc -= optind;
    if (argc != 0) {
	fatal(2, "extra args found, usage: %s [-v debug_level]", program);
	/*NOTREACHED*/
    }
    dbg(1, "debug level: %d", dbg_lvl);

    /*
     * turn the alpha 4-way
     */
    dbg(1, "turn the alpha 4-way");
    turn_len = lavarnd_turn_len(sizeof(alpha)-1, 4);
    dbg(2, "4-way turn length of alpha: %d", turn_len);
    if (turn_len != 28) {
	fatal(3, "4-way lava_turn length: %d != 28", turn_len);
	/*NOTREACHED*/
    }
    turn = x_malloc(turn_len);
    memset(turn, '!', turn_len);
    turn_ret = lava_turn(alpha, sizeof(alpha)-1, 4, turn);
    if (dbg_lvl >= 3) {
	for (i=0; i < turn_len; ++i) {
	    dbg(3, "4-way turned[%d]: %c (%02x)",
		i, (char)turn[i], (int)turn[i]);
	}
    }
    if (turn != turn_ret) {
	fatal(4, "4-way lava_turn returned a different buffer");
	/*NOTREACHED*/
    }
    if (memcmp(turn, "aeimquy"
		     "bfjnrvz"
		     "cgkosw\0"
		     "dhlptx\0", turn_len) != 0) {
	fatal(5, "bad 4-way alpha turn");
	/*NOTREACHED*/
    }
    memset(turn, '@', turn_len);
    x_free(turn);

    /*
     * turn the alpha 5-way
     */
    dbg(1, "turn the alpha 5-way");
    turn_len = lavarnd_turn_len(sizeof(alpha)-1, 5);
    dbg(2, "5-way turn length of alpha: %d", turn_len);
    if (turn_len != 30) {
	fatal(6, "5-way lava_turn length: %d != 30", turn_len);
	/*NOTREACHED*/
    }
    turn = x_malloc(turn_len);
    memset(turn, '!', turn_len);
    turn_ret = lava_turn(alpha, sizeof(alpha)-1, 5, turn);
    if (dbg_lvl >= 3) {
	for (i=0; i < turn_len; ++i) {
	    dbg(3, "5-way turned[%d]: %c (%02x)",
		i, (char)turn[i], (int)turn[i]);
	}
    }
    if (turn != turn_ret) {
	fatal(7, "5-way lava_turn returned a different buffer");
	/*NOTREACHED*/
    }
    if (memcmp(turn, "afkpuz"
		     "bglqv\0"
		     "chmrw\0"
		     "dinsx\0"
		     "ejoty\0", turn_len) != 0) {
	fatal(8, "bad 5-way alpha turn");
	/*NOTREACHED*/
    }
    memset(turn, '@', turn_len);
    x_free(turn);

    /*
     * blk turn the alpha 4-way
     */
    dbg(1, "blk turn the alpha 4-way");
    turn_len = lavarnd_blk_turn_len(sizeof(alpha)-1, 4);
    dbg(2, "4-way blk turn length of alpha: %d", turn_len);
    if (turn_len != 80) {
	fatal(9, "4-way blk_lava_turn length: %d != 80", turn_len);
	/*NOTREACHED*/
    }
    turn = x_malloc(turn_len);
    memset(turn, '!', turn_len);
    turn_ret = lava_blk_turn(alpha, sizeof(alpha)-1, 4, turn);
    if (dbg_lvl >= 3) {
	for (i=0; i < turn_len; ++i) {
	    dbg(3, "4-way blk turned[%d]: %c (%02x)",
		i, (char)turn[i], (int)turn[i]);
	}
    }
    if (turn != turn_ret) {
	fatal(10, "4-way lava_turn blk returned a different buffer");
	/*NOTREACHED*/
    }
    if (memcmp(turn, "aeimquy\0\0\0\0\0\0\0\0\0\0\0\0\0"
		     "bfjnrvz\0\0\0\0\0\0\0\0\0\0\0\0\0"
		     "cgkosw\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		     "dhlptx\0\0\0\0\0\0\0\0\0\0\0\0\0\0", turn_len) != 0) {
	fatal(11, "bad 4-way blk alpha turn");
	/*NOTREACHED*/
    }
    memset(turn, '@', turn_len);
    x_free(turn);

    /*
     * blk turn the alpha 5-way
     */
    dbg(1, "blk turn the alpha 5-way");
    turn_len = lavarnd_blk_turn_len(sizeof(alpha)-1, 5);
    dbg(2, "5-way blk turn length of alpha: %d", turn_len);
    if (turn_len != 100) {
	fatal(12, "5-way blk_lava_turn length: %d != 100", turn_len);
	/*NOTREACHED*/
    }
    turn = x_malloc(turn_len);
    memset(turn, '!', turn_len);
    turn_ret = lava_blk_turn(alpha, sizeof(alpha)-1, 5, turn);
    if (dbg_lvl >= 3) {
	for (i=0; i < turn_len; ++i) {
	    dbg(3, "5-way blk turned[%d]: %c (%02x)",
		i, (char)turn[i], (int)turn[i]);
	}
    }
    if (turn != turn_ret) {
	fatal(13, "5-way lava_turn blk returned a different buffer");
	/*NOTREACHED*/
    }
    if (memcmp(turn, "afkpuz\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		     "bglqv\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		     "chmrw\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		     "dinsx\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		     "ejoty\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", turn_len) != 0) {
	fatal(14, "bad 5-way blk alpha turn");
	/*NOTREACHED*/
    }
    memset(turn, '@', turn_len);
    x_free(turn);

    /*
     * salt blk turn the alpha 4-way
     */
    dbg(1, "salt blk turn the alpha 4-way");
    turn_len = lavarnd_salt_blk_turn_len(sizeof(cap_alpha)-1,
	    				 sizeof(alpha)-1, 4);
    dbg(2, "4-way salt blk turn length of alpha: %d", turn_len);
    if (turn_len != 80) {
	fatal(15, "4-way salt blk_lava_turn length: %d != 80", turn_len);
	/*NOTREACHED*/
    }
    turn = x_malloc(turn_len);
    memset(turn, '!', turn_len);
    turn_ret = lava_salt_blk_turn(cap_alpha, sizeof(cap_alpha)-1,
	    			  alpha, sizeof(alpha)-1, 4, turn);
    if (dbg_lvl >= 3) {
	for (i=0; i < turn_len; ++i) {
	    dbg(3, "4-way salt blk turned[%d]: %c (%02x)",
		i, (char)turn[i], (int)turn[i]);
	}
    }
    if (turn != turn_ret) {
	fatal(16, "4-way lava_turn salt blk returned a different buffer");
	/*NOTREACHED*/
    }
    if (memcmp(turn, "AEIMQUYaeimquy\0\0\0\0\0\0"
		     "BFJNRVZbfjnrvz\0\0\0\0\0\0"
		     "CGKOSW\0cgkosw\0\0\0\0\0\0\0"
		     "DHLPTX\0dhlptx\0\0\0\0\0\0\0", turn_len) != 0) {
	fatal(17, "bad 4-way salt blk alpha turn");
	/*NOTREACHED*/
    }
    memset(turn, '@', turn_len);
    x_free(turn);

    /*
     * salt blk turn the alpha 5-way
     */
    dbg(1, "salt blk turn the alpha 5-way");
    turn_len = lavarnd_salt_blk_turn_len(sizeof(cap_alpha)-1,
	    				 sizeof(alpha)-1, 5);
    dbg(2, "5-way salt blk turn length of alpha: %d", turn_len);
    if (turn_len != 100) {
	fatal(18, "5-way salt blk_lava_turn length: %d != 100", turn_len);
	/*NOTREACHED*/
    }
    turn = x_malloc(turn_len);
    memset(turn, '!', turn_len);
    turn_ret = lava_salt_blk_turn(cap_alpha, sizeof(cap_alpha)-1,
	    			  alpha, sizeof(alpha)-1, 5, turn);
    if (dbg_lvl >= 3) {
	for (i=0; i < turn_len; ++i) {
	    dbg(3, "5-way salt blk turned[%d]: %c (%02x)",
		i, (char)turn[i], (int)turn[i]);
	}
    }
    if (turn != turn_ret) {
	fatal(19, "5-way lava_turn salt blk returned a different buffer");
	/*NOTREACHED*/
    }
    if (memcmp(turn, "AFKPUZafkpuz\0\0\0\0\0\0\0\0"
		     "BGLQV\0bglqv\0\0\0\0\0\0\0\0\0"
		     "CHMRW\0chmrw\0\0\0\0\0\0\0\0\0"
		     "DINSX\0dinsx\0\0\0\0\0\0\0\0\0"
		     "EJOTY\0ejoty\0\0\0\0\0\0\0\0\0", turn_len) != 0) {
	fatal(20, "bad 5-way salt blk alpha turn");
	/*NOTREACHED*/
    }
    memset(turn, '@', turn_len);
    x_free(turn);

    /*
     * look at computed nway values
     */
    dbg(1, "look at computed nway values");
    for (i=0; i < sizeof(rate_set)/sizeof(rate_set[0]); ++i) {
	int nway;	/* computed nway value */

	nway = lava_nway_value(350000, rate_set[i]);
	dbg(3, "size: 350000  rate: %f  nway: %d", rate_set[i], nway);
	if (nway != nway350000_set[i]) {
	    fatal(21, "size: 350000  rate: %f  nway: %d != %d",
		  rate_set[i], nway, nway350000_set[i]);
	}
    }

    /*
     * look at the computed lavarnd length
     */
    dbg(1, "look at the computed lavarnd length");
    for (i=0; i < sizeof(rate_set)/sizeof(rate_set[0]); ++i) {
	int len;	/* computed lavarnd value */

	len = lavarnd_len(350000, rate_set[i]);
	dbg(3, "size: 350000  rate: %f  len: %d", rate_set[i], len);
	if (len != out350000_set[i]) {
	    fatal(22, "size: 350000  rate: %f  len: %d != %d",
		  rate_set[i], len, out350000_set[i]);
	}
    }

    /*
     * test lavarand without salting
     */
    dbg(1, "test lavarand");
    output_len = lavarnd_len(sizeof(input)-1, 2.0);
    dbg(2, "lavarnd_len: %d", output_len);
    output = x_malloc(output_len);
    memset(output, '!', output_len);
    /**/
    turn_len = lavarnd_blk_turn_len(sizeof(input)-1, 4);
    turn = x_malloc(turn_len);
    memset(turn, '!', output_len);
    turn_ret = lava_blk_turn(input, sizeof(input)-1, 5, turn);
    dbg(3, "input turned: %100s", turn);
    if (strcmp(turned_input, turn) != 0) {
	fatal(23, "lavarnd turn returned the wrong string");
	/*NOTREACHED*/
    }
    /**/
    i = lavarnd(0, input, sizeof(input)-1, 2.0, output, output_len);
    dbg(2, "lavarnd returns: %d", i);
    if (i != 100) {
	fatal(24, "lavarnd returned: %d != 100", i);
	/*NOTREACHED*/
    }
    for (i=0; i < output_len/sizeof(u_int32_t); ++i) {
	dbg(3, "output[%d]: %02x%02x%02x%02x == 0x%08x",
		i,
		output[i] & 0xff,
		(output[i] >> 8) & 0xff,
		(output[i] >> 16) & 0xff,
		(output[i] >> 24) & 0xff,
		output[i]);
    }
    for (i=0; i < output_len/sizeof(u_int32_t); ++i) {
	if (output[i] != output_test[i]) {
	    fatal(25, "lavarnd word %d output %08x != %08x",
		      i, output[i], output_test[i]);
	    /*NOTREACHED*/
	}
    }

    /*
     * verify that salting changes lavarnd output
     */
    dbg(1, "verify that salting changes lavarnd output");
    output_len = lavarnd_len(sizeof(input)-1, 2.0);
    dbg(2, "salted lavarnd_len: %d", output_len);
    output = x_malloc(output_len);
    memset(output, '!', output_len);
    /**/
    turn_len = lavarnd_blk_turn_len(sizeof(input)-1, 4);
    turn = x_malloc(turn_len);
    memset(turn, '!', output_len);
    turn_ret = lava_blk_turn(input, sizeof(input)-1, 5, turn);
    dbg(3, "salted input turned: %100s", turn);
    /**/
    for (trial=0; trial < MAX_TRIAL; ++trial) {
	dbg(1, "salting lavarnd trail #%d", trial);
	i = lavarnd(1, input, sizeof(input)-1, 2.0, output, output_len);
	dbg(2, "salted lavarnd returns: %d", i);
	if (i != 100) {
	    fatal(26, "salted lavarnd returned: %d != 100", i);
	    /*NOTREACHED*/
	}
	for (i=0; i < output_len/sizeof(u_int32_t); ++i) {
	    dbg(3, "salted output[%d]: %02x%02x%02x%02x = 0x%08x",
		    i,
		    output[i] & 0xff,
		    (output[i] >> 8) & 0xff,
		    (output[i] >> 16) & 0xff,
		    (output[i] >> 24) & 0xff,
		    output[i]);
	}
	/*
	 * NOTE: Sometimes the test below will fail even though the lavarnd
	 * 	 output is correct.  The odds of lavarnd output incorrectly
	 * 	 being declared a failure is 1 in 2^32 (1 in 4294967296).
	 */
	for (i=0; i < output_len/sizeof(u_int32_t); ++i) {
	    if (output[i] == output_test[i]) {
		dbg(0, "salted lavarnd word %d output %08x still same as %08x",
		       i, output[i], output_test[i]);
		break;
	    }
	}
	if (i < output_len/sizeof(u_int32_t)) {
	    continue;
	}
	/*
	 * NOTE: Sometimes the test below will fail even though the lavarnd
	 *	 output is correct.  The odds of lavarnd output incorrectly
	 *	 being declared a failure is 20 in 2^32 (1 in 214748364.8)
	 */
	for (i=0; i < output_len/sizeof(u_int32_t)-1; ++i) {
	    int j;

	    for (j=i+1; j < output_len/sizeof(u_int32_t); ++j) {
		if (output[i] == output[j]) {
		    dbg(0,
			"salted lavarnd word %d output %08x same as word %d",
			    i, output[i], j);
		    break;
		}
	    }
	    if (j < output_len/sizeof(u_int32_t)) {
		break;
	    }
	}
	if (i < output_len/sizeof(u_int32_t)-1) {
	    continue;
	}
	/* test must have worked */
	break;
    }
    if (trial != 0) {
	fatal(27, "salted lavarnd failed %d trials!\n", MAX_TRIAL);
	/*NOTREACHED*/
    }

    /*
     * all is OK if we reached here
     */
    lavarnd_cleanup();
    dbg(1, "all tests are OK");
    return 0;
}


/*
 * x_malloc - allocate memory or exit
 *
 * given:
 * 	len	length of memory to allocate
 *
 * returns:
 * 	allocated storage (or exits on error)
 */
static void *
x_malloc(int len)
{
    void *ret;		/* allocated storage or NULL ==> error */

    /*
     * firewall
     */
    if (len <= 0) {
	fatal(100, "x_malloc was given arg: %d <= 0", len);
	/*NOTREACHED*/
    }

    /*
     * allocate
     */
    ret = malloc(len);
    if (ret == NULL) {
	fatal(101, "malloc of %d octets failed", len);
	/*NOTREACHED*/
    }

    /*
     * return new storage
     */
    return ret;
}


/*
 * x_free - free allocated memory or exit
 *
 * given:
 * 	buf	allocated memory to free
 */
static void
x_free(void *buf)
{
    if (buf == NULL) {
	fatal(102, "attempt to free NULL ptr");
	/*NOTREACHED*/
    }
    free(buf);
}


/*
 * dbg - print a debug message if the level is high enough
 *
 * given:
 *	level	print a message of the -v level is >= this value
 *	fmt	printf format of message
 */
static void
dbg(int level, char *fmt, ...)
{
    va_list ap;		/* argument pointer */

    /* do nothing of the level is too low */
    if (level > dbg_lvl) {
	return;
    }

    /* firewall - only report up to a 9 digit (with sign) level */
    if (level > 999999999) {
	level = 999999999;
    } else if (level < -99999999) {
	level = -99999999;
    }

    /* start the var arg setup and fetch our first arg */
    va_start(ap, fmt);

    /* firewall */
    if (fmt == NULL) {
	fmt = "<<NULL format>>";
    }

    /*
     * output header, message and newline
     */
    fprintf(stderr, "%s: dbg[%d]: ", program, level);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
}


/*
 * fatal - print a message and exit
 *
 * given:
 *	code	exit code
 *	fmt	printf format of message
 */
static void
fatal(int code, char *fmt, ...)
{
    va_list ap;		/* argument pointer */

    /* start the var arg setup and fetch our first arg */
    va_start(ap, fmt);

    /* firewall */
    if (fmt == NULL) {
	fmt = "<<NULL format>>";
    }

    /*
     * output header, message and newline
     */
    fprintf(stderr, "%s: !!!FATAL!!!: ", program);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);

    /* exit */
    exit(code);
}
