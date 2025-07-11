/*
 * lavacam - generic lava camera definitions
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: lavacam.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


#if !defined(__LAVARND_LAVACAM_H__)
#define __LAVARND_LAVACAM_H__


/*
 * common driver constants
 */
#define BITS_PER_OCTET (8)		/* bits in an octet */
#define OCTET_CNT (1<<(BITS_PER_OCTET))	/* number of values in an octet */


/*
 * include each of the name_state.h driver state files here
 */
#include "LavaRnd/pwc_state.h"
#include "LavaRnd/ov511_state.h"
#include "LavaRnd/have/cam_videodev.h"


/*
 * lavacam - generic camera state and settings
 */
union lavacam {
    struct pwc_state pwc;	/* pwc - Philips Web Camera */
    struct ov511_state ov511;	/* ov511 - OmniVision OV511 Web Camera */
};


/*
 * opsize - how and where to read from the camera
 */
struct opsize {
    /* read and mmap size data */
    int readsize;	/* total frame read size, 0 ==> do not read */
    int read_lavaoff;	/* offset in buffer to LavaRnd process on read */
    int read_lavalen;	/* length after offset to LavaRnd process on read */
    int mmapsize;	/* total frame mmap size, 0 ==> do not mmap */
    int mmap_lavaoff;	/* offset in buffer to LavaRnd process on mmap */
    int mmap_lavalen;	/* length after offset to LavaRnd process on mmap */
    /* camera image size information */
    int framesize;	/* size of a frame in octets */
    int frames;		/* number of frames in region */
    int offsets[VIDEO_MAX_FRAME];	/* video_mbuf offsets for frames */
    int palette;	/* camera palette */
    int height;		/* image height in pixels */
    int width;		/* image width in pixels */
    /* working sanity check info - set by xyz_open() from stuct xyz_state */
    int top_x;		/* ignore top_x octets computing uncommon octet fract */
    double min_fract;	/* uncommon octet values must be at least this fract */
    int half_x;		/* half_x most common values must be <50% of octets */
    double diff_fract;	/* min fraction of different/same bits between frames */
    /* sanity check previous chaos frame comparison */
    void *prev_frame;	/* previous chaos frame (initialized to all 0's) */
    /* camera use fields */
    time_t open_time;		/* opening timestamp in time(2) format */
    u_int64_t frame_num;	/* current frame number generated */
    u_int64_t insane_cnt;	/* number of frames rejected due to insanity */
    /* read/mmap frame data */
    int use_read;	/* TRUE ==> using read, FALSE ==> using mmap */
    void *image;	/* pointer to start of read or mmap buffer or NULL */
    int image_len;	/* length of the image buffer, or 0 */
    void *chaos;	/* pointer to start of chaotic data in image or NULL */
    int chaos_len;	/* max length of the chaos segment if image, or 0 */
};
/*
 * NOTE: The offsets array is filled in by xxx_opsize() during the call
 *	 to lavacam_open.  It comes from the offsets element of the
 *	 struct video_mbuf as returned by the VIDIOCGMBUF ioctl.
 */


/*
 * lavacam_arg_flag - flags set via lavacam_argv()
 */
struct lavacam_flag {
    double D_flag;	/* -D delay time */
    double T_flag;	/* -T warm_up/sleep time  */
    int A_flag;		/* 0 ==> process only high entropy 1 ==> process all */
    int M_flag;		/* 0 ==> do not use mmap, use reads only */
    int v_flag;		/* -v verbose_level */
    int E_flag;		/* avoid frame dumping when savefile exists */
    char *program;	/* 1st arg */
    char *savefile;	/* optional savefile or NULL ==> no save */
    char *newfile;	/* write to newfile, then rename to savefile, NULL ==> none */
    double interval;	/* aprox interval in seconds to write savefile, <=0 ==> none */
};


/*
 * include each of the camera name_drvr.h driver files here
 */
#include "LavaRnd/pwc_drvr.h"
#include "LavaRnd/ov511_drvr.h"


/*
 * generic camera errors
 */
#include "LavaRnd/lavaerr.h"


/*
 * pallette spaces
 *
 * A common set of pallette values are found in <linux/videodev.h>.
 * When PALLETTE_VIDEO4LINUX is passed as the 'set' arg to chaos_zone(),
 * then the 'palette' arg is taken to be a VIDEO_PALETTE_XYZ constant
 * value found in <linux/videodev.h>.
 *
 * Other pallette spaces exist.  Should one want to use a pallette that
 * is not part of the Video4Linux pallette set, then simply add a new
 * PALLETTE_XYZ value below and modify the chaos_zone() function in
 * palette.c to understand the set=PALLETTE_XYZ, palette arg combination.
 */
#define PALLETTE_NONE (0)		/* not a valid pallette set */
#define PALLETTE_VIDEO4LINUX (1)	/* Video4Linux pallette set */


/*
 * external functions
 */
extern int camtype(char *type_name);
extern void lavacam_print_types(FILE *stream);
extern int lavacam_get(int type, int cam_fd, union lavacam *u_cam_p);
extern int lavacam_set(int type, int cam_fd, union lavacam *u_cam_p);
extern int lavacam_LavaRnd(int type, union lavacam *u_cam_p);
extern int lavacam_check(int type, union lavacam *u_cam_p);
extern int lavacam_print(int type, FILE *stream, int cam_fd,
		         union lavacam *cam, struct opsize *siz);
extern void lavacam_usage(int type, char *prog, char *typename);
extern int lavacam_argv(int type, int argc,char **argv,
			union lavacam *u_cam_p, struct lavacam_flag *flag);
extern int lavacam_open(int type, char *devname, union lavacam *o_cam,
			union lavacam *n_cam, struct opsize *siz,
			int def, struct lavacam_flag *flag);
extern int lavacam_close(int type, int cam_fd, struct opsize *siz,
			 struct lavacam_flag *flag);
extern int lavacam_get_frame(int type, int cam_fd, struct opsize *siz);
extern double lavacam_uncom_fract(u_int8_t *frame, int len, int top_x,
				  int *p_half_lvl);
extern double lavacam_bitdiff_fract(u_int8_t *frame1, u_int8_t *frame2,
				    int len);
extern int lavacam_sanity(struct opsize *siz);
extern int lavacam_msync(int type, int cam_fd, union lavacam *cam,
			 struct opsize *siz);
extern int lavacam_wait_frame(int type, int cam_fd, double max_wait);
extern int chaos_zone(int set, int palette, int frame_size,
		      int height, int width, int *chaos_offset, int *chaos_len);
extern const char *palette_name(int set, int palette);
extern int valid_palette(int set, int palette);
extern int frame_size(int set, int palette, int bufsize, int frames,
		      int *offsets, int height, int width);


#endif /* __LAVARND_LAVACAM_H__ */
