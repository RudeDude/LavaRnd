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

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include <sys/types.h>
#include "LavaRnd/random.h"
#include "LavaRnd/lavarnd.h"
#include "LavaRnd/lavaerr.h"
#include "LavaRnd/sysstuff.h"


MODULE = LavaRnd::Util		PACKAGE = LavaRnd::Util

int
lavarnd_len(inlen, rate)
    	int inlen;
	double rate;

    PROTOTYPE: $$

    CODE:
    	RETVAL = lavarnd_len(inlen, rate);

    OUTPUT:
    	RETVAL


SV *
lava_turn(...)

    PROTOTYPE: $$

    PREINIT:
    	SV *arg;		/* input arg */
	U8 *input;		/* input buffer */
	int inlen;		/* input (1st) arg length */
	int len;		/* length of 'input' and 'output' */
	int nway;		/* number of buffers to turn into */
	SV *result;		/* lavarnd result */
	U8 *output;		/* pointer to lavarnd result buffer */
	int outlen;		/* allocated length of ret/output */
	U8 *ret;		/* lava_blk_turn() return value */

    CODE:
	/*
	 * setup
	 */
	lastop_errno = LAVAERR_OK;

	/*
	 * parse args
	 */
	/* $turned = lava_blk_turn($input, $nway); */
	if (items != 2) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
		    "DEBUG: lava_turn: invalid number of args: %d\n", items);
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}
	if (!SvOK(ST(1))) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
		    "DEBUG: lava_turn: 2nd arg is an invalid value\n");
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	} else {
	    nway = SvIV(ST(1));
	}
	if (!SvOK(ST(0))) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_turn: 1st arg is not SvOK\n");
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}
	arg = ST(0);
	inlen = SvCUR(arg);
	input = SvPVX(arg);
	if (inlen < 1) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_turn: invalid input length: %d\n", inlen);
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}
	if (nway < 1) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_turn: invalid nway: %d\n", nway);
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}

	/*
	 * allocate the output buffer
	 */
	outlen = lavarnd_turn_len(inlen, nway);
#if defined(EXTRA_DEBUG)
	fprintf(stderr,"DEBUG: lava_turn: inlen: %d  nway: %d  outlen: %d\n",
			inlen, nway, outlen);
#endif
	if (outlen < 1) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr, "DEBUG: lava_turn: bogus outlen: %d\n", outlen);
#endif
	}
	result = newSV(outlen);
	output = SvPVX(result);
	if (output == NULL) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr, "DEBUG: lava_turn: output == NULL\n");
#endif
	    lastop_errno = LAVAERR_MALLOC;
	    XSRETURN_UNDEF;
	}

	/*
	 * make the scalar a valid memory block
	 */
	SvPOK_on(result);
	SvCUR_set(result, outlen);

	/*
	 * turn
	 */
	ret = lava_turn(input, inlen, nway, output);
	if (ret == NULL) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr, "DEBUG: lavarnd error code: %d\n", ret);
#endif
	    lastop_errno = LAVAERR_IMPOSSIBLE;
	    XSRETURN_UNDEF;
	}

	/*
	 * return the scalar
	 */
	RETVAL = result;

    OUTPUT:
	RETVAL


SV *
lava_blk_turn(...)

    PROTOTYPE: $$

    PREINIT:
    	SV *arg;		/* input arg */
	U8 *input;		/* input buffer */
	int inlen;		/* input (1st) arg length */
	int len;		/* length of 'input' and 'output' */
	int nway;		/* number of buffers to turn into */
	SV *result;		/* lavarnd result */
	U8 *output;		/* pointer to lavarnd result buffer */
	int outlen;		/* allocated length of ret/output */
	U8 *ret;		/* lava_blk_turn() return value */

    CODE:
	/*
	 * setup
	 */
	lastop_errno = LAVAERR_OK;

	/*
	 * parse args
	 */
	/* $turned = lava_blk_turn($input, $nway); */
	if (items != 2) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_blk_turn: invalid number of args: %d\n",
		    items);
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}
	if (!SvOK(ST(1))) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_blk_turn: 2nd arg is an invalid value\n");
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	} else {
	    nway = SvIV(ST(1));
	}
	if (!SvOK(ST(0))) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_blk_turn: 1st arg is not SvOK\n");
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}
	arg = ST(0);
	inlen = SvCUR(arg);
	input = SvPVX(arg);
	if (inlen < 1) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_blk_turn: invalid input length: %d\n", inlen);
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}
	if (nway < 1) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_blk_turn: invalid nway: %d\n", nway);
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}

	/*
	 * allocate the output buffer
	 */
	outlen = lavarnd_blk_turn_len(inlen, nway);
#if defined(EXTRA_DEBUG)
	fprintf(stderr, "DEBUG: lava_blk_turn: inlen: %d  nway: %d  "
			"outlen: %d\n",
			inlen, nway, outlen);
#endif
	if (outlen < 1) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_blk_turn: bogus outlen: %d\n", outlen);
#endif
	}
	result = newSV(outlen);
	output = SvPVX(result);
	if (output == NULL) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr, "DEBUG: lava_blk_turn: output == NULL\n");
#endif
	    lastop_errno = LAVAERR_MALLOC;
	    XSRETURN_UNDEF;
	}

	/*
	 * make the scalar a valid memory block
	 */
	SvPOK_on(result);
	SvCUR_set(result, outlen);

	/*
	 * turn
	 */
	ret = lava_blk_turn(input, inlen, nway, output);
	if (ret == NULL) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr, "DEBUG: lavarnd error code: %d\n", ret);
#endif
	    lastop_errno = LAVAERR_IMPOSSIBLE;
	    XSRETURN_UNDEF;
	}

	/*
	 * return the scalar
	 */
	RETVAL = result;

    OUTPUT:
	RETVAL


SV *
lava_salt_blk_turn(...)

    PROTOTYPE: $$$

    PREINIT:
    	SV *sarg;		/* salt arg */
	U8 *salt;		/* salt */
	int saltlen;		/* salt (1st) arg length */
    	SV *arg;		/* input arg */
	U8 *input;		/* input buffer */
	int inlen;		/* input (2nd) arg length */
	int len;		/* length of 'input' and 'output' */
	int nway;		/* number of buffers to turn into */
	SV *result;		/* lavarnd result */
	U8 *output;		/* pointer to lavarnd result buffer */
	int outlen;		/* allocated length of ret/output */
	U8 *ret;		/* lava_salt_blk_turn() return value */

    CODE:
	/*
	 * setup
	 */
	lastop_errno = LAVAERR_OK;

	/*
	 * parse args
	 */
	/* $turned = lava_salt_blk_turn($input, $nway); */
	if (items != 3) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_salt_blk_turn: invalid number of args: %d\n",
		    items);
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}
	if (!SvOK(ST(2))) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_salt_blk_turn: "
		    "3rd arg is an invalid value\n");
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	} else {
	    nway = SvIV(ST(2));
	}
	if (!SvOK(ST(1))) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_salt_blk_turn: 2nd arg is not SvOK\n");
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}
	arg = ST(1);
	inlen = SvCUR(arg);
	input = SvPVX(arg);
	if (inlen < 1) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_salt_blk_turn: "
		    "2nd arg invalid input length: %d\n", inlen);
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}
	if (!SvOK(ST(0))) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_salt_blk_turn: 1st arg is not SvOK\n");
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}
	sarg = ST(0);
	saltlen = SvCUR(sarg);
	salt = SvPVX(sarg);
	if (saltlen < 1) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_salt_blk_turn: "
		    "1st arg invalid input length: %d\n", inlen);
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}
	if (nway < 1) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_salt_blk_turn: invalid nway: %d\n", nway);
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}

	/*
	 * allocate the output buffer
	 */
	outlen = lavarnd_salt_blk_turn_len(saltlen, inlen, nway);
#if defined(EXTRA_DEBUG)
	fprintf(stderr, "DEBUG: lava_salt_blk_turn: "
			"inlen: %d  nway: %d  outlen: %d\n",
			inlen, nway, outlen);
#endif
	if (outlen < 1) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_salt_blk_turn: bogus outlen: %d\n", outlen);
#endif
	}
	result = newSV(outlen);
	output = SvPVX(result);
	if (output == NULL) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_salt_blk_turn: output == NULL\n");
#endif
	    lastop_errno = LAVAERR_MALLOC;
	    XSRETURN_UNDEF;
	}

	/*
	 * make the scalar a valid memory block
	 */
	SvPOK_on(result);
	SvCUR_set(result, outlen);

	/*
	 * turn
	 */
	ret = lava_salt_blk_turn(salt, saltlen, input, inlen, nway, output);
	if (ret == NULL) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lava_salt_blk_turn: "
		    "lavarnd error code: %d\n", ret);
#endif
	    lastop_errno = LAVAERR_IMPOSSIBLE;
	    XSRETURN_UNDEF;
	}

	/*
	 * return the scalar
	 */
	RETVAL = result;

    OUTPUT:
	RETVAL


int
lava_nway_value(inlen, rate)
    	int inlen;
	double rate;

    PROTOTYPE: $$

    CODE:
    	RETVAL = lava_nway_value(inlen, rate);

    OUTPUT:
    	RETVAL


SV *
lavarnd(...)

    PROTOTYPE: $$$

    PREINIT:
    	SV *arg;		/* chaotic input arg */
	U8 *input;		/* pointer to chaotic data */
	int inlen;		/* input (1st) arg length */
	double rate = 1.0;	/* increase (>1.0) or decrease (<1.0) output */
	int use_salt = 1;	/* 1 ==> system_stuff salt, 0 ==> no salt */
	SV *result;		/* lavarnd result */
	U8 *output;		/* pointer to lavarnd result buffer */
	int outlen;		/* allocated length of ret/output */
	int ret;		/* lavarnd() return value */

    CODE:
	/*
	 * setup
	 */
	lastop_errno = LAVAERR_OK;

	/*
	 * parse args
	 */
	/* $rnddata = lavarnd($input [, $rate [, $use_salt]]); */
	switch (items) {
	case 3:
	    if (!SvOK(ST(2))) {
#if defined(EXTRA_DEBUG)
		fprintf(stderr,
			"DEBUG: lavarnd: 3rd arg is an invalid value\n");
#endif
		lastop_errno = LAVAERR_BADARG;
	    	XSRETURN_UNDEF;
	    } else {
	    	use_salt = ((SvNV(ST(2)) == 0.0) ? 0 : 1);
	    }
	    /*FALLTHRU*/
	case 2:
	    if (!SvOK(ST(1))) {
#if defined(EXTRA_DEBUG)
		fprintf(stderr,
			"DEBUG: lavarnd: 2nd arg is an invalid value\n");
#endif
		lastop_errno = LAVAERR_BADARG;
	    	XSRETURN_UNDEF;
	    } else {
	    	rate = SvNV(ST(1));
	    }
	    /*FALLTHRU*/
	case 1:
	    if (!SvOK(ST(0))) {
#if defined(EXTRA_DEBUG)
		fprintf(stderr, "DEBUG: lavarnd: 1st arg is not SvOK\n");
#endif
		lastop_errno = LAVAERR_BADARG;
	    	XSRETURN_UNDEF;
	    }
	    arg = ST(0);
	    break;
	default:
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lavarnd: invalid number of args: %d\n", items);
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}
	inlen = SvCUR(arg);
	input = SvPVX(arg);
#if defined(EXTRA_DEBUG)
	fprintf(stderr, "DEBUG: lavarnd: use_salt: %d rate: %f len: %d\n",
			use_salt, rate, inlen);
#endif
	if (inlen < 1) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lavarnd: invalid input length: %d\n", inlen);
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}
	if (rate <= 0.0) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lavarnd: invalid rate: %f\n", rate);
#endif
	    lastop_errno = LAVAERR_BADARG;
	    XSRETURN_UNDEF;
	}
#if defined(EXTRA_DEBUG)
	if (use_salt) {
	    fprintf(stderr, "DEBUG: lavarnd: will use salt\n");
	} else {
	    fprintf(stderr, "DEBUG: lavarnd: will NOT use salt\n");
	}
#endif

	/*
	 * allocate the output buffer
	 */
	outlen = lavarnd_len(inlen, rate);
	if (outlen < 1) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lavarnd: bogus outlen: %d\n", outlen);
#endif
	}
	result = newSV(outlen);
	output = SvPVX(result);
	if (output == NULL) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lavarnd: output == NULL\n");
#endif
	    lastop_errno = LAVAERR_MALLOC;
	    XSRETURN_UNDEF;
	}

	/*
	 * make the scalar a valid memory block
	 */
	SvPOK_on(result);
	SvCUR_set(result, outlen);

	/*
	 * perform the lavarnd process
	 */
	ret = lavarnd(use_salt, input, inlen, rate, output, outlen);
	if (ret < 0) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr,
	    	    "DEBUG: lavarnd: lavarnd error code: %d\n", ret);
#endif
	    lastop_errno = ret;
	    XSRETURN_UNDEF;
	}

	/*
	 * return the scalar
	 */
	RETVAL = result;

    OUTPUT:
	RETVAL


SV *
system_stuff()

    PROTOTYPE:

    PREINIT:
    	SV *result;			/* system_stuff result */
    	struct system_stuff *sdata;	/* system_stuff buffer */

    CODE:
	/* allocate the output buffer */
	result = newSV(sizeof(struct system_stuff));
	sdata = (struct system_stuff *)SvPVX(result);
	if (sdata == NULL) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr, "DEBUG: sdata == NULL\n");
#endif
	    lastop_errno = LAVAERR_MALLOC;
	    XSRETURN_UNDEF;
	}

	/* collect system stuff */
	system_stuff(sdata, 0);

	/*
	 * make the scalar a valid memory block
	 */
	SvPOK_on(result);
	SvCUR_set(result, sizeof(struct system_stuff));

	/*
	 * return the scalar
	 */
	RETVAL = result;

    OUTPUT:
	RETVAL


SV *
lavarnd_errno(...)

    PROTOTYPE: ;$

    PREINIT:
    	SV *arg;

    CODE:
	switch (items) {
	case 0:
	    RETVAL = newSViv(lavarnd_errno);
	    break;
	case 1:
	    if (!SvOK(ST(0))) {
		XSRETURN_UNDEF;
	    }
	    RETVAL = newSViv(lavarnd_errno);
	    lavarnd_errno = SvIVx(ST(0));
	    break;
	default:
	    XSRETURN_UNDEF;
	}

    OUTPUT:
    	RETVAL


SV *
lastop_errno()

    PROTOTYPE:

    CODE:
	RETVAL = newSViv(lastop_errno);
	SvIOK_only(RETVAL);

    OUTPUT:
    	RETVAL
