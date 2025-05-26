/*
 * rawio - raw I/O with appropriate retries and alarm checking
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: rawio.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include "LavaRnd/lavaerr.h"
#include "LavaRnd/rawio.h"

#if defined(DMALLOC)
#  include <dmalloc.h>
#endif

#include "LavaRnd/have/have_sys_time.h"
#if defined(HAVE_SYS_TIME_H)
#  include <sys/time.h>
#endif

#include "LavaRnd/have/have_pselect.h"


/*
 * public alarm state
 *
 * NOTE: This state is not re-entrant.  We only have 1 alarm signal.
 *       We only can setup 1 alarm at a time.
 */
int lava_ring = LAVA_NO_ALARM;	/* current alarm state */
double lava_duration = 0.0;	/* time between set and clear alarm */
double lava_dur_arg = 0.0;	/* duration requested for alarm */


/*
 * private alarm state
 */
static int isalarm = 0;	/* 1 => alarm has been set */
static int issimple = 0;	/* 1 => simple alarm has been set */
struct itimerval invalid_timer;	/* not a valid alarm time */
invalid_timer.it_value.tv_sec = -1;
invalid_timer.it_value.tv_usec = -1;

struct itimerval timer = {	/* current alarm time */
    {0, 0}, {0, 0}
};
static struct itimerval old_timer = {	/* previous alarm time */
    {-1, -1}, {-1, -1}
};
static struct itimerval no_time = {	/* zero timer - no alarm */
    {0, 0}, {0, 0}
};
static void my_alarm(int);	/* our signal handler */
static struct sigaction old_sigaction;	/* prev alarm handler action */
static double start_time = -1.0;	/* when our alarm was set */
static double end_time = 0.0;	/* when our alarm was cleared */


/*
 * raw_read - perform a low level read on a descriptor
 *
 * Read from a descriptor.  Return read count, or 0 if EOF, <0 if error
 * or LAVAERR_TIMEOUT if chk_alarm and the lava_ring indicator has been set.
 *
 * given:
 *      fd              open descriptor
 *      buf             where to read data
 *      len             max number of octets that the port will return
 *      chk_alarm       TRUE ==> check lava_ring, FALSE ==> ignore lava_ring
 *
 * returns:
 *      count           octets of data read or 0 (if EOF) or < 0 on error
 *
 * NOTE: If chk_alarm and we detect a lava_ring we will simply return the
 *       current I/O count.  However if we have not read anything, then
 *       we will return the LAVAERR_TIMEOUT error.  It is up to the calling
 *       function to check for lava_ring and to call clear_simple_alarm() when
 *       it was found to be set.
 *
 * NOTE: This function only returns 0 on EOF.  Should the lava_ring
 *       indicator be triggered and nothing has been read, we will
 *       return the LAVAERR_TIMEOUT error.
 */
int
raw_read(int fd, void *buf, size_t count, int chk_alarm)
{
    int ret;	/* low level read return */
    size_t total;	/* total read count */
    char *next;	/* where to read next */

    /*
     * firewall
     */
    if (count < 1 || buf == NULL) {
	/* bogus args */
	return LAVAERR_BADARG;
    }

    /*
     * try to read until EOF or error or done
     */
    total = 0;
    next = (char *)buf;
    do {

	/*
	 * check for alarms
	 */
	if (chk_alarm && lava_ring) {
	    /* alarm timeout */
	    if (total == 0) {
		return LAVAERR_TIMEOUT;
	    } else {
		return total;
	    }
	}

	/*
	 * perform a low level read
	 */
	ret = read(fd, next, count - total);
	if (ret > 0) {
	    total += ret;
	    next += ret;
	}

    } while ((ret < 0 && errno == EINTR) || (ret > 0 && total < count));

    /*
     * return count or error
     */
    if (ret < 0) {
	if (errno == EAGAIN) {
	    return LAVAERR_NONBLOCK;
	} else {
	    return LAVAERR_IOERR;
	}
    }
    return total;
}


/*
 * read_once - perform only one low level read on a descriptor
 *
 * Read from a descriptor.  Return read count, or 0 if EOF, <0 if error
 * or LAVAERR_TIMEOUT if chk_alarm and the lava_ring indicator has been set.
 *
 * Unlike raw_read() which tries to read the entire count (or until
 * EOF or lava_ring), this function only performs a single (potentially
 * blocking) read.
 *
 * given:
 *      fd              open descriptor
 *      buf             where to read data
 *      len             max number of octets that the port will return
 *      chk_alarm       TRUE ==> check lava_ring, FALSE ==> ignore lava_ring
 *
 * returns:
 *      count           octets of data returned or < 0 on error
 *
 * NOTE: If chk_alarm and we detect a lava_ring we will simply return the
 *       current I/O count.  However if we have not read anything, then
 *       we will return the LAVAERR_TIMEOUT error.  It is up to the calling
 *       function to check for lava_ring and to call clear_simple_alarm() when
 *       it was found to be set.
 *
 * NOTE: This function only returns 0 on EOF.  Should the lava_ring
 *       indicator be triggered and nothing has been read, we will
 *       return the LAVAERR_TIMEOUT error.
 */
int
read_once(int fd, void *buf, size_t count, int chk_alarm)
{
    int ret;	/* low level read return */

    /*
     * firewall
     */
    if (count < 1 || buf == NULL) {
	/* bogus args */
	return LAVAERR_BADARG;
    }

    /*
     * read from the descriptor
     */
    do {

	/* check for alarms */
	if (chk_alarm && lava_ring) {
	    /* alarm timeout */
	    return LAVAERR_TIMEOUT;
	}

	/* perform the low level read */
	errno = 0;
	ret = read(fd, buf, count);
    } while (errno == EINTR);

    /*
     * return count or error
     */
    if (ret < 0) {
	if (errno == EAGAIN) {
	    return LAVAERR_NONBLOCK;
	} else {
	    return LAVAERR_IOERR;
	}
    }
    return ret;
}


/*
 * noblock_read - perform non-blocking low level read on a descriptor
 *
 * Read from a descriptor.  Retry if EINTR.  Return 0 if EOF, <0 if error
 * or if the lava_ring indicator has been set.
 *
 * If no data has been read and a read operation would block (because
 * there is no data immediately available) then the LAVAERR_NONBLOCK error
 * is returned.  However if some data has been read and an internal read
 * operation would block, then the number of octets already read is returned.
 *
 * Unlike raw_read() or read_once(), this function will not block
 * if there is not enough or no immediate data to read.
 *
 * given:
 *      fd              open descriptor
 *      buf             where to read data
 *      len             max number of octets that the port will return
 *      chk_alarm       TRUE ==> check lava_ring, FALSE ==> ignore lava_ring
 *
 * returns:
 *      count           octets of data returned or < 0 on error
 *                      or return LAVAERR_NONBLOCK if no data was
 *                      read and I/O operation would block
 *
 * NOTE: If chk_alarm and we detect a lava_ring we will simply return the
 *       current I/O count (which may be 0).  It is up to the calling function
 *       to call clear_simple_alarm().
 *
 * NOTE: This function only returns 0 on EOF.  Should the lava_ring
 *       indicator be triggered and nothing has been read, we will
 *       return the LAVAERR_TIMEOUT error.
 */
int
noblock_read(int fd, void *buf, size_t count, int chk_alarm)
{
    int ret;	/* low level read or select return */
    size_t total;	/* total read count */
    char *next;	/* where to read next */
    fd_set rd_set;	/* read select set */
    struct timeval notime;	/* no time to wait */

    /*
     * firewall
     */
    if (count < 1 || buf == NULL) {
	/* bogus args */
	return LAVAERR_BADARG;
    }

    /*
     * try to read until EOF or error or done
     */
    total = 0;
    next = (char *)buf;
    do {

	/*
	 * check for alarms
	 */
	if (chk_alarm && lava_ring) {
	    /* alarm timeout */
	    if (total == 0) {
		return LAVAERR_TIMEOUT;
	    } else {
		return total;
	    }
	}

	/*
	 * common select setup
	 */
	FD_ZERO(&rd_set);
	FD_SET(fd, &rd_set);
	notime.tv_sec = 0;
	notime.tv_usec = 0;

	/*
	 * check for blocking
	 */
	do {
	    /* clear system error code */
	    errno = 0;

	    /*
	     * select
	     *
	     * NOTE: Because we are selecting for a 0.0 second duration,
	     *       we do not need to keep track of the amount of time
	     *       spent in select()/pselect().
	     */
#if defined(HAVE_PSELECT)
	    ret = pselect(fd + 1, &rd_set, NULL, NULL, &notime, NULL);
#else /* HAVE_PSELECT */
	    ret = select(fd + 1, &rd_set, NULL, NULL, &notime);
#endif /* HAVE_PSELECT */
	} while (ret < 0 && errno == EINTR);
	if (ret < 0) {
	    return LAVAERR_IOERR;
	} else if (ret == 0) {
	    if (total > 0) {
		return total;
	    } else {
		return LAVAERR_NONBLOCK;
	    }
	}

	/*
	 * perform a low level read
	 */
	ret = read(fd, next, count - total);
	if (ret > 0) {
	    total += ret;
	    next += ret;
	}

    } while ((ret < 0 && errno == EINTR) || (ret > 0 && total < count));

    /*
     * return count or error
     */
    if (ret < 0) {
	if (errno == EAGAIN) {
	    return LAVAERR_NONBLOCK;
	} else {
	    return LAVAERR_IOERR;
	}
    }
    return total;
}


/*
 * nilblock_read - perform tiny blocking low level read on a descriptor
 *
 * Read from a descriptor.  Retry if EINTR.  Return 0 if EOF, <0 if error
 * or if the lava_ring indicator has been set.
 *
 * If no data has been read and a read operation would block (because
 * there is no data immediately available) then the LAVAERR_NONBLOCK error
 * is returned.  However if some data has been read and an internal read
 * operation would block, then the number of octets already read is returned.
 *
 * Unlike raw_read() or read_once(), this function will not block
 * very long if there is not enough or no immediate data to read.
 *
 * Unlike noblock_read() this function will wait for a LAVA_TINY_TIME
 * period of time before giving up.
 *
 * given:
 *      fd              open descriptor
 *      buf             where to read data
 *      len             max number of octets that the port will return
 *      chk_alarm       TRUE ==> check lava_ring, FALSE ==> ignore lava_ring
 *
 * returns:
 *      count           octets of data returned or < 0 on error
 *                      or return LAVAERR_NONBLOCK if no data was
 *                      read and I/O operation would block
 *
 * NOTE: If chk_alarm and we detect a lava_ring we will simply return the
 *       current I/O count (which may be 0).  It is up to the calling function
 *       to call clear_simple_alarm().
 *
 * NOTE: This function only returns 0 on EOF.  Should the lava_ring
 *       indicator be triggered and nothing has been read, we will
 *       return the LAVAERR_TIMEOUT error.
 */
int
nilblock_read(int fd, void *buf, size_t count, int chk_alarm)
{
    int ret;	/* low level read or select return */
    size_t total;	/* total read count */
    char *next;	/* where to read next */
    fd_set rd_set;	/* read select set */
    struct timeval niltime;	/* no time to wait */

    /*
     * firewall
     */
    if (count < 1 || buf == NULL) {
	/* bogus args */
	return LAVAERR_BADARG;
    }

    /*
     * try to read until EOF or error or done
     */
    total = 0;
    next = (char *)buf;
    do {

	/*
	 * check for alarms
	 */
	if (chk_alarm && lava_ring) {
	    /* alarm timeout */
	    if (total == 0) {
		return LAVAERR_TIMEOUT;
	    } else {
		return total;
	    }
	}

	/*
	 * common select setup
	 */
	FD_ZERO(&rd_set);
	FD_SET(fd, &rd_set);
	niltime.tv_sec = 0;
	niltime.tv_usec = double_to_tv_usec(LAVA_TINY_TIME);

	/*
	 * check for blocking
	 */
	do {
	    /* clear system error code */
	    errno = 0;

	    /*
	     * select
	     *
	     * NOTE: Because we are selecting for a tiny/nero-0 sec duration,
	     *       we do not need to keep track of the amount of time
	     *       spent in select()/pselect().
	     */
#if defined(HAVE_PSELECT)
	    ret = pselect(fd + 1, &rd_set, NULL, NULL, &niltime, NULL);
#else /* HAVE_PSELECT */
	    ret = select(fd + 1, &rd_set, NULL, NULL, &niltime);
#endif /* HAVE_PSELECT */
	} while (ret < 0 && errno == EINTR);
	if (ret < 0) {
	    return LAVAERR_IOERR;
	} else if (ret == 0) {
	    if (total > 0) {
		return total;
	    } else {
		return LAVAERR_NONBLOCK;
	    }
	}

	/*
	 * perform a low level read
	 */
	ret = read(fd, next, count - total);
	if (ret > 0) {
	    total += ret;
	    next += ret;
	}

    } while ((ret < 0 && errno == EINTR) || (ret > 0 && total < count));

    /*
     * return count or error
     */
    if (ret < 0) {
	if (errno == EAGAIN) {
	    return LAVAERR_NONBLOCK;
	} else {
	    return LAVAERR_IOERR;
	}
    }
    return total;
}


/*
 * raw_write - perform a low level write on a descriptor
 *
 * Write to a descriptor.  Return write count, or 0 if EOF, <0 if error
 * or LAVAERR_TIMEOUT if chk_alarm and the lava_ring indicator has been set.
 *
 * given:
 *      fd              open descriptor
 *      buf             from where to write data
 *      len             max number of octets that we will write
 *      chk_alarm       TRUE ==> check lava_ring, FALSE ==> ignore lava_ring
 *
 * returns:
 *      count           octets of data written or 0 (if EOF) or < 0 on error
 *
 * NOTE: If chk_alarm and we detect a lava_ring we will simply return the
 *       current I/O count.  However if we have not read anything, then
 *       we will return the LAVAERR_TIMEOUT error.  It is up to the calling
 *       function to check for lava_ring and to call clear_simple_alarm() when
 *       it was found to be set.
 *
 * NOTE: This function only returns 0 on EOF.  Should the lava_ring
 *       indicator be triggered and nothing has been written, we will
 *       return the LAVAERR_TIMEOUT error.
 */
int
raw_write(int fd, void *buf, size_t count, int chk_alarm)
{
    int ret;	/* low level write return */
    size_t total;	/* total write count */
    char *next;	/* where to write next */

    /*
     * firewall
     */
    if (count < 1 || buf == NULL) {
	/* bogus args */
	return LAVAERR_BADARG;
    }

    /*
     * try to write until EOF or error or done
     */
    total = 0;
    next = (char *)buf;
    do {

	/*
	 * check for alarms
	 */
	if (chk_alarm && lava_ring) {
	    /* alarm timeout */
	    if (total == 0) {
		return LAVAERR_TIMEOUT;
	    } else {
		return total;
	    }
	}

	/*
	 * perform a low level write
	 */
	ret = write(fd, next, count - total);
	if (ret > 0) {
	    total += ret;
	    next += ret;
	}

    } while ((ret < 0 && errno == EINTR) || (ret > 0 && total < count));

    /*
     * return count or error
     */
    if (ret < 0) {
	if (errno == EAGAIN) {
	    return LAVAERR_NONBLOCK;
	} else {
	    return LAVAERR_IOERR;
	}
    }
    return total;
}


/*
 * write_once - perform only one low level write on a descriptor
 *
 * Write to a descriptor.  Return write count, or 0 if EOF, <0 if error
 * or LAVAERR_TIMEOUT if chk_alarm and the lava_ring indicator has been set.
 *
 * Unlike raw_write() which tries to write the entire count (or until
 * EOF or lava_ring), this function only performs a single (potentially
 * blocking) write.
 *
 * given:
 *      fd              open descriptor
 *      buf             from where to write data
 *      len             max number of octets that we will write
 *      chk_alarm       TRUE ==> check lava_ring, FALSE ==> ignore lava_ring
 *
 * returns:
 *      count           octets of data written or < 0 on error
 *
 * NOTE: If chk_alarm and we detect a lava_ring we will simply return the
 *       current I/O count.  However if we have not write anything, then
 *       we will return the LAVAERR_TIMEOUT error.  It is up to the calling
 *       function to check for lava_ring and to call clear_simple_alarm() when
 *       it was found to be set.
 *
 * NOTE: This function only returns 0 on EOF.  Should the lava_ring
 *       indicator be triggered and nothing has been write, we will
 *       return the LAVAERR_TIMEOUT error.
 */
int
write_once(int fd, void *buf, size_t count, int chk_alarm)
{
    int ret;	/* low level write return */

    /*
     * firewall
     */
    if (count < 1 || buf == NULL) {
	/* bogus args */
	return LAVAERR_BADARG;
    }

    /*
     * write from the descriptor
     */
    do {

	/* check for alarms */
	if (chk_alarm && lava_ring) {
	    /* alarm timeout */
	    return LAVAERR_TIMEOUT;
	}

	/* perform the low level write */
	errno = 0;
	ret = write(fd, buf, count);
    } while (errno == EINTR);

    /*
     * return count or error
     */
    if (ret < 0) {
	if (errno == EAGAIN) {
	    return LAVAERR_NONBLOCK;
	} else {
	    return LAVAERR_IOERR;
	}
    }
    return ret;
}


/*
 * noblock_write - perform non-blocking low level write on a descriptor
 *
 * Write to a descriptor.  Return write count, or 0 if EOF, <0 if error
 * or LAVAERR_TIMEOUT if chk_alarm and the lava_ring indicator has been set.
 *
 * If no data has been written and a write operation would block
 * then the LAVAERR_NONBLOCK error is returned.  However if some data
 * has been written and an internal write operation would block, then
 * the number of octets already written is returned.
 *
 * Unlike raw_write() or write_once(), this function will not block
 * if no data can be immediately written.
 *
 * given:
 *      fd              open descriptor
 *      buf             from where to write data
 *      len             max number of octets that this function will write
 *      chk_alarm       TRUE ==> check lava_ring, FALSE ==> ignore lava_ring
 *
 * returns:
 *      count           octets of data written or < 0 on error
 *                      or return LAVAERR_NONBLOCK if no data was
 *                      write and I/O operation would block
 *
 * NOTE: If chk_alarm and we detect a lava_ring we will simply return the
 *       current I/O count (which may be 0).  It is up to the calling function
 *       to call clear_simple_alarm().
 */
int
noblock_write(int fd, void *buf, size_t count, int chk_alarm)
{
    int ret;	/* low level write or select return */
    size_t total;	/* total write count */
    char *next;	/* where to write next */
    fd_set wr_set;	/* write select set */
    struct timeval notime;	/* no time to wait */

    /*
     * firewall
     */
    if (count < 1 || buf == NULL) {
	/* bogus args */
	return LAVAERR_BADARG;
    }

    /*
     * try to write until EOF or error or done
     */
    total = 0;
    next = (char *)buf;
    do {

	/*
	 * check for alarms
	 */
	if (chk_alarm && lava_ring) {
	    /* alarm timeout */
	    if (total == 0) {
		return LAVAERR_TIMEOUT;
	    } else {
		return total;
	    }
	}

	/*
	 * common select setup
	 */
	FD_ZERO(&wr_set);
	FD_SET(fd, &wr_set);
	notime.tv_sec = 0;
	notime.tv_usec = 0;

	/*
	 * check for blocking
	 */
	do {
	    /* clear system error code */
	    errno = 0;

	    /*
	     * select
	     *
	     * NOTE: Because we are selecting for a 0.0 second duration,
	     *       we do not need to keep track of the amount of time
	     *       spent in select()/pselect().
	     */
#if defined(HAVE_PSELECT)
	    ret = pselect(fd + 1, NULL, &wr_set, NULL, &notime, NULL);
#else /* HAVE_PSELECT */
	    ret = select(fd + 1, NULL, &wr_set, NULL, &notime);
#endif /* HAVE_PSELECT */
	} while (ret < 0 && errno == EINTR);
	if (ret < 0) {
	    return LAVAERR_IOERR;
	} else if (ret == 0) {
	    if (total > 0) {
		return total;
	    } else {
		return LAVAERR_NONBLOCK;
	    }
	}

	/*
	 * perform a low level write
	 */
	ret = write(fd, next, count - total);
	if (ret > 0) {
	    total += ret;
	    next += ret;
	}

    } while ((ret < 0 && errno == EINTR) || (ret > 0 && total < count));

    /*
     * return count or error
     */
    if (ret < 0) {
	if (errno == EAGAIN) {
	    return LAVAERR_NONBLOCK;
	} else {
	    return LAVAERR_IOERR;
	}
    }
    return total;
}


/*
 * nilblock_write - perform tiny blocking low level write on a descriptor
 *
 * Write to a descriptor.  Return write count, or 0 if EOF, <0 if error
 * or LAVAERR_TIMEOUT if chk_alarm and the lava_ring indicator has been set.
 *
 * If no data has been written and a write operation would block
 * then the LAVAERR_NONBLOCK error is returned.  However if some data
 * has been written and an internal write operation would block, then
 * the number of octets already written is returned.
 *
 * Unlike raw_write() or write_once(), this function will not block
 * very long if no data can be written in a short period of time.
 *
 * Unlike noblock_write() this function will wait for a LAVA_TINY_TIME
 * period of time before giving up.
 *
 * given:
 *      fd              open descriptor
 *      buf             from where to write data
 *      len             max number of octets that this function will write
 *      chk_alarm       TRUE ==> check lava_ring, FALSE ==> ignore lava_ring
 *
 * returns:
 *      count           octets of data written or < 0 on error
 *                      or return LAVAERR_NONBLOCK if no data was
 *                      write and I/O operation would block
 *
 * NOTE: If chk_alarm and we detect a lava_ring we will simply return the
 *       current I/O count (which may be 0).  It is up to the calling function
 *       to call clear_simple_alarm().
 */
int
nilblock_write(int fd, void *buf, size_t count, int chk_alarm)
{
    int ret;	/* low level write or select return */
    size_t total;	/* total write count */
    char *next;	/* where to write next */
    fd_set wr_set;	/* write select set */
    struct timeval niltime;	/* tiny time to wait */

    /*
     * firewall
     */
    if (count < 1 || buf == NULL) {
	/* bogus args */
	return LAVAERR_BADARG;
    }

    /*
     * try to write until EOF or error or done
     */
    total = 0;
    next = (char *)buf;
    do {

	/*
	 * check for alarms
	 */
	if (chk_alarm && lava_ring) {
	    /* alarm timeout */
	    if (total == 0) {
		return LAVAERR_TIMEOUT;
	    } else {
		return total;
	    }
	}

	/*
	 * common select setup
	 */
	FD_ZERO(&wr_set);
	FD_SET(fd, &wr_set);
	niltime.tv_sec = 0;
	niltime.tv_usec = double_to_tv_usec(LAVA_TINY_TIME);

	/*
	 * check for blocking
	 */
	do {
	    /* clear system error code */
	    errno = 0;

	    /*
	     * select
	     *
	     * NOTE: Because we are selecting for a tiny/nero-0 sec duration,
	     *       we do not need to keep track of the amount of time
	     *       spent in select()/pselect().
	     */
#if defined(HAVE_PSELECT)
	    ret = pselect(fd + 1, NULL, &wr_set, NULL, &niltime, NULL);
#else /* HAVE_PSELECT */
	    ret = select(fd + 1, NULL, &wr_set, NULL, &niltime);
#endif /* HAVE_PSELECT */
	} while (ret < 0 && errno == EINTR);
	if (ret < 0) {
	    return LAVAERR_IOERR;
	} else if (ret == 0) {
	    if (total > 0) {
		return total;
	    } else {
		return LAVAERR_NONBLOCK;
	    }
	}

	/*
	 * perform a low level write
	 */
	ret = write(fd, next, count - total);
	if (ret > 0) {
	    total += ret;
	    next += ret;
	}

    } while ((ret < 0 && errno == EINTR) || (ret > 0 && total < count));

    /*
     * return count or error
     */
    if (ret < 0) {
	if (errno == EAGAIN) {
	    return LAVAERR_NONBLOCK;
	} else {
	    return LAVAERR_IOERR;
	}
    }
    return total;
}


/*
 * noblock_fd - setup a descriptor for non-blocking I/O
 *
 * given:
 *      fd              open descriptor
 *
 * returns:
 *      0 ==> OK, <0 ==> error
 */
int
noblock_fd(int fd)
{
    if (fcntl(fd, F_SETFL, (long)O_NONBLOCK) < 0) {
	return LAVAERR_FCNTLERR;
    }
    return 0;
}


/*
 * set_alarm - setup an alarm for a fixed period of time, save old state
 *
 * given:
 *      duration        how long (in seconds) before the alarm is set
 *                      0 ==> no alarm to set
 *
 * returns:
 *      lava_ring       alarm ring indication or error status
 *
 * NOTE: Even if this function returns an error, clear_alarm() should
 *       be called.
 */
int
set_alarm(double duration)
{
    struct sigaction sig_ign;	/* how we ignore the SIGALRM */
    struct sigaction action;	/* how we setup the SIGALRM */
    static struct itimerval tmp_timer;	/* temp storage of old_timer */

    /*
     * firewall
     */
    if (isalarm) {
	/* report attempt to set an already set alarm */
	/* needed to have called clear_alarm first */
	return LAVA_DUP_ALARM;
    }

    /*
     * deal with the case where there is zero (0.0) alarm duration
     */
    issimple = 0;
    if (duration == 0.0) {
	return clear_alarm();
    }

    /*
     * firewall - must have a positive duration
     */
    if (duration < LAVA_TINY_TIME) {
	duration = LAVA_TINY_TIME;
    }
    lava_dur_arg = duration;

    /*
     * set pre-alarm conditions
     *
     * These pre-alarm conditions help distinguish an alarm that was setup
     * by some different function sometime earlier and our setup.
     */
    lava_ring = LAVA_NO_ALARM;
    old_timer = invalid_timer;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = my_alarm;
    action.sa_flags = 0;
    memset(&sig_ign, 0, sizeof(struct sigaction));
    sig_ign.sa_handler = SIG_IGN;
    sig_ign.sa_flags = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = double_to_tv_sec(duration);
    timer.it_value.tv_usec = double_to_tv_usec(duration);
    end_time = 0.0;
    lava_duration = 0.0;

    /*
     * We first clear the timer before setting the SIGALRM sigaction
     * to try and avoid having our sigaction being hit with the
     * previous alarm interval.
     *
     * We remember the old timer state, and load it into the
     * old_timer state after we have setup our alarm.
     */
    start_time = right_now();
    if (start_time < 0.0) {
	/* simply note time failure */
	lava_ring |= LAVAERR_GETTIME;
	start_time = -1.0;
    }
    tmp_timer = invalid_timer;
    if (setitimer(ITIMER_REAL, &no_time, &tmp_timer) < 0) {
	/* setitimer failure */
	lava_ring |= LAVA_ALARM_ERR;
	return lava_ring;
    }

    /*
     * Next we ignore the SIGALRM signal
     *
     * We remember the old signal state for when we clear the signal.
     */
    if (sigaction(SIGALRM, &sig_ign, &old_sigaction) < 0) {
	/* sigaction failure */
	lava_ring |= LAVA_ALARM_ERR;
	return lava_ring;
    }

    /*
     * set pre-alarm conditions
     *
     * These pre-alarm conditions help distinguish an alarm that was setup
     * by some different function sometime earlier and our setup.
     */
    lava_ring = LAVA_NO_ALARM;
    old_timer = invalid_timer;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = my_alarm;
    action.sa_flags = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = double_to_tv_sec(duration);
    timer.it_value.tv_usec = double_to_tv_usec(duration);
    end_time = 0.0;
    lava_duration = 0.0;

    /*
     * set the alarm
     *
     * Once the old_timer is changed from invalid_timer to the old
     * timer value (which can never be invalid_timer), we know that
     * the alarm is safely under our control until clear_alarm()
     * is called (or someone else rips off our alarm).
     *
     * NOTE: Even though the alarm interval is cleared at this point,
     *       it is still possible for the previous interval alarm
     *       (at this point kept in tmp_timer) to deliver a SIGALRM.
     *       This is because it is possible, under pathological situations,
     *       for there to be a non-trivial delay between when a
     *       signal is raised and when it is delivered.  Clearing the
     *       alarm interval may prevent new alarms from being raised,
     *       but it does not prevent an already raised alarm (that
     *       has been delayed) from being delivered now or further below.
     *
     *       Some care is taken with the way the my_alarm() SIGALRM
     *       catcher works.  At this point old_timer is set to an
     *       invalid value: invalid_timer.  When we are fully setup,
     *       and when we have performed two system calls, then we change
     *       old_timer to tmp_timer (which is != invalid_timer) at
     *       this point.
     */
    if (sigaction(SIGALRM, &action, NULL) < 0) {
	/* sigaction failure */
	lava_ring |= LAVA_ALARM_ERR;
	return lava_ring;
    }
    if (setitimer(ITIMER_REAL, &timer, NULL) < 0) {
	/* setitimer failure */
	lava_ring |= LAVA_ALARM_ERR;
	return lava_ring;
    }
    old_timer = tmp_timer;	/* let my_alarm() know we are ready */

    /*
     * note that the alarm was set
     */
    isalarm = 1;

    /*
     * return current ring status
     */
    return lava_ring;
}


/*
 * set_simple_alarm - setup a alarm, do not attempt to preserve old alarm
 *
 * Set an alarm.  Do not bother to preserve the existing interval / alarm
 * timer and signal handler.  Assume there is no existing alarm in effect.
 *
 * given:
 *      duration        how long (in seconds) before the alarm is set
 *                      0 ==> no alarm to set
 * returns:
 *      lava_ring       alarm ring indication or error status
 *
 * NOTE: Even if this function returns an error, clear_simple_alarm() should
 *       be called.
 */
int
set_simple_alarm(double duration)
{
    struct sigaction action;	/* how we setup the SIGALRM */

    /*
     * firewall
     */
    if (isalarm) {
	/* report attempt to set an already set alarm */
	/* needed to have called clear_simple_alarm first */
	return LAVA_DUP_ALARM;
    }

    /*
     * deal with the case where there is zero (0.0) alarm duration
     *
     * We do not touch the alarm or SIGALRM state, we set lava_ring
     * to LAVA_NO_ALARM and return early.
     */
    if (duration == 0.0) {
	return clear_simple_alarm();
    }

    /*
     * must have a positive duration
     */
    if (duration < LAVA_TINY_TIME) {
	duration = LAVA_TINY_TIME;
    }
    lava_dur_arg = duration;

    /*
     * set pre-alarm conditions
     *
     * These pre-alarm conditions help distinguish an alarm that was setup
     * by some different function sometime earlier and our setup.
     */
    lava_ring = LAVA_NO_ALARM;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = my_alarm;
    action.sa_flags = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = double_to_tv_sec(duration);
    timer.it_value.tv_usec = double_to_tv_usec(duration);

    /*
     * set the alarm
     */
    if (sigaction(SIGALRM, &action, NULL) < 0) {
	/* sigaction failure */
	lava_ring |= LAVA_ALARM_ERR;
	return lava_ring;
    }
    if (setitimer(ITIMER_REAL, &timer, NULL) < 0) {
	/* setitimer failure */
	lava_ring |= LAVA_ALARM_ERR;
	return lava_ring;
    }

    /*
     * note that the alarm was set
     */
    isalarm = 1;
    issimple = 1;

    /*
     * return current ring status
     */
    return lava_ring;
}


/*
 * clear_alarm - clear a previously set alarm and restore old alarm state
 *
 * returns:
 *      lava_ring       alarm ring indication
 *
 * NOTE: One must call this function after calling set_alarm() so that
 *       the previous alarm state can be restored and so that set_alarm()
 *       can be called again.
 */
int
clear_alarm(void)
{
    struct sigaction sig_ign;	/* how we ignore the SIGALRM */

    /*
     * firewall
     */
    if (!isalarm) {
	/* no alarm set */
	return LAVA_NOSET_ALARM;
    }
    if (issimple) {
	return clear_simple_alarm();
    }

    /*
     * do nothing else if we had no (0.0) alarm duration
     */
    if (lava_dur_arg == 0.0) {
	/* 0 duration alarms never ring */
	return LAVA_NO_ALARM;
    }

    /*
     * disable our timer
     */
    if (setitimer(ITIMER_REAL, &no_time, &timer) < 0) {
	/* setitimer failure */
	lava_ring |= LAVA_ALARM_ERR;
	isalarm = 0;
	return lava_ring;
    }

    /*
     * restore the old signal handler
     */
    memset(&sig_ign, 0, sizeof(struct sigaction));
    sig_ign.sa_handler = SIG_IGN;
    sig_ign.sa_flags = 0;
    if (sigaction(SIGALRM, &sig_ign, NULL) < 0) {
	/* sigaction failure */
	lava_ring |= LAVA_ALARM_ERR;
	return lava_ring;
    }
    if (sigaction(SIGALRM, &old_sigaction, NULL) < 0) {
	/* sigaction failure */
	lava_ring |= LAVA_ALARM_ERR;
	return lava_ring;
    }
    isalarm = 0;

    /*
     * determine the duration from set_alarm() to now
     */
    lava_duration = 0.0;
    if (start_time > 0.0) {
	end_time = right_now();
	if (end_time < 0.0) {
	    /* simply note the right_now failure */
	    lava_ring |= LAVAERR_GETTIME;
	} else {
	    lava_duration = end_time - start_time;
	}
    }

    /*
     * restore the old timer, if it was set
     *
     * We attempt to account for the time spent in our own signal handler
     * by attempting to remove it from the current signal handler.
     */
    if (memcmp(&old_timer, &no_time, sizeof(struct itimerval)) == 0) {
	double time_left = 0.0;	/* time left on timer */

	/*
	 * Remove lava_duration seconds from the old timer.  If
	 * this would cause the timer to go neg, then set the
	 * timer to 0.01 seconds from now.  We pick 0.01 seconds
	 * because it is a small interval that normally would
	 * permit a timeslice of processing (such as our return)
	 * to occur before the alarm goes off.
	 */
	time_left = timeval_to_double(old_timer.it_value) - lava_duration;
	if (time_left < LAVA_TINY_TIME) {
	    time_left = LAVA_TINY_TIME;
	}
	old_timer.it_value.tv_sec = double_to_tv_sec(time_left);
	old_timer.it_value.tv_usec = double_to_tv_usec(time_left);

	/*
	 * we will attempt to set the timer to the old_timer value
	 * adjusted for our set/clear_alarm duration.
	 */
	if (setitimer(ITIMER_REAL, &old_timer, NULL) < 0) {
	    /* setitimer failure */
	    lava_ring |= LAVA_ALARM_ERR;
	}
    }

    /*
     * return last ring status
     */
    return lava_ring;
}


/*
 * clear_simple_alarm - clear a alarm and do not restore old alarm state
 *
 * returns:
 *      lava_ring       alarm ring indication
 *
 * NOTE: One must call this function after calling set_simple_alarm() so that
 *       the previous alarm state can be restored and so that set_alarm()
 *       can be called again.
 *
 * NOTE: Do not call clear_simple_alarm() if set_alarm() was previously
 *       called.
 */
int
clear_simple_alarm(void)
{
    struct sigaction sig_ign;	/* how we ignore the SIGALRM */

    /*
     * firewall
     */
    if (!isalarm) {
	/* no alarm set */
	return LAVA_NOSET_ALARM;
    }
    if (!issimple) {
	return clear_alarm();
    }

    /*
     * do nothing else if we had no (0.0) alarm duration
     */
    if (lava_dur_arg == 0.0) {
	/* 0 duration alarms never ring */
	return LAVA_NO_ALARM;
    }

    /*
     * disable the signal handler
     */
    memset(&sig_ign, 0, sizeof(struct sigaction));
    sig_ign.sa_handler = SIG_IGN;
    sig_ign.sa_flags = 0;
    if (sigaction(SIGALRM, &sig_ign, NULL) < 0) {
	/* sigaction failure */
	lava_ring |= LAVA_ALARM_ERR;
	return lava_ring;
    }

    /*
     * disable our timer
     */
    if (setitimer(ITIMER_REAL, &no_time, &timer) < 0) {
	/* setitimer failure */
	lava_ring |= LAVA_ALARM_ERR;
	issimple = 0;
	isalarm = 0;
	return lava_ring;
    }
    isalarm = 0;
    issimple = 0;

    /*
     * return last ring status
     */
    return lava_ring;
}


/*
 * my_alarm
 *
 * This function is called whenever our SIGALRM is triggered.  It may
 * occur between the time we setup the sigaction / signal handler
 * and when we set the interval.  We distinguish between our internal
 * and the previous interval by looking at the old_timer value.
 * If it is invalid_timer, then it was the previous alarm interval,
 * otherwise it was our alarm interval.
 */
static void
my_alarm(int sig)
{
    /*
     * who's interval alarm timer went off?
     */
    if (memcmp(&old_timer, &invalid_timer, sizeof(struct itimerval)) == 0) {
	lava_ring |= LAVA_OLD_RING;
    } else {
	lava_ring |= LAVA_MY_RING;
    }
    return;
}


/*
 * lava_sleep - sleep for a period of time without touching timers and sigs
 *
 * given:
 *      secs    seconds to sleep
 */
void
lava_sleep(double secs)
{
    struct timespec tsecs;	/* secs as a timeval */

    /*
     * firewall
     */
    if (secs < LAVA_TINY_TIME) {
	/* neg, 0 or sub-tiny amount of time take nil time to sleep */
	return;
    }

    /*
     * convert secs to a timespec
     */
    tsecs.tv_sec = double_to_tv_sec(secs);
    tsecs.tv_nsec = double_to_tv_nsec(secs);

    /*
     * sleep and return
     */
    (void)nanosleep(&tsecs, NULL);
    return;
}


/*
 * right_now - return the time in Seconds since the Epoch
 *
 * returns:
 *      Seconds since the Epoch as a double, or -1.0 ==> error
 */
double
right_now(void)
{
    struct timeval now;	/* current time */

    /*
     * get time
     */
    if (gettimeofday(&now, NULL) < 0) {
	return -1.0;
    }
    return timeval_to_double(now);
}
