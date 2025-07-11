/*
 * lavaop_i - perform a lavapool requisition operation ops interactively
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: lavaop_i.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "LavaRnd/lavaerr.h"
#include "LavaRnd/fetchlava.h"
#include "LavaRnd/rawio.h"
#include "LavaRnd/lava_debug.h"
#include "LavaRnd/cfg.h"
#include "LavaRnd/cleanup.h"


/*
 * static variables
 */
static char *program;	/* our name */

static int process_request(lavaback lavacall, int len, int out_fd);
static void print_callbacks(void);	/* bad callback */
static void usage(int exitcode);


int
main(int argc, char *argv[])
{
    int callback = 0;	/* lavacall as an int given on command line */
    int len = 0;	/* request length */
    char *cfg_file = LAVA_RANDOM_CFG;	/* config file, NULL => use default */
    char *out_file = NULL;	/* random output filename */
    int out_fd = -1;	/* out_file file descriptor */
    long long total_len = 0;	/* total amount of data written to out_file */
    int ret = 0;	/* operation return code */

    /*
     * parse args
     */
    program = argv[0];
    if (argc >= 3 && strcmp(argv[1], "-r") == 0) {
	cfg_file = argv[2];
	argv += 2;
	argc -= 2;
    }
    if (argc == 2) {
	out_file = argv[1];
    } else {
	usage(1);		/* exit(1) */
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
	exit(2);
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
	    exit(3);
	}
	printf("debugging enabled: $LAVA_DEBUG is %s\n\n", debug_env);
	fflush(stdout);
    }
#else /* LAVA_DEBUG */
    printf("debugging disabled, compiled without -DLAVA_DEBUG\n");
    fflush(stdout);
#endif /* LAVA_DEBUG */

    /*
     * open output file
     */
    errno = 0;
    out_fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC,
		  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (out_fd < 0) {
	fprintf(stderr, "%s: cannot open output file: %s: %s\n",
		program, out_file, strerror(errno));
	exit(4);
    }

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
	exit(5);
    }

    /*
     * request loop
     */
    printf("Use callback of 0 to list callbacks\n");
    printf("Anything other than 2 integers will cause an exit\n\n");
    do {

	/*
	 * prompt
	 */
	printf("callback len: ");
	fflush(stdout);
	ret = fscanf(stdin, "%d %d", &callback, &len);
	if (ret == EOF || ret != 2) {
	    break;
	}
	fputc('\n', stdout);
	fflush(stdout);

	/*
	 * parse prompt
	 */
	if (callback < 0) {
	    break;
	}
	if (len <= 0 && callback != 0) {
	    fprintf(stderr, "%s: len: %d must be > 0\n\n", program, len);
	    fflush(stderr);
	    continue;
	}

	/*
	 * take action based on callback
	 */
	switch (callback) {
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
	    ret = process_request((lavaback)callback, len, out_fd);
	    if (ret < 0) {
		fprintf(stderr, "%s: request error: %d\n", program, ret);
		fflush(stderr);
	    } else {
		total_len += (long long)ret;
		printf("total data written: %lld\n", total_len);
	    }
	    break;
	case 0:
	    print_callbacks();
	    break;
	default:
	    fprintf(stderr, "%s: unknown callback: %d\n", program, callback);
	    print_callbacks();
	    break;
	}

	/*
	 * prep for next prompt
	 */
	fputc('\n', stdout);
	fflush(stdout);

    } while (callback >= 0);

    /*
     * all done
     */
    close(out_fd);

    /*
     * cleanup - so things like valgrind will not complain about memory leaks
     */
    lava_dormant();
    exit(0);
}


/*
 * process_request - process a LavaRnd request
 *
 * given:
 *      lavacall        lava callback to use
 *      len             amount of data to write
 *      out_fd          open file descriptor to write random data
 *
 * return:
 *      amount of random data or <=0 ==> error
 */
static int
process_request(lavaback lavacall, int len, int out_fd)
{
    u_int8_t *buf = NULL;	/* data from lavapool daemon */
    int datalen;	/* actual amount of random data obtained */
    int ret;	/* raw_write() return, amount written or err */

    /*
     * allocate random data buffer
     */
    buf = (u_int8_t *) malloc(len + 1);
    if (buf == NULL) {
	fprintf(stderr, "%s: malloc %d octet buffer failed\n", program, len);
	LAVA_DEBUG_E("process_request", LAVAERR_MALLOC);
	return LAVAERR_MALLOC;
    }

    /*
     * prefill for debugging purposes
     */
    memset(buf, '~', len);
    buf[len] = '\0';

    /*
     * make a raw lavapool request
     */
    printf("requesting %d octets\n", len);
    ret = raw_random(buf, len, (lavaback)lavacall, &datalen, TRUE);
    if (ret < 0) {
	fprintf(stderr, "%s: raw_lavaop failed: %d\n", program, ret);
	LAVA_DEBUG_E("process_request", ret);
	return ret;
    }
    printf("received %d octets\n", datalen);

    /*
     * dump the result
     */
    printf("writing %d octets\n", datalen);
    ret = raw_write(out_fd, buf, datalen, FALSE);
    if (ret < 0) {
	fprintf(stderr, "%s: write of lavapool data failed: %d\n",
		program, ret);
	LAVA_DEBUG_E("process_request", ret);
	return ret;
    } else if (ret != datalen) {
	fprintf(stderr, "%s: partial write: %d out of %d\n",
		program, ret, datalen);
	LAVA_DEBUG_E("process_request", LAVAERR_PARTIAL);
	return LAVAERR_PARTIAL;
    }

    /*
     * cleanup - so things like valgrind will not complain about memory leaks
     */
    free(buf);
    return datalen;
}


/*
 * print_callbacks - print the values of the valid callbacks
 */
static void
print_callbacks(void)
{
    fprintf(stdout, "\tcallback arg must be one of these integers:\n\n");
    fprintf(stdout, "\t%d\tLAVACALL_LAVA_EXIT (def)\n",
	    (int)LAVACALL_LAVA_EXIT);
    fprintf(stdout, "\t%d\tLAVACALL_LAVA_RETRY\n", (int)LAVACALL_LAVA_RETRY);
    fprintf(stdout, "\t%d\tLAVACALL_LAVA_RETURN\n", (int)LAVACALL_LAVA_RETURN);
    fprintf(stdout, "\t%d\tLAVACALL_TRY_HIGH\n", (int)LAVACALL_TRY_HIGH);
    fprintf(stdout, "\t%d\tLAVACALL_TRY_MED\n", (int)LAVACALL_TRY_MED);
    fprintf(stdout, "\t%d\tLAVACALL_TRY_ANY\n", (int)LAVACALL_TRY_ANY);
    fprintf(stdout, "\t%d\tLAVACALL_TRYONCE_HIGH\n",
	    (int)LAVACALL_TRYONCE_HIGH);
    fprintf(stdout, "\t%d\tLAVACALL_TRYONCE_MED\n", (int)LAVACALL_TRYONCE_MED);
    fprintf(stdout, "\t%d\tLAVACALL_TRYONCE_ANY\n", (int)LAVACALL_TRYONCE_ANY);
    fprintf(stdout, "\t%d\tLAVACALL_S100_HIGH\n", (int)LAVACALL_S100_HIGH);
    fprintf(stdout, "\t%d\tLAVACALL_S100_MED\n", (int)LAVACALL_S100_MED);
    fprintf(stdout, "\t%d\tLAVACALL_S100_ANY\n", (int)LAVACALL_S100_ANY);
    fflush(stdout);
    return;
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
    fprintf(stderr, "usage: %s [-r cfg.random] outfile\n", program);
    exit(exitcode);
}
