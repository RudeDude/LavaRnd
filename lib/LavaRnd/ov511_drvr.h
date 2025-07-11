/*
 * ov511_drvr - OmniVision OV511 camera definitions and driver interface
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: ov511_drvr.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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

#if !defined(__LAVARND_OV511_DRVR_H__)
#  define __LAVARND_OV511_DRVR_H__


/*
 * required includes
 */
#  include <sys/types.h>
#  include <sys/ioctl.h>

#  include "LavaRnd/have/ov511_cam.h"
#  include "LavaRnd/have/cam_videodev.h"


/*
 * ov511 defines
 */
#  define OV511_UNSET_PICT 65536	/* picture value has no meaning */
#  define OV511_DEF_BRIGHT 48640	/* with autobright=0, ave pixel 128 */
#  define OV511_MIN_PICT 0		/* minimum picture value */
#  define OV511_MAX_PICT 65535		/* maximum picture value */
#  define OV511_MIN_SIZE 1		/* minimum pixel width & height */
#  define OV511_MIN_PALETTE VIDEO_PALETTE_GREY	   /* min palette number */
#  define OV511_MAX_PALETTE VIDEO_PALETTE_YUV410P  /* max palette number */
#  define OV511_DEF_PALETTE VIDEO_PALETTE_YUV420P  /* 24bit RGB */

/*
 * external functions
 */
extern int ov511_get(int cam_fd, union lavacam *u_cam_p);
extern int ov511_set(int cam_fd, union lavacam *u_cam_p);
extern int ov511_LavaRnd(union lavacam *u_cam_p, int model);
extern int ov511_check(union lavacam *u_cam_p);
extern int ov511_print(FILE * stream, int cam_fd,
		     union lavacam *u_cam_p, struct opsize *siz);
extern void ov511_usage(char *prog, char *typename);
extern int ov511_argv(int argc, char **argv, union lavacam *u_cam_p,
		    struct lavacam_flag *flag, int model);
extern int ov511_open(char *devname, int model, union lavacam *o_cam,
		    union lavacam *n_cam, struct opsize *siz, int def,
		    struct lavacam_flag *flag);
extern int ov511_close(int cam_fd, struct opsize *siz,
		     struct lavacam_flag *flag);
extern int ov511_get_frame(int cam_fd, struct opsize *siz);
extern int ov511_msync(int cam_fd, union lavacam *u_cam_p, struct opsize *siz);
extern int ov511_wait_frame(int cam_fd, double max_wait);


#endif /* __LAVARND_OV511_DRVR_H__ */
