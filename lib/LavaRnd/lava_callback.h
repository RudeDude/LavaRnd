/*
 * callback - lavapool interface callbacks
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: lava_callback.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


#if !defined(__LAVARND_LAVA_CALLBACK_H__)
#  define __LAVARND_LAVA_CALLBACK_H__

#  include <sys/types.h>
#  include "lavaquality.h"


/*
 * callback - default random number callback
 *
 * This is the callback that is used when the random number service encounters
 * an error.  Usually this error is a failure to communicate with the lavapool
 * daemon in a specified amount of time.  Other errors include malloc
 * failures and system call errors.  See lavaerr.h for details on possible
 * errors.
 *
 * Here is a handy cheat sheet:
 *
 *      LAVACALL_LAVA_EXIT
 *
 *          Only return LavaRnd data.
 *
 *          If service is down, retry a few times, pausing in between tries.
 *          If all else fails, exit.
 *
 *      LAVACALL_LAVA_RETRY
 *
 *          Only return LavaRnd data.
 *
 *          If service is down, try again after a pause.  Keep trying
 *          until the service is back up.
 *
 *      LAVACALL_LAVA_RETURN
 *
 *          Only return LavaRnd data.
 *
 *          If service is down, retry a few times, pausing in between tries.
 *          If all else fails, return an error.
 *
 *      LAVACALL_TRY_HIGH
 *      LAVACALL_TRY_MED
 *      LAVACALL_TRY_ANY
 *
 *          Try to return LavaRnd data, but if you cannot, return a certain
 *          quality (HIGH, MEDium or ANY quality) s100 data instead.
 *
 *          If the lavapool times out/fails, use the s100 data instead.
 *          Thus in the case of a dead lavapool daemon, there will always
 *          by a delay before data is returned.  You might want to use
 *          one of the LAVACALL_TRYONCE_XYZ types below instead.
 *
 *      LAVACALL_TRYONCE_HIGH
 *      LAVACALL_TRYONCE_MED
 *      LAVACALL_TRYONCE_ANY
 *
 *          Try to return LavaRnd data, but if ever fail to communicate
 *          with the daemon, then from then on, just use s100 instead ...
 *          or at least keep using s100 until some other callback (such
 *          as LAVACALL_TRY_XYZ) manages to successfully communicate with
 *          the daemon.
 *
 *          When returning s100 data, return s100 data that is of a given
 *          quality (HIGH, MEDium or ANY quality).
 *
 *      LAVACALL_S100_HIGH
 *      LAVACALL_S100_MED
 *      LAVACALL_S100_ANY
 *
 *          Only return s100 data.  When returning s100 data, return s100
 *          data that is of a given quality (HIGH, MEDium or ANY quality).
 */


/*
 * callback return values indicating what internal functions should do
 *
 * If an error is encountered while attempting to fulfill a request
 * for random data, and the callback function is NOT one of the
 * LAVACALL_XYZ values listed above, then internal LavaRnd functions
 * will call the callback function and use its return value to
 * determine what to do next.
 *
 * The callback function, when called, is given the following information:
 *
 *      buf - pointer to where data is being written
 *      len - size of the initial request / size of buf in octets
 *      offset - amount of data already written
 *      err - error code (LAVAERR_TIMEOUT, LAVAERR_IOERR, ...) (see lavaerr.h)
 *      ret - FALSE ==> calling function (public interface) will ignore errors,
 *            TRUE ==> calling function will receive a status code
 */
typedef double (*lavaback) (u_int8_t * buf, int len, int offset, int err,
			    int ret);


/*
 * special pre-defined callbacks
 *
 *    LAVACALL_LAVA_EXIT
 *      Exit after exit_retries failures to obtain data.  The exit_retries is
 *      defined by cfg.lavapool config file.  Only LavaRnd data is returned.
 *
 *      The maximum quality returned is LAVA_QUAL_LAVARND.
 *      The minimum quality returned is LAVA_QUAL_LAVARND.
 *
 *    LAVACALL_LAVA_RETRY
 *      Keep retrying after a failures to obtain data in the hopes that
 *      things will work someday.  If they don't then the public interface
 *      function will not return.  Only LavaRnd data is returned.
 *
 *      The maximum quality returned is LAVA_QUAL_LAVARND.
 *      The minimum quality returned is LAVA_QUAL_LAVARND.
 *
 *    LAVACALL_LAVA_RETURN
 *
 *      Try up to return_retries times to obtain data.  If it still cannot
 *      obtain LavaRnd data, turn return an error.  Only LavaRnd data is
 *      returned.
 *
 *      The maximum quality returned is LAVA_QUAL_LAVARND.
 *      The minimum quality returned is LAVA_QUAL_LAVARND.
 *
 *      NOTE: The caller must be prepared to check for error codes and
 *            act accordingly when the multiple attempts to contact the
 *            lavapool server fail.
 *
 *    LAVACALL_TRY_HIGH
 *      First attempt to return LavaRnd data.  If it cannot obtain
 *      LAVA_QUAL_LAVARND data, then return s100 data of LAVA_QUAL_S100HIGH
 *      quality.  If the data quality is too low, then the LAVAERR_POORQUAL
 *      error code will be returned.  It is assumed
 *      that the user's code will check the return code.
 *
 *      NOTE: On this callback, we will always try to read the
 *            the LavaRnd service first.  If the lavapool is not
 *            available, then this routine will ALWAYS timeout before
 *            trying s100 data.  Thus LAVACALL_TRY_XYZ callbacks will
 *            be very slow in the face of a dead lavapool daemon.
 *            You might want to use LAVACALL_TRYONCE_HIGH instead.
 *
 *      NOTE: If due to a lavapool daemon failure the s100 quality
 *            level drops below an acceptable level, AND you manage
 *            to restart/connect to the daemon again, then you might
 *            want to call s100_unload() which will force an attempt
 *            to reseed with quality LavaRnd data again.
 *
 *      The maximum quality returned is LAVA_QUAL_LAVARND.
 *      The minimum quality returned is LAVA_QUAL_S100HIGH.
 *
 *    LAVACALL_TRY_MED
 *      First attempt to return LavaRnd data.  If it cannot obtain
 *      LAVA_QUAL_LAVARND data, then return s100 data of LAVA_QUAL_S100MED
 *      or LAVA_QUAL_S100HIGH quality.  If the data quality is too low,
 *      then the LAVAERR_POORQUAL error code will be returned.  It is
 *      assumed that the user's code will check the return code.
 *
 *      NOTE: On this callback, we will always try to read the
 *            the LavaRnd service first.  If the lavapool is not
 *            available, then this routine will ALWAYS timeout before
 *            trying s100 data.  Thus LAVACALL_TRY_XYZ callbacks will
 *            be very slow in the face of a dead lavapool daemon.
 *            You might want to use LAVACALL_TRYONCE_MED instead.
 *
 *      NOTE: If due to a lavapool daemon failure the s100 quality
 *            level drops below an acceptable level, AND you manage
 *            to restart/connect to the daemon again, then you might
 *            want to call s100_unload() which will force an attempt
 *            to reseed with quality LavaRnd data again.
 *
 *      The maximum quality returned is LAVA_QUAL_LAVARND.
 *      The minimum quality returned is LAVA_QUAL_S100MED.
 *
 *    LAVACALL_TRY_ANY
 *      First attempt to return LavaRnd data.  If it cannot obtain
 *      LAVA_QUAL_LAVARND data, then return s100 data of any quality.
 *
 *      NOTE: On this callback, we will always try to read the
 *            the LavaRnd service first.  If the lavapool is not
 *            available, then this routine will ALWAYS timeout before
 *            trying s100 data.  Thus LAVACALL_TRY_XYZ callbacks will
 *            be very slow in the face of a dead lavapool daemon.
 *            You might want to use LAVACALL_TRYONCE_ANY instead.
 *
 *      The maximum quality returned is LAVA_QUAL_LAVARND.
 *      The minimum quality returned is LAVA_QUAL_S100LOW.
 *
 *    LAVACALL_TRYONCE_HIGH
 *      First attempt to return LavaRnd data.  If it cannot obtain
 *      LAVA_QUAL_LAVARND data, then always return s100 data of
 *      LAVA_QUAL_S100HIGH quality.  If the data quality is too low,
 *      then the LAVAERR_POORQUAL error code will be returned.  It is
 *      assumed that the user's code will check the return code.
 *
 *      NOTE: Unlike LAVACALL_TRY_HIGH, this callback will use s100 after
 *            the first lavapool failure.  Thus this callback will not
 *            introduce timeout delays (except for the first failure)
 *            every time data is requested.
 *
 *      NOTE: After a lavapool failure this callback will not try to
 *            contact the daemon until some other callback has successfully
 *            communicated with it.  To reset the 'do not bother contacting
 *            the daemon flag', use the LAVACALL_TRY_HIGH callback or call
 *            the fetchlava_reset() function.
 *
 *      NOTE: If due to a lavapool daemon failure the s100 quality
 *            level drops below an acceptable level, AND you manage
 *            to restart/connect to the daemon again, then you might
 *            want to call s100_unload() which will force an attempt
 *            to reseed with quality LavaRnd data again.
 *
 *      The maximum quality returned is LAVA_QUAL_LAVARND.
 *      The minimum quality returned is LAVA_QUAL_S100HIGH.
 *
 *    LAVACALL_TRYONCE_MED
 *      First attempt to return LavaRnd data.  If it cannot obtain
 *      LAVA_QUAL_LAVARND data, then always return s100 data of
 *      LAVA_QUAL_S100MED quality.  If the data quality is too low,
 *      then the LAVAERR_POORQUAL error code will be returned.  It is
 *      assumed that the user's code will check the return code.
 *
 *      NOTE: Unlike LAVACALL_TRY_MED, this callback will use s100 after
 *            the first lavapool failure.  Thus this callback will not
 *            introduce timeout delays (except for the first failure)
 *            every time data is requested.
 *
 *      NOTE: After a lavapool failure this callback will not try to
 *            contact the daemon until some other callback has successfully
 *            communicated with it.  To reset the 'do not bother contacting
 *            the daemon flag', use the LAVACALL_TRY_MED callback or call
 *            the fetchlava_reset() function.
 *
 *      NOTE: If due to a lavapool daemon failure the s100 quality
 *            level drops below an acceptable level, AND you manage
 *            to restart/connect to the daemon again, then you might
 *            want to call s100_unload() which will force an attempt
 *            to reseed with quality LavaRnd data again.
 *
 *      The maximum quality returned is LAVA_QUAL_LAVARND.
 *      The minimum quality returned is LAVA_QUAL_S100MED.
 *
 *    LAVACALL_TRYONCE_ANY
 *      First attempt to return LavaRnd data.  If it cannot obtain
 *      LAVA_QUAL_LAVARND data, then always return s100 data of any
 *      quality.
 *
 *      NOTE: Unlike LAVACALL_TRY_ANY, this callback will use s100 after
 *            the first lavapool failure.  Thus this callback will not
 *            introduce timeout delays (except for the first failure)
 *            every time data is requested.
 *
 *      NOTE: After a lavapool failure this callback will not try to
 *            contact the daemon until some other callback has successfully
 *            communicated with it.  To reset the 'do not bother contacting
 *            the daemon flag', use the LAVACALL_TRY_ANY callback or call
 *            the fetchlava_reset() function.
 *
 *      The maximum quality returned is LAVA_QUAL_LAVARND.
 *      The minimum quality returned is LAVA_QUAL_S100LOW.
 *
 *    LAVACALL_S100_HIGH
 *      Only return s100 data.  Only data of LAVA_QUAL_S100HIGH
 *      is acceptable.  If the data quality is too low, then the
 *      LAVAERR_POORQUAL error code will be returned.  It is assumed
 *      that the user's code will check the return code.
 *
 *      NOTE: If due to a lavapool daemon failure the s100 quality
 *            level drops below an acceptable level, AND you manage
 *            to restart/connect to the daemon again, then you might
 *            want to call s100_unload() which will force an attempt
 *            to reseed with quality LavaRnd data again.
 *
 *      The maximum quality returned is LAVA_QUAL_S100HIGH.
 *      The minimum quality returned is LAVA_QUAL_S100HIGH.
 *
 *    LAVACALL_S100_MED
 *      Only return s100 data.  The best quality data will be returned,
 *      however only data of LAVA_QUAL_S100MED or LAVA_QUAL_S100HIGH
 *      is acceptable.  If the data quality is too low, then the
 *      LAVAERR_POORQUAL error code will be returned.  It is assumed
 *      that the user's code will check the return code.
 *
 *      NOTE: If due to a lavapool daemon failure the s100 quality
 *            level drops below an acceptable level, AND you manage
 *            to restart/connect to the daemon again, then you might
 *            want to call s100_unload() which will force an attempt
 *            to reseed with quality LavaRnd data again.
 *
 *      The maximum quality returned is LAVA_QUAL_S100HIGH.
 *      The minimum quality returned is LAVA_QUAL_S100MED.
 *
 *    LAVACALL_S100_ANY
 *      Only return s100 data.  The best quality data will be returned,
 *      however data of any quality is acceptable.
 *
 *      The maximum quality returned is LAVA_QUAL_S100HIGH.
 *      The minimum quality returned is LAVA_QUAL_S100LOW.
 *
 *    LAVACALL_LAVA_INVALID
 *	This is never an valid callback.  Use of this callback
 *	is undefined.
 */
#  define CASE_LAVACALL_INVALID (0)
#  define LAVACALL_LAVA_INVALID ((lavaback)CASE_LAVACALL_INVALID)
#  define CASE_LAVACALL_LAVA_EXIT (1)
#  define LAVACALL_LAVA_EXIT ((lavaback)CASE_LAVACALL_LAVA_EXIT)
#  define CASE_LAVACALL_LAVA_RETRY (2)
#  define LAVACALL_LAVA_RETRY ((lavaback)CASE_LAVACALL_LAVA_RETRY)
#  define CASE_LAVACALL_LAVA_RETURN (3)
#  define LAVACALL_LAVA_RETURN ((lavaback)CASE_LAVACALL_LAVA_RETURN)
#  define CASE_LAVACALL_TRY_HIGH (4)
#  define LAVACALL_TRY_HIGH ((lavaback)CASE_LAVACALL_TRY_HIGH)
#  define CASE_LAVACALL_TRY_MED (5)
#  define LAVACALL_TRY_MED ((lavaback)CASE_LAVACALL_TRY_MED)
#  define CASE_LAVACALL_TRY_ANY (6)
#  define LAVACALL_TRY_ANY ((lavaback)CASE_LAVACALL_TRY_ANY)
#  define CASE_LAVACALL_TRYONCE_HIGH (7)
#  define LAVACALL_TRYONCE_HIGH ((lavaback)CASE_LAVACALL_TRYONCE_HIGH)
#  define CASE_LAVACALL_TRYONCE_MED (8)
#  define LAVACALL_TRYONCE_MED ((lavaback)CASE_LAVACALL_TRYONCE_MED)
#  define CASE_LAVACALL_TRYONCE_ANY (9)
#  define LAVACALL_TRYONCE_ANY ((lavaback)CASE_LAVACALL_TRYONCE_ANY)
#  define CASE_LAVACALL_S100_HIGH (10)
#  define LAVACALL_S100_HIGH ((lavaback)CASE_LAVACALL_S100_HIGH)
#  define CASE_LAVACALL_S100_MED (11)
#  define LAVACALL_S100_MED ((lavaback)CASE_LAVACALL_S100_MED)
#  define CASE_LAVACALL_S100_ANY (12)
#  define LAVACALL_S100_ANY ((lavaback)CASE_LAVACALL_S100_ANY)


/*
 * special callback return values
 *
 * If the callback function returns a value >0, then that is the timeout
 * value (in sec) for a retry.  Otherwise it must be one of the following
 * values which have special meaning:
 *
 *    LAVA_CALLBACK_EXIT
 *      The callback function is requisition that the internal routine exit.
 *
 *    LAVA_CALLBACK_RETURN
 *      The callback function is requesting that the internal function return
 *      it its caller.  Here the caller is the public interface that the
 *      user code called to obtain random data.
 *
 *      The callback function should consider the 'ret' argument before
 *      returning this value.  If 'ret' is FALSE, the caller (via a public
 *      interface) is ignoring status codes.  The callback function may
 *      want to return LAVA_CALLBACK_S100LOW, when 'ret' is FALSE to
 *      avoid returning highly non-random bogus values.
 *
 *      For example, the libc-like public interface function random()
 *      simply returns a long (4 octets with sign bit masked off).  The
 *      random() function has no concept of an error.  If in the course
 *      of obtaining random()'s 4 random octets some error occurs, then
 *      the callback function will be called with a ret value of FALSE.
 *      If the callback returns LAVA_CALLBACK_RETURN forcing the internal
 *      function to return, then some undefined and likely highly non-random
 *      bogus value will be returned by random().
 *
 *   LAVA_CALLBACK_S100LOW
 *      The internal function should use the s100 function to return the
 *      best s100 data that it can.  Any quality of data of is acceptable.
 *
 *      If 'ret' is FALSE, then the the caller (via a public interface)
 *      ignoring status codes.  By the callback function returning this
 *      value in that case, some pseudo-random data is returned instead
 *      of highly non-random bogus value.
 *
 *   LAVA_CALLBACK_S100MED
 *      The internal function should use the s100 function to return the
 *      best s100 data that it can.   The quality of data must be
 *      LAVA_QUAL_S100MED or better.  If the quality is not up to
 *      LAVA_QUAL_S100MED standards, then the LAVAERR_POORQUAL error
 *      code is returned.
 *
 *      The callback function should consider the 'ret' argument before
 *      returning this value.  If 'ret' is FALSE, the caller (via a public
 *      interface) is ignoring status codes.  The callback function may
 *      want to return LAVA_CALLBACK_S100LOW, when 'ret' is FALSE to
 *      avoid returning highly non-random bogus values.
 *
 *   LAVA_CALLBACK_S100HIGH
 *      The internal function should use the s100 function to return the
 *      best s100 data that it can.   The quality of data must be
 *      LAVA_QUAL_S100HIGH or better.  If the quality is not up to
 *      LAVA_QUAL_S100HIGH standards, then the LAVAERR_POORQUAL error code
 *      is returned.
 *
 *      The callback function should consider the 'ret' argument before
 *      returning this value.  If 'ret' is FALSE, the caller (via a public
 *      interface) is ignoring status codes.  The callback function may
 *      want to return LAVA_CALLBACK_S100LOW, when 'ret' is FALSE to
 *      avoid returning highly non-random bogus values.
 */
#  define LAVA_CALLBACK_EXIT (0.0)	/* do not return, just exit */
#  define LAVA_CALLBACK_RETURN (-1.0)	/* return error status or best effort */
#  define LAVA_CALLBACK_S100LOW (-2.0)	/* return LAVA_QUAL_S100HIGH data */
#  define LAVA_CALLBACK_S100MED (-3.0)	/* return LAVA_QUAL_S100MED data */
#  define LAVA_CALLBACK_S100HIGH (-4.0)	/* return LAVA_QUAL_S100LOW data */


#endif /* __LAVARND_LAVA_CALLBACK_H__ */
