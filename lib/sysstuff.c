/*
 * sysstuff - return a buffer filled with system state related junk
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: sysstuff.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


/*
 * PORTING NOTE:
 *
 *    If when porting this code to your system and something
 *    won't compile, just remove the offending code and/or replace it
 *    with some other non-blocking system call that queries
 *    but does not alter the system or process.  You don't
 *    have to have every call below to get the needed functionality.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>

#include "LavaRnd/sysstuff.h"
#include "LavaRnd/fnv1.h"

#if defined(DMALLOC)
#  include <dmalloc.h>
#endif


/*
 * try_sha1_file - form the SHA1 hash of the first BUFSIZ of a file, if possible
 *
 * usage:
 *      path    path of file to hash
 *      hash    pointer to a SHA_INFO structure to fill out
 *
 * NOTE: This routine makes an attempt to SHA1 hash up to the first BUFSIZ
 *       of the contents of a file.  This routine leaves the SHA_INFO structure
 *       unchanged on open or read errors or bogus args.
 *
 * NOTE: We do not care if we fail to open/read the file.  This function is
 *       only being used by system_stuff() which is just collecting salt.
 */
static void
try_sha1_file(char *path, SHA_INFO *hash)
{
    char buf[BUFSIZ];	/* content from the first BUFSIZ octets of the file */
    int fd;	/* open file descriptor */
    int readsiz;	/* amount read from file, or <0 ==> error */

    /*
     * firewall
     */
    if (path == NULL || hash == NULL) {
	/* bad args */
	return;
    }

    /*
     * open file
     */
    fd = open(path, O_RDONLY);
    if (fd < 0) {
	/* failed to open */
	return;
    }

    /*
     * read what we can into the buffer and then close the file
     */
    readsiz = read(fd, buf, BUFSIZ);
    (void)close(fd);
    if (readsiz < 0) {
	/* read error */
	return;
    }

    /*
     * hash the buffer
     */
    lava_sha_init(hash);
    lava_sha_update(hash, buf, readsiz);
    lava_sha_final2(hash);
    return;
}


/*
 * system_stuff - collect system state related stuff
 *
 * given:
 *      sdata           pointer to a struct system_stuff to write
 *      partial_set     TRUE ==> do not collect stuff that rarely changes
 *                               between system_stuff() calls
 *
 * Generate a bunch of stuff based on system and process information.
 *
 * NOTE: This is not a good source of chaotic data.  The lavarand
 *       system does a much better job of that.  See:
 *
 *              http://www.lavarnd.com/index.html
 *
 * This code is used when we cannot contact random daemon and our lib
 * interface insists on random interface calls always return something.
 *
 * PORTING NOTE:
 *
 *    If when porting this code to your system and something
 *    won't compile, just remove that line or replace it with
 *    some other system call.  We don't have to have every call
 *    operating below.  We only want to hash the resulting data.
 *
 * NOTE: It is OK if these system calls fail.  It is OK there are
 *       attempts the access files that do not exist.
 */
void
system_stuff(struct system_stuff *sdata, int partial_set)
{
    /*
     * pick up process/system information
     *
     * NOTE: We do care (that much) if these calls fail.  We do not
     *       need to process any data in the 'sdata' structure.
     */

    /*
     * Make this the first element/action
     *
     * SHA-1 hash everything as is was before
     *
     * NOTE: We realize that the process of building the first_hash
     *       element will alter it so that we will not produce an exact
     *       HASH of the way it was before.  We don't need to be exact here.
     */
    lava_sha_init(&sdata->first_hash);
    lava_sha_update(&sdata->first_hash, (u_int8_t *) & sdata, sizeof(sdata));
    lava_sha_final2(&sdata->first_hash);

    /*
     * These fields are set on every system_stuff() call.
     *
     * These fields are put near the top so help mark and sequence
     * successive calls to this function.
     */
#if defined(HAVE_GETTIME)
    (void)clock_gettime(CLOCK_REALTIME, &sdata->realtime);
#endif
#if defined(HAVE_SYS_TIME_H)
    (void)gettimeofday(&sdata->tp, NULL);
#endif
    ++sdata->counter;

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
    if (!partial_set) {
#if defined(HAVE_GETRLIMIT)
	(void)getrlimit(RLIMIT_CPU, &sdata->rlimit_cpu);
	(void)getrlimit(RLIMIT_FSIZE, &sdata->rlimit_fsize);
	(void)getrlimit(RLIMIT_DATA, &sdata->rlimit_data);
	(void)getrlimit(RLIMIT_STACK, &sdata->rlimit_stack);
	(void)getrlimit(RLIMIT_CORE, &sdata->rlimit_core);
	(void)getrlimit(RLIMIT_RSS, &sdata->rlimit_rss);
	(void)getrlimit(RLIMIT_NPROC, &sdata->rlimit_nproc);
	(void)getrlimit(RLIMIT_NOFILE, &sdata->rlimit_nofile);
	(void)getrlimit(RLIMIT_MEMLOCK, &sdata->rlimit_memlock);
	(void)getrlimit(RLIMIT_AS, &sdata->rlimit_as);
#endif
#if defined(HAVE_GETPRID)
	sdata->getprid = getprid();
#endif
	sdata->getpid = getpid();
#if defined(HAVE_GETPPID)
	sdata->getppid = getppid();
#endif
#if defined(HAVE_UID_T)
	sdata->getuid = getuid();
	sdata->geteuid = geteuid();
	sdata->getgid = getgid();
	sdata->getegid = getegid();
#endif
	(void)stat(".", &sdata->stat_dot);
	(void)stat("..", &sdata->stat_dotdot);
	(void)stat("/", &sdata->stat_root);
	(void)stat("/dev", &sdata->stat_dev);
	(void)stat("/proc", &sdata->stat_proc);
#if defined(HAVE_USTAT)
	(void)ustat(sdata->stat_dotdot.st_dev, &sdata->ustat_dotdot);
	(void)ustat(sdata->stat_dot.st_dev, &sdata->ustat_dot);
	(void)ustat(sdata->stat_root.st_dev, &sdata->ustat_root);
	(void)ustat(sdata->stat_dev.st_dev, &sdata->ustat_dev);
	(void)ustat(sdata->stat_proc.st_dev, &sdata->ustat_proc);
	(void)ustat(sdata->fstat_stdin.st_dev, &sdata->ustat_stdin);
	(void)ustat(sdata->fstat_stdout.st_dev, &sdata->ustat_stdout);
	(void)ustat(sdata->fstat_stderr.st_dev, &sdata->ustat_stderr);
#endif
#if defined(HAVE_STATFS)
	(void)statfs(".", &sdata->statfs_dot);
	(void)statfs("..", &sdata->statfs_dotdot);
	(void)statfs("/", &sdata->statfs_root);
	(void)statfs("/dev", &sdata->statfs_dev);
	(void)statfs("/proc", &sdata->statfs_proc);
	(void)statfs("/tmp", &sdata->statfs_tmp);
	(void)statfs("/var/tmp", &sdata->statfs_vartmp);
#endif
#if defined(HAVE_GETDTABLESIZE)
	sdata->tablesize = getdtablesize();
#endif
#if defined(HAVE_GETHOSTID)
	sdata->hostid = gethostid();
#endif
#if defined(HAVE_GETPRIORITY)
	sdata->priority = getpriority(PRIO_PROCESS, 0);
#endif
#if defined(HAVE_GETPGRP)
	sdata->p_group = getpgrp();
#endif
#if defined(HAVE_SYS_TIME_H)
	(void)gettimeofday(&sdata->tp_first, NULL);
#endif
	try_sha1_file("/proc/cpuinfo", &sdata->proc_cpuinfo);
	try_sha1_file("/proc/ioports", &sdata->proc_ioports);
	try_sha1_file("/proc/ksyms", &sdata->proc_ksyms);
	try_sha1_file("/proc/meminfo", &sdata->proc_meminfo);
	try_sha1_file("/proc/pci", &sdata->proc_pci);
	try_sha1_file("/proc/net/unix", &sdata->proc_net_unix);
	try_sha1_file("/proc/interrupts", &sdata->proc_interrupts);
	try_sha1_file("/proc/loadavg", &sdata->proc_loadavg);
	sdata->sdata_p = (char *)&sdata;
	sdata->size = sizeof(sdata);
    }

    /*
     * These fields are set on every system_stuff() call.
     *
     * Most of these fields change on a frequent basis.
     */
    (void)stat("/tmp", &sdata->stat_tmp);
    (void)stat("/var/tmp", &sdata->stat_vartmp);
    (void)fstat(0, &sdata->fstat_stdin);
    (void)fstat(1, &sdata->fstat_stdout);
    (void)fstat(2, &sdata->fstat_stderr);
#if defined(HAVE_USTAT)
    (void)ustat(sdata->stat_tmp.st_dev, &sdata->ustat_tmp);
    (void)ustat(sdata->stat_vartmp.st_dev, &sdata->ustat_vartmp);
#endif
#if defined(HAVE_SBRK)
    sdata->sbrk = sbrk(0);
#endif
#if defined(HAVE_GETRUSAGE)
    (void)getrusage(RUSAGE_SELF, &sdata->rusage);
    (void)getrusage(RUSAGE_CHILDREN, &sdata->rusage_chld);
#endif
#if defined(HAVE_GETCONTEXT)
    (void)getcontext(&sdata->ctx);
#endif
#if defined(HAVE_TIMES)
    (void)times(&sdata->times);
#endif
    sdata->time = time(NULL);
    (void)setjmp(sdata->env);
    try_sha1_file("/proc/slabinfo", &sdata->proc_slabinfo);
    try_sha1_file("/proc/stat", &sdata->proc_stat);
    try_sha1_file("/proc/uptime", &sdata->proc_uptime);
    try_sha1_file("/proc/net/dev", &sdata->proc_net_dev);
    try_sha1_file("/proc/net/netstat", &sdata->proc_net_netstat);
    try_sha1_file("/proc/net/rt_cache_stat", &sdata->proc_net_rt_cache_stat);
    try_sha1_file("/proc/net/tcp", &sdata->proc_net_tcp);
    sdata->fnv_seq_value = fnv_seq();
    sdata->hash_val = fnv1_hash((char *)&sdata, sizeof(sdata));
#if defined(HAVE_SYS_TIME_H)
    (void)gettimeofday(&sdata->tp2, NULL);
#endif

    /*
     * Make this the last element/action
     *
     * SHA-1 hash everything as it is now
     *
     * NOTE: We realize that the process of building the final_hash
     *       element will alter it so that we will not produce an exact
     *       HASH of the final state.  We don't need to be exact here.
     */
    lava_sha_init(&sdata->final_hash);
    lava_sha_update(&sdata->final_hash, (u_int8_t *) & sdata, sizeof(sdata));
    lava_sha_final2(&sdata->final_hash);

    /*
     * all done!!!
     */
    return;
}
