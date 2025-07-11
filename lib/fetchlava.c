/*
 * fetchlava - raw interface to obtain data from the lavapool daemon
 *
 * These routines provide the low and middle level interface to
 * the random number services.  Only the middle level interface
 * is accessible externally, however.
 *
 * NOTE: Looking for the high level call interface?   Try random.c and
 *       random_libc.c or perhaps s100.c.
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: fetchlava.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include "LavaRnd/lavaerr.h"
#include "LavaRnd/rawio.h"
#include "LavaRnd/fetchlava.h"
#include "LavaRnd/cfg.h"
#include "LavaRnd/s100.h"
#include "LavaRnd/lava_debug.h"

#if defined(DMALLOC)
#  include <dmalloc.h>
#endif


/*
 * lavarnd_errno - previous LavaRnd error code or 0
 *
 * See the comment in random.h for details about this value.
 *
 * NOTE: This is an external error indicator which the calling
 *       routine is free to clear at any time.  Do not depend
 *       on this value for internal error status.  This is
 *       NOT the daemon_err or s100_seed_err flag!!!!
 */
int lavarnd_errno = LAVAERR_OK;


/*
 * lastop_errno - error code (or 0) of the most recent random or libc-like call
 *
 * See the comment in random.h for details about this value.
 *
 * NOTE: This is an external error indicator which the calling
 *       routine is free to clear at any time.  Do not depend
 *       on this value for internal error status.  This is
 *       NOT the daemon_err or s100_seed_err flag!!!!
 */
int lastop_errno = LAVAERR_OK;


/*
 * timeout - next raw_lavapool() timeout
 */
static double next_timeout = LAVA_DEF_CALLBACK_WAIT;	/* next timeout value */

/*
 * TIME_SLIDE(retry,limit,min,max)
 *
 *      retry   call retry number (0..limit)
 *      limit   maximum number of retries allowed
 *      min     minimum wait (when retry is 0)
 *      max     maximum wait (when retry is limit)
 */
#define TIME_SLIDE(retry,limit,min,max) \
    (((int)(retry) <= 0) ? (double)(min) : \
     ((((int)(retry) >= (int)(limit)) || ((int)(limit) < 2)) ? (double)(max) : \
      (((double)(max) * (double)(retry) / (double)(limit)) + \
       ((double)(min) * (1.0 - ((double)(retry) / (double)(limit))))) \
     ) \
    )

/*
 * random number interface to the lavapool daemon
 */
static int is_cfg = FALSE;	/* TRUE ==> lavapool configured */
static int daemon_err = FALSE;	/* TRUE ==> previous lavapool failure */
static int s100_seed_err = 0;	/* seeding s100 errors up to s100_retries */
static double s100_timeout = LAVA_DEF_S100_MIN_WAIT;	/* s100 seed timeout */
struct cfg_random cfg_random;	/* master interface configuration structure */

#define MAXLINE 1024		/* longest cfg.random line allowed */


/*
 * default random number interface to the lavapool daemon configuration
 */
static struct cfg_random def_cfg = {
    LAVA_DEF_LAVAPOOL,		/* lavapool request port */
    LAVA_DEF_MALLOCED_STR,	/* constant lavapool string */
    LAVA_DEF_MAXREQ,		/* max request size allowed */
    LAVA_DEF_EXIT_RETRIES,	/* def retry connections if exit */
    LAVA_DEF_EXIT_MIN_WAIT,	/* def min retry timeout if exit */
    LAVA_DEF_EXIT_MAX_WAIT,	/* def max retry timeout if exit */
    LAVA_DEF_RETRY_MIN_WAIT,	/* def min retry timeout if retry */
    LAVA_DEF_RETRY_INC_WAIT,	/* def timeout increment if retry */
    LAVA_DEF_RETRY_MIN_WAIT,	/* def max retry timeout if retry */
    LAVA_DEF_RETURN_RETRIES,	/* def retry connections if return */
    LAVA_DEF_RETURN_MIN_WAIT,	/* def min retry timeout if return */
    LAVA_DEF_RETURN_MAX_WAIT,	/* def max retry timeout if return */
    LAVA_DEF_S100_RETRIES,	/* def retry connections if seeding s100 */
    LAVA_DEF_S100_PRESEED_AMT,	/* def amt before seeding s100 fallback */
    LAVA_DEF_S100_MIN_WAIT,	/* def min retry timeout if seeding s100 */
    LAVA_DEF_S100_MAX_WAIT,	/* def max retry timeout if seeding s100 */
    LAVA_DEF_CALLBACK_WAIT,	/* def initial timeout if callback */
    LAVA_DEF_PRELOAD		/* def time to wait during lava_preload() */
};


/*
 * static declarations
 */
static int lava_maxlen(lavaback callback);
static void preseed_s100(lavaback callback, int len);
static int parse_lavapool(char *filename, struct cfg_random *config);
static int config_lavapool(char *cfg_file, struct cfg_random *config);
static int def_lavapool(struct cfg_random *config);
static struct cfg_random *dup_lavapool_cfg(struct cfg_random *cfg1,
					   struct cfg_random *cfg2);
static void free_cfg_random(struct cfg_random *config);


/*
 * font loaded buffer
 *
 * This is a buffer where the available data is always at the front.  I.e.,
 *
 *      +--- start
 *      V
 *      |---available data---|---free space---|
 *      <------- avail ------>
 *      <---------------- size --------------->
 */
struct s_frontbuf {
    u_int8_t *start;	/* start of buffer or NULL ==> no buffer */
    int avail;	/* amount of available data at front of buffer */
    int size;	/* allocated size or intended size if start == NULL */
};
typedef struct s_frontbuf frontbuf;

/*
 * buffered LavaRnd data
 */
#define MIN_BUF_SIZE (64)	/* smallest buffer size in octets */
#define MAX_BUF_SIZE (4*LAVA_MAX_LEN_ARG)	/* largest buffer size in octets */
#define BUF_GROW (2)		/* default size growth factor */
static frontbuf lavabuf = {	/* LavaRnd buffer */
    NULL, 0, MIN_BUF_SIZE
};


/*
 * private subtractive 100 shuffle state - private s100 generator
 *
 * While users can their own s100 states, we use a single state so that
 * we can control the seed process.  The lavapool / s100 interface is
 * not an interface where one can reseed and repeat the output.
 */
static s100shuf s100 = {
    0,				/* not seeded */
    0,				/* need to seed now */
    0,				/* no seed length */
    0,				/* no octets in output buffer */
    LAVA_QUAL_NONE,		/* no initial quality */
    NULL,			/* no initialized output buffer */
};
static int32_t octets_to_preseed = LAVA_DEF_S100_PRESEED_AMT;


/*
 * LavaRnd subtractive 100 shuffle seed buffer
 *
 * This load buffer is the same size as the s100 generator seed.
 */
static u_int8_t lava_s100_seed[sizeof(struct s100_seed)];


/*
 * preload_cfg - preload a cfg.random config file
 *
 * given:
 *      cfg_file        path to cfg.random (or NULL for default)
 *
 * returns:
 *      0 - all of OK, *config is the new configuration
 *      <0 - error, LavaRnd library system left in an unfigured state
 *
 *           The only reasons for returning an error here is if we
 *           fail to load the default configuation (perhaps out of memory?),
 *           or the cfg_file exists and has a syntax error.
 *
 * NOTE: Within this file, callers to this function check is_cfg to
 *       determine if a call to this function is needed.
 *
 * NOTE: If the cfg_file does not exist, then the internal default
 *       configuation is used and 0 is returned.
 */
int
preload_cfg(char *cfg_file)
{
    struct stat buf;	/* used to determine if cfg_file exists */
    int ret;	/* return code */

    /* preload the default */
    LAVA_DEBUG_I("preload_cfg", "loading default config", "");
    ret = def_lavapool(&cfg_random);
    if (ret < 0) {
	/* failed to load the default configuation */
	LAVA_DEBUG_E("preload_cfg", ret);
	is_cfg = FALSE;
	return ret;
    }
    LAVA_DEBUG_I("preload_cfg",
		 "default lavapool port: %s", cfg_random.lavapool);

    /* load the config file if requested */
    if (cfg_file != NULL) {

	/* load config file only if it exists */
	LAVA_DEBUG_I("preload_cfg", "about to load config file: %s", cfg_file);
	if (stat(cfg_file, &buf) >= 0) {
	    ret = config_lavapool(cfg_file, &cfg_random);
	    if (ret < 0) {
		LAVA_DEBUG_I("preload_cfg",
			     "failed to load config file: %s", cfg_file);
		is_cfg = FALSE;
	    } else {
		LAVA_DEBUG_I("preload_cfg",
			     "lavapool port: %s", cfg_random.lavapool);
		is_cfg = TRUE;
	    }

	    /* we have already preloaded the default */
	} else {
	    LAVA_DEBUG_I("preload_cfg",
			 "config file not found, keeping default config", "");
	    is_cfg = TRUE;
	}

	/* we loaded default configuration instead parsing a config file */
    } else {
	LAVA_DEBUG_I("preload_cfg",
		     "no specific config, keeping default config", "");
	is_cfg = TRUE;
    }
    return ret;
}

/*
 * load_cfg - preload a cfg.random config file
 *
 * given:
 *      cfg_file        path to cfg.random (or NULL for default)
 *
 * returns:
 *      0 - all of OK, *config is the new configuration
 *      <0 - error, LavaRnd library system left in an unfigured state
 *
 *           The only reasons for returning an error here is if we
 *           fail to load the default configuation (perhaps out of memory?),
 *           or the the cfg_file does not exist,
 *           or the the cfg_file exists and has a syntax error.
 *
 * NOTE: Within this file, callers to this function check is_cfg to
 *       determine if a call to this function is needed.
 *
 * NOTE: Unlinke preload_cfg(), if cfg_file is NON-NULL then it must exist.
 *       This function will not fall back on the default configuration
 *       if the cfg_file turns up missing.
 */
int
load_cfg(char *cfg_file)
{
    int ret;	/* return code */

    /* preload the default */
    LAVA_DEBUG_I("load_cfg", "loading default config", "");
    ret = def_lavapool(&cfg_random);
    if (ret < 0) {
	/* failed to load the default configuation */
	LAVA_DEBUG_E("load_cfg", ret);
	is_cfg = FALSE;
	return ret;
    }
    LAVA_DEBUG_I("load_cfg", "default lavapool port: %s", cfg_random.lavapool);

    /* load the config file if requested */
    if (cfg_file != NULL) {

	/* load config file */
	ret = config_lavapool(cfg_file, &cfg_random);
	if (ret < 0) {
	    LAVA_DEBUG_I("load_cfg",
			 "failed to load config file: %s", cfg_file);
	    is_cfg = FALSE;
	} else {
	    LAVA_DEBUG_I("load_cfg", "lavapool port: %s", cfg_random.lavapool);
	    is_cfg = TRUE;
	}

	/* we loaded default configuration instead parsing a config file */
    } else {
	LAVA_DEBUG_I("load_cfg",
		     "no specific config, keeping default config", "");
	is_cfg = TRUE;
    }
    return ret;
}


/*
 * lava_timeout - set the next lavapool request timeout
 *
 * given:
 *      timeout         set new next_timeout if > 0.0, do not change if <= 0.0
 *
 * returns:
 *      previous next_timeout value
 *
 * NOTE: If timeout is <= 0.0, the timeout value is not set.  The current
 *       timeout value is returned.  Thus lava_timeout(0.0) will return
 *       but not change the lavapool request period.
 */
double
lava_timeout(double timeout)
{
    double prev = next_timeout;	/* previous timeout */

    /*
     * set new timeout
     */
    if (timeout > 0.0) {
	next_timeout = timeout;
    }

    /*
     * return previous timeout
     */
    return prev;
}


/*
 * lava_maxlen - determine the maximum raw_random() length for a given callback
 *
 * given:
 *      callback        callback function or LAVACALL_XYZ
 *
 * returns:
 *      maximum raw_random() length allowed
 */
static int
lava_maxlen(lavaback callback)
{
    int maxlen = LAVA_MAX_LEN_ARG;	/* maximum raw_random() length */

    /*
     * determine the maximum length
     */
    switch ((int)callback) {
    case CASE_LAVACALL_S100_HIGH:
    case CASE_LAVACALL_S100_MED:
    case CASE_LAVACALL_S100_ANY:
	maxlen = 0x7fffffff;
	break;
    }
    return maxlen;
}


/*
 * preseed_s100 - preseed the private s100 generator if needed
 *
 * given:
 *      callback        callback function or LAVACALL_XYZ
 *      len             amount of LavaRnd or s100 data requested
 */
static void
preseed_s100(lavaback callback, int len)
{
    /*
     * quick return we do not need to reseed
     */
    switch ((int)callback) {
    case CASE_LAVACALL_LAVA_EXIT:
    case CASE_LAVACALL_LAVA_RETRY:
    case CASE_LAVACALL_LAVA_RETURN:
    case CASE_LAVACALL_S100_HIGH:
    case CASE_LAVACALL_S100_MED:
    case CASE_LAVACALL_S100_ANY:
	/* private s100 generator not needed/used */
	return;
    }
    /* inline equiv of: if (s100_loadleft(&s100) > 0) { return; } */
    if (s100.seeded && (s100.nextspin > 0 || s100.rndbuf_len > 0)) {
	/* private s100 generator already seeded */
	return;
    }
    if (cfg_random.s100_preseed_amt == -1) {
	/* s100_preseed_amt configured to not preseed */
	return;
    }
    /* case: private s100 generator might be needed/used */

    /*
     * determine if we have output enough data to need a preseed
     *
     * We could also test for len >= 0, however the callers
     * to this function already check for that.  Besides, if
     * someone did call this function with a negative len
     * it would have the interesting effect of delaying the
     * preseed time.  That might be useful someday.
     */
    if (len < octets_to_preseed) {
	/* have not output enough LavaRnd data to need a preseed */
	octets_to_preseed -= len;
	return;
    } else if (octets_to_preseed < 0) {
	/* preseed previously failed, do not try again */
	return;
    }

    /*
     * preseed the s100 generator, if possible
     */
    LAVA_DEBUG_S("preseed_s100", "about to preseed s100: %d",
		 octets_to_preseed);
    (void)lava_preload(-1, TRUE);
    /* inline equiv of: if (s100_loadleft(&s100) > 0) { ... } */
    if (s100.seeded && (s100.nextspin > 0 || s100.rndbuf_len > 0)) {
	octets_to_preseed = 0;
	LAVA_DEBUG_S("preseed_s100", "s100 now preseeded: %d",
		     octets_to_preseed);
    } else {
	/* no need to attempt to preseed again after 1st failure */
	octets_to_preseed = -1;
	LAVA_DEBUG_S("preseed_s100", "failed to preseed s100: %d",
		     octets_to_preseed);
    }
    return;
}


/*
 * def_lava_timeout - set the default next lavapool request timeout
 *
 * given:
 *      callback        callback function or LAVACALL_XYZ
 *
 * returns:
 *      new next_timeout value
 *
 *      Also the s100_timeout timeout is set based on the current
 *      s100_seed_err seed count.
 */
static double
def_lava_timeout(lavaback callback)
{
    double timeout;	/* new default timeout */

    /*
     * determine s100 timeout value
     */
    s100_timeout = TIME_SLIDE(s100_seed_err, cfg_random.s100_retries,
			      cfg_random.s100_min_wait,
			      cfg_random.s100_max_wait);

    /*
     * determine the initial timeout
     */
    switch ((int)callback) {
    case CASE_LAVACALL_LAVA_EXIT:
	timeout = cfg_random.exit_min_wait;
	break;
    case CASE_LAVACALL_LAVA_RETRY:
	timeout = cfg_random.retry_min_wait;
	break;
    case CASE_LAVACALL_LAVA_RETURN:
	timeout = cfg_random.return_min_wait;
	break;
    case CASE_LAVACALL_S100_HIGH:
    case CASE_LAVACALL_S100_MED:
    case CASE_LAVACALL_S100_ANY:
	timeout = s100_timeout;
	break;
    case CASE_LAVACALL_TRY_HIGH:
    case CASE_LAVACALL_TRY_MED:
    case CASE_LAVACALL_TRY_ANY:
	if (cfg_random.retry_min_wait > s100_timeout) {
	    timeout = cfg_random.retry_min_wait;
	} else {
	    timeout = s100_timeout;
	}
	break;
    case CASE_LAVACALL_TRYONCE_HIGH:
    case CASE_LAVACALL_TRYONCE_MED:
    case CASE_LAVACALL_TRYONCE_ANY:
	if (daemon_err) {
	    timeout = s100_timeout;
	} else if (cfg_random.retry_min_wait > s100_timeout) {
	    timeout = cfg_random.retry_min_wait;
	} else {
	    timeout = s100_timeout;
	}
	break;
    default:
	timeout = cfg_random.def_callback_wait;
	break;
    }

    /*
     * set the new timeout
     */
    lava_timeout(timeout);
    return timeout;
}


/*
 * raw_lavaop - perform the I/O for a lavapool request (hidden low-level call)
 *
 * This function performs the socket open, request formation, sending
 * the request, and receiving the reply from a lavapool daemon.
 * It also handles timeout conditions.
 *
 * Unlike raw_lavapool, this function will only perform a single
 * lavapool operation.  It will at most return cfg_random.maxrequest octets
 * regardless of size of len.  It is the caller's responsibility to
 * call this function multiple times if a larger request is needed.
 *
 * given:
 *      port            host:port or /socket/path request port, NULL => default
 *      buf             description of where to place lavapool data
 *      len             request length, which must be <= cfg_random.maxrequest
 *      timeout         request timeout or 0.0 => no timeout
 *
 * returns:
 *      count           octets received or <0 ==> error
 *
 * NOTE: 0 is not returned.
 *
 * NOTE: It is assumed that buf is at least len octets long.
 *
 * NOTE: Looking for the random number interfaces?   Try random.c and
 *       random_libc.c or perhaps s100.c.
 *
 * NOTE: As a side effect, this function will set daemon_err to TRUE or FALSE
 *       depending on if there was an error or not while communicating with
 *       the lavapool daemon.
 *
 * NOTE: Normal user code should not be calling this function.  If LAVA_DEBUG
 *       is defined, this function will be externally callable.  Normal code
 *       should not count on this call being accessible.  The arguments and
 *       side effects of this call may change without notice from time to time.
 *
 * NOTE: This function does NOT touch the lavarnd_errno value.  That value
 *       is controlled by the raw_random() and lava_preload() functions alone.
 *       Errors reported by the raw_lavaop() interface are not recorded
 *       in the lavarnd_errno value!
 */
#if defined(LAVA_DEBUG)
int
#else
static int
#endif
raw_lavaop(char *port, u_int8_t * buf, int len, double timeout)
{
    int fd;	/* open connected socket to lavapool */
    int len_left;	/* octets left to read */
    int outlen;	/* length of string in the out buffer */
    int request;	/* lavapool request size */
    int error;	/* error code to return */
    char out[LAVA_REQBUFLEN + 1];	/* request output buffer */
    int ret;	/* I/O return value */

    /*
     * firewall
     */
    if (port == NULL || buf == NULL || timeout < 0.0) {
	LAVA_DEBUG_E("raw_lavaop", LAVAERR_BADARG);
	return LAVAERR_BADARG;
    }
    if (len <= 0) {
	/* invalid argument */
	LAVA_DEBUG_E("raw_lavaop", LAVAERR_BADLEN);
	lavarnd_errno = LAVAERR_BADLEN;
	lastop_errno = LAVAERR_BADLEN;
	LAVA_DEBUG_L("raw_lavaop");
	return LAVAERR_BADLEN;
    }

    /*
     * determine the request size
     */
    if (len > cfg_random.maxrequest) {
	request = cfg_random.maxrequest;
    } else {
	request = len;
    }
    LAVA_DEBUG_P("raw_lavaop", "lavapool op requesting %d octets", request);

    /*
     * setup for timeout, if needed
     */
    (void)set_simple_alarm(timeout);
    if (lava_ring) {
	/* early timeout or alarm error */
	error = lavaerr_ret(lava_ring);
	(void)clear_simple_alarm();
	daemon_err = TRUE;
	LAVA_DEBUG_E("raw_lavaop", error);
	return error;
    }

    /*
     * open socket to the random daemon
     */
    fd = lava_connect(port);
    if (lava_ring) {
	/* early timeout or alarm error */
	(void)clear_simple_alarm();
	error = lavaerr_ret(lava_ring);
	if (fd >= 0) {
	    (void)close(fd);
	}
	daemon_err = TRUE;
	LAVA_DEBUG_E("raw_lavaop", error);
	return error;
    } else if (fd < 0) {
	/* failed to connect */
	(void)clear_simple_alarm();
	error = LAVAERR_OPENERR;
	daemon_err = TRUE;
	LAVA_DEBUG_E("raw_lavaop", error);
	return error;
    }

    /*
     * format output request
     */
    snprintf(out, LAVA_REQBUFLEN, "%d\n", request);
    out[LAVA_REQBUFLEN] = '\0';
    outlen = strlen(out);

    /*
     * write the request to the random daemon
     */
    for (len_left = outlen, ret = 1; len_left > 0 && ret > 0; len_left -= ret) {

	/*
	 * write some more request data
	 */
	ret = raw_write(fd, out + outlen - len_left, len_left, TRUE);
	if (lava_ring) {
	    /* early timeout or alarm error */
	    (void)clear_simple_alarm();
	    error = lavaerr_ret(lava_ring);
	    (void)close(fd);
	    daemon_err = TRUE;
	    LAVA_DEBUG_E("raw_lavaop", error);
	    return error;
	} else if (ret < 0) {
	    /* write failure */
	    (void)clear_simple_alarm();
	    error = LAVAERR_IOERR;
	    (void)close(fd);
	    daemon_err = TRUE;
	    LAVA_DEBUG_E("raw_lavaop", error);
	    return error;
	} else if (ret == 0) {
	    /* EOF on write */
	    (void)clear_simple_alarm();
	    error = LAVAERR_EOF;
	    (void)close(fd);
	    daemon_err = TRUE;
	    LAVA_DEBUG_E("raw_lavaop", error);
	    return error;
	}
    }

    /*
     * load random daemon data into the lavabuf
     */
    for (len_left = request, ret = 1; len_left > 0 && ret > 0; len_left -= ret) {

	/*
	 * read some more data
	 */
	ret = raw_read(fd, buf + (request - len_left), len_left, TRUE);
	if (lava_ring) {
	    /* early timeout or alarm error */
	    (void)clear_simple_alarm();
	    error = lavaerr_ret(lava_ring);
	    (void)close(fd);
	    daemon_err = TRUE;
	    LAVA_DEBUG_E("raw_lavaop", error);
	    return error;
	} else if (ret < 0) {
	    /* read failure */
	    (void)clear_simple_alarm();
	    error = LAVAERR_IOERR;
	    (void)close(fd);
	    daemon_err = TRUE;
	    LAVA_DEBUG_E("raw_lavaop", error);
	    return error;
	} else if (ret == 0) {
	    /* EOF on read */
	    (void)clear_simple_alarm();
	    error = LAVAERR_EOF;
	    (void)close(fd);
	    daemon_err = TRUE;
	    LAVA_DEBUG_E("raw_lavaop", error);
	    return error;
	}
    }

    /*
     * cleanup
     */
    (void)clear_simple_alarm();
    (void)close(fd);

    /*
     * return result
     */
    LAVA_DEBUG_P("raw_lavaop", "lavapool op returned %d octets", request);
    LAVA_DEBUG_H("raw_lavaop", buf, request);
    return request;
}


/*
 * raw_lavapool - obtain data from a lavapool daemon (hidden low-level call)
 *
 * This function manages a single from the lavapool daemon.  It attempts to
 * buffer requests so as to minimize the overhead in communicating with
 * the lavapool.  It reports timeouts.
 *
 * Here are some of the issues related to buffering:
 *
 *      We speculate that we will mostly see a few types of request behavior.
 *      One is sequence of small requests, mostly of the same size.  The
 *      other is an initial large request followed by a sequence of
 *      requests, either all small or all large.  Some programs will only
 *      make a few requests.  Other programs will make many requests.
 *      At this point we have no data to model which is likely.
 *
 *      We wish to minimize the number of times we have go to the lavapool
 *      to obtain data.  We do this by buffering.  As buffers are exhausted
 *      new larger buffers are formed.  Over time we will make fewer and
 *      fewer requests of larger size.
 *
 *      We don't want to waste lavapool data.  We don't want to initially
 *      fill a huge buffer only to have the user make a few small small
 *      requests and waste the remainder of the huge buffer.
 *
 *      We would like to minimize the amount of memory transfers.  Doing
 *      a free of an empty buffer and then allocating a new larger buffer
 *      is better than a realloc because the realloc would make a useless
 *      copy of unused buffer space.
 *
 *      A single lavapool transfer operation goes into a single buffer.
 *      Our model does not allow us to scatter a single transfer operation
 *      across two different buffers.  We can either load new data into
 *      our buffer or directly into the user's buffer.  Obviously we don't
 *      want to transfer a large chunk of data into our buffer only
 *      to have to them memory copy it into the user's buffer.  On the
 *      other hand we don't want to perform a tiny transfer operation directly
 *      into the user's buffer as the overhead of doing an lavapool operation
 *      for a small transfer is also wasteful.
 *
 *      We do not want to expand our buffers to huge sizes and waste memory.
 *      Our buffers must have a upper size bound.  Also the lavapool protocol
 *      will limit how much data we will transfer in a single operation.
 *
 *      It should be noted that because our data is random, we do NOT have
 *      to give it to the user in any particular order.  We are not reading
 *      data from a file or tape.  We can load the user's request buffer in
 *      any order.
 *
 * Here is our solution:
 *
 *      If the user's request can be completely satisfied by the data we
 *      have in our buffer, then we transfer what is needed to the user's
 *      buffer and return.
 *
 *      If we have some data in our buffer, but not enough to satisfy the
 *      user's request, then we transfer everything we have from our
 *      buffer, FREE our buffer, and then work on completing the remainder
 *      of the user's request.
 *
 *          NOTE: The reason for the free/malloc instead of realloc to
 *                twice our original buffer size is to avoid useless
 *                memory copies.
 *
 *      When working on completing the remainder of the user's request
 *      we compare the size of the unfulfilled portion of the user's
 *      request and the size of what our buffer was before (old buffer
 *      size) it was freed.  If the unfulfilled size was as big or bigger
 *      than or old buffer size, then we perform transfer operation(s)
 *      directly into the user's buffers and record our intent to
 *      make our new buffer twice as big as it was before.  We do
 *      not need allocate this new buffer size during this user request
 *      because the user's request is now satisfied.  However if
 *      our unfulfilled size is smaller than our old buffer size then
 *      we will allocate a new buffer twice the old size, perform transfer
 *      operations into our new buffer until it is full and finally
 *      copy data out of our buffer into the user's buffer.
 *
 *      The final case is where we have a user request and no buffer.
 *      We have no buffer because we have never allocated one before
 *      (this is the first request) or because we freed an old buffer,
 *      doubled the intended size and did not get around to allocating
 *      it because we did a direct transfer to the user's buffer
 *      on the previous request.
 *
 *      For the initial request (never had a buffer before) we set
 *      our intended size to an initial buffer size.  We consider
 *      that the 'previous' buffer size was 1/2 of the initial size.
 *      At this point we compare the unfulfilled portion of the
 *      user's request (which is the entire request) and proceed
 *      as we do above.
 *
 * given:
 *      port            host:port or /socket/path of request port
 *      buf             description of where to place lavapool data
 *                          NULL ==> do not copy data, leave in lavabuf
 *                                   (used to pre-load the lavabuf buffer)
 *      len             max number of octets that the port will return
 *      timeout         request timeout or 0.0 => no timeout
 *
 * returns:
 *      count           octets of data (which will be len) or < 0 on error
 *
 * NOTE: 0 is not returned.
 *
 * NOTE: It is assumed that buf is at least len octets long.
 *
 * NOTE: Looking for the random number interfaces?   Try random.c and
 *       random_libc.c or perhaps s100.c.
 *
 * NOTE: The buf==NULL is intended for use by the lava_preload() function.
 */
static int
raw_lavapool(char *port, u_int8_t * buf, int len, double timeout)
{
    int remainder;	/* remainder of user request to fill */
    int able;	/* data we are able to transfer */
    int oldsize;	/* pre-expanded buffer size */

    /*
     * firewall
     */
    LAVA_DEBUG_S("raw_lavapool", "asking for %d LavaRnd octets", len);
    if (port == NULL || timeout < 0.0) {
	LAVA_DEBUG_E("raw_lavapool", LAVAERR_BADARG);
	return LAVAERR_BADARG;
    }
    if (len <= 0) {
	/* invalid argument */
	LAVA_DEBUG_E("raw_lavapool", LAVAERR_BADLEN);
	lavarnd_errno = LAVAERR_BADLEN;
	lastop_errno = LAVAERR_BADLEN;
	LAVA_DEBUG_L("raw_lavapool");
	return LAVAERR_BADLEN;
    }

    /*
     * try to fulfill the request from our buffer, if we have one
     */
    oldsize = lavabuf.size;
    /* case: we have some buffer data */
    if (buf != NULL && lavabuf.start != NULL && lavabuf.avail > 0) {

	/* copy what we are able, up to the entire user's request */
	able = (lavabuf.avail > len) ? len : lavabuf.avail;
	LAVA_DEBUG_B("raw_lavapool", "copying %d octets from %d in buffer",
		     able, lavabuf.avail);
	lavabuf.avail -= able;
	memcpy(buf, lavabuf.start + lavabuf.avail, able);
	LAVA_DEBUG_B("raw_lavapool",
		     "after %d octet copy, buffer as %d octets",
		     able, lavabuf.avail);
	buf += able;
	remainder = len - able;

	/* case: we exhausted our offer */
	if (lavabuf.avail <= 0) {

	    /* free our old buffer */
	    free(lavabuf.start);
	    lavabuf.start = NULL;
	    lavabuf.avail = 0;

	    /* note the size of our next buffer */
	    LAVA_DEBUG_B("raw_lavapool", "next buffer will grow from %d to %d",
			 lavabuf.size, lavabuf.size * BUF_GROW);
	    lavabuf.size *= BUF_GROW;
	}
	/* case: we have no buffer data */
    } else {
	remainder = len;
    }

    /*
     * case: we completely satisfied the user's request from our buffer
     */
    if (remainder <= 0) {
	return len;
    }
    /* at this point we do not have an allocated buffer, only a new size */

    /*
     * case: the unfulfilled request is >= old size
     */
    if (buf != NULL && remainder >= oldsize) {

	/*
	 * fulfill the user's request by transfers directly into
	 * the user's buffer
	 */
	do {

	    /* perform a direct transfer operation */
	    able = raw_lavaop(port, buf, remainder, timeout);
	    if (able < 0) {
		/* return lavaop error */
		LAVA_DEBUG_E("raw_lavapool", able);
		return able;
	    }

	    /* transfer accounting */
	    LAVA_DEBUG_B("raw_lavapool",
			 "direct transfer of %d octets, need %d more",
			 able, remainder - able);
	    buf += able;
	    remainder -= able;

	} while (remainder > 0);

	/*
	 * case: the unfulfilled request is < old size
	 *
	 * NOTE: When remainder < oldsize then we know that remainder < newsize
	 *       and this the new buffer will be large enough to fulfill
	 *       the request with buffer data left over.
	 */
    } else {

	/*
	 * allocate our buffer if we do not already have one
	 */
	if (lavabuf.start == NULL) {
	    lavabuf.start = (u_int8_t *) malloc(lavabuf.size);
	    if (lavabuf.start == NULL) {
		LAVA_DEBUG_E("raw_lavapool", LAVAERR_MALLOC);
		return LAVAERR_MALLOC;
	    }
	    lavabuf.avail = 0;
	}

	/*
	 * fill our the buffer
	 */
	do {

	    /* perform a direct transfer operation */
	    able = raw_lavaop(port, lavabuf.start + lavabuf.avail,
			      lavabuf.size - lavabuf.avail, timeout);
	    if (able < 0) {
		/* return lavaop error */
		LAVA_DEBUG_E("raw_lavapool", able);
		return able;
	    }

	    /* transfer accounting */
	    LAVA_DEBUG_B("raw_lavapool",
			 "adding %d octets to the %d octets already in buffer",
			 able, lavabuf.avail);
	    lavabuf.avail += able;
	    LAVA_DEBUG_B("raw_lavapool",
			 "copyed in %d octets to make %d octets available",
			 able, lavabuf.avail);

	} while (lavabuf.avail < lavabuf.size);

	/*
	 * copy out buffer data to the user's buffer if we have one
	 */
	if (buf != NULL) {
	    LAVA_DEBUG_B("raw_lavapool",
			 "copyout %d octets from %d octet buffer",
			 remainder, lavabuf.avail);
	    lavabuf.avail -= remainder;
	    memcpy(buf, lavabuf.start + lavabuf.avail, remainder);
	    LAVA_DEBUG_B("raw_lavapool",
			 "after %d octet copy, buffer has %d octets",
			 remainder, lavabuf.avail);
	}
    }

    /*
     * user request is complete and fulfilled
     */
    return len;
}


/*
 * raw_s100lava - obtain s100 data, maybe seeded LavaRnd (hidden low-level call)
 *
 * This function manages obtaining data from the s100 generator.  It attempts
 * to seed and reseed the generator when needed.  It tracks the quality
 * and returns error if the quality is lower than acceptable (as indicated
 * by the caller).  It also manages seed timeouts.
 *
 * given:
 *      port            host:port or /socket/path of request port
 *      buf             description of where to place s100 data
 *      len             max number of s100 octets to store at buf
 *      qual            pointer to s100 data quality produced (or NULL)
 *      minqual         minimum allowed quality (LAVA_QUAL_NONE if no min)
 *
 * returns:
 *      count           octets of s100 data returned or < 0 on error
 *
 * The minqual establishes a quality floor below which data is not
 * allowed to drop.  If the quality of the data drops below this floor,
 * an LAVAERR_POORQUAL error is returned.
 *
 * NOTE: Pass LAVA_QUAL_NONE as the minqual arg if one does not care
 *       about the quality of data returned.  Pass NULL the qual arg
 *       if one does not want the min quality information returned.
 *       By passing both NULL (qual) and LAVA_QUAL_NONE (minqual)
 *       the entire quality checking code is by passed.
 *
 * NOTE: 0 is never returned.
 *
 * NOTE: It is assumed that buf is at least len octets long.
 *
 * NOTE: There is some magic behind the data quality metric.  See the
 *       comments and NOTEs in the s100_quality() function for details.
 *
 * NOTE: Looking for the random number interfaces?   Try random.c and
 *       random_libc.c or perhaps s100.c.
 *
 * NOTE: The low-level s100 function could be considered s100_randomcpy()
 *       in s100.c.
 *
 * NOTE: Unlike the raw_lavapool() interface, this interface does not
 *       have a timeout argument.  Instead a static count of the
 *       number of consecutive LavaRnd seed errors is kept and
 *       an internal timeout is computed based on the s100_retries,
 *       s100_min_wait and s100_max_wait cfg.random values.
 */
static int
raw_s100lava(char *port, u_int8_t * buf, int len, lavaqual *qual,
	     lavaqual minqual)
{
    int remaining;	/* octets remaining before reseed is needed */
    int copied;	/* s100 octets already copied to output */
    int able;	/* number of octets we are able to copy now */
    lavaqual quality = LAVA_QUAL_NONE;	/* s100 data quality */

    /*
     * firewall
     */
    LAVA_DEBUG_S("raw_s100lava", "asking for %d s100 octets", len);
    if (port == NULL || buf == NULL) {
	/* invalid argument */
	LAVA_DEBUG_E("raw_s100lava", LAVAERR_BADARG);
	return LAVAERR_BADARG;
    }
    if (len <= 0) {
	/* invalid argument */
	LAVA_DEBUG_E("raw_s100lava", LAVAERR_BADLEN);
	lavarnd_errno = LAVAERR_BADLEN;
	lastop_errno = LAVAERR_BADLEN;
	LAVA_DEBUG_L("raw_s100lava");
	return LAVAERR_BADLEN;
    }

    /*
     * Reseed and pump cycle
     */
    /* initialize for pump loop */
    copied = 0;
    remaining = s100_loadleft(&s100);
    if (remaining > 0 && ((int)minqual > (int)LAVA_QUAL_NONE || qual != NULL)) {
	quality = s100_quality(&s100);
	LAVA_DEBUG_Q("raw_s100lava", "initialized pump loop", quality);
	if ((int)quality < (int)minqual) {
	    /* quality is too poor */
	    LAVA_DEBUG_E("raw_s100lava", LAVAERR_POORQUAL);
	    return LAVAERR_POORQUAL;
	}
    }
    /* data pump loop */
    do {

	/*
	 * If we do not have enough seed data, then try to reseed
	 *
	 * If the raw_lavapool() call fails, we will try our best with a
	 * default generator state perturbed by system state.
	 */
	if (remaining <= 0) {
	    static int load_len = 0;	/* octets needed to fully seed s100 */
	    static int lim_reported = 0;	/* 1 ==> reported retry limit */
	    lavaqual tqual;	/* temp quality value */
	    int ret;	/* raw_lavapool return status/size */

	    /*
	     * try to seed s100 with LavaRnd data
	     */
	    if (load_len <= 0) {
		load_len = sizeof(struct s100_seed);
	    }
	    if (s100_seed_err > cfg_random.s100_retries) {
		/* lavapool daemon failed before, do not retry */
		ret = 0;
	    } else {
		/* sleep if we are retrying to seed but not at the limit */
		if (s100_seed_err > 0) {
		    /* sleep before a retry */
		    s100_timeout = TIME_SLIDE(s100_seed_err - 1,
					      cfg_random.s100_retries,
					      cfg_random.s100_min_wait,
					      cfg_random.s100_max_wait);
		    LAVA_DEBUG_Z("raw_s100lava", s100_timeout);
		    lava_sleep(s100_timeout);
		}
		/* try to seed */
		ret = raw_lavapool(port, lava_s100_seed, load_len,
				   s100_timeout);
	    }

	    /*
	     * case: seed was successful
	     */
	    if (ret > 0) {
		/* successful seed */
		LAVA_DEBUG_S("raw_s100lava",
			     "loading s100 generator with %d octets of seed",
			     ret);
		s100_load(&s100, lava_s100_seed, ret);
		remaining = s100_loadleft(&s100);
		/* reset s100 seed error accounting */
		if (s100_seed_err > 0) {
		    s100_seed_err = 0;
		    s100_timeout = cfg_random.s100_min_wait;
		    lim_reported = 0;
		}
		LAVA_DEBUG_C("raw_s100lava",
			     "s100 seed successful, remaining: %d", remaining);

		/*
		 * case: seed attempt failed
		 */
	    } else {
		/* seeding failed, force seed with degraded system state */
		LAVA_DEBUG_S("raw_s100lava",
			     "s100 seed with degraded system state + %d octets",
			     0);
		s100_load(&s100, NULL, 0);
		/* update s100 seed error accounting */
		if (s100_seed_err <= cfg_random.s100_retries) {
		    LAVA_DEBUG_C("raw_s100lava",
				 "s100 seed failed, retry: %d", s100_seed_err);
		    ++s100_seed_err;
		    lim_reported = 0;
		    /* report when we reach the retry limit */
		} else if (lim_reported == 0) {
		    LAVA_DEBUG_C("raw_s100lava",
				 "s100 seed failed, retry limit: %d",
				 cfg_random.s100_retries);
		    lim_reported = 1;
		}
	    }

	    /*
	     * determine what has happened to the quality
	     */
	    if ((int)minqual > (int)LAVA_QUAL_NONE || qual != NULL) {
		tqual = s100_quality(&s100);
		LAVA_DEBUG_Q("raw_s100lava", "data pump loop", tqual);
		if (quality == LAVA_QUAL_NONE) {
		    /* 1st reseed is our new quality level */
		    quality = tqual;
		    LAVA_DEBUG_Q("raw_s100lava", "1st s100 seed", quality);
		} else if ((int)tqual < (int)quality) {
		    /* last reseed lowered our quality */
		    quality = tqual;
		    LAVA_DEBUG_Q("raw_s100lava", "s100 reseed", quality);
		}
		if ((int)quality < (int)minqual) {
		    /* quality is too poor */
		    LAVA_DEBUG_E("raw_s100lava", LAVAERR_POORQUAL);
		    return LAVAERR_POORQUAL;
		}
	    }
	}

	/*
	 * satisfy as much of the request as we can
	 */
	if (remaining > 0) {
	    /* copy as much data as we can at this quality */
	    able = ((remaining < (len - copied)) ? remaining : len - copied);
	} else {
	    /* seeding failed, output we much degraded data as we need */
	    able = (len - copied);
	}
	s100_randomcpy(&s100, buf + copied, able);
	copied += able;

	/* end of data pump loop */
    } while (copied < len);

    /*
     * save our quality if requested
     */
    if (qual) {
	*qual = quality;
    }

    /*
     * return copy count
     */
    LAVA_DEBUG_Q("raw_s100lava", "returned data", quality);
    return copied;
}


/*
 * raw_random - process raw data from LavaRnd and s100 (mid-level call)
 *
 * This function manages the return and error conditions from the low-level
 * raw_lavapool() and raw_s100lava() calls.  It takes care of retries,
 * errors (perhaps due to low level timeouts or poor s100 data quality)
 * overall quality tracking, output size and callback processing.
 *
 * given:
 *      buf             where to put LavaRnd data
 *      len             amount of LavaRnd or s100 data requested
 *                          NOTE: len must be <= LAVA_MAX_LEN_ARG
 *                                unless only s100 data is requested
 *      callback        what to do on error or callback function
 *      amt_p           amount of data returned or NULL if don't care
 *      has_ret         FALSE ==> calling function will ignore errors,
 *                      TRUE ==> calling function will receive a status code
 *
 * returns:
 *      quality of the data returned,
 *              > LAVA_QUAL_NONE    ==>  success (with some level of quality)
 *              LAVA_QUAL_NONE (0)  ==>  no returned data
 *              < 0                 ==>  error (LAVERR_XYZ)
 *
 * NOTE: Looking for the random number interfaces?   Try random.c and
 *       random_libc.c or perhaps s100.c.
 *
 * NOTE: The LAVA_MAX_LEN_ARG value applies if the callback method
 *       is capable of directly requesting lavapool data.  Callbacks such
 *       as LAVACALL_S100_HIGH, LAVACALL_S100_MED, and LAVACALL_S100_ANY
 *       do not directly request lavapool data (except when attempting
 *       to seed/reseed) may use any signed int length.
 *
 * NOTE: As a side effect, the external LavaRnd error indicator, lavarnd_errno,
 *       may be set to a value < 0 if the requirements of the callback
 *       cannot be satisfied.
 *
 * NOTE: As a side effect, the external LavaRnd error indicator, lastop_errno,
 *       may be set to a value < 0 if the requirements of the callback
 *       cannot be satisfied.
 *
 * NOTE: This function does not set lastop_errno to LAVAERR_OK when it
 *       starts.  The lastop_errno value is cleared by routines in
 *       random.c so those the higher level calls can detect problems
 *       over potentially multiple calls to this function.
 */
int
raw_random(u_int8_t * buf, int len, lavaback callback, int *amt_p, int has_ret)
{
    int status;	/* raw_lavapool return status */
    lavaqual quality;	/* min random quality in buffer */
    lavaqual p_qual;	/* previous call data quality */
    int retry_cnt;	/* fetch data retry count */
    double timeout;	/* current timeout value */
    double pause;	/* seconds to wait for retry */
    int offset;	/* octets already written */
    int stop_loop;	/* TRUE ==> stop retry loop now */

    /*
     * firewall
     */
    LAVA_DEBUG_S("raw_random", "asking for %d octets", len);
    LAVA_DEBUG_V("raw_random", callback);
    if (buf == NULL) {
	/* invalid argument */
	LAVA_DEBUG_E("raw_random", LAVAERR_BADARG);
	lavarnd_errno = LAVAERR_BADARG;
	lastop_errno = LAVAERR_BADARG;
	LAVA_DEBUG_L("raw_random");
	return LAVAERR_BADARG;
    }
    if (len > lava_maxlen(callback)) {
	/* invalid argument */
	LAVA_DEBUG_E("raw_random", LAVAERR_TOOMUCH);
	lavarnd_errno = LAVAERR_TOOMUCH;
	lastop_errno = LAVAERR_TOOMUCH;
	LAVA_DEBUG_L("raw_random");
	return LAVAERR_TOOMUCH;
    }
    if (len <= 0) {
	/* invalid argument */
	LAVA_DEBUG_E("raw_random", LAVAERR_BADLEN);
	lavarnd_errno = LAVAERR_BADLEN;
	lastop_errno = LAVAERR_BADLEN;
	LAVA_DEBUG_L("raw_random");
	return LAVAERR_BADLEN;
    }
    timeout = 0.0;

    /*
     * configure the LavaRnd library system
     */
    if (!is_cfg) {
	if (preload_cfg(LAVA_RANDOM_CFG) < 0) {
	    /* config failed */
	    LAVA_DEBUG_E("raw_random", LAVAERR_BADCFG);
	    lavarnd_errno = LAVAERR_BADCFG;
	    lastop_errno = LAVAERR_BADCFG;
	    LAVA_DEBUG_L("raw_random");
	    return LAVAERR_BADCFG;
	}
    }

    /*
     * set the initial timeout
     */
    timeout = def_lava_timeout(callback);

    /*
     * preseed private s100 generator if needed
     *
     * In cases where we might need to use a private s100 generator
     * (when contacting the lavapool daemon fails, and the callback
     * suggests that s100 could be used an an alternative), and
     * when we are about to output enough LavaRnd lavapool data,
     * and the "s100_preseed_amt" configuration value is >= 0,
     * and the private s100 generator has not already been preseeded,
     * and the previous attempt to preseed the private s100 generator
     * did not fail, then
     * we will try preseed the private s100 generator.
     */
    preseed_s100(callback, len);

    /*
     * obtain the data
     */
    retry_cnt = 0;
    offset = 0;
    stop_loop = FALSE;
    quality = LAVA_QUAL_NONE;
    do {

	/*
	 * try to obtain the data
	 *
	 * We will try for LavaRnd data unless LAVACALL_S100_XYZ or
	 * LAVACALL_TRYONCE_XYZ after a lavapool daemon failure.
	 */
	p_qual = LAVA_QUAL_LAVARND;
	switch ((int)callback) {
	case CASE_LAVACALL_S100_HIGH:
	    status = raw_s100lava(cfg_random.lavapool,
				  buf + offset, len - offset,
				  &p_qual, LAVA_QUAL_S100HIGH);
	    LAVA_DEBUG_Q("raw_random", "LAVACALL_S100_HIGH/s100", p_qual);
	    break;
	case CASE_LAVACALL_S100_MED:
	    status = raw_s100lava(cfg_random.lavapool,
				  buf + offset, len - offset,
				  &p_qual, LAVA_QUAL_S100MED);
	    LAVA_DEBUG_Q("raw_random", "LAVACALL_S100_MED/s100", p_qual);
	    break;
	case CASE_LAVACALL_S100_ANY:
	    status = raw_s100lava(cfg_random.lavapool,
				  buf + offset, len - offset,
				  &p_qual, LAVA_QUAL_S100LOW);
	    LAVA_DEBUG_Q("raw_random", "LAVACALL_S100_ANY/s100", p_qual);
	    break;
	case CASE_LAVACALL_TRYONCE_HIGH:
	    if (daemon_err) {
		status = raw_s100lava(cfg_random.lavapool,
				      buf + offset, len - offset,
				      &p_qual, LAVA_QUAL_S100HIGH);
		LAVA_DEBUG_Q("raw_random",
			     "LAVACALL_TRYONCE_HIGH/s100", p_qual);
	    } else {
		status = raw_lavapool(cfg_random.lavapool,
				      buf + offset, len - offset, timeout);
		if (status > 0) {
		    p_qual = LAVA_QUAL_LAVARND;
		    LAVA_DEBUG_Q("raw_random",
				 "LAVACALL_TRYONCE_HIGH/lavapool", p_qual);
		}
	    }
	    break;
	case CASE_LAVACALL_TRYONCE_MED:
	    if (daemon_err) {
		status = raw_s100lava(cfg_random.lavapool,
				      buf + offset, len - offset,
				      &p_qual, LAVA_QUAL_S100MED);
		LAVA_DEBUG_Q("raw_random",
			     "LAVACALL_TRYONCE_MED/s100/s100", p_qual);
	    } else {
		status = raw_lavapool(cfg_random.lavapool,
				      buf + offset, len - offset, timeout);
		if (status > 0) {
		    p_qual = LAVA_QUAL_LAVARND;
		    LAVA_DEBUG_Q("raw_random",
				 "LAVACALL_TRYONCE_MED/lavapool", p_qual);
		}
	    }
	    break;
	case CASE_LAVACALL_TRYONCE_ANY:
	    if (daemon_err) {
		status = raw_s100lava(cfg_random.lavapool,
				      buf + offset, len - offset,
				      &p_qual, LAVA_QUAL_S100LOW);
		LAVA_DEBUG_Q("raw_random",
			     "LAVACALL_TRYONCE_ANY/s100/s100", p_qual);
	    } else {
		status = raw_lavapool(cfg_random.lavapool,
				      buf + offset, len - offset, timeout);
		if (status > 0) {
		    p_qual = LAVA_QUAL_LAVARND;
		    LAVA_DEBUG_Q("raw_random",
				 "LAVACALL_TRYONCE_ANY/lavapool", p_qual);
		}
	    }
	    break;
	default:
	    status = raw_lavapool(cfg_random.lavapool,
				  buf + offset, len - offset, timeout);
	    if (status > 0) {
		p_qual = LAVA_QUAL_LAVARND;
		LAVA_DEBUG_Q("raw_random", "default case/lavapool", p_qual);
	    }
	    break;
	}

	/*
	 * case: raw_xyz() returned an error
	 */
	if (status <= 0) {
	    ++retry_cnt;
	    switch ((int)callback) {
	    case CASE_LAVACALL_LAVA_EXIT:
		/*
		 * exit if too many errors
		 */
		if (retry_cnt > cfg_random.exit_retries) {
		    /* exit retry limit reached */
		    LAVA_DEBUG_C("raw_random",
				 "LAVACALL_LAVA_EXIT: "
				 "reached retry limit: %d, exiting",
				 cfg_random.exit_retries);
		    lavarnd_errno = status;
		    lastop_errno = status;
		    LAVA_DEBUG_L("raw_random");
		    exit(1);
		}
		LAVA_DEBUG_C("raw_random",
			     "LAVACALL_LAVA_EXIT: retry: %d", retry_cnt);

		/*
		 * wait a bit before retrying
		 */
		LAVA_DEBUG_Z("raw_random", timeout);
		lava_sleep(timeout);

		/*
		 * exit retry limit not reached: try, try again with new timeout
		 */
		timeout = TIME_SLIDE(retry_cnt,
				     cfg_random.exit_retries - 1,
				     cfg_random.exit_min_wait,
				     cfg_random.exit_max_wait);
		(void)lava_timeout(timeout);
		break;

	    case CASE_LAVACALL_LAVA_RETRY:
		/*
		 * wait a bit before retrying
		 */
		LAVA_DEBUG_C("raw_random",
			     "LAVACALL_LAVA_RETRY: retry: %d", retry_cnt);
		LAVA_DEBUG_Z("raw_random", timeout);
		lava_sleep(timeout);

		/*
		 * if we did not succeed, try try again!
		 */
		timeout += cfg_random.retry_inc_wait;
		if (timeout > cfg_random.retry_max_wait) {
		    timeout = cfg_random.retry_max_wait;
		}
		(void)lava_timeout(timeout);
		break;

	    case CASE_LAVACALL_LAVA_RETURN:
		/*
		 * return if too many errors
		 */
		if (retry_cnt > cfg_random.return_retries) {

		    /* return error if we did nothing */
		    LAVA_DEBUG_C("raw_random",
				 "LAVACALL_LAVA_RETURN: "
				 "reached retry limit: %d, loop stop",
				 cfg_random.exit_retries);
		    stop_loop = TRUE;
		    lavarnd_errno = status;
		    lastop_errno = status;
		    LAVA_DEBUG_L("raw_random");

		    /*
		     * try some more
		     */
		} else {

		    /*
		     * wait a bit before retrying
		     */
		    LAVA_DEBUG_C("raw_random",
				 "LAVACALL_LAVA_RETURN: retry: %d", retry_cnt);
		    LAVA_DEBUG_Z("raw_random", timeout);
		    lava_sleep(timeout);

		    /*
		     * retry limit not reached: try, try again with new timeout
		     */
		    timeout = TIME_SLIDE(retry_cnt,
					 cfg_random.return_retries - 1,
					 cfg_random.return_min_wait,
					 cfg_random.return_max_wait);
		    (void)lava_timeout(timeout);
		}
		break;

	    case CASE_LAVACALL_TRY_HIGH:
		LAVA_DEBUG_C("raw_random",
			     "LAVACALL_TRY_HIGH: retry: %d, try s100 high",
			     retry_cnt);
		status = raw_s100lava(cfg_random.lavapool,
				      buf + offset, len - offset,
				      &p_qual, LAVA_QUAL_S100HIGH);
		LAVA_DEBUG_Q("raw_random", "LAVACALL_S100_HIGH/s100", p_qual);
		stop_loop = TRUE;
		if (status < 0) {
		    lavarnd_errno = status;
		    lastop_errno = status;
		    LAVA_DEBUG_L("raw_random");
		}
		break;

	    case CASE_LAVACALL_TRYONCE_HIGH:
		LAVA_DEBUG_C("raw_random",
			     "LAVACALL_TRYONCE_HIGH: retry: %d, now s100 high",
			     retry_cnt);
		status = raw_s100lava(cfg_random.lavapool,
				      buf + offset, len - offset,
				      &p_qual, LAVA_QUAL_S100HIGH);
		LAVA_DEBUG_Q("raw_random",
			     "LAVACALL_TRYONCE_HIGH/s100", p_qual);
		stop_loop = TRUE;
		if (status < 0) {
		    lavarnd_errno = status;
		    lastop_errno = status;
		    LAVA_DEBUG_L("raw_random");
		}
		break;

	    case CASE_LAVACALL_TRY_MED:
		LAVA_DEBUG_C("raw_random",
			     "LAVACALL_TRY_MED: retry: %d, try s100 high/med",
			     retry_cnt);
		status = raw_s100lava(cfg_random.lavapool,
				      buf + offset, len - offset,
				      &p_qual, LAVA_QUAL_S100MED);
		LAVA_DEBUG_Q("raw_random", "LAVACALL_TRY_MED/s100", p_qual);
		stop_loop = TRUE;
		if (status < 0) {
		    lavarnd_errno = status;
		    lastop_errno = status;
		    LAVA_DEBUG_L("raw_random");
		}
		break;

	    case CASE_LAVACALL_TRYONCE_MED:
		LAVA_DEBUG_C("raw_random",
			     "LAVACALL_TRYONCE_MED: retry: %d, now s100 high/med",
			     retry_cnt);
		status = raw_s100lava(cfg_random.lavapool,
				      buf + offset, len - offset,
				      &p_qual, LAVA_QUAL_S100MED);
		LAVA_DEBUG_Q("raw_random", "LAVACALL_TRYONCE_MED/s100",
			     p_qual);
		stop_loop = TRUE;
		if (status < 0) {
		    lavarnd_errno = status;
		    lastop_errno = status;
		    LAVA_DEBUG_L("raw_random");
		}
		break;

	    case CASE_LAVACALL_TRY_ANY:
		LAVA_DEBUG_C("raw_random",
			     "LAVACALL_TRY_ANY: retry: %d, try s100 any",
			     retry_cnt);
		status = raw_s100lava(cfg_random.lavapool,
				      buf + offset, len - offset,
				      &p_qual, LAVA_QUAL_S100LOW);
		LAVA_DEBUG_Q("raw_random", "LAVACALL_TRY_ANY/s100", p_qual);
		stop_loop = TRUE;
		if (status < 0) {
		    lavarnd_errno = status;
		    lastop_errno = status;
		    LAVA_DEBUG_L("raw_random");
		}
		break;

	    case CASE_LAVACALL_TRYONCE_ANY:
		LAVA_DEBUG_C("raw_random",
			     "LAVACALL_TRYONCE_ANY: retry: %d, now s100 any",
			     retry_cnt);
		status = raw_s100lava(cfg_random.lavapool,
				      buf + offset, len - offset,
				      &p_qual, LAVA_QUAL_S100LOW);
		LAVA_DEBUG_Q("raw_random", "LAVACALL_TRYONCE_ANY/s100",
			     p_qual);
		stop_loop = TRUE;
		if (status < 0) {
		    lavarnd_errno = status;
		    lastop_errno = status;
		    LAVA_DEBUG_L("raw_random");
		}
		break;

	    case CASE_LAVACALL_S100_HIGH:
		LAVA_DEBUG_C("raw_random",
			     "LAVACALL_S100_HIGH: retry: %d, loop stop",
			     retry_cnt);
		stop_loop = TRUE;
		lavarnd_errno = status;
		lastop_errno = status;
		LAVA_DEBUG_L("raw_random");
		break;

	    case CASE_LAVACALL_S100_MED:
		LAVA_DEBUG_C("raw_random",
			     "LAVACALL_S100_MED: retry: %d, loop stop",
			     retry_cnt);
		stop_loop = TRUE;
		lavarnd_errno = status;
		lastop_errno = status;
		LAVA_DEBUG_L("raw_random");
		break;

	    case CASE_LAVACALL_S100_ANY:
		LAVA_DEBUG_C("raw_random",
			     "LAVACALL_S100_ANY: retry: %d, loop stop",
			     retry_cnt);
		stop_loop = TRUE;
		lavarnd_errno = status;
		lastop_errno = status;
		LAVA_DEBUG_L("raw_random");
		break;

	    default:		/* callback function control */
		/*
		 * LavaRnd failed, call the callback function
		 */
		LAVA_DEBUG_C("raw_random",
			     "user_callback: retry: %d", retry_cnt);
		pause = callback(buf, len, offset, status, has_ret);
		if (pause == LAVA_CALLBACK_EXIT) {

		    /* callback requested that we exit */
		    LAVA_DEBUG_C("raw_random",
				 "user_callback: retry: %d will exit",
				 retry_cnt);
		    lavarnd_errno = status;
		    lastop_errno = status;
		    LAVA_DEBUG_L("raw_random");
		    exit(2);

		} else if (pause == LAVA_CALLBACK_RETURN) {

		    /* callback requested we return our best effort */
		    LAVA_DEBUG_C("raw_random",
				 "user_callback: retry: %d try s100 any",
				 retry_cnt);
		    status = raw_s100lava(cfg_random.lavapool,
					  buf + offset, len - offset,
					  &p_qual, LAVA_QUAL_NONE);
		    LAVA_DEBUG_Q("raw_random",
				 "LAVA_CALLBACK_RETURN/s100", p_qual);
		    stop_loop = TRUE;
		    lavarnd_errno = status;
		    lastop_errno = status;
		    LAVA_DEBUG_L("raw_random");

		} else if (pause == LAVA_CALLBACK_S100HIGH) {

		    /* callback requested we return s100 data of high quality */
		    LAVA_DEBUG_C("raw_random",
				 "user_callback: retry: %d try s100 high",
				 retry_cnt);
		    status = raw_s100lava(cfg_random.lavapool,
					  buf + offset, len - offset,
					  &p_qual, LAVA_QUAL_S100HIGH);
		    LAVA_DEBUG_Q("raw_random",
				 "LAVA_CALLBACK_S100HIGH/s100", p_qual);
		    if (status < 0) {
			lavarnd_errno = status;
			lastop_errno = status;
			LAVA_DEBUG_L("raw_random");
		    }

		} else if (pause == LAVA_CALLBACK_S100MED) {

		    /* callback requested we return s100 data of med quality */
		    LAVA_DEBUG_C("raw_random",
				 "user_callback: retry: %d try s100 med",
				 retry_cnt);
		    status = raw_s100lava(cfg_random.lavapool,
					  buf + offset, len - offset,
					  &p_qual, LAVA_QUAL_S100MED);
		    LAVA_DEBUG_Q("raw_random",
				 "LAVA_CALLBACK_S100MED/s100", p_qual);
		    if (status < 0) {
			lavarnd_errno = status;
			lastop_errno = status;
			LAVA_DEBUG_L("raw_random");
		    }

		} else if (pause == LAVA_CALLBACK_S100LOW) {

		    /* callback requested we return s100 data of any quality */
		    LAVA_DEBUG_C("raw_random",
				 "user_callback: retry: %d try s100 any",
				 retry_cnt);
		    status = raw_s100lava(cfg_random.lavapool,
					  buf + offset, len - offset,
					  &p_qual, LAVA_QUAL_S100LOW);
		    LAVA_DEBUG_Q("raw_random",
				 "LAVA_CALLBACK_S100LOW/s100", p_qual);
		    if (status < 0) {
			lavarnd_errno = status;
			lastop_errno = status;
			LAVA_DEBUG_L("raw_random");
		    }

		} else if (pause < 0.0) {

		    /* unexpected timeout value, assume exit */
		    LAVA_DEBUG_C("raw_random",
				 "user_callback: retry: %d unexpected callback",
				 retry_cnt);
		    lavarnd_errno = status;
		    lastop_errno = status;
		    LAVA_DEBUG_L("raw_random");
		    exit(3);

		} else {

		    /* callback sleep for timeout value */
		    LAVA_DEBUG_C("raw_random",
				 "user_callback: retry: %d sleep and retry",
				 retry_cnt);
		    LAVA_DEBUG_Z("raw_random", pause);
		    lava_sleep(pause);
		}
		timeout = lava_timeout(0.0);
		break;
	    }
	}

	/*
	 * case: we obtained some data
	 */
	if (status > 0) {

	    /* count output */
	    offset += status;

	    /* track the minimum quality */
	    if ((int)p_qual < (int)quality || quality == LAVA_QUAL_NONE) {
		quality = p_qual;
		LAVA_DEBUG_Q("raw_random", "new minimum", quality);
	    }

	    /* reset the initial timeout to default */
	    timeout = def_lava_timeout(callback);
	    retry_cnt = 0;
	}

	/* otherwise retry until forced to stop or done */
    } while (!stop_loop && offset < len);
    /* lavarnd_errno firewall */
    if (offset < len && lavarnd_errno >= 0) {
	/* did not satisfy the request and yet lavarnd_errno is not <0 ! */
	LAVA_DEBUG_E("raw_random", LAVAERR_IMPOSSIBLE);
	lavarnd_errno = LAVAERR_IMPOSSIBLE;
	lastop_errno = LAVAERR_IMPOSSIBLE;
	LAVA_DEBUG_L("raw_random");
    }

    /*
     * all done
     */
    if (amt_p != NULL) {
	*amt_p = offset;
    }
    LAVA_DEBUG_Q("raw_random", "returned data", quality);
    LAVA_DEBUG_R("raw_random", offset);
    LAVA_DEBUG_H("raw_random", buf, offset);
    return quality;
}


/*
 * lava_preload - prepare for lavapool use
 *
 * We will attempt to pre-fill the lavabuf buffer and/or seed the
 * subtractive 100 shuffle generator.  We will try to contact the
 * lavapool daemon once and wait for up to preload_wait seconds
 * for a reply.  If no reply, then return LAVAERR_TIMEOUT.
 *
 * We will not attempt to load more data than the largest
 * buffer size: MAX_BUF_SIZE.
 *
 * If we already have buffered data, we will attempt to expand the
 * buffer or fill the buffer if there is not enough data/room.
 *
 * given:
 *      lavacnt         requested pre-load in octets,
 *                          0 ==> MIN_BUF_SIZE, <0 ==> do not pre-load
 *      seed_s100       TRUE ==> also seed s100, FALSE ==> ignore it
 *
 * returns:
 *      number of preloaded lavapool octets, not counting those
 *      that may have been used to see the private s100 generator
 *
 * NOTE: The return may be more than requested if the buffer already
 *       contained more data.  It may be less than requested if
 *       amount requested was close to the MAX_BUF_SIZE and the
 *       s100 generator was seeded.
 *
 * NOTE: lava_preload(-1, FALSE) will just return the amount of
 *       available data without touching generator states.
 *
 * NOTE: As a side effect, the external LavaRnd error indicator, lavarnd_errno,
 *       may be set to a value < 0 if the preload request cannot be satisfied.
 *
 * NOTE: As a side effect, the external LavaRnd error indicator, lastop_errno,
 *       may be set to a value < 0 if the preload request cannot be satisfied.
 *
 * NOTE: Because raw_random() may call lava_preload() indirectly via
 *       preseed_s100(), this function cannot call raw_random().
 *       Instead it calls raw_lavapool() with a special NULL buf arg.
 */
int
lava_preload(int lavacnt, int seed_s100)
{
    int bufsiz;	/* size of lavabuf needed */
    int avail;	/* available lavabuf octets or error code */

    /*
     * determine what we will try to preload
     */
    if (seed_s100) {
	/* ignore a seed_s100 request if the private has been seeded already */
	/* inline equiv of: if (s100_loadleft(&s100) > 0) { ... } */
	if (s100.seeded && (s100.nextspin > 0 || s100.rndbuf_len > 0)) {
	    /* private s100 generator already seeded */
	    seed_s100 = FALSE;
	    LAVA_DEBUG_S("lava_preload",
			 "lavacnt: %d, seed_s100 forced FALSE", lavacnt);
	} else {
	    LAVA_DEBUG_S("lava_preload", "lavacnt: %d + s100 seed", lavacnt);
	}
    } else {
	LAVA_DEBUG_S("lava_preload", "lavacnt: %d", lavacnt);
    }

    /*
     * configure the LavaRnd library system
     */
    if (!is_cfg) {
	if (preload_cfg(LAVA_RANDOM_CFG) < 0) {
	    /* config failed */
	    LAVA_DEBUG_E("lava_preload", LAVAERR_BADARG);
	    lavarnd_errno = LAVAERR_BADARG;
	    lastop_errno = LAVAERR_BADARG;
	    LAVA_DEBUG_L("lava_preload");
	    return LAVAERR_BADCFG;
	}
    }

    /*
     * initialize the avail count to what we have now, if any
     */
    if (lavabuf.start == NULL) {
	avail = 0;
    } else {
	avail = lavabuf.avail;
    }

    /*
     * Attempt to pre-load the lavapool buffer and/or
     * we need data to seed the s100 generator.
     */
    if (lavacnt >= 0 || seed_s100) {

	/*
	 * increase our request if also seeding s100
	 */
	if (seed_s100) {
	    if (lavacnt < 0) {
		lavacnt = sizeof(struct s100_seed);
	    } else {
		lavacnt += sizeof(struct s100_seed);
	    }
	}

	/*
	 * make it at least MIN_BUF_SIZE, and no more than MAX_BUF_SIZE
	 */
	if (lavacnt < MIN_BUF_SIZE) {
	    lavacnt = MIN_BUF_SIZE;
	} else if (lavacnt > MAX_BUF_SIZE) {
	    lavacnt = MAX_BUF_SIZE;
	}
	LAVA_DEBUG_B("lava_preload",
		     "have %d octets available, will try for %d octets",
		     avail, lavacnt);

	/*
	 * if we need more data, obtain some
	 */
	if (lavabuf.start == NULL || lavabuf.avail < lavacnt) {

	    /*
	     * determine how large the buffer become
	     */
	    for (bufsiz = MIN_BUF_SIZE;
		 bufsiz < lavacnt && bufsiz < MAX_BUF_SIZE;
		 bufsiz *= BUF_GROW) {
	    }
	    if (bufsiz > MAX_BUF_SIZE) {
		bufsiz = MAX_BUF_SIZE;
	    }
	    LAVA_DEBUG_B("lava_preload",
			 "buffer will grow fron %d to %d octets",
			 lavabuf.size, bufsiz);

	    /*
	     * case: prep if do not have a buffer
	     */
	    if (lavabuf.start == NULL) {
		lavabuf.size = bufsiz;
		lavabuf.avail = 0;

		/*
		 * case: expand existing buffer if too small
		 */
	    } else if (lavabuf.size < lavacnt) {
		char *new;	/* our new buffer */

		/*
		 * allocate a larger buffer
		 */
		new = (u_int8_t *) malloc(bufsiz);
		if (new == NULL) {
		    LAVA_DEBUG_E("lava_preload", LAVAERR_MALLOC);
		    lavarnd_errno = LAVAERR_MALLOC;
		    lastop_errno = LAVAERR_MALLOC;
		    LAVA_DEBUG_L("lava_preload");
		    return LAVAERR_MALLOC;
		}
		/* copy over existing data */
		LAVA_DEBUG_B("lava_preload",
			     "will copy over %d octets to new %d octet buffer",
			     lavabuf.avail, bufsiz);
		memcpy(new, lavabuf.start, lavabuf.avail);
		/* swap buffers */
		free(lavabuf.start);
		lavabuf.start = new;
		/* note new size */
		lavabuf.size = bufsiz;
	    }

	    /*
	     * try to preload the buffer
	     */
	    avail = raw_lavapool(cfg_random.lavapool, NULL, lavacnt,
				 cfg_random.preload_wait);
	    if (avail < 0) {
		/* buffer load failed, return error now */
		LAVA_DEBUG_E("lava_preload", avail);
		lavarnd_errno = avail;
		lastop_errno = avail;
		LAVA_DEBUG_L("lava_preload");
		return avail;
	    }

	    /*
	     * note the available data in the buffer
	     */
	    /* firewall */
	    if (lavabuf.start == NULL) {
		/* we cannot have lost a buffer! */
		LAVA_DEBUG_E("lava_preload", LAVAERR_IMPOSSIBLE);
		lavarnd_errno = LAVAERR_IMPOSSIBLE;
		lastop_errno = LAVAERR_IMPOSSIBLE;
		LAVA_DEBUG_L("lava_preload");
		return LAVAERR_IMPOSSIBLE;
	    }
	    avail = lavabuf.avail;
	    LAVA_DEBUG_B("lava_preload",
			 "now have %d octets available in an %d octet buffer",
			 lavabuf.avail, lavabuf.size);
	}
    }

    /*
     * attempt to seed the s100 generator if requested
     */
    if (seed_s100) {

	/*
	 * firewall - we must have enough data to high quality seed s100
	 */
	if (lavabuf.start == NULL ||
	    avail < sizeof(struct s100_seed) ||
	    lavabuf.avail < sizeof(struct s100_seed)) {
	    LAVA_DEBUG_E("lava_preload", LAVAERR_IMPOSSIBLE);
	    lavarnd_errno = LAVAERR_IMPOSSIBLE;
	    lastop_errno = LAVAERR_IMPOSSIBLE;
	    LAVA_DEBUG_L("lava_preload");
	    return LAVAERR_IMPOSSIBLE;
	}

	/*
	 * seed with the data from the end of the buffer
	 */
	LAVA_DEBUG_S("lava_preload",
		     "seeding s100 generator with %d octets of seed",
		     sizeof(struct s100_seed));
	s100_load(&s100,
		  lavabuf.start + (lavabuf.avail - sizeof(struct s100_seed)),
		  sizeof(struct s100_seed));

	/*
	 * remove the data used by the s100 seed
	 */
	lavabuf.avail -= sizeof(struct s100_seed);
	avail -= sizeof(struct s100_seed);
	LAVA_DEBUG_B("lava_preload",
		     "now have %d octets available after %d octet s100 seed",
		     lavabuf.avail, sizeof(struct s100_seed));
    }

    /*
     * return available count data
     */
    return avail;
}


/*
 * config_lavapool - load a lavapool configuration file
 *
 * given:
 *      cfg_file        path to cfg.random (or NULL for default path)
 *      *config         configuration to set
 *
 * returns:
 *      0 - all of OK, *config is the new configuration
 *      <0 - error, *config is not changed
 *
 * NOTE: One might want call def_lavapool(config) before calling
 *       cfg_random(config) so that in case this routine
 *       fails, one is left with the default configuration.
 */
static int
config_lavapool(char *cfg_file, struct cfg_random *config)
{
    int ret;	/* function return code */

    /*
     * firewall
     */
    if (config == NULL) {
	return -1;		/* bad arg */
    }
    if (cfg_file == NULL) {
	cfg_file = LAVA_RANDOM_CFG;
    }

    /*
     * attempt parse the default file
     */
    ret = parse_lavapool(cfg_file, config);
    return ret;
}


/*
 * def_lavapool - load the default configuration
 *
 * given:
 *      *config  - configuration to set
 *
 * returns:
 *      0 - all of OK, *config is the new configuration
 *      <0 - error, invalid args or out of memory
 *
 * NOTE: Even if -1 is returned, the config (assuming it was non-NULL)
 *       will have non-string values initialized to default values.
 */
static int
def_lavapool(struct cfg_random *config)
{
    /*
     * firewall
     */
    if (config == NULL) {
	return -1;		/* bad arg */
    }

    /*
     * copy in the default configuration
     */
    if (dup_lavapool_cfg(&def_cfg, config) == NULL) {
	return -1;
    }
    return 0;			/* success */
}


/*
 * parse_lavapool - parse a cfg.random configuration file
 *
 * given:
 *      filename - name of the cfg.random configuration file
 *      *config  - configuration to set
 *
 * returns:
 *      0 - all of OK, *config is the new configuration
 *      <0 - error, *config is not changed
 *
 * All lines that the empty or that start with # are treated as
 * configuration file comments and are ignored.
 */
static int
parse_lavapool(char *filename, struct cfg_random *config)
{
    FILE *f;	/* configuration file stream */
    char buf[MAXLINE + 1];	/* config file buffer */
    int linenum;	/* config line number */
    struct cfg_random new;	/* new configuration to set */

    /*
     * firewall
     */
    if (filename == NULL || config == NULL) {
	return -1;		/* bad arg */
    }
    new.lavapool = NULL;
    new.malloced_str = 0;

    /*
     * open the cfg.random file
     */
    f = fopen(filename, "r");
    if (f == NULL) {
	return -1;		/* bad open */
    }

    /*
     * initialize the new configuration to default values
     */
    new = def_cfg;

    /*
     * parse each line
     */
    buf[MAXLINE - 1] = '\0';
    buf[MAXLINE] = '\0';
    linenum = 0;
    while (fgets(buf, MAXLINE, f) != NULL) {
	char *fld1;	/* start if 1st field */
	char *fld2;	/* start if 2nd field */
	char *p;

	/* count line */
	++linenum;

	/* lines that are too long */
	if (buf[MAXLINE - 1] != '\0' && buf[MAXLINE - 1] != '\n') {
	    fclose(f);
	    free_cfg_random(&new);
	    return -1;		/* line too long */
	}

	/* ignore lines that begin with # */
	if (buf[0] == '#') {
	    continue;
	}

	/* ignore leading whitespace */
	fld1 = buf + strspn(buf, " \t");

	/* ignore empty lines */
	if (fld1[0] == '\0' || fld1[0] == '\n') {
	    continue;
	}

	/* must have an =, the 1st which splits the two fields */
	p = strchr(fld1, '=');
	if (p == NULL) {
	    fclose(f);
	    free_cfg_random(&new);
	    return -1;
	}
	*p = '\0';
	fld2 = p + 1;

	/* trim trailing whitespace from the end of the 1st field */
	for (--p; p > fld1; --p) {
	    if (!isascii(*p) || !isspace(*p)) {
		break;
	    }
	}
	if (p <= fld1) {
	    fclose(f);
	    free_cfg_random(&new);
	    return -1;
	}
	*(p + 1) = '\0';

	/* trim leading whitespace from the start of the 2nd field */
	fld2 += strspn(fld2, " \t");

	/* trim trailing whitespace from end of the 2nd field */
	for (p = fld2 + strlen(fld2) - 1; p > fld2; --p) {
	    if (!isascii(*p) || !isspace(*p)) {
		break;
	    }
	}
	*(p + 1) = '\0';

	/*
	 * we now have two fields, match on the 1st field
	 *
	 * This is not the most efficient way to parse, but it does
	 * the job for as little as we need to do this.
	 */
	if (strcmp(fld1, "lavapool") == 0) {
	    free_cfg_random(&new);
	    new.lavapool = strdup(fld2);
	    if (new.lavapool == NULL) {
		fclose(f);
		return -1;
	    }
	    new.malloced_str = 1;
	} else if (strcmp(fld1, "maxrequest") == 0) {
	    errno = 0;
	    new.maxrequest = strtoul(fld2, NULL, 0);
	    if (errno == ERANGE || new.maxrequest < 1) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    } else if (new.maxrequest > LAVA_MAX_LEN_ARG) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    }
	} else if (strcmp(fld1, "exit_retries") == 0) {
	    errno = 0;
	    new.exit_retries = strtol(fld2, NULL, 0);
	    if (errno == ERANGE || new.exit_retries < 0) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    }
	} else if (strcmp(fld1, "exit_min_wait") == 0) {
	    errno = 0;
	    new.exit_min_wait = strtod(fld2, NULL);
	    if (errno == ERANGE || new.exit_min_wait <= 0.0) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    }
	} else if (strcmp(fld1, "exit_max_wait") == 0) {
	    errno = 0;
	    new.exit_max_wait = strtod(fld2, NULL);
	    if (errno == ERANGE || new.exit_max_wait <= 0.0) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    }
	} else if (strcmp(fld1, "retry_min_wait") == 0) {
	    errno = 0;
	    new.retry_min_wait = strtod(fld2, NULL);
	    if (errno == ERANGE || new.retry_min_wait <= 0.0) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    }
	} else if (strcmp(fld1, "retry_inc_wait") == 0) {
	    errno = 0;
	    new.retry_inc_wait = strtod(fld2, NULL);
	    if (errno == ERANGE || new.retry_inc_wait < 0.0) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    }
	} else if (strcmp(fld1, "retry_max_wait") == 0) {
	    errno = 0;
	    new.retry_max_wait = strtod(fld2, NULL);
	    if (errno == ERANGE || new.retry_max_wait <= 0.0) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    }
	} else if (strcmp(fld1, "return_retries") == 0) {
	    errno = 0;
	    new.return_retries = strtol(fld2, NULL, 0);
	    if (errno == ERANGE || new.return_retries < 0) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    }
	} else if (strcmp(fld1, "return_min_wait") == 0) {
	    errno = 0;
	    new.return_min_wait = strtod(fld2, NULL);
	    if (errno == ERANGE || new.return_min_wait <= 0.0) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    }
	} else if (strcmp(fld1, "return_max_wait") == 0) {
	    errno = 0;
	    new.return_max_wait = strtod(fld2, NULL);
	    if (errno == ERANGE || new.return_max_wait <= 0.0) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    }
	} else if (strcmp(fld1, "s100_retries") == 0) {
	    errno = 0;
	    new.s100_retries = strtol(fld2, NULL, 0);
	    if (errno == ERANGE || new.s100_retries < 0) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    }
	} else if (strcmp(fld1, "s100_preseed_amt") == 0) {
	    errno = 0;
	    new.s100_preseed_amt = strtol(fld2, NULL, 0);
	    if (errno == ERANGE || new.s100_preseed_amt < -1) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    }
	} else if (strcmp(fld1, "s100_min_wait") == 0) {
	    errno = 0;
	    new.s100_min_wait = strtod(fld2, NULL);
	    if (errno == ERANGE || new.s100_min_wait <= 0.0) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    }
	} else if (strcmp(fld1, "s100_max_wait") == 0) {
	    errno = 0;
	    new.s100_max_wait = strtod(fld2, NULL);
	    if (errno == ERANGE || new.s100_max_wait <= 0.0) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    }
	} else if (strcmp(fld1, "def_callback_wait") == 0) {
	    errno = 0;
	    new.def_callback_wait = strtod(fld2, NULL);
	    if (errno == ERANGE || new.def_callback_wait <= 0.0) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    }
	} else if (strcmp(fld1, "preload_wait") == 0) {
	    errno = 0;
	    new.preload_wait = strtod(fld2, NULL);
	    if (errno == ERANGE || new.preload_wait <= 0.0) {
		fclose(f);
		free_cfg_random(&new);
		return -1;
	    }
	} else {
	    fclose(f);
	    free_cfg_random(&new);
	    return -1;
	}
    }
    if (!feof(f)) {
	fclose(f);
	free_cfg_random(&new);
	return -1;		/* I/O error */
    }
    fclose(f);

    /*
     * perform other sanity checks on parsed values
     */
    if (new.exit_max_wait <= new.exit_min_wait ||
	new.return_max_wait <= new.return_min_wait ||
	new.s100_max_wait <= new.s100_min_wait) {
	free_cfg_random(&new);
	return -1;
    }

    /*
     * we have a valid configuration, load its duplicate into 2nd arg and return
     */
    if (dup_lavapool_cfg(&new, config) == NULL) {
	free_cfg_random(&new);
	return -1;
    }
    /* preload the need count in case we use private s100 generator */
    octets_to_preseed = config->s100_preseed_amt;
    free_cfg_random(&new);
    return 0;			/* success */
}


/*
 * dup_lavapool_cfg - duplicate a configuration
 *
 * given:
 *      cfg1    pointer to configuration to duplicate
 *      cfg2    pointer to where to place duplicated information
 *
 * returns:
 *      cfg2 if OK or NULL if error
 *
 * This function takes care of internal malloc-ing.
 *
 * NOTE: Call free_lavapool_cfg() when the configuration is no longer needed.
 *
 * NOTE: Even if this function returns NULL, it will copy all but the
 *       lavapool element string, provided that this function was
 *       given non-NULL args.
 */
static struct cfg_random *
dup_lavapool_cfg(struct cfg_random *cfg1, struct cfg_random *cfg2)
{
    struct cfg_random new;	/* newly configuration */

    /* firewall */
    if (cfg1 == NULL || cfg2 == NULL) {
	return NULL;
    }

    /*
     * duplicate non-strings
     */
    new = *cfg1;

    /*
     * duplicate strings
     */
    new.lavapool = (cfg1->lavapool ? strdup(cfg1->lavapool) : NULL);
    new.malloced_str = ((new.lavapool == NULL) ? 0 : 1);
    free_cfg_random(cfg2);
    *cfg2 = new;
    if (cfg1->lavapool != NULL && new.lavapool == NULL) {
	return NULL;
    }

    /*
     * return duplication
     */
    return cfg2;
}


/*
 * free_cfg_random - free the name of lavapool socket if it was malloced
 *
 * given:
 *	pointer to a LavaRnd configuration
 */
static void
free_cfg_random(struct cfg_random *config)
{
    /*
     * firewall
     */
    if (config == NULL) {
	return;
    }

    /*
     * free string if it was previously malloced
     */
    if (config->malloced_str && config->lavapool != NULL) {
	free(config->lavapool);
	config->lavapool = NULL;
    }
    config->malloced_str = 0;
}


/*
 * fetchlava_cleanup - cleanup to make memory leak checkers happy
 *
 * NOTE: This function is called from the dormant() function.
 */
void
fetchlava_cleanup(void)
{
    /* free name of lavapool socket if it was malloced */
    free_cfg_random(&cfg_random);

    /* free internal lavabuf */
    if (lavabuf.start != NULL) {
	free(lavabuf.start);
	lavabuf.start = NULL;
    }
    lavabuf.avail = 0;
    lavabuf.size = MIN_BUF_SIZE;

    /* declare ourselves no longer configured */
    is_cfg = FALSE;
}
