/*
 * client - client type channel operations
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: client.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>

#include "LavaRnd/rawio.h"
#include "LavaRnd/cfg.h"

#include "chan.h"
#include "dbg.h"
#include "cfg_lavapool.h"
#include "pool.h"

#if defined(DMALLOC)
#include <dmalloc.h>
#endif


/*
 * The client state changes are as follows:
 *
 *	ALLOCED	==> OPEN
 *	CLOSE	==> OPEN
 *
 *	OPEN	==> READ	[read & exception select]
 *	OPEN	==> CLOSE	[read & exception select]
 *
 *	READ	==> READ	[read & exception select]
 *	READ	==> GATHER	[read & exception select]
 *	READ	==> CLOSE	[read & exception select]
 *
 *	GATHER	==> WRITE
 *	GATHER	==> CLOSE
 *
 *	WRITE	==> WRITE	[write & exception select]
 *	WRITE	==> CLOSE	[write & exception select]
 *
 *	CLOSE   ==> ALLOCED
 *
 * in addition to the following state changes:
 *
 *	*	==> HALT
 */


/*
 * client_read_mask - if the next state is read selectable
 *
 * 0 ==> do not add to read select mask
 * 1 ==> 	add to read select mask
 *
 * NOTE: The order of this array must match the chanstate type in chan.h!
 */
int const client_read_mask[LAST_STATE+1] = {
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
 * client_write_mask - if the next state is write selectable
 *
 * 0 ==> do not add to write select mask
 * 1 ==> 	add to write select mask
 *
 * NOTE: The order of this array must match the chanstate type in chan.h!
 */
int const client_write_mask[LAST_STATE+1] = {
    0,	/* ALLOCED */
    0,	/* ACCEPT */
    0,	/* OPEN */
    0,	/* READ */
    0,	/* GATHER */
    1,	/* WRITE */
    0,	/* CLOSE */
    0	/* HALT */
};


/*
 * client_exception_mask - if the next state is exception selectable
 *
 * 0 ==> do not add to exception select mask
 * 1 ==> 	add to exception select mask
 *
 * NOTE: The order of this array must match the chanstate type in chan.h!
 */
int const client_exception_mask[LAST_STATE+1] = {
    0,	/* ALLOCED */
    0,	/* ACCEPT */
    0,	/* OPEN */
    1,	/* READ */
    0,	/* GATHER */
    1,	/* WRITE */
    0,	/* CLOSE */
    0	/* HALT */
};


/*
 * client_preselect_ready - if the next state is auto non-blocking ready
 *
 * 0 ==> not ready for automatic non-blocking operation
 * 1 ==>     ready for automatic non-blocking operation
 *
 * NOTE: The order of this array must match the chanstate type in chan.h!
 */
int const client_preselect_ready[LAST_STATE+1] = {
    0,	/* ALLOCED */
    0,	/* ACCEPT */
    1,	/* OPEN */
    0,	/* READ */
    1,	/* GATHER */
    0,	/* WRITE */
    1,	/* CLOSE */
    0	/* HALT */
};


/*
 * static functions
 */
static void read_client(client *ch);
static void gather_client(client *ch);
static void write_client(client *ch);
static void client_force_close(client *ch);


/*
 * do_client_op - perform the channel type specific operation
 *
 * given:
 *	ch	pointer to a channel
 *	cycle	why this operation as selected, in what part of the cycle
 *
 * NOTE: The caller is usually chan_indx_op().
 */
void
do_client_op(client *ch, chancycle cycle)
{
    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(10, "do_client_op", "NULL arg");
	/*NOTREACHED*/
    }
    if (! VALID_STATE(ch->nxtstate)) {
	fatal(10, "do_client_op", "chan[%d] invalid state: %d",
		  ch->indx, ch->nxtstate);
	/*NOTREACHED*/
    }

    /*
     * perform operation based on current channel cycle
     */
    switch ((int)cycle) {
    case CYCLE_PRESELECT:
	switch ((int)ch->nxtstate) {
	case GATHER:
	    dbg(4, "do_client_op",
		   "pre-select op: gather chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    gather_client(ch);
	    break;

	case CLOSE:
	    dbg(4, "do_client_op",
	    	   "pre-select close op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    close_client(ch);
	    break;

	default:
	    warn("do_client_op",
	    	 "ignore pre-select op: chan[%d] state: %s ==> %s",
		 ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    break;
	}
	break;

    case CYCLE_SELECTEXECPT:
	if (client_read_mask[ch->nxtstate] ||
	    client_write_mask[ch->nxtstate] ||
	    client_exception_mask[ch->nxtstate]) {
	    dbg(4, "do_client_op",
	    	   "select-exception close op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    close_client(ch);
	} else {
	    warn("do_client_op",
	    	 "ignore select-exception op: chan[%d] state: %s ==> %s",
		 ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	}
	break;

    case CYCLE_SELECTREAD:
	switch ((int)ch->nxtstate) {
	case READ:
	    dbg(4, "do_client_op",
		   "select-read op: read op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    read_client(ch);
	    break;

	default:
	    warn("do_client_op",
	    	 "ignore select-read read op: chan[%d] state: %s ==> %s",
		 ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    break;
	}
	break;

    case CYCLE_SELECTWRITE:
	switch ((int)ch->nxtstate) {
	case WRITE:
	    dbg(4, "do_client_op",
		   "select-read op: write op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    write_client(ch);
	    break;

	default:
	    warn("do_client_op",
	    	 "ignore select-read op: chan[%d] state: %s ==> %s",
		 ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	    break;
	}
	break;

    default:
	warn("do_client_op",
	     "op: chan[%d] invalid cycle: %d", ch->indx, cycle);
	break;
    }
    return;
}


/*
 * client_select_mask - enable in a select if needed
 *
 * given:
 *	ch	pointer to a client channel
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
client_select_mask(client *ch, fd_set *rd, fd_set *wr, fd_set *ex)
{
    int ret = -1;		/* descriptor set or -1 */

    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(10, "client_select_mask", "NULL arg");
	/*NOTREACHED*/
    }
    if (ch->fd < 0) {
	dbg(4, "client_select_mask", "op: chan[%d] state: %s ==> %s not open",
	    ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	return -1;
    }
    if (! VALID_STATE(ch->nxtstate)) {
	fatal(10, "client_select_mask", "chan[%d] invalid state: %d",
		  ch->indx, ch->nxtstate);
	/*NOTREACHED*/
    }

    /*
     * set mask as needed
     */
    if (rd != NULL && client_read_mask[ch->nxtstate]) {
	FD_SET(ch->fd, rd);
	ret = ch->fd;
	if (dbg_lvl >= 5) {
	    dbg(5, "client_select_mask",
		"op: chan[%d]  fd: %d  state: %s ==> %s read mask set",
		ch->indx, ch->fd, STATE_NAME(ch->curstate),
		STATE_NAME(ch->nxtstate));
    	}
    }
    if (wr != NULL && client_write_mask[ch->nxtstate]) {
	FD_SET(ch->fd, wr);
	ret = ch->fd;
	if (dbg_lvl >= 5) {
	    dbg(5, "client_select_mask",
		"op: chan[%d]  fd: %d  state: %s ==> %s write mask set",
		ch->indx, ch->fd, STATE_NAME(ch->curstate),
		STATE_NAME(ch->nxtstate));
    	}
    }
    if (ex != NULL && client_exception_mask[ch->nxtstate]) {
	FD_SET(ch->fd, ex);
	ret = ch->fd;
	if (dbg_lvl >= 5) {
	    dbg(5, "client_select_mask",
		"op: chan[%d]  fd: %d  state: %s ==> %s exception mask set",
		ch->indx, ch->fd, STATE_NAME(ch->curstate),
		STATE_NAME(ch->nxtstate));
    	}
    }
    return ret;
}


/*
 * client_pre_select_op - perform a pre-select operation if allowed
 *
 * Automatic operations are operations that will not block and
 * do not need select to trigger then.  The channel must be in
 * a state where normal automatic processing is possible.  For example,
 * is cannot be in an ALLOCED or HALT current state.
 *
 * given:
 *	ch	pointer to a client channel
 *
 * NOTE: It is assumed that the caller (such as chan_cycle()) has verified
 #	 that the current and next states are valid.
 */
void
client_pre_select_op(client *ch)
{
    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(10, "client_pre_select_op", "NULL arg");
	/*NOTREACHED*/
    }
    if (! VALID_STATE(ch->nxtstate)) {
	fatal(10, "client_pre_select_op", "chan[%d] invalid state: %d",
		  ch->indx, ch->nxtstate);
	/*NOTREACHED*/
    }

    /*
     * perform an operation if automatic operation allowed
     */
    if (client_preselect_ready[ch->nxtstate]) {

	/*
	 * as an optimization, we do not GATHER on an empty pool
	 */
	if (pool_level() == 0 && ch->nxtstate == GATHER) {
	    dbg(4, "client_pre_select_op",
		"chan[%d]: skip GATHER on an empty pool", ch->indx);
	    return;
	}

	/*
	 * perform the pre-select channel operation
	 */
	if (dbg_lvl >= 4) {
	    dbg(4, "client_pre_select_op",
	        "start op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	}
	do_client_op(ch, CYCLE_PRESELECT);
	if (dbg_lvl >= 4) {
	    dbg(4, "client_pre_select_op",
	        "end op: chan[%d] state: %s ==> %s",
		ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
	}
    }
    return;
}


/*
 * mk_client - make a channel a client channel
 */
chan *
mk_client(client *ch)
{
    int indx;		/* remembered channel index */

    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(10, "mk_client", "NULL arg");
	/*NOTREACHED*/
    }

    /*
     * clear values
     */
    indx = ch->indx;
    memset(ch, 0, sizeof(*ch));
    ch->indx = indx;

    /*
     * force type
     */
    ch->type = TYPE_CLIENT;

    /*
     * force ALLOCED state
     */
    ch->random = NULL;
    ch->curstate = ALLOCED;
    ch->nxtstate = OPEN;
    dbg(2, "mk_client", "op: chan[%d] now client channel: state: %s ==> %s",
	ch->indx, STATE_NAME(ch->curstate), STATE_NAME(ch->nxtstate));
    return (chan *)ch;
}


/*
 * mk_open_client - make a channel an OPEN client channel
 *
 * We will form a client channel in the OPEN state.  We will use a
 * closed client channel, or if none is found, we will create a new
 * client channel.
 *
 * given:
 *	fd	open socket to associated with the client
 *
 * returns:
 *	OPEN client channel or NULL if error
 */
chan *
mk_open_client(int fd)
{
    chan *c;			/* channel being opened */
    client *ch;			/* client channel */
    int indx;			/* remembered channel index */

    /*
     * firewall
     */
    if (fd < 0) {
	warn("mk_open_client", "passed neg descriptor: %d", fd);
	return NULL;
    }

    /*
     * find a channel
     *
     * We will look for a CLOSED client channel first.  If we do not
     * find one, we will create a new channel
     */
    c = find_chan(TYPE_CLIENT, CLOSE);
    if (c == NULL) {
	c = find_chan(TYPE_CLIENT, ALLOCED);
	if (c == NULL) {
	    c = mk_chan(TYPE_CLIENT);
	    if (c == NULL) {
		warn("mk_open_client", "unable to create a client channel");
		return NULL;
	    }
	}
    }
    ch = &(c->client);

    /*
     * clear values
     */
    indx = ch->indx;
    if (ch->random != NULL) {
	free(ch->random);
    }
    memset(ch, 0, sizeof(*ch));
    ch->indx = indx;
    ch->fd = -1;
    ch->open_op = -1.0;
    ch->last_op = -1.0;
    ch->timelimit = 0.0;
    ch->timeout = -1.0;
    ch->random = NULL;

    /*
     * force type
     */
    ch->type = TYPE_CLIENT;

    /*
     * place the information into the channel
     */
    about_now = right_now();
    ch->open_op = about_now;
    ch->last_op = ch->open_op;
    if (cfg_lavapool.timeout > 0.0) {
	ch->timelimit = cfg_lavapool.timeout;
	ch->timeout = ch->open_op + cfg_lavapool.timeout;
    } else {
	ch->timelimit = 0.0;
	ch->timeout = 0.0;
    }
    ch->fd = fd;
    set_chanindx(ch->indx, ch->fd);
    dbg(3, "mk_open_client", "chan[%d]: was %s ==> %s, force %s ==> %s",
    	   c->client.indx,
	   STATE_NAME(c->client.curstate),  STATE_NAME(c->client.nxtstate),
	   STATE_NAME(OPEN), STATE_NAME(READ));
    ch->curstate = OPEN;
    ch->nxtstate = READ;
    return c;
}


/*
 * read_client - read a request count from a client
 *
 * We will read a octet request count from the client.  We will
 * keep reading until LAVA_REQBUFLEN chars have been read or
 * until a \n or \r is read.
 *
 * If the read buffer is terminated by a \n or \r and contains only
 * digits and is <= the maxrequest (from cfg.lavapool) then
 * the request count is recorded and we will move into GATHER state.
 *
 * If the read buffer is not terminated by \n or \r and is not full, we
 * remain in READ state.
 *
 * If the read buffer is full without a terminating \n or \r, or
 * if the read buffer contains non-digits, or if the request count
 * is too large (> maxrequest), then we will move into CLOSE state.
 *
 * This function does nothing if the channel is HALTed.
 *
 * given:
 *	ch client channel
 *
 * NOTE: It is assumed that the caller has used select() (perhaps via
 *	 the chan_select() function) to determine that the descriptor
 *	 is readable, otherwise this call may block.
 */
static void
read_client(client *ch)
{
    int ret;		/* system call return */
    int old_readcnt;	/* read count before read_once() was called */
    int i;

    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(11, "read_client", "NULL arg");
	/*NOTREACHED*/
    }
    if (ch->type != TYPE_CLIENT) {
	warn("read_client", "chan[%d]: expected type %d, found: %d",
			    ch->indx, TYPE_CLIENT, ch->type);
	return;
    }
    if (ch->curstate == HALT || ch->nxtstate == HALT) {
	warn("read_client", "chan[%d]: is HALTed, cannot read", ch->indx);
	return;
    }
    if ((int)ch->nxtstate != READ) {
	warn("read_client","chan[%d] next state: %s != %s",
	     ch->indx, STATE_NAME(ch->nxtstate), STATE_NAME(READ));
	return;
    }

    /*
     * close if we exceed our time limit
     */
    if (ch->timeout > 0.0 && ch->timeout < about_now) {
	dbg(3, "read_client", "chan[%d]: "
			      "pre-read timeout: %.3f < about now: %.3f",
			      ch->indx, ch->timeout, about_now);
	client_force_close(ch);
	return;
    }

    /*
     * close if the read buffer is already full
     */
    if (ch->readcnt >= LAVA_REQBUFLEN) {
	dbg(3, "read_client", "chan[%d]: read buffer already full: %d >= %d",
			      ch->indx, ch->readcnt, LAVA_REQBUFLEN);
	client_force_close(ch);
	return;
    }

    /*
     * read what we can
     */
    old_readcnt = ch->readcnt;
    errno = 0;
    ret = read_once(ch->fd, ch->readbuf+ch->readcnt,
    		    LAVA_REQBUFLEN-ch->readcnt, FALSE);
    if (ret < 0) {
	dbg(3, "read_client", "chan[%d]: read error: %s",
			      ch->indx, strerror(errno));
	client_force_close(ch);
	return;
    } else if (ret == 0) {
	dbg(3, "read_client", "chan[%d]: early EOF", ch->indx);
	client_force_close(ch);
	return;
    }
    dbg(3, "read_client", "chan[%d]: read %d readcnt: %d",
	   ch->indx, ret, ch->readcnt);
    ch->readcnt += ret;

    /*
     * parse the request count chars that we read
     */
    ch->readbuf[ch->readcnt+ret] = '\0';
    for (i = old_readcnt; i < ch->readcnt; ++i) {

	/*
	 * stop reading after the first \n or \r or \0
	 */
	if (ch->readbuf[i] == '\n' || ch->readbuf[i] == '\r' ||
	    ch->readbuf[i] == '\0') {

	    /*
	     * we have a full read count request, check value
	     */
	    ch->readbuf[i] = '\0';
	    ch->request = strtol(ch->readbuf, NULL, 0);
	    if (ch->request <= 0) {
		dbg(3, "read_client", "chan[%d]: "
		    "request count too small: %d <= 0", ch->indx, ch->request);
		client_force_close(ch);
		return;
	    } else if (ch->request > cfg_random.maxrequest) {
		dbg(3, "read_client", "chan[%d]: "
				      "request count too large: %d > %d",
		    ch->indx, ch->request, cfg_random.maxrequest);
		client_force_close(ch);
		return;
	    }

	    /*
	     * look for timeout
	     */
	    about_now = right_now();
	    if (ch->timeout > 0.0 && ch->timeout < about_now) {
		dbg(3, "read_client", "chan[%d]: "
				      "read timeout: %.3f < about now: %.3f",
				      ch->indx, ch->timeout, about_now);
		client_force_close(ch);
		return;
	    }

	    /*
	     * update accounting
	     */
	    ch->last_op = about_now;
	    if (ch->timeout > 0.0) {
		ch->timelimit -= (about_now - ch->open_op);
	    }
	    ch->timeout = 0.0;	/* clear timeout while gathering */

	    /*
	     * we are done reading and are ready to GATHER
	     */
	    dbg(3, "read_client",
	    	   "chan[%d]: state was %s ==> %s, now %s ==> %s",
		   ch->indx, STATE_NAME(ch->curstate),
		   STATE_NAME(ch->nxtstate),
		   STATE_NAME(GATHER), STATE_NAME(GATHER));
	    ch->curstate = GATHER;
	    ch->nxtstate = GATHER;
	    return;

	/*
	 * look invalid chars in count
	 *
	 * NOTE: Hex numbers such as 0x34 or 0X15 are OK as long as the
	 *	 x (or X) is in the 2nd char
	 */
	} else if (!isascii(ch->readbuf[i]) ||
		   (i == 1 && isascii(ch->readbuf[1] != 'x' &&
		   	      isascii(ch->readbuf[1] != 'X')) &&
		    !isdigit(ch->readbuf[1])) || !isdigit(ch->readbuf[i])) {

	    /*
	     * bogus number, close down
	     */
	    dbg(3, "read_client", "chan[%d]: non-digits in count: <<%s>>",
	    			  ch->indx, ch->readbuf);
	    client_force_close(ch);
	    return;
	}
    }

    /*
     * look for full request buffer without completion
     */
    if (ch->readcnt >= LAVA_REQBUFLEN) {
	dbg(3, "read_client", "chan[%d]: read buffer full: %d >= %d",
			      ch->indx, ch->readcnt, LAVA_REQBUFLEN);
	client_force_close(ch);
	return;
    }

    /*
     * update accounting
     */
    if (ch->curstate != READ) {
	dbg(3, "read_client", "chan[%d]: state was %s ==> %s, now %s ==> %s",
			      ch->indx, STATE_NAME(ch->curstate),
	    STATE_NAME(ch->nxtstate), STATE_NAME(READ));
	ch->curstate = READ;
	ch->nxtstate = READ;
    }
    return;
}


/*
 * gather_client - gather LavaRnd data for a client request
 *
 * We will allocate and copy in the LavaRnd data requested by the client.
 * If we were able to copy in all of the requested data, then the
 * client will move into the WRITE state.
 *
 * This function does nothing if the channel is HALTed.
 *
 * given:
 *	ch		client channel
 *
 * NOTE: It is assumed that the caller has used select() (perhaps via
 *	 the chan_select() function) to determine that the descriptor
 *	 is readable, otherwise this call may block.
 */
static void
gather_client(client *ch)
{
    int ret;		/* function return */

    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(11, "gather_client", "NULL arg");
	/*NOTREACHED*/
    }
    if (ch->type != TYPE_CLIENT) {
	warn("gather_client", "chan[%d]: expected type %d, found: %d",
			      ch->indx, TYPE_CLIENT, ch->type);
	return;
    }
    if (ch->curstate == HALT || ch->nxtstate == HALT) {
	warn("gather_client", "chan[%d]: HALTed, cannot read", ch->indx);
	return;
    }
    if ((int)ch->nxtstate != GATHER) {
	warn("gather_client", "chan[%d] next state: %s != %s",
	     ch->indx, STATE_NAME(ch->nxtstate), STATE_NAME(GATHER));
	return;
    }

    /*
     * firewall - watch for bogus requests and request states
     */
    if (ch->request <= 0) {
	warn("gather_client", "chan[%d] request size: %d <= 0",
			      ch->indx, ch->request);
	client_force_close(ch);
	return;
    }
    if (ch->random != NULL && ch->gathercnt < 0) {
	warn("gather_client", "chan[%d] gather count: %d < 0",
			      ch->indx, ch->gathercnt);
	client_force_close(ch);
	return;
    } else if (ch->random != NULL && ch->gathercnt > ch->request) {
	warn("gather_client", "chan[%d] gather count: %d > request: %d",
			      ch->indx, ch->gathercnt, ch->request);
	client_force_close(ch);
    }

    /*
     * malloc the random buffer if we do not have one
     */
    if (ch->random == NULL) {
	ch->random = (u_int8_t *)malloc(ch->request);
	if (ch->random == NULL) {
	    warn("gather_client", "chan[%d]: unable to malloc %d octets",
	    			  ch->indx, ch->request);
	    client_force_close(ch);
	    return;
	}
	ch->gathercnt = 0;
    }

    /*
     * determine how much LavaRnd data we can/should copy
     */
    ret = drain_pool(ch->random+ch->gathercnt, ch->request-ch->gathercnt);

    /*
     * update accounting
     */
    ch->gathercnt += ret;
    if (ch->gathercnt == ch->request) {
	ch->last_op = about_now;
	dbg(3, "gather_client",
	       "chan[%d]: state was %s ==> %s, now %s ==> %s",
	       ch->indx, STATE_NAME(ch->curstate),
	    STATE_NAME(ch->nxtstate), STATE_NAME(WRITE), STATE_NAME(WRITE));
	ch->curstate = WRITE;
	ch->nxtstate = WRITE;
    } else if (ch->gathercnt > ch->request) {
	warn("gather_client", "chan[%d] gathered too much: %d > request: %d",
			      ch->indx, ch->gathercnt, ch->request);
	client_force_close(ch);
    } else if (ch->curstate != GATHER) {
	dbg(3, "gather_client",
	       "chan[%d]: state was %s ==> %s, now %s ==> %s",
	       ch->indx, STATE_NAME(ch->curstate),
	    STATE_NAME(ch->nxtstate), STATE_NAME(GATHER), STATE_NAME(GATHER));
	ch->curstate = GATHER;
	ch->nxtstate = GATHER;
    }
    if (ch->gathercnt < ch->request) {
	dbg(3, "gather_client", "chan[%d]: gathered %d, need %d more",
				ch->indx, ret, ch->request-ch->gathercnt);
    } else {
	dbg(2, "gather_client", "chan[%d]: completed gather of %d octets",
				ch->indx, ch->request);
    }
    return;
}


/*
 * write_client - write a request count from a client
 *
 * We will attempt to write all of the gathered LavaRnd data to the
 * client or until timeout.
 *
 * This function does nothing if the channel is HALTed.
 *
 * given:
 *	ch client channel
 *
 * NOTE: It is assumed that the caller has used select() (perhaps via
 *	 the chan_select() function) to determine that the descriptor
 *	 is readable, otherwise this call may block.
 */
static void
write_client(client *ch)
{
    int ret;		/* system call return */

    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(11, "write_client", "NULL arg");
	/*NOTREACHED*/
    }
    if (ch->type != TYPE_CLIENT) {
	warn("write_client", "chan[%d]: expected type %d, found: %d",
			     ch->indx, TYPE_CLIENT, ch->type);
	return;
    }
    if (ch->curstate == HALT || ch->nxtstate == HALT) {
	warn("write_client", "chan[%d]: HALTed, cannot read", ch->indx);
	return;
    }
    if (ch->random == NULL) {
	warn("write_client", "chan[%d]: had a NULL random buffer", ch->indx);
	return;
    }
    if ((int)ch->nxtstate != WRITE) {
	warn("write_client", "chan[%d] next state: %s != %s",
	     ch->indx, STATE_NAME(ch->nxtstate), STATE_NAME(WRITE));
	return;
    }

    /*
     * close if we exceed our time limit
     */
    if (ch->timeout > 0.0 && ch->timeout < about_now) {
	dbg(3, "write_client", "chan[%d]: "
			       "pre-write timeout: %.3f < about now: %.3f",
			       ch->indx, ch->timeout, about_now);
	client_force_close(ch);
	return;
    }

    /*
     * close if we have written everything
     */
    if (ch->writecnt >= ch->request) {
	dbg(3, "write_client", "chan[%d]: "
			       "write cnt: %d >= gather cnt: %d",
			       ch->indx, ch->writecnt, ch->request);
	client_force_close(ch);
	return;
    }

    /*
     * write as much as we can
     */
    errno = 0;
    ret = write_once(ch->fd, ch->random+ch->writecnt,
    		     ch->request-ch->writecnt, FALSE);
    if (ret < 0) {
	dbg(3, "write_client", "chan[%d]: write error: %s",
			       ch->indx, strerror(errno));
	client_force_close(ch);
	return;
    }
    if (ret == 0) {
	dbg(3, "ch->writecnt", "chan[%d]: write EOF", ch->indx);
	client_force_close(ch);
	return;
    }

    /*
     * update accounting
     */
    ch->writecnt += ret;
    dbg(3, "write_client", "chan[%d]: write %d writecnt: %d",
    			   ch->indx, ret, ch->writecnt);

    /*
     * update accounting
     */
    if (ch->writecnt >= ch->request) {
        /* we have written everything */
	ch->last_op = about_now;
	dbg(3, "write_client", "chan[%d]: write complete", ch->indx);
	dbg(3, "gather_client",
	       "chan[%d]: state was %s ==> %s, now %s ==> %s",
	       ch->indx, STATE_NAME(ch->curstate),
	    STATE_NAME(ch->nxtstate), STATE_NAME(WRITE), STATE_NAME(CLOSE));
	ch->curstate = WRITE;
	ch->nxtstate = CLOSE;
    } else if (ch->curstate != WRITE) {
	/* need to write more */
	dbg(3, "gather_client",
	       "chan[%d]: state was %s ==> %s, now %s ==> %s",
	       ch->indx, STATE_NAME(ch->curstate),
	    STATE_NAME(ch->nxtstate), STATE_NAME(WRITE), STATE_NAME(WRITE));
	ch->curstate = WRITE;
	ch->nxtstate = WRITE;
    }
    return;
}


/*
 * close_client - close a client channel
 *
 * We will ensure that a client channel is closed.  If the descriptor is
 * open, we will attempt to close it.
 *
 * On success the channel is left in a CLOSE state.
 *
 * This function does nothing if the channel is not a TYPE_CLIENT type.
 * This function does nothing if the channel is HALTed.
 *
 * given:
 *	ch client channel
 */
void
close_client(client *ch)
{
    int ret;		/* system call return */

    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(11, "close_client", "NULL arg");
	/*NOTREACHED*/
    }
    if (ch->type != TYPE_CLIENT) {
	warn("close_client", "chan[%d]: expected type %d, found: %d",
			    ch->indx, TYPE_CLIENT, ch->type);
	return;
    }
    if (ch->curstate == HALT || ch->nxtstate == HALT) {
	warn("close_client", "chan[%d]: HALTed, cannot close", ch->indx);
	return;
    }

    /*
     * close descriptor
     */
    if (ch->fd >= 0) {
	errno = 0;
	ret = close(ch->fd);
	if (ret < 0) {
	    dbg(3, "close_client", "chan[%d]: failed to close fd %d: %s",
		   ch->indx, ch->fd, strerror(errno));
	} else {
	    dbg(4, "close_client", "chan[%d]: closed fd %d", ch->indx, ch->fd);
	}
	clear_chanindx(ch->indx, ch->fd);
	ch->fd = -1;
    }

    /*
     * free the allocated data if needed
     */
    if (ch->random != NULL) {
	free(ch->random);
	ch->random = NULL;
    }

    /*
     * set state
     */
    ch->last_op = about_now;
    dbg(3, "close_client", "chan[%d]: state was %s ==> %s, now %s ==> %s",
			 ch->indx, STATE_NAME(ch->curstate),
	STATE_NAME(ch->nxtstate), STATE_NAME(CLOSE), STATE_NAME(ALLOCED));
    ch->curstate = CLOSE;
    ch->nxtstate = ALLOCED;
    return;
}


/*
 * client_force_close - force close of a non-HALTed client channel
 *
 * We will ensure that a client channel is closed.  If the descriptor is
 * open, we will attempt to close it.
 *
 * On success the channel is left in a CLOSE state.
 *
 * This function does nothing if the channel is not a TYPE_CLIENT type.
 *
 * given:
 *	ch client channel
 */
static void
client_force_close(client *ch)
{
    /* announce */
    dbg(2, "client_force_close", "chan[%d]: will be forced to CLOSE",
	ch->indx);

    /* close */
    close_client(ch);
    return;
}
