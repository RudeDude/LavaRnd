/*
 * simple_url - simple / naive URL to content retriever
 *
 * The URL content fetching is simple / naive URL in that it only deals
 * with successful 200 codes and not server redirects.
 *
 * @(#) $Revision: 10.2 $
 * @(#) $Id: simple_url.c,v 10.2 2003/11/09 22:37:03 lavarnd Exp $
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


#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "LavaRnd/lavaerr.h"
#include "LavaRnd/rawio.h"

#include "lava_retry.h"
#include "simple_url.h"
#include "dbg.h"

#if defined(DMALLOC)
#  include <dmalloc.h>
#endif


/*
 * static internal state
 *
 * Buffers and sockets needed to perform the simple LavaRnd operation.
 * Part of the simple / naive nature of this code is that there is
 * a single instance of this data.  But that is all that is really
 * needed here anyway ...
 *
 * NOTE: The lava_url_content pointer should not be changed by an
 *       external function.  If non-NULL it will point to malloced
 *       data.  However this data should not be freed directly by
 *       an external function.  Instead lava_url_fullcleanup()
 *       should be called to free this (and other internal) storage.
 *
 * NOTE: The con_len counts the offset as well as data read form the URL.
 */
#define LAVA_IO_SIZ (65536)	/* LavaRnd to web URL read size */
#define LAVA_INIT_SIZE (LAVA_IO_SIZ*8)	/* LavaRnd to web buffer size */
#define LAVA_WWW_OK "200"	/* HTTP all OK code */
#define LAVA_MAX_SIZE (16*1024*1024)	/* maximum URL buffer size */
 /**/ u_int8_t * lava_url_content = NULL;	/* static URL content buffer */
static int con_maxlen = 0;	/* malloced length of lava_url_content */
static int con_len = 0;	/* data found in lava_url_content */
static char *url_cpy = NULL;	/* saved copy of URL */
static char *host_cpy = NULL;	/* malloced hostname:port */
static char *path_cpy = NULL;	/* malloced /path part */
static char *get_url = NULL;	/* malloced GET request */
static int get_url_len = 0;	/* length of get_url */
 /**/ static struct lava_retry no_retry = {
    0.0, 0.0, 0, 0.0, 0.0
};

/*
 * static functions
 */
static int lava_parse_url(char *path);
static void lava_url_tidy(int fd);
static int lava_raw_get_url(char *url, u_int32_t offset, int chk_ring);


/*
 * lava_parse_url - parse an http://hostname[:port]/path host:port and path
 *
 * given:
 *      url             http://hostname[:port]/path
 *
 * returns:
 *      0 ==> parse OK
 *      <0 ==> parse error
 *
 *      updates *host_cpy       pointer to a malloced hostname:port
 *      updates *path_cpy       pointer to a malloced /path
 *      updated *get_pcy        pointer to a GET request
 *
 * NOTE: Will free host_cpy, get_url path_cpy beforehand if they are non-NULL.
 */
static int
lava_parse_url(char *path)
{
    int path_cpy_len;	/* string length of the path used */
    char *p;
    char *q;

    /*
     * firewall
     */
    if (path == NULL || *path == '\0') {
	return LAVAERR_BADARG;
    }

    /*
     * case: if already parsed this path, use previous result
     */
    if (host_cpy != NULL && path_cpy != NULL && url_cpy != NULL &&
	strcmp(path, url_cpy) == 0) {
	/* path is identical to previous path, use stored results */
	return 0;
    }

    /*
     * free previous URL parse data
     */
    if (host_cpy != NULL) {
	free(host_cpy);
	host_cpy = NULL;
    }
    if (path_cpy != NULL) {
	free(path_cpy);
	path_cpy = NULL;
    }
    if (get_url != NULL) {
	free(get_url);
	get_url = NULL;
    }

    /*
     * find the hostname
     */
    if (strncmp(path, "http://", sizeof("http://") - 1) == 0) {
	p = path + sizeof("http://") - 1;
    } else {
	return LAVAERR_BADADDR;
    }

    /*
     * find the path
     */
    q = strchr(p, '/');

    /*
     * case: no path, just the hostname
     */
    if (q == NULL) {

	/*
	 * the URL is just http://hostname
	 */
	host_cpy = (char *)malloc(strlen(p) + sizeof(":80"));
	if (host_cpy == NULL) {
	    return LAVAERR_MALLOC;
	}
	strcpy(host_cpy, p);
	if (strchr(host_cpy, ':') == NULL) {
	    strcat(host_cpy, ":80");
	}

	/*
	 * assume just a / path
	 */
	path_cpy = strdup("/");
	if (path_cpy == NULL) {
	    free(host_cpy);
	    host_cpy = NULL;
	    return LAVAERR_MALLOC;
	}
	path_cpy_len = 1;

	/*
	 * malloc a new copy of hostname
	 *
	 * We add extra room for a :80 of the hostname does not have a :port
	 * on it.
	 */
    } else {

	host_cpy = (char *)malloc(q - p + sizeof(":80"));
	if (host_cpy == NULL) {
	    return LAVAERR_MALLOC;
	}
	strncpy(host_cpy, p, q - p);
	host_cpy[q - p] = '\0';
	if (strchr(host_cpy, ':') == NULL) {
	    strcat(&(host_cpy[q - p]), ":80");
	}

	/*
	 * malloc a new path
	 */
	path_cpy = strdup(q);
	if (path_cpy == NULL) {
	    free(host_cpy);
	    host_cpy = NULL;
	    return LAVAERR_MALLOC;
	}
	path_cpy_len = strlen(path_cpy);
    }

    /*
     * save the GET request based on the path we parsed
     *
     * We form a request of the form:
     *
     *      GET path HTTP/1.0\n\n
     */
    get_url_len = sizeof("GET  HTTP/1.0\r\n\r\n")-1 + path_cpy_len + 1;
    get_url = (char *)calloc(sizeof(char), get_url_len+1);
    if (get_url == NULL) {
	free(host_cpy);
	host_cpy = NULL;
	free(path_cpy);
	path_cpy = NULL;
	return LAVAERR_MALLOC;
    }
    strcpy(get_url, "GET ");
    strncat(get_url, path_cpy, path_cpy_len);
    get_url[sizeof("GET ") + path_cpy_len - 1] = '\0';
    strcat(get_url, " HTTP/1.0\r\n\r\n");
    get_url[get_url_len] = '\0';

    /*
     * save the URL that we parsed
     */
    url_cpy = strdup(path);
    if (url_cpy == NULL) {
	free(host_cpy);
	host_cpy = NULL;
	free(path_cpy);
	path_cpy = NULL;
	free(get_url);
	get_url = NULL;
	return LAVAERR_MALLOC;
    }

    /*
     * return success
     */
    return LAVAERR_OK;
}


/*
 * lava_url_tidy - tidy up some of the internal buffers
 *
 * given:
 *      fd      open socket of >= 0
 */
static void
lava_url_tidy(int fd)
{
    if (fd >= 0) {
	shutdown(fd, SHUT_RDWR);
	close(fd);
    }
    return;
}


/*
 * lava_url_cleanup - cleanup all url related buffers
 */
void
lava_url_cleanup(void)
{
    /*
     * free the malloced buffers not cleaned up by lava_url_tidy()
     */
    if (host_cpy != NULL) {
	free(host_cpy);
	host_cpy = NULL;
    }
    if (path_cpy != NULL) {
	free(path_cpy);
	path_cpy = NULL;
    }
    if (url_cpy != NULL) {
	free(url_cpy);
	url_cpy = NULL;
    }
    if (get_url != NULL) {
	free(get_url);
	get_url = NULL;
    }
    if (lava_url_content != NULL) {
	free(lava_url_content);
	lava_url_content = NULL;
    }
    con_maxlen = 0;
}


/*
 * lava_raw_get_url - place the contents of a URL into lava_url_content buffer
 *
 * given:
 *      url             the URL to fetch
 *      offset          offset within lava_url_content buffer to load
 *      chk_alarm       TRUE ==> check lava_ring, FALSE ==> ignore lava_ring
 *
 * returns:
 *      number of octets read from the URL or <0 ==> error
 *
 * NOTE: This routine is coded for the general common case.  It does not
 *       understand a re-direct.
 *
 * NOTE: While con_len includes the length of the offset data as well
 *       as the URL data read, the return value of this function is
 *       just the number of octets returned from the URL.  I.e.,
 *       the return value does not include the offset.
 */
static int
lava_raw_get_url(char *url, u_int32_t offset, int chk_ring)
{
    int ret;	/* function or system call status */
    int fd;	/* socket descriptor */
    char *p;
    int i;

    /*
     * parse the url
     */
    ret = lava_parse_url(url);
    if (ret < 0) {
	/* bad or non-http:// URL */
	return ret;
    }

    /*
     * open a socket to the host
     */
    dbg(5, "lava_raw_get_url", "opening socket to %s", host_cpy);
    fd = lava_connect(host_cpy);
    if (fd < 0) {
	/* failed to connect to web server */
	lava_url_tidy(fd);
	return fd;
    }
    if (chk_ring && lava_ring) {
	/* alarm timeout */
	lava_url_tidy(fd);
	return LAVAERR_TIMEOUT;
    }

    /*
     * write the HTTP/1.0 request
     */
    dbg(5, "lava_raw_get_url", "writing GET %s HTTP/1.0", path_cpy);
    ret = raw_write(fd, get_url, get_url_len, chk_ring);
    if (chk_ring && lava_ring) {
	/* alarm timeout */
	lava_url_tidy(fd);
	return LAVAERR_TIMEOUT;
    }
    if (ret != get_url_len) {
	/* failed to send HTTP request to web server */
	lava_url_tidy(fd);
	return LAVAERR_IOERR;
    }

    /*
     * ensure that we have a buffer
     */
    if (lava_url_content == NULL || con_maxlen <= 0) {
	lava_url_content = (char *)malloc(LAVA_INIT_SIZE + offset);
	if (lava_url_content == NULL) {
	    /* out of memory */
	    lava_url_tidy(fd);
	    return LAVAERR_MALLOC;
	}
	con_maxlen = LAVA_INIT_SIZE + offset;
    }

    /*
     * read the reply until EOF
     *
     * We will read BUFSIZ octets at a time until EOF.  We will expand
     * the buffer (after initially allocating it) as needed.
     */
    con_len = offset;
    dbg(5, "lava_raw_get_url", "reading URL reply");
    do {

	/*
	 * ensure that we have another buffer's worth of space
	 */
	while (con_len + LAVA_IO_SIZ > con_maxlen &&
	       con_maxlen <= LAVA_MAX_SIZE) {

	    /* expand the allocated size of the buffer */
	    lava_url_content =
	      (char *)realloc(lava_url_content, con_maxlen * 2);
	    if (lava_url_content == NULL) {
		/* out of memory */
		lava_url_tidy(fd);
		return LAVAERR_MALLOC;
	    }
	    con_maxlen *= 2;
	}
	if (con_len + LAVA_IO_SIZ > con_maxlen) {
	    /* buffer is at max size, stop reading */
	    break;
	}

	/* read in more data */
	ret = raw_read(fd, lava_url_content + con_len, LAVA_IO_SIZ, chk_ring);
	if (ret > 0) {
	    con_len += ret;
	}
	if (chk_ring && lava_ring) {
	    /* alarm timeout */
	    lava_url_tidy(fd);
	    return LAVAERR_TIMEOUT;
	}
    } while (ret > 0);

    /*
     * close down socket and ensure that all is well
     */
    lava_url_tidy(fd);

    /*
     * ensure that the URL content is NUL terminated
     */
    if (con_len < con_maxlen) {
	lava_url_content[con_len] = '\0';
    } else {
	lava_url_content[con_maxlen - 1] = '\0';
    }

    /*
     * look for the http result code
     *
     * A successful reply will start with:
     *
     *          HTTP/x.y 200 OK\r\n
     *
     * We will skip to the first whitespace and look at the code.
     */
    if (con_len - offset > 0) {

	/* find the end of the 1st token */
	i = strcspn(lava_url_content + offset, " \t\r\n");
	if (i == 0) {
	    /* missing 1st token end */
	    return LAVAERR_BADDATA;
	}
	p = lava_url_content + offset + i;
	if (*p == '\r' || *p == '\n') {
	    /* only 1 token on 1st line */
	    return LAVAERR_BADDATA;
	}

	/* the next token should be the "200" code */
	++p;
	if (strncmp(p, LAVA_WWW_OK, sizeof(LAVA_WWW_OK) - 1) != 0) {
	    /* web server did not return a 200 OK code */
	    return LAVAERR_BADDATA;
	}
	p += sizeof(LAVA_WWW_OK) - 1;
	if (!isascii(*p) || !isspace(*p)) {
	    /* web server started '200', but was '200foo', not just '200' */
	    return LAVAERR_BADDATA;
	}
    }

    /*
     * we have contents of the URL, return length
     */
    dbg(3, "lava_raw_get_url", "prefix: %d  url get: %d  ret len: %d",
	offset, con_len - offset, con_len);
    return con_len - offset;
}


/*
 * lava_get_url - place the contents of a URL into lava_url_content
 *
 * Try to obtain the contents of a URL and place it into a buffer
 * pointed to by the global pointer lava_url_content.
 *
 * given:
 *      url             the URL to fetch
 *      minlen          minimum URL length that is acceptable, 0 ==> any
 *      lava_retry      how to retry, how often and how frequently, NULL==>none
 *
 * returns:
 *      number of chars returned into the internal buffer or <0 ==> error
 *
 * NOTE: This routine is coded for the general common case.  It does not
 *       understand a URL re-direct for example.
 *
 * NOTE: The URL contents is placed into a malloced buffer.  This buffer
 *       should be freed by calling lava_url_fullcleanup() instead of
 *       being freed directly.
 *
 * NOTE: The lava_url_content pointer should not be changed by an
 *       external function.
 */
int
lava_get_url(char *url, int32_t minlen, struct lava_retry *lava_retry)
{
    double pauseval = 0.0;	/* time to pause between retries */
    double timeoutval = 0.0;	/* timeout of an operation */
    int try;	/* url fetch try */
    int ret;	/* function or system call status */

    /*
     * firewall
     */
    if (url == NULL) {
	return LAVAERR_BADARG;
    }
    if (lava_retry == NULL) {
	lava_retry = &no_retry;
    }
    if (lava_retry->timeout < 0.0) {
	lava_retry->timeout = 0.0;
    }
    if (lava_retry->timeout_grow < 0.0) {
	lava_retry->timeout_grow = 0.0;
    }
    if (lava_retry->retry < 0) {
	lava_retry->retry = 0;
    }
    if (lava_retry->pause < 0.0) {
	lava_retry->pause = 0.0;
    }
    if (lava_retry->pause_grow < 0.0) {
	lava_retry->pause_grow = 0.0;
    }
    pauseval = lava_retry->pause;
    timeoutval = lava_retry->timeout;

    /*
     * try to get the URL
     */
    try = 0;
    do {

	/*
	 * pause between retries
	 */
	if (try > 0) {
	    pauseval *= lava_retry->pause_grow;
	    if (pauseval > LAVA_MAX_PAUSE) {
		pauseval = LAVA_MAX_PAUSE;
	    }
	    timeoutval *= lava_retry->timeout_grow;
	    if (timeoutval > LAVA_MAX_PAUSE) {
		timeoutval = LAVA_MAX_PAUSE;
	    }
	    if (pauseval > 0.0) {
		dbg(3, "lava_get_url", "pausing %.3f sec", pauseval);
		lava_sleep(pauseval);
	    }
	}

	/*
	 * get the URL contents
	 */
	if (timeoutval > 0.0) {
	    set_simple_alarm(timeoutval);
	    ret = lava_raw_get_url(url, 0, TRUE);
	    if (clear_simple_alarm()) {
		ret = LAVAERR_TIMEOUT;
		dbg(3, "lava_get_url",
		    "URL timeout after %.3f sec", timeoutval);
	    }
	} else {
	    ret = lava_raw_get_url(url, 0, FALSE);
	}
	++try;

    } while (try < lava_retry->retry && (ret < 0 || ret < minlen));
    if (ret < 0) {
	return ret;
    }
    if (lava_url_content == NULL) {
	return LAVAERR_IMPOSSIBLE;
    }

    /*
     * check the result for return too small
     */
    if (ret >= 0 && ret < minlen) {
	ret = LAVAERR_TINYDATA;
    }
    return ret;
}
