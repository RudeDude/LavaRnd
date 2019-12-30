/*
 * lavaop - perform a single lavapool requisition operation
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: lavaop.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <sys/types.h>

#include "LavaRnd/lavaerr.h"
#include "LavaRnd/fetchlava.h"
#include "LavaRnd/rawio.h"
#include "LavaRnd/lava_debug.h"
#include "LavaRnd/cfg.h"
#include "LavaRnd/cleanup.h"


/*
 * static variables
 */
static void usage(int exitcode);
static char *program;	/* our name */


int
main(int argc, char *argv[])
{
    lavaback lavacall;	/* lava callback to use */
    u_int8_t *buf = NULL;	/* data from lavapool daemon */
    int len = 0;	/* request length */
    int datalen;	/* length of returned data */
    char *cfg_file = LAVA_RANDOM_CFG;	/* config file, NULL => use default */
    int ret;	/* operation return code */

    /*
     * parse args
     */
    program = argv[0];
    lavacall = LAVACALL_LAVA_EXIT;
    if (argc >= 3 && strcmp(argv[1], "-r") == 0) {
	cfg_file = argv[2];
	argv += 2;
	argc -= 2;
    }
    switch (argc) {
    case 3:
	lavacall = (lavaback)strtol(argv[2], NULL, 0);
	switch ((int)lavacall) {
	case CASE_LAVACALL_LAVA_EXIT:
	case CASE_LAVACALL_LAVA_RETRY:
	case CASE_LAVACALL_LAVA_RETURN:
	case CASE_LAVACALL_TRY_HIGH:
	case CASE_LAVACALL_TRY_MED:
	case CASE_LAVACALL_TRY_ANY:
	case CASE_LAVACALL_TRYONCE_HIGH:
	case CASE_LAVACALL_TRYONCE_MED:
	case CASE_LAVACALL_TRYONCE_ANY:
	case CASE_LAVACALL_S100_HIGH:
	case CASE_LAVACALL_S100_MED:
	case CASE_LAVACALL_S100_ANY:
	    break;
	default:
	    fprintf(stderr, "%s: unknown callback: %d\n",
		    program, (int)lavacall);
	    usage(1);		/* exit(1) */
	    /*NOTREACHED*/
	}
	/*FALLTHRU*/
    case 2:
	len = strtol(argv[1], NULL, 0);
	if (len <= 0) {
	    fprintf(stderr, "%s: length must be > 0\n", program);
	    exit(2);
	}
	break;
    default:
	usage(3);		/* exit(3) */
	/*NOTREACHED*/
    }

    /*
     * setup debugging
     */
#if defined(LAVA_DEBUG)
    ret = lava_init_debug();
    if (ret < 0) {
	fprintf(stderr, "%s: LavaRnd debug failed to initialize: %d\n",
		program, ret);
	exit(4);
    } else if (ret == 0) {
#  if 0
	fprintf(stderr,
		"%s: $LAVA_DEBUG not found or empty, debugging disabled\n",
		program);
#  endif
    } else {
	char *debug_env = NULL;	/* $LAVA_DEBUG */

	debug_env = getenv("LAVA_DEBUG");
	if (debug_env == NULL) {
	    fprintf(stderr,
		    "%s: LavaRnd debug initialized without $LAVA_DEBUG!\n",
		    program);
	    exit(5);
	}
    }
#endif /* LAVA_DEBUG */

    /*
     * preload the lavapool config file
     *
     * If -r was not specified, then cfg_file is NULL and the default
     * configuration is used.
     */
    ret = preload_cfg(cfg_file);
    if (ret < 0) {
	fprintf(stderr, "%s: lavapool config file load error: %d\n",
		program, ret);
	exit(6);
    }

    /*
     * allocate buffer
     */
    buf = (u_int8_t *) malloc(len + 1);
    if (buf == NULL) {
	fprintf(stderr, "%s: malloc %d octet buffer failed\n", program, len);
	exit(7);
    }
    /* pre-initialize for debugging */
    memset(buf, '~', len);
    buf[len] = '\0';

    /*
     * make a raw lavapool request
     */
    ret = raw_random(buf, len, (lavaback)lavacall, &datalen, TRUE);
    if (ret < 0) {
	fprintf(stderr, "%s: raw_lavaop failed: %d\n", program, ret);
	exit(8);
    }

    /*
     * dump the result
     */
    ret = raw_write(1, buf, datalen, FALSE);
    if (ret < 0) {
	fprintf(stderr, "%s: write of lavapool data failed: %d\n",
		program, ret);
	exit(9);
    } else if (ret != datalen) {
	fprintf(stderr, "%s: partial write: %d out of %d\n",
		program, ret, datalen);
	exit(10);
    }

    /*
     * cleanup - so things like valgrind will not complain about memory leaks
     */
    free(buf);
    lava_dormant();
    exit(0);
}


/*
 * usage - print a usage message and exit
 *
 * given:
 *      exitcode        value to exit with after print usage message
 */
static void
usage(int exitcode)
{
    fprintf(stderr, "usage: %s [-r cfg.random] length [callback]\n", program);
    fprintf(stderr, "\n\tcallback arg must be one of these integers:\n\n");
    fprintf(stderr, "\t%d\tLAVACALL_LAVA_EXIT (def)\n",
	    (int)LAVACALL_LAVA_EXIT);
    fprintf(stderr, "\t%d\tLAVACALL_LAVA_RETRY\n", (int)LAVACALL_LAVA_RETRY);
    fprintf(stderr, "\t%d\tLAVACALL_LAVA_RETURN\n", (int)LAVACALL_LAVA_RETURN);
    fprintf(stderr, "\t%d\tLAVACALL_TRY_HIGH\n", (int)LAVACALL_TRY_HIGH);
    fprintf(stderr, "\t%d\tLAVACALL_TRY_MED\n", (int)LAVACALL_TRY_MED);
    fprintf(stderr, "\t%d\tLAVACALL_TRY_ANY\n", (int)LAVACALL_TRY_ANY);
    fprintf(stderr, "\t%d\tLAVACALL_TRYONCE_HIGH\n",
	    (int)LAVACALL_TRYONCE_HIGH);
    fprintf(stderr, "\t%d\tLAVACALL_TRYONCE_MED\n", (int)LAVACALL_TRYONCE_MED);
    fprintf(stderr, "\t%d\tLAVACALL_TRYONCE_ANY\n", (int)LAVACALL_TRYONCE_ANY);
    fprintf(stderr, "\t%d\tLAVACALL_S100_HIGH\n", (int)LAVACALL_S100_HIGH);
    fprintf(stderr, "\t%d\tLAVACALL_S100_MED\n", (int)LAVACALL_S100_MED);
    fprintf(stderr, "\t%d\tLAVACALL_S100_ANY\n", (int)LAVACALL_S100_ANY);
    exit(exitcode);
}
