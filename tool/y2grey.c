/*
 * y2grey - read Y Luminance values, output pseudo Grey scale RGB ppm image
 *
 * The input is assumed to be the Y Luminance from a YUV4:2:0 planar image.
 * The output is a grey scale image, in RGB form with a PPM header.
 *
 * This file does not perform the standard YUV to RGB convertion.  It cannot
 * because the input lacks U and V pseudocolor components.  We treat the Y
 * luminance values as light RGB light levels.  For each value of Y,
 * we output an RGB value of YYY.  The RGB values produced are Grey scale,
 * but because this file is not doing true YUV->RGB conversion, we call
 * the output "pseudo Grey scale RGB".
 *
 * For a contrasting Grey scale conversion process, see y2yuv.c.
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: y2grey.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
 * @(#) $Source: /home/lavarnd/int/tool/RCS/y2grey.c,v $
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

#define USAGE "usage: %s width height file\n"


int
main(int argc, char *argv[])
{
    int width;	/* image width in pixels */
    int height;	/* image height in pixels */
    char *in_name;	/* YUV input filename */
    int in_fd;	/* input file descriptor */
    struct stat ibuf;	/* input file stats */
    int in_size;	/* expected YUV size in octets */
    u_int8_t *y;	/* Y image read from in_name */
    u_int8_t *rgb;	/* converted RGB image */
    int i;
    int j;

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
    in_size = width * height;	/* Y from YUV420P is 1 octets per pixel */
    if (in_size != ibuf.st_size) {
	fprintf(stderr, "%s: file %s size: %d != %d = %d * %d\n",
		program, in_name, (int)ibuf.st_size, in_size, width, height);
	exit(4);
    }

    /*
     * allocate Y data buffer
     */
    y = (u_int8_t *) malloc(in_size);
    if (y == NULL) {
	fprintf(stderr, "%s: failed to malloc %d octets for %s image\n",
		program, in_size, in_name);
	exit(5);
    }

    /*
     * allocate RGB image buffer
     */
    rgb = (u_int8_t *) malloc(in_size * 3);
    if (rgb == NULL) {
	fprintf(stderr, "%s: failed to malloc %d octets for RGB image\n",
		program, in_size * 3);
	exit(6);
    }

    /*
     * read in Y values
     */
    i = read(in_fd, y, in_size);
    if (i != in_size) {
	fprintf(stderr, "%s: read of %s returned %d != %d\n",
		program, in_name, i, in_size);
	exit(7);
    }
    (void)close(in_fd);

    /*
     * convert Y to pseudo Grey scale YYY/RGB
     *
     * See comment about regarding this hack.
     */
    for (i = 0, j = 0; i < in_size; ++i) {
	rgb[j++] = y[i];
	rgb[j++] = y[i];
	rgb[j++] = y[i];
    }

    /*
     * write image to stdout
     */
    printf("P6\n%d %d\n255\n", width, height);
    errno = 0;
    i = fwrite(rgb, 1, in_size * 3, stdout);
    if (i != in_size * 3) {
	if (ferror(stdout)) {
	    fprintf(stderr, "%s: failed to write %d octets of RGB\n",
		    program, in_size * 2);
	    exit(8);
	} else if (feof(stdout)) {
	    fprintf(stderr, "%s: EOF while writing %d octets of RGB\n",
		    program, in_size * 2);
	    exit(9);
	} else {
	    fprintf(stderr, "%s: RGB fwrite returned %d != %d octets\n",
		    program, i, in_size * 2);
	    exit(10);
	}
    }

    /*
     * all done
     */
    fclose(stdout);
    return 0;
}
