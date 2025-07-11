/*
 * palette - determine chaotic data location in a buffer for a given palette
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: palette.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <sys/types.h>

#include "LavaRnd/lavacam.h"
#include "LavaRnd/have/cam_videodev.h"
#include "LavaRnd/lavaerr.h"


/*
 * chaos_zone - for a given pallette, determine where chaos is found in buffer
 *
 * given:
 *	set		pallette set (usually PALLETTE_VIDEO4LINUX)
 *	palette		Video4Linux palette number (see <linux/videodev.h>)
 *	frame_size	size of frame containing palette encoded data
 *	height		height of frame in pixels (or 0)
 *	width		width of frame in pixels (or 0)
 *	chaos_offset	ptr to offset in frame where chaotic data resides
 *	chaos_len	ptr to length of chaotic data in frame
 *
 * The conbination of space and palette define a unique encoding.
 *
 * returns:
 *	0 (LAVAERR_OK) ==> sucessfull data return
 *	<0 ==> error
 */
int
chaos_zone(int set, int palette, int frame_size,
	   int height, int width, int *chaos_offset, int *chaos_len)
{
    int offset;		/* chaos_offset value to return */
    int len;		/* chaos_len value to return */
    int pixels;		/* number of pixels in frame, or 0 */

    /*
     * firewall
     */
    if (frame_size <= 0 || chaos_offset == NULL || chaos_len == NULL) {
	return LAVAERR_BADARG;
    }

    /*
     * assume we will use entire buffer unless otherwise stated
     */
    offset = 0;
    pixels = height * width;	/* might use frame_size if pixels is 0 */
    len = 0;

    /*
     * determine buffer start and length fraction based on set & palette
     */
    switch (set) {

    /*
     * Video4Linux palette set <linux/videodev.h>
     */
    case PALLETTE_VIDEO4LINUX:
	switch (palette) {
	case 0:				/* unspecified palette */
	    return LAVACAM_ERR_PALUNSET;
	    /*NOTREACHED*/
	case VIDEO_PALETTE_GREY:	/* Linear greyscale */
	    /* entire buffer is grey scale, use all of buffer */
	    len = pixels;
	    if (len <= 0) {
	    	len = frame_size;
	    }
	    break;
	case VIDEO_PALETTE_HI240:	/* High 240 cube (BT848) */
	    /* BT848 is used for raw capture frame, use all of buffer */
	    len = frame_size;
	    break;
	case VIDEO_PALETTE_RGB565:	/* 565 16 bit RGB */
	    /* 5 bits red / 6 bits green / 5 bits blue - 16 bits per pixel */
	    /* 2 octets per pixel */
	    /* RGB mixed thruout buffer, use all of buffer */
	    len = 2 * pixels;
	    if (len <= 0) {
	    	len = frame_size;
	    }
	    break;
	case VIDEO_PALETTE_YUV422:	/* YUV422 capture */
	    /* YYUV octet pattern?, Y spread thruout frame, use all of buffer */
	    /* 2 octets per pixel */
	    /* XXX - is this correct or is it like YUV422 planar? */
	    len = 2 * pixels;
	    if (len <= 0) {
	    	len = frame_size;
	    }
	    break;
	case VIDEO_PALETTE_RGB24:	/* 24bit RGB */
	    /* 1 octet red, 1 octet green, 1 octet blue */
	    /* 3 octets per pixel */
	    /* RGB mixed thruout buffer, use all of buffer */
	    len = 3 * pixels;
	    if (len <= 0) {
	    	len = frame_size;
	    }
	    break;
	case VIDEO_PALETTE_RGB32:	/* 32bit RGB */
	    /* 1 octet red, 1 octet green, 1 octet blue, 1 octet unused */
	    /* 4 octets per pixel */
	    /* RGB mixed thruout buffer with unused octet, use all of buffer */
	    len = 4 * pixels;
	    if (len <= 0) {
	    	len = frame_size;
	    }
	    break;
	case VIDEO_PALETTE_RGB555:	/* 555 15bit RGB */
	    /* 5 bits red / 6 bits green / 5 bits blue - 16 bits per pixel */
	    /* 2 octets per pixel */
	    /* RGB mixed thruout buffer with unused bits, use all of buffer */
	    len = 2 * pixels;
	    if (len <= 0) {
	    	len = frame_size;
	    }
	    break;
	case VIDEO_PALETTE_YUYV:	/* YUYV mix */
	    /* 1 octet Y, 1 octet U, 1 octet Y, 1 octet V */
	    /* each 4 octets is 2 pixels - 2 octets per pixel */
	    /* because Y is mixed thruout buffer, use all of buffer */
	    len = 2 * pixels;
	    if (len <= 0) {
	    	len = frame_size;
	    }
	    break;
	case VIDEO_PALETTE_UYVY:	/* UYVY */
	    /* 1 octet U, 1 octet Y, 1 octet V, 1 octet Y */
	    /* each 4 octets is 2 pixels - 2 octets per pixel */
	    /* because Y is mixed thruout buffer, use all of buffer */
	    len = 2 * pixels;
	    if (len <= 0) {
	    	len = frame_size;
	    }
	    break;
	case VIDEO_PALETTE_YUV420:	/* YUV 4:2:2 */
	    /* YYYYUV octet pattern?, Y is thruout frame, use all of buffer */
	    /* 1.5 octets per pixel */
	    /* XXX - is this correct or is it like YUV420 planar? */
	    len = 3 * pixels / 2;
	    if (len <= 0) {
	    	len = frame_size;
	    }
	    break;
	case VIDEO_PALETTE_YUV411:	/* YUV411 capture */
	    /* UYVY octet pattern, Y is thruout frame, use all of buffer */
	    /* each 12 octets is 8 pixels - 1.5 octets per pixel */
	    /* XXX - is this correct or is it like YUV411 planar? */
	    len = 3 * pixels / 2;
	    if (len <= 0) {
	    	len = frame_size;
	    }
	    break;
	case VIDEO_PALETTE_RAW:		/* RAW capture (BT848) */
	    /* raw encoding, use all of buffer */
	    len = frame_size;
	    break;
	case VIDEO_PALETTE_YUV422P:	/* YUV 4:2:2 Planar */
	    /* frame is 1/2 Y, 1/4 U, 1/4 V, use only Y part of frame */
	    /* leads with 1 Y octet per pixel */
	    len = pixels;
	    if (len <= 0) {
	    	len = frame_size / 2;
	    }
	    break;
	case VIDEO_PALETTE_YUV420P:	/* YUV 4:2:0 Planar */
	    /* frame is 2/3 Y, 1/6 U, 1/6 V, use only Y part of frame */
	    /* leads with 1 Y octet per pixel */
	    len = pixels;
	    if (len <= 0) {
	    	len = 2*frame_size / 3;
	    }
	    break;
	case VIDEO_PALETTE_YUV410P:	/* YUV 4:1:0 Planar */
	    /* frame is 8/9 Y, 1/18 U, 1/18 V, use only Y part of frame */
	    /* leads with 1 Y octet per pixel */
	    len = pixels;
	    if (len <= 0) {
		len = 8*frame_size / 9;
	    }
	    break;
	default:
	    return LAVAERR_PALETTE;
	    /*NOTREACHED*/
	    break;
	}
	break;

    /*
     * unknown palette set
     */
    default:
    	return LAVAERR_PALSET;
	/*NOTREACHED*/
    }

    /*
     * firewall
     */
    /* must not start after end of frame - catch rounding errors */
    if (offset >= frame_size) {
    	offset = frame_size-1;
    }
    /* must not start before frame - catch rounding errors */
    if (offset < 0) {
    	offset = 0;
    }
    /* must not go beyond end of frame - catch rounding errors */
    if (offset+len > frame_size) {
	len = frame_size - offset;
    }

    /*
     * return values
     */
    *chaos_offset = offset;
    *chaos_len = len;
    return LAVACAM_ERR_OK;
}


/*
 * palette_name - return name of a palette
 *
 * given:
 *	set		pallette set (usually PALLETTE_VIDEO4LINUX)
 *	palette		Video4Linux palette number (see <linux/videodev.h>)
 *
 * returns:
 *	constant string containing the extended name of the palette
 */
const char *
palette_name(int set, int palette)
{

    /*
     * determine palette name based on the palette set
     */
    switch (set) {

    /*
     * Video4Linux palette set <linux/videodev.h>
     */
    case PALLETTE_VIDEO4LINUX:
	/*
	 * print palette information
	 */
	switch (palette) {
	case 0:
	    return "Palette: not set";
	case VIDEO_PALETTE_GREY:
	    return "Palette: Linear greyscale";
	case VIDEO_PALETTE_HI240:
	    return "Palette: High 240 cube (BT848)";
	case VIDEO_PALETTE_RGB565:
	    return "Palette: 565 16 bit RGB";
	case VIDEO_PALETTE_RGB24:
	    return "Palette: 24bit RGB";
	case VIDEO_PALETTE_RGB32:
	    return "Palette: 32bit RGB";
	case VIDEO_PALETTE_RGB555:
	    return "Palette: 555 15bit RGB";
	case VIDEO_PALETTE_YUV422:
	    return "Palette: YUV422 capture";
	case VIDEO_PALETTE_YUYV:
	    return "Palette: YUYV capture";
	case VIDEO_PALETTE_UYVY:
	    return "Palette: UYVY capture";
	case VIDEO_PALETTE_YUV420:
	    return "Palette: YUV 4:2:0";
	case VIDEO_PALETTE_YUV411:
	    return "Palette: YUV411 capture";
	case VIDEO_PALETTE_RAW:
	    return "Palette: RAW capture (BT848)";
	case VIDEO_PALETTE_YUV422P:
	    return "Palette: YUV 4:2:2 Planar";
	case VIDEO_PALETTE_YUV411P:
	    return "Palette: YUV 4:1:1 Planar";
	case VIDEO_PALETTE_YUV420P:
	    return "Palette: YUV 4:2:0 Planar";
	case VIDEO_PALETTE_YUV410P:
	    return "Palette: YUV 4:1:0 Planar";
	default:
	    return "Unknown Video4Linux palette";
	}
	break;
    }
    return "Unknown palette set";
}


/*
 * valid_palette - determine if we have not set a valid palette
 *
 * given:
 *	set		pallette set (usually PALLETTE_VIDEO4LINUX)
 *	palette		Video4Linux palette number (see <linux/videodev.h>)
 *
 * returns:
 *	0 (LAVAERR_OK) ==> the set/palette is a valid palette
 *	<0 ==> not a valid palette or set
 *
 * NOTE: Returns LAVACAM_ERR_PALUNSET if the palette has not been set.
 */
int
valid_palette(int set, int palette)
{

    /*
     * look at the set / palette combimation
     */
    switch (set) {

    /*
     * Video4Linux palette set <linux/videodev.h>
     */
    case PALLETTE_VIDEO4LINUX:
	/*
	 * print palette information
	 */
	switch (palette) {
	case 0:
	    /* palette not set */
	    return LAVACAM_ERR_PALUNSET;
	case VIDEO_PALETTE_GREY:
	case VIDEO_PALETTE_HI240:
	case VIDEO_PALETTE_RGB565:
	case VIDEO_PALETTE_RGB24:
	case VIDEO_PALETTE_RGB32:
	case VIDEO_PALETTE_RGB555:
	case VIDEO_PALETTE_YUV422:
	case VIDEO_PALETTE_YUYV:
	case VIDEO_PALETTE_UYVY:
	case VIDEO_PALETTE_YUV420:
	case VIDEO_PALETTE_YUV411:
	case VIDEO_PALETTE_RAW:
	case VIDEO_PALETTE_YUV422P:
	case VIDEO_PALETTE_YUV411P:
	case VIDEO_PALETTE_YUV420P:
	case VIDEO_PALETTE_YUV410P:
	    /* valid palette */
	    return LAVACAM_ERR_OK;
	default:
	    /* invalid palette */
	    return LAVAERR_PALETTE;
	}
	break;
    }
    /* invalid set */
    return LAVAERR_PALSET;
}


/*
 * frame_size - determine size of a frame
 *
 * We attempt to determine size of a frame by one of 3 methods.  The first
 * method to work is used:
 *
 *    1) If the offsets array was supplied, and frames > 1, and the
 *	 diffence between the first and second offset > 0, then
 *	 use the difference of the first 2 offsets.
 *
 *    2) If bufsize > 0 and frames > 0, then use bufsize/frames.
 *
 *    3) Use the palette, height and width to determine frame size.
 *
 * given:
 *	set		pallette set (usually PALLETTE_VIDEO4LINUX)
 *	palette		Video4Linux palette number (see <linux/videodev.h>)
 *	bufsize		size of buffer for all frames (or 0)
 *	frames		number of frames in bufsize (or 0)
 *	offsets		array of frame offsets within buffer, or NULL ==> none
 *	height		height of frame in pixels (or 0)
 *	width		width of frame in pixels (or 0)
 *
 * returns:
 *	>0 ==> size of a frame in octets
 *	<0 ==> could not estimate size of a frame (LAVACAM_ERR_NOSIZE returned)
 *
 * NOTE: The offsets array must be at least 2 elements long.  Typically it
 *	 a copy of data returned in the struct video_mbuf by the
 *	 VIDIOCGMBUF ioctl.
 */
int
frame_size(int set, int palette, int bufsize, int frames, int *offsets,
	   int height, int width)
{
    int frame_size = 0;		/* estimated frame size */

    /*
     * use the difference of the first 2 offsets if possible
     */
    if (offsets != NULL && frames > 1 && offsets[0] >= 0 &&
    	offsets[1] > offsets[0]) {
	frame_size = offsets[1] - offsets[0];
	return frame_size;
    }

    /*
     * use bufsize & frame count calculation if possible
     */
    if (bufsize > 0 && frames > 0) {
	frame_size = bufsize/frames;
	return frame_size;
    }

    /*
     * given height * width pixels, use set / palette to determine image size
     */
    if (height > 0 && width > 0) {
	switch (set) {

	/*
	 * Video4Linux palette set <linux/videodev.h>
	 */
	case PALLETTE_VIDEO4LINUX:

	    /*
	     * print palette information
	     */
	    switch (palette) {
	    case 0:
		/* palette not set */
		break;
	    case VIDEO_PALETTE_GREY:
		frame_size = height * width;
		break;
	    case VIDEO_PALETTE_HI240:
		/* cannot determine size of raw frame */
		break;
	    case VIDEO_PALETTE_RGB565:
		frame_size = 2 * height * width;
		break;
	    case VIDEO_PALETTE_RGB24:
		frame_size = 3 * height * width;
		break;
	    case VIDEO_PALETTE_RGB32:
		frame_size = 4 * height * width;
		break;
	    case VIDEO_PALETTE_RGB555:
		frame_size = 2 * height * width;
		break;
	    case VIDEO_PALETTE_YUV422:
		frame_size = 2 * height * width;
		break;
	    case VIDEO_PALETTE_YUYV:
		frame_size = 2 * height * width;
		break;
	    case VIDEO_PALETTE_UYVY:
		frame_size = 2 * height * width;
		break;
	    case VIDEO_PALETTE_YUV420:
		frame_size = 3 * height * width / 2;
		break;
	    case VIDEO_PALETTE_YUV411:
		frame_size = 3 * height * width / 2;
		break;
	    case VIDEO_PALETTE_RAW:
		/* cannot determine size of raw frame */
		break;
	    case VIDEO_PALETTE_YUV422P:
		frame_size = 2 * height * width;
		break;
	    case VIDEO_PALETTE_YUV411P:
		frame_size = 3 * height * width / 2;
		break;
	    case VIDEO_PALETTE_YUV420P:
		frame_size = 3 * height * width / 2;
		break;
	    case VIDEO_PALETTE_YUV410P:
		frame_size = (height * width) + (height * width / 16);
		break;
	    default:
		/* invalid palette */
		break;
	    }
	    break;

	/* invalid set */
	default:
	    break;
	}
    }

    /*
     * report error if no frame calculation is possible
     */
    if (frame_size <= 0) {
	return LAVACAM_ERR_NOSIZE;
    }

    /*
     * return frame size
     */
    return frame_size;
}
