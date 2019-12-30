/*
 * camop - generic camera operation to specific camera operation
 *
 * @(#) $Revision: 10.2 $
 * @(#) $Id: camop.c,v 10.2 2003/11/09 22:37:03 lavarnd Exp $
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
 * Here is the general model for calling the camera op interface as found
 * in the libLavaRnd_cam.a library.
 *
 * This code was taken from the tool camdump, from the src/tool/camdump.c file.
 *
 *      char *program;          (* our name *)
 *      struct lavacam_flag flag;       (* flags set via lavacam_argv() *)
 *      char *typename;         (* name of camera type *)
 *      char *devname;          (* camera device name *)
 *      int arg_shift;          (* argc/argv parse shift value *)
 *      int type;               (* camera type *)
 *      union lavacam o_cam;    (* open camera state *)
 *      union lavacam n_cam;    (* new camera state *)
 *      struct opsize siz;      (* camera operation size *)
 *      long long frame;        (* frame number *)
 *      long long maxlen = -1;  (* maximum length to write, <0 ==> infinite *)
 *      long long written = 0;  (* amount of data written *)
 *      int sanity;             (* lavacam_sanity check result *)
 *
 *      (*
 *       * parse args
 *       *)
 *      program = argv[0];
 *      (* find camera type *)
 *      if (argc < 3) {
 *          fprintf(stderr,
 *              "usage: %s cam_type devname [-flags ... args ...]\n", program);
 *          exit(1);
 *      }
 *      typename = argv[1];
 *      type = camtype(typename);
 *      if (type < 0) {
 *          lava_print_camtypes(stderr);
 *          exit(2);
 *      }
 *      devname = argv[2];
 *      argc -= 2;
 *      argv += 2;
 *      argv[0] = program;
 *      (* parse beyond cam_type and devname *)
 *      arg_shift = lavacam_argv(type, argc, argv, &n_cam, &flag);
 *      if (arg_shift < 0) {
 *          lavacam_usage(type, typename, program, typename);
 *          exit(3);
 *      }
 *      argc -= arg_shift;
 *      argv += arg_shift;
 *      (* at this point, argv[1], if argc > 1, is the next arg to parse *)
 *      if (argc != 2) {
 *          fprintf(stderr, "%s: expected 1 arg after parameters, found %d\n",
 *                  program, argc);
 *          exit(4);
 *      }
 *      maxlen = strtoll(argv[1], NULL, 0);     (* <0 ==> infinite pump *)
 *
 *      (*
 *       * open the camera
 *       *)
 *      cam_fd = lavacam_open(type, devname, &o_cam, &n_cam, &siz, 0, &flag);
 *      if (cam_fd < 0) {
 *          fprintf(stderr, "%s: cannot open %s camera at %s\n",
 *                  program, typename, devname);
 *          exit(5);
 *      }
 *
 *      (*
 *       * pump out data
 *       *)
 *      frame = 0;
 *      for (written = 0;
 *           (maxlen < 0) || (written < maxlen);
 *           written += siz.chaos_len) {
 *
 *          (*
 *           * wait for the next frame
 *           *)
 *          i = lavacam_wait_frame(type, cam_fd, -1.0);
 *          if (i < 0) {
 *              fprintf(stderr, "%s: error while waiting for frame %lld: %d\n",
 *                  program, frame, i);
 *              exit(6);
 *          }
 *
 *          (*
 *           * get the next frame
 *           *)
 *          i = lavacam_get_frame(type, cam_fd, &siz);
 *          if (i < 0) {
 *              fprintf(stderr, "%s: failed to get frame %lld: %d\n",
 *                  program, frame, i);
 *              exit(7);
 *          }
 *
 *          (*
 *           * write the frame to stdout
 *           *)
 *          sanity = lavacam_sanity(&siz);
 *          if (sanity == LAVACAM_OK) {
 *              if (maxlen < 0 || written+siz.chaos_len <= maxlen) {
 *                  i = raw_write(STDOUT, siz.chaos, siz.chaos_len, 0);
 *              } else {
 *                  i = raw_write(STDOUT, siz.chaos, (int)(maxlen-written), 0);
 *              }
 *              if (i < 0) {
 *                  fprintf(stderr, "%s: cannot write frame %lld: %d\n",
 *                      program, frame, i);
 *                  exit(8);
 *              } else if (i == 0) {
 *                  if (flag.v_flag >= 1) {
 *                      fprintf(stderr, "EOF on write frame: %lld\n", frame);
 *                  }
 *                  exit(0);
 *              }
 *          } else {
 *              if (flag.v_flag >= 2) {
 *                  fprintf(stderr,
 *                          "ignore frame: %lld fails sanity check: %s\n",
 *                          frame, lava_err_name(sanity));
 *              }
 *          }
 *
 *          (*
 *           * release the frame if mapping
 *           *)
 *          i = lavacam_msync(type, cam_fd, &n_cam, &siz);
 *          if (i < 0) {
 *              fprintf(stderr, "%s: msync release error frame %lld: %d\n",
 *                  program, frame, i);
 *              exit(9);
 *          }
 *          ++frame;
 *
 *          (*
 *           * wait, if requested, before the next frame
 *           *)
 *          if (flag.D_flag > 0.0) {
 *              if (flag.v_flag >= 2) {
 *                  fprintf(stderr, "wait for %.3f seconds\n", flag.D_flag);
 *              }
 *              lava_sleep(flag.D_flag);
 *          }
 *    }
 *
 *    (*
 *     * close camera
 *     *)
 *    (void) lavacam_close(type, cam_fd, &siz);
 *    return 0;
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "LavaRnd/lavacam.h"


struct camop {
    int model;		/* model number - (see lava_state in xyz_drvr.c) */
    char *module;	/* name of Linux module used by the webcam */
    char *type;		/* webcam type */
    char *name;		/* short name */
    char *fullname;	/* full extended name */

    /*
     * operation functions
     */
    /* get camera parameters */
    int (*get) (int fd, union lavacam * cam);
    /* set camera parameters */
    int (*set) (int fd, union lavacam * cam);
    /* preset change state with LavaRnd entropy optimal values */
    int (*LavaRnd) (union lavacam * cam, int model);

    /* check camera parameters */
    int (*check) (union lavacam * cam);
    /* print camera parameters */
    int (*print) (FILE * stream, int fd, union lavacam * cam,
		  struct opsize * siz);

    /* print a usage message for command line setting args */
    void (*usage) (char *prog, char *typename);
    /* parse command line args for setting camera values */
    int (*argv) (int argc, char **argv, union lavacam * cam,
		 struct lavacam_flag * flag, int model);

    /* open a camera */
    int (*open) (char *devname, int model, union lavacam * o_cam,
		 union lavacam * n_cam, struct opsize * siz, int def,
		 struct lavacam_flag * flag);
    /* close a camera */
    int (*close) (int fd, struct opsize * siz, struct lavacam_flag * flag);

    /* get a frame of data */
    int (*get_frame) (int cam_fd, struct opsize * siz);
    /* release a mmap processed buffer */
    int (*msync) (int fd, union lavacam * cam, struct opsize * siz);
    /* select wait for next frame */
    int (*wait_frame) (int fd, double max_wait);
};
static struct camop cam_switch[] = {
    {730, "pwc", "pwc730", "Philips_730", "Logitech QuickCam 3000 Pro",
     pwc_get, pwc_set, pwc_LavaRnd,
     pwc_check, pwc_print,
     pwc_usage, pwc_argv,
     pwc_open, pwc_close,
     pwc_get_frame, pwc_msync, pwc_wait_frame},

    {740, "pwc", "pwc740", "Philips_740", "Philips 740 camera",
     pwc_get, pwc_set, pwc_LavaRnd,
     pwc_check, pwc_print,
     pwc_usage, pwc_argv,
     pwc_open, pwc_close,
     pwc_get_frame, pwc_msync, pwc_wait_frame},

    {513, "ov511", "dsbc100", "DSB-C100", "D-Link DSB-C100 camera",
     ov511_get, ov511_set, ov511_LavaRnd,
     ov511_check, ov511_print,
     ov511_usage, ov511_argv,
     ov511_open, ov511_close,
     ov511_get_frame, ov511_msync, ov511_wait_frame},

    /* MUST be end of list */
    {-1, NULL, NULL, NULL, NULL,
     NULL, NULL, NULL,
     NULL, NULL,
     NULL, NULL,
     NULL, NULL,
     NULL, NULL, NULL}
};

/*
 * TYPE_MAX - index of highest cam_switch index that is non-NULL
 * UNVALID_TYPE(x) - true ==> x is not an valid cam_switch[] index
 */
#define TYPE_MAX (((int)sizeof(cam_switch) / (int)sizeof(cam_switch[0])) - 2)
#define VALID_TYPE(x) ((((x) < 0) && ((x) > TYPE_MAX)) ? 1 : 0)


/*
 * onebit - number of 1 bits in an octet
 */
static int onebit[OCTET_CNT] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

/*
 * static functions
 */
static struct camop *find_camtype(char *type_name);
static int compar(const void *, const void *);


/*
 * find_camtype - find a camera type in the camop table
 *
 * given:
 *      type_name       type of camera
 *
 * returns:
 *      camera op table pointer or NULL ==> type not found
 */
static struct camop *
find_camtype(char *type_name)
{
    struct camop *p;

    /*
     * firewall
     */
    if (type_name == NULL) {
	return NULL;
    }

    /*
     * search the list
     */
    for (p = cam_switch; p->type != NULL; ++p) {
	if (strcmp(type_name, p->type) == 0) {
	    return p;
	}
    }
    /* not found */
    return NULL;
}


/*
 * camtype - determine if the camera type is a valid type
 *
 * given:
 *      type_name       name of the type of camera
 *
 * returns:
 *      >= 0 ==> camera type, <0 ==> error,
 *      e.g., LAVACAM_ERR_TYPE ==> invalid or unknown type
 */
int
camtype(char *type_name)
{
    struct camop *p;

    /*
     * firewall
     */
    if (type_name == NULL) {
	return LAVACAM_ERR_ARG;
    }

    /*
     * look for the type
     */
    p = find_camtype(type_name);
    if (p == NULL) {
	return LAVACAM_ERR_TYPE;
    }

    /*
     * return index
     */
    return (p - cam_switch);
}


/*
 * lavacam_print_types - print the supported camera types
 *
 * given:
 *      stream          where to print camera type list
 */
void
lavacam_print_types(FILE * stream)
{
    struct camop *p;

    /*
     * firewall
     */
    if (stream == NULL) {
	fprintf(stderr, "print_camtypes: NULL arg\n");
	return;
    }

    /*
     * print list
     */
    fprintf(stream, "Supported camera types and modules:\n");
    for (p = cam_switch; p->type != NULL; ++p) {
	fprintf(stream, "\t%s\t\t%s\tmodule: %s\n",
		p->type, p->fullname, p->module);
    }
    return;
}


/*
 * lavacam_get - get an open camera's state
 *
 * given:
 *      type    type of camera
 *      cam_fd  open camera file descriptor
 *      cam     pointer to camera state
 *
 * returns:
 *      0 ==> OK, <0 ==> error
 */
int
lavacam_get(int type, int cam_fd, union lavacam *cam)
{
    /*
     * firewall
     */
    if (VALID_TYPE(type) || cam_fd < 0 || cam == NULL) {
	return LAVACAM_ERR_ARG;
    }

    /*
     * get camera parameters
     */
    return cam_switch[type].get(cam_fd, cam);
}


/*
 * lavacam_set - set an open camera's state
 *
 * given:
 *      type    type of camera
 *      cam_fd  open camera file descriptor
 *      cam     pointer to camera state change if mask is non-zero
 *
 * returns:
 *      0 ==> OK, <0 ==> error
 */
int
lavacam_set(int type, int cam_fd, union lavacam *cam)
{
    int check_ret;	/* parameter check return */

    /*
     * firewall
     */
    if (VALID_TYPE(type) || cam_fd < 0 || cam == NULL) {
	return LAVACAM_ERR_ARG;
    }

    /*
     * check values before setting
     */
    check_ret = cam_switch[type].check(cam);
    if (check_ret < 0) {
	return check_ret;
    }

    /*
     * set camera parameters
     */
    return cam_switch[type].set(cam_fd, cam);
}


/*
 * lavacam_LavaRnd - preset change state with LavaRnd entropy optimal values
 *
 * given:
 *      type    type of camera
 *      cam     pointer to camera state
 *
 * returns:
 *      0 ==> OK, <0 ==> error
 */
int
lavacam_LavaRnd(int type, union lavacam *cam)
{
    /*
     * firewall
     */
    if (VALID_TYPE(type) || cam == NULL) {
	return LAVACAM_ERR_ARG;
    }

    /*
     * get camera parameters
     */
    return cam_switch[type].LavaRnd(cam, cam_switch[type].model);
}


/*
 * lavacam_check - determine if the camera state change is valid
 *
 * given:
 *      type    type of camera
 *      cam     pointer to camera state change to be checked
 *
 * returns:
 *      0 ==> masked values of union are OK, <0 ==> error in masked values
 */
int
lavacam_check(int type, union lavacam *cam)
{
    /*
     * firewall
     */
    if (VALID_TYPE(type) || cam == NULL) {
	return LAVACAM_ERR_ARG;
    }

    /*
     * set camera parameters
     */
    return cam_switch[type].check(cam);
}


/*
 * lavacam_print - print state of an open camera
 *
 * given:
 *      type    type of camera
 *      stream  open stream on which to print
 *      cam_fd  open camera descriptor
 *      cam     pointer to camera state
 *      siz     pointer to operation size and buffer structure
 *
 * returns:
 *      0 ==> all masked values are valid, <0 ==> error
 */
int
lavacam_print(int type, FILE * stream, int cam_fd,
	      union lavacam *cam, struct opsize *siz)
{
    /*
     * firewall
     */
    if (stream == NULL || VALID_TYPE(type) || cam_fd < 0 ||
	cam == NULL || siz == NULL) {
	return LAVACAM_ERR_ARG;
    }

    /*
     * set camera parameters
     */
    return cam_switch[type].print(stream, cam_fd, cam, siz);
}


/*
 * lavacam_usage - print an argc/argv usage message
 *
 * given:
 *      type            type of camera
 *      prog            name of calling prog
 *      typename        name of camera type
 *
 * NOTE: The typical way to call this function is:
 *
 *      ret = lavacam_argv(type, argc, argv, cam);
 *      if (ret < 0) {
 *          lavacam_usage(type, argv[0], argv[1]);
 *      }
 *      argc -= ret;
 *      argv += ret;
 */
void
lavacam_usage(int type, char *prog, char *typename)
{
    /*
     * firewall
     */
    if (prog == NULL) {
	fprintf(stderr, "lavacam_usage: called with NULL prog arg\n");
	return;
    }
    if (VALID_TYPE(type)) {
	fprintf(stderr, "%s: lavacam_usage: invalid type: %d\n", prog, type);
	return;
    }

    /*
     * print usage message
     */
    cam_switch[type].usage(prog, typename);
    return;
}


/*
 * lavacam_argv - argc/argv parse, sets a cameras state change and global flags
 *
 * given:
 *      type    type of camera
 *      argc    command line argc count
 *      argv    point to array of command line argument strings
 *      cam     pointer to values to be set if mask is non-zero
 *      flag    flags to set
 *
 * returns:
 *      amount of args to skip, <0 ==> error
 */
int
lavacam_argv(int type, int argc, char **argv, union lavacam *cam,
	     struct lavacam_flag *flag)
{
    int argv_ret;	/* argv parse return */
    int check_ret;	/* parameter check return */

    /*
     * firewall
     */
    if (VALID_TYPE(type) || argc <= 0 || argv == NULL || argv[0] == NULL ||
	cam == NULL || flag == NULL) {
	return LAVACAM_ERR_ARG;
    }

    /*
     * parse command line, turn into camera parameters
     */
    argv_ret = cam_switch[type].argv(argc, argv, cam, flag,
				     cam_switch[type].model);
    if (argv_ret < 0) {
	return argv_ret;
    }

    /*
     * sanity check parameters
     */
    check_ret = cam_switch[type].check(cam);
    if (check_ret < 0) {
	return check_ret;
    }

    /*
     * return argv shift
     */
    return argv_ret;
}


/*
 * lavacam_open - open a camera, verify its type, return opening camera state
 *
 * given:
 *      type        type of camera
 *      devname     camera device file
 *      o_cam       where to place the open camera state
 *      n_cam       is non-NULL, set this new state and channel number
 *                      updated with the final open state if non-NULL
 *      siz         pointer to operation size and buffer structure
 *      def         0 ==> restore saved user settings, 1 ==> keep cur setting
 *      flag        flags set via lavacam_argv()
 *
 * returns:
 *      >=0 ==> camera file descriptor, <0 ==> error
 *
 * NOTE: This function directly sets the open_time struct opsize element
 *       on a successful open.
 */
int
lavacam_open(int type, char *devname, union lavacam *o_cam,
	     union lavacam *n_cam, struct opsize *siz,
	     int def, struct lavacam_flag *flag)
{
    int ret;	/* driver open return */

    /*
     * firewall
     */
    if (VALID_TYPE(type) || devname == NULL || o_cam == NULL || flag == NULL) {
	return LAVACAM_ERR_ARG;
    }

    /*
     * open the camera
     */
    ret = cam_switch[type].open(devname, cam_switch[type].model,
				o_cam, n_cam, siz, def, flag);
    if (ret >= 0) {
	siz->open_time = time(NULL);
    }
    return ret;
}


/*
 * lavacam_close - close an open camera
 *
 * given:
 *      type        type of camera
 *      cam_fd      open camera descriptor
 *      siz         pointer to operation size and buffer structure
 *      flag        flags set via lavacam_argv()
 *
 * returns:
 *      >=0 ==> camera closed, <0 ==> close
 */
int
lavacam_close(int type, int cam_fd, struct opsize *siz,
	      struct lavacam_flag *flag)
{
    /*
     * firewall
     */
    if (VALID_TYPE(type) || cam_fd < 0 || siz == NULL || flag == NULL) {
	return LAVACAM_ERR_ARG;
    }

    /*
     * close the camera
     */
    return cam_switch[type].close(cam_fd, siz, flag);
}


/*
 * lavacam_read - read from the camera
 *
 * given:
 *      type        type of camera
 *      cam_fd      open camera descriptor
 *      siz         pointer to operation size and buffer structure
 *
 * returns:
 *      >= 0 ==> octets read, <0 ==> error
 *
 * NOTE: This function increments the frame_num struct opsize field
 *       on a successful get.
 */
int
lavacam_get_frame(int type, int cam_fd, struct opsize *siz)
{
    int ret;	/* driver get_frame return */

    /*
     * firewall
     */
    if (VALID_TYPE(type) || cam_fd < 0 || siz == NULL) {
	return LAVACAM_ERR_ARG;
    }

    /*
     * read from the camera
     */
    ret = cam_switch[type].get_frame(cam_fd, siz);
    if (ret >= 0) {
	++siz->frame_num;
    }
    return ret;
}


/*
 * compar - qsort compare function to sort a set of ints in reverse order
 *
 * given:
 *      ptr_a   point to 1st int
 *      ptr_b   point to 2nd int
 *
 * returns:
 *      -1 if *ptr_a > *ptr_b,
 *      0 if *ptr_a == *ptr_b,
 *      1 if *ptr_a < *ptr_b
 */
static int
compar(const void *ptr_a, const void *ptr_b)
{
    /*
     * firewall
     */
    if (ptr_a == NULL || ptr_b == NULL) {
	return 0;
    }

    /*
     * compare
     */
    if (*((int *)ptr_a) > *((int *)ptr_b)) {
	return -1;
    } else if (*((int *)ptr_a) < *((int *)ptr_b)) {
	return 1;
    } else {
	return 0;
    }
}


/*
 * lavacam_uncom_fract - determine the fraction of uncommon octets in a frame
 *
 * given:
 *      frame           pointer to the frame
 *      len             length of frame in octets
 *      top_x           ignore the top_x common octet values
 *      p_half_x        where to record 1/2 level value (NULL ==> ignore)
 *
 * returns:
 *      value from 0.0 to 1.0 giving the faction of octets in the frame
 *	that are uncommon.  0.0 means none of the octets are uncommon all
 *	are in the top_x.  As the return value approaches 0.0, more and
 *	more of the octets are uncommon (i.e., greater portion not in
 *	the top_x).
 *
 * The p_half_x, if non-NULL, is where the 1/2 level is recorded.
 * The 1/2 level is the point where the *p_half_x values first exceed
 * 50% of the available octets.
 *
 * NOTE: Reasonable top_x are between 1 and 255.  A top_x of 0 means
 *       there are no common octets (thus all are uncommon).  A top_x
 *       of 256 means all common (this none are uncommon).
 */
double
lavacam_uncom_fract(u_int8_t * frame, int len, int top_x, int *p_half_x)
{
    int tally[OCTET_CNT];	/* tally of octet values in frame */
    int common;			/* number of common octets */
    double uncom_fract;		/* fraction of uncommon octets */
    int half_len;		/* len/2 */
    int hlvl;			/* used to find half_x value */
    int i;

    /*
     * firewall - no frame means no uncommon octets
     */
    if (frame == NULL || len < 1) {
	return 0.0;
    }
    /* case: absurd top_x values */
    if (top_x < 0 ) {
	/* no common means all are uncommon */
	top_x = 0;
    } else if (top_x > OCTET_CNT) {
	/* all common means none are uncommon */
	top_x = OCTET_CNT;
    }

    /*
     * tally the octet values
     */
    half_len = len / 2;
    memset(tally, 0, sizeof(tally));
    for (i = 0; i < len; ++i) {
	tally[(int)frame[i]]++;
    }

    /*
     * sort tally
     */
    qsort(tally, OCTET_CNT, sizeof(tally[0]), compar);

    /*
     * determine the fraction of uncommon octets
     */
    common = 0;
    hlvl = -1;
    for (i = 0; i < top_x; ++i) {
	common += tally[i];
	if (p_half_x != NULL && hlvl < 0 && common >= half_len) {
	    hlvl = i;
	}
    }
    uncom_fract = (double)(len - common) / (double)len;

    /*
     * if we did not find the half level and want to know it, keep looking
     */
    if (p_half_x != NULL && hlvl < 0) {
	for (; i < OCTET_CNT; ++i) {
	    common += tally[i];
	    if (hlvl < 0 && common >= half_len) {
		hlvl = i;
		break;
	    }
	}
    }

    /*
     * save the half level value if we want to know it
     */
    if (p_half_x != NULL) {
	*p_half_x = hlvl;
    }

    /*
     * return uncommon fraction
     */
    return uncom_fract;
}


/*
 * lavacam_bitdiff_fract - determine faction of bits different in two buffers
 *
 * given:
 *      frame1          pointer to the 1st frame
 *      frame2          pointer to the 2nd frame
 *      len             length of frames in octets
 *
 * return:
 *      value from 0.0 to 1.0 giving the faction of bits in frame1 that
 *         are different than in frame2.  A value of 0.0 means that the
 *         frames are exactly the same.  A value of 1.0 means that one
 *         frame is the bit-wise complement of the other frame (all bits
 *         are different).
 *
 * NOTE: Due to counting bits, the maximum length that is actually
 *       examined is 2^32 bits == 2^29 octets.
 */
double
lavacam_bitdiff_fract(u_int8_t * frame1, u_int8_t * frame2, int len)
{
    unsigned long one_bits;	/* 1 bit difference */
    double bitdiff_fract;	/* fraction of different bits */
    int i;

    /*
     * firewall
     */
    if (len < 1) {
	/* no length means no bits are different */
	return 0.0;
    }
    if (frame1 == NULL || frame2 == NULL) {
	/* missing frame(s) means no bits are different */
	return 0.0;
    }
    /* -3 is because there are 8 (2 to the 3rd power) bits per octet */
    if (len >= (1L << ((sizeof(one_bits) * BITS_PER_OCTET) - 3))) {
	/* bit length overflows signed int, reduce length to avoid overflow */
	len = (1L << ((sizeof(one_bits) * BITS_PER_OCTET) - 3)) - 1;
    }

    /*
     * count bit difference
     */
    one_bits = 0;
    for (i = 0; i < len; ++i) {
	one_bits += onebit[frame1[i] ^ frame2[i]];
    }

    /*
     * return the bit difference fraction
     */
    bitdiff_fract = (double)one_bits / (double)(len * BITS_PER_OCTET);
    return bitdiff_fract;
}


/*
 * lavacam_sanity - perform a sanity check the frame just read
 *
 * The purpose of this check is to prevent obvious camera failures
 * and major flaws from being used to compute LavaRnd values.
 *
 * given:
 *      siz         pointer to operation size and buffer structure
 *
 * returns:
 *      0 ==> OK, <0 ==> error
 *
 * NOTE: This function increments the insane_cnt field of the struct opsize
 *       when a frame is returned as insane.
 *
 * NOTE: This function saves a sane frame into the prev_frame struct opsize
 *       buffer for comparison next time.  Insane frames are not copied.
 *
 * TODO: Consider other non-CPU intensive tests to further improve
 *       sanity checks.  Also consider a way to dynamically compute the
 *       top_x, min_fract and diff_fract values for a given camera,
 *       if that is at all possible.
 */
int
lavacam_sanity(struct opsize *siz)
{
    double uncom_fract;		/* fraction of uncommon octets */
    double bitdiff_fract;	/* fraction of different bits */
    int half_x;			/* half_x most common values <50% of octets */

    /*
     * firewall
     */
    if (siz == NULL) {
	return LAVACAM_ERR_ARG;
    }

    /*
     * verify that we have enough uncommon octet values
     *
     * It may be usual for a frame to have a some common octet values
     * (low light level values) and (stuck nearly/fully on values).
     * These common octet values typically are not very entropic.  The
     * entroy typically lies in the less common octet values.
     */
    if (siz->min_fract > 0.0) {
	uncom_fract = lavacam_uncom_fract(siz->chaos,
					  siz->chaos_len, siz->top_x, &half_x);
	if (uncom_fract < siz->min_fract) {
	    /*
	     * The siz->top_x common octet values occupy too many of
	     * the octets in the frame.  There are not enough
	     * uncommon octet values.
	     */
	    ++siz->insane_cnt;
	    return LAVACAM_ERR_UNCOMMON;
	}

	/*
	 * check the half level
	 *
	 * The siz->half_x most common values account for >=50% of the
	 * octets in the frame.  The half_x value indicates that the
	 * half_x most common octet values occupy >=50% of the octets
	 * in the frame.  So we need to compare siz->half_x with half_x.
	 */
	if (half_x < siz->half_x) {
	    /*
	     * The half_x most common octet values, there they first
	     * exceed 50% of the octets in the frame, is too low.
	     * The frame is too dominated by the more common values.
	     */
	    ++siz->insane_cnt;
	    return LAVACAM_ERR_HALFLVL;
	}
    }

    /*
     * verify that frame are not too similar or too different to prev frame
     *
     * If a camera is repeating frames, then they will be bit-for-bit
     * similar if not identical.  If a camera is taking frames too
     * quickly, or if the camera is being subjected to a constant
     * light source, then successive frames will be very similar.
     * If a number of bits are flip-flopping, then successive frames
     * will appear bit-wise complements of each other too many times.
     * Too much similarity is just as bad is too much difference.
     *
     * We do not compare the 1st frame because the prev_frame buffer
     * has not been initialized yet.
     */
    if (siz->frame_num > 1 && siz->diff_fract > 0.0) {
	bitdiff_fract = lavacam_bitdiff_fract(siz->chaos, siz->prev_frame,
					      siz->chaos_len);
	if (bitdiff_fract < siz->diff_fract) {
	    /*
	     * frame is too similar to the previous frame
	     */
	    ++siz->insane_cnt;
	    return LAVACAM_ERR_OVERSAME;
	} else if ((1.0 - bitdiff_fract) < siz->diff_fract) {
	    /*
	     * frame is too different from the previous frame
	     */
	    ++siz->insane_cnt;
	    return LAVACAM_ERR_OVERDIFF;
	}
    }

    /*
     * This frame is not too similar to the previous frame, so make
     * this frame the new previous frame for next time.
     */
    if (siz->diff_fract > 0.0) {
	memcpy(siz->prev_frame, siz->chaos, siz->chaos_len);
    }

    /*
     * sanity check passed
     */
    return LAVAERR_OK;
}


/*
 * lavacam_msync - release/sync after processing a mmap captured buffer
 *
 * given:
 *      type        type of camera
 *      cam_fd      open camera descriptor
 *      cam         pointer to camera parameters
 *      siz         pointer to operation size and buffer structure
 *
 * returns:
 *      0 ==> OK, <0 ==> error
 */
int
lavacam_msync(int type, int cam_fd, union lavacam *cam, struct opsize *siz)
{
    /*
     * firewall
     */
    if (VALID_TYPE(type) || cam_fd < 0 || cam == NULL || siz == NULL) {
	return LAVACAM_ERR_ARG;
    }

    /*
     * mmap the camera
     */
    return cam_switch[type].msync(cam_fd, cam, siz);
}


/*
 * lavacam_wait_frame - use select to wait for the next camera frame
 *
 * given:
 *      type        type of camera
 *      cam_fd      open camera descriptor
 *      max_wait    wait for up to this many seconds, <0.0 ==> infinite
 *
 * returns:
 *      1 ==> frame is ready, 0 ==> timeout, <0 ==> error
 */
int
lavacam_wait_frame(int type, int cam_fd, double max_wait)
{
    /*
     * firewall
     */
    if (VALID_TYPE(type) || cam_fd < 0) {
	return LAVACAM_ERR_ARG;
    }

    /*
     * wait until the next frame
     */
    return cam_switch[type].wait_frame(cam_fd, max_wait);
}
