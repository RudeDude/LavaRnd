/*
 * liblava_s100_high - use only high quality s100
 *
 * This is the compiled in default callback used by the liblava_s100_high.a
 * library.  See the lava_callback.h, random.c and random_lib.c files
 * for more details.
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: liblava_s100_high.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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

#include "LavaRnd/lava_callback.h"

#if defined(DMALLOC)
#  include <dmalloc.h>
#endif


/*
 * default compiled in callback
 *
 * The lava_callback is the default callback that is used by the random.c
 * and random_libc.c interface.
 *
 * The default compiled in callback may be overridden by calling the
 * set_lava_callback() function below.
 */
lavaback lava_callback = LAVACALL_S100_HIGH;
