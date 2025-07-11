/*
 * ov511_camop - perform OmniVision OV511 specific web camera operations
 *
 * @(#) $Revision: 10.3 $
 * @(#) $Id: ov511_drvr.c,v 10.3 2003/08/25 08:45:25 lavarnd Exp $
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
 * NOTE: This is the raw interface to a specific camera.  You should be
 *       calling the lavacam_*() routines in camop.c instead of directly
 *       calling these functions.  For example, call lavacam_open() instead
 *       of ov511_open.  See the comment at the top of src/lib/camop.c for
 *       sample code.  See also some of the tools under src/tool as well.
 */


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>

#include "LavaRnd/rawio.h"
#include "LavaRnd/lavacam.h"
#include "LavaRnd/lavaerr.h"

#include "LavaRnd/have/cam_videodev.h"
#include "LavaRnd/have/have_pselect.h"


/*
 * Special OV511 driver debugging - not related to LavaRnd debugging
 */
#if defined(LAVA_OV511_DEBUG)
#  define DBG(c) fprintf(stderr, \
			  "Error: %s:%d: %d\n", __FILE__, __LINE__, (c))
#  define DBGW(s) fprintf(stderr, \
			  "Warning: %s:%d: %s\n", __FILE__, __LINE__, (s))
#  define DBGS(s,v) fprintf(stderr, \
			  "Set: %s:%d: %s = %d\n", __FILE__, __LINE__ ,(s) ,(v))
#else
#  define DBG(c)
#  define DBGW(c)
#  define DBGS(s,v)
#endif


/*
 * ov511 simulation - simulate the ov511 driver for test on non-Linux hosts
 */
#if defined(OV511_SIMULATION)
#  define ioctl(fd, request, arg) ov511_ioctl((fd), (request), (arg))

static int ov511_ioctl(int fd, int request, void *arg);
#endif /* OV511_SIMULATION */


/*
 * LavaRnd entropy optimal values
 *
 * These magic numbers were determine by experiment with a
 * D-Link DSB-C100 webcam.
 */
static struct ov511_state dsb_c100 = {
    /* these values are read/write */
    OV511_DEF_BRIGHT,		/* max brightness level */
    OV511_MAX_PICT,		/* max hue */
    OV511_MAX_PICT,		/* max colour saturation */
    OV511_MAX_PICT,		/* max image contrast level */
    288,			/* image height in pixels */
    352,			/* image width in pixels */
    OV511_DEF_PALETTE,		/* type of image palette */
    /* there values are write only */
    /* these values are read only */
    0,				/* numerical camera subtype */
    0,				/* minimum width in pixels */
    0,				/* maximum width in pixels */
    0,				/* minimum height in pixels */
    0,				/* maximum height in pixels */
    0,				/* buffer size in pixels */
    0,				/* size of a frame in octets */
    0,				/* number of video buffer frames */
    0,				/* pixel/picture depth in bits */
    /* special driver/utilities elements */
    OV511_STATE_MASK,		/* bit mask of state values to set/change */
    0,				/* video channel num, usually 0, set on open */
    /* tmp sanity check into - initialized by default, set by ov511_argv() */
    0,
    0.0,
    0,
    0.0
};


/*
 * LavaRnd values for given models
 */
struct lava_state {
    int model;			/* camera model - see struct camop in camop.c */
    struct ov511_state *state;	/* optimal LavaRnd entropy values */
    double warmup;		/* default seconds to delay during open */
    int def_top_x;		/* default ignore top_x octets as common */
    double def_min_fract;	/* default uncommon octet value fraction */
    int def_half_x;		/* default 1/2 level value */
    double def_diff_fract;	/* default different/same fraction */
};

/*
 * specific ov511 camera settings
 *
 * The a 4-sigma margin of the average of 131072 frames as reported by
 * the camsantiy tool:
 *
 *	camsanity dsbc100 /dev/video0 /var/tmp/dsbc100_sanity 10000 64 -L -v 2
 *
 * with the ov511 modules.conf setting of:
 *
 *	options ov511 autobright=1 autogain=1 autoexp=1 fastset=1 compress=0 \
 *		led=0 force_palette=15
 *
 * for a D-Link DSB-C100 camera:
 *
 *   top_4
 *
 *      The def_top_x of 4 was picked because the average top_4 level was
 *      near 0.6826895 (1-sigma).
 *
 *   top_4 = 0.567556
 *
 * 	For top_5, the min bitdiff_fract was 0.613153 and standard deviation
 *	was 0.009119.  We go 5 sigma below the min for a margin of error.
 *
 *   half_x = 3
 *
 *      The smallest level with a max uncom_fract > 0.5 was 5 and the
 *	standard deviation was 0.254740.  We go 5 sigma below the min
 *	for a margin of error.
 *
 *   diff_fract = 0.212036
 *
 *	The min bitdiff_fract was 0.229234 and standard deviation
 *	was 0.003440.  We go 5 sigma below the min for a margin of error.
 *
 * Therefore a wide margin of error is used in the lava_state below:
 *
 *    def_top_x = 4
 *    def_min_fract = 0.567556
 *    def_half_x = 3
 *    def_diff_fract = 0.212036
 *
 * If your camera produces insane frames, then perhaps:
 *
 *      your camera is broken and behaving poorly
 *      your camera may be someout unusual, tuning with -x, -X, -d, -2
 *          values might be the answer if you do not have to
 *          tune too far
 *      light is leaking into your camera ... cover it up?
 *      your camera is too cold to generate enough thermal noise
 *
 * If you want to disable, by default, sanity checking, inialize
 * the following lava_state structure elements as follows:
 *
 *	def_top_x = 0
 *	def_min_fract = 0.0
 *	def_half_x = 0
 *	def_diff_fract = 0.0
 *
 * The sanity check values should be dynamically determined so that
 * individual camera and camera enviroment differences can be
 * accounted for automatically. - XXX
 */
static struct lava_state lava_state[] = {
    {513, &dsb_c100, 10.0, 4, 0.567556, 3, 0.212036},	/* D-Link DSB-C100 */

    {-1, NULL, 0.0, 0, 0, 0.0, 0.0}		/* must be last */
};

#define STATE_COUNT (sizeof(lava_state) / sizeof(lava_state[0]))
#define STARTUP_FRAMES (5)	/* try to read at least these frames */


/*
 * ov511_model - determine which LavaRnd value model applies
 *
 * given:
 *      model       camera model number
 *
 * returns:
 *      index into lava_state[] to use, or
 *            <0 (LAVACAM_ERR_IDENT) ==> error
 */
static int
ov511_model(int model)
{
    int i;

    /*
     * look for default state
     */
    for (i = 0; i < STATE_COUNT; ++i) {
	if (model == lava_state[i].model) {
	    break;
	}
    }
    if (i >= STATE_COUNT || lava_state[i].model < 0) {
	return LAVACAM_ERR_IDENT;
    }
    return i;
}


/*
 * ov511_opsize - return read/mmap operation size and buffer structure
 *
 * given:
 *      cam_fd      open camera descriptor
 *      u_cam_p     pointer to camera state
 *      siz         pointer to operation size and buffer structure
 *      flag        flags set via lavacam_argv()
 *
 * returns:
 *       0 ==> OK, read/mmap size, <0 ==> error
 *
 * NOTE: This function is only called from within ov511_open().
 *
 * NOTE: This function loads the working sanity check values from
 *       the temp driver check values.
 *
 * NOTE: This function sets the bufsize, framesize and frames.
 */
static int
ov511_opsize(int cam_fd, union lavacam *u_cam_p, struct opsize *siz,
	   struct lavacam_flag *flag)
{
    struct ov511_state *cam;	/* u_cam_p as ov511 union element */
    struct video_mbuf vm_buf;	/* video mbuf information */
    int chaos_offset;		/* offset in frame for chaotic data */
    int chaos_len;		/* length of chaotic data in frame */
    int ret;			/* return code */

    /*
     * firewall
     */
    if (cam_fd < 0 || u_cam_p == NULL || siz == NULL || flag == NULL) {
	DBG(LAVACAM_ERR_ARG);
	return LAVACAM_ERR_ARG;
    }

    /*
     * pick out the driver specific union element
     */
    cam = &(u_cam_p->ov511);

    /*
     * initialize opsize structure
     */
    memset(siz, 0, sizeof(*siz));
    siz->prev_frame = NULL;
    siz->image = NULL;
    siz->chaos = NULL;

    /*
     * copy over tmp sanity check files into working struct opsize elements
     */
    siz->top_x = cam->tmp_top_x;
    siz->min_fract = cam->tmp_min_fract;
    siz->half_x = cam->tmp_half_x;
    siz->diff_fract = cam->tmp_diff_fract;

    /*
     * get video mbuf information
     */
    memset(&vm_buf, 0, sizeof(vm_buf));
    vm_buf.size = -1;
    vm_buf.frames = -1;
    if (ioctl(cam_fd, VIDIOCGMBUF, &vm_buf) < 0) {
	DBG(LAVACAM_ERR_GETPARAM);
	return LAVACAM_ERR_GETPARAM;
    }
    if (vm_buf.size >= 0) {
	cam->bufsize = vm_buf.size;
    }
    if (vm_buf.frames >= 0) {
	cam->frames = vm_buf.frames;
    }
    memcpy(siz->offsets, vm_buf.offsets, sizeof(siz->offsets));

    /*
     * determine frame size
     */
    ret = frame_size(PALLETTE_VIDEO4LINUX, cam->palette, cam->bufsize,
    		     cam->frames, siz->offsets, cam->height, cam->width);
    if (ret < 0) {
	DBG(ret);
	return ret;
    }
    cam->framesize = ret;

    /*
     * determine location of chaotic data within a frame
     */
    ret = chaos_zone(PALLETTE_VIDEO4LINUX, cam->palette, cam->framesize,
    		     cam->height, cam->width, &chaos_offset, &chaos_len);
    if (ret < 0) {
	DBG(ret);
	return ret;
    }

    /*
     * determine mmap parameters
     */
    if (cam->bufsize > 0 && cam->frames > 0) {

	/* full buffer size */
	siz->mmapsize = cam->bufsize;

	/* use the last frame */
	if (cam->frames < VIDEO_MAX_FRAME &&
	    siz->offsets[cam->frames - 1] > 0) {
	    siz->mmap_lavaoff = siz->offsets[cam->frames - 1];
	} else {
	    siz->mmap_lavaoff = (cam->frames - 1) * cam->framesize;
	}

	/* determine how much of the frame we will use */
	if (flag->A_flag) {
	    /* -A ==> use all the frame */
	    siz->mmap_lavalen = cam->framesize;
	} else {
	    /* use the chaos part of the frame, offset accordingly */
	    siz->mmap_lavalen = chaos_len;
	    siz->mmap_lavaoff += chaos_offset;
	}
    }

    /*
     * determine read parameters
     */
    if (cam->height > 0 && cam->width > 0) {

	/* full buffer size */
	siz->readsize = cam->framesize;

	/* read all the frame */
	siz->read_lavaoff = 0;

	/* determine how much of the frame we will use */
	if (flag->A_flag) {
	    /* -A ==> use all the frame */
	    siz->read_lavalen = cam->framesize;
	} else {
	    /* use the chaos part of the frame, offset accordingly */
	    siz->read_lavalen = chaos_len;
	    siz->read_lavaoff += chaos_offset;
	}
    }

    /*
     * fill in the final opsize information from the camera's state
     */
    siz->frames = cam->frames;
    siz->palette = cam->palette;
    siz->height = cam->height;
    siz->width = cam->width;
    siz->framesize = cam->framesize;

    /*
     * data is complete
     */
    return LAVACAM_ERR_OK;
}


/*
 * ov511_mmap - mmap camera buffer
 *
 * given:
 *      cam_fd      open camera descriptor
 *      u_cam_p     pointer to camera state
 *      siz         pointer to operation size and buffer structure
 *
 * returns:
 *      mmap-ed buffer, or NULL ==> error
 */
static void *
ov511_mmap(int cam_fd, union lavacam *u_cam_p, struct opsize *siz)
{
    struct video_mmap vm;	/* video buffer description */
    void *ret;			/* close return value */

    /*
     * firewall
     */
    if (cam_fd < 0 || u_cam_p == NULL || siz == NULL) {
	DBG(0);
	return NULL;
    }
    if (siz->use_read == TRUE || siz->image_len <= 0) {
	/* -M disabled mmap */
	DBG(0);
	return NULL;
    }

    /*
     * try to mmap the camera
     */
    ret = mmap(NULL, siz->image_len, PROT_READ, MAP_SHARED, cam_fd, 0);
    if (ret == MAP_FAILED) {
	DBG(0);
	return NULL;
    }
    siz->image = ret;

    /*
     * set mmap image size
     */
    vm.frame = siz->frames - 1;
    vm.format = siz->palette;
    vm.height = siz->height;
    vm.width = siz->width;
    if (ioctl(cam_fd, VIDIOCMCAPTURE, &vm) < 0) {
	DBG(0);
	return NULL;
    }

    /*
     * sync our first frame
     */
    if (ov511_msync(cam_fd, u_cam_p, siz) < 0) {
	DBG(0);
	return NULL;
    }
    return ret;
}


/*
 * ov511_get - get an open camera's state
 *
 * given:
 *      cam_fd      open camera descriptor
 *      u_cam_p     pointer to values of the camera
 *
 * returns:
 *      0 ==> OK, <0 ==> error
 *
 * NOTE: This function will clear out the tmp sanity check values.
 */
int
ov511_get(int cam_fd, union lavacam *u_cam_p)
{
    struct video_capability vcap;	/* video capability */
    char camname[sizeof(vcap.name)+1];	/* camera name */
    struct video_picture vpic;		/* video picture status */
    struct video_window vwin;		/* video window info */
    struct video_mbuf vm_buf;		/* video mbuf information */
    struct ov511_state *cam;		/* u_cam_p as ov511 union element */

    /*
     * firewall
     */
    if (cam_fd < 0 || u_cam_p == NULL) {
	DBG(LAVACAM_ERR_ARG);
	return LAVACAM_ERR_ARG;
    }

    /*
     * pick out the driver specific union element
     */
    cam = &(u_cam_p->ov511);

    /*
     * initialize cam state
     */
    memset(cam, 0, sizeof(cam[0]));

    /*
     * verify that we have a Phillis camera
     */
    vcap.maxwidth = -1;
    vcap.maxheight = -1;
    vcap.minwidth = -1;
    vcap.minheight = -1;
    memset(&vcap, 0, sizeof(vcap));
    vcap.type = -1;
    if (ioctl(cam_fd, VIDIOCGCAP, &vcap) < 0) {
	DBG(LAVACAM_ERR_NOCAM);
	return LAVACAM_ERR_NOCAM;
    }
    strncpy(camname, vcap.name, sizeof(vcap.name));
    camname[sizeof(vcap.name)] = '\0';
    if (camname[0] == '\0') {
	DBG(LAVACAM_ERR_IDENT);
	return LAVACAM_ERR_IDENT;
    }
    if (vcap.type >= 0) {
	cam->subtype = vcap.type;
    }
    if (vcap.minheight > 0) {
	cam->minheight = vcap.minheight;
    }
    if (vcap.maxheight > 0) {
	cam->maxheight = vcap.maxheight;
    }
    if (vcap.minwidth > 0) {
	cam->minwidth = vcap.minwidth;
    }
    if (vcap.maxwidth > 0) {
	cam->maxwidth = vcap.maxwidth;
    }

    /*
     * get current video picture data
     */
    memset(&vpic, 0, sizeof(vpic));
    if (ioctl(cam_fd, VIDIOCGPICT, &vpic) < 0) {
	DBG(LAVACAM_ERR_GETPARAM);
	return LAVACAM_ERR_GETPARAM;
    }
    cam->bright = vpic.brightness;
    cam->hue = vpic.hue;
    cam->colour = vpic.colour;
    cam->contrast = vpic.contrast;
    cam->depth = vpic.depth;
    cam->palette = vpic.palette;

    /*
     * obtain video window information
     */
    memset(&vwin, 0, sizeof(vwin));
    if (ioctl(cam_fd, VIDIOCGWIN, &vwin) < 0) {
	DBG(LAVACAM_ERR_GETPARAM);
	return LAVACAM_ERR_GETPARAM;
    }
    cam->height = vwin.height;
    cam->width = vwin.width;

    /*
     * get video mbuf information
     */
    memset(&vm_buf, 0, sizeof(vm_buf));
    vm_buf.size = -1;
    vm_buf.frames = -1;
    if (ioctl(cam_fd, VIDIOCGMBUF, &vm_buf) < 0) {
	DBG(LAVACAM_ERR_GETPARAM);
	return LAVACAM_ERR_GETPARAM;
    }
    if (vm_buf.size >= 0) {
	cam->bufsize = vm_buf.size;
    }
    if (vm_buf.frames >= 0) {
	cam->frames = vm_buf.frames;
    }

    /*
     * all done
     */
    return LAVACAM_ERR_OK;
}


/*
 * ov511_set - set an open camera's state
 *
 * given:
 *      cam_fd      open camera descriptor
 *      u_cam_p     pointer to new camera setting if mask is non-zero
 *
 * returns:
 *      0 ==> OK, <0 ==> error
 *
 * NOTE: The tmp sanity check values are ignored by this function.
 */
int
ov511_set(int cam_fd, union lavacam *u_cam_p)
{
    struct video_capability vcap;	/* video capability */
    char camname[sizeof(vcap.name)+1];	/* camera name */
    struct video_picture vpic;		/* video picture status */
    struct video_window vwin;		/* video window info */
    struct ov511_state *cam;		/* u_cam_p as ov511 union element */

    /*
     * firewall
     */
    if (cam_fd < 0 || u_cam_p == NULL) {
	DBG(LAVACAM_ERR_ARG);
	return LAVACAM_ERR_ARG;
    }

    /*
     * pick out the driver specific union element
     */
    cam = &(u_cam_p->ov511);

    /*
     * quick exit if mask is empty
     */
    if (cam->mask == 0) {
	return LAVACAM_ERR_OK;
    }

    /*
     * verify that we have an ov511 camera
     */
    memset(&vcap, 0, sizeof(vcap));
    if (ioctl(cam_fd, VIDIOCGCAP, &vcap) < 0) {
	DBG(LAVACAM_ERR_NOCAM);
	return LAVACAM_ERR_NOCAM;
    }
    strncpy(camname, vcap.name, sizeof(vcap.name));
    camname[sizeof(vcap.name)] = '\0';
    if (camname[0] == '\0') {
	DBG(LAVACAM_ERR_IDENT);
	return LAVACAM_ERR_IDENT;
    }

    /*
     * set video picture if requested
     */
    if (OV511_TEST(cam->mask, bright) ||
	OV511_TEST(cam->mask, hue) ||
	OV511_TEST(cam->mask, colour) ||
        OV511_TEST(cam->mask, contrast)) {

	/*
	 * get current video picture data
	 */
	memset(&vpic, 0, sizeof(vpic));
	if (ioctl(cam_fd, VIDIOCGPICT, &vpic) < 0) {
	    DBG(LAVACAM_ERR_GETPARAM);
	    return LAVACAM_ERR_GETPARAM;
	}

	/*
	 * change picture values per flags
	 */
	if (OV511_TEST(cam->mask, bright)) {
	    vpic.brightness = cam->bright;
	    DBGS("brightness", vpic.brightness);
	}
	if (OV511_TEST(cam->mask, hue)) {
	    vpic.hue = cam->hue;
	    DBGS("hue", vpic.hue);
	}
	if (OV511_TEST(cam->mask, colour)) {
	    vpic.colour = cam->colour;
	    DBGS("colour", vpic.colour);
	}
	if (OV511_TEST(cam->mask, contrast)) {
	    vpic.contrast = cam->contrast;
	    DBGS("contrast", vpic.contrast);
	}

	/*
	 * set new video picture data
	 */
	if (ioctl(cam_fd, VIDIOCSPICT, &vpic) < 0) {
	    DBG(LAVACAM_ERR_SETPARAM);
	    return LAVACAM_ERR_SETPARAM;
	}
    }

    /*
     * set camera palette
     *
     * Some/most cameras to not support some/most palettes.  For example,
     * the D-Link DSB-C100 camera only supports palettes:
     *
     *		1  VIDEO_PALETTE_GREY
     *		3  VIDEO_PALETTE_RGB565
     *		4  VIDEO_PALETTE_RGB24
     *		7  VIDEO_PALETTE_YUV422
     *		8  VIDEO_PALETTE_YUYV
     *	       10  VIDEO_PALETTE_YUV420
     *	       13  VIDEO_PALETTE_YUV422P
     *	       15  VIDEO_PALETTE_YUV420P
     *
     * If we fail to set the palettes to the requested value, we will
     * return LAVACAM_ERR_PALETTE instead of LAVACAM_ERR_SETPARAM.
     */
    if (OV511_TEST(cam->mask, palette)) {

	/*
	 * get current video picture data
	 */
	memset(&vpic, 0, sizeof(vpic));
	if (ioctl(cam_fd, VIDIOCGPICT, &vpic) < 0) {
	    DBG(LAVACAM_ERR_GETPARAM);
	    return LAVACAM_ERR_GETPARAM;
	}

	/*
	 * change the palette
	 */
	vpic.palette = cam->palette;
	DBGS("palette", vpic.palette);

	/*
	 * set new video picture with the new palette
	 */
	if (ioctl(cam_fd, VIDIOCSPICT, &vpic) < 0) {
	    DBG(LAVACAM_ERR_PALETTE);
	    return LAVACAM_ERR_PALETTE;
	}
    }

    /*
     * change video window values if requested
     */
    if (OV511_TEST(cam->mask, height) ||
        OV511_TEST(cam->mask, width)) {

	/*
	 * obtain video window information
	 */
	memset(&vwin, 0, sizeof(vwin));
	if (ioctl(cam_fd, VIDIOCGWIN, &vwin) < 0) {
	    DBG(LAVACAM_ERR_GETPARAM);
	    return LAVACAM_ERR_GETPARAM;
	}

	/*
	 * change video window values per flags
	 */
	if (OV511_TEST(cam->mask, height)) {
	    vwin.height = cam->height;
	    DBGS("height", vwin.height);
	}
	if (OV511_TEST(cam->mask, width)) {
	    vwin.width = cam->width;
	    DBGS("width", vwin.width);
	}

	/*
	 * set the new video window values
	 */
	if (ioctl(cam_fd, VIDIOCSWIN, &vwin) < 0) {
	    DBG(LAVACAM_ERR_SETPARAM);
	    return LAVACAM_ERR_SETPARAM;
	}
    }

    /*
     * all done
     */
    return LAVACAM_ERR_OK;
}


/*
 * ov511_LavaRnd - preset change state with LavaRnd entropy optimal values
 *
 * given:
 *      u_cam_p     pointer to camera state
 *      model       camera model number
 *
 * returns:
 *      0 ==> OK, <0 ==> error
 *
 * NOTE: This function will set the tmp sanity check faules to their
 *       defaults according to the lava_state[] default values.
 */
int
ov511_LavaRnd(union lavacam *u_cam_p, int model)
{
    int model_indx;

    /*
     * firewall
     */
    if (u_cam_p == NULL) {
	DBG(LAVACAM_ERR_ARG);
	return LAVACAM_ERR_ARG;
    }

    /*
     * look for default state index
     */
    model_indx = ov511_model(model);
    if (model_indx < 0) {
	return model_indx;
    }

    /*
     * load the LavaRnd entropy optimal values
     */
    memcpy(&(u_cam_p->ov511), lava_state[model_indx].state,
	   sizeof(u_cam_p->ov511));

    /*
     * set the default tmp sanity check values
     */
    u_cam_p->ov511.tmp_top_x = lava_state[model_indx].def_top_x;
    u_cam_p->ov511.tmp_min_fract = lava_state[model_indx].def_min_fract;
    u_cam_p->ov511.tmp_half_x = lava_state[model_indx].def_half_x;
    u_cam_p->ov511.tmp_diff_fract = lava_state[model_indx].def_diff_fract;
    return LAVACAM_ERR_OK;
}


/*
 * ov511_check - determine if the camera state change is valid
 *
 * given:
 *      u_cam_p     pointer to camera values to check if mask is non-zero
 *
 * returns:
 *      0 ==> masked values of union are OK, <0 ==> error in masked values
 */
int
ov511_check(union lavacam *u_cam_p)
{
    struct ov511_state *cam;	/* u_cam_p as ov511 union element */

    /*
     * firewall
     */
    if (u_cam_p == NULL) {
	DBG(LAVACAM_ERR_ARG);
	return LAVACAM_ERR_ARG;
    }

    /*
     * pick out the driver specific union element
     */
    cam = &(u_cam_p->ov511);

    /*
     * quick exit if mask is empty
     */
    if (cam->mask == 0) {
	return LAVACAM_ERR_OK;
    }

    /*
     * check values if the mask allows
     */
    if (OV511_TEST(cam->mask, bright)) {
	if (cam->bright < OV511_MIN_PICT || cam->bright > OV511_MAX_PICT) {
	    DBG(LAVACAM_ERR_RANGE);
	    return LAVACAM_ERR_RANGE;
	}
    }
    if (OV511_TEST(cam->mask, hue)) {
	if (cam->hue < OV511_MIN_PICT || cam->hue > OV511_MAX_PICT) {
	    DBG(LAVACAM_ERR_RANGE);
	    return LAVACAM_ERR_RANGE;
	}
    }
    if (OV511_TEST(cam->mask, colour)) {
	if (cam->colour < OV511_MIN_PICT || cam->colour > OV511_MAX_PICT) {
	    DBG(LAVACAM_ERR_RANGE);
	    return LAVACAM_ERR_RANGE;
	}
    }
    if (OV511_TEST(cam->mask, contrast)) {
	if (cam->contrast < OV511_MIN_PICT || cam->contrast > OV511_MAX_PICT) {
	    DBG(LAVACAM_ERR_RANGE);
	    return LAVACAM_ERR_RANGE;
	}
    }
    if (OV511_TEST(cam->mask, palette)) {
	if (cam->palette < VIDEO_PALETTE_GREY ||
	    cam->palette > VIDEO_PALETTE_YUV410P) {
	    DBG(LAVACAM_ERR_RANGE);
	    return LAVACAM_ERR_RANGE;
	}
    }
    if (OV511_TEST(cam->mask, height)) {
	if (cam->subtype >= 0 && cam->minheight > 0 && cam->maxheight > 0) {
	    if (cam->height < cam->minheight || cam->height > cam->maxheight) {
		DBG(LAVACAM_ERR_RANGE);
		return LAVACAM_ERR_RANGE;
	    }
	} else {
	    if (cam->height < OV511_MIN_SIZE || cam->height > OV511_MAX_PICT) {
		DBG(LAVACAM_ERR_RANGE);
		return LAVACAM_ERR_RANGE;
	    }
	}
    }
    if (OV511_TEST(cam->mask, width)) {
	if (cam->subtype >= 0 && cam->minwidth > 0 && cam->maxwidth > 0) {
	    if (cam->width < cam->minwidth || cam->width > cam->maxwidth) {
		DBG(LAVACAM_ERR_RANGE);
		return LAVACAM_ERR_RANGE;
	    }
	} else {
	    if (cam->width < OV511_MIN_SIZE || cam->width > OV511_MAX_PICT) {
		DBG(LAVACAM_ERR_RANGE);
		return LAVACAM_ERR_RANGE;
	    }
	}
    }

    /*
     * all is OK
     */
    return LAVACAM_ERR_OK;
}


/*
 * ov511_print - print state of an open camera
 *
 * given:
 *      stream      where to print camera info
 *      cam_fd      open camera file descriptor
 *      u_cam_p     pointer to camera state
 *      siz         pointer to operation size and buffer structure
 *
 * returns:
 *      0 ==> unable to obtain camera information
 *      1 ==> camera information obtained and printed
 */
int
ov511_print(FILE * stream, int cam_fd, union lavacam *u_cam_p,
	    struct opsize *siz)
{
    struct ov511_state *cam;		/* u_cam_p as ov511 union element */
    struct video_capability vcap;	/* video capability */
    char camname[sizeof(vcap.name)+1];	/* camera name and NUL byte */

    /*
     * firewall
     */
    if (stream == NULL || cam_fd < 0 || u_cam_p == NULL || siz == NULL) {
	DBG(LAVACAM_ERR_ARG);
	return LAVACAM_ERR_ARG;
    }

    /*
     * pick out the driver specific union element
     */
    cam = &(u_cam_p->ov511);

    /*
     * obtain camera name
     */
    memset(vcap.name, 0, sizeof(vcap.name));
    if (ioctl(cam_fd, VIDIOCGCAP, &vcap) < 0) {
	fprintf(stream, "ov511_print: unable to get video capability\n");
	DBG(LAVACAM_ERR_GETPARAM);
	return LAVACAM_ERR_GETPARAM;
    }
    strncpy(camname, vcap.name, sizeof(vcap.name));
    camname[sizeof(vcap.name)] = '\0';

    /*
     * print the camera name
     */
    if (vcap.name[0] == '\0') {
	fprintf(stream, "\tCamera name: <<__unknown__>>\n");
    } else {
	fprintf(stream, "\tCamera name: %s\n", camname);
    }

    /*
     * print ov511 camera type
     */
    if (cam->subtype >= 0) {
	fprintf(stream, "\tov511 camera type: %d\n", cam->subtype);
    } else {
	fprintf(stream, "\tNot an ov511 camera\n");
    }

    /*
     * print picture information
     */
    if (cam->bright != OV511_UNSET_PICT) {
	fprintf(stream, "\tPicture brightness: %d\n", cam->bright);
    }
    if (cam->hue != OV511_UNSET_PICT) {
	fprintf(stream, "\tPicture hue: %d\n", cam->hue);
    }
    if (cam->colour != OV511_UNSET_PICT) {
	fprintf(stream, "\tPicture colour: %d\n", cam->colour);
    }
    if (cam->contrast != OV511_UNSET_PICT) {
	fprintf(stream, "\tPicture contrast: %d\n", cam->contrast);
    }
    if (cam->depth != OV511_UNSET_PICT) {
	fprintf(stream, "\tPicture depth: %d\n", cam->depth);
    }

    /*
     * print min/max width/height
     */
    if (cam->minwidth >= OV511_MIN_SIZE) {
	fprintf(stream, "\tMin width: %d\n", cam->minwidth);
    } else {
	fprintf(stream, "\tUnknown min width\n");
    }
    if (cam->minheight >= OV511_MIN_SIZE) {
	fprintf(stream, "\tMin height: %d\n", cam->minheight);
    } else {
	fprintf(stream, "\tUnknown min height\n");
    }
    if (cam->maxwidth >= OV511_MIN_SIZE) {
	fprintf(stream, "\tMax width: %d\n", cam->maxwidth);
    } else {
	fprintf(stream, "\tUnknown max width\n");
    }
    if (cam->maxheight >= OV511_MIN_SIZE) {
	fprintf(stream, "\tMax height: %d\n", cam->maxheight);
    } else {
	fprintf(stream, "\tUnknown max height\n");
    }

    /*
     * print the video window information
     */
    if (cam->width >= OV511_MIN_SIZE) {
	fprintf(stream, "\tWidth: %d\n", cam->width);
    } else {
	fprintf(stream, "\tUnknown width\n");
    }
    if (cam->height >= OV511_MIN_SIZE) {
	fprintf(stream, "\tHeight: %d\n", cam->height);
    } else {
	fprintf(stream, "\tUnknown height\n");
    }

    /*
     * print video buffer information
     */
    if (cam->bufsize >= 0) {
	fprintf(stream, "\tVideo mbuf size: %d\n", cam->bufsize);
    } else {
	fprintf(stream, "\tUnknown mbuf size\n");
    }
    if (cam->frames >= 0) {
	fprintf(stream, "\tVideo mbuf count: %d\n", cam->frames);
    } else {
	fprintf(stream, "\tUnknown mbuf count\n");
    }

    /*
     * print palette information
     */
    fprintf(stream, "\t%s (%d)\n",
    		    palette_name(PALLETTE_VIDEO4LINUX, cam->palette),
		    cam->palette);

    /*
     * output working sanity check parameters
     */
    fprintf(stream, "\ncamera sanity check parameters:\n");
    fprintf(stream, "\t%d most frequent octet values are considered common\n",
    		    siz->top_x);
    fprintf(stream, "\tmax fraction of frame containing common octet "
    		    "values: %.6f\n",
    		    1.0 - siz->min_fract);
    fprintf(stream, "\t%d most common values must be < 1/2 of frame octets\n",
    		    siz->half_x);
    fprintf(stream, "\tmin fraction of bits same in next frame: %.6f\n",
    		    siz->diff_fract);
    fprintf(stream, "\tmax fraction of bits same in next frame: %.6f\n",
    		    1.0 - siz->diff_fract);

    /*
     * output use fields
     */
    fprintf(stream, "\ncamera use since this process opened the camera:\n");
    /* ctime returns a newline, so the next printf should not end in one */
    fprintf(stream, "\tcamera open time: %s", ctime(&siz->open_time));
    fprintf(stream, "\tframe count: %lld\n", siz->frame_num);
    fprintf(stream, "\tinsane frame count: %lld\n", siz->insane_cnt);

    /*
     * output opsizes
     */
    fprintf(stream, "\ncamera op sizes:\n");
    if (siz->readsize > 0) {
	fprintf(stream, "\tread size: %d\n", siz->readsize);
	fprintf(stream, "\tread LavaRnd offset: %d\n", siz->read_lavaoff);
	fprintf(stream, "\tread LavaRnd length: %d\n", siz->read_lavalen);
    } else {
	fprintf(stream, "\treading not allowed\n");
    }
    if (siz->mmapsize > 0) {
	fprintf(stream, "\n\tmmap size: %d\n", siz->mmapsize);
	fprintf(stream, "\tmmap frame size: %d\n", siz->framesize);
	fprintf(stream, "\tmmap frames: %d\n", siz->frames);
	fprintf(stream, "\tmmap LavaRnd offset: %d\n", siz->mmap_lavaoff);
	fprintf(stream, "\tmmap LavaRnd length: %d\n", siz->mmap_lavalen);
    } else {
	fprintf(stream, "\n\tmmaping not allowed\n");
    }
    if (siz->use_read) {
	fprintf(stream, "\n\twill use read I/O\n");
	fprintf(stream, "\tread image: %d\n", siz->image_len);
    } else {
	fprintf(stream, "\n\twill use mmap I/O\n");
	fprintf(stream, "\tmmap image: %d\n", siz->image_len);
    }
    fprintf(stream, "\tchaos size: %d\n", siz->chaos_len);
    if (siz->image != NULL && siz->chaos != NULL) {
	fprintf(stream, "\tchaos offset: %d\n",
		(int)(siz->chaos - siz->image));
    } else {
	fprintf(stream, "\tchaos offset: UNKNOWN\n");
    }

    /*
     * all done
     */
    return 1;
}


/*
 * ov511_usage - print an argc/argv usage message
 *
 * given:
 *      prog            program name
 *      typename        name of camera type
 */
void
ov511_usage(char *prog, char *typename)
{
    /*
     * firewall
     */
    if (prog == NULL) {
	prog = "<<__NULL__>>";
    }
    if (typename == NULL) {
	typename = "<<__NULL__>>";
    }

    /*
     * print usage message to stderr
     */
    fprintf(stderr,
	    "usage: %s %s devname [-flags ... args ...]\n"
	    "\n"
	    "\t-b bright\tbrightness level [0..%d]\n"
	    "\t-h hue\t\thue level [0..%d]\n"
	    "\t-c colour\tcolour level [0..%d]\n"
	    "\t-n contrast\tcontrast level [0..%d]\n"
	    "\t-g arg\t\tUNUSED\n"
	    "\t-H height\theight in pixels (>=%d)\n"
	    "\t-W width\twidth in pixels (>=%d)\n"
	    "\t-f arg\t\tUNUSED\n"
	    "\t-C arg\t\tUNUSED\n"
	    "\t-a arg\t\tUNUSED\n"
	    "\t-s arg\t\tUNUSED\n"
	    "\t-m arg\t\tUNUSED\n"
	    "\t-R arg\t\tUNUSED\n"
	    "\t-B arg\t\tUNUSED\n"
	    "\t-o arg\t\tUNUSED\n"
	    "\t-O arg\t\tUNUSED\n"
	    "\t-S arg\t\tUNUSED\n"
	    "\t-l arg\t\tUNUSED\n"
	    "\t-F arg\t\tUNUSED\n"
	    "\t-N arg\t\tUNUSED\n"
	    "\t-P palette\tset camera frame palette [%d..%d]\n"
	    "\t-v level\tverbose mode, print settings before/after\n"
	    "\t-T tsec\t\twarm-up/sleep for tsec seconds\n"
	    "\t-D tsec\t\tframe delay tsec seconds between frames\n"
	    "\t-V vid_chan\tvideo channel (>= 0)\n"
	    "\t-M\t\tdisable mmap processing, use reads instead\n"
	    "\t-L\t\tset LavaRnd entropy optimal mode\n"
	    "\t-A\t\tprocess all of the image, not just the high entropy part\n"
	    "\t-E\t\tavoid frame dumping when savefile exists\n"
	    "\t-X top_x\ttop_x common octets are common octets [0, %d]\n"
	    "\t-x min_fract\tmin fraction uncommon octets in frame [0.0, 1.0]\n"
	    "\t-2 half_x\thalf_x common octets occupy <50%% octets [0, %d]\n"
	    "\t-d min_fract\tmin fraction of diff bits between frames [0.0, 0.5]\n",
	    prog, typename,
	    OV511_MAX_PICT,
	    OV511_MAX_PICT,
	    OV511_MAX_PICT,
	    OV511_MAX_PICT,
	    OV511_MIN_SIZE,
	    OV511_MIN_SIZE,
	    OV511_MIN_PALETTE, OV511_MAX_PALETTE,
	OCTET_CNT, OCTET_CNT/2);
    return;
}


/*
 * ov511_argv - argc/argv parse, sets a camera's state change and global flags
 *
 * given:
 *      argc        command line argc count
 *      argv        point to array of command line argument strings
 *      u_cam_p     pointer to values to be set if mask is non-zero
 *      flag        flags to set
 *      model       camera model number
 *
 * returns:
 *      amount of args to skip, <0 ==> error
 *
 * NOTE: This function will initialize the tmp sanity check values to
 *       their defaults according to the lava_state[] default values.
 *       If -L is was given, the tmp sanity check values will be
 *       reloaded according to the lava_state[] default values.
 *       If -x, -X, -d was given, the tmp sanity check values will be
 *       modified according to the flag.  Sometime later during the
 *       ov511_open(), these tmp sanity check values will be loaded
 *       into the working sanity check values in the struct opsize.
 */
int
ov511_argv(int argc, char **argv, union lavacam *u_cam_p,
	 struct lavacam_flag *flag, int model)
{
    int tmp;			/* temporary holder of a parsed argument */
    double dtmp;		/* temporary holder of double/float arg */
    struct ov511_state *cam;	/* u_cam_p as ov511 union element */
    char *optarg;		/* option argument */
    int c;			/* option */
    int model_indx;		/* default state model index */
    int i;

    /*
     * firewall
     */
    if (argc <= 0 || argv == NULL || argv[0] == NULL || u_cam_p == NULL ||
	flag == NULL) {
	DBG(LAVACAM_ERR_ARG);
	return LAVACAM_ERR_ARG;
    }
    flag->program = argv[0];

    /*
     * pick out the driver specific union element
     */
    cam = &(u_cam_p->ov511);

    /*
     * look for default state index
     */
    model_indx = ov511_model(model);
    if (model_indx < 0) {
	return model_indx;
    }

    /*
     * initialize
     */
    memset(cam, 0, sizeof(cam[0]));
    flag->D_flag = 0.0;
    flag->T_flag = -1.0;
    flag->A_flag = 0;
    flag->M_flag = 1;
    flag->v_flag = 0;
    flag->E_flag = 0;
    flag->savefile = NULL;
    flag->newfile = NULL;
    flag->interval = 0.0;
    /* set the default tmp sanity check values */
    cam->tmp_top_x = lava_state[model_indx].def_top_x;
    cam->tmp_min_fract = lava_state[model_indx].def_min_fract;
    cam->tmp_half_x = lava_state[model_indx].def_half_x;
    cam->tmp_diff_fract = lava_state[model_indx].def_diff_fract;

    /*
     * parse args
     *
     * We would like to use getopt(), but many implementations do not
     * allow one to call getopt() with a different set of args.  Thus
     * if one uses getopt() to parse the command line arguments, one
     * cannot later on use getopt() to parse the driver args.
     *
     * We are forced to roll our own simple arg parser.  *sigh*
     */
    for (i = 1; i < argc; ++i) {

	/*
	 * must be a -<char>
	 */
	if (argv[i] == NULL) {
	    fprintf(stderr, "%s: ov511_argv[%d] is NULL\n", argv[0], i);
	    DBG(LAVACAM_ERR_ARG);
	    return LAVACAM_ERR_ARG;
	}
	if (argv[i][0] != '-') {
	    /* end of -options, stop parsing args */
	    break;
	}

	/*
	 * process the -option
	 */
	c = (int)argv[i][1];
	switch (c) {
	case 'b':
	    if (i < argc - 1) {
		optarg = argv[++i];
	    } else {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    tmp = (int)strtol(optarg, NULL, 0);
	    if (tmp < OV511_MIN_PICT || tmp > OV511_MAX_PICT) {
		fprintf(stderr, "%s: brightness must be in range [%d..%d]\n",
			flag->program, OV511_MIN_PICT, OV511_MAX_PICT);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    cam->bright = tmp;
	    OV511_SET(cam->mask, bright);
	    break;
	case 'h':
	    if (i < argc - 1) {
		optarg = argv[++i];
	    } else {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    tmp = (int)strtol(optarg, NULL, 0);
	    if (tmp < OV511_MIN_PICT || tmp > OV511_MAX_PICT) {
		fprintf(stderr, "%s: hue must be in range [%d..%d]\n",
			flag->program, OV511_MIN_PICT, OV511_MAX_PICT);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    cam->hue = tmp;
	    OV511_SET(cam->mask, hue);
	    break;
	case 'c':
	    if (i < argc - 1) {
		optarg = argv[++i];
	    } else {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    tmp = (int)strtol(optarg, NULL, 0);
	    if (tmp < OV511_MIN_PICT || tmp > OV511_MAX_PICT) {
		fprintf(stderr, "%s: colour must be in range [%d..%d]\n",
			flag->program, OV511_MIN_PICT, OV511_MAX_PICT);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    cam->colour = tmp;
	    OV511_SET(cam->mask, colour);
	    break;
	case 'n':
	    if (i < argc - 1) {
		optarg = argv[++i];
	    } else {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    tmp = (int)strtol(optarg, NULL, 0);
	    if (tmp < OV511_MIN_PICT || tmp > OV511_MAX_PICT) {
		fprintf(stderr, "%s: contrast must be in range [%d..%d]\n",
			flag->program, OV511_MIN_PICT, OV511_MAX_PICT);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    cam->contrast = tmp;
	    OV511_SET(cam->mask, contrast);
	    break;
	case 'g':
	    /* gamma is not used - silently ignore */
	    if (i >= argc - 1) {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    break;
	case 'H':
	    if (i < argc - 1) {
		optarg = argv[++i];
	    } else {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    tmp = (int)strtol(optarg, NULL, 0);
	    if (tmp < OV511_MIN_SIZE) {
		fprintf(stderr, "%s: height must be >= %d\n",
			flag->program, OV511_MIN_SIZE);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    cam->height = tmp;
	    OV511_SET(cam->mask, height);
	    break;
	case 'W':
	    if (i < argc - 1) {
		optarg = argv[++i];
	    } else {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    tmp = (int)strtol(optarg, NULL, 0);
	    if (tmp < OV511_MIN_SIZE) {
		fprintf(stderr, "%s: width must be >= %d\n",
			flag->program, OV511_MIN_SIZE);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    cam->width = tmp;
	    OV511_SET(cam->mask, width);
	    break;
	case 'f':
	    /* frames per second is not used - silently ignore */
	    if (i >= argc - 1) {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    break;
	case 'C':
	    /* compression is not used - silently ignore */
	    if (i >= argc - 1) {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    break;
	case 'a':
	    /* AGC is not used - silently ignore */
	    if (i >= argc - 1) {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    break;
	case 's':
	    /* shutter speed is not used - silently ignore */
	    if (i >= argc - 1) {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    break;
	case 'm':
	    /* white balance mode is not used - silently ignore */
	    if (i >= argc - 1) {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    break;
	case 'R':
	    /* red white balance level is not used - silently ignore */
	    if (i >= argc - 1) {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    break;
	case 'B':
	    /* blue white balance level is not used - silently ignore */
	    if (i >= argc - 1) {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    break;
	case 'o':
	    /* LED on msec is not used - silently ignore */
	    if (i >= argc - 1) {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    break;
	case 'O':
	    /* LED off msec is not used - silently ignore */
	    if (i >= argc - 1) {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    break;
	case 'S':
	    /* electronic sharpness is not used - silently ignore */
	    if (i >= argc - 1) {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    break;
	case 'l':
	    /* backlight compression is not used - silently ignore */
	    if (i >= argc - 1) {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    break;
	case 'F':
	    /* image flicker suppression is not used - silently ignore */
	    if (i >= argc - 1) {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    break;
	case 'N':
	    /* dynamic noise reduction is not used - silently ignore */
	    if (i >= argc - 1) {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    break;
	case 'P':
	    if (i < argc - 1) {
		optarg = argv[++i];
	    } else {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    tmp = (int)strtol(optarg, NULL, 0);
	    if (tmp < OV511_MIN_PALETTE || tmp > OV511_MAX_PALETTE) {
		fprintf(stderr, "%s: type of image palette must be "
			"in the range [%d..%d]\n",
			flag->program, OV511_MIN_PALETTE, OV511_MAX_PALETTE);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    cam->palette = tmp;
	    OV511_SET(cam->mask, palette);
	    break;
	case 'v':
	    if (i < argc - 1) {
		optarg = argv[++i];
	    } else {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    tmp = (int)strtol(optarg, NULL, 0);
	    if (tmp < 0) {
		fprintf(stderr, "%s: verbose level must be >= 0\n",
			flag->program);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    flag->v_flag = tmp;
	    break;
	case 'E':
	    flag->E_flag = 1;
	    break;
	case 'T':
	    if (i < argc - 1) {
		optarg = argv[++i];
	    } else {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    dtmp = (double)atof(optarg);
	    if (dtmp < 0.0) {
		fprintf(stderr, "%s: warm-up time must be >= 0.0\n",
			flag->program);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    flag->T_flag = dtmp;
	    break;
	case 'D':
	    if (i < argc - 1) {
		optarg = argv[++i];
	    } else {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    dtmp = (double)atof(optarg);
	    if (dtmp < 0.0) {
		fprintf(stderr, "%s: delay time must be >= 0.0\n",
			flag->program);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    flag->D_flag = dtmp;
	    break;
	case 'V':
	    if (i < argc - 1) {
		optarg = argv[++i];
	    } else {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    tmp = (int)strtol(optarg, NULL, 0);
	    if (tmp < 0) {
		fprintf(stderr, "%s: video channel must be >= 0\n",
			flag->program);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    cam->channel = tmp;
	    break;
	case 'M':
	    flag->M_flag = 0;
	    break;
	case 'L':
	    tmp = ov511_LavaRnd(u_cam_p, model);
	    if (tmp < 0) {
		fprintf(stderr, "%s: LavaRnd mode set failed\n",
			flag->program);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    break;
	case 'A':
	    flag->A_flag = 1;
	    break;
	case 'X':
	    if (i < argc - 1) {
		optarg = argv[++i];
	    } else {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    tmp = (int)strtol(optarg, NULL, 0);
	    if (tmp < 0 || tmp > OCTET_CNT) {
		fprintf(stderr, "%s: top_x must be >= 0 and <= %d\n",
			flag->program, OCTET_CNT);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    cam->tmp_top_x = tmp;
	    break;
	case 'x':
	    if (i < argc - 1) {
		optarg = argv[++i];
	    } else {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    dtmp = (double)atof(optarg);
	    if (dtmp < 0.0 || dtmp > 1.0) {
		fprintf(stderr, "%s: min_fract must be >= 0.0 and <= 1.0\n",
			flag->program);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    cam->tmp_min_fract = dtmp;
	    break;
	case '2':
	    if (i < argc - 1) {
		optarg = argv[++i];
	    } else {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    tmp = (int)strtol(optarg, NULL, 0);
	    if (tmp < 0 || tmp > OCTET_CNT / 2) {
		fprintf(stderr, "%s: half_x must be >= 0 and <= %d\n",
			flag->program, OCTET_CNT / 2);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    cam->tmp_half_x = tmp;
	    break;
	case 'd':
	    if (i < argc - 1) {
		optarg = argv[++i];
	    } else {
		fprintf(stderr,
			"%s: ov511_argv[%d] -%c missing next argument\n",
			argv[0], i, c);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    dtmp = (double)atof(optarg);
	    if (dtmp < 0.0 || dtmp > 0.5) {
		fprintf(stderr, "%s: diff_fract must be >= 0.0 and <= 0.5\n",
			flag->program);
		DBG(LAVACAM_ERR_ARG);
		return LAVACAM_ERR_ARG;
	    }
	    cam->tmp_diff_fract = dtmp;
	    break;
	default:
	    fprintf(stderr,
		    "%s: ov511_argv[%d] -%c is unknown\n", argv[0], i, c);
	    DBG(LAVACAM_ERR_ARG);
	    return LAVACAM_ERR_ARG;
	}
    }

    /*
     * parse an optimal savefile/interval argument pair
     */
    if (argv[i] != NULL && argv[i + 1] != NULL) {

	/* save the savefile argument */
	flag->savefile = strdup(argv[i]);
	if (flag->savefile == NULL) {
	    DBG(LAVAERR_MALLOC);
	    return LAVAERR_MALLOC;
	}

	/* form the savefile.new name */
	flag->newfile =
	  (char *)malloc(strlen(flag->savefile) + sizeof(".new"));
	if (flag->newfile == NULL) {
	    free(flag->savefile);
	    flag->savefile = NULL;
	    DBG(LAVAERR_MALLOC);
	    return LAVAERR_MALLOC;
	}
	snprintf(flag->newfile, strlen(flag->savefile) + sizeof(".new"),
		 "%s.new", flag->savefile);

	/* note interval */
	flag->interval = (double)atof(argv[i + 1]);
	if (flag->interval <= 0.0) {
	    fprintf(stderr, "%s: interval: %.3f must be >0.0\n",
		    flag->program, flag->interval);
	    free(flag->savefile);
	    flag->savefile = NULL;
	    free(flag->newfile);
	    flag->newfile = NULL;
	    DBG(LAVACAM_ERR_ARG);
	    return LAVACAM_ERR_ARG;
	}

	/* skip over these two args */
	i += 2;
    }

    /*
     * return arg adjustment value
     */
    return i - 1;
}


/*
 * ov511_open - open a camera, verify its type, return opening camera state
 *
 * given:
 *      devname     camera device file
 *      model       camera model number
 *      o_cam_p     where to place the open camera state
 *      n_cam_p     is non-NULL, set this new state and channel number
 *                      updated with the final open state if non-NULL
 *      siz         pointer to operation size and buffer structure to fill in
 *      def         0 ==> restore saved user settings, 1 ==> keep cur setting
 *      flag        flags set via lavacam_argv()
 *
 * returns:
 *      >=0 ==> camera file descriptor, <0 ==> error
 *
 * NOTE: The video channel number will be placed into cam->channel.
 *       The siz and buffer structure will be setup on successful return.
 *       The n_cam_p state, if it was non-NULL, will be updated as well.
 *
 * NOTE: The function will also transfer tmp sanity check parameters from
 *       the union lavacam to the opsize structure via the ov511_opsize() call.
 */
int
ov511_open(char *devname, int model, union lavacam *o_cam_p,
	   union lavacam *n_cam_p, struct opsize *siz, int def,
	   struct lavacam_flag *flag)
{
    struct ov511_state *cam;	/* o_cam_p as ov511 union element */
    union lavacam t_cam;	/* temp new camera state */
    struct ov511_state *n_cam;	/* n_cam_p as ov511 union element */
    struct video_channel chan;	/* current video channel */
    double open_warmup;		/* if >0.0, seconds to warmup camera */
    int model_indx;		/* default state model index */
    int cam_fd;			/* open device file descriptor */
    int ret;			/* return from fetch of camera state */
    int i;

    /*
     * firewall
     */
    if (devname == NULL || o_cam_p == NULL || siz == NULL) {
	DBG(LAVACAM_ERR_ARG);
	return LAVACAM_ERR_ARG;
    }

    /*
     * pick out the driver specific union element
     */
    cam = &(o_cam_p->ov511);
    if (n_cam_p == NULL) {
	n_cam = NULL;
    } else {
	n_cam = &(n_cam_p->ov511);
    }

    /*
     * look for default state index
     */
    model_indx = ov511_model(model);
    if (model_indx < 0) {
	return model_indx;
    }

    /*
     * attempt to open the device
     */
    cam_fd = open(devname, O_RDONLY);
    if (cam_fd < 0) {
	/* unable to open camera */
	if (errno == EACCES) {
	    /* permission denied */
	    DBG(LAVAERR_PERMOPEN);
	    return LAVAERR_PERMOPEN;
	} else {
	    /* other open failure */
	    DBG(LAVACAM_ERR_OPEN);
	    return LAVACAM_ERR_OPEN;
	}
    }

    /*
     * get/set video channel
     */
    chan.channel = (n_cam == NULL ? 0 : n_cam->channel);
    if (ioctl(cam_fd, VIDIOCGCHAN, &chan) == 0) {
	if (ioctl(cam_fd, VIDIOCSCHAN, &chan) != 0) {
	    DBG(LAVACAM_ERR_CHAN);
	    return LAVACAM_ERR_CHAN;
	}
    } else {
	DBG(LAVACAM_ERR_CHAN);
	return LAVACAM_ERR_CHAN;
    }

    /*
     * initialize cam state
     */
    memset(cam, 0, sizeof(cam[0]));

    /*
     * fetch the camera state
     */
    ret = ov511_get(cam_fd, o_cam_p);
    if (ret < 0) {
	/* perhaps device is not a Phillis camera */
	return ret;
    }
    cam->channel = (n_cam == NULL ? 0 : n_cam->channel);
    /* set the default tmp sanity check values */
    if (n_cam == NULL) {
	cam->tmp_top_x = lava_state[model_indx].def_top_x;
	cam->tmp_min_fract = lava_state[model_indx].def_min_fract;
	cam->tmp_half_x = lava_state[model_indx].def_half_x;
	cam->tmp_diff_fract = lava_state[model_indx].def_diff_fract;
    } else {
	cam->tmp_top_x = n_cam->tmp_top_x;
	cam->tmp_min_fract = n_cam->tmp_min_fract;
	cam->tmp_half_x = n_cam->tmp_half_x;
	cam->tmp_diff_fract = n_cam->tmp_diff_fract;
    }

    /*
     * set the new camera state, if one was supplied, then re-fetch it
     */
    if (n_cam != NULL) {

	/* set the new state */
	ret = ov511_set(cam_fd, n_cam_p);
	if (ret < 0) {
	    /* failed to load new camera settings */
	    return ret;
	}
	/* re-fetch the camera state */
	ret = ov511_get(cam_fd, n_cam_p);
	if (ret < 0) {
	    /* perhaps device is not a Phillis camera */
	    return ret;
	}
	n_cam->tmp_top_x = cam->tmp_top_x;
	n_cam->tmp_min_fract = cam->tmp_min_fract;
	n_cam->tmp_half_x = cam->tmp_half_x;
	n_cam->tmp_diff_fract = cam->tmp_diff_fract;
	/* copy new camera state into our working state */
	memcpy((void *)&t_cam, (void *)n_cam_p, sizeof(t_cam));

	/*
	 * no new state, just work with the opening state
	 */
    } else {
	memcpy((void *)&t_cam, (void *)o_cam_p, sizeof(t_cam));
    }

    /*
     * set OV511_DEF_PALETTE palette if no palette has been set
     */
    ret = valid_palette(PALLETTE_VIDEO4LINUX, cam->palette);
    if (ret < 0) {
	struct video_picture vpic;	/* video picture status */

    	/*
	 * palette is not set, try to set the OV511_DEF_PALETTE palette
	 */
    	memset(&vpic, 0, sizeof(vpic));
	if (ioctl(cam_fd, VIDIOCGPICT, &vpic) < 0) {
	    DBG(LAVACAM_ERR_GETPARAM);
	    return LAVACAM_ERR_GETPARAM;
	}
	vpic.palette = OV511_DEF_PALETTE;
	if (ioctl(cam_fd, VIDIOCSPICT, &vpic) < 0) {
	    DBG(LAVACAM_ERR_SETPARAM);
	    return LAVACAM_ERR_SETPARAM;
	}
	cam->palette = vpic.palette;

	/*
	 * see if we can locate chaotic data within a frame now
	 */
        ret = valid_palette(PALLETTE_VIDEO4LINUX, cam->palette);
	if (ret < 0) {
	    /* could not set a vaild palette */
	    DBG(ret);
	    return ret;
	}
    }

    /*
     * determine camera recommended read/mmap size
     */
    i = ov511_opsize(cam_fd, &t_cam, siz, flag);
    if (i < 0) {
	/* opsize failed, return error */
	return i;
    }

    /*
     * determine if we will read or mmap frames
     */
    if (siz->readsize <= 0 && flag->M_flag) {
	return LAVACAM_ERR_NOREAD;
    } else if (siz->mmapsize <= 0 && !flag->M_flag) {
	return LAVACAM_ERR_NOMMAP;
    }
    siz->use_read = !flag->M_flag;
    if (siz->use_read) {
	siz->image_len = siz->readsize;
	siz->chaos_len = siz->read_lavalen;
    } else {
	siz->image_len = siz->mmapsize;
	siz->chaos_len = siz->mmap_lavalen;
    }

    /*
     * setup to read or mmap
     */
    if (siz->use_read) {

	/*
	 * read setup
	 */
	if (siz->image_len <= 0) {
	    DBG(LAVACAM_ERR_NOREAD);
	    return LAVACAM_ERR_NOREAD;
	}
	siz->image = malloc(siz->image_len);
	if (siz->image == NULL) {
	    DBG(LAVACAM_ERR_NOREAD);
	    return LAVACAM_ERR_NOREAD;
	}
	siz->chaos = siz->image + siz->read_lavaoff;
	siz->chaos_len = siz->read_lavalen;

    } else {

	/*
	 * mmap setup
	 */
	if (siz->image_len <= 0) {
	    DBG(LAVACAM_ERR_NOMMAP);
	    return LAVACAM_ERR_NOMMAP;
	}
	siz->image = ov511_mmap(cam_fd, &t_cam, siz);
	if (siz->image == NULL) {
	    DBG(LAVACAM_ERR_NOMMAP);
	    return LAVACAM_ERR_NOMMAP;
	}
	siz->chaos = siz->image + siz->mmap_lavaoff;
	siz->chaos_len = siz->mmap_lavalen;
    }

    /*
     * allocate and zero previous chaos frame buffer
     */
    siz->prev_frame = calloc(1, siz->chaos_len);
    if (siz->prev_frame == NULL) {
	DBG(LAVAERR_MALLOC);
	return LAVAERR_MALLOC;
    }

    /*
     * determine the camera warmup time, if any
     */
    if (flag->T_flag > 0.0) {

	/* use the -T flag instead of model default warmup */
	open_warmup = flag->T_flag;

    } else {

	/* use model default warmup time due to lack of -T flag */
	open_warmup = lava_state[model_indx].warmup;
    }

    /*
     * warm up camera by tossing frames for a period of time if requested
     */
    if (open_warmup > 0.0) {
	double now;	/* current time as a double */
	double end;	/* end of warmup time */
    	double warmup_delay;	/* sleep between frames */

	/*
	 * setup timing loop
	 */
	now = right_now();
	if (now < 0.0) {
	    DBG(LAVAERR_GETTIME);
	    return LAVAERR_GETTIME;
	}
	end = now + open_warmup;
	warmup_delay = (double)open_warmup / 1000000.0;

	/*
	 * read until warm up time is over
	 */
	while (now < end) {

	    /*
	     * wait for the next frame
	     */
	    if (ov511_wait_frame(cam_fd, end - now) < 0) {
		if (siz->use_read && siz->image != NULL) {
		    free(siz->image);
		} else if (!siz->use_read && siz->image != NULL &&
			   siz->image_len > 0) {
		    (void)munmap(siz->image, siz->image_len);
		}
		siz->image = NULL;
		siz->image_len = 0;
		DBG(LAVACAM_ERR_WARMUP);
		return LAVACAM_ERR_WARMUP;
	    }

	    /*
	     * read or mmap the next frame and toss it
	     *
	     * NOTE: This call is not really needed if mmaping
	     */
	    if (siz->use_read && ov511_get_frame(cam_fd, siz) < 0) {
		if (siz->use_read && siz->image != NULL) {
		    free(siz->image);
		} else if (!siz->use_read && siz->image != NULL &&
			   siz->image_len > 0) {
		    (void)munmap(siz->image, siz->image_len);
		}
		siz->image = NULL;
		siz->image_len = 0;
		DBG(LAVACAM_ERR_WARMUP);
		return LAVACAM_ERR_WARMUP;
	    }

	    /*
	     * release the frame we obtained (only needed if mmapping)
	     */
	    if (!siz->use_read && ov511_msync(cam_fd, &t_cam, siz) < 0) {
		DBG(LAVACAM_ERR_WARMUP);
		return LAVACAM_ERR_WARMUP;
	    }

	    /*
	     * determine the new now
	     */
	    now = right_now();
	    if (now < 0.0) {
		if (siz->use_read && siz->image != NULL) {
		    free(siz->image);
		} else if (!siz->use_read && siz->image != NULL &&
			   siz->image_len > 0) {
		    (void)munmap(siz->image, siz->image_len);
		}
		siz->image = NULL;
		siz->image_len = 0;
		DBG(LAVAERR_GETTIME);
		return LAVAERR_GETTIME;
	    }

	    /*
	     * sleep a little bit before next frame, if needed
	     */
	    if (end > now) {
	    	usleep((int)(1000000.0 *
		    ((end-now) > warmup_delay) ? warmup_delay : (end-now)));
	    }
	}
    }

    /*
     * return open file descriptor
     */
    return cam_fd;
}


/*
 * ov511_close - close an open camera
 *
 * given:
 *      cam_fd   open camera descriptor
 *      siz      pointer to operation size and buffer structure
 *      flag        flags set via lavacam_argv()
 *
 * returns:
 *      0 ==> OK, <0 ==> error
 */
int
ov511_close(int cam_fd, struct opsize *siz, struct lavacam_flag *flag)
{
    int ret;	/* close return value */

    /*
     * firewall
     */
    if (cam_fd < 0 || siz == NULL || flag == NULL) {
	DBG(LAVACAM_ERR_ARG);
	return LAVACAM_ERR_ARG;
    }

    /*
     * free read buffer if reading
     */
    if (siz->use_read) {
	if (siz->image != NULL) {
	    (void)free(siz->image);
	}

	/*
	 * or munmap if mmapped
	 */
    } else {
	if (siz->image != NULL && siz->image_len > 0) {
	    (void)munmap(siz->image, siz->image_len);
	}
    }

    /*
     * free previous chaos frame
     */
    if (siz->prev_frame != NULL) {
	(void)free(siz->prev_frame);
	siz->prev_frame = NULL;
    }

    /*
     * clear read/mmap frame data
     */
    siz->image = NULL;
    siz->image_len = 0;
    siz->chaos = NULL;
    siz->chaos_len = 0;

    /*
     * free savefile/newfile is they are present
     */
    if (flag->savefile != NULL) {
	free(flag->savefile);
	flag->savefile = NULL;
    }
    if (flag->newfile != NULL) {
	free(flag->newfile);
	flag->newfile = NULL;
    }
    flag->interval = 0.0;

    /*
     * close device
     */
    ret = close(cam_fd);
    if (ret < 0) {
	DBG(LAVACAM_ERR_CLOSE);
	return LAVACAM_ERR_CLOSE;
    }
    return LAVACAM_ERR_OK;
}


/*
 * ov511_get_frame - get the next frame from the camera
 *
 * given:
 *      cam_fd   open camera descriptor
 *      siz      pointer to operation size and buffer structure
 *
 * returns:
 *      >= 0 ==> chaos octets obtained, < 0 ==> error
 *
 * NOTE: One must call ov511_msync(), if mmapping, prior to calling
 *       this function.  To be on the safe side, always call ov511_msync()
 *       prior to calling this function.
 *
 * NOTE: To not block, call ov511_wait_frame() (or read select on the
 *       open file descriptor) before calling this function.
 *
 * NOTE: To be honest, this function does nothing useful when mmaping.
 *       One can access the mmapped data (via siz->image or siz->chaos)
 *       without calling this function if ov511_msync() and ov511_wait_frame()
 *       are called before accessing the data.  However, there is no
 *       harm in calling this function if mmapping.
 */
int
ov511_get_frame(int cam_fd, struct opsize *siz)
{
    int op_ret = 0;	/* read or mmap count */

    /*
     * firewall
     */
    if (cam_fd < 0 || siz == NULL) {
	DBG(LAVACAM_ERR_ARG);
	return LAVACAM_ERR_ARG;
    }

    /*
     * read if reading
     */
    if (siz->use_read) {

	/*
	 * read the data
	 */
	if (siz->image == NULL || siz->image_len <= 0) {
	    DBG(LAVACAM_ERR_NOSIZE);
	    return LAVACAM_ERR_NOSIZE;
	}
	op_ret = read(cam_fd, siz->image, siz->image_len);
	if (op_ret < 0) {
	    DBG(LAVACAM_ERR_IOERR);
	    return LAVACAM_ERR_IOERR;
	}
	if (op_ret != siz->image_len) {
	    DBG(LAVACAM_ERR_FRAME);
	    return LAVACAM_ERR_FRAME;
	}
    }

    /*
     * report the amount of chaotic data returned
     */
    return siz->chaos_len;
}


/*
 * ov511_msync - release/sync after processing a mmap captured buffer
 *
 * given:
 *      cam_fd      open camera descriptor
 *      u_cam_p     pointer to camera state
 *      siz         pointer to operation size and buffer structure
 *
 * returns:
 *      0 ==> OK, <0 ==> error
 *
 * NOTE: If we a reading, we immediately return OK (0) after doing nothing.
 */
int
ov511_msync(int cam_fd, union lavacam *u_cam_p, struct opsize *siz)
{
    struct ov511_state *cam;	/* u_cam_p as ov511 union element */
    struct video_mmap vm;	/* video buffer description */
    int framenum;		/* which frame we are releasing */

    /*
     * firewall
     */
    if (cam_fd < 0 || u_cam_p == NULL || siz == NULL) {
	DBG(LAVACAM_ERR_ARG);
	return LAVACAM_ERR_ARG;
    }
    if (siz->use_read == TRUE) {
	/* reading, nothing to do */
	return LAVACAM_ERR_OK;
    }

    /*
     * pick out the driver specific union element
     */
    cam = &(u_cam_p->ov511);

    /*
     * firewall on camera state
     */
    if (cam->frames < 1 || cam->palette < 0 ||
    	cam->height <= 0 || cam->width <= 0) {
	DBG(LAVACAM_ERR_ARG);
	return LAVACAM_ERR_ARG;
    }

    /*
     * release
     */
    framenum = cam->frames - 1;
    if (ioctl(cam_fd, VIDIOCSYNC, &framenum) < 0) {
	DBG(LAVACAM_ERR_SYNC);
	return LAVACAM_ERR_SYNC;
    }

    /*
     * set mmap image size for the next frame
     */
    vm.frame = framenum;
    vm.format = cam->palette;
    vm.height = cam->height;
    vm.width = cam->width;
    if (ioctl(cam_fd, VIDIOCMCAPTURE, &vm) < 0) {
	DBG(LAVACAM_ERR_SYNC);
	return LAVACAM_ERR_SYNC;
    }
    return LAVACAM_ERR_OK;
}


/*
 * ov511_wait_frame - use select to wait for the next camera frame
 *
 * given:
 *      cam_fd      open camera descriptor
 *      max_wait    wait for up to this many seconds, <0.0 ==> infinite
 *
 * returns:
 *      1 ==> frame is ready, 0 ==> timeout, <0 ==> error
 */
int
ov511_wait_frame(int cam_fd, double max_wait)
{
    fd_set rd_set;		/* select mask for reading */
    struct timeval sel_wait;	/* select time, if >= 0 */
    int ret;			/* select return value */
    double start = 0.0;		/* pre-select time */
    double end;			/* post-select time */
    double delay;		/* time spent in select() or pselect() */

    /*
     * firewall
     */
    /* NOTE: timeout may be NULL */
    if (cam_fd < 0) {
	DBG(LAVACAM_ERR_ARG);
	return LAVACAM_ERR_ARG;
    }

    /*
     * common select setup
     */
    FD_ZERO(&rd_set);
    FD_SET(cam_fd, &rd_set);
    if (max_wait >= 0.0) {
	sel_wait.tv_sec = double_to_tv_sec(max_wait);
	sel_wait.tv_usec = double_to_tv_usec(max_wait);
    }
    /* keep track of time selected in case of signals */
    if (max_wait >= 0.0) {
	start = right_now();
	if (start < 0.0) {
	    DBG(LAVAERR_GETTIME);
	    return LAVAERR_GETTIME;
	}
    }

    /*
     * wait for the next frame or until we wait too long (if max_wait)
     */
    do {
	/* clear system error code */
	errno = 0;

	/*
	 * select
	 */
#if defined(HAVE_PSELECT)
	if (max_wait >= 0.0) {
	    ret = pselect(cam_fd + 1, &rd_set, NULL, NULL, &sel_wait, NULL);
	} else {
	    ret = pselect(cam_fd + 1, &rd_set, NULL, NULL, NULL, NULL);
	}
#else /* HAVE_PSELECT */
	if (max_wait >= 0.0) {
	    ret = select(cam_fd + 1, &rd_set, NULL, NULL, &sel_wait);
	} else {
	    ret = select(cam_fd + 1, &rd_set, NULL, NULL, NULL);
	}
#endif /* HAVE_PSELECT */

	/*
	 * adjust select time based on how long we just waited
	 */
	if (ret < 0 && errno == EINTR && max_wait >= 0.0) {
	    end = right_now();
	    if (end < 0.0) {
		DBG(LAVAERR_GETTIME);
		return LAVAERR_GETTIME;
	    }
	    delay = end - start;
	    if (delay >= max_wait) {
		break;
	    }
	    sel_wait.tv_sec = double_to_tv_sec(max_wait - delay);
	    sel_wait.tv_usec = double_to_tv_usec(max_wait - delay);
	}
    } while (ret < 0 && errno == EINTR);

    /*
     * return select status
     */
    return ret;
}


#if defined(OV511_SIMULATION)
/*
 * ov511_ioctl - simulate calling the ov511 driver
 */
static int
ov511_ioctl(int fd, int request, void *arg)
{
    char *name;		/* name of IOCTL request */

    /*
     * print args
     */
    switch (request) {
    case VIDIOCGCAP: name = "VIDIOCGCAP"; break;
    case VIDIOCGCHAN: name = "VIDIOCGCHAN"; break;
    case VIDIOCGMBUF: name = "VIDIOCGMBUF"; break;
    case VIDIOCGPICT: name = "VIDIOCGPICT"; break;
    case VIDIOCGWIN: name = "VIDIOCGWIN"; break;
    case VIDIOCMCAPTURE: name = "VIDIOCMCAPTURE"; break;
    case VIDIOCSCHAN: name = "VIDIOCSCHAN"; break;
    case VIDIOCSPICT: name = "VIDIOCSPICT"; break;
    case VIDIOCSWIN: name = "VIDIOCSWIN"; break;
    case VIDIOCSYNC: name = "VIDIOCSYNC"; break;
    case VIDIOCGFBUF: name = "VIDIOCGFBUF"; break;
    case VIDIOCGUNIT: name = "VIDIOCGUNIT"; break;
    case OV511IOC_GINTVER: name = "OV511IOC_GINTVER"; break;
    case OV511IOC_GUSHORT: name = "OV511IOC_GUSHORT"; break;
    case OV511IOC_SUSHORT: name = "OV511IOC_SUSHORT"; break;
    case OV511IOC_GUINT: name = "OV511IOC_GUINT"; break;
    case OV511IOC_SUINT: name = "OV511IOC_SUINT"; break;
    case OV511IOC_WI2C: name = "OV511IOC_WI2C"; break;
    case OV511IOC_RI2C: name = "OV511IOC_RI2C"; break;
    default: name = "((unknown))"; break;
    }
    fprintf(stderr, "ioctl(%d, %s, %p)\n", fd, name, arg);

#undef ioctl
    /*
     * call the real ioctl - XXX replace with stub
     */
    return ioctl(fd, request, arg);
}
#endif /* OV511_SIMULATION */
