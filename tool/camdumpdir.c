/*
 * camdumpdir - dump Y Luminance camera data into separate files in a directory
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: camdumpdir.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "LavaRnd/rawio.h"
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
    int cam_fd;			/* open camera file descriptor */
    char *dirname = NULL;	/* directory to hold image files */
    int filecnt = 0;		/* number of files to form */
    struct opsize siz;		/* camera operation size */
    long long filenum;		/* file number */
    time_t now;			/* the current time */
    int i;

    /*
     * parse args
     */
    program = argv[0];
    if (argc < 5 && (argc < 3 || strcmp(argv[2], "-help") != 0)
    		 && (argc < 3 || (strcmp(argv[1], "list") != 0 ||
		 		  strcmp(argv[2], "all") != 0))) {
	fprintf(stderr,
		"usage: %s cam_type dev dir count [-flags ... args ...]\n"
		"\n"
		"\tcam_type\tcamera type\n"
		"\tdev\t\tcamera device name\n"
		"\tdir\t\tdirectory into which lumanance frames are written\n"
		"\tcount\t\tnumber of frames to write into dir\n"
		"\n"
		"for help on -flags and args, try:  %s cam_type -help\n"
		"for list of allowed cam_type, try: %s list all\n",
		program, program, program);
	exit(1);
    }
    /* parse cam_type */
    typename = argv[1];
    /* parse dev */
    devname = argv[2];
    /* list camera types if "list all" is given */
    if (strcmp(typename, "list") == 0 && strcmp(devname, "all") == 0) {
	lavacam_print_types(stderr);
	fflush(stderr);
	exit(0);
    }
    /* validate camera type */
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
    /* parse directory name */
    dirname = argv[3];
    /* parse file count */
    filecnt = strtol(argv[4], NULL, 0);
    if (filecnt <= 0) {
	fprintf(stderr, "%s: filecnt must be > 0\n", program);
    }
    argc -= 4;
    argv += 4;
    argv[0] = program;
    /* parse beyond cam_type, dev, dir, count */
    arg_shift = lavacam_argv(type, argc, argv, &n_cam, &flag);
    if (arg_shift < 0) {
	lavacam_usage(type, program, typename);
	exit(3);
    }
    argc -= arg_shift;
    argv += arg_shift;
    if (flag.v_flag >= 1) {
	fprintf(stderr, "will create %d files in %s\n", filecnt, dirname);
    }

    /*
     * create the directory
     */
    errno = 0;
    if (mkdir(dirname, 0775) < 0) {
	if (errno == EEXIST) {
	    if (flag.v_flag >= 1) {
		fprintf(stderr, "directory %s already exists\n", dirname);
	    }
	} else {
	    fprintf(stderr, "%s: failed to mkdir %s: %s\n",
		    program, dirname, strerror(errno));
	    exit(4);
	}
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
    if (flag.v_flag >= 1) {
	fprintf(stderr, "camera open on descriptor: %d\n", cam_fd);
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
     * output camera state if debugging
     */
    if (flag.v_flag >= 1) {
	printf("camera state:\n");
	lavacam_print(type, stdout, cam_fd, &n_cam, &siz);
	fflush(stdout);
    }

    /*
     * change to camera directory
     */
    errno = 0;
    if (chdir(dirname) < 0) {
	fprintf(stderr, "%s: failed to cd %s: %s\n",
		program, dirname, strerror(errno));
	exit(8);
    }
    if (flag.v_flag >= 1) {
	fprintf(stderr, "\ncd %s\n", dirname);
    }

    /*
     * output the new camera state if debugging
     */
    if (flag.v_flag >= 5) {
	fprintf(stderr, "open camera state:\n");
	lavacam_print(type, stderr, cam_fd, &n_cam, &siz);
	fflush(stderr);
    }

    /*
     * pump out frame data
     */
    if (flag.v_flag >= 1) {
	fprintf(stderr, "%s op size: %d\n",
		(siz.use_read ? "read" : "mmap"), siz.chaos_len);
    }
    for (filenum = 0; filenum < filecnt; ++filenum) {
	char filename[sizeof("img0123456789.yuv")];	/* filename to create */
	int fd;	/* open file to write */

	/*
	 * wait for the next frame
	 */
	i = lavacam_wait_frame(type, cam_fd, -1.0);
	if (i < 0) {
	    fprintf(stderr, "%s: error while waiting for frame %lld: %d\n",
		    program, siz.frame_num, i);
	    exit(9);
	}

	/*
	 * get the next frame
	 */
	i = lavacam_get_frame(type, cam_fd, &siz);
	if (i < 0) {
	    fprintf(stderr, "%s: failed to get frame %lld: %d\n",
		    program, siz.frame_num, i);
	    exit(10);
	}

	/*
	 * sanity check the frame
	 */
	i = lavacam_sanity(&siz);
	if (i < 0) {
	    if (flag.v_flag >= 1) {
		fprintf(stderr,
			"ignoring frame number: %lld insane count: %lld\n",
			siz.frame_num, siz.insane_cnt);
	    }

	    /* ignore this loop */
	    --filenum;

	/*
	 * frame is sane, process it
	 */
	} else {

	    /*
	     * creat/open a new file
	     */
	    errno = 0;
	    sprintf(filename, "img%010lld.yuv", filenum);
	    fd = open(filename, O_CREAT|O_WRONLY, S_IREAD|S_IRGRP|S_IROTH);
	    if (fd < 0) {
		/* let the loop skip this filename */
		fprintf(stderr,
			"skip frame write, cannot open filename: %s: %s\n",
			filename, strerror(errno));
	    } else {

		/*
		 * write the frame to the file
		 */
		i = raw_write(fd, siz.chaos, siz.chaos_len, 0);

		if (i < 0) {
		    fprintf(stderr, "%s: cannot write frame %lld: %d\n",
			    program, siz.frame_num, i);
		    exit(11);
		} else if (i == 0) {
		    if (flag.v_flag >= 1) {
			fprintf(stderr,
				"EOF on write frame: %lld\n", siz.frame_num);
		    }
		    exit(0);
		} else if (flag.v_flag >= 3) {
		    fprintf(stderr, "wrote file: %s\n", filename);
		}
		close(fd);
	    }
	}

	/*
	 * release the frame if mapping
	 */
	i = lavacam_msync(type, cam_fd, &n_cam, &siz);
	if (i < 0) {
	    fprintf(stderr, "%s: msync release error frame %lld: %d\n",
		    program, siz.frame_num, i);
	    exit(12);
	}

	/*
	 * wait, if requested, before the next frame
	 */
	if (flag.D_flag > 0.0) {
	    if (flag.v_flag >= 2) {
		fprintf(stderr, "wait for %.2f seconds\n", flag.D_flag);
	    }
	    lava_sleep(flag.D_flag);
	}
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
     * output frame counts
     */
    if (flag.v_flag >= 1) {
	printf("camera open time: %s", ctime(&(siz.open_time)));
	now = time(NULL);
	printf("camera close time: %s", ctime(&now));
	printf("duration: ~%ld second(s)\n", now - siz.open_time);
	printf("\ntotal frame count: %lld\n", siz.frame_num);
	printf("insane frame count: %lld\n", siz.insane_cnt);
	if (siz.frame_num > 0) {
	    printf("sanity: %8.5f%%\n",
		    100 * (1.0 - ((double)siz.insane_cnt/siz.frame_num)));
	}
    }

    /*
     * all done!   -- Jessica Noll, Age 2
     */
    return 0;
}
