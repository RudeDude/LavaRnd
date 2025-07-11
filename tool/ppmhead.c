/*
 * ppmhead - Form a ppm file header for a given width and height
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: ppmhead.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


static char *program;	/* our name */

int
main(int argc, char *argv[])
{
    int rows;	/* number of rows in the image */
    int cols;	/* number of cols in the image */

    /*
     * parse args
     */
    program = argv[0];
    if (argc != 3) {
	fprintf(stderr, "usage: %s rows cols\n", program);
	exit(1);
    }
    rows = atoi(argv[1]);
    cols = atoi(argv[2]);

    /*
     * print PPM header
     */
    printf("P6\n%d %d\n255\n", cols, rows);
    return 0;
}
