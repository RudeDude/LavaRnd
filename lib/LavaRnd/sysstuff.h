/*
 * sysstuff - return a buffer filled with system state related stuff
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: sysstuff.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


#if !defined(__LAVARND_SYSSTUFF_H__)
#  define __LAVARND_SYSSTUFF_H__


/*
 * PORTING NOTE:
 *
 *    If when porting this code to your system and something
 *    won't compile, just remove the offending code and/or replace it
 *    with some other non-blocking system call that queries
 *    but does not alter the system or process.  You don't
 *    have to have every call below to get the needed functionality.
 */


/*
 * included needed for collecting system stuff
 */
#  include <stdio.h>
#  include <errno.h>
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <string.h>
#  include <stdlib.h>
#  include <setjmp.h>
#  include <sys/time.h>
#  include <signal.h>

#  include "LavaRnd/have/have_time.h"
#  if defined(HAVE_TIME_H)
#    include <time.h>
#  endif

#  include "LavaRnd/have/have_sys_time.h"
#  if defined(HAVE_SYS_TIME_H)
#    include <sys/time.h>
#  endif

#  include "LavaRnd/have/have_sys_times.h"
#  if defined(HAVE_SYS_TIMES_H)
#    include <sys/times.h>
#  endif

#  include "LavaRnd/have/have_sys_resource.h"
#  if defined(HAVE_SYS_RESOURCE_H)
#    include <sys/resource.h>
#  endif

#  include "LavaRnd/have/have_statfs.h"
#  if defined(HAVE_STATFS)
#    include <sys/vfs.h>
#  endif

#  include "LavaRnd/have/have_getcontext.h"
#  if defined(HAVE_GETCONTEXT)
#    include <ucontext.h>
#  endif

#  include "LavaRnd/have/have_ustat_h.h"
#  if defined(HAVE_USTAT_H)
#    include <ustat.h>
#  endif

#  include "LavaRnd/have/have_gettime.h"
#  include "LavaRnd/have/have_getprid.h"
#  include "LavaRnd/have/have_getppid.h"
#  include "LavaRnd/have/have_uid_t.h"
#  include "LavaRnd/have/have_ustat.h"
#  include "LavaRnd/have/have_rusage.h"
#  if defined(HAVE_GETRUSAGE) && defined(HAVE_SYS_RESOURCE_H)
#    include <sys/resource.h>
#  endif
#  include "LavaRnd/have/have_sbrk.h"
#  include "LavaRnd/have/have_getrlimit.h"
#  include "LavaRnd/have/have_getpriority.h"
#  include "LavaRnd/have/have_getpgrp.h"

#  include "sha1.h"


/*
 * Here is a bunch of system stuff
 */
struct system_stuff {	/* data collected from system calls */

    /*
     * Make this the first element/action
     *
     * SHA-1 hash everything as is was before
     *
     * NOTE: We realize that the process of building the first_hash
     *       element will alter it so that we will not produce an exact
     *       HASH of the way it was before.  We don't need to be exact here.
     */
    SHA_INFO first_hash;	/* SHA-1 prior to start of call */

    /*
     * These fields are set on every system_stuff() call.
     *
     * These fields are put near the top so help mark and sequence
     * successive calls to this function.
     */
#  if defined(HAVE_GETTIME)
    struct timespec realtime;	/* POSIX realtime clock */
#  endif
#  if defined(HAVE_SYS_TIME_H)
    struct timeval tp;	/* time of day */
#  endif
    unsigned long long counter;	/* call counter */

    /*
     * These fields are only set when partial_set is TRUE,
     * usually only on the first call to system_stuff().
     *
     * Most of these fields rarely change and almost never change for
     * a specific process.  In some cases the field can (and does
     * frequently) change.  Such fields are put into the 'set the
     * value once' field so to provide some variability between
     * different instances / restarts process.
     */
#  if defined(HAVE_GETRLIMIT)
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
#  endif
#  if defined(HAVE_GETPRID)
    prid_t getprid;	/* project ID */
#  endif
    pid_t getpid;	/* process ID */
#  if defined(HAVE_GETPPID)
    pid_t getppid;	/* parent process ID */
#  endif
#  if defined(HAVE_UID_T)
    uid_t getuid;	/* real user ID */
    uid_t geteuid;	/* effective user ID */
    gid_t getgid;	/* real group ID */
    gid_t getegid;	/* effective group ID */
#  endif
    struct stat stat_dot;	/* stat of "." */
    struct stat stat_dotdot;	/* stat of ".." */
    struct stat stat_root;	/* stat of "/" */
    struct stat stat_dev;	/* stat of "/dev" */
    struct stat stat_proc;	/* stat of "/proc" */
#  if defined(HAVE_USTAT)
    struct ustat ustat_dot;	/* usage stat of "." */
    struct ustat ustat_dotdot;	/* usage stat of ".." */
    struct ustat ustat_root;	/* usage stat of "/" */
    struct ustat ustat_dev;	/* usage stat of "/dev" */
    struct ustat ustat_proc;	/* usage stat of "/proc" */
    struct ustat ustat_stdin;	/* usage stat of stdin */
    struct ustat ustat_stdout;	/* usage stat of stdout */
    struct ustat ustat_stderr;	/* usage stat of stderr */
#  endif
#  if defined(HAVE_STATFS)
    struct statfs statfs_dot;	/* file system stats of "." */
    struct statfs statfs_dotdot;	/* file system stats of ".." */
    struct statfs statfs_root;	/* file system stats of "/" */
    struct statfs statfs_dev;	/* file system stats of "/dev" */
    struct statfs statfs_proc;	/* file system stats of "/proc" */
    struct statfs statfs_tmp;	/* file system stats of "/tmp" */
    struct statfs statfs_vartmp;	/* file system stats of "/var/tmp" */
#  endif
#  if defined(HAVE_GETDTABLESIZE)
    int tablesize;	/* size of file descriptor table */
#  endif
#  if defined(HAVE_GETHOSTID)
    long int hostid;	/* host ID */
#  endif
#  if defined(HAVE_GETPRIORITY)
    int priority;	/* program scheduling priority */
#  endif
#  if defined(HAVE_GETPGRP)
    int p_group;	/* process group */
#  endif
#  if defined(HAVE_SYS_TIME_H)
    struct timeval tp_first;	/* time when the first call was made */
#  endif
    SHA_INFO proc_cpuinfo;	/* 1st BUFSIZ SHA-1 hash of /proc/cpuinfo */
    SHA_INFO proc_ioports;	/* 1st BUFSIZ SHA-1 hash of /proc/ioports */
    SHA_INFO proc_ksyms;	/* 1st BUFSIZ SHA-1 hash of /proc/ksyms */
    SHA_INFO proc_meminfo;	/* 1st BUFSIZ SHA-1 hash of /proc/meminfo */
    SHA_INFO proc_pci;	/* 1st BUFSIZ SHA-1 hash of /proc/pci */
    SHA_INFO proc_net_unix;	/* 1st BUFSIZ SHA-1 hash of /proc/net/unix */
    SHA_INFO proc_interrupts;	/* 1st BUFSIZ SHA-1 hash of /proc/interrupts */
    SHA_INFO proc_loadavg;	/* 1st BUFSIZ SHA-1 hash of /proc/loadavg */
    char *sdata_p;	/* address of this structure */
    size_t size;	/* size of this data structure */

    /*
     * These fields are set on every system_stuff() call.
     *
     * Most of these fields change on a frequent basis.
     */
    struct stat stat_tmp;	/* stat of "/tmp" */
    struct stat stat_vartmp;	/* stat of "/var/tmp" */
    struct stat fstat_stdin;	/* stat of stdin */
    struct stat fstat_stdout;	/* stat of stdout */
    struct stat fstat_stderr;	/* stat of stderr */
#  if defined(HAVE_USTAT)
    struct ustat ustat_tmp;	/* usage stat of "/tmp" */
    struct ustat ustat_vartmp;	/* usage stat of "/var/tmp" */
#  endif
#  if defined(HAVE_SBRK)
    void *sbrk;	/* end of data segment */
#  endif
#  if defined(HAVE_GETRUSAGE)
    struct rusage rusage;	/* resource utilization */
    struct rusage rusage_chld;	/* resource utilization of children */
#  endif
#  if defined(HAVE_GETCONTEXT)
    ucontext_t ctx;	/* user context */
#  endif
#  if defined(HAVE_TIMES)
    struct tms times;	/* process times */
#  endif
    time_t time;	/* local time */
    jmp_buf env;	/* setjmp() context */
    SHA_INFO proc_slabinfo;	/* 1st BUFSIZ SHA-1 hash of /proc/slabinfo */
    SHA_INFO proc_stat;	/* 1st BUFSIZ SHA-1 hash of /proc/stat */
    SHA_INFO proc_uptime;	/* 1st BUFSIZ SHA-1 hash of /proc/uptime */
    SHA_INFO proc_net_dev;	/* 1st BUFSIZ SHA-1 hash of /proc/net/dev */
    SHA_INFO proc_net_netstat;	/* 1st BUFSIZ SHA-1 hash of /proc/net/netstat */
    SHA_INFO proc_net_rt_cache_stat;	/* ... hash of /proc/net/rt_cache_stat */
    SHA_INFO proc_net_tcp;	/* 1st BUFSIZ SHA-1 hash of /proc/net/tcp */
    double fnv_seq_value;	/* FNV sequence number over [0.0,1.0] */
    u_int64_t hash_val;	/* fnv64 hash of sdata */
#  if defined(HAVE_SYS_TIME_H)
    struct timeval tp2;	/* time of day again */
#  endif

    /*
     * Make this the last element/action
     *
     * SHA-1 hash everything as it is now
     *
     * NOTE: We realize that the process of building the final_hash
     *       element will alter it so that we will not produce an exact
     *       HASH of the final state.  We don't need to be exact here.
     */
    SHA_INFO final_hash;	/* SHA-1 prior to end of call */
};


/*
 * external functions
 */
extern void system_stuff(struct system_stuff *sdata, int partial_set);


#endif /* __LAVARND_SYSSTUFF_H__ */
