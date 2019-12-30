/*
 * pwc_state - Philips pwc camera state
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: pwc_state.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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

#if !defined(__LAVARND_PWC_STATE_H__)
#  define __LAVARND_PWC_STATE_H__


/*
 * pwc_state - Philips camera settings and state
 *
 * NOTE: not every Philips camera has each of these camera settings
 *
 * NOTE: The mask element tells pwc_set() what to set and pwc_check() what
 *       to check.
 *
 * NOTE: The channel element is set by pwc_argv() and is set by pwc_open()
 *       if the n_cam_p argument is non-NULL.
 *
 * NOTE: The order of these elements must be coordinated with the
 *	 static struct pwc_state optimal value(s) in pwc_drvr.c.
 */
struct pwc_state {
    /* these values are read/write */
    int bright;		/* brightness level */
    int hue;		/* hue */
    int colour;		/* colour saturation */
    int contrast;	/* image contrast level */
    int gamma;		/* brightness gamma */
    int height;		/* image height in pixels */
    int width;		/* image width in pixels */
    int fps;		/* camera frame rate */
    int compress;	/* camera compression (preference) */
    int white_mode;	/* auto/manual white balance mode */
    int white_red;	/* manual white balance red level */
    int white_blue;	/* manual white balance blue level */
    int on;		/* LED on time in msec */
    int off;		/* LED off time in msec */
    int sharpness;	/* electronic sharpness and sharpness */
    int backlight;	/* backlight compression */
    int flicker;	/* image flicker suppression */
    int dyn_noise;	/* dynamic noise reduction */
    /* there values are write only */
    int agc;		/* auto/manual gain control */
    int speed;		/* shutter speed - higher number ==> slower speed */
    /* these values are read only */
    int subtype;	/* numerical camera subtype */
    int minwidth;	/* minimum width in pixels */
    int maxwidth;	/* maximum width in pixels */
    int minheight;	/* minimum height in pixels */
    int maxheight;	/* maximum height in pixels */
    int read_red;	/* current white balance red level */
    int read_blue;	/* current white balance blue level */
    int bufsize;	/* image size in octets */
    int framesize;	/* size of a frame in octets */
    int frames;		/* number of video buffer frames */
    int depth;		/* pixel/picture depth in bits */
    int palette;	/* type of image palette */
    /* special driver/utilities elements */
    unsigned long mask;	/* bit mask of state values to set/change */
    int channel;	/* video channel number, usually 0, defaults to 0 */
    /* tmp sanity check into - initialized by default, set by pwc_argv() */
    int tmp_top_x;		/* tmp top_x struct opsize value */
    double tmp_min_fract;	/* tmp min_fract struct opsize value */
    int tmp_half_x;		/* tmp half_x struct opsize value */
    double tmp_diff_fract;	/* tmp diff_fract struct opsize value */
};


/*
 * state mask - what is set/modify
 */
#  define PWC_STATE_bright	0x00000001
#  define PWC_STATE_hue		0x00000002
#  define PWC_STATE_colour	0x00000004
#  define PWC_STATE_contrast	0x00000008
#  define PWC_STATE_gamma	0x00000010
#  define PWC_STATE_height	0x00000020
#  define PWC_STATE_width	0x00000040
#  define PWC_STATE_fps		0x00000080
#  define PWC_STATE_compress	0x00000100
#  define PWC_STATE_white_mode	0x00000200
#  define PWC_STATE_white_red	0x00000400
#  define PWC_STATE_white_blue	0x00000800
#  define PWC_STATE_on		0x00001000
#  define PWC_STATE_off		0x00002000
#  define PWC_STATE_agc		0x00004000
#  define PWC_STATE_speed	0x00008000
#  define PWC_STATE_sharpness	0x00010000
#  define PWC_STATE_backlight	0x00020000
#  define PWC_STATE_flicker	0x00040000
#  define PWC_STATE_dyn_noise	0x00080000

#  define PWC_STATE_MASK		0x000fffff

/* the state mask value for a given PWC_STATE_name */
#  define PWC_MASK(name) PWC_STATE_##name

/* set in variable x, the state mask value for PWC_STATE_name */
#  define PWC_SET(x,name) ((x) |= PWC_MASK(name))

/* clear in variable x, the state mask value for PWC_STATE_name */
#  define PWC_CLEAR(x,name) ((x) &= ~PWC_MASK(name))

/* true (non-0) if variable x has the state mask value PWC_STATE_name set */
#  define PWC_TEST(x,name) ((x) & PWC_MASK(name))


#endif /* __LAVARND_PWC_STATE_H__ */
