/*
 * lavaurl - obtain LavaRnd data from a URL of a chaotic source
 *
 * usage:
 *	(see #define USAGE below)
 *
 * NOTE: Without the url arg, it is assumed that args will be fed on stdin.
 *
 * @(#) $Revision: 10.2 $
 * @(#) $Id: lavaurl.c,v 10.2 2003/11/09 22:37:03 lavarnd Exp $
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

/*
 * IMPORTANT NOTE: The quality of the LavaRnd data is only as good as
 *		   the chaotic quality of the URL contents.
 */


#include <string.h>
#include <sys/types.h>
#include <values.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

#include "LavaRnd/lavaerr.h"
#include "LavaRnd/rawio.h"
#include "LavaRnd/cleanup.h"

#include "LavaRnd/lavarnd.h"
#include "LavaRnd/sha1.h"

#include "lava_retry.h"
#include "dbg.h"
#include "simple_url.h"
#include "pool.h"

#if defined(DMALLOC)
#include <dmalloc.h>
#endif


/*
 * usage
 */
char *program = "";		/* our name */
char *prog = "";		/* basename of our name */

#define USAGE \
  "%s [-h] [-a] [-l logfile] [-S] [-Q] [-N] [-v dbg]\n" \
  "    url [len [rate [minurl]]]\n\n" \
  \
  "    -h           output this usage message\n" \
  "    -a           adjust rate according to internal buffer level\n" \
  "    -l logfile   append debug / error messages to log\n" \
  "    -S           send debug, warnings and error messages to syslog\n" \
  "    -Q           dont send debug, warnings & error messages to stderr\n" \
  "    -N           do not add a system state prefix to URL contents\n" \
  "    -v dbg       debug level\n" \
  "    -t           timing test, use a single URL (not for normal use)\n" \
  \
  "    url          chaotic source URL\n" \
  "    len          output size (0==>no limit, def==>0)\n" \
  "    rate         output rate factor (def: 1.0,<1.0 faster,>1.0 slower)\n" \
  "    minurl       min length of url content (0==>no min, def==>0)\n"

/*
 * global flags
 */
static char *url = NULL;	/* chaotic source URL */
static u_int32_t output_len = 0; /* maximum output size (0==>no limit) */
static double rate = 1.0;	/* output rate factor */
static int32_t minurl = 0;	/* min length of url content (0==>no min) */
static char *revision = "$Revision: 10.2 $";
static char *version = NULL;	/* x.y part of revision */
static int use_sysstuff = TRUE;	/* FALSE ==> dont prepend with system stuff */
static int adj_rate = FALSE;	/* TRUE ==> adjust rate per buffer level */
static int time_test = FALSE;	/* TRUE ==> time trials, single URL */

/*
 * error backoff delay flags
 */
static double min_retry = 0.0;	/* resetting retry timeout to this value */
static double retry_sleep = 0.0;	/* time to delay before retrying URL */

#define MAX_RETRY_DELAY (10.0)	/* max retry error delay in seconds */
#define RETRY_STEP (0.2)   	/* retry error pause increment */
#define MIN_WRITE_WAIT (0.01)	/* min time to wait for a write */
#define FULL_WRITE_ADD (10.0)	/* addition time wait for write when full */
#define URL_TIMEOUT (3.51)	/* base URL fetch timeout */
#define URL_TIMEOUT_GROW (1.5)	/* expand timeout on successive tries */
#define URL_RETRY (3)		/* try to get the URL this many times */
#define URL_PAUSE (0.25)	/* pause time between URL retries */
#define URL_PAUSE_GROW (2.0)	/* pause time between URL retries */

/*
 * internal lava buffer
 */
static char *pool = NULL;	/* internal lava pool buffer */
static int pool_len = 0;	/* amount of data in the lava buffer */
static int max_pool = 0;	/* allocated size of lava */

#define MAX_LAVA (16384*SHA_DIGESTSIZE)	 /* largest allocated size of lava */
#define LOW_POOL (IOSIZE/2)		 /* level below which rate doubles */
#define HIGH_POOL (MAX_LAVA-IOSIZE)	 /* level at/above which rate 1/2's */

/*
 * output related
 */
#define OUT_FD (1)		/* assume output is on stdout (1) */
#define IOSIZE (MAXPOOL_IO)	/* largest I/O performed */

/*
 * static forward declarations
 */
static int parse_args(int argc, char *argv[]);
static void clear_retry(void);
static void retry_pause(void);
static void final_cleanup(int sig);


int
main(int argc, char *argv[])
{
    int remaining;		/* octets left to output */
    u_int64_t written;		/* debugging output count */
    int32_t len = 0;		/* URL content length */
    int new_lava;		/* amount of new data added to pool */
    fd_set wr_set;		/* write select set */
    int select_ret;		/* return value from select */
    struct timeval wr_time;	/* max pause time between requests */
    double wr_timeout;		/* timeout for writes */
    int write_size;		/* amount of data we will try to write */
    int write_ret;		/* write return call */
    struct lava_retry retry;	/* how retry when obtaining URL contents */
    int last_level;		/* previous pool level */
    int url_fetched = FALSE;	/* TRUE ==> fetched URL before */
    int prog_malloced = FALSE;	/* TRUE ==> prog is a malloced string */
    char *p;

    /*
     * pre-setup
     */
    program = argv[0];
    p = strrchr(program, '/');
    if (p == NULL) {
	prog = strdup(program);
	prog_malloced = TRUE;
    } else {
    	prog = strdup(p+1);
	prog_malloced = TRUE;
    }
    if (prog == NULL) {
	prog = "lavaurl";
	prog_malloced = FALSE;
    }
    (void) signal(SIGHUP, final_cleanup);
    (void) signal(SIGINT, final_cleanup);
    (void) signal(SIGQUIT, final_cleanup);
    (void) signal(SIGPIPE, final_cleanup);
    (void) signal(SIGALRM, SIG_IGN);
    /* determine version */
    p = strchr(revision, ' ');
    if (p == NULL) {
	version = "0.0";
	revision = "";
    } else {
	version = strdup(p+1);
	if (p == NULL) {
	    version = "0.1";
	    revision = "";
	} else {
	    p = strchr(version, ' ');
	    if (p != NULL) {
		*p = '\0';
	    }
	}
    }

    /*
     * setup I/O
     */
    close(0);

    /*
     * parse args
     */
    if (parse_args(argc, argv) < 0) {
	fprintf(stderr, USAGE, program);
	exit(2);
    }
    dbg(1, "main", "%s version %s start", program, version);

    /*
     * setup lava pool
     */
    remaining = output_len;
    if (output_len < 1) {
	max_pool = MAX_LAVA;
    } else {
	max_pool = (output_len > MAX_LAVA) ? MAX_LAVA : output_len;
    }
    if (max_pool < IOSIZE) {
	max_pool = IOSIZE;
    }
    pool = (char *)malloc(max_pool);
    if (pool == NULL) {
	fatal(3, "main", "unable to allocated a %d octet lava pool", max_pool);
	/*NOTREACHED*/
    }
    dbg(1, "main", "internal pool can hold %d octets", max_pool);
    pool_len = 0;
    last_level = 0;

    /*
     * setup URL retry parameters
     */
    retry.timeout = URL_TIMEOUT;
    retry.timeout_grow = URL_TIMEOUT_GROW;
    retry.retry = URL_RETRY;
    retry.pause = URL_PAUSE;
    retry.pause_grow = URL_PAUSE_GROW;

    /*
     * output as needed
     */
    written = 0;
    while (output_len == 0 || remaining > 0) {

	/*
	 * obtain more lava if we need it
	 */
	last_level = pool_len;
	if (pool_len+SHA_DIGESTSIZE < max_pool) {

	    double tmprate;	/* pool level adjusted LavaRnd rate */

	    /*
	     * obtain the contents of the URL
	     */
	    if (!time_test || !url_fetched) {
		len = lava_get_url(url, minurl, &retry);
	    }
	    if (len <= 0) {
		dbg(2, "main", "lava_get_url error: %d", len);
		retry_pause();
		continue;
	    }
	    url_fetched = TRUE;

	    /*
	     * determine the LavaRnd output processing rate
	     */
	    tmprate = rate;
	    if (adj_rate) {
		if (pool_len == 0) {
		    tmprate *= 4.0;
		} else if (pool_len < LOW_POOL) {
		    tmprate *= 2.0;
		} else if (pool_len >= HIGH_POOL) {
		    tmprate /= 2.0;
		}
		/* keep rate within [0.0625,16.0] */
		if (tmprate > 16.0) {
		    tmprate = 16.0;
		} else if (tmprate < 0.0625) {
		    tmprate = 0.25;
		}
	    }

	    /*
	     * convert string to LavaRnd data
	     */
	    new_lava = lavarnd(use_sysstuff,
		    	       lava_url_content, (u_int32_t)len, tmprate,
			       pool + pool_len,
			       (u_int32_t) (max_pool - pool_len));
	    if (new_lava < 0) {
		warn("main", "lavarnd error, returned: %d", new_lava);
		retry_pause();
		continue;
	    } else {
		pool_len += new_lava;
		dbg(2, "main",
		    "added %d octets, rate %.3f, pool has %d octets", new_lava,
		    tmprate, pool_len);
	    }
	}

	/*
	 * quickly refill is the pool was previously empty
	 */
	if (pool_len <= 0) {
	    dbg(2, "main", "pool is empty, immediate refill");
	    continue;
	} else if (last_level <= 0) {
	    dbg(2, "main", "pool previous empty, immediate refill");
	    clear_retry();	/* not an error */
	    continue;
	}

	/*
	 * determine how long we are willing to wait before writing
	 */
	FD_ZERO(&wr_set);
	FD_SET(OUT_FD, &wr_set);
	if (pool_len+SHA_DIGESTSIZE < max_pool) {
	    wr_timeout =
	      MIN_WRITE_WAIT + (FULL_WRITE_ADD * pool_len / max_pool);
	    wr_time.tv_sec = double_to_tv_sec(wr_timeout);
	    wr_time.tv_usec = double_to_tv_usec(wr_timeout);
	    dbg(2, "main", "select for up to %.3f seconds", wr_timeout);
	    select_ret = select(OUT_FD+1, NULL, &wr_set, NULL, &wr_time);
	} else {
	    wr_timeout = 0.0;
	    dbg(2, "main", "select for up to forever");
	    select_ret = select(OUT_FD+1, NULL, &wr_set, NULL, NULL);
	}
	if (select_ret <= 0) {
	    dbg(2, "main", "cannot write at the moment");
	    clear_retry();	/* not an error */
	    continue;
	}

	/*
	 * determine how much we can or want to write
	 *
	 * We will not write more than IOSIZE octets.  We must not
	 * write more than what we have in the pool.  If we have
	 * an output length, we will not write more than that.
	 */
	write_size = (pool_len < IOSIZE) ? pool_len : IOSIZE;
	if (output_len > 0 && write_size > remaining) {
	    write_size = remaining;
	}

	/*
	 * try to write what we determined we want without blocking
	 */
	write_ret = nilblock_write(OUT_FD, pool+pool_len-write_size,
					   write_size, 0);
	if (write_ret == LAVAERR_NONBLOCK) {
	    dbg(2, "main", "cannot write without blocking at the moment");
	    clear_retry();	/* not an error */
	    continue;
	}
	if (write_ret < 0) {
	    dbg(2, "main", "write failed, ret: %d", write_ret);
	    continue;
	}
	dbg(2, "main", "wrote %d out of %d octets", write_ret, write_size);
	written += (u_int64_t)write_ret;

	/*
	 * drain the pool
	 */
	pool_len -= write_ret;
	if (output_len > 0) {
	    remaining -= write_ret;
	    dbg(1, "main", "need %d more octets, pool has %d octets",
		remaining, pool_len);
	} else {
	    dbg(1, "main", "total output %lld octets, pool has %d octets",
		written, pool_len);
	}

	/*
	 * successful operation
	 */
	clear_retry();
    }

    /*
     * final cleanup
     */
    final_cleanup(0);
    if (prog_malloced && prog != NULL) {
	free(prog);
    }
    unsetlog();
    lava_url_cleanup();
    lavarnd_cleanup();
    lava_dormant();
    return 0;
}


/*
 * parse_args - parse the command line arguments
 *
 * given:
 *	argc	arg count
 *	argv	array of pointers to strings terminated by a NULL
 *
 * returns:
 *	>= 0 ==> non-flag arg count, -1 ==> parse error or print help message
 *
 * NOTE: This will also setup syslog processing if -S.
 */
static int
parse_args(int argc, char *argv[])
{
    extern char *optarg;	/* option argument */
    extern int optind;		/* argv index of the next arg */
    int use_syslog = 0;			/* 1 => open/use syslog */
    char *logfile = NULL;		/* name of debug / log file */
    int use_stderr = 1;			/* 1 => msgs to stderr */
    int i;

    while ((i = getopt(argc, argv, "ahl:SQNv:t")) != -1) {
	switch (i) {
	case 'h':
	    return -1;
	    break;
	case 'a':
	    adj_rate = TRUE;
	    break;
	case 'l':
	    logfile = optarg;
	    (void) setlog(logfile);
	    break;
	case 'S':
	    use_syslog = 1;
	    break;
	case 'Q':
	    use_stderr = 0;
	    break;
	case 'N':
	    use_sysstuff = FALSE;
	    break;
	case 'v':
	    dbg_lvl = atoi(optarg);
	    break;
	case 't':
	    time_test = TRUE;
	    break;
	default:
	    return -1;
	    break;
	}
    }

    /*
     * setup and check syslog
     */
    if (use_syslog) {
	open_syslog();
    }
    if (!use_stderr) {
	dont_use_stderr();
    	fclose(stderr);	/* no messages to stderr */
    }

    /*
     * check args
     */
    argv += optind;
    argc -= optind;
    for (i=0; i < argc; ++i) {
	dbg(2, "parse_args", "argv[%d]: %s", i, argv[i]);
    }
    dbg(2, "main", "non-flag arg count: %d", argc);
    switch (argc) {
    case 4:
	minurl = strtol(argv[3], NULL, 0);
	/*FALLTHRU*/
    case 3:
	rate = atof(argv[2]);
	/*FALLTHRU*/
    case 2:
	output_len = strtoul(argv[1], NULL, 0);
	/*FALLTHRU*/
    case 1:
	url = argv[0];
	break;
    default:
	return -1;
    }
    if (rate < 0.0625 || rate > 16.0) {
	warn("parse_args", "rate: %.3f must be between 0.0625 and 16.0", rate);
	return -1;
    }
    if (minurl < 0) {
	warn("parse_args", "minrul: %d must be >= 0", minurl);
	return -1;
    }
    return argc;
}


/*
 * clear_retry - clear the retry usec clock
 *
 * We call this function to reset the retry timeout to the minimum value.
 */
static void
clear_retry(void)
{
    /*
     * set the retry sleep to the minimum
     */
    retry_sleep = min_retry;

    /*
     * drop the minimum next time of > 0
     */
    min_retry -= RETRY_STEP;
    if (min_retry < 0.0) {
	min_retry = 0.0;
    }
    dbg(4, "clear_retry", "reset pause to %.3f sec", retry_sleep);
}


/*
 * retry_pause - pause for a period of time and then increase retry time
 */
static void
retry_pause(void)
{
    /*
     * sleep after we encountered a problem
     */
    dbg(2, "retry_pause", "pause %.3f sec", retry_sleep);
    if (retry_sleep > 0.0) {
	lava_sleep(retry_sleep);
    }

    /*
     * sleep longer next time, maybe
     */
    retry_sleep += RETRY_STEP;

    /*
     * if beyond max, bring up the minimum use that as the new pause time
     */
    if (retry_sleep > MAX_RETRY_DELAY) {
	min_retry += RETRY_STEP;
	if (min_retry > MAX_RETRY_DELAY) {
	    min_retry = MAX_RETRY_DELAY;
	}
	dbg(5, "retry_pause", "min pause is now %.3f sec", min_retry);
	retry_sleep = min_retry;
    }
    dbg(3, "retry_pause", "next pause will be %.3f sec", retry_sleep);
    return;
}


/*
 * final_cleanup - cleanup and exit
 *
 * given:
 *	sig	>0 ==> signal caught, need top exit, 0 ==> cleanup & return
 */
static void
final_cleanup(int sig)
{
    /*
     * announce
     */
    dbg(1, "final_cleanup", "arg: %d", sig);

    /*
     * lavaurl cleanup
     */
    if (pool != NULL) {
	pool_len = 0;
	free(pool);
	max_pool = 0;
    }
    if (revision != NULL && revision[0] != '0') {
	free(version);
    }

    /*
     * all done
     */
    if (sig != 0) {
	exit(1);
    }
}
