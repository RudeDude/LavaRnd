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

MODULE = LavaRnd::Return		PACKAGE = LavaRnd::Return


SV *
random8()

    PROTOTYPE:

    PREINIT:
	U8 randval;	/* random value */

    CODE:
	randval = (U8)random8();
	if (lastop_errno < 0) {
	    XSRETURN_UNDEF;
	}
	RETVAL = newSViv(randval);
	SvIOK_only(RETVAL);

    OUTPUT:
    	RETVAL


SV *
random16()

    PROTOTYPE:

    PREINIT:
	U16 randval;	/* random value */

    CODE:
	randval = (U16)random16();
	if (lastop_errno < 0) {
	    XSRETURN_UNDEF;
	}
	RETVAL = newSViv(randval);
	SvIOK_only(RETVAL);

    OUTPUT:
    	RETVAL


SV *
random32()

    PROTOTYPE:

    PREINIT:
	U32 randval;	/* random value */

    CODE:
	randval = (U32)random32();
	if (lastop_errno < 0) {
	    XSRETURN_UNDEF;
	}
	/*
	 * We must cast to double because IV's are signed
	 * 32 bit values and random32() returns (and we want)
	 * unsigned 32 bit values.  Perl lacks an unsigned 32
	 * bit (or 64 bit) internal data type.  *sigh*
	 */
	RETVAL = newSVnv((double)randval);
	SvNOK_on(RETVAL);

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
#endif
	    XSRETURN_UNDEF;
	}

	/* allocate the random scalar */
	ret = newSV(len);

	/* find where the actual memory block resides */
	buf = SvPVX(ret);
	if (buf == NULL) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr, "DEBUG: buf == NULL\n");
#endif
	    XSRETURN_UNDEF;
	}

	/* make the scalar a valid memory block */
	SvPOK_on(ret);
	SvCUR_set(ret, len);

	/* load the memory block with random data */
	retlen = randomcpy(buf, len);
	if (retlen != len) {
#if defined(EXTRA_DEBUG)
	    fprintf(stderr, "DEBUG: retlen:%d != len:%d\n", retlen, len);
#endif
	    XSRETURN_UNDEF;
	}

	/* return the scalar */
	RETVAL = ret;

    OUTPUT:
    	RETVAL

    CLEANUP:
	if (lastop_errno < 0) {
	    XSRETURN_UNDEF;
	}


SV *
drandom()

    PROTOTYPE:

    PREINIT:
	double randval;	/* random value */

    CODE:
	randval = (double)drandom();
	if (lastop_errno < 0) {
	    XSRETURN_UNDEF;
	}
	RETVAL = newSVnv(randval);
	SvNOK_on(RETVAL);

    OUTPUT:
    	RETVAL


SV *
dcrandom()

    PROTOTYPE:

    PREINIT:
	double randval;	/* random value */

    CODE:
	randval = (double)dcrandom();
	if (lastop_errno < 0) {
	    XSRETURN_UNDEF;
	}
	RETVAL = newSVnv(randval);
	SvNOK_on(RETVAL);

    OUTPUT:
    	RETVAL


SV *
random_val(beyond)
	U32 beyond

    PROTOTYPE: $

    PREINIT:
	U32 randval;	/* random value */

    CODE:
	randval = (U32)random_val(beyond);
	if (lastop_errno < 0) {
	    XSRETURN_UNDEF;
	}
	/*
	 * We must cast to double because IV's are signed
	 * 32 bit values and random32() returns (and we want)
	 * unsigned 32 bit values.  Perl lacks an unsigned 32
	 * bit (or 64 bit) internal data type.  *sigh*
	 */
	RETVAL = newSVnv((double)randval);
	SvNOK_on(RETVAL);

    OUTPUT:
    	RETVAL


SV *
random_coin()

    PROTOTYPE:

    PREINIT:
	U8 randval;	/* random value */

    CODE:
	randval = (U8)random8();
	if (lastop_errno < 0) {
	    XSRETURN_UNDEF;
	}
	if (random8() & 0x08) {
	    RETVAL = &PL_sv_yes;
	} else {
	    RETVAL = &PL_sv_no;
	}

    OUTPUT:
    	RETVAL


SV *
lavarnd_errno(...)

    PROTOTYPE: ;$

    CODE:
	switch (items) {
	case 0:
	    RETVAL = newSViv(lavarnd_errno);
	    SvIOK_only(RETVAL);
	    break;
	case 1:
	    if (!SvOK(ST(0))) {
		XSRETURN_UNDEF;
	    }
	    RETVAL = newSViv(lavarnd_errno);
	    SvIOK_only(RETVAL);
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
