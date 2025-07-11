/*
 * random - high level random number interface without libc overrides
 *
 * This is a high level interface to random number services.  These
 * functions do not conflict with random number services found in libc.
 * See random_libc.c for libc replacement functions.
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: random.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


#if !defined(__LAVARND_RANDOM_H__)
#  define __LAVARND_RANDOM_H__


#  include "lava_callback.h"
#  include "lavaerr.h"


/*
 * long double, double, and long
 *
 * This is the size of the mantissa in an IEEE floating value (double).
 */
#  define BITS_8 (0xffUL)
#  define BITS_16 (0xffffUL)
#  define BITS_32 (0xffffffffUL)
 /**/
#  define BITS_53 (0x1fffffffffffffULL)
#  define DBITS_53 ((double)BITS_53)
#  define DPOW_53 (DBITS_53+(double)1.0)
   /**/
#  define BITS_64 (0xffffffffffffffffULL)
#  define LDBITS_64 ((long double)BITS_64)
#  define LDPOW_64 (LDBITS_64+(long double)1.0)
  /*
   * default compiled in callback
   *
   * The lava_callback is the default callback that is used by this high level
   * random number interface.  It is typically initialized by one of the
   * libLava_XYZ.c that is linked into a liblava_XYZ.a library.
   *
   * The default compiled in callback may be overridden by calling the
   * set_lava_callback() function below.
   */
extern lavaback lava_callback;


/*
 * lavarnd_errno - most recent LavaRnd error code
 *
 * Whenever LavaRnd is unable to satisfy a request, a value <0 is stored
 * in lavarnd_errno.  The user / calling routine is free to clear this
 * value at any time.
 *
 * The intent of lavarnd_errno is to allow a caller to determine
 * when a random number interface as returned bad data.  For example,
 * the libc emulation interface function random() has no direct means
 * if indicating that it was unable to return data of sufficent quality.
 * A caller can get around this interface limitation by checking the
 * lavarnd_errno value for a negative value.  Whenever lavarnd_errno < 0,
 * the return value from such interfaces should be considered unreliable
 * and should not be used.
 *
 * LavaRnd errors vary depending on which callback interface is used.
 * For example, if LAVACALL_LAVA_RETURN is used when there is failure
 * to contact the lavapool daemon, the underlying funcion (raw_random())
 * will return an error and lavarnd_errno will be set to a value < 0.
 * And if LAVACALL_S100_MED is used and the s100 generator goes too
 * long without a reseed (say, because of inability to contact the lavapool
 * daemon) then lavarnd_errno will be set to a value < 0.
 *
 * The selection of the call back interface is usually predefined
 * by the particular library used.  In the above examples, the
 * LAVACALL_LAVA_RETURN callback is predefined by liblava_return.a.
 * The LAVACALL_S100_MED callback is predefined by liblava_s100_med.a.
 *
 * What can one do when lavarnd_errno becomes < 0?  First, the
 * data from of the previous random number call should not be used.
 * Second, one should clear the lavarnd_errno by setting it to
 * LAVAERR_OK (0).  Third, one could consider either retrying the
 * call (perhaps at a later time) or change the callback to a less
 * stringent level (via the set_lava_callback() function) if the
 * application permits.  For example one could go from
 * LAVACALL_S100_HIGH to LAVACALL_S100_MED.
 *
 * See lava_callback.h for details about callback interfaces.
 */
extern int lavarnd_errno;


/*
 * lastop_errno - error code (or 0) of the most recent random or libc-like call
 *
 * When a random or libc-like call begins, it clears (sets to 0) this
 * lastop_errno value.  If there is an LavaRnd error that sets the
 * lavarnd_errno, then the lastop_errno value will also be set.
 *
 * Language interfaces such as Perl use this value to detect errors
 * and return undefined values.
 *
 * While the user is free to clear this value as well, there is little
 * point in doing so because the next random or libc-like call will
 * clear it.  Unlike lavarnd_errno that records the most recent error,
 * the lastop_errno records the success (0) or failure (<0) of the
 * most recent random or libc-like call.
 */
extern int lastop_errno;


/*
 * external functions
 */
extern u_int8_t random8(void);
extern u_int16_t random16(void);
extern u_int32_t random32(void);
extern u_int64_t random64(void);
extern int randomcpy(u_int8_t * ptr, int len);
extern double drandom(void);
extern double dcrandom(void);
extern long double ldrandom(void);
extern long double ldcrandom(void);
extern u_int32_t random_val(u_int32_t beyond);
extern u_int64_t random_lval(u_int64_t beyond);


#endif /* __LAVARND_RANDOM_H__ */
