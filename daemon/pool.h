/*
 * pool - lavapool operations
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: pool.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


#if !defined(__POOL_H__)
#  define __POOL_H__


#  include <sys/types.h>


#  define MAXPOOL_IO (65536)	/* max read/write from a pool at one time */


/*
 * external variables
 */
extern void init_pool(u_int32_t size);
extern int fill_pool_from_fd(int fd);
extern int fill_pool_from_chaos(void *buf, int buflen);
extern int drain_pool(u_int8_t * buf, int cnt);
extern u_int32_t pool_level(void);
extern double pool_frac(void);
extern double pool_rate_factor(void);
extern void free_pool(void);


#endif /* __POOL_H__ */
