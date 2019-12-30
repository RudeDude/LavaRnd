/*
 * ov511_state - OmniVision OV511 camera state
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: ov511_state.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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

#if !defined(__LAVARND_OV511_STATE_H__)
#  define __LAVARND_OV511_STATE_H__


/*
 * ov511_state - OmniVision OV511 camera settings and state
 *
 * NOTE: not every OmniVision OV511 camera has each of these camera settings
 *
 * NOTE: The mask element tells ov511_set() what to set and ov511_check() what
 *       to check.
 *
 * NOTE: The channel element is set by ov511_argv() and is set by ov511_open()
 *       if the n_cam_p argument is non-NULL.
 *
 * NOTE: The order of these elements must be coordinated with the
 *	 static struct ov511_state optimal value(s) in ov511_drvr.c.
 */
struct ov511_state {
    /* these values are read/write */
    int bright;		/* brightness level */
    int hue;		/* hue */
    int colour;		/* colour saturation */
    int contrast;	/* image contrast level */
    int height;		/* image height in pixels */
    int width;		/* image width in pixels */
    int palette;	/* type of image palette */
    /* there values are write only */
    /* these values are read only */
    int subtype;	/* numerical camera subtype */
    int minwidth;	/* minimum width in pixels */
    int maxwidth;	/* maximum width in pixels */
    int minheight;	/* minimum height in pixels */
    int maxheight;	/* maximum height in pixels */
    int bufsize;	/* image size in octets */
    int framesize;	/* size of a frame in octets */
    int frames;		/* number of video buffer frames */
    int depth;		/* pixel/picture depth in bits */
    /* special driver/utilities elements */
    unsigned long mask;	/* bit mask of state values to set/change */
    int channel;	/* video channel number, usually 0, defaults to 0 */
    /* tmp sanity check into - initialized by default, set by ov511_argv() */
    int tmp_top_x;		/* tmp top_x struct opsize value */
    double tmp_min_fract;	/* tmp min_fract struct opsize value */
    int tmp_half_x;		/* tmp half_x struct opsize value */
    double tmp_diff_fract;	/* tmp diff_fract struct opsize value */
};


/*
 * state mask - what is set/modify
 */
#  define OV511_STATE_bright		0x00000001
#  define OV511_STATE_hue		0x00000002
#  define OV511_STATE_colour		0x00000004
#  define OV511_STATE_contrast		0x00000008
#  define OV511_STATE_height		0x00000010
#  define OV511_STATE_width		0x00000020
#  define OV511_STATE_palette		0x00000040

#  define OV511_STATE_MASK		0x0000007f

/* the state mask value for a given OV511_STATE_name */
#  define OV511_MASK(name) OV511_STATE_##name

/* set in variable x, the state mask value for OV511_STATE_name */
#  define OV511_SET(x,name) ((x) |= OV511_MASK(name))

/* clear in variable x, the state mask value for OV511_STATE_name */
#  define OV511_CLEAR(x,name) ((x) &= ~OV511_MASK(name))

/* true (non-0) if variable x has the state mask value OV511_STATE_name set */
#  define OV511_TEST(x,name) ((x) & OV511_MASK(name))


#endif /* __LAVARND_OV511_STATE_H__ */
