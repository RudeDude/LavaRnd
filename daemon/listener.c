/*
 * listener - listen type channel operations
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: listener.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "LavaRnd/rawio.h"
#include "LavaRnd/cfg.h"

#include "chan.h"
#include "dbg.h"
#include "cfg_lavapool.h"

#if defined(DMALLOC)
#  include <dmalloc.h>
#endif


/*
 * The listen state changes are as follows:
 *
 *      ALLOCED ==> OPEN
 *      CLOSE   ==> OPEN
 *
 *      OPEN    ==> ACCEPT
 *      OPEN    ==> CLOSE
 *
 *      ACCEPT  ==> ACCEPT      [read & exception select]
 *      ACCEPT  ==> CLOSE       [read & exception select]
 *
 *      CLOSE   ==> ALLOCED
 *
 * in addition to the following state changes:
 *
 *      *       ==> HALT
 */


/*
 * listener_read_mask - if the next state is read selectable
 *
 * 0 ==> do not add to read select mask
 * 1 ==>        add to read select mask
 *
 * NOTE: The order of this array must match the chanstate type in chan.h!
 */
int const listener_read_mask[LAST_STATE + 1] = {
    0,				/* ALLOCED */
    1,				/* ACCEPT */
    0,				/* OPEN */
    0,				/* READ */
    0,				/* GATHER */
    0,				/* WRITE */
    0,				/* CLOSE */
    0				/* HALT */
};


/*
 * listener_write_mask - if the next state is write selectable
 *
 * 0 ==> do not add to write select mask
 * 1 ==>        add to write select mask
 *
 * NOTE: The order of this array must match the chanstate type in chan.h!
 */
int const listener_write_mask[LAST_STATE + 1] = {
    0,				/* ALLOCED */
    0,				/* ACCEPT */
    0,				/* OPEN */
    0,				/* READ */
    0,				/* GATHER */
    0,				/* WRITE */
    0,				/* CLOSE */
    0				/* HALT */
};


/*
 * listener_exception_mask - if the next state is exception selectable
 *
 * 0 ==> do not add to exception select mask
 * 1 ==>        add to exception select mask
 *
 * NOTE: The order of this array must match the chanstate type in chan.h!
 */
int const listener_exception_mask[LAST_STATE + 1] = {
    0,				/* ALLOCED */
    1,				/* ACCEPT */
    0,				/* OPEN */
    0,				/* READ */
    0,				/* GATHER */
    0,				/* WRITE */
    0,				/* CLOSE */
    0				/* HALT */
};


/*
 * listener_preselect_ready - if the next state is auto non-blocking ready
 *
 * 0 ==> not ready for automatic non-blocking operation
 * 1 ==>     ready for automatic non-blocking operation
 *
 * NOTE: The order of this array must match the chanstate type in chan.h!
 */
int const listener_preselect_ready[LAST_STATE + 1] = {
    0,				/* ALLOCED */
    0,				/* ACCEPT */
    1,				/* OPEN */
    0,				/* READ */
    0,				/* GATHER */
    0,				/* WRITE */
    1,				/* CLOSE */
    0				/* HALT */
};


/*
 * static functions
 */
static void open_listener(listener *ch, char *cmd);


/*
 * do_listener_op - perform the channel type specific operation
 *
 * given:
 *      ch      pointer to a channel
 *      cycle   why this operation as selected, in what part of the cycle
 *
 * NOTE: The caller is usually chan_indx_op().
 */
void
do_listener_op(listener *ch, chancycle cycle)
{
    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(10, "do_listener_op", "NULL arg");
	/*NOTREACHED*/
    }
    if (!VALID_STATE(ch->nxtstate)) {
	fatal(10, "do_listener_op", "chan[%d] invalid state: %d",
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
	    dbg(4, "do_listener_op",
		"pre-select open op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    open_listener(ch, NULL);
	    break;

	case CLOSE:
	    dbg(4, "do_listener_op",
		"pre-select close op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    close_listener(ch);
	    break;

	default:
	    warn("do_listener_op",
		 "ignore pre-select op: chan[%d] state: %s ==> %s",
		 ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    break;
	}
	break;

    case CYCLE_SELECTEXECPT:
	if (listener_read_mask[ch->nxtstate] ||
	    listener_write_mask[ch->nxtstate] ||
	    listener_exception_mask[ch->nxtstate]) {
	    dbg(4, "do_listener_op",
		"select-exception close op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    close_listener(ch);
	} else {
	    warn("do_listener_op",
		 "ignore select-exception op: chan[%d] state: %s ==> %s",
		 ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	}
	break;

    case CYCLE_SELECTREAD:
	switch ((int)ch->nxtstate) {
	case ACCEPT:
	    dbg(4, "do_listener_op",
		"select-read op: read op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    accept_listener(ch);
	    break;

	default:
	    warn("do_listener_op",
		 "ignore select-read op: chan[%d] state: %s ==> %s",
		 ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    break;
	}
	break;

    case CYCLE_SELECTWRITE:
	warn("do_listener_op",
	     "ignore select-write op: chan[%d] state: %s ==> %s",
	     ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	break;

    default:
	warn("do_listener_op",
	     "op: chan[%d] invalid cycle: %d", ch->indx, cycle);
	break;
    }
    return;
}


/*
 * listener_select_mask - enable in a select if needed
 *
 * given:
 *      ch      pointer to a listener channel
 *      rd      pointer to a select read mask or NULL if ignored
 *      wr      pointer to a select write mask or NULL if ignored
 *      ex      pointer to a select exception mask or NULL if ignored
 *
 * returns:
 *      descriptor set, or -1 if nothing was set
 *
 * NOTE: The caller is usually chan_select().
 */
int
listener_select_mask(listener *ch, fd_set * rd, fd_set * wr, fd_set * ex)
{
    int ret = -1;	/* descriptor set or -1 */

    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(10, "listener_select_mask", "NULL arg");
	/*NOTREACHED*/
    }
    if (ch->fd < 0) {
	dbg(4, "listener_select_mask",
	    "op: chan[%d] state: %s ==> %s not open",
	    ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	return -1;
    }
    if (!VALID_STATE(ch->nxtstate)) {
	fatal(10, "listener_select_mask", "chan[%d] invalid state: %d",
	      ch->indx, ch->nxtstate);
        /*NOTREACHED*/
    }

    /*
     * set mask as needed
     */
    if (rd != NULL && listener_read_mask[ch->nxtstate]) {
	FD_SET(ch->fd, rd);
	ret = ch->fd;
	if (dbg_lvl >= 5) {
	    dbg(5, "listener_select_mask",
		"op: chan[%d]  fd: %d  state: %s ==> %s read mask set",
		ch->indx, ch->fd, STATE_NAME(ch->curstate),
		STATE_NAME(ch->nxtstate));
	}
    }
    if (wr != NULL && listener_write_mask[ch->nxtstate]) {
	FD_SET(ch->fd, wr);
	ret = ch->fd;
	if (dbg_lvl >= 5) {
	    dbg(5, "listener_select_mask",
		"op: chan[%d]  fd: %d  state: %s ==> %s write mask set",
		ch->indx, ch->fd, STATE_NAME(ch->curstate),
		STATE_NAME(ch->nxtstate));
	}
    }
    if (ex != NULL && listener_exception_mask[ch->nxtstate]) {
	FD_SET(ch->fd, ex);
	ret = ch->fd;
	if (dbg_lvl >= 5) {
	    dbg(5, "listener_select_mask",
		"op: chan[%d]  fd: %d  state: %s ==> %s exception mask set",
		ch->indx, ch->fd, STATE_NAME(ch->curstate),
		STATE_NAME(ch->nxtstate));
	}
    }
    return ret;
}


/*
 * listener_pre_select_op - perform a pre-select operation if allowed
 *
 * Automatic operations are operations that will not block and
 * do not need select to trigger then.  The channel must be in
 * a state where normal automatic processing is possible.  For example,
 * is cannot be in an ALLOCED or HALT current state.
 *
 * given:
 *      ch      pointer to a listener channel
 *
 * NOTE: The caller is usually chan_cycle().
 */
void
listener_pre_select_op(listener *ch)
{
    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(10, "listener_pre_select_op", "NULL arg");
	/*NOTREACHED*/
    }
    if (!VALID_STATE(ch->nxtstate)) {
	fatal(10, "listener_pre_select_op", "chan[%d] invalid state: %d",
	      ch->indx, ch->nxtstate);
        /*NOTREACHED*/
    }

    /*
     * perform an operation if automatic operation allowed
     */
    if (listener_preselect_ready[ch->nxtstate]) {
	if (dbg_lvl >= 4) {
	    dbg(4, "listener_pre_select_op",
		"start op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	}
	do_listener_op(ch, CYCLE_PRESELECT);
	if (dbg_lvl >= 4) {
	    dbg(4, "listener_pre_select_op",
		"end op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	}
    }
    return;
}


/*
 * mk_listener - make a channel a listener channel
 */
chan *
mk_listener(listener *ch)
{
    int indx;	/* remembered channel index */

    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(10, "mk_listener", "NULL arg");
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

    /*
     * force type
     */
    ch->type = TYPE_LISTENER;

    /*
     * force ALLOCED state
     */
    ch->curstate = ALLOCED;
    ch->nxtstate = OPEN;
    dbg(2, "mk_listener",
	"op: chan[%d] now listener channel: state: %s ==> %s", ch->indx,
	STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
    return (chan *)ch;
}


/*
 * mk_open_listener - make a channel an OPEN listener channel
 *
 * We will form a listener channel in the OPEN state.  We will use a
 * closed listener channel, or if none is found, we will create a new
 * listener channel.
 *
 * returns:
 *      OPEN listener channel or NULL if error
 *
 * NOTE: This function uses the NULL cmd interface of open_listener() which
 *       assumes that cfg_random.lavapool contains the socket port or path.
 */
chan *
mk_open_listener(void)
{
    chan *c;	/* channel being opened */

    /*
     * find a channel
     *
     * We will look for a CLOSED listener channel first.  If we do not
     * find one, we will create a new channel
     */
    c = find_chan(TYPE_LISTENER, CLOSE);
    if (c == NULL) {
	c = find_chan(TYPE_LISTENER, ALLOCED);
	if (c == NULL) {
	    c = mk_chan(TYPE_LISTENER);
	    if (c == NULL) {
		warn("mk_open_listener",
		     "unable to create a listener channel");
		return NULL;
	    }
	}
    }

    /*
     * open the channel
     */
    dbg(3, "mk_open_listener", "chan[%d]: %s ==> %s, force nxtstate: %s",
	c->listener.indx,
	STATE_NAME(c->listener.curstate), STATE_NAME(c->listener.nxtstate),
	STATE_NAME(OPEN));
    c->listener.nxtstate = OPEN;

    open_listener(&(c->listener), NULL);
    if (c->listener.curstate != OPEN) {
	return NULL;
    }
    return c;
}


/*
 * open_listener - open an listener channel
 *
 * The channel will have socket on which it will listener for new
 * client connections.
 *
 * This function does nothing if the channel is HALTed.
 *
 * given:
 *      ch      listener channel
 *      port    host:port or /socket/path
 *                or NULL ==> use cfg_random.lavapool
 */
static void
open_listener(listener *ch, char *port)
{
    int fd;	/* socket descriptor */

    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(10, "open_listener", "NULL ch arg");
	/*NOTREACHED*/
    }
    if (port == NULL) {
	port = cfg_random.lavapool;
    }
    if (port == NULL) {
	fatal(10, "open_listener", "NULL port arg and NULL lavapool");
	/*NOTREACHED*/
    }
    if (ch->curstate == HALT || ch->nxtstate == HALT) {
	warn("open_listener", "chan[%d] is/will HALT, cannot open", ch->indx);
	return;
    }
    if (ch->nxtstate != OPEN) {
	warn("open_listener", "chan[%d]: %s ==> %s, nxtstate != %s",
	     ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate),
	     STATE_NAME(OPEN));
	return;
    }

    /*
     * try to open the socket
     */
    fd = lava_listen(port);
    if (fd < 0) {
	dbg(2, "open_listener", "unable to open: %s", port);
	return;
    }

    /*
     * place the information into the channel
     */
    ch->fd = fd;
    set_chanindx(ch->indx, fd);
    ch->count = 0;
    ch->last_op = about_now;
    dbg(3, "open_listener", "chan[%d]: state was %s ==> %s, now %s ==> %s",
	ch->indx, STATE_NAME(ch->curstate),
	STATE_NAME(ch->nxtstate), STATE_NAME(OPEN), STATE_NAME(ACCEPT));
    ch->curstate = OPEN;
    ch->nxtstate = ACCEPT;
    return;
}


/*
 * accept_listener - accept a connection on a listening socket
 *
 * We will accept a connection on an open TYPE_LISTENER channel (in OPEN or
 * ACCEPT state).  This new connection will become an TYPE_CLIENT channel.
 *
 * This function does nothing if the channel is HALTed.
 *
 * given:
 *      ch       listener channel
 *
 * NOTE: It is assumed that the caller has used select() (perhaps via
 *       the chan_select() function) to determine that the descriptor
 *       is readable, otherwise this call may block.
 *
 * NOTE: The calling chain is assumed to have previously determined that
 *       there are not too many client chains in existence.  For example,
 *       this check is done in chan_select() where too many open clients
 *       will case the read select mask bits remain cleared (and thus the
 *       chan_select() will not call chan_indx_op() which in turn will not
 *       call do_listener_op() which in turn will not call this function).
 */
void
accept_listener(listener *ch)
{
    int fd;	/* newly accepted socket */
    struct sockaddr addr;	/* address of other end of accepted socket */
    socklen_t addrlen;	/* length of address in addr */
    chan *new;	/* new client channel */

    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(10, "accept_listener", "NULL ch arg");
	/*NOTREACHED*/
    }
    if (ch->type != TYPE_LISTENER) {
	warn("accept_listener", "expected type %d, found: %d",
	     TYPE_LISTENER, ch->type);
	return;
    }
    if (ch->curstate == HALT || ch->nxtstate == HALT) {
	warn("accept_listener", "chan[%d] is HALTed, cannot accept", ch->indx);
	return;
    }
    if (ch->nxtstate != ACCEPT) {
	warn("accept_listener", "chan[%d]: %s ==> %s, nxtstate != %s",
	     ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate),
	     STATE_NAME(ACCEPT));
	return;
    }

    /*
     * accept a new connection
     */
    memset(&addr, 0, sizeof(addr));
    memset(&addrlen, 0, sizeof(addrlen));
    fd = accept(ch->fd, &addr, &addrlen);
    if (fd < 0) {
	return;
    }

    /*
     * make the connection non-blocking
     */
    if (noblock_fd(fd) < 0) {
	warn("accept_listener", "unable to non-block client socket");
	close(fd);
	return;
    }

    /*
     * place the information into the client channel
     */
    new = mk_open_client(fd);
    if (new == NULL) {
	warn("accept_listener", "unable to open a client");
	close(fd);
	return;
    }

    /*
     * place the information into the channel
     */
    ++ch->count;
    ch->last_op = about_now;
    if (ch->curstate != ACCEPT) {
	dbg(3, "accept_listener",
	    "chan[%d]: state was %s ==> %s, now %s ==> %s",
	    ch->indx, STATE_NAME(ch->curstate),
	    STATE_NAME(ch->nxtstate), STATE_NAME(ACCEPT), STATE_NAME(ACCEPT));
	ch->curstate = ACCEPT;
	ch->nxtstate = ACCEPT;
    }
    return;
}


/*
 * close_listener - close a listener channel
 *
 * We will ensure that a listener channel is closed.  If the descriptor is
 * open, we will attempt to close it.
 *
 * On success the channel is left in a CLOSE state.
 *
 * This function does nothing if the channel is not a TYPE_LISTENER type.
 * This function does nothing if the channel is HALTed.
 *
 * given:
 *      ch       listener channel
 */
void
close_listener(listener *ch)
{
    int ret;	/* system call return */

    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(11, "close_listener", "NULL arg");
	/*NOTREACHED*/
    }
    if (ch->type != TYPE_LISTENER) {
	warn("close_listener", "expected type %d, found: %d",
	     TYPE_LISTENER, ch->type);
	return;
    }
    if (ch->curstate == HALT || ch->nxtstate == HALT) {
	warn("close_listener", "chan[%d] is HALTed, cannot close", ch->indx);
	return;
    }

    /*
     * close descriptor
     */
    if (ch->fd >= 0) {
	errno = 0;
	ret = close(ch->fd);
	if (ret < 0) {
	    dbg(3, "close_listener", "chan[%d]: failed to close fd %d: %s",
		ch->indx, ch->fd, strerror(errno));
	} else {
	    dbg(4, "close_listener", "chan[%d]: closed fd %d: %s",
		ch->indx, ch->fd);
	}
	clear_chanindx(ch->indx, ch->fd);
	ch->fd = -1;
    }

    /*
     * set state
     */
    ch->last_op = about_now;
    dbg(3, "close_listener", "chan[%d]: state was %s ==> %s, now %s ==> %s",
	ch->indx, STATE_NAME(ch->curstate),
	STATE_NAME(ch->nxtstate), STATE_NAME(CLOSE), STATE_NAME(ALLOCED));
    ch->curstate = CLOSE;
    ch->nxtstate = ALLOCED;
    return;
}
