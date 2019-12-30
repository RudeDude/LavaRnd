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
#include "LavaRnd/random_libc.h"

MODULE = LavaRnd::Retry		PACKAGE = LavaRnd::Retry


U8
random8()

    PROTOTYPE:

    CODE:
	RETVAL = (U8)random8();

    OUTPUT:
    	RETVAL


U16
random16()

    PROTOTYPE:

    CODE:
	RETVAL = (U16)random16();

    OUTPUT:
    	RETVAL


U32
random32()

    PROTOTYPE:

    CODE:
	RETVAL = (U32)random32();

    OUTPUT:
    	RETVAL


SV *
random_data(len)
	int len

    PROTOTYPE: $

    PREINIT:
	long retlen;		/* quant of random data obtained, <0 ==> err */
	U8 *buf;		/* pointer to len octets */
	SV *ret;		/* scalar of random data to return */

    CODE:
	/* firewall - len sanity check */
#if defined(EXTRA_DEBUG)
	fprintf(stderr, "len: %d\n", len);
#endif
    	if (len <= 0) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr, "len <= 0\n");
	    XSRETURN_UNDEF;
#else
	    /* module interface only allows return of random data */
	    /* simulate exit :=) */
	    exit(100);
#endif
	}

	/* allocate the random scalar */
	ret = newSV(len);

	/* find where the actual memory block resides */
	buf = SvPVX(ret);
	if (buf == NULL) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr, "DEBUG: buf == NULL\n");
	    XSRETURN_UNDEF;
#else
	    /* module interface only allows return of random data */
	    /* simulate exit :=) */
	    exit(100);
#endif
	}

	/* make the scalar a valid memory block */
	SvPOK_on(ret);
	SvCUR_set(ret, len);

	/* load the memory block with random data */
	retlen = randomcpy(buf, len);
	if (retlen != len) {
	    /* this should never happen - but just in case it does ... */
#if defined(EXTRA_DEBUG)
	    fprintf(stderr, "DEBUG: retlen:%d != len:%d\n", retlen, len);
	    XSRETURN_UNDEF;
#else
	    /* module interface only allows return of random data */
	    /* simulate exit :=) */
	    exit(100);
#endif
	}

	/* return the scalar */
	RETVAL = ret;

    OUTPUT:
    	RETVAL


double
drandom()

    PROTOTYPE:

    CODE:
	RETVAL = (double)drandom();

    OUTPUT:
    	RETVAL


double
dcrandom()

    PROTOTYPE:

    CODE:
	RETVAL = (double)dcrandom();

    OUTPUT:
    	RETVAL


U32
random_val(beyond)
	U32 beyond

    PROTOTYPE: $

    CODE:
	RETVAL = (U32)random_val(beyond);

    OUTPUT:
    	RETVAL


bool
random_coin()

    PROTOTYPE:

    CODE:
	RETVAL = (bool)((random8() & 0x08) ? 1 : 0);

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
