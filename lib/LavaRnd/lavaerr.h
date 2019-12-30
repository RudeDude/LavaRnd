/*
 * lavaerr - lava
 *
 * We use the LavaRnd random number generator service.  See:
 *
 *	http://www.LavaRnd.com
 *
 * for details.
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


#if !defined(__LAVARND_LAVAERR_H__)
#define __LAVARND_LAVAERR_H__


/*
 * truth - as we know it
 */
#if !defined(TRUE)
# define TRUE 1
#endif
#if !defined(FALSE)
# define FALSE 0
#endif


/*
 * return codes for raw I/O calls
 *
 * NOTE: You should update the lava_err_name() function in lava_debug.c
 *	 when you add/change/remove LAVAERR_XYZ values below.
 */
#define LAVAERR_OK (0)		/* not an error */
#define LAVAERR_MALLOC (-1)	/* unable to alloc or realloc data */
#define LAVAERR_TIMEOUT (-2)	/* request did not respond in time */
#define LAVAERR_PARTIAL (-3)	/* request returned less than maximum data */
#define LAVAERR_BADARG (-4)	/* invalid argument passed to function */
#define LAVAERR_SOCKERR (-5)	/* error in creating a socket */
#define LAVAERR_OPENERR (-6)	/* error opening socket to daemon */
#define LAVAERR_BADADDR (-7)	/* unknown host, port or missing socket path */
#define LAVAERR_ALARMERR (-8)	/* unexpected error during alarm set or clear */
#define LAVAERR_IOERR (-9)	/* error during I/O operation */
#define LAVAERR_BADDATA (-10)	/* data received was malformed */
#define LAVAERR_TINYDATA (-11)	/* data received was too small */
#define LAVAERR_IMPOSSIBLE (-12) /* an impossible internal error occurred */
#define LAVAERR_NONBLOCK (-13)  /* I/O operation would block */
#define LAVAERR_BINDERR (-14)	/* unable to bind socket to address */
#define LAVAERR_CONNERR (-15)	/* unable to connect socket to address */
#define LAVAERR_LISTENERR (-16)	/* unable to listen on socket */
#define LAVAERR_ACCEPTERR (-17)	/* error in accepting new socket connection */
#define LAVAERR_FCNTLERR (-18)	/* fcntl or ioctl failed */
#define LAVAERR_BADCFG (-19)	/* failed in lavapool configuration */
#define LAVAERR_POORQUAL (-20)	/* data quality is below min allowed level */
#define LAVAERR_ENV_PARSE (-21)	/* parse error on env variable $LAVA_DEBUG */
#define LAVAERR_NO_DEBUG (-22)	/* library compiled without -DLAVA_DEBUG */
#define LAVAERR_SIGERR (-23)    /* signal related error */
#define LAVAERR_GETTIME (-24)	/* unexpected gettimeofday() error */
#define LAVAERR_TOOMUCH (-25)	/* requested too much data */
#define LAVAERR_BADLEN (-26)	/* requested length invalid */
#define LAVAERR_EOF (-27)	/* EOF during I/O operation */
#define LAVAERR_PALSET (-28)	/* unknown pallette set */
#define LAVAERR_PALETTE (-29)	/* pallette value not valid for pallette set */
#define LAVAERR_PERMOPEN (-30)	/* open failed due to file permissions */


/*
 * generic camera errors
 *
 * NOTE: These error codes < -99 are reserved for things like driver errors.
 *
 * NOTE: You should update the lava_err_name() function in lava_debug.c
 *	 when you add/change/remove LAVAERR_XYZ values below.
 */
#define LAVACAM_ERR_OK (0)	    /* not an error, all is OK */
#define LAVACAM_ERR_ARG (-100)	    /* bad function argument */
#define LAVACAM_ERR_NOCAM (-101)    /* not a camera or camera is not working */
#define LAVACAM_ERR_IDENT (-102)    /* could not identify camera name */
#define LAVACAM_ERR_TYPE (-103)	    /* wrong camera type */
#define LAVACAM_ERR_GETPARAM (-104) /* could not get a camera setting value */
#define LAVACAM_ERR_SETPARAM (-105) /* could not set a camera setting value */
#define LAVACAM_ERR_RANGE (-106)    /* masked camera setting is out of range */
#define LAVACAM_ERR_OPEN (-107)	    /* failed to open camera */
#define LAVACAM_ERR_CLOSE (-108)    /* failed to close camera */
#define LAVACAM_ERR_NOSIZE (-109)   /* unable to determine read/mmap size */
#define LAVACAM_ERR_SYNC (-110)	    /* mmap buffer release error */
#define LAVACAM_ERR_WARMUP (-111)   /* error during warm up after open */
#define LAVACAM_ERR_PALETTE (-112)  /* camera does not support the palette */
#define LAVACAM_ERR_CHAN (-113)	    /* failed to get/set video channel */
#define LAVACAM_ERR_NOREAD (-114)   /* cannot read, try mmap instead */
#define LAVACAM_ERR_NOMMAP (-115)   /* cannot mmap, try read instead */
#define LAVACAM_ERR_IOERR (-116)    /* I/O error */
#define LAVACAM_ERR_FRAME (-117)    /* frame read turned partial data */
#define LAVACAM_ERR_SIGERR (-118)   /* signal related error */
#define LAVACAM_ERR_UNCOMMON (-119) /* top_x common values are too common */
#define LAVACAM_ERR_OVERSAME (-120) /* too many bits similar with prev frame */
#define LAVACAM_ERR_OVERDIFF (-121) /* too few bits similar with prev frame */
#define LAVACAM_ERR_HALFLVL (-122)  /* top half_x is more than half of frame */
#define LAVACAM_ERR_PALUNSET (-123) /* pallette has not been set for camera */


#endif /* __LAVARND_LAVAERR_H__ */
