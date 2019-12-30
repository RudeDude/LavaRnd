/*
 * y2pseudoyuv - read Y Luminance values, output psuedocolor YUV YUV4:2:0 planar
 *
 * The input is assumed to be the Y Luminance from a YUV4:2:0 planar image.
 * The output is a simulated YUV YUV4:2:0 planar image where the U and V
 * pseudocolors are derived from the Y values.  The dymanic range if the Y
 * values are is expanded to fill the [0,255] space.
 *
 * The colors in the resulting YUV YUV4:2:0 planar image are simulated.
 * The luminance range of colors is expanded to the maximum range to
 * maximize contrast.
 *
 * One can use the yuv2ppm to convert output from this program
 * into RGB form with a PPM header.
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: y2pseudoyuv.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


static char *program;	/* our name */

#define USAGE "%s width height file\n"


int
main(int argc, char *argv[])
{
    int width;	/* image width in pixels */
    int height;	/* image height in pixels */
    char *in_name;	/* YUV input filename */
    int in_fd;	/* input file descriptor */
    struct stat ibuf;	/* input file stats */
    int y_size;	/* expected Y size in octets */
    int yuv_size;	/* expected YUV size in octets */
    u_int8_t *yuv;	/* YUV buffer, where Y values are loaded */
    int min_y;	/* lowest raw Y value */
    int max_y;	/* highest raw Y value */
    int y;
    int u;
    int v;

    /*
     * parse args
     */
    program = argv[0];
    if (argc != 4) {
	fprintf(stderr, USAGE, program);
	exit(1);
    }
    width = strtol(argv[1], NULL, 0);
    height = strtol(argv[2], NULL, 0);
    in_name = argv[3];
    errno = 0;

    /*
     * try to open filename
     */
    in_fd = open(in_name, O_RDONLY);
    if (in_fd < 0) {
	fprintf(stderr, "%s: cannot open %s: %s",
		program, in_name, strerror(errno));
	exit(2);
    }

    /*
     * verify that the Y data is the correct size
     */
    if (fstat(in_fd, &ibuf) < 0) {
	fprintf(stderr, "%s: cannot fstat open file %s: %s",
		program, in_name, strerror(errno));
	exit(3);
    }
    y_size = width * height;	/* Y from YUV420P is 1 octets per pixel */
    if (y_size != ibuf.st_size) {
	fprintf(stderr, "%s: file %s size: %d != %d = %d * %d\n",
		program, in_name, (int)ibuf.st_size, y_size, width, height);
	exit(4);
    }
    if (y_size % 2 == 1) {
	fprintf(stderr, "%s: file %s size: %d x %d = %d is an odd number\n",
		program, in_name, width, height, y_size);
    }
    yuv_size = 3 * y_size / 2;

    /*
     * allocate YUV image buffer
     */
    yuv = (u_int8_t *) malloc(yuv_size + 1);
    if (yuv == NULL) {
	fprintf(stderr, "%s: failed to malloc %d octets for YUV image\n",
		program, yuv_size + 1);
	exit(6);
    }

    /*
     * read in Y values
     */
    y = read(in_fd, yuv, y_size);
    if (y != y_size) {
	fprintf(stderr, "%s: read of %s returned %d != %d\n",
		program, in_name, y, y_size);
	exit(7);
    }
    (void)close(in_fd);

    /*
     * normalize Y values over [0,255] range
     */
    /* determine current Y range */
    min_y = 256;
    max_y = 0;
    for (y = 0; y < y_size; ++y) {
	min_y = (yuv[y] < min_y) ? yuv[y] : min_y;
	max_y = (yuv[y] > max_y) ? yuv[y] : max_y;
    }
    /* if range is not [0,255], shift and expand to fill [0,255] */
    if (min_y != 0 && max_y != 255) {
	double expand;	/* range expansion factor */

	/* special case, all pixels have same value */
	if (min_y == max_y) {
	    /* fill with dark pixel values */
	    memset(yuv, 0, y_size);

	    /* expand the range to fill [0,255] */
	} else {
	    expand = 255.999999 / (max_y - min_y);
	    for (y = 0; y < y_size; ++y) {
		yuv[y] = (u_int8_t) ((yuv[y] - min_y) * expand);
	    }
	}
    }

    /*
     * form U and V from Y
     *
     * There are 4 Y octets per U and V octet.
     *
     * We set both the U and V values based on mid-range bias modified
     * by 2 of the 4 Y values.
     */
    for (y = 0, u = y_size, v = u + y_size / 4; y < y_size; y += 4, ++u, ++v) {
	yuv[u] = 128 + yuv[y] - yuv[y + 2];
	yuv[v] = 128 + yuv[y + 1] - yuv[y + 3];
    }

    /*
     * write YUV image to stdout
     */
    errno = 0;
    y = fwrite(yuv, 1, yuv_size, stdout);
    if (y != yuv_size) {
	if (ferror(stdout)) {
	    fprintf(stderr, "%s: failed to write %d octets of YUV\n",
		    program, yuv_size);
	    exit(8);
	} else if (feof(stdout)) {
	    fprintf(stderr, "%s: EOF while writing %d octets of YUV\n",
		    program, yuv_size);
	    exit(9);
	} else {
	    fprintf(stderr, "%s: YUV fwrite returned %d != %d octets\n",
		    program, y, yuv_size);
	    exit(10);
	}
    }

    /*
     * all done
     */
    fclose(stdout);
    return 0;
}
