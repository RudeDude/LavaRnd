/*
 * lava_debug - lavapool library debugging, warnings and errors
 *
 * Debug is controlled by the environment variable $LAVA_DEBUG.
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: lava_debug.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include "LavaRnd/have/have_sys_time.h"
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#include "LavaRnd/rawio.h"
#include "LavaRnd/lavaquality.h"
#include "LavaRnd/lava_debug.h"
#include "LavaRnd/lavaerr.h"
#include "LavaRnd/lava_callback.h"

#define MIN_UID 100	/* uid must be >= to debug to file */

#define LAVA_USE_SYSLOG 1	/* use syslog() to report messages */
#define LAVA_USE_STDERR 2	/* print log messages to stderr */
#define LAVA_USE_FILE 3		/* append log messages to a file */

#define HEX_LINE 16	/* octets printed in hex per line */

#if defined(DMALLOC)
#include <dmalloc.h>
#endif


/*
 * LavaRnd_errno - most recent LavaRnd error code
 *
 * See the comment in random.h for details about this value.
 */
extern int lavarnd_errno;


/*
 * debug state
 */
static int lava_debug_init = FALSE;	/* debug already initialized */
int lava_debug_enabled = FALSE;	/* TRUE ==> some debugging was enabled */
int lava_syslog = FALSE;	/* TRUE ==> log via syslog */
static FILE *lava_log = NULL;	/* file to log, or stderr */
int have_lava_debug_a = FALSE;	/* enable almost all debugging (except a,A,h) */
int have_lava_debug_A = FALSE;	/* enable all debugging, including h */

int have_lava_debug_b = FALSE;	/* display lavapool buffering activity */
int have_lava_debug_c = FALSE;	/* display callback activity */
int have_lava_debug_e = FALSE;	/* display errors */
int have_lava_debug_f = FALSE;	/* print file, line, funct with other debug */
int have_lava_debug_h = FALSE;	/* display random data returned as hex */
int have_lava_debug_i = FALSE;	/* display init & configuration information */
int have_lava_debug_l = FALSE;	/* display setting of lavarnd_errno */
int have_lava_debug_p = FALSE;	/* display lavapool activity */
int have_lava_debug_q = FALSE;	/* display quality of random data */
int have_lava_debug_r = FALSE;	/* display amount random data returned */
int have_lava_debug_s = FALSE;	/* display external & internal service calls */
int have_lava_debug_t = FALSE;	/* print time with other debug messages */
int have_lava_debug_v = FALSE;	/* display callback argument value */
int have_lava_debug_z = FALSE;	/* display sleeping between retries */

#if defined(LAVA_DEBUG)
/*
 * usage string
 */
static char *usage =
"The environment variable $LAVA_DEBUG must be of the form:\n"
"\n"
"    flags:type[=option]\n"
"\n"
"where flags is one or more of:\n"
"\n"
"    0\tdisplay this usage message and exit\n"
"\n"
"    a\tenable all debugging except f, h, and t\n"
"    A\tenable all debugging, including f, h and t\n"
"    b\tdisplay lavapool buffering activity\n"
"    c\tdisplay random calls and callback activity\n"
"    e\tdisplay errors and warnings\n"
"    f\tprint calling file, line and function with debug messages\n"
"    h\tdisplay random data returned as hex strings\n"
"    i\tdisplay initialization and configuration information\n"
"    l\tdisplay setting of lavarnd_errno\n"
"    p\tdisplay lavapool activity\n"
"    q\tdisplay current quality level\n"
"    r\tdisplay amount of returned data\n"
"    s\tdisplay external & internal service calls\n"
"    t\tprint time with debug messages\n"
"    v\tdisplay callback argument value\n"
"    z\tdisplay sleeps\n"
"\n"
"and type is either:\n"
"\n"
"    syslog=name\tsend messages to syslog, tag with name_lavalib\n"
"    stderr\t\tsend messages to stderr\n"
"    file=filename\tappend messages to the file 'filename' if uid>=MIN_UID\n"
"\n"
"or $LAVA_DEBUG must be empty or missing to disable all debugging.\n";
#endif /* LAVA_DEBUG */


/*
 * debug_msg - print a debug message
 *
 * given:
 *	file	file name of caller
 *	line	line number of caller
 *	func	name of calling function
 *	fmt	printf format of message
 */
void
lava_debug_msg(char *file, int line, char *func, char *fmt, ...)
{
    va_list ap;			/* argument pointer */
    char newfmt[BUFSIZ+1];	/* syslog with leading tag */
    double now;			/* now as a double */

    /* firewall */
    if (file == NULL) {
	file = "<<NULL file>>";
    }
    if (func == NULL) {
	func = "<<NULL function>>";
    }
    if (fmt == NULL) {
	fmt = "<<NULL format>>";
    }

    /* start the var arg setup and fetch our first arg */
    va_start(ap, fmt);

    /*
     * case: print debug message with time
     */
    if (have_lava_debug_t == TRUE) {

	/*
	 * get the time now
	 */
	now = right_now();

	/*
	 * case: print with file and time
	 */
	if (have_lava_debug_f == TRUE) {
	    if (lava_syslog == TRUE) {
		snprintf(newfmt, BUFSIZ, "lava_debug(%s:%d %s): %.6f: %s",
			 file, line, func, now, fmt);
		syslog(LOG_DEBUG, newfmt, ap);
	    } else if (lava_log != NULL) {
		fprintf(lava_log, "lava_debug(%s:%d %s): %.6f: ",
			file, line, func, now);
		vfprintf(lava_log, fmt, ap);
		fputc('\n', lava_log);
		fflush(lava_log);
	    } else {
		fprintf(stderr, "lava_debug(%s:%d %s): %.6f: ",
			file, line, func, now);
		vfprintf(stderr, fmt, ap);
		fputc('\n', stderr);
		fflush(stderr);
	    }

	/*
	 * case: print without file but with time
	 */
	} else {
	    if (lava_syslog == TRUE) {
		snprintf(newfmt, BUFSIZ, "lava_debug: %.6f: %s", now, fmt);
		syslog(LOG_DEBUG, newfmt, ap);
	    } else if (lava_log != NULL) {
		fprintf(lava_log, "lava_debug: %.6f: ", now);
		vfprintf(lava_log, fmt, ap);
		fputc('\n', lava_log);
		fflush(lava_log);
	    } else {
		fprintf(stderr, "lava_debug: %.6f: ", now);
		vfprintf(stderr, fmt, ap);
		fputc('\n', stderr);
		fflush(stderr);
	    }
	}

    /*
     * case: print debug message without time
     */
    } else {

	/*
	 * case: print with file and no time
	 */
	if (have_lava_debug_f == TRUE) {
	    if (lava_syslog == TRUE) {
		snprintf(newfmt, BUFSIZ, "lava_debug(%s:%d %s): %s",
			 file, line, func, fmt);
		syslog(LOG_DEBUG, newfmt, ap);
	    } else if (lava_log != NULL) {
		fprintf(lava_log, "lava_debug(%s:%d %s): ", file, line, func);
		vfprintf(lava_log, fmt, ap);
		fputc('\n', lava_log);
		fflush(lava_log);
	    } else {
		fprintf(stderr, "lava_debug(%s:%d %s): ", file, line, func);
		vfprintf(stderr, fmt, ap);
		fputc('\n', stderr);
		fflush(stderr);
	    }

	/*
	 * case: print without file and without time
	 */
	} else {
	    if (lava_syslog == TRUE) {
		snprintf(newfmt, BUFSIZ, "lava_debug: %s", fmt);
		syslog(LOG_DEBUG, newfmt, ap);
	    } else if (lava_log != NULL) {
		fprintf(lava_log, "lava_debug: ");
		vfprintf(lava_log, fmt, ap);
		fputc('\n', lava_log);
		fflush(lava_log);
	    } else {
		fprintf(stderr, "lava_debug: ");
		vfprintf(stderr, fmt, ap);
		fputc('\n', stderr);
		fflush(stderr);
	    }
	}
    }
    return;
}


#if defined(LAVA_DEBUG)
/*
 * debug_usage - print a $LAVA_DEBUG usage message on stderr
 */
static void
debug_usage(void)
{
    fprintf(stderr, "%s", usage);
    return;
}


/*
 * lava_open_syslog - open a syslog channel for logging
 *
 * given:
 *	name	name to log with
 */
static void
lava_open_syslog(char *name)
{
    /*
     * firewall
     */
    if (name == NULL) {
	name = "<<NO_NAME>>";
    }

    /*
     * open the syslog interface
     */
    openlog(name, LOG_CONS|LOG_PID, LOG_LOCAL5);
    lava_syslog = TRUE;
    lava_debug_msg(__FILE__, __LINE__, "lava_open_syslog",
		   "opening syslog channel");
}


/*
 * lava_open_logfile - open a file for appending
 *
 * given:
 *	name	name to log file to append
 *
 * NOTE: This function assumes the caller as checked for low uid.
 */
static void
lava_open_logfile(char *name)
{
    /*
     * firewall - NULL assumes stderr
     */
    if (name == NULL) {
	lava_log = stderr;
	lava_debug_msg(__FILE__, __LINE__, "lava_open_logfile",
		       "logging to stderr: %s", name);

    /*
     * open the log file for appending
     */
    } else {
	lava_log = fopen(name, "a");
	if (lava_log == NULL) {
	    lava_log = stderr;
	    lava_debug_msg(__FILE__, __LINE__, "lava_open_logfile",
			   "cannot append to log file: %s, using stderr",
			   name);
	} else {
	    lava_debug_msg(__FILE__, __LINE__, "lava_open_logfile",
		    "appending to log file: %s", name);
	}
    }
    return;
}
#endif /* LAVA_DEBUG */


/*
 * lava_init_debug - parse $LAVA_DEBUG
 *
 * This routine will look for a parse the environment variable $LAVA_DEBUG.
 * See usage string above for details.
 *
 * given:
 *	environment variable $LAVA_DEBUG
 *
 * returns:
 *	<0 ==> error (such as LAVAERR_ENV_PARSE or LAVAERR_NO_DEBUG)
 *	0 ==> no debugging was enabled
 *	      $LAVA_DEBUG not found or empty
 *	1 ==> some debugging was enabled
 *
 * NOTE: If $LAVA_DEBUG is missing or empty, no debugging is enabled.
 *
 * NOTE: On a parse error for $LAVA_DEBUG, a usage message is written
 *	 to stderr and all debugging is disabled.
 *
 * NOTE: This function should be called only once.  All subsequent calls
 *	 will be ignored.
 */
int
lava_init_debug(void)
{
#if defined(LAVA_DEBUG)
    int lava_log_type = 0;	/* LAVA_USE_XYZ file, or 0 ==> none */
    char *env_str;		/* $LAVA_DEBUG environment value */
    char *type;			/* type[=option] string */
    char *option;		/* option string */
    char *p;

    /*
     * firewall - may be called only once
     */
    if (lava_debug_init) {
	return 0;
    }
    lava_debug_init = TRUE;

    /*
     * fetch $LAVA_DEBUG
     */
    env_str = getenv("LAVA_DEBUG");

    /*
     * case: no $LAVA_DEBUG or $LAVA_DEBUG is empty
     */
    if (env_str == NULL || env_str[0] == '\0') {
	/* no debugging enabled */
	lava_debug_enabled = FALSE;
	return 0;
    }

    /*
     * case: must have a :
     */
    type = strchr(env_str, ':');
    if (type == NULL) {
	/* parse error */
	lava_debug_enabled = FALSE;
	debug_usage();
	return 0;
    }
    ++type;

    /*
     * parse value after :
     */
    if (strncmp(type, "syslog=", sizeof("syslog=")-1) ==0) {
	option = type + sizeof("syslog=")-1;
	lava_log_type = LAVA_USE_SYSLOG;
    } else if (strcmp(type, "stderr") == 0) {
	option = NULL;
	lava_log_type = LAVA_USE_STDERR;
    } else if (strncmp(type, "file=", sizeof("file=")-1) == 0) {
	if (geteuid() < MIN_UID) {
	    /* uid too low for file logging */
	    lava_debug_enabled = FALSE;
	    debug_usage();
	    fprintf(stderr, "\n\tuid to small to log to a file: %d < %d\n",
	    	    geteuid(), MIN_UID);
	    return 0;
	} else {
	    option = type + sizeof("file=")-1;
	    lava_log_type = LAVA_USE_FILE;
        }
    } else {
	/* parse error */
	lava_debug_enabled = FALSE;
	debug_usage();
	fprintf(stderr, "\n\tunknown type: %s\n", type);
	return 0;
    }

    /*
     * parse flags before :
     */
    for (p=env_str; *p != ':' && *p != '\0' && p<(type-1); ++p) {
    	switch (*p) {
	case 'a':
	    have_lava_debug_a = TRUE;
	    have_lava_debug_b = TRUE;
	    have_lava_debug_c = TRUE;
	    have_lava_debug_e = TRUE;
	    have_lava_debug_p = TRUE;
	    have_lava_debug_i = TRUE;
	    have_lava_debug_l = TRUE;
	    have_lava_debug_q = TRUE;
	    have_lava_debug_r = TRUE;
	    have_lava_debug_s = TRUE;
	    have_lava_debug_v = TRUE;
	    have_lava_debug_z = TRUE;
	    lava_debug_enabled = TRUE;
	    break;
	case 'A':
	    have_lava_debug_a = TRUE;
	    have_lava_debug_A = TRUE;
	    have_lava_debug_b = TRUE;
	    have_lava_debug_c = TRUE;
	    have_lava_debug_e = TRUE;
	    have_lava_debug_f = TRUE;
	    have_lava_debug_h = TRUE;
	    have_lava_debug_i = TRUE;
	    have_lava_debug_l = TRUE;
	    have_lava_debug_p = TRUE;
	    have_lava_debug_q = TRUE;
	    have_lava_debug_r = TRUE;
	    have_lava_debug_s = TRUE;
	    have_lava_debug_t = TRUE;
	    have_lava_debug_v = TRUE;
	    have_lava_debug_z = TRUE;
	    lava_debug_enabled = TRUE;
	    break;
	case 'b':
	    have_lava_debug_b = TRUE;
	    lava_debug_enabled = TRUE;
	    break;
	case 'c':
	    have_lava_debug_c = TRUE;
	    lava_debug_enabled = TRUE;
	    break;
	case 'e':
	    have_lava_debug_e = TRUE;
	    lava_debug_enabled = TRUE;
	    break;
	case 'f':
	    have_lava_debug_f = TRUE;
	    lava_debug_enabled = TRUE;
	    break;
	case 'h':
	    have_lava_debug_h = TRUE;
	    lava_debug_enabled = TRUE;
	    break;
	case 'i':
	    have_lava_debug_i = TRUE;
	    lava_debug_enabled = TRUE;
	    break;
	case 'l':
	    have_lava_debug_l = TRUE;
	    lava_debug_enabled = TRUE;
	    break;
	case 'p':
	    have_lava_debug_p = TRUE;
	    lava_debug_enabled = TRUE;
	    break;
	case 'q':
	    have_lava_debug_q = TRUE;
	    lava_debug_enabled = TRUE;
	    break;
	case 'r':
	    have_lava_debug_r = TRUE;
	    lava_debug_enabled = TRUE;
	case 's':
	    have_lava_debug_s = TRUE;
	    lava_debug_enabled = TRUE;
	    break;
	case 't':
	    have_lava_debug_t = TRUE;
	    lava_debug_enabled = TRUE;
	    break;
	case 'v':
	    have_lava_debug_v = TRUE;
	    lava_debug_enabled = TRUE;
	    break;
	case 'z':
	    have_lava_debug_z = TRUE;
	    lava_debug_enabled = TRUE;
	    break;
	case '0':
	    /* 0 ==> print usage message and exit */
	    debug_usage();
	    exit(0);
	    /*NOTREACHED*/
	default:
	    /* parse error */
	    have_lava_debug_a = FALSE;
	    have_lava_debug_A = FALSE;
	    have_lava_debug_b = FALSE;
	    have_lava_debug_c = FALSE;
	    have_lava_debug_e = FALSE;
	    have_lava_debug_f = FALSE;
	    have_lava_debug_h = FALSE;
	    have_lava_debug_i = FALSE;
	    have_lava_debug_l = FALSE;
	    have_lava_debug_p = FALSE;
	    have_lava_debug_q = FALSE;
	    have_lava_debug_r = FALSE;
	    have_lava_debug_s = FALSE;
	    have_lava_debug_t = FALSE;
	    have_lava_debug_v = FALSE;
	    have_lava_debug_z = FALSE;
	    lava_debug_enabled = FALSE;
	    debug_usage();
	    return 0;
	    break;
	}
    }

    /*
     * process option
     */
    if (lava_log_type == LAVA_USE_SYSLOG) {
	lava_open_syslog(option);
    } else if (lava_log_type == LAVA_USE_FILE) {
	lava_open_logfile(option);
    }

    /*
     * some debug was enabled
     */
    return 1;

    /*
     * If we were compiled without -DLAVA_DEBUG, then the macros
     * within other files in this directory will not operate.
     * So in this case we refuse to initialize debugging and
     * return an error.
     */
#else /* LAVA_DEBUG */
    lava_debug_init = FALSE;
    return LAVAERR_NO_DEBUG;
#endif /* LAVA_DEBUG */
}


/*
 * lava_quality - name of quality level
 *
 * given:
 *	quality		quality level
 *
 * returns:
 *	name of quality level
 */
static char *
lava_quality(lavaqual quality)
{
    switch (quality) {
    case LAVA_QUAL_NONE:
	return "none";
	break;
    case LAVA_QUAL_S100LOW:
	return "low s100";
	break;
    case LAVA_QUAL_S100MED:
	return "med s100";
	break;
    case LAVA_QUAL_S100HIGH:
	return "high s100";
	break;
    case LAVA_QUAL_LAVARND:
	return "LavaRnd";
	break;
    }
    return "invalid quality";
}


/*
 * lava_err_name - name of quality level
 *
 * given:
 *	err		error number
 *
 * returns:
 *	name of error
 */
const char *
lava_err_name(int err)
{
    switch (err) {
    case LAVAERR_OK:
	return "not an error";
    case LAVAERR_MALLOC:
	return "unable to alloc or realloc data";
    case LAVAERR_TIMEOUT:
	return "request did not respond in time";
    case LAVAERR_PARTIAL:
	return "request returned less than maximum data";
    case LAVAERR_BADARG:
	return "invalid argument passed to function";
    case LAVAERR_SOCKERR:
	return "error in creating a socket";
    case LAVAERR_OPENERR:
	return "error opening socket to daemon";
    case LAVAERR_BADADDR:
	return "unknown host, port or missing socket path";
    case LAVAERR_ALARMERR:
	return "unexpected error during alarm set or clear";
    case LAVAERR_IOERR:
	return "error during I/O operation";
    case LAVAERR_BADDATA:
	return "data received was malformed";
    case LAVAERR_TINYDATA:
	return "data received was too small";
    case LAVAERR_IMPOSSIBLE:
	return "an impossible internal error occurred";
    case LAVAERR_NONBLOCK:
	return "I/O operation would block";
    case LAVAERR_BINDERR:
	return "unable to bind socket to address";
    case LAVAERR_CONNERR:
	return "unable to connect socket to address";
    case LAVAERR_LISTENERR:
	return "unable to listen on socket";
    case LAVAERR_ACCEPTERR:
	return "error in accepting new socket connection";
    case LAVAERR_FCNTLERR:
	return "fcntl or ioctl failed";
    case LAVAERR_BADCFG:
	return "failed in lavapool configuration";
    case LAVAERR_POORQUAL:
	return "data quality is below min allowed level";
    case LAVAERR_ENV_PARSE:
	return "parse error on env variable $LAVA_DEBUG";
    case LAVAERR_NO_DEBUG:
	return "library compiled without -DLAVA_DEBUG";
    case LAVAERR_SIGERR:
	return "signal related error";
    case LAVAERR_GETTIME:
	return "unexpected gettimeofday() error";
    case LAVAERR_TOOMUCH:
    	return "requested too much data";
    case LAVAERR_BADLEN:
    	return "requested length invalid";
    case LAVAERR_EOF:
    	return "EOF during I/O operation";
    case LAVAERR_PALSET:
    	return "unknown pallette set";
    case LAVAERR_PALETTE:
    	return "pallette value not valid for pallette set";
    case LAVAERR_PERMOPEN:
    	return "open failed due to file permissions";
    /**/
    case LAVACAM_ERR_ARG:
	return "bad function argument";
    case LAVACAM_ERR_NOCAM:
	return "not a camera or camera is not working";
    case LAVACAM_ERR_IDENT:
	return "could not identify camera name";
    case LAVACAM_ERR_TYPE:
	return "wrong camera type";
    case LAVACAM_ERR_GETPARAM:
	return "could not get a camera setting value";
    case LAVACAM_ERR_SETPARAM:
	return "could not set a camera setting value";
    case LAVACAM_ERR_RANGE:
	return "masked camera setting is out of range";
    case LAVACAM_ERR_OPEN:
	return "failed to open camera";
    case LAVACAM_ERR_CLOSE:
	return "failed to close camera";
    case LAVACAM_ERR_NOSIZE:
	return "unable to determine read/mmap size";
    case LAVACAM_ERR_SYNC:
	return "mmap buffer release error";
    case LAVACAM_ERR_WARMUP:
	return "error during warm-up after open";
    case LAVACAM_ERR_PALETTE:
	return "camera does not support the palette";
    case LAVACAM_ERR_CHAN:
	return "failed to get/set video channel";
    case LAVACAM_ERR_NOREAD:
	return "cannot read, try mmap instead";
    case LAVACAM_ERR_NOMMAP:
	return "cannot mmap, try read instead";
    case LAVACAM_ERR_IOERR:
	return "I/O error";
    case LAVACAM_ERR_FRAME:
	return "frame read turned partial data";
    case LAVACAM_ERR_SIGERR:
	return "signal related error";
    case LAVACAM_ERR_UNCOMMON:
    	return "top_x common values are too common";
    case LAVACAM_ERR_OVERSAME:
    	return "too many bits similar with prev frame";
    case LAVACAM_ERR_OVERDIFF:
    	return "too few bits similar with prev frame";
    case LAVACAM_ERR_HALFLVL:
	return "top half_x is more than half of frame";
    case LAVACAM_ERR_PALUNSET:
	return "pallette has not been set for camera";
    }
    return "unknown error";
}


/*
 * lava_debug_b - display lavapool buffering activity
 *
 * given:
 *	file	filename of call point
 *	line	line number of call point
 *	func	function name of call point
 *	fmt	message to format
 *	int1	1st integer in fmt
 *	int2	2nd integer in fmt
 */
void
lava_debug_b(char *file, int line, char *func, char *fmt, int int1, int int2)
{
    if (lava_debug_enabled == TRUE && have_lava_debug_b == TRUE) {
	lava_debug_msg(file, line, func, fmt, int1, int2);
    }
}


/*
 * lava_debug_c - display random calls and callback activity
 *
 * given:
 *	file	filename of call point
 *	line	line number of call point
 *	func	function name of call point
 *	fmt	message to format
 *	int1	1st integer in fmt
 */
void
lava_debug_c(char *file, int line, char *func, char *fmt, int int1)
{
    if (lava_debug_enabled == TRUE && have_lava_debug_c == TRUE) {
	lava_debug_msg(file, line, func, fmt, int1);
    }
}


/*
 * lava_debug_e - display errors
 *
 * given:
 *	file	filename of call point
 *	line	line number of call point
 *	func	function name of call point
 *	err	error number
 */
void
lava_debug_e(char *file, int line, char *func, int err)
{
    if (lava_debug_enabled == TRUE && have_lava_debug_e == TRUE) {
	lava_debug_msg(file, line, func,
		       "error %d: %s", err, lava_err_name(err));
    }
}


/*
 * lava_debug_h - display random data returned as hex
 *
 * given:
 *	file	filename of call point
 *	line	line number of call point
 *	func	function name of call point
 *	buf	buffer of random data
 *	len	length of buffer in octets
 */
void
lava_debug_h(char *file, int line, char *func, u_int8_t *buf, int len)
{
    int i;

    if (lava_debug_enabled == TRUE && have_lava_debug_h == TRUE) {

	/* firewall */
	if (len <= 0) {
	    lava_debug_msg(file, line, func, "lava_debug_h: len %d <= 0", len);
	} else if (file == NULL) {
	    lava_debug_msg(file, line, func, "lava_debug_h: buf is NULL");
	} else {
	    lava_debug_msg(file, line, func, "%d octets of data follow", len);

	    /* print in sets of 16 */
	    for (i=0; i < len-HEX_LINE+1; i += HEX_LINE) {
		lava_debug_msg(file, line, func,
			"%05x:  "
			"%02x %02x %02x %02x %02x %02x %02x %02x "
			"%02x %02x %02x %02x %02x %02x %02x %02x",
			i,
			buf[i+0], buf[i+1], buf[i+2], buf[i+3],
			buf[i+4], buf[i+5], buf[i+6], buf[i+7],
			       buf[i + 8], buf[i + 9], buf[i + 10],
			       buf[i + 11], buf[i + 12], buf[i + 13],
			       buf[i + 14], buf[i + 15]);
	    }
	    /* print last set - can you say hack? */
	    switch (len-i) {
	    case 16:
		lava_debug_msg(file, line, func,
			"%05x:  "
			"%02x %02x %02x %02x %02x %02x %02x %02x "
			"%02x %02x %02x %02x %02x %02x %02x %02x",
			i,
			buf[i+0], buf[i+1], buf[i+2], buf[i+3],
			buf[i+4], buf[i+5], buf[i+6], buf[i+7],
			       buf[i + 8], buf[i + 9], buf[i + 10],
			       buf[i + 11], buf[i + 12], buf[i + 13],
			       buf[i + 14], buf[i + 15]);
		break;
	    case 15:
		lava_debug_msg(file, line, func,
			"%05x:  "
			"%02x %02x %02x %02x %02x %02x %02x %02x "
			"%02x %02x %02x %02x %02x %02x %02x",
			i,
			buf[i+0], buf[i+1], buf[i+2], buf[i+3],
			buf[i+4], buf[i+5], buf[i+6], buf[i+7],
			       buf[i + 8], buf[i + 9], buf[i + 10],
			       buf[i + 11], buf[i + 12], buf[i + 13],
			       buf[i + 14]);
		break;
	    case 14:
		lava_debug_msg(file, line, func,
			"%05x:  "
			"%02x %02x %02x %02x %02x %02x %02x %02x "
			"%02x %02x %02x %02x %02x %02x",
			i,
			buf[i+0], buf[i+1], buf[i+2], buf[i+3],
			buf[i+4], buf[i+5], buf[i+6], buf[i+7],
			       buf[i + 8], buf[i + 9], buf[i + 10],
			       buf[i + 11], buf[i + 12], buf[i + 13]);
		break;
	    case 13:
		lava_debug_msg(file, line, func,
			"%05x:  "
			"%02x %02x %02x %02x %02x %02x %02x %02x "
			"%02x %02x %02x %02x %02x",
			i,
			buf[i+0], buf[i+1], buf[i+2], buf[i+3],
			buf[i+4], buf[i+5], buf[i+6], buf[i+7],
			       buf[i + 8], buf[i + 9], buf[i + 10],
			       buf[i + 11], buf[i + 12]);
		break;
	    case 12:
		lava_debug_msg(file, line, func,
			"%05x:  "
			"%02x %02x %02x %02x %02x %02x %02x %02x "
			"%02x %02x %02x %02x",
			i,
			buf[i+0], buf[i+1], buf[i+2], buf[i+3],
			buf[i+4], buf[i+5], buf[i+6], buf[i+7],
			       buf[i + 8], buf[i + 9], buf[i + 10],
			       buf[i + 11]);
		break;
	    case 11:
		lava_debug_msg(file, line, func,
			"%05x:  "
			"%02x %02x %02x %02x %02x %02x %02x %02x "
			"%02x %02x %02x",
			i,
			buf[i+0], buf[i+1], buf[i+2], buf[i+3],
			buf[i+4], buf[i+5], buf[i+6], buf[i+7],
			buf[i+8], buf[i+9], buf[i+10]);
		break;
	    case 10:
		lava_debug_msg(file, line, func,
			"%05x:  "
			"%02x %02x %02x %02x %02x %02x %02x %02x "
			"%02x %02x",
			i,
			buf[i+0], buf[i+1], buf[i+2], buf[i+3],
			buf[i+4], buf[i+5], buf[i+6], buf[i+7],
			buf[i+8], buf[i+9]);
		break;
	    case 9:
		lava_debug_msg(file, line, func,
			"%05x:  "
			"%02x %02x %02x %02x %02x %02x %02x %02x "
			"%02x",
			i,
			buf[i+0], buf[i+1], buf[i+2], buf[i+3],
			buf[i+4], buf[i+5], buf[i+6], buf[i+7],
			buf[i+8]);
		break;
	    case 8:
		lava_debug_msg(file, line, func,
			"%05x:  "
			"%02x %02x %02x %02x %02x %02x %02x %02x",
			i,
			buf[i+0], buf[i+1], buf[i+2], buf[i+3],
			buf[i+4], buf[i+5], buf[i+6], buf[i+7]);
		break;
	    case 7:
		lava_debug_msg(file, line, func,
			"%05x:  "
			"%02x %02x %02x %02x %02x %02x %02x",
			i,
			buf[i+0], buf[i+1], buf[i+2], buf[i+3],
			buf[i+4], buf[i+5], buf[i+6]);
		break;
	    case 6:
		lava_debug_msg(file, line, func,
			"%05x:  "
			"%02x %02x %02x %02x %02x %02x",
			i,
			buf[i+0], buf[i+1], buf[i+2], buf[i+3],
			buf[i+4], buf[i+5]);
		break;
	    case 5:
		lava_debug_msg(file, line, func,
			"%05x:  "
			"%02x %02x %02x %02x %02x",
			i,
			buf[i+0], buf[i+1], buf[i+2], buf[i+3],
			buf[i+4]);
		break;
	    case 4:
		lava_debug_msg(file, line, func,
			"%05x:  "
			"%02x %02x %02x %02x",
			i,
			buf[i+0], buf[i+1], buf[i+2], buf[i+3]);
		break;
	    case 3:
		lava_debug_msg(file, line, func,
			"%05x:  "
			"%02x %02x %02x",
			       i, buf[i + 0], buf[i + 1], buf[i + 2]);
		break;
	    case 2:
		lava_debug_msg(file, line, func,
			"%05x:  "
			       "%02x %02x", i, buf[i + 0], buf[i + 1]);
		break;
	    case 1:
		lava_debug_msg(file, line, func,
			       "%05x:  " "%02x", i, buf[i + 0]);
		break;
	    }
	}
    }
}


/*
 * lava_debug_i - display initialization and configuration information
 *
 * given:
 *	file	filename of call point
 *	line	line number of call point
 *	func	function name of call point
 *	fmt	format of message to print
 *	str	string to print
 */
void
lava_debug_i(char *file, int line, char *func, char *fmt, char *str)
{
    if (lava_debug_enabled == TRUE && have_lava_debug_e == TRUE) {
	lava_debug_msg(file, line, func, fmt, str);
    }
}


/*
 * lava_debug_l - display lavarnd_errno value settings
 *
 * given:
 *	file	filename of call point
 *	line	line number of call point
 *	func	function name of call point
 */
void
lava_debug_l(char *file, int line, char *func)
{
    if (lava_debug_enabled == TRUE && have_lava_debug_l == TRUE) {
	lava_debug_msg(file, line, func,
		       "lavarnd_errno %d: %s",
		       lavarnd_errno, lava_err_name(lavarnd_errno));
    }
}


/*
 * lava_debug_p - display lavapool activity
 *
 * given:
 *	file	filename of call point
 *	line	line number of call point
 *	func	function name of call point
 *	fmt	message to format
 *	int1	1st integer in fmt
 */
void
lava_debug_p(char *file, int line, char *func, char *fmt, int int1)
{
    if (lava_debug_enabled == TRUE && have_lava_debug_p == TRUE) {
	lava_debug_msg(file, line, func, fmt, int1);
    }
}


/*
 * lava_debug_q - display quality of random data
 *
 * given:
 *	file	filename of call point
 *	line	line number of call point
 *	func	function name of call point
 *	str	string to print in quality debug output
 *	quality	quality of data returned
 */
void
lava_debug_q(char *file, int line, char *func, char *str, lavaqual quality)
{
    if (lava_debug_enabled == TRUE && have_lava_debug_q == TRUE) {
	if (str == NULL) {
	    str = "<<NULL>>";
	}
	lava_debug_msg(file, line, func,
		       "%s data quality: %s", str, lava_quality(quality));
    }
}


/*
 * lava_debug_r - display amount of data being returned
 *
 * given:
 *	file	filename of call point
 *	line	line number of call point
 *	func	function name of call point
 *	len	amount of data being returned
 */
void
lava_debug_r(char *file, int line, char *func, int len)
{
    if (lava_debug_enabled == TRUE && have_lava_debug_r == TRUE) {
	lava_debug_msg(file, line, func, "octets returned: %d", len);
    }
}


/*
 * lava_debug_s - display external & internal service calls
 *
 * given:
 *	file	filename of call point
 *	line	line number of call point
 *	func	function name of call point
 *	fmt	message to format
 *	int1	1st integer in fmt
 */
void
lava_debug_s(char *file, int line, char *func, char *fmt, int int1)
{
    if (lava_debug_enabled == TRUE && have_lava_debug_s == TRUE) {
	lava_debug_msg(file, line, func, fmt, int1);
    }
}


/*
 * lava_debug_v - display callback argument value
 *
 * given:
 *	file		filename of call point
 *	line		line number of call point
 *	func		function name of call point
 *	callback	callback argument value to display
 */
void
lava_debug_v(char *file, int line, char *func, int callback)
{
    char *name = NULL;		/* name of callback value or NULL */

    if (lava_debug_enabled == TRUE && have_lava_debug_v == TRUE) {

	/*
	 * determine callback argument value name, if possible
	 */
	switch (callback) {
	case CASE_LAVACALL_LAVA_EXIT:
	    name = "LAVACALL_LAVA_EXIT";
	    break;
	case CASE_LAVACALL_LAVA_RETRY:
	    name = "LAVACALL_LAVA_RETRY";
	    break;
	case CASE_LAVACALL_LAVA_RETURN:
	    name = "LAVACALL_LAVA_RETURN";
	    break;
	case CASE_LAVACALL_TRY_HIGH:
	    name = "LAVACALL_TRY_HIGH";
	    break;
	case CASE_LAVACALL_TRY_MED:
	    name = "LAVACALL_TRY_MED";
	    break;
	case CASE_LAVACALL_TRY_ANY:
	    name = "LAVACALL_TRY_ANY";
	    break;
	case CASE_LAVACALL_TRYONCE_HIGH:
	    name = "LAVACALL_TRYONCE_HIGH";
	    break;
	case CASE_LAVACALL_TRYONCE_MED:
	    name = "LAVACALL_TRYONCE_MED";
	    break;
	case CASE_LAVACALL_TRYONCE_ANY:
	    name = "LAVACALL_TRYONCE_ANY";
	    break;
	case CASE_LAVACALL_S100_HIGH:
	    name = "LAVACALL_S100_HIGH";
	    break;
	case CASE_LAVACALL_S100_MED:
	    name = "LAVACALL_S100_MED";
	    break;
	case CASE_LAVACALL_S100_ANY:
	    name = "LAVACALL_S100_ANY";
	    break;
	case CASE_LAVACALL_INVALID:
	    name = "LAVACALL_LAVA_INVALID";
	    break;
	default:
	    name = "unknown_LAVACALL_value";
	    break;
	}

	/*
	 * report callback value
	 */
	if (name == NULL) {
	    lava_debug_msg(file, line, func,
			   "user defined callback: %08x", callback);
	} else {
	    lava_debug_msg(file, line, func, "pre-defined callback: %s", name);
	}
    }
}


/*
 * lava_debug_z - display sleeping between retries
 *
 * given:
 *	file	filename of call point
 *	line	line number of call point
 *	func	function name of call point
 *	dtime	time to sleep
 */
void
lava_debug_z(char *file, int line, char *func, double dtime)
{
    if (lava_debug_enabled == TRUE && have_lava_debug_z == TRUE) {
	lava_debug_msg(file, line, func, "sleeping %.6f sec", dtime);
    }
}
