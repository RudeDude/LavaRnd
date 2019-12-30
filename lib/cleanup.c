/*
 * dormant - so things like valgrind will not complain about memory leaks
 *
 * This call will attempt to clean up malloc memory / close things down
 * so that memory checkers such as valgrind will not complain.  Normally
 * there is no need to call this function.
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: cleanup.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

#include "LavaRnd/rawio.h"
#include "LavaRnd/fetchlava.h"

#if defined(DMALLOC)
#  include <dmalloc.h>
#endif


/*
 * lava_dormant - make the LavaRnd processing dormant by freeing memory / etc
 *
 * This call will attempt to clean up malloc memory / close things down
 * so that memory checkers such as valgrind will not complain.  Normally
 * there is no need to call this function.
 */
void
lava_dormant(void)
{
    /* lavasocket.c */
    lavasocket_cleanup();

    /* fetchlava.a */
    fetchlava_cleanup();
}
