/*
 * lavapool - LavaRnd pool of cryptographically strong data
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: lavapool.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>

#include "LavaRnd/sha1.h"
#include "LavaRnd/rawio.h"
#include "LavaRnd/cfg.h"
#include "LavaRnd/fetchlava.h"
#include "LavaRnd/cleanup.h"
#include "LavaRnd/lavarnd.h"

#include "chan.h"
#include "cfg_lavapool.h"
#include "pool.h"
#include "dbg.h"

#if defined(DMALLOC)
#include <dmalloc.h>
#endif


#define MIN_TIMEOUT (5.0)		/* min timeout for a cycle */
#define MAX_TIMEOUT (240.0-MIN_TIMEOUT)	/* max timeout for a cycle */


/*
 * global vars
 */
char *program = "";	/* our name */
char *prog = "";	/* basename of our name */


/*
 * static functions and vars
 */
static void config(char *rnd_cfg_name, char *lava_cfg_name);
static double timeout_value(void);
static void drop_privs(char *username, char *chrootdir);


/*
 * static variables
 */
static char *usage =
    "usage: %s [-r cfg.random] [-p cfg.lavapool] [-l logfile] [-S] [-Q] [-b]\n"
    "		[-c chrootdir] [-u username] [-v verbose_lvl] [-t runtime]\n\n"
    "\t-h		output this usage message\n"
    "\t-r cfg.random	cfg.random random interface config file path\n"
    "\t-p cfg.lavapool	cfg.lavapool lavapool daemon config file path\n"
    "\t-l logfile	append debug / error messages to log\n"
    "\t-S		send debug, warnings and error messages to syslog\n"
    "\t-Q		dont send debug, warnings & error messages to stderr\n"
    "\t-b		fork into background after arg check\n"
    "\t-c chrootdir	chroot to chrootdir after startup / initialization\n"
    "\t-u username	convert to user after startup / initialization\n"
    "\t-v verbose_lvl	verbosity / debug level\n"
    "\t-t runtime	cleanup & exit after runtime secs (0.0 => forever)\n";


int
main(int argc, char *argv[])
{
    char *rnd_cfg_name = LAVA_RANDOM_CFG;	/* cfg.random , NULL==>def */
    char *lava_cfg_name = LAVA_LAVAPOOL_CFG;	/* cfg.lavapool filename */
    char *chrootdir = NULL;		/* non-null ==> chroot dir */
    char *username = NULL;		/* non-null ==> username to operate */
    int use_syslog = 0;			/* 1 => open/use syslog */
    char *logfile = NULL;		/* name of debug / log file */
    int use_stderr = 1;			/* 1 => msgs to stderr */
    int fork_mode = 0;			/* 1 =< fork into background */
    double timeout;			/* channel cycle timeout value */
    double runtime = 0.0;		/* cleanup & exit after runtime secs */
    double endtime = 0.0;		/* end time of chan loop or 0.0 */
    extern char *optarg;		/* option argument */
    extern int optind;			/* argv index of the next arg */
    int prog_malloced = FALSE;		/* TRUE ==> prog is a malloced string */
    char *p;
    int i;

    /*
     * parse args
     */
    program = argv[0];
    p = strrchr(program, '/');
    if (p == NULL) {
	prog = strdup(program);
	prog_malloced = TRUE;
    } else {
    	prog = strdup(p+1);
	prog_malloced = TRUE;
    }
    if (prog == NULL) {
	prog = "lavapool";
	prog_malloced = FALSE;
    }
    while ((i = getopt(argc, argv, "hr:l:p:SQbv:c:u:t:")) != -1) {
	switch (i) {
	case 'h':
	    fprintf(stderr, usage, prog);
	    exit(1);
	case 'r':
	    rnd_cfg_name = optarg;
	    break;
	case 'l':
	    logfile = optarg;
	    (void) setlog(logfile);
	    break;
	case 'p':
	    lava_cfg_name = optarg;
	    break;
	case 'S':
	    use_syslog = 1;
	    break;
	case 'Q':
	    use_stderr = 0;
	    break;
	case 'b':
	    fork_mode = 1;
	    break;
	case 'v':
	    dbg_lvl = strtol(optarg, NULL, 0);
	    break;
	case 'c':
	    chrootdir = optarg;
	    break;
	case 'u':
	    username = optarg;
	    break;
	case 't':
	    runtime = strtod(optarg, NULL);
	    if (runtime < 0.0) {
	    	runtime = 0.0;
	    }
	    break;
	default:
	    fprintf(stderr, usage, prog);
	    exit(1);
	}
    }

    /*
     * I/O and syslog cleanup
     */
    fclose(stdin);	/* we do not need/use stdin */
    fclose(stdout);	/* we do not need/use stdout */
    signal(SIGPIPE, SIG_IGN);
    if (!use_stderr) {
	dont_use_stderr();
    	fclose(stderr);	/* no messages to stderr */
    }
    if (use_syslog) {
	open_syslog();
    }
    if (optind-argc != 0) {
	if (use_stderr) {
	    fprintf(stderr, usage, program);
	}
	fatal(1, "main", "invalid number of args");
	/*NOTREACHED*/
    }

    /*
     * fork into background if needed
     *
     * We fork here before any debug message just in case the
     * initial syslog daemon connection hangs.
     *
     * We fork here after the initial I/O setup or closed and
     * all parameter checks are done.  While it could die early
     * due to configuration setup or channel failure, at least by
     * this point it won't due due to a trivial command line problem.
     */
    if (fork_mode) {
	pid_t pid;	/* what fork returns */

	/* try to fork */
	pid = fork();

	/* fail if bad fork */
	if (pid < 0) {
	    fatal(2, "main", "fork failed");
	    /*NOTREACHED*/
	}

	/* parent exit so child can go into the background */
	if (pid) {
	    exit(0);
	}
    }

    /*
     * report parameters
     */
    dbg(1, "main", "dbg_lvl: %d", dbg_lvl);
    if (rnd_cfg_name == NULL) {
	dbg(1, "main", "cfg.random comes from compiled in defaults");
    } else {
	dbg(1, "main", "cfg.random: %s", rnd_cfg_name);
    }
    dbg(1, "main", "cfg.lavapool: %s", lava_cfg_name);

    /*
     * configure
     */
    config(rnd_cfg_name, lava_cfg_name);

    /*
     * initialize the pool
     */
    init_pool(cfg_lavapool.poolsize);

    /*
     * initialize the channel index array
     */
    alloc_chanindx();

    /*
     * create the initial chaos channel to gather LavaRnd data
     */
    if (mk_open_chaos() == NULL) {
	fatal(3, "main", "failed to open the initial chaos channel");
	/*NOTREACHED*/
    }

    /*
     * create the initial listener channel to obtain client requests
     */
    if (mk_open_listener() == NULL) {
	fatal(4, "main", "failed to open the initial listener channel");
	/*NOTREACHED*/
    }

    /*
     * time to chroot and drop privileges if possible
     */
    drop_privs(username, chrootdir);

    /*
     * process requests and fill the pool loop
     */
    if (runtime > 0.0) {
	endtime = right_now() + runtime;
    }
    do {

	/*
	 * determine timeout internal for the channel cycle
	 */
	timeout = timeout_value();

	/*
	 * perform a as many channel cycle phases as we can
	 *
	 * NOTE: This call does a:
	 *
	 *	about_now = right_now();
	 */
	chan_cycle(timeout);

    } while (endtime == 0.0 || endtime > about_now);

    /*
     * cleanup - so things like valgrind will not complain about memory leaks
     */
    chan_close();
    lavarnd_cleanup();
    lava_dormant();
    free_cfg_lavapool(&cfg_lavapool);
    free_pool();
    free_allchan();
    if (prog_malloced && prog != NULL) {
	free(prog);
    }
    exit(0);
}


/*
 * config - configure interface and daemon lavapool information
 *
 * given:
 *	rnd_cfg_name	name of cfg.random (or NULL ==> use defaults)
 *	priv_cfgfile	name of cfg.lavapool
 *
 * NOTE: This function exits on configuration failure.
 */
static void
config(char *rnd_cfg_name, char *lava_cfg_name)
{
    int ret;		/* config return value */

    /*
     * parse the cfg.random configuration file
     */
    ret = preload_cfg(rnd_cfg_name);
    if (ret < 0) {
	if (rnd_cfg_name == NULL) {
	    fatal(5, "main", "unable to load default cfg.random file: %d",
		     ret);
	} else {
	    fatal(6, "main", "unable to load cfg.random file: %s: %d",
		     rnd_cfg_name, ret);
	}
	/*NOTREACHED*/
    }
    dbg(2, "config", "parsed cfg.random: %s", rnd_cfg_name);

    /*
     * parse the cfg.lavapool configuration file
     */
    if (config_priv(lava_cfg_name, &cfg_lavapool) < 0) {
	fatal(7, "main",
	      "unable to load cfg.lavapool file: %s", lava_cfg_name);
	/*NOTREACHED*/
    }
    dbg(2, "config", "parsed cfg.lavapool: %s", lava_cfg_name);

    /*
     * configuration firewall
     */
    if (cfg_lavapool.poolsize < cfg_random.maxrequest) {
	fatal(8, "main", "poolsize: %d must be >= maxrequest: %d",
		 cfg_lavapool.poolsize, cfg_random.maxrequest);
	/*NOTREACHED*/
    }
}


/*
 * timeout_value - determine cycle timeout value
 *
 * returns:
 *	seconds to wait, <0 ==> wait for a client request forever
 *
 * NOTE: When the pool is full, we will wait forever for a client request.
 */
static double
timeout_value(void)
{
    double fill_speed;	/* speed, 1.0 ==> fast, 0.0 ==> slow, <0 ==> stop */

    /*
     * determine pool filling speed
     */
    fill_speed = pool_rate_factor();

    /*
     * wait forever for a client request if the pool is full
     */
    if (fill_speed < 0.0) {
	return -1.0;
    }

    /*
     * wait between fast_cycle and low_cycle according to the speed
     */
    return ((fill_speed * cfg_lavapool.fast_cycle) +
	    ((1.0-fill_speed) * cfg_lavapool.slow_cycle));
}


/*
 * drop_privs - chroot and give away our privs if we can
 *
 * given:
 * 	username	if non-NULL, attempt change to this user
 * 	chrootdir	if non-NULL, attempt to chroot to this directory
 *
 * NOTE: This function will exit on any error.
 */
static void
drop_privs(char *username, char *chrootdir)
{
    struct passwd *pwbuf = NULL;	/* username's password entry */
    uid_t orig_uid;			/* original uid */

    /*
     * prepare to setup for username conversion, if requested
     */
    if (username != NULL) {

	/*
	 * lookup the username
	 */
	pwbuf = getpwnam(username);
	if (pwbuf == NULL) {
	    fatal(9, "drop_privs", "user %s is unknown", username);
	    /*NOTREACHED*/
	}
    }

    /*
     * chroot, if requested
     */
    if (chrootdir != NULL) {
	if (chdir(chrootdir) < 0) {
	    fatal(10, "drop_privs", "cannot cd %s", chrootdir);
	    /*NOTREACHED*/
	}
	if (chroot(chrootdir) < 0) {
	    fatal(11, "drop_privs", "cannot chroot %s", chrootdir);
	    /*NOTREACHED*/
	}
	if (chdir("/") < 0) {
	    fatal(12, "drop_privs", "cd / failed after chroot %s", chrootdir);
	    /*NOTREACHED*/
	}
    }

    /*
     * become username if requested
     */
    if (username != NULL && pwbuf != NULL) {

	/*
	 * set the user's group(s)
	 */
	if (initgroups(username, pwbuf->pw_gid) < 0) {
	    fatal(13, "drop_privs", "cannot init groups for user %s", username);
	    /*NOTREACHED*/
	}
	if (setgid(pwbuf->pw_gid) < 0) {
	    fatal(14, "drop_privs", "cannot set group for user %s", username);
	    /*NOTREACHED*/
	}

	/*
	 * set the user's id
	 */
	orig_uid = getuid();
	if (setuid(pwbuf->pw_uid) < 0) {
	    fatal(15, "drop_privs", "cannot set uid for user %s", username);
	    /*NOTREACHED*/
	}
	if (orig_uid == 0 && setuid(0) >= 0) {
	    fatal(16, "drop_privs", "detected setuid kernel bug!", username);
	    /*NOTREACHED*/
	}
    }
    return;
}
