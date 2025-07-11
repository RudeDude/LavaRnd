/*
 * camget - Get the current camera state
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: camget.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
    int type;			/* camera type */
    char *typename;		/* name of camera type */
    union lavacam cam;		/* camera state */
    struct opsize siz;		/* camera operation size */
    char *devname;		/* camera device name */
    int cam_fd;			/* open camera file descriptor */
    int i;

    /*
     * parse args
     */
    program = argv[0];
    if (argc != 3) {
	fprintf(stderr, "usage: %s cam_type devname\n", program);
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
    /* print cam_type specific arg help if device is -help */
    if (strcmp(devname, "-help") == 0) {
	lavacam_usage(type, program, typename);
	exit(0);
    }

    /*
     * open camera
     */
    /* we do not use lavacam_argv(), so we must prep open structures */
    memset(&flag, 0, sizeof(flag));
    memset(&cam, 0, sizeof(cam));
    memset(&siz, 0, sizeof(siz));
    flag.D_flag = 0.0;		/* -D delay time */
    flag.T_flag = 0.0;		/* -T warm_up/sleep time */
    flag.A_flag = 0;	/* 0 ==> process only high entropy 1 ==> process all */
    flag.M_flag = 1;		/* 0 ==> do not use mmap, use reads only */
    flag.v_flag = 0;		/* -v verbose_level */
    flag.E_flag = 0;		/* no -E */
    flag.program = program;	/* program name */
    flag.savefile = NULL;	/* not saving images in a file */
    flag.newfile = NULL;	/* not saving images in a file */
    flag.interval = 0.0;	/* not saving images in a file */
    /* perform the lavacam open operation */
    printf("opening %s camera on %s\n", typename, devname);
    cam_fd = lavacam_open(type, devname, &cam, NULL, &siz, 0, &flag);
    if (cam_fd < 0) {
	fprintf(stderr, "%s: cannot open %s camera at %s: %s\n",
		program, typename, devname, lava_err_name(cam_fd));
	exit(3);
    }

    /*
     * sanity check the opsize information
     */
    i = lavacam_get(type, cam_fd, &cam);
    if (i < 0) {
	fprintf(stderr,
		"%s: failed to get %s camera state at %s after open: %s\n",
		program, typename, devname, lava_err_name(i));
    	exit(4);
    }
    i = lavacam_check(type, &cam);
    if (i < 0) {
	fprintf(stderr,
		"%s: check failed for %s camera at %s after open: %s\n",
		program, typename, devname, lava_err_name(i));
    	exit(5);
    }

    /*
     * output camera state
     */
    printf("camera state:\n");
    lavacam_print(type, stdout, cam_fd, &cam, &siz);
    fflush(stdout);

    /*
     * close camera
     */
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
