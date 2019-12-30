/*
 * camsanity - determine sanity parameters of a camera
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: camsanity.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <math.h>

#include "LavaRnd/rawio.h"
#include "LavaRnd/lavacam.h"
#include "LavaRnd/cleanup.h"
#include "LavaRnd/lava_debug.h"

#define SIGMA (0.6826895)	/* 1-sigma standard div factor */

char *program;	/* our name */

static double average(double *data, int n);
static double stddev(double *data, int n, int is_all);

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
    long long framecnt = 0;	/* number of frames to capture */
    int top_x = 0;		/* max top_x to consider */
    struct opsize siz;		/* camera operation size */
    long long framenum;		/* frame number */

    FILE *f_diff;		/* bit difference fraction file */
    FILE **f_tx;		/* uncommon fraction file */
    FILE *f_half;		/* 1/2 level file */
    FILE *final;		/* final stats file */
    double *half_x;		/* 1/2 level values */
    double *bitdiff_fract;	/* all factions of bits different */
    double **uncom_fract;	/* all fractions of uncommon octets */
    int min_half_x;		/* min 1/2 level value */
    int max_half_x;		/* max 1/2 level value */
    double min_bitdiff_fract;	/* min faction of bits different */
    double max_bitdiff_fract;	/* max faction of bits different */
    double min_uncom_fract[OCTET_CNT+1];  /* min fraction of uncommon octets */
    double max_uncom_fract[OCTET_CNT+1];  /* max fraction of uncommon octets */

    int recommend_top_x;		/* recommended def_top_x value */
    double recommend_min_fract;		/* recommended def_min_fract value */
    int recommend_half_x;		/* recommended def_half_x value */
    double recommend_diff_fract;	/* recommended def_diff_fract value */

    double t;		/* temp value */
    double d;		/* temp value */
    int i;		/* temp value */

    /*
     * parse args
     */
    program = argv[0];
    /* find camera type */
    if (argc < 6) {
	fprintf(stderr,
		"usage: %s cam_type dev dir count top_x"
		" [-flags ... args ...]\n",
		program);
	exit(1);
    }
    typename = argv[1];
    type = camtype(typename);
    if (type < 0) {
	lavacam_print_types(stderr);
	fflush(stderr);
	exit(2);
    }
    devname = argv[2];
    dirname = argv[3];
    framecnt = strtoll(argv[4], NULL, 0);
    if (framecnt <= 0) {
	fprintf(stderr, "%s: framecnt must be > 0\n", program);
	exit(3);
    }
    top_x = strtol(argv[5], NULL, 0);
    if (top_x <= 0 || top_x > 256) {
	fprintf(stderr, "%s: top_x must be > 0 and <= 256\n", program);
	exit(4);
    }
    /*
     * We fake things so that -L -x 0 -d 0 is processed before any -flags.
     * This command acts as if '-L -x 0 -d 0' is always placed before any
     * user supplied -flags and args.
     *
     * This allows somebody to supply different values of -x and -d on the
     * command line that override this 'disable frame sanity check' default.
     *
     * By setting -x 0 (min_fract==0) and -d 0 (diff_fract==0), the entire
     * frame sanity check system is bypassed.  We will process all frames
     * (sane and insane) unless different values of -x and -d are supplied
     * on the command line.
     *
     * NOTE: -X top_x and -2 half_x have no effect if -x 0 -d 0, so we
     *	     do not need to fake '-X 0 -2 0' here.
     *
     * NOTE: This code depends on the 1st thru 5th arguments being processed
     *	     above so that we can replace then below.  This code depends on
     *	     the 6th arg being the start of the user supplied -flags and args.
     */
    argv[0] = program;
    argv[1] = "-L";
    argv[2] = "-x";
    argv[3] = "0";
    argv[4] = "-d";
    argv[5] = "0";
    /* parse beyond cam_type and devname */
    arg_shift = lavacam_argv(type, argc, argv, &n_cam, &flag);
    if (arg_shift < 0) {
	lavacam_usage(type, program, typename);
	exit(5);
    }
    argc -= arg_shift;
    argv += arg_shift;
    if (flag.v_flag >= 1) {
	fprintf(stderr, "will process %lld frames\n", framecnt);
    }
    if (flag.v_flag >= 1) {
	fprintf(stderr, "will create %d files top_x in %s\n", top_x, dirname);
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
	    exit(6);
	}
    }

    /*
     * open camera
     */
    if (flag.v_flag >= 1 && flag.T_flag > 0.0) {
	fprintf(stderr, "open warm-up time: %.2f seconds\n", flag.T_flag);
    }
    cam_fd = lavacam_open(type, devname, &o_cam, &n_cam, &siz, 0, &flag);
    if (cam_fd < 0) {
	fprintf(stderr, "%s: cannot open %s camera at %s: %s\n",
		program, typename, devname, lava_err_name(cam_fd));
	exit(7);
    }
    if (flag.v_flag >= 1) {
	fprintf(stderr, "camera open on descriptor: %d\n", cam_fd);
    }

    /*
     * sanity check the opsize information
     */
    if (flag.v_flag >= 1) {
	i = lavacam_get(type, cam_fd, &n_cam);
	if (i < 0) {
	    fprintf(stderr,
		    "%s: failed to get %s camera state at %s after open: %s\n",
		    program, typename, devname, lava_err_name(i));
	    exit(8);
	}
	i = lavacam_check(type, &n_cam);
	if (i < 0) {
	    fprintf(stderr,
		    "%s: check failed for %s camera at %s after open: %s\n",
		    program, typename, devname, lava_err_name(i));
	    exit(9);
	}
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
	exit(10);
    }
    if (flag.v_flag >= 3) {
	fprintf(stderr, "cd %s\n", dirname);
    }

    /*
     * open the bit.diff stat files
     */
    errno = 0;
    f_diff = fopen("bit.diff", "w");
    if (f_diff == NULL) {
	fprintf(stderr, "%s: failed to open %s/bitdiff: %s\n",
		program, dirname, strerror(errno));
	exit(11);
    }

    /*
     * open the top_x stat files
     */
    errno = 0;
    f_tx = (FILE **) malloc((top_x + 1) * sizeof(FILE *));
    if (f_tx == NULL) {
	fprintf(stderr,
		"%s: failed to allocate %d FILE ptrs: %s\n",
		program, top_x + 1, strerror(errno));
	exit(12);
    }
    f_tx[0] = NULL;
    for (i = 1; i <= top_x; ++i) {
	char filename[sizeof("top_x.256")];	/* filename to create */

	sprintf(filename, "top_x.%03d", i);
	errno = 0;
	f_tx[i] = fopen(filename, "w");
	if (f_tx[i] == NULL) {
	    fprintf(stderr, "%s: failed to open %s/top_x.%03d: %s\n",
		    program, dirname, i, strerror(errno));
	    exit(13);
	}
    }

    /*
     * open the f_half stat files
     */
    errno = 0;
    f_half = fopen("half.lvl", "w");
    if (f_half == NULL) {
	fprintf(stderr, "%s: failed to open %s/half.lvl: %s\n",
		program, dirname, strerror(errno));
	exit(14);
    }

    /*
     * open the final stat files
     */
    errno = 0;
    final = fopen("final.stat", "w");
    if (final == NULL) {
	fprintf(stderr, "%s: failed to open %s/final.stat: %s\n",
		program, dirname, strerror(errno));
	exit(15);
    }

    /*
     * initialize stat values */
    errno = 0;
    bitdiff_fract = (double *)malloc((framecnt + 1) * sizeof(bitdiff_fract[0]));
    if (bitdiff_fract == NULL) {
	fprintf(stderr,
		"%s: failed to allocate %lld bitdiff_fract doubles: %s\n",
		program, framecnt + 1, strerror(errno));
	exit(16);
    }
    bitdiff_fract[0] = 0.0;
    min_bitdiff_fract = 2.0;
    max_bitdiff_fract = -1.0;
    min_uncom_fract[0] = 0.0;
    max_uncom_fract[0] = 0.0;
    errno = 0;
    uncom_fract = (double **)malloc((top_x + 1) * sizeof(double *));
    if (uncom_fract == NULL) {
	fprintf(stderr,
		"%s: failed to allocate %d uncom_fract ptrs: %s\n",
		program, OCTET_CNT + 1, strerror(errno));
	exit(17);
    }
    uncom_fract[0] = NULL;
    for (i = 1; i <= top_x; ++i) {
	min_uncom_fract[i] = 2.0;
	max_uncom_fract[i] = -1.0;
	uncom_fract[i] = (double *)malloc((framecnt + 1) * sizeof(double));
	if (uncom_fract[i] == NULL) {
	    fprintf(stderr,
		    "%s: failed to allocate %lld uncom_fract[%d] doubles: %s\n",
		    program, framecnt + 1, i, strerror(errno));
	    exit(18);
	} for (framenum = 0; framenum <= framecnt; ++framenum) {
	    uncom_fract[i][framenum] = 0.0;
	}
    }
    min_half_x = OCTET_CNT + 1;
    max_half_x = -1;
    errno = 0;
    half_x = (double *)malloc((framecnt + 1) * sizeof(double));
    if (half_x == NULL) {
	fprintf(stderr,
		"%s: failed to allocate %lld half_x doubles: %s\n",
		program, framecnt + 1, strerror(errno));
	exit(19);
    } for (framenum = 0; framenum <= framecnt; ++framenum) {
	half_x[framenum] = 0.0;
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
    if (flag.v_flag >= 2) {
	fprintf(stderr, "%s op size: %d\n",
		(siz.use_read ? "read" : "mmap"), siz.chaos_len);
    }
    for (framenum = 0; framenum <= framecnt; ++framenum) {

	/*
	 * wait for the next frame
	 */
	i = lavacam_wait_frame(type, cam_fd, -1.0);
	if (i < 0) {
	    fprintf(stderr, "%s: error while waiting for frame %lld: %d\n",
		    program, siz.frame_num, i);
	    exit(20);
	}

	/*
	 * get the next frame
	 */
	i = lavacam_get_frame(type, cam_fd, &siz);
	if (i < 0) {
	    fprintf(stderr, "%s: failed to get frame %lld: %d\n",
		    program, siz.frame_num, i);
	    exit(21);
	}

	/*
	 * perform sanity compotations on the frame if not 1st frame
	 */
	if (framenum > 0) {

	    /*
	     * record faction of bits different from previous frame
	     */
	    if (flag.v_flag >= 3) {
		fprintf(stderr, "recording stats for frame: %lld\n", framenum);
	    }
	    bitdiff_fract[framenum] =
	      lavacam_bitdiff_fract(siz.prev_frame, siz.chaos, siz.chaos_len);
	    fprintf(f_diff, "%lld %f\n", framenum - 1,
		    bitdiff_fract[framenum]);
	    if (flag.v_flag >= 4) {
		printf("bitdiff_fract[%lld] = %f\n",
		       framenum - 1, bitdiff_fract[framenum]);
	    }
	    fflush(f_diff);

	    /*
	     * update bits different stats
	     */
	    if (bitdiff_fract[framenum] < min_bitdiff_fract) {
		min_bitdiff_fract = bitdiff_fract[framenum];
	    }
	    if (bitdiff_fract[framenum] > max_bitdiff_fract) {
		max_bitdiff_fract = bitdiff_fract[framenum];
	    }

	    /*
	     * record top_x uncommon fraction values
	     */
	    for (i = 1; i <= top_x; ++i) {

		/*
		 * save top_x uncommon fraction value
		 */
		if (i == top_x) {
		    int half_value;	/* computed 1/2 level */

		    /* compute uncom_fract and 1/2 level */
		    uncom_fract[i][framenum] =
		      lavacam_uncom_fract(siz.chaos, siz.chaos_len,
					  i, &half_value);
		    /* process 1/2 level stats */
		    half_x[framenum] = (double)half_value;
		    if (half_x[framenum] < min_half_x) {
			min_half_x = half_x[framenum];
		    }
		    if (half_x[framenum] > max_half_x) {
			max_half_x = half_x[framenum];
		    }
		    fprintf(f_half, "%lld %d\n", framenum - 1, half_value);
		    fflush(f_half);
		    if (flag.v_flag >= 4) {
			printf("half_x[%lld] = %d\n", framenum - 1,
			       half_value);
		    }

		} else {

		    /* compute uncom_fract but not the 1/2 level */
		    uncom_fract[i][framenum] =
		      lavacam_uncom_fract(siz.chaos, siz.chaos_len, i, NULL);
		}
		fprintf(f_tx[i], "%lld %f\n", framenum - 1,
			uncom_fract[i][framenum]);
		if (flag.v_flag >= 5) {
		    printf("uncom_fract[%d][%lld] = %f\n",
			   i, framenum - 1, uncom_fract[i][framenum]);
		}
		fflush(f_tx[i]);

		/*
		 * update uncommon fraction stats
		 */
		if (uncom_fract[i][framenum] < min_uncom_fract[i]) {
		    min_uncom_fract[i] = uncom_fract[i][framenum];
		}
		if (uncom_fract[i][framenum] > max_uncom_fract[i]) {
		    max_uncom_fract[i] = uncom_fract[i][framenum];
		}
	    }
	}

	/*
	 * same current frame for next cycle - simulate lavacam_sanity() action
	 */
	memcpy(siz.prev_frame, siz.chaos, siz.chaos_len);

	/*
	 * release the frame if mapping
	 */
	i = lavacam_msync(type, cam_fd, &n_cam, &siz);
	if (i < 0) {
	    fprintf(stderr, "%s: msync release error frame %lld: %d\n",
		    program, siz.frame_num, i);
	    exit(22);
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
     * search for def_top_x - find average uncom_fract near 0.6826895 (1-sigma)
     */
    d = 1.0;
    for (recommend_top_x = 1; recommend_top_x <= top_x; ++recommend_top_x) {
	t = d;
	d = average(uncom_fract[recommend_top_x], framenum);
	if (d < SIGMA) {
	    /* we may be near our mark */
	    break;
	}
    }
    if (recommend_top_x > top_x) {
	/* we cannot recommend a def_top_x value */
	recommend_top_x = -1;
	if (flag.v_flag >= 1) {
	    printf("cannot recommend a def_top_x value, top_x: %d to small\n",
	    	    top_x);
	}
    } else if (recommend_top_x > 1) {
	/* pick the cloest top_x to SIGMA */
	if (SIGMA - d > t - SIGMA) {
	    /* previous value is closer than current */
	    --recommend_top_x;
	}
    }

    /*
     * determine def_min_fract - 5-sigma below min of recommend_top_x if known
     */
    if (recommend_top_x > 0) {
    	recommend_min_fract = min_uncom_fract[recommend_top_x] - 5 *
			      stddev(uncom_fract[recommend_top_x], framenum, 1);
    	if (recommend_min_fract <= 0.0) {
	    recommend_min_fract = -1.0;
	    if (flag.v_flag >= 1) {
	    	printf("cannot recommend def_min_fract value: "
		       "min of top_%d=%f - 5-sigma of top_%d=%f <= 0.0\n",
		       recommend_top_x, min_uncom_fract[recommend_top_x],
		       recommend_top_x,
		       stddev(uncom_fract[recommend_top_x], framenum, 1));
	    }
	}
    } else {
	/* no def_top_x ==> no def_min_fract */
 	recommend_min_fract = -1.0;
	if (flag.v_flag >= 1) {
	    printf("cannot recommend def_min_fract value without "
	    	   "a def_top_x value\n");
	}
    }

    /*
     * determine def_half_x - 5-sigma less than min_half_x
     */
    if (min_half_x >= top_x) {
	/* we cannot recommend a def_half_x value */
	recommend_half_x = -1;
	if (flag.v_flag >= 1) {
	    printf("cannot recommend def_half_x, top_x: %d is too small\n",
	    	   top_x);
	}
    } else {
    	recommend_half_x = (int)(min_half_x - 5*stddev(half_x, framenum, 1));
	if (recommend_half_x < 0) {
	    /* we cannot recommend a def_half_x value */
	    recommend_half_x = -1;
	    if (flag.v_flag >= 1) {
		printf("cannot recommend def_half_x, value would be < 0\n");
	    }
	}
    }

    /*
     * determine def_diff_fract - 1-sigma below min min_bitdiff_fract
     */
    recommend_diff_fract = min_bitdiff_fract - 5 *
	stddev(bitdiff_fract, framenum, 1);
    if (recommend_diff_fract <= 0.0) {
	/* we cannot recommend a def_diff_fract value */
	if (flag.v_flag >= 1) {
	    printf("cannot recommend def_diff_fract value: "
		   "min_bitdiff_fract=%f - 5*sdv_bitdiff_fract=%f <= 0.0",
		   min_bitdiff_fract,
		   stddev(bitdiff_fract, framenum, 1));
	}
	recommend_diff_fract = -1.0;
    }

    /*
     * output final stats
     */
    fprintf(final, "min_bitdiff_fract = %f\n", min_bitdiff_fract);
    fprintf(final, "max_bitdiff_fract = %f\n", max_bitdiff_fract);
    fprintf(final, "ave_bitdiff_fract = %f\n",
	    average(bitdiff_fract, framenum));
    fprintf(final, "sdv_bitdiff_fract = %f\n",
	    stddev(bitdiff_fract, framenum, 1));
    fprintf(final, "min_half_x = %d\n", min_half_x);
    fprintf(final, "max_half_x = %d\n", max_half_x);
    fprintf(final, "ave_half_x = %f\n", average(half_x, framenum));
    fprintf(final, "sdv_half_x = %f\n", stddev(half_x, framenum, 1));
    for (i = 1; i <= top_x; ++i) {
	fprintf(final, "min_uncom_fract[%d] = %f\n", i, min_uncom_fract[i]);
	fprintf(final, "max_uncom_fract[%d] = %f\n", i, max_uncom_fract[i]);
	fprintf(final, "ave_uncom_fract[%d] = %f\n",
		i, average(uncom_fract[i], framenum));
	fprintf(final, "sdv_uncom_fract[%d] = %f\n",
		i, stddev(uncom_fract[i], framenum, 1));
    }
    if (flag.v_flag >= 1) {
	printf("bitdiff_fract(min,max,ave,stv): %f %f %f %f\n",
	       min_bitdiff_fract, max_bitdiff_fract,
	       average(bitdiff_fract, framenum),
	       stddev(bitdiff_fract, framenum, 1));
	printf("half_x(min,max,ave,stv): %d %d %f %f\n",
	       min_half_x, max_half_x,
	       average(half_x, framenum), stddev(half_x, framenum, 1));
	for (i = 1; i <= top_x; ++i) {
	    printf("uncom_fract[%d](min,max,ave,stv): %f %f %f %f\n",
		   i, min_uncom_fract[i], max_uncom_fract[i],
		   average(uncom_fract[i], framenum),
		   stddev(uncom_fract[i], framenum, 1));
	}
    }

    /*
     * output final recomendations
     */
    if (recommend_top_x >= 0) {
	fprintf(final, "def_top_x = %d\n", recommend_top_x);
	if (flag.v_flag >= 1) {
	    printf("\ndef_top_x = %d\n", recommend_top_x);
	}
    } else {
	fprintf(final, "def_top_x = UNKNOWN\n");
	if (flag.v_flag >= 1) {
	    printf("\ndef_top_x = UNKNOWN\n");
	}
    }
    if (recommend_min_fract >= 0.0) {
	fprintf(final, "def_min_fract = %f\n", recommend_min_fract);
	fprintf(final, "    min_value = %f\n",
		min_uncom_fract[recommend_top_x]);
	fprintf(final, "    sigma_val = %f\n",
		stddev(uncom_fract[recommend_top_x], framenum, 1));
	if (flag.v_flag >= 1) {
	    printf("def_min_fract = %f\n", recommend_min_fract);
	    printf("    min_value = %f\n",
		   min_uncom_fract[recommend_top_x]);
	    printf("    sigma_val = %f\n",
		   stddev(uncom_fract[recommend_top_x], framenum, 1));
	}
    } else {
	fprintf(final, "def_min_fract = UNKNOWN\n");
	fprintf(final, "    min_value = UNKNOWN\n");
	fprintf(final, "    sigma_val = UNKNOWN\n");
	if (flag.v_flag >= 1) {
	    printf("def_min_fract = UNKNOWN\n");
	    printf("    min_value = UNKNOWN\n");
	    printf("    sigma_val = UNKNOWN\n");
	}
    }
    if (recommend_half_x >= 0) {
	fprintf(final, "def_half_x = %d\n", recommend_half_x);
	fprintf(final, "   min_val = %d\n", min_half_x);
	fprintf(final, "   sdv_val = %f\n", stddev(half_x, framenum, 1));
	if (flag.v_flag >= 1) {
	    printf("def_half_x = %d\n", recommend_half_x);
	    printf("   min_val = %d\n", min_half_x);
	    printf("   sdv_val = %f\n", stddev(half_x, framenum, 1));
	}
    } else {
	fprintf(final, "def_half_x = UNKNOWN\n");
	fprintf(final, "   min_val = %d\n", min_half_x);
	fprintf(final, "   sdv_val = %f\n", stddev(half_x, framenum, 1));
	if (flag.v_flag >= 1) {
	    printf("def_half_x = UNKNOWN\n");
	    printf("   min_val = %d\n", min_half_x);
	    printf("   sdv_val = %f\n", stddev(half_x, framenum, 1));
	}
    }
    if (recommend_diff_fract >= 0.0) {
	fprintf(final, "def_diff_fract = %f\n", recommend_diff_fract);
	fprintf(final, "      min_diff = %f\n", min_bitdiff_fract);
	fprintf(final, "    sigma_diff = %f\n",
		stddev(bitdiff_fract, framenum, 1));
	if (flag.v_flag >= 1) {
	    printf("def_diff_fract = %f\n", recommend_diff_fract);
	    printf("      min_diff = %f\n", min_bitdiff_fract);
	    printf("    sigma_diff = %f\n",
	    	   stddev(bitdiff_fract, framenum, 1));
	}
    } else {
	fprintf(final, "def_diff_fract = UNKNOWN\n");
	fprintf(final, "      min_diff = %f\n", min_bitdiff_fract);
	fprintf(final, "    sigma_diff = %f\n",
		stddev(bitdiff_fract, framenum, 1));
	if (flag.v_flag >= 1) {
	    printf("def_diff_fract = UNKNOWN\n");
	    printf("      min_diff = %f\n", min_bitdiff_fract);
	    printf("    sigma_diff = %f\n",
	    	   stddev(bitdiff_fract, framenum, 1));
	}
    }
    fflush(final);

    /*
     * close camera
     */
    (void)lavacam_close(type, cam_fd, &siz, &flag);
    fclose(f_diff);
    for (i = 1; i <= top_x; ++i) {
	fclose(f_tx[i]);
    }
    fclose(final);

    /*
     * cleanup
     */
    free(f_tx);
    free(bitdiff_fract);
    for (i = 1; i <= top_x; ++i) {
	free(uncom_fract[i]);
    }
    free(uncom_fract);
    free(half_x);
    fclose(f_half);
    lava_dormant();
    return 0;
}


/*
 * average - compute the average of a set of values
 *
 * given:
 *      data    point to 'n' doubles
 *
 * returns:
 *      average
 */
static double
average(double *data, int n)
{
    double sum;	/* sum of data points */
    double average;	/* average value of data points */
    int i;

    /*
     * firewall
     */
    if (data == NULL || n < 1) {
	/* no data has a 0 average :-) */
	return 0.0;
    }

    /*
     * compute sum and average
     */
    sum = 0.0;
    for (i = 0; i < n; ++i) {
	sum += data[i];
    }
    average = sum / (double)n;
    return average;
}


/*
 * stddev - standard deviation
 *
 * given:
 *      data    point to 'n' doubles
 *      n       number of data points
 *      is_all  TRUE ==> data is the entire population
 *              FALSE ==> data is a selected sample of the population
 *
 * returns:
 *      standard deviation of data
 *
 * As the 'Fallacies and Traps' section of the "Statistics The Easy Way",
 * 3rd edition (by Douglas Downing & Jeffery Clark; Barron's publications,
 * 1997; ISBN 0-8120-9392-5) points out on p 229, example 3: The choice
 * if divisor in the standard deviation will either be 'n' (when the
 * entire population is being calculated) or 'n-1' (when a random partial
 * subset / sample of the entire population is being calculated.
 *
 * The usual formula for standard deviation is:
 *
 *      sqrt[ 1/(N-1 or N) * [ sum{ X(i)^2 } - N*ave_of_Xs ]]      (^ is power)
 *
 * As is pointed out in 'Numerical Recipes in C', 2nd edition (by Press,
 * Teukolsky, Vetterling, Flannery; Cambridge University Press, 1992;
 * ISBN 0-521-43108-5) points out on p 613 (section 14.1 - formula 14.1.8),
 * better results with less roundoff errors can be achieved by using:
 *
 *      sqrt[ 1/(N-1 or N) * [ sum{ (X(i) - ave_of_Xs)^2 } -
 *                             N * sum{ X(i) - ave_of_Xs }^2 ]]   (^ is power)
 *
 */
static double
stddev(double *data, int n, int is_all)
{
    double ave;	/* average value of data points */
    double diff;	/* absolute difference from average */
    double sumdiff;	/* sum of diff's */
    double sumsqdiff;	/* sum of square of diff's */
    double std_dev;	/* standard deviation */
    int i;

    /*
     * firewall
     */
    if (data == NULL || n < 1 || (!is_all && n < 2)) {
	/* no data has a 0 standard deviation :-) */
	return 0.0;
    }

    /*
     * compute the variation sums
     */
    ave = average(data, n);
    sumdiff = 0.0;
    sumsqdiff = 0.0;
    for (i = 0; i < n; ++i) {
	diff = fabs(data[i] - ave);
	sumdiff += diff;
	sumsqdiff += diff * diff;
    }

    /*
     * return standard deviation
     */
    std_dev = (sumsqdiff - (sumdiff * sumdiff) / (double)n) /
      (double)(is_all ? n : n - 1);
    if (std_dev < 0.0) {
	std_dev = 0.0;
    } else {
	std_dev = sqrt(std_dev);
    }
    return std_dev;
}
