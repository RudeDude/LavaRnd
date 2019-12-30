/*
 * poolout - output data from the lavapool daemon
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: poolout.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <unistd.h>
#include <errno.h>

#include "LavaRnd/random.h"
#include "LavaRnd/random_libc.h"
#include "LavaRnd/lava_debug.h"
#include "LavaRnd/cleanup.h"

char *program;	/* our name */


int
main(int argc, char *argv[])
{
#if defined(LAVA_DEBUG)
    char *debug_env = NULL;	/* $LAVA_DEBUG */
#endif /* LAVA_DEBUG */
    u_int8_t *buf;	/* random output buffer */
    long len;	/* length of buf / amount of data needed */
    long cycle_cnt;	/* total number of cycles */
    long cycle;	/* current cycle number */
    long usec;	/* microsecond pause between cycles */
    long ret;	/* randomcpy return count */
    int verbose;	/* 1 ==> verbose output */
    extern int optind;	/* argv index of the next arg */
    int i;

    /*
     * parse args
     */
    program = argv[0];
    verbose = 0;
    ret = 0;
    while ((i = getopt(argc, argv, "v")) != -1) {
	switch (i) {
	case 'v':
	    verbose = 1;
	    break;
	default:
	    ret = -1;
	    break;
	}
    }
    argc -= optind - 1;
    argv += optind - 1;
    if (argc != 4 || ret != 0) {
	fprintf(stderr, "usage: %s [-v] len cycles usec-pause\n\n", program);
	fprintf(stderr, "\t-v\t\tverbose msgs to stderr\n");
	fprintf(stderr, "\tlen\t\tcycle output length in octets\n");
	fprintf(stderr, "\tcyles\t\ttotal output cycles\n");
	fprintf(stderr, "\tusec-pause\tmicrosecond pause between cycles\n");
	exit(1);
    }
    len = strtol(argv[1], NULL, 0);
    if (len <= 0) {
	fprintf(stderr, "%s: len: %ld must be > 0\n", program, len);
	exit(2);
    }
    cycle_cnt = strtol(argv[2], NULL, 0);
    if (cycle_cnt <= 0) {
	fprintf(stderr, "%s: cycle_cnt: %ld must be > 0\n", program,
		cycle_cnt);
	exit(3);
    }
    usec = strtol(argv[3], NULL, 0);
    if (usec < 0) {
	fprintf(stderr, "%s: usec-pause: %ld must be >= 0\n", program, usec);
	exit(4);
    }
    if (verbose) {
	fprintf(stderr, "len: %ld  cycles: %ld  pause: %.6f sec\n",
		len, cycle_cnt, ((double)usec / 1000000.0));
    }

    /*
     * allocate output buffer
     */
    buf = (u_int8_t *) malloc(len);
    if (buf == NULL) {
	fprintf(stderr, "%s: failed to mallloc %ld octets\n", program, len);
	exit(5);
    }

    /*
     * setup for debugging
     */
#if defined(LAVA_DEBUG)
    ret = lava_init_debug();
    if (ret < 0) {
	fprintf(stderr, "%s: LavaRnd debug failed to initialize: %ld\n",
		program, ret);
	exit(6);
    } else if (ret == 0) {
	debug_env = getenv("LAVA_DEBUG");
	if (debug_env == NULL) {
#  if 0
	    fprintf(stderr, "%s: $LAVA_DEBUG not found\n", program);
#  endif
	} else if (strlen(debug_env) <= 0) {
#  if 0
	    fprintf(stderr, "%s: $LAVA_DEBUG is empty\n", program);
#  endif
	} else {
	    fprintf(stderr,
		    "%s: LavaRnd libs not compiled with -DLAVA_DEBUG\n",
		    program);
	}
    } else {
	debug_env = getenv("LAVA_DEBUG");
	if (debug_env == NULL) {
	    fprintf(stderr,
		    "%s: LavaRnd debug initialized without $LAVA_DEBUG!\n",
		    program);
	    exit(7);
	}
	fprintf(stderr, "debugging enabled: $LAVA_DEBUG is %s\n", debug_env);
    }
#endif /* LAVA_DEBUG */

    /*
     * output cycles
     */
    for (cycle = 0; cycle < cycle_cnt; ++cycle) {

	/*
	 * ask for a chunk of random data
	 */
	if (verbose) {
	    fprintf(stderr, "cycle %ld about to randomcpy %ld octets\n",
		    cycle, len);
	}
	lavarnd_errno = 0;
	ret = randomcpy(buf, len);
	if (ret < 0) {
	    fprintf(stderr,
		    "%s: randomcpy: error return: %ld (%s)\n",
		    program, ret, lava_err_name(ret));
	    --cycle;
	} else if (ret == 0) {
	    fprintf(stderr,
		    "%s: randomcpy: error return: 0 lavarnd_errno:%d (%s)\n",
		    program, lavarnd_errno, lava_err_name(lavarnd_errno));
	    --cycle;
	} else {

	    /*
	     * output random data
	     */
	    if (ret != len) {
		fprintf(stderr,
			"%s: warning: asked for %ld octets, received: %ld\n",
			program, len, ret);
	    }
	    clearerr(stdout);
	    (void)fwrite(buf, sizeof(u_int8_t), ret, stdout);
	    if (feof(stdout)) {
		fprintf(stderr, "%s: EOF in output\n", program);
		exit(8);
	    } else if (ferror(stdout)) {
		fprintf(stderr, "%s: stdout error: %s\n",
			program, strerror(errno));
		exit(9);
	    }
	}

	/*
	 * pause if requested
	 */
	if (usec > 0) {
	    usleep(usec);
	}
    }

    /*
     * cleanup - so things like valgrind will not complain about memory leaks
     */
    free(buf);
    lava_dormant();

    /*
     * all done
     */
    return 0;
}
