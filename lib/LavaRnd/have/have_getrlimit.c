/*
 * have_getrlimit - determine if we have the getrlimit() call
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: have_getrlimit.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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

#include <unistd.h>
#include <stdlib.h>

#include "have_sys_time.h"
#if defined(HAVE_SYS_TIME_H)
#  include <sys/time.h>
#endif

#include "have_sys_resource.h"
#if defined(HAVE_SYS_RESOURCE_H)
#  include <sys/resource.h>
#endif


struct rlimit rlimit_cpu;	/* CPU time in seconds */
struct rlimit rlimit_fsize;	/* Maximum filesize */
struct rlimit rlimit_data;	/* max data size */
struct rlimit rlimit_stack;	/* max stack size */
struct rlimit rlimit_core;	/* max core file size */
struct rlimit rlimit_rss;	/* max resident set size */
struct rlimit rlimit_nproc;	/* max number of processes */
struct rlimit rlimit_nofile;	/* max number of open files */
struct rlimit rlimit_memlock;	/* max locked-in-memory address space */
struct rlimit rlimit_as;	/* address space (virtual memory) limit */


int
main()
{
    (void)getrlimit(RLIMIT_CPU, &rlimit_cpu);
    (void)getrlimit(RLIMIT_FSIZE, &rlimit_fsize);
    (void)getrlimit(RLIMIT_DATA, &rlimit_data);
    (void)getrlimit(RLIMIT_STACK, &rlimit_stack);
    (void)getrlimit(RLIMIT_CORE, &rlimit_core);
    (void)getrlimit(RLIMIT_RSS, &rlimit_rss);
    (void)getrlimit(RLIMIT_NPROC, &rlimit_nproc);
    (void)getrlimit(RLIMIT_NOFILE, &rlimit_nofile);
    (void)getrlimit(RLIMIT_MEMLOCK, &rlimit_memlock);
    (void)getrlimit(RLIMIT_AS, &rlimit_as);
    exit(0);
}
