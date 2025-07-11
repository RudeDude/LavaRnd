/*
 * chaos - chaos type channel operations
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: chaos.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "LavaRnd/rawio.h"
#include "LavaRnd/cfg.h"
#include "LavaRnd/lavacam.h"
#include "LavaRnd/lavaquality.h"
#include "LavaRnd/lava_debug.h"

#include "chan.h"
#include "pool.h"
#include "dbg.h"
#include "cfg_lavapool.h"

#if defined(DMALLOC)
#include <dmalloc.h>
#endif


/*
 * The chaos state changes are as follows:
 *
 *	ALLOCED	==> OPEN
 *	CLOSE	==> OPEN
 *
 *	OPEN	==> READ	[read & exception select]
 *	READ	==> READ	[read & exception select]
 *	READ	==> CLOSE	[read & exception select]
 *
 *	CLOSE   ==> ALLOCED
 *
 * in addition to the following state changes:
 *
 *	*	==> HALT
 */


/*
 * chaos_read_mask - if the next state is read selectable
 *
 * 0 ==> do not add to read select mask
 * 1 ==> 	add to read select mask
 *
 * NOTE: The order of this array must match the chanstate type in chan.h!
 */
static int const chaos_read_mask[LAST_STATE+1] = {
    0,	/* ALLOCED */
    0,	/* ACCEPT */
    0,	/* OPEN */
    1,	/* READ */
    0,	/* GATHER */
    0,	/* WRITE */
    0,	/* CLOSE */
    0	/* HALT */
};


/*
 * chaos_write_mask - if the next state is write selectable
 *
 * 0 ==> do not add to write select mask
 * 1 ==> 	add to write select mask
 *
 * NOTE: The order of this array must match the chanstate type in chan.h!
 */
static int const chaos_write_mask[LAST_STATE+1] = {
    0,	/* ALLOCED */
    0,	/* ACCEPT */
    0,	/* OPEN */
    0,	/* READ */
    0,	/* GATHER */
    0,	/* WRITE */
    0,	/* CLOSE */
    0	/* HALT */
};


/*
 * chaos_exception_mask - if the next state is exception selectable
 *
 * 0 ==> do not add to exception select mask
 * 1 ==> 	add to exception select mask
 *
 * NOTE: The order of this array must match the chanstate type in chan.h!
 */
static int const chaos_exception_mask[LAST_STATE+1] = {
    0,	/* ALLOCED */
    0,	/* ACCEPT */
    0,	/* OPEN */
    1,	/* READ */
    0,	/* GATHER */
    0,	/* WRITE */
    0,	/* CLOSE */
    0	/* HALT */
};


/*
 * chaos_preselect_ready - if the next state is auto non-blocking ready
 *
 * 0 ==> not ready for automatic non-blocking operation
 * 1 ==>     ready for automatic non-blocking operation
 *
 * NOTE: The order of this array must match the chanstate type in chan.h!
 */
static int const chaos_preselect_ready[LAST_STATE+1] = {
    0,	/* ALLOCED */
    0,	/* ACCEPT */
    1,	/* OPEN */
    1,	/* READ */
    0,	/* GATHER */
    0,	/* WRITE */
    1,	/* CLOSE */
    0	/* HALT */
};


/*
 * static functions
 */
static void open_chaos(chaos *ch, char *cmd);
static void read_chaos(chaos *ch);
static void chaos_force_close(chaos *ch);
static int willing_to_frame_dump(chaos *ch);
static double time_to_next_dump(chaos *ch);
static int frame_dump_if_ready(chaos *ch);
static int frame_dump(chaos *ch);


/*
 * insane frame count
 */
#define INSANE_FRAME_WARN_CNT (10000)	/* warn after so many insane frames */
#define INSANE_FRAME_FIRST_WARN (10)	/* report this many 1st insane frames */


/*
 * do_chaos_op - perform the channel type specific operation
 *
 * given:
 *	ch	pointer to a chaos channel
 *	cycle	why this operation as selected, in what part of the cycle
 *
 * NOTE: The caller is usually chan_indx_op().
 */
void
do_chaos_op(chaos *ch, chancycle cycle)
{
    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(10, "do_chaos_op", "NULL arg");
	/*NOTREACHED*/
    }
    if (! VALID_STATE(ch->nxtstate)) {
	fatal(10, "do_chaos_op", "chan[%d] invalid state: %d",
		  ch->indx, ch->nxtstate);
	/*NOTREACHED*/
    }

    /*
     * perform operation based on current channel cycle
     */
    switch ((int)cycle) {
    case CYCLE_PRESELECT:
	switch ((int)ch->nxtstate) {
	case OPEN:
	    dbg(4, "do_chaos_op",
	    	   "pre-select open op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    open_chaos(ch, NULL);
	    break;

	case READ:
	    dbg(4, "do_chaos_op",
	    	   "pre-select read op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    if (willing_to_frame_dump(ch)) {
		need_cycle_before(ch->indx, time_to_next_dump(ch));
	    }
	    break;

	case CLOSE:
	    dbg(4, "do_chaos_op",
	    	   "pre-select close op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    close_chaos(ch);
	    break;

	default:
	    warn("do_chaos_op",
	    	 "ignore pre-select op: chan[%d] state: %s ==> %s",
		 ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    break;
	}
	break;

    case CYCLE_SELECTEXECPT:
	if (chaos_read_mask[ch->nxtstate] ||
	    chaos_write_mask[ch->nxtstate] ||
	    chaos_exception_mask[ch->nxtstate]) {
	    dbg(4, "do_chaos_op",
	    	   "select-exception close op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    close_chaos(ch);
	} else {
	    warn("do_chaos_op",
	    	 "ignore select-exception op: chan[%d] state: %s ==> %s",
		 ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	}
	break;

    case CYCLE_SELECTREAD:
	switch ((int)ch->nxtstate) {
	case READ:
	    dbg(4, "do_chaos_op",
	    	   "select-read read op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    read_chaos(ch);
	    break;

	default:
	    warn("do_chaos_op",
	    	 "ignore select-read op: chan[%d] state: %s ==> %s",
		 ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    break;
	}
	break;

    case CYCLE_SELECTWRITE:
	warn("do_chaos_op",
	     "ignore select-write op: chan[%d] state: %s ==> %s",
	     ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	break;

    default:
	warn("do_chaos_op", "op: chan[%d] invalid cycle: %d", ch->indx, cycle);
	break;
    }
    return;
}


/*
 * chaos_select_mask - enable in a select if needed
 *
 * given:
 *	ch	pointer to a chaos channel
 *	rd	pointer to a select read mask or NULL if ignored
 *	wr	pointer to a select write mask or NULL if ignored
 *	ex	pointer to a select exception mask or NULL if ignored
 *
 * returns:
 *	descriptor set, or -1 if nothing was set
 *
 * NOTE: The caller is usually chan_select().
 */
int
chaos_select_mask(chaos *ch, fd_set *rd, fd_set *wr, fd_set *ex)
{
    int ret = -1;		/* descriptor set or -1 */

    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(10, "chaos_select_mask", "NULL arg");
	/*NOTREACHED*/
    }
    if (ch->fd < 0) {
	dbg(4, "chaos_select_mask", "op: chan[%d] state: %s ==> %s not open",
	    ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	return -1;
    }
    if (! VALID_STATE(ch->nxtstate)) {
	fatal(10, "chaos_select_mask", "chan[%d] invalid state: %d",
		  ch->indx, ch->nxtstate);
	/*NOTREACHED*/
    }

    /*
     * set mask as needed
     */
    if (rd != NULL && chaos_read_mask[ch->nxtstate]) {
	FD_SET(ch->fd, rd);
	ret = ch->fd;
	if (dbg_lvl >= 5) {
	    dbg(5, "chaos_select_mask",
		"op: chan[%d]  fd: %d  state: %s ==> %s read mask set",
		ch->indx, ch->fd, STATE_NAME(ch->curstate),
		STATE_NAME(ch->nxtstate));
    	}
    }
    if (wr != NULL && chaos_write_mask[ch->nxtstate]) {
	FD_SET(ch->fd, wr);
	ret = ch->fd;
	if (dbg_lvl >= 5) {
	    dbg(5, "chaos_select_mask",
		"op: chan[%d]  fd: %d  state: %s ==> %s write mask set",
		ch->indx, ch->fd, STATE_NAME(ch->curstate),
		STATE_NAME(ch->nxtstate));
    	}
    }
    if (ex != NULL && chaos_exception_mask[ch->nxtstate]) {
	FD_SET(ch->fd, ex);
	ret = ch->fd;
	if (dbg_lvl >= 5) {
	    dbg(5, "chaos_select_mask",
		"op: chan[%d]  fd: %d  state: %s ==> %s exception mask set",
		ch->indx, ch->fd, STATE_NAME(ch->curstate),
		STATE_NAME(ch->nxtstate));
    	}
    }
    return ret;
}


/*
 * chaos_pre_select_op - perform a pre-select operation if allowed
 *
 * Automatic operations are operations that will not block and
 * do not need select to trigger then.  The channel must be in
 * a state where normal automatic processing is possible.  For example,
 * is cannot be in an ALLOCED or HALT current state.
 *
 * given:
 *	ch	pointer to a chaos channel
 *
 * NOTE: The caller is usually chan_cycle().
 */
void
chaos_pre_select_op(chaos *ch)
{
    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(10, "chaos_pre_select_op", "NULL arg");
	/*NOTREACHED*/
    }
    if (! VALID_STATE(ch->nxtstate)) {
	fatal(10, "chaos_pre_select_op", "chan[%d] invalid state: %d",
		  ch->indx, ch->nxtstate);
	/*NOTREACHED*/
    }

    /*
     * perform an operation if automatic operation allowed and
     * if the pool is not full
     */
    if (chaos_preselect_ready[ch->nxtstate]) {
	if (dbg_lvl >= 4) {
	    dbg(4, "chaos_pre_select_op",
	        "start op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	}
	do_chaos_op(ch, CYCLE_PRESELECT);
	if (dbg_lvl >= 4) {
	    dbg(4, "chaos_pre_select_op",
	        "end op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	}
    }
    return;
}


/*
 * mk_chaos - make a channel a chaos channel
 *
 * given:
 *	ch	chaos channel to make (into ALLOC state)
 */
chan *
mk_chaos(chaos *ch)
{
    int indx;		/* remembered channel index */

    /*
    * firewall
    */
    if (ch == NULL) {
	fatal(10, "mk_chaos", "NULL arg");
	/*NOTREACHED*/
    }

    /*
     * clear values
     */
    indx = ch->indx;
    memset(ch, 0, sizeof(*ch));
    ch->indx = indx;
    ch->fd = -1;
    ch->last_op = -1.0;
    ch->fast_select = 0;
    ch->driver = FALSE;
    ch->driver_type = LAVACAM_ERR_TYPE;
    ch->next_file = 0.0;

    /*
     * force type
     */
    ch->type = TYPE_CHAOS;

    /*
     * force ALLOCED state
     */
    ch->curstate = ALLOCED;
    ch->nxtstate = OPEN;
    dbg(2, "mk_chaos", "op: chan[%d] now chaos channel: state: %s ==> %s",
	ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
    ch->pid = 0;
    ch->driver = FALSE;
    ch->driver_type = LAVACAM_ERR_TYPE;
    return (chan *)ch;
}


/*
 * mk_open_chaos - make a channel an OPEN chaos channel
 *
 * We will form a chaos channel in the OPEN state.  We will use a
 * closed chaos channel, or if none is found, we will create a new
 * chaos channel.
 *
 * returns:
 *	OPEN chaos channel or NULL if error
 *
 * NOTE: This function uses the NULL cmd interface of open_chaos() which
 *	 assumes that cfg_lavapool.chaos contains the command line to use.
 */
chan *
mk_open_chaos(void)
{
    chan *c;			/* channel being opened */

    /*
     * find a channel
     *
     * We will look for a CLOSED chaos channel first.  If we do not
     * find one, we will create a new channel
     */
    c = find_chan(TYPE_CHAOS, CLOSE);
    if (c == NULL) {
	c = find_chan(TYPE_CHAOS, ALLOCED);
	if (c == NULL) {
	    c = mk_chan(TYPE_CHAOS);
	    if (c == NULL) {
		warn("mk_open_chaos", "unable to create a chaos channel");
		return NULL;
	    }
	}
    }

    /*
     * open the channel
     */
    dbg(3, "mk_open_chaos", "chan[%d]: %s ==> %s, force nxtstate: %s",
    	   c->chaos.indx,
	   STATE_NAME(c->chaos.curstate),  STATE_NAME(c->chaos.nxtstate),
	   STATE_NAME(OPEN));
    c->chaos.nxtstate = OPEN;

    open_chaos(&(c->chaos), NULL);
    if (c->chaos.curstate != OPEN) {
	return NULL;
    }
    return c;
}


/*
 * open_chaos - open an chaos channel
 *
 * The channel will either open a driver or it will form
 * a pipe to a forked chaos process.  A driver is where the
 * first arg is :direct.  Anything else is a chaos command.
 *
 * If we are opening a driver, then we will obtain a file
 * descriptor on which to read and issue ioctls.
 *
 * If we are opening up a command to a forked process, we
 * will have the only read side of the pipe open.  The write
 * side of the pipe on the client side will be attached to its stdout.
 *
 * This function does nothing if the channel is HALTed.
 *
 * given:
 *	ch 	chaos channel
 *	cmd	args for command or driver
 *		  or NULL ==> use cfg_lavapool.chaos
 */
static void
open_chaos(chaos *ch, char *cmd)
{
    char *arg[MAX_ARG_CNT+1];	/* args from chaos string */
    char **argv;		/* working pointer into arg[] array */
    int argc;			/* arg count found on the command line */
    char *cmdline;		/* clone of cmd */
    int pipefd[2];		/* pipe descriptors */
    pid_t pid;			/* forked pid or 0 or error */
    char *driver_name = NULL;	/* name of driver used */
    char *device_name = NULL;	/* name of driver device */
    char *p;
    int i;

    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(10, "open_chaos", "NULL ch arg");
	/*NOTREACHED*/
    }
    if (cmd == NULL) {
	cmd = cfg_lavapool.chaos;
    }
    if (cmd == NULL) {
	fatal(10, "open_chaos", "NULL cmd arg and NULL chaos");
	/*NOTREACHED*/
    }
    if (ch->curstate == HALT || ch->nxtstate == HALT) {
	warn("open_chaos", "chan[%d] is/will HALT, cannot open", ch->indx);
	return;
    }
    if (ch->nxtstate != OPEN) {
	warn("open_chaos", "chan[%d]: %s ==> %s, nxtstate != %s",
	     ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate),
	     STATE_NAME(OPEN));
	return;
    }

    /*
     * tokenize command string
     */
    cmdline = strdup(cmd);
    if (cmdline == NULL) {
	warn("open_chaos", "unable to duplicate command line");
	return;
    }
    for (argc=0, p=strtok(cmdline, " \t\n");
	 argc < MAX_ARG_CNT && p != NULL; ++argc, p = strtok(NULL, " \t\n")) {
	 arg[argc] = p;
    }
    if (argc >= MAX_ARG_CNT && p != NULL) {
	warn("open_chaos", "command line has too many args");
	free(cmdline);
	return;
    }
    arg[argc] = NULL;
    argv = &arg[0];

    /*
     * using a driver
     */
    if (strcmp(arg[0], ":driver") == 0) {
	int arg_shift;			/* argc/argv parse shift value */
	union lavacam o_cam;		/* open camera state */
	union lavacam n_cam;		/* camera new state */
	struct lavacam_flag flag;	/* flags set via lavacam_argv() */
	struct opsize siz;		/* operation size & buffer structure */

	/*
	 * obtain the driver type
	 */
	driver_name = argv[1];
	device_name = argv[2];
	ch->driver_type = camtype(driver_name);
	if (ch->driver_type < 0) {
	    warn("open_chaos", "unknown driver name: %s", driver_name);
	    free(cmdline);
	    return;
	}

	/*
	 * process the driver args
	 */
	argc -= 2;
	argv += 2;
	argv[0] = arg[0];
	arg_shift = lavacam_argv(ch->driver_type, argc, argv, &n_cam, &flag);
	if (arg_shift < 0) {
	    warn("open_chaos", "invalid driver command line: %s", cmd);
	    warn("open_chaos", "lavacam_argv error code: %d", arg_shift);
	    warn("open_chaos", "lavacam_argv driver type: %d",
		 ch->driver_type);
	    free(cmdline);
	    return;
	}
	argc -= arg_shift;
	argv += arg_shift;
	if (argc > 1) {
	    warn("open_chaos", "unexpended command line args: %s", cmd);
	    warn("open_chaos", "first unexpended arg: %s", argv[0]);
	    free(cmdline);
	    return;
	}

	/*
	 * open the driver
	 */
	if (flag.T_flag > 0.0) {
	    dbg(1, "open_chaos", "will spend %.2f seconds opening %s",
		    flag.T_flag, device_name);
	} else {
	    dbg(1, "open_chaos", "about to open: %s", device_name);
	}
	ch->fd = lavacam_open(ch->driver_type, device_name, &o_cam, &n_cam,
			      &siz, 0, &flag);
	if (ch->fd < 0) {
	    warn("open_chaos", "unable to open %s camera on %s: %d",
		 driver_name, device_name, ch->fd);
	    free(cmdline);
	    return;
	}

	/*
	 * driver opens can take a long time, so get an updated now time
	 */
	about_now = right_now();

	/*
	 * save open information in the chaos structure
	 */
	ch->pid = 0;
	ch->driver = TRUE;
	memcpy((void *)&ch->cam, (void *)&n_cam, sizeof(ch->cam));
	memcpy((void *)&ch->siz, (void *)&siz, sizeof(ch->siz));
	memcpy((void *)&ch->flag, (void *)&flag, sizeof(ch->flag));

	/*
	 * set next savefile time
	 */
	if (willing_to_frame_dump(ch)) {
	    ch->next_file = about_now + ch->flag.interval;
	}

    /*
     * using a co-process / command
     */
    } else {

	/*
	 * create a pipe
	 */
	if (pipe(pipefd) < 0) {
	    warn("open_chaos", "unable to create a pipe: %s", strerror(errno));
	    free(cmdline);
	    return;
	}

	/*
	 * make the pipe non-blocking
	 */
	if (noblock_fd(pipefd[0]) < 0 || noblock_fd(pipefd[1]) < 0) {
	    warn("open_chaos", "unable non-block pipe: %s", strerror(errno));
	    close(pipefd[0]);
	    close(pipefd[1]);
	    free(cmdline);
	    return;
	}

	/*
	 * fork for a chaos process
	 */
	fflush(stdout);
	fflush(stderr);
	pid = fork();
	if (pid < 0) {
	    warn("open_chaos", "unable to fork a process: %s",
		 strerror(errno));
	    close(pipefd[0]);
	    close(pipefd[1]);
	    free(cmdline);
	    return;

	/*
	 * child process
	 */
	} else if (pid == 0) {

	    /*
	     * close down all descriptors except the write side of the pipe
	     */
	    for (i=0; i < chanindx_len; ++i) {
		if (i != pipefd[1]) {
		    (void) close(i);
		}
	    }

	    /*
	     * move the write side of the pipe into stdout
	     */
	    errno = 0;
	    if (dup2(pipefd[1], 1) < 0) {
		/* silently exit since we are not the parent process */
		exit(errno);
	    }

	    /*
	     * exec the command
	     */
	    (void) execv(arg[0], argv);
	    /* silently exit since we are not the parent process */
	    exit(errno);

	    /*
	     * end of child process code
	     */
	}

	/*
	 * parent process code
	 */
	(void) close(pipefd[1]);
	ch->fd = pipefd[0];
	ch->driver_type = LAVACAM_ERR_TYPE;
	ch->driver = FALSE;
    }

    /*
     * place the information into the channel
     */
    set_chanindx(ch->indx, ch->fd);
    ch->count = 0;
    ch->last_op = about_now;
    dbg(3, "open_chaos", "chan[%d]: state was %s ==> %s, now %s ==> %s",
			 ch->indx, STATE_NAME(ch->curstate),
	STATE_NAME(ch->nxtstate), STATE_NAME(OPEN), STATE_NAME(READ));
    if (ch->driver && driver_name != NULL && device_name != NULL) {
	dbg(3, "open_chaos", "chan[%d]: using driver: %s (%d) on %s",
	       ch->indx, driver_name, ch->driver_type, device_name);
    } else {
	dbg(3, "open_chaos", "chan[%d]: using command: %s", ch->indx, cmd);
    }
    ch->curstate = OPEN;
    ch->nxtstate = READ;
    free(cmdline);
    return;
}


/*
 * read_chaos - read a chaos channel
 *
 * We will attempt to fill the pool form the channel's open descriptor
 * (via fill_pool() via read_once()).
 *
 * On success the channel is left in a READ state.
 *
 * This function does nothing if the channel is not a TYPE_CHAOS type
 * and the current or next state is not READ.
 *
 * This function does nothing if the channel is HALTed.
 *
 * given:
 *	ch 	chaos channel
 *
 * NOTE: It is assumed that the caller has used select() (perhaps via
 *	 the chan_select() function) to determine that the descriptor
 *	 is readable, otherwise this call may block.
 */
static void
read_chaos(chaos *ch)
{
    int ret;		/* system call return */
    int skip_frame;	/* TRUE ==> do not LavaRnd process this frame */
    int sanity;		/* <0 ==> frame is insane */
    int half_x;		/* computed 1/2 value level */

    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(11, "read_chaos", "NULL arg");
	/*NOTREACHED*/
    }
    if (ch->type != TYPE_CHAOS) {
	warn("read_chaos", "expected type %d, found: %d",
			    TYPE_CHAOS, ch->type);
	return;
    }
    if (ch->curstate == HALT || ch->nxtstate == HALT) {
	warn("read_chaos", "chan[%d] is/will HALT, cannot read", ch->indx);
	return;
    }
    if ((int)ch->nxtstate != READ) {
	warn("read_chaos", "chan[%d] next state: %s != %s",
	     ch->indx, STATE_NAME(ch->nxtstate), STATE_NAME(READ));
	return;
    }

    /*
     * fill the pool via chaos driver buffer
     */
    if (ch->driver == TRUE) {

	/*
	 * get the next frame from the driver
	 */
	dbg(4, "read_chaos", "chan[%d]: read frame from driver", ch->indx);
	ret = lavacam_get_frame(ch->driver_type, ch->fd, &ch->siz);
	if (ret < 0) {
	    dbg(2, "read_chaos",
		"chan[%d]: lavacam_get_frame error: %d", ch->indx, ret);
	    chaos_force_close(ch);
	    return;
	}

	/*
	 * frame firewall
	 */
	if (ch->siz.chaos == NULL) {
	    fatal(12, "read_chaos", "chan[%d]: NULL channel chaos buffer",
		      ch->indx);
	    /*NOTREACHED*/
	}
	if (ch->siz.chaos_len < 0) {
	    fatal(13, "read_chaos",
		      "chan[%d]: neg channel chaos buffer len: %d",
		      ch->indx, ch->siz.chaos_len);
	    /*NOTREACHED*/
	}

	/*
	 * frame sanity check
	 */
	sanity = lavacam_sanity(&ch->siz);
	if (sanity < 0) {

	    /* skip this insane frame */
	    skip_frame = TRUE;

	    /*
	     * we will warn for the first few insane frames, and
	     * then report every so many frames
	     */
	    if (ch->siz.insane_cnt <= INSANE_FRAME_FIRST_WARN ||
	        (ch->siz.insane_cnt % INSANE_FRAME_WARN_CNT) == 0) {
		if (ch->siz.insane_cnt <= INSANE_FRAME_FIRST_WARN) {
		    warn("read_chaos",
		         "chan[%d]: %s: frame: %lld insane frame cnt: %lld: %s",
			 ch->indx,
			 ((ch->siz.insane_cnt <= INSANE_FRAME_FIRST_WARN) ?
			  "repting initial insanity" :
			  "repting every so often"),
			 ch->siz.frame_num, ch->siz.insane_cnt,
			 lava_err_name(sanity));
		}
		warn("read_chaos", "uncom_fract: %f",
		     lavacam_uncom_fract(ch->siz.chaos,
					 ch->siz.chaos_len, ch->siz.top_x,
					 &half_x));
		warn("read_chaos", "half_x: %d bitdiff_fract: %f",
		     half_x,
		     lavacam_bitdiff_fract(ch->siz.prev_frame,
					   ch->siz.chaos, ch->siz.chaos_len));
	    	warn("read_chaos",
		     "configured levels: half_x: %d top_x: %d "
		     "bitdiff_fract: %f uncom_fract: %f",
		     ch->siz.half_x, ch->siz.top_x, ch->siz.min_fract,
		     ch->siz.diff_fract);

	    /*
	     * if we are not warning, but we are debugging, then
	     * we will issue the same insane frame reports as a debug message
	     */
	    } else if (dbg_lvl > 1) {
		dbg(2, "read_chaos",
		       "chan[%d]: frame %lld insane frame cnt: %lld: %s",
		       ch->indx,
		       ch->siz.frame_num, ch->siz.insane_cnt,
		       lava_err_name(sanity));
		dbg(2, "read_chaos",
		       "uncom_fract: %f",
		       lavacam_uncom_fract(ch->siz.chaos,
					   ch->siz.chaos_len, ch->siz.top_x,
					   &half_x));
		dbg(2, "read_chaos",
		       "half_x: %d bitdiff_fract: %f",
		       half_x,
		       lavacam_bitdiff_fract(ch->siz.prev_frame,
					     ch->siz.chaos, ch->siz.chaos_len));
	    	dbg(3, "read_chaos",
		       "min levels: half_x: %d top_x: %d bitdiff_fract: %f "
		       "uncom_fract: %f",
		       ch->siz.half_x, ch->siz.top_x, ch->siz.min_fract,
		       ch->siz.diff_fract);
	    }

	/*
	 * frame is not insane, process it
	 */
	} else {

	    /*
	     * savefile frame dump if we are willing and ready
	     */
	    skip_frame = frame_dump_if_ready(ch);

	    /*
	     * case: LavaRnd process a buffer of chaos data and fill with data
	     */
	    if (!skip_frame) {
		dbg(2, "read_chaos",
		       "chan[%d]: chaos driver buf: pool level: %u",
		       ch->indx, pool_level());
		ret = fill_pool_from_chaos(ch->siz.chaos, ch->siz.chaos_len);

		if (ret < 0) {
		    dbg(2, "read_chaos",
			   "chan[%d]: fill_pool_from_chaos_buf error: %d",
			   ch->indx, ret);
		    chaos_force_close(ch);
		    return;
		}
	    }
	}

	/*
	 * release the driver frame
	 */
	dbg(5, "read_chaos", "chan[%d]: release frame", ch->indx);
	ret = lavacam_msync(ch->driver_type, ch->fd, &ch->cam, &ch->siz);
	if (ret < 0) {
	    dbg(2, "read_chaos",
		"chan[%d]: lavacam_msync error: %d", ch->indx, ret);
	    chaos_force_close(ch);
	    return;
	}

    /*
     * fill the pool via pipe to co-process
     */
    } else {
	dbg(2, "read_chaos", "chan[%d]: pipe read: pool level: %u",
	       ch->indx, pool_level());
	ret = fill_pool_from_fd(ch->fd);
	if (ret < 0) {
	    dbg(2, "read_chaos",
		   "chan[%d]: fill_pool_from_fd error: %d", ch->indx, ret);
	    chaos_force_close(ch);
	    return;
	}
    }

    /*
     * change current and next state as needed
     */
    if (ch->curstate != READ) {
	dbg(3, "open_chaos", "chan[%d]: state was %s ==> %s, now %s ==> %s",
			     ch->indx, STATE_NAME(ch->curstate),
	    STATE_NAME(ch->nxtstate), STATE_NAME(READ), STATE_NAME(READ));
	ch->curstate = READ;
	ch->nxtstate = READ;
    }

    /*
     * accounting
     */
    ch->last_op = about_now;
    ch->count += (u_int64_t)ret;
    dbg(4, "read_chaos", "chan[%d]: ret: %d count: %lld, pool level: %u",
    			 ch->indx, ret, ch->count, pool_level());
    return;
}


/*
 * close_chaos - close a chaos channel
 *
 * We will ensure that a chaos channel is closed.  If the descriptor is
 * open, we will attempt to close it.  If the channel as a child process,
 * we will attempt to SIGTERM it.
 *
 * On success the channel is left in a CLOSE state.
 *
 * This function does nothing if the channel is not a TYPE_CHAOS type.
 * This function does nothing if the channel is HALTed.
 *
 * given:
 *	ch 	chaos channel
 */
void
close_chaos(chaos *ch)
{
    int ret;		/* system call return */
    int i;

    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(11, "close_chaos", "NULL arg");
	/*NOTREACHED*/
    }
    if (ch->type != TYPE_CHAOS) {
	warn("close_chaos", "expected type %d, found: %d",
			    TYPE_CHAOS, ch->type);
	return;
    }
    if (ch->curstate == HALT || ch->nxtstate == HALT) {
	warn("close_chaos", "chan[%d] is/will HALT, cannot close", ch->indx);
	return;
    }

    /*
     * device cleanup
     */
    if (ch->driver) {

	/*
	 * close down driver interface
	 */
	i = lavacam_close(ch->driver_type, ch->fd, &ch->siz, &ch->flag);
	if (i < 0) {
	    dbg(3, "close_chaos", "chan[%d]: failed to driver close fd %d: %d",
		   ch->indx, ch->fd, i);
	}

	/*
	 * cleanup chaos structure
	 */
	clear_chanindx(ch->indx, ch->fd);
	ch->fd = -1;
	ch->pid = 0;
	ch->driver = FALSE;
	ch->driver_type = -1;
	memset((void *)&ch->cam, 0, sizeof(ch->cam));
	memset((void *)&ch->siz, 0, sizeof(ch->siz));
	ch->siz.image = NULL;
	ch->siz.chaos = NULL;

	memset((void *)&ch->flag, 0, sizeof(ch->flag));

    /*
     * co-process cleanup
     */
    } else {

	/*
	 * close descriptor
	 */
	if (ch->fd >= 0) {
	    errno = 0;
	    ret = close(ch->fd);
	    if (ret < 0) {
		dbg(3, "close_chaos", "chan[%d]: failed to close fd %d: %s",
		       ch->indx, ch->fd, strerror(errno));
	    } else {
		dbg(4, "close_chaos", "chan[%d]: closed fd %d: %s",
		       ch->indx, ch->fd);
	    }
	    clear_chanindx(ch->indx, ch->fd);
	    ch->fd = -1;
	}

	/*
	 * kill child if using a co-process command
	 */
	if (ch->pid > 0) {
	    errno = 0;
	    ret =  kill(ch->pid, SIGTERM);
	    if (ret < 0) {
		dbg(3, "close_chaos", "chan[%d]: could not kill %d: %s",
		       ch->indx, (int)(ch->pid), strerror(errno));
	    } else {
		dbg(4, "close_chaos", "chan[%d]: killed %d: %s",
		       ch->indx, (int)ch->pid);
	    }
	    ch->pid = 0;
	}
    }

    /*
     * set state
     */
    dbg(3, "close_chaos", "chan[%d]: state was %s ==> %s, now %s ==> %s",
			 ch->indx, STATE_NAME(ch->curstate),
	STATE_NAME(ch->nxtstate), STATE_NAME(CLOSE), STATE_NAME(ALLOCED));
    ch->last_op = about_now;
    ch->curstate = CLOSE;
    ch->nxtstate = ALLOCED;
    return;
}


/*
 * chaos_force_close - force close of a non-HALTed chaos channel
 *
 * We will ensure that a chaos channel is closed.  If the descriptor is
 * open, we will attempt to close it.
 *
 * On success the channel is left in a CLOSE state.
 *
 * This function does nothing if the channel is not a TYPE_CHAOS type.
 *
 * given:
 *	ch 	chaos channel
 */
static void
chaos_force_close(chaos *ch)
{
    /* announce */
    dbg(2, "chaos_force_close", "chan[%d]: will be forced to CLOSE", ch->indx);

    /* close */
    close_chaos(ch);
    return;
}


/*
 * willing_to_frame_dump - determine if we frame dump at all
 *
 * given:
 *	ch 	chaos channel
 *
 * returns:
 * 	TRUE ==> willing to dump a frame to disk
 * 	FALSE ==> not interested in frame dumping
 */
static int
willing_to_frame_dump(chaos *ch)
{
    /*
     * We are willing to dump frames if:
     *
     *	1) We have a safefile filename
     *	2) We have a newfile filename
     *	3) We have a reasonable frame dump interval
     */
    if (ch->flag.savefile != NULL && ch->flag.newfile != NULL &&
	ch->flag.interval > 0.0) {
	return TRUE;
    }
    return FALSE;
}


/*
 * time_to_next_dump - return time to next frame dump of a willing channel
 *
 * given:
 *	ch 	chaos channel willing to frame dump
 *
 * returns:
 * 	>= 0.0 ==> time to next frame dump, < 0.0 ==> no frame dump scheduled
 *
 * NOTE: The caller must call willing_to_frame_dump() and receive a TRUE
 * 	 responce before calling this function.
 */
static double
time_to_next_dump(chaos *ch)
{
    /*
     * case: no schedled frame dump
     */
    if (ch->next_file <= 0.0) {
	/* no dump scheduled */
	dbg(5, "time_to_next_dump", "no schedled frame dump");
	return -1.0;
    }

    /*
     * case: return the next frame dump time
     */
    dbg(4, "time_to_next_dump", "next frame dump time: %.3f", ch->next_file);
    return ch->next_file;
}


/*
 * ready_to_frame_dump - determine if we are ready and willing to frame dump
 *
 * given:
 *	ch 	chaos channel
 *
 * returns:
 * 	TRUE ==> ready and willing to dump a frame to disk
 * 	FALSE ==> either not ready or not interested in frame dumping
 */
int
ready_to_frame_dump(chaos *ch)
{
    /*
     * We are ready to dump if:
     *
     * 	1) We are willing to dump frames at all
     * 	2) Enough time has passed since the last dump (or channel open)
     * 	3) We are in the correct state.
     */
    if (willing_to_frame_dump(ch) && ch->next_file < about_now &&
	(chaos_read_mask[ch->curstate] || chaos_read_mask[ch->nxtstate])) {
	return TRUE;
    }
    return FALSE;
}


/*
 * frame_dump_if_ready - dump a frame to a savefile if we ready and willing
 *
 * given:
 *	ch 	chaos channel
 *
 * returns:
 * 	TRUE ==> some new frame data was written to disk
 * 	FALSE ==> no new frame data was written to disk
 */
static int
frame_dump_if_ready(chaos *ch)
{
    int skip_frame;	/* TRUE ==> do not LavaRnd process this frame */

    /*
     * case: savefile frame dump
     *
     * If forming savefiles and we are due, write frame instead
     * of LavaRnd processing.  It is critical that we do NOT
     * LavaRnd process any frame that we write out on disk.
     */
    skip_frame = FALSE;
    if (ready_to_frame_dump(ch)) {

	/* frame dump */
	skip_frame = frame_dump(ch);
	if (skip_frame) {
	    /* dumped a frame & quickly need another for lavapool use */
	    ch->fast_select = TRUE;
	}

	/*
	 * Do not attempt to dump another frame right away, even
	 * if we were not successful (or -E and frame is non-empty).
	 * We do not want to be constantly retrying the frame dump.
	 */
	ch->next_file = about_now + ch->flag.interval;
    }
    return skip_frame;
}


/*
 * frame_dump - dump a frame to a savefile
 *
 * given:
 *	ch 	chaos channel
 *
 * returns:
 * 	TRUE ==> some new frame data was written to disk
 * 	FALSE ==> no new frame data was written to disk,
 * 		  OK to LavaRnd process this frame
 */
static int
frame_dump(chaos *ch)
{
    int framefd;	/* newfile file descriptor */
    int write_ret;	/* return from raw_write() */
    struct stat buf;	/* savefile stat for -E checking */

    /*
     * firewall
     */
    if (ch->flag.newfile == NULL || ch->flag.savefile == NULL) {
	warn("frame_dump", "NULL newfile or savefile string");
	return FALSE;
    }

    /*
     * If -E was given, do nothing if the savefile is non-empty
     */
    if (ch->flag.E_flag &&stat(ch->flag.savefile, &buf) >= 0 &&
	    buf.st_size > 0) {
	dbg(4, "frame_dump", "savefile %s exists", ch->flag.savefile);
	return FALSE;
    }

    /*
     * open/create the newfile
     *
     * We form the newfile, even if -E was given.  Sure, there is a "race"
     * condition between the check to see savefile exists above and
     * the same dump below.  This is OK because we just rename the newfile
     * in place of any savefile that was created during "race" window.
     */
    errno = 0;
    framefd = open(ch->flag.newfile,
		   O_CREAT|O_EXCL|O_WRONLY|O_TRUNC,
		   S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
    if (framefd < 0 && errno == EEXIST) {

	/*
	 * newfile was left around, remove it and retry the open
	 */
	if (unlink(ch->flag.newfile) < 0) {
	    warn("frame_dump", "found newfile: %s, cannot remove it: %s",
		  ch->flag.newfile, strerror(errno));
	    return FALSE;
	} else {
	    warn("frame_dump", "removed previous newfile: %s",
		   ch->flag.newfile);
	    errno = 0;
	    framefd = open(ch->flag.newfile,
			   O_CREAT|O_EXCL|O_WRONLY|O_TRUNC,
			   S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
	}
    }
    if (framefd < 0) {

	/*
	 * failed to open frame, so LavaRnd process it instead
	 */
	warn("frame_dump", "failed to open/create %s: %s",
	     ch->flag.newfile, strerror(errno));
	return FALSE;

    /*
     * dump frame to savefile
     */
    } else {

	/*
	 * write frame to newfile
	 */
	write_ret =
	  raw_write(framefd, ch->siz.chaos, ch->siz.chaos_len, FALSE);

	/*
	 * case: raw_write error
	 */
	if (write_ret < 0) {
	    /*
	     * Even though the raw_write failed, some data might
	     * have been written.  We will not LavaRnd process
	     * this frame in case that happened.
	     */
	    warn("frame_dump", "bad frame wrote to %s: %s",
		 ch->flag.newfile, write_ret);
	    /* try to remove the newfile due to the error */
	    (void) close(framefd);
	    errno = 0;
	    if (unlink(ch->flag.newfile) < 0) {
		warn("frame_dump", "unable to remove %s: %s",
		     ch->flag.newfile, strerror(errno));
	    }

	/*
	 * case: partial write
	 */
	} else if (write_ret != ch->siz.chaos_len) {

	    /*
	     * Even though all data was not written, some data might
	     * have been written.  We will not LavaRnd process
	     * this frame in case that happened.
	     */
	    warn("frame_dump", "wrote %d instead of %d octets to %s",
		 ch->flag.newfile, write_ret, ch->siz.chaos_len);
	    /* try to remove the newfile due to the error */
	    (void) close(framefd);
	    errno = 0;
	    if (unlink(ch->flag.newfile) < 0) {
		warn("frame_dump", "cannot to remove %s: %s",
		     ch->flag.newfile, strerror(errno));
	    }

	/*
	 * move newfile to savefile
	 */
	} else {
	    /* chmod 0664 just to be sure */
	    (void) fchmod(framefd, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
	    (void) close(framefd);
	    errno = 0;
	    if (rename(ch->flag.newfile, ch->flag.savefile) < 0) {
		warn("frame_dump", "cannot mv %s %s: %s",
		     ch->flag.newfile, ch->flag.savefile, strerror(errno));
	    } else {
		dbg(2, "frame_dump", "chan[%d]: frame dump to %s",
		       ch->indx, ch->flag.savefile);
	    }
	}
    }
    return TRUE;
}
