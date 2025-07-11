/*
 * lava_retry - now to retry an operation
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: lava_retry.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


#if !defined(__LAVA_RETRY_H__)
#  define __LAVA_RETRY_H__


/*
 * lava_retry - defines how to retry, how often and how frequently
 */
struct lava_retry {
    double timeout;	/* max time for a single fetch try, 0 ==> no timeout */
    double timeout_grow;	/* expand timeout on successive tries, 1.0==>same */
    int retry;	/* max number of retries, or 0 ==> infinite retries */
    double pause;	/* time to pause between retries */
    double pause_grow;	/* expand pause on successive tries, 1.0==>same */
};

#  define LAVA_MAX_PAUSE (1.0e8)/* do not double pause beyond this period */


#endif /* __LAVA_RETRY_H__ */
