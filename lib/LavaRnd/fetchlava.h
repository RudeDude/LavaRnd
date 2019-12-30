/*
 * fetchlava - raw interface to obtain data from the lavapool daemon
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: fetchlava.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


#if !defined(__LAVARND_FETCHLAVA_H__)
#  define __LAVARND_FETCHLAVA_H__

#  include "lava_callback.h"


/*
 * external variables and functions
 */
#  if defined(LAVA_DEBUG)
extern int raw_lavaop(char *port, u_int8_t * buf, int len, double timeout);
#  endif
extern int preload_cfg(char *cfg_file);
extern int load_cfg(char *cfg_file);
extern double lava_timeout(double timeout);
extern int raw_random(u_int8_t * buf, int len, lavaback callback,
		      int *amt_p, int has_ret);
extern int lava_preload(int lavacnt, int seed_s100);
extern void fetchlava_cleanup(void);


#endif /* __LAVARND_FETCHLAVA_H__ */
