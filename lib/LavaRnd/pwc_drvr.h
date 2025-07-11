/*
 * pwc_drvr - Philips pwc camera definitions and driver interface
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: pwc_drvr.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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

#if !defined(__LAVARND_PWC_DRVR_H__)
#  define __LAVARND_PWC_DRVR_H__


/*
 * required includes
 */
#  include <sys/types.h>
#  include <sys/ioctl.h>

#  include "LavaRnd/have/pwc_cam.h"


/*
 * pwc defines
 */
#  define PWC_MAX_AGC 65535	/* MAX and neg-MAX AGC */
#  define PWC_UNSET_PICT 0xffff	/* picture value has no meaning */
#  define PWC_MIN_PICT 0	/* minimum picture value */
#define PWC_MAX_PICT (PWC_UNSET_PICT-1)	/* maximum picture value */
#  define PWC_MIN_SIZE 1	/* minimum pixel width & height */
#  define PWC_MIN_FPS 1		/* minimum frames per second */
#  define PWC_MIN_COMPRESSION 0	/* minimum (no) compression */
#  define PWC_MAX_COMPRESSION 3	/* maximum compression */
#  define PWC_AUTO_AGC (-1)	/* automatic AGC */
#  define PWC_MIN_AGC (-1)	/* minimum (auto) AGC */
#  define PWC_MAX_AGC 65535	/* maximum (non-auto) AGC */
#  define PWC_AUTO_SPEED (-1)	/* automatic shutter speed */
#  define PWC_MIN_SPEED (-1)	/* minimum (auto) shutter speed */
#  define PWC_MAX_SPEED 65535	/* maximum shutter speed */
#  define PWC_MANUAL_WH_BAL 3	/* manual white balance mode */
#  define PWC_MIN_WH_BAL_MODE 0	/* min white balance mode */
#  define PWC_MAX_WH_BAL_MODE 4	/* max white balance mode */
#  define PWC_MIN_WH_BAL_LEVEL 0/* min white balance level */
#  define PWC_MAX_WH_BAL_LEVEL 65535	/* max white balance level */
#  define PWC_MIN_LED_TIME 0	/* minimum LED time */
#  define PWC_MAX_LED_TIME 25000/* maximum LED time */
#define PWC_UNSET_LED_TIME (-2)		/* our convention for unset LED times */
#define PWC_AUTO_SHARPNESS (-1)		/* automatic electronic sharpness */
#  define PWC_MIN_SHARPNESS (-1)/* low electronic sharpness */
#  define PWC_MAX_SHARPNESS (65536)	/* high electronic sharpness */
#  define PWC_UNSET_BACKLIGHT (-2)	/* our value for unset back-light */
#  define PWC_UNSET_FLICKER (-2)/* our value for unset flicker */
#  define PWC_MIN_DYNNOISE 0	/* no dynamic noise reduction */
#  define PWC_MAX_DYNNOISE 3	/* max dynamic noise reduction */

/*
 * external functions
 */
extern int pwc_get(int cam_fd, union lavacam *u_cam_p);
extern int pwc_set(int cam_fd, union lavacam *u_cam_p);
extern int pwc_LavaRnd(union lavacam *u_cam_p, int model);
extern int pwc_check(union lavacam *u_cam_p);
extern int pwc_print(FILE * stream, int cam_fd,
		     union lavacam *u_cam_p, struct opsize *siz);
extern void pwc_usage(char *prog, char *typename);
extern int pwc_argv(int argc, char **argv, union lavacam *u_cam_p,
		    struct lavacam_flag *flag, int model);
extern int pwc_open(char *devname, int model, union lavacam *o_cam,
		    union lavacam *n_cam, struct opsize *siz, int def,
		    struct lavacam_flag *flag);
extern int pwc_close(int cam_fd, struct opsize *siz,
		     struct lavacam_flag *flag);
extern int pwc_get_frame(int cam_fd, struct opsize *siz);
extern int pwc_msync(int cam_fd, union lavacam *u_cam_p, struct opsize *siz);
extern int pwc_wait_frame(int cam_fd, double max_wait);


#endif /* __LAVARND_PWC_DRVR_H__ */
