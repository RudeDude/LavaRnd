/*
 * rawio - low level I/O routines
 */
/*
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


#if !defined(__LAVARND_RAWIO_H__)
#define __LAVARND_RAWIO_H__


/*
 * alarm ring indication
 */
#define LAVA_NO_ALARM	  0	/* no alarm has rung */
#define LAVA_MY_RING	  0x1	/* our alarm call went off */
#define LAVA_OLD_RING	  0x2	/* previously set alarm went off */
#define LAVA_DUP_ALARM	  0x4	/* attempt to set alarm that was already set */
#define LAVA_ALARM_ERR	  0x8	/* error in setting up the alarm */
#define LAVA_NOSET_ALARM 0x10	/* the alarm was not previously set */
#define LAVA_RING_ERR	 0x1e	/* mask for alarm errors */

#define is_lava_ring(x) ((long)(x) == LAVA_MY_RING)
#define is_lava_ring_error(x) ((long)(x) & LAVA_RING_ERR)
#define lavaerr_ret(x) (is_lava_ring_error(x)?LAVAERR_ALARMERR:LAVAERR_TIMEOUT)

#define LAVA_TINY_TIME (0.005)	/* a short interval - usually ~1/2 clock tick */

#define double_to_tv_sec(x) ((long)(x))
#define double_to_tv_usec(x) ((long)((((double)(x) - \
				     (double)double_to_tv_sec(x))*1.0e6) + 0.5))
#define double_to_tv_nsec(x) ((long)((((double)(x) - \
				     (double)double_to_tv_sec(x))*1.0e9) + 0.5))
#define timeval_to_double(x) ((double)((x).tv_sec) + \
			      (((double)((x).tv_usec))/1.0e6))

/* NOTE: We pick 20 because 2^64 (max unsigned long long) is 20 digits */
#define LAVA_REQBUFLEN (20+1+1)	/* longest read of request size + \n + 1 */


/*
 * external functions and vars
 */
extern int set_alarm(double duration);
extern int set_simple_alarm(double duration);
extern int clear_alarm(void);
extern int clear_simple_alarm(void);
extern void lava_sleep(double secs);
extern double right_now(void);
extern void lavasocket_cleanup(void);


/*
 * low level I/O
 */
extern int raw_read(int fd, void *buf, size_t count, int chk_alarm);
extern int read_once(int fd, void *buf, size_t count, int chk_alarm);
extern int noblock_read(int fd, void *buf, size_t count, int chk_alarm);
extern int nilblock_read(int fd, void *buf, size_t count, int chk_alarm);
extern int raw_write(int fd, void *buf, size_t count, int chk_alarm);
extern int write_once(int fd, void *buf, size_t count, int chk_alarm);
extern int noblock_write(int fd, void *buf, size_t count, int chk_alarm);
extern int nilblock_write(int fd, void *buf, size_t count, int chk_alarm);
extern int noblock_fd(int fd);


/*
 * generic raw daemon socket open
 */
extern int lava_connect(char *);
extern int lava_listen(char *);

/*
 * external vars
 */
extern int lava_ring;			/* current alarm state */
extern double lava_duration;		/* time between set and clear alarm */
extern double lava_dur_arg;		/* duration requested for alarm */


#endif /* __LAVARND_RAWIO_H__ */
