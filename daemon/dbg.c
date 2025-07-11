/*
 * dbg - lavapool daemon debugging, warnings and errors
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: dbg.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <string.h>

#include "dbg.h"
#include "LavaRnd/rawio.h"

#if defined(DMALLOC)
#  include <dmalloc.h>
#endif


/*
 * static declarations
 */
static int use_syslog = 0;	/* 1 ==> write messages to syslog */
static FILE *log = NULL;	/* if non-NULL, write messages to logfile */
static int use_stderr = 1;	/* 1 ==> write messages to stderr */


/*
 * global declarations
 */
int dbg_lvl = 0;	/* debug level, 0 => none */


/*
 * open_syslog - open up the syslog interface for debug, warnings and errors
 */
void
open_syslog(void)
{
    /*
     * prep syslog interface
     */
    openlog("lavapool", LOG_CONS | LOG_PID, LOG_DAEMON);
    use_syslog = 1;
}


/*
 * unsetlog - close down logging facility
 */
void
unsetlog(void)
{
    /* close down syslog facility */
    if (use_syslog) {
	closelog();
    }

    /* close down log file facility */
    if (log != NULL) {
	fclose(log);
    }
    return;
}


/*
 * setlog - set the log
 *
 * given:
 *      filename        filename to open for appending
 *
 * returns:
 *      TRUE ==> success, FALSE ==> error
 *
 * NOTE: The old log file is not closed.
 *
 * NOTE: On error, the old log is kept in place.  An warning message
 *       will be issued on the old log.
 */
int
setlog(char *filename)
{
    FILE *stream;	/* new log stream */

    /*
     * firewall - NULL means use stderr
     */
    if (filename == NULL) {
	log = NULL;
	return TRUE;
    }

    /*
     * try to open file for appending
     */
    stream = fopen(filename, "a");
    if (stream == NULL) {
	warn("setlog", "failed to set log to %s", filename);
	return FALSE;
    }

    /*
     * complete the log setting
     */
    log = stream;
    dbg(1, "setlog", "set log to: %s", filename);
    return TRUE;
}


/*
 * dont_use_stderr - do not send debug, warnings and errors to stderr
 */
void
dont_use_stderr(void)
{
    use_stderr = 0;
}


/*
 * dbg - print a debug message if the level is high enough
 *
 * given:
 *      level   print a message of the -v level is >= this value
 *      func    name of calling function
 *      fmt     printf format of message
 */
void
dbg(int level, char *func, char *fmt, ...)
{
    va_list ap;	/* argument pointer */
    char *newfmt;	/* syslog with leading tag */
    int newfmtlen;	/* length of newfmt */
    double now = 0.0;	/* current time stamp */

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

    /* firewall */
    if (func == NULL) {
	func = "<<NULL function>>";
    }
    if (fmt == NULL) {
	fmt = "<<NULL format>>";
    }

    /* get the time if output to logfile or srderr */
    if (log != NULL || use_stderr) {
	/* use a private time to avoid interfering with other functions */
	now = right_now();
    }

    /* start the var arg setup and fetch our first arg */
    va_start(ap, fmt);

    /*
     * output header, message and newline
     */
    if (use_syslog) {
	/* output to syslog */
	newfmtlen = strlen(fmt) + sizeof("dbg[999999999]: ") + 1;
	newfmt = malloc(newfmtlen);
	if (newfmt != NULL) {
	    snprintf(newfmt, newfmtlen, "dbg[%d]: %s", level, fmt);
	    syslog(LOG_NOTICE, newfmt, ap);
	    free(newfmt);
	} else {
	    syslog(LOG_NOTICE, fmt, ap);
	}
    }
    if (log != NULL) {
	/* output to logfile */
	fprintf(log, "%17.6f [%d]: %s: ", now, level, func);
	vfprintf(log, fmt, ap);
	fputc('\n', log);
	fflush(log);
    }
    if (use_stderr) {
	/* output to stderr */
	fprintf(stderr, "%17.6f [%d]: %s: ", now, level, func);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	fflush(stderr);
    }
}


/*
 * warn - print a warning message
 *
 * given:
 *      level   print a message of the -v level is >= this value
 *      func    name of calling function
 *      fmt     printf format of message
 */
void
warn(char *func, char *fmt, ...)
{
    va_list ap;	/* argument pointer */
    char *newfmt;	/* syslog with leading tag */
    int newfmtlen;	/* length of newfmt */
    double now = 0.0;	/* current time stamp */

    /* firewall */
    if (func == NULL) {
	func = "<<NULL function>>";
    }
    if (fmt == NULL) {
	fmt = "<<NULL format>>";
    }

    /* get the time if output to logfile or srderr */
    if (log != NULL || use_stderr) {
	/* use a private time to avoid interfering with other functions */
	now = right_now();
    }

    /* start the var arg setup and fetch our first arg */
    va_start(ap, fmt);

    /*
     * output header, message and newline
     */
    if (use_syslog) {
	/* output to syslog */
	newfmtlen = strlen(fmt) + sizeof("WARNING: ") + 1;
	newfmt = malloc(newfmtlen);
	if (newfmt != NULL) {
	    snprintf(newfmt, newfmtlen, "WARNING: %s", fmt);
	    syslog(LOG_WARNING, newfmt, ap);
	    free(newfmt);
	} else {
	    syslog(LOG_WARNING, fmt, ap);
	}
    }
    if (log != NULL) {
	/* output to logfile */
	fprintf(log, "%17.6f: %s: WARNING: ", now, func);
	vfprintf(log, fmt, ap);
	fputc('\n', log);
	fflush(log);
    }
    if (use_stderr) {
	/* output to stderr */
	fprintf(stderr, "%17.6f: %s: WARNING: ", now, func);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	fflush(stderr);
    }
}


/*
 * fatal - print a message and exit
 *
 * given:
 *      code    exit code
 *      func    name of calling function
 *      fmt     printf format of message
 */
void
fatal(int code, char *func, char *fmt, ...)
{
    va_list ap;	/* argument pointer */
    char *newfmt;	/* syslog with leading tag */
    int newfmtlen;	/* length of newfmt */
    double now = 0.0;	/* current time stamp */

    /* firewall */
    if (func == NULL) {
	func = "<<NULL function>>";
    }
    if (fmt == NULL) {
	fmt = "<<NULL format>>";
    }

    /* get the time if output to logfile or srderr */
    if (log != NULL || use_stderr) {
	/* use a private time to avoid interfering with other functions */
	now = right_now();
    }

    /* start the var arg setup and fetch our first arg */
    va_start(ap, fmt);

    /*
     * output header, message and newline
     */
    if (use_syslog) {
	/* output to syslog */
	newfmtlen = strlen(fmt) + sizeof("!!!FATAL!!!: ") + 1;
	newfmt = malloc(newfmtlen);
	if (newfmt != NULL) {
	    snprintf(newfmt, newfmtlen, "!!!FATAL!!!: %s", fmt);
	    syslog(LOG_ERR, newfmt, ap);
	    free(newfmt);
	} else {
	    syslog(LOG_ERR, fmt, ap);
	}
    }
    if (log != NULL) {
	/* output to logfile */
	fprintf(log, "%17.6f: %s: !!!FATAL!!!: ", now, func);
	vfprintf(log, fmt, ap);
	fputc('\n', log);
	fflush(log);
    }
    if (use_stderr) {
	/* output to stderr */
	fprintf(stderr, "%17.6f: %s: !!!FATAL!!!: ", now, func);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	fflush(stderr);
    }

    /* exit */
    exit(code);
}
