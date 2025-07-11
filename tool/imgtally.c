/*
 * imgtally - perform tally and other stats on a series of images
 *
 * @(#) $Revision: 10.2 $
 * @(#) $Id: imgtally.c,v 10.2 2003/08/25 10:15:48 lavarnd Exp $
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
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "LavaRnd/rawio.h"
#include "LavaRnd/lavacam.h"
#include "LavaRnd/cleanup.h"
#include "LavaRnd/lava_debug.h"
#include "chi_tbl.h"


/*
 * forward declarations
 */
typedef long long llng;
typedef unsigned char uchr;
static void entropy_stats(long framecnt, long analcnt,
			  llng octetcnt, llng *octet_tally, llng *bit_sum);
static int chi1_slot(llng data0, llng data1);


/*
 * private globals
 */
static char *program;	/* our name */
static int dbg = 0;	/* -v: debug level */

#define LOG_E_OF_2 0.69314718055994530942	/* log_e 2 */
#define MAX_PIX (256)		/* maximum pixel value */


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
    struct opsize siz;		/* camera operation size */
    char *tally_file;		/* file to store tally count */
    FILE *t_stream;		/* open tally_file stream */

    llng *bit_sum;		/* bit by bit sum of values */
    llng octet_tally[MAX_PIX];	/* tally of octet values found */
    long cycle = 0;		/* stats every cycle files, 0==> at end */
    long framecnt;		/* number of files processed */
    long framenum;		/* frame number */
    llng octetcnt;		/* total number of octets processed */
    struct stat statbuf;	/* file status buffer */
    extern char *optarg;	/* option argument */
    extern int optind;		/* argv index of the next arg */
    time_t now;			/* the current time */
    uchr *p;
    int i;

    /*
     * parse args
     */
    program = argv[0];
    if (argc < 5 && (argc < 3 || strcmp(argv[2], "-help") != 0)
    		 && (argc < 3 || (strcmp(argv[1], "list") != 0 ||
		 		  strcmp(argv[2], "all") != 0))) {
	fprintf(stderr,
		"usage: %s cam_type dev count tally_file "
		    "[-flags ... args ...]\n"
		"\n"
		"\tcam_type\tcamera type\n"
		"\tdev\t\tcamera device name\n"
		"\tcount\t\tframes to process\n"
		"\ttally_file\twhere write statistical info\n"
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
    /* parse frame count */
    framecnt = strtol(argv[3], NULL, 0);
    if (framecnt <= 0) {
	fprintf(stderr, "%s: framecnt must be > 0\n", program);
	exit(3);
    }
    /* parse tally output file */
    tally_file = argv[4];
    t_stream = fopen(tally_file, "w");
    if (t_stream == NULL) {
    	fprintf(stderr, "%s: failed to open tally_file: %s",
		program, tally_file);
	exit(4);
    }
    /* parse beyond cam_type and devname */
    argv += 4;
    argc -= 4;
    argv[0] = program;
    arg_shift = lavacam_argv(type, argc, argv, &n_cam, &flag);
    if (arg_shift < 0) {
	lavacam_usage(type, program, typename);
	exit(5);
    }
    argc -= arg_shift;
    argv += arg_shift;
    /* save debug level */
    dbg = flag.v_flag; /* XXX - not needed */

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
	exit(6);
    }
    if (flag.v_flag >= 2) {
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
	    exit(7);
	}
	i = lavacam_check(type, &n_cam);
	if (i < 0) {
	    fprintf(stderr,
		    "%s: check failed for %s camera at %s after open: %s\n",
		    program, typename, devname, lava_err_name(i));
	    exit(8);
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
     * allocate our sum arrays
     */
    bit_sum = (llng *)calloc(sizeof(llng), (siz.chaos_len) * 8);
    if (bit_sum == NULL) {
	fprintf(stderr, "%s: calloc of %d octets failed\n", program,
		(siz.chaos_len) * sizeof(int) * 8);
	perror("2nd malloc");
	exit(9);
    }

    /*
     * clear other stat storage
     */
    memset(octet_tally, 0, sizeof(octet_tally));
    framenum = 0;
    octetcnt = 0LL;

    /*
     * process each frame
     */
    if (flag.v_flag >= 2) {
	fprintf(stderr, "%s op size: %d\n",
		(siz.use_read ? "read" : "mmap"), siz.chaos_len);
    }
    for (framenum = 0; framenum < framecnt; ++framenum) {

	/*
	 * wait for the next frame
	 */
	i = lavacam_wait_frame(type, cam_fd, -1.0);
	if (i < 0) {
	    fprintf(stderr, "%s: error while waiting for frame %lld: %d\n",
		    program, siz.frame_num, i);
	    exit(10);
	}

	/*
	 * get the next frame
	 */
	i = lavacam_get_frame(type, cam_fd, &siz);
	if (i < 0) {
	    fprintf(stderr, "%s: failed to get frame %lld: %d\n",
		    program, siz.frame_num, i);
	    exit(11);
	}

	/*
	 * tally and sum the octets
	 */
	octetcnt += (llng)siz.chaos_len;
	for (i = 0; i < siz.chaos_len; ++i) {
	    unsigned int octet = ((uchr *)(siz.chaos))[i];  /* octet value */
	    int j;

	    /* octet stats */
	    ++octet_tally[octet];

	    /* bit stat on the octet */
	    for (j = 0; j < 8; ++j) {
		if (octet & (1 << j)) {
		    ++bit_sum[(i<<3) | j];
		}
	    }
	}
	if (flag.v_flag >= 3) {
	    fprintf(stderr,
	    	    "frame: %ld, processed %lld octets\n", framenum, octetcnt);
	}

	/*
	 * if on the correct file cycle, output stats
	 */
	if ((cycle > 0) && ((framenum % cycle) == 0)) {
	    entropy_stats(framenum, siz.chaos_len,
	    		  octetcnt, octet_tally, bit_sum);
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
     * output final stats
     */
    entropy_stats(framenum, siz.chaos_len, octetcnt, octet_tally, bit_sum);

    /*
     * write the tally stats to the tally file
     */
    for (i=0; i < MAX_PIX; ++i) {
	fprintf(t_stream, "%d %lld\n", i, octet_tally[i]);
    }
    fclose(t_stream);

    /*
     * close the camera
     */
    if (flag.v_flag >= 1) {
    	fprintf(stderr, "closing %s camera on %s\n", typename, devname);
    }
    i = lavacam_close(type, cam_fd, &siz, &flag);
    if (i < 0) {
	fprintf(stderr, "%s: cannot close %s camera at %s: %s\n",
		program, typename, devname, lava_err_name(i));
    }

    /*
     * output frame counts
     */
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

    /*
     * all done!   -- Jessica Noll, Age 2
     */
    return 0;
}


/*
 * entropy_stats - output entropy stats
 *
 * given:
 *      framenum        number of frames processed
 *      analcnt         octets per file to analyze
 *      octetcnt        total number of octets processed
 *      octet_tally     pointer to array octet tally count
 *      bit_sum         bit counts sum for each bit position
 */
static void
entropy_stats(long framenum, long analcnt,
	      llng octetcnt, llng *octet_tally, llng *bit_sum)
{
    long chi_lvl_tally[CHI_PROB];	/* tally of chi-square levels found */
    double chi_entropy;	/* Chi^2 entropy estimate */
    double shan_bit_entropy;	/* Shannon theory entropy for bits */
    double shan_pix_entropy;	/* Shannon theory entropy for pixels */
    double uncertainty;	/* Shannon uncertainty: (p(x) * log2(p(x))) */
    int i;

    /*
     * output file stats
     */
    printf("\n\n=-=-=\nprocessed %ld frames\n", framenum);
    printf("processed %ld octets per frame\n", analcnt);
    printf("processed %lld octets\n", octetcnt);

    /*
     * chi-square level tally each bit count
     */
    memset(chi_lvl_tally, 0, sizeof(chi_lvl_tally));
    for (i = 0; i < analcnt * 8; ++i) {
	int level;	/* return from chi_slot() */

	/*
	 * determine the chi-square level
	 */
	level = chi1_slot(bit_sum[i], framenum - bit_sum[i]);

	/*
	 * tally valid levels
	 */
	if (level >= 0) {
	    ++chi_lvl_tally[level];
	} else {
	    ++chi_lvl_tally[CHI_PROB - 1];
	}
    }

    /*
     * print octet stats if debugging
     */
    if (dbg >= 2) {
	printf("octet count\n");
	for (i = 0; i < MAX_PIX; i += 8) {
	    printf("%02x: %lld %lld %lld %lld %lld %lld %lld %lld\n",
		   i, octet_tally[i], octet_tally[i + 1],
		   octet_tally[i + 2], octet_tally[i + 3],
		   octet_tally[i + 4], octet_tally[i + 5],
		   octet_tally[i + 6], octet_tally[i + 7]);
	}
    }

    /*
     * print Chi^2 stats
     */
    if (dbg >= 2) {
	printf("\nchi^2 distribution\n");
    }
    chi_entropy = 0.0;
    for (i = 0; i < CHI_PROB - 1; ++i) {
	if (dbg >= 2) {
	    /* verbose chi^2 stats */
	    printf("chi^2 [%.2f-%.2f]%%:\t%10ld\t%6.2f%%\n",
		   100.0 * chiprob[i + 1], 100.0 * chiprob[i], chi_lvl_tally[i],
		   100.0 * (double)chi_lvl_tally[i] / ((double)analcnt * 8.0));
	}
	chi_entropy += (double)chiprob[i + 1] * (double)chi_lvl_tally[i];
    }
    if (dbg >= 2) {
	/* verbose chi^2 stats */
	printf("chi^2 [excess]:\t\t%10ld\t%6.2f%%\n",
	       chi_lvl_tally[CHI_PROB - 1],
	       100.0 * (double)chi_lvl_tally[CHI_PROB -
					     1] / ((double)analcnt * 8.0));
    }
    printf("\nNOTE: The Chi^2 variation guess does not produce relibable\n");
    printf("entropy data.  It should be used for comparison purposes only.\n");
    printf("For entropy measurement use the Shannon data below.\n");
    printf("Chi^2 bit variation guess:\t\t%ld bits\t%6.2f%%\n",
    	   (long)chi_entropy,
	   100.0 * chi_entropy / ((double)analcnt * 8.0));
    printf("Chi^2 variation guess in bits/octet:\t\t\t%8.4f\n\n",
    	   chi_entropy / analcnt);

    /*
     * print Shannon information theory stats for image bits across frames
     *
     *          Entropy == -1.0 * sum(i=0; i<m; ++i) { p(i) * log2(p(i)) }
     *
     *          m == number of symbols (for binary bits, that is 2)
     *          p(i) == probability of the i-th symbol
     *          log2(x) == log base 2
     *
     * So for a simple 2 symbol alphabet {0,1}:
     *
     *          p(1) = one_bit_count / total_bits
     *          p(0) = 1 - p(1)
     *
     *          Entropy = (p(0) * log2(p(0))) + (p(1) * log2(p(1)));
     *
     *          For the end cases:  p(x) == 0  ==>  (p(x) * log2(p(x))) = 0
     *                              p(x) == 1  ==>  (p(x) * log2(p(x))) = 0
     */
    shan_bit_entropy = 0.0;
    for (i = 0; i < analcnt * 8; ++i) {
	double prob_one;	/* p(1): probability of a 1 bit */

	/*
	 * For the end cases:  p(x) == 0  ==>  (p(x) * log2(p(x))) = 0
	 *                     p(x) == 1  ==>  (p(x) * log2(p(x))) = 0
	 */
	if (bit_sum[i] == 0 || bit_sum[i] == framenum) {
	    /* shan_bit_entropy += 0.0; */
	    continue;
	}

	/*
	 * calculate Shannon uncertainty
	 */
	prob_one = (double)bit_sum[i] / (double)framenum;
	/* log(x): log base e, so log2(x) == log(x)/log(2) */
	uncertainty = ((1.0 - prob_one) * log(1.0 - prob_one) / LOG_E_OF_2) +
	  (1.0 - prob_one * log(prob_one) / LOG_E_OF_2);
	shan_bit_entropy += uncertainty;
    }
    printf("Shannon entropy bit measure:\t%6.2f\n", shan_bit_entropy);

    /*
     * print Shannon information theory stats for pixels within an image
     *
     * See above for details on how Shannon information theory entropy is
     * calculated.
     *
     * In this example we have MAX_PIX symbols, octet values for a pixel.
     *
     *          p(x) = probability that a pixel has value x
     *               = octet_tally[x] / octetcnt
     */
    shan_pix_entropy = 0.0;
    for (i = 0; i < MAX_PIX; ++i) {
	double prob;	/* p(x): probability of a given pixel value */

	/*
	 * For the end cases:  p(x) == 0  ==>  (p(x) * log2(p(x))) = 0
	 *                     p(x) == 1  ==>  (p(x) * log2(p(x))) = 0
	 */
	if (octet_tally[i] == 0 || octet_tally[i] == octetcnt) {
	    /* shan_pix_entropy += 0.0; */
	    continue;
	}

	/*
	 * calculate Shannon uncertainty
	 */
	prob = (double)octet_tally[i] / (double)octetcnt;
	/* log(x): log base e, so log2(x) == log(x)/log(2) */
	/* Yes, we do a -= because it is -1.0 * sum(...) {...} */
	shan_pix_entropy -= ((prob * log(prob) / LOG_E_OF_2));
    }
    printf("Shannon total image entropy:\t%6.2f\n",
	   shan_pix_entropy * analcnt);
    printf("Shannon estimate entropy/pixel:\t%11.4f\n\n", shan_pix_entropy);

    fflush(stdout);
    return;
}

/*
 * chi1_slot - determine the prob slot for 1 degree of freedom
 *
 * given:
 *      slot0           1st data count
 *      slot1           2nd data count
 *
 * returns:
 *      i, such that there between a:
 *
 *              chiprob[i+1] and a chiprob[i]
 *
 *      chance that the data is random.  That is, the percentage is in
 *      the range:
 *
 *              100*chiprob[i] <= actual_percentage < 100*chiprob[i+1]
 */
static int
chi1_slot(llng slot0, llng slot1)
{
    double num_samples;	/* total number of samples */
    double expected;	/* expected number of samples */
    double chisquare;	/* chi-square value of data */
    int i;

    /*
     * determine the expected number per sample
     */
    num_samples = (double)slot0 + (double)slot1;
    expected = num_samples / 2.0;
    if (expected == 0.0) {
	fprintf(stderr, "%s: chi1_slot 0 count\n", program);
	exit(13);
    }

    /*
     * calculate chi-square
     */
    chisquare = (((double)slot0 - expected) * ((double)slot0 - expected) +
		 ((double)slot1 - expected) * ((double)slot1 - expected)) /
      expected;

    /*
     * determine the chi-square levels we are between
     *
     * The last entry in chi_tbl[1][CHI_PROB-1] is a sentinel.
     */
    for (i = CHI_PROB - 2; i >= 0; --i) {
	if (chisquare >= chi_tbl[1][i]) {
	    return i;
	}
    }
    return 0;
}
