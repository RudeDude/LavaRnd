/*
 * camset - Set the current camera state
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: camset.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <unistd.h>
#include <string.h>

#include "LavaRnd/lavacam.h"
#include "LavaRnd/lava_debug.h"

char *program;	/* our name */


int
main(int argc, char *argv[])
{
    struct lavacam_flag flag;	/* flags set via lavacam_argv() */
    int arg_shift;		/* argc/argv parse shift value */
    int type;			/* camera type */
    char *typename;		/* name of camera type */
    union lavacam n_cam;	/* camera new state */
    union lavacam o_cam;	/* camera open state */
    char *devname;		/* camera device name */
    struct opsize siz;		/* camera operation size */
    int cam_fd;			/* open camera file descriptor */
    int i;

    /*
     * parse args
     */
    program = argv[0];
    if (argc < 3) {
	fprintf(stderr,
		"usage: %s cam_type devname [-flags ... args ...]\n", program);
	fprintf(stderr, "       %s list all\n", program);
	fprintf(stderr, "       %s cam_type -help\n", program);
	exit(1);
    }
    typename = argv[1];
    devname = argv[2];
    /* list camera types if "list all" is given */
    if (strcmp(typename, "list") == 0 && strcmp(devname, "all") == 0) {
	lavacam_print_types(stderr);
	fflush(stderr);
	exit(0);
    }
    /* find camera type */
    type = camtype(typename);
    if (type < 0) {
	lavacam_print_types(stderr);
	fflush(stderr);
	exit(2);
    }
    argc -= 2;
    argv += 2;
    argv[0] = program;
    /* print cam_type specific arg help if device is -help */
    if (strcmp(devname, "-help") == 0) {
	lavacam_usage(type, program, typename);
	exit(0);
    }
    /* parse beyond cam_type and devname */
    arg_shift = lavacam_argv(type, argc, argv, &n_cam, &flag);
    if (arg_shift < 0) {
	lavacam_usage(type, program, typename);
	exit(3);
    }
    argc -= arg_shift;
    argv += arg_shift;
    if (argc != 1) {
	lavacam_usage(type, program, typename);
	exit(4);
    }

    /*
     * open camera
     */
    if (flag.v_flag >= 1) {
    	fprintf(stderr, "opening %s camera on %s\n", typename, devname);
    }
    cam_fd = lavacam_open(type, devname, &o_cam, &n_cam, &siz, 0, &flag);
    if (cam_fd < 0) {
	fprintf(stderr, "%s: cannot open %s camera at %s: %s\n",
		program, typename, devname, lava_err_name(cam_fd));
	exit(5);
    }

    /*
     * sanity check the opsize information
     */
    i = lavacam_get(type, cam_fd, &n_cam);
    if (i < 0) {
	fprintf(stderr,
		"%s: failed to get %s camera state at %s after open: %s\n",
		program, typename, devname, lava_err_name(i));
    	exit(6);
    }
    i = lavacam_check(type, &n_cam);
    if (i < 0) {
	fprintf(stderr,
		"%s: check failed for %s camera at %s after open: %s\n",
		program, typename, devname, lava_err_name(i));
    	exit(7);
    }

    /*
     * output the new camera state if debugging
     */
    if (flag.v_flag >= 1) {
	printf("new camera state:\n");
	lavacam_print(type, stdout, cam_fd, &n_cam, &siz);
	fflush(stdout);
    }

    /*
     * close camera
     */
    if (flag.v_flag >= 1) {
    	fprintf(stderr, "\nclosing %s camera on %s\n", typename, devname);
    }
    i = lavacam_close(type, cam_fd, &siz, &flag);
    if (i < 0) {
	fprintf(stderr, "%s: cannot close %s camera at %s: %s\n",
		program, typename, devname, lava_err_name(i));
    }

    /*
     * all done!   -- Jessica Noll, Age 2
     */
    return 0;
}
