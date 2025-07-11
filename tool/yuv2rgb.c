/*
 * yuv2rgb - convert a buffer of YUV4:2:0 planar (YUV420P) to RGB
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: yuv2rgb.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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

/*
 * The YUV4:2:0 planar format is:
 *
 *      2/3 block of Y values
 *      1/6 block of U values
 *      1/6 block of V values
 *
 * There are 1.5 octets per pixel.  There are 4 Y octets for every U octet.
 * There are 4 Y octets for every V octet.  There is 1 U octet for every V.
 *
 * The RGB format is:
 *
 *      R octet, G octet, B octet
 *
 * There are 3 octets per pixel.
 */

#include <stdio.h>
#include <sys/types.h>

#include "yuv2rgb.h"


/*
 * yuv2rgb - convert a buffer of YUV4:2:0 planar (YUV420P) to RGB
 *
 * given:
 *      yuv     pointer to start of YUV input buffer
 *      rgb     pointer to start of RGB output buffer
 *      width   width of image in pixels
 *      height  height of image in pixels
 *
 * returns:
 *      pointer to start of RGB output buffer, or NULL if error (bad args)
 *
 * YUV is defined to have the following relationship with RGB:
 *
 *      Y = 16 + (0.587*G + 0.299*R + 0.114*B)
 *      U = 128 + 0.493 * (B-Y)
 *      V = 128 + 0.877 * (R-Y)
 * or:
 *      Y = 16 + (299.0/1000.0)*R + (587.0/1000.0)*G + (114.0/1000.0)*B
 *      U = 128 + 493.0 * (B-Y) / 1000.0
 *      V = 128 + 877.0 * (R-Y) / 1000.0
 *
 * so therefore, the following is the inverse relationship:
 *
 *      R = (Y-16) + (1000.0/877.0)*(V-128)
 *      G = (Y-16) - (147407000.0/253795907.0)*(V-128)
 *                 - (99978000.0/253795907.0)*(U-128)
 *      B = (Y-16) + (1000.0/493.0)*(U-128)
 */
u_int8_t *
yuv2rgb(u_int8_t * yuv, u_int8_t * rgb, int width, int height)
{
    u_int8_t *y_buf;	/* pointer to current Y luminance value */
    u_int8_t *u_buf;	/* pointer to current U pseudo-colour value */
    u_int8_t *v_buf;	/* pointer to current V pseudo-colour value */
    u_int8_t *rgb_buf;	/* pointer to current RGB value */
    int x;	/* column number */
    int y;	/* row number */

    /*
     * firewall
     */
    if (yuv == NULL || rgb == NULL || width <= 0 || height <= 0) {
	return NULL;
    }
    /*
     * determine Y, U and V starting points and steps
     */
    y_buf = yuv;
    u_buf = y_buf + width * height;
    v_buf = u_buf + width * height / 4;
    rgb_buf = rgb;

    /*
     * process a row at a time
     */
    for (y = 0; y < height; ++y) {

	/*
	 * process each pixel in the row
	 */
	for (x = 0; x < width; ++x) {
	    int Y, U, V;	/* YUV values */
	    int R, G, B;	/* RGB values */

	    /* obtain the normalized Y, U and V values for the pixel */
	    Y = *(y_buf++) - 16;
	    U = u_buf[x >> 1] - 128;
	    V = v_buf[x >> 1] - 128;

	    /* convert to RGB */
	    R = Y + (1000.0 / 877.0) * V;
	    G =
	      Y - (147407000.0 / 253795907.0) * V -
	      (99978000.0 / 253795907.0) * U;
	    B = Y + (1000.0 / 493.0) * U;

	    /* store normalized RGB */
	    *(rgb_buf++) = ((R > 255) ? 255 : ((R < 0) ? 0 : R));
	    *(rgb_buf++) = ((G > 255) ? 255 : ((G < 0) ? 0 : G));
	    *(rgb_buf++) = ((B > 255) ? 255 : ((B < 0) ? 0 : B));
	}

	/*
	 * step U and V
	 */
	if ((y % 2) == 1) {
	    u_buf += width / 2;
	    v_buf += width / 2;
	}
    }

    /*
     * all done
     */
    return rgb;
}
