/*
 * simple_url - simple / naive URL to content retriever
 *
 * The URL content fetching is simple / naive URL in that it only deals
 * with successful 200 codes and not server redirects.  The LavaRnd data
 * converter is fully operational and correct.
 *
 * @(#) $Revision: 10.2 $
 * @(#) $Id: simple_url.h,v 10.2 2003/11/09 22:37:03 lavarnd Exp $
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


#if !defined(__SIMPLE_URL_H__)
#  define __SIMPLE_URL_H__

#  include <sys/types.h>

/*
 * lavapool data fetching constants
 */
#  define LAVA_DEF_PAUSE (3)	/* default retry time */
#  define LAVA_DEF_TIMEOUT (5)	/* default random daemon fetch timeout */
#  define LAVA_S100_TIMEOUT (10)/* timeout when requesting s100 table data */



/*
 * external functions
 */
extern u_int8_t *lava_url_content;
extern void lava_url_cleanup(void);
extern int lava_get_url(char *url, int32_t minlen,
			struct lava_retry *lava_retry);


#endif /* __SIMPLE_URL_H__ */
