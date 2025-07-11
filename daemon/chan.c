/*
 * chan - general channel processing
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: chan.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

#include "LavaRnd/rawio.h"
#include "LavaRnd/fnv1.h"

#include "chan.h"
#include "dbg.h"
#include "cfg_lavapool.h"
#include "pool.h"

#if defined(DMALLOC)
#  include <dmalloc.h>
#endif


/*
 * channels
 */
static chan *ch = NULL;
double about_now = 0.0;	/* aprox. "Seconds since the Epoch" in chan cycle */
static u_int64_t op_cycle = 0ULL;	/* chan operation cycle */

#define ALLOC_SET (8)		/* channels to allocate at one time */
#define IDLE_TIMEOUT (2.0)	/* select time when nothing is selectable */


/*
 * channel index
 *
 * chanindx[i] holds the chan array index for file descriptor i or <0
 *
 * NOTE: The length of chanindx is chanindx_len.
 */
static u_int32_t chanlen = 0;
static int *chanindx = NULL;
u_int32_t chanindx_len = 0;


/*
 * state name
 *
 * NOTE: The order of this array must match the chanstate type in chan.h!
 */
const char *const state_name[LAST_STATE + 1] = {
    "ALLOCED",
    "ACCEPT",
    "OPEN",
    "READ",
    "GATHER",
    "WRITE",
    "CLOSE",
    "HALT"
};


/*
 * chan_cycle_timeout
 *
 * The chan_cycle_timeout is first set by chan_cycle() before the
 * pre-select operation are performed.  Then during pre-select
 * operations need_cycle_before() may be called with timing information
 * that might shorten the channel cycle timeout.  Finally after
 * the all pre-select operations have been performed, the
 * chan_cycle_timeout is picked up by chan_cycle() and used
 * in place it its orignal argument.
 */
static double chan_cycle_timeout = 0.0;


/*
 * static functions
 */
static void alloc_chan(u_int32_t len);
static void chan_indx_op(u_int32_t indx, chancycle cycle);
static int chan_select(double timelen);


/*
 * alloc_chanindx - initialize the channel index and type array
 */
void
alloc_chanindx(void)
{
    int i;

    /*
     * we always make the index the size of the descriptor table
     */
    i = getdtablesize();
    if (i <= 0) {
	fatal(8, "main", "unable to get descriptor table size");
	/*NOTREACHED*/
    }
    dbg(2, "alloc_chanindx", "descriptor table size: %d", i);

    /*
     * create and initialize the channel index array
     */
    if (chanindx != NULL) {
	free(chanindx);
    }
    chanindx = (int *)malloc(i * sizeof(u_int32_t));
    if (chanindx == NULL) {
	fatal(9, "alloc_chanindx", "unable to malloc %d channel indexes", i);
	/*NOTREACHED*/
    }
    chanindx_len = i;

    /*
     * determine which descriptors are unused and which are in use already
     */
    for (i = 0; i < chanindx_len; ++i) {
	int arg;	/* open flags */

	if (fcntl(i, F_GETFL, &arg) < 0) {
	    chanindx[i] = LAVA_UNUSED_FD;
	} else {
	    dbg(3, "alloc_chanindx", "descriptor %d already open", i);
	    chanindx[i] = LAVA_SYS_FD;
	}
    }
    return;
}


/*
 * alloc_chan - setup the chan array
 *
 * given:
 *      len     number of channels to initialize
 *
 * NOTE: This function will increase but not decrease the chan length.
 */
static void
alloc_chan(u_int32_t len)
{
    int start;	/* starting index to initialize */
    int i;

    /*
     * firewall
     *
     * We never shrink the channel array.
     */
    if (len < 1 || len <= chanlen) {
	return;
    }

    /*
     * case: initial malloc
     */
    if (chanlen <= 0 || ch == NULL) {

	/*
	 * malloc the array
	 */
	ch = (chan *)malloc(len * sizeof(chan));
	if (ch == NULL) {
	    fatal(11, "alloc_chan", "unable to malloc %d channels", len);
	    /*NOTREACHED*/
	}
	chanlen = len;
	start = 0;

	/*
	 * case: expand array
	 */
    } else {
	chan *p;

	/*
	 * realloc the array
	 */
	p = (chan *)realloc(ch, len * sizeof(chan));
	if (p == NULL) {
	    fatal(11, "alloc_chan",
		  "unable to malloc %d more channels", len - chanlen);
	    /*NOTREACHED*/
	}
	ch = p;
	start = chanlen;
	chanlen = len;
    }

    /*
     * initialize the new channels
     */
    memset(ch + start, 0, len - start);
    for (i = start; i < chanlen; ++i) {
	ch[i].common.indx = i;
	ch[i].common.type = TYPE_NONE;
	ch[i].common.curstate = ALLOCED;
	ch[i].common.nxtstate = ALLOCED;
	ch[i].common.fd = -1;
    }
    return;
}


/*
 * mk_chan - make a channel of a given type
 *
 * This function will look for an unused channel, set its type and
 * call the make function for that type.  If all channels are in use,
 * it will allocate another channel.
 *
 * We search for channels in a circular fashion.  We start from
 * beyond the last search position.  When we reach the end we
 * start back around from the beginning.
 *
 * given:
 *      type    type of channel to make
 *
 * returns:
 *      pointer to the make channel or NULL if error
 */
chan *
mk_chan(chantype type)
{
    static int last_indx = 0;	/* last index searched */
    int start;	/* starting index to look for new ones */
    chan *ret = NULL;	/* return channel that was made */
    int i;
    int j;

    /*
     * firewall
     */
    switch ((int)type) {
    case TYPE_LISTENER:
    case TYPE_CLIENT:
    case TYPE_CHAOS:
	break;
    default:
	warn("mk_chan", "asked to make an inappropriate type: %d", (int)type);
	return NULL;
    }

    /*
     * look for a TYPE_NONE channel
     */
    for (i = 0; i < chanlen; ++i) {

	/* cycle thru channels starting with last index */
	j = (i + last_indx) % chanlen;

	/* cycle processing */
	if (ch[j].common.type == TYPE_NONE) {
	    /* found a channel, make it */
	    switch ((int)type) {
	    case TYPE_LISTENER:
		ret = mk_listener(&(ch[j].listener));
		break;
	    case TYPE_CLIENT:
		ret = mk_client(&(ch[j].client));
		break;
	    case TYPE_CHAOS:
		ret = mk_chaos(&(ch[j].chaos));
		break;
	    }
	    /* start next search beyond this spot */
	    last_indx = (i + 1) % chanlen;
	    return ret;
	}
    }

    /*
     * allocate more channels
     */
    start = chanlen;
    alloc_chan(chanlen + ALLOC_SET);
    for (i = start; i < chanlen; ++i) {

	/* cycle thru channels starting with last index */
	j = (i + last_indx) % chanlen;

	/* cycle processing */
	if (ch[j].common.type == TYPE_NONE) {
	    /* found a channel, make it */
	    switch ((int)type) {
	    case TYPE_LISTENER:
		ret = mk_listener(&(ch[j].listener));
		break;
	    case TYPE_CLIENT:
		ret = mk_client(&(ch[j].client));
		break;
	    case TYPE_CHAOS:
		ret = mk_chaos(&(ch[j].chaos));
		break;
	    }
	    /* start next search beyond this spot */
	    last_indx = (i + 1) % chanlen;
	    return ret;
	}
    }

    /*
     * did not find a channel, advance start position for next time
     */
    warn("mk_chan", "falling off end, returning NULL");
    if (chanlen != 0) {
	last_indx = (last_indx + 1) % chanlen;
    }
    return NULL;
}


/*
 * halt_chan - force a channel into a halted state
 *
 * given:
 *      ch      channel pointer
 */
void
halt_chan(chan *ch)
{
    /*
     * firewall
     */
    if (ch == NULL) {
	fatal(10, "halt_chan", "NULL arg");
	/*NOTREACHED*/
    }

    /*
     * force into a halted state
     */
    dbg(1, "halt_chan", "chan[%d]: was current state: %d (%s) next: %d (%s)",
	ch->common.indx,
	ch->common.curstate, STATE_NAME(ch->common.curstate),
	ch->common.nxtstate, STATE_NAME(ch->common.nxtstate));
    ch->common.curstate = HALT;
    ch->common.nxtstate = HALT;
    warn("halt_chan", "chan[%d]: forced into HALT state", ch->common.indx);

    return;
}


/*
 * find_chan - find a given channel type in a given current state
 *
 * We search for channels in a circular fashion.  We start from
 * beyond the last search position.  When we reach the end we
 * start back around from the beginning.
 *
 * given:
 *      type            type of channel to find
 *      curstate        current state of channel to find
 *
 * returns:
 *      channel with the given type and current state or NULL
 */
chan *
find_chan(chantype type, chanstate curstate)
{
    static int last_indx = 0;	/* last index searched */
    int i;
    int j;

    /*
     * firewall
     */
    if (chanlen <= 0) {
	return NULL;
    }

    /*
     * cycle thru all channels from beyond the point of the previous
     * search, remembering where we stopped and advancing the next
     * start position beyond it.
     */
    for (i = 0; i < chanlen; ++i) {

	/* cycle thru channels starting with last index */
	j = (i + last_indx) % chanlen;

	/* cycle processing */
	if (ch[j].common.type == type && ch[j].common.curstate == curstate) {

	    /* start next search beyond this spot */
	    last_indx = (i + 1) % chanlen;

	    /* found a channel, make it */
	    return &(ch[j]);
	}
    }

    /*
     * did not find a channel, advance start position for next time
     */
    if (chanlen != 0) {
	last_indx = (last_indx + 1) % chanlen;
    }
    return NULL;
}


/*
 * chan_indx_op - perform an operation on a given channel
 *
 * given:
 *      indx    channel index to operate on
 *      cycle   why this operation as selected, in what part of the cycle
 */
static void
chan_indx_op(u_int32_t indx, chancycle cycle)
{
    /*
     * firewall
     */
    if (indx >= chanlen) {
	fatal(10, "chan_indx_op",
	      "attempted operation on invalid index: %d must be < %d",
	      indx, chanlen);
	/*NOTREACHED*/
	return;
    }
    if (ch[indx].common.indx != indx) {
	fatal(10, "chan_indx_op",
	      "chan index: %d != %d", ch[indx].common.indx, indx);
	/*NOTREACHED*/
	return;
    }
    if (ch[indx].common.curstate < 0 || ch[indx].common.curstate > LAST_STATE) {
	warn("chan_cycle", "chan[%d]: invalid current state: %d",
	     indx, (int)ch[indx].common.curstate);
	halt_chan(&(ch[indx]));
	return;
    }
    if (ch[indx].common.nxtstate < 0 || ch[indx].common.nxtstate > LAST_STATE) {
	warn("chan_cycle", "chan[%d]: invalid next state: %d",
	     indx, (int)ch[indx].common.nxtstate);
	halt_chan(&(ch[indx]));
	return;
    }

    /*
     * perform the operation based on type
     */
    switch (ch[indx].common.type) {
    case TYPE_NONE:
	fatal(10, "chan_indx_op",
	      "attempted operation on TYPE_NONE channel type, indx: %d",
	      indx);
	/*NOTREACHED*/
	return;
    case TYPE_LISTENER:
	do_listener_op(&(ch[indx].listener), cycle);
	return;
    case TYPE_CLIENT:
	do_client_op(&(ch[indx].client), cycle);
	return;
    case TYPE_CHAOS:
	do_chaos_op(&(ch[indx].chaos), cycle);
	return;
    case TYPE_SYSTEM:
	fatal(10, "chan_indx_op",
	      "attempted operation on TYPE_SYSTEM channel type, indx: %d",
	      indx);
	/*NOTREACHED*/
	break;
    default:
	fatal(10, "chan_indx_op",
	      "attempted operation on unknown channel type: %d, indx: %d",
	      (int)ch[indx].common.type, indx);
	/*NOTREACHED*/
	break;
    }
    return;
}


/*
 * chan_select - perform a select operation on all channels that need it
 *
 * This function will build up a select mask based on channels that
 * are in a selectable current state.  It will then perform a select
 * statement and return the select value.
 *
 * We search for select ready descriptors in a circular fashion.  We start
 * from beyond the last search position.  When we reach the end we
 * start back around from the beginning.
 *
 * given:
 *      timelen         max seconds to, 0 ==> immediate return, <0 ==> forever
 *
 * returns:
 *      select return value (number of descriptors set) or -1
 */
static int
chan_select(double timelen)
{
    static int last_desc = 0;	/* last descriptor index searched */
    int n;	/* highest-numbered descriptor set + 1 */
    fd_set rd;	/* read select mask */
    fd_set wr;	/* write select mask */
    fd_set ex;	/* exception select mask */
    struct timeval timeout;	/* select timeout */
    struct timeval *ptimeout;	/* select timeout arg or NULL */
    double start = 0.0;	/* pre-select time */
    double stop;	/* post-select time */
    int ret;	/* select or call return */
    int action;	/* number of select based actions left */
    int too_many;	/* TRUE ==> too many client channels open */
    int looped;	/* TRUE ==> we looped around descriptor end already */
    static int toggle = 0;	/* a value that alternates between 0 and 1 */
    double fill_speed;	/* 1.0 => fast fill, 0 => slow, -1.0 => none */
    double fill_odds;	/* odds that we may chaos read 1/2 the time */
    int i;
    int j;

    /*
     * initialize select values
     */
    toggle = (toggle ? 0 : 1);
    n = 0;
    FD_ZERO(&rd);
    FD_ZERO(&wr);
    FD_ZERO(&ex);
    if (timelen < 0.0) {
	ptimeout = NULL;
    } else {
	if (timelen == 0.0) {
	    timeout.tv_sec = 0;
	    timeout.tv_usec = 0;
	    ptimeout = &timeout;
	} else {
	    timeout.tv_sec = double_to_tv_sec(timelen);
	    timeout.tv_usec = double_to_tv_usec(timelen);
	    ptimeout = &timeout;
	}
    }

    /*
     * determine if we have too many open client channels
     */
    too_many = FALSE;
    if (cfg_lavapool.maxclients > 0) {
	int cnt;	/* open client channel count */

	for (i = 0, cnt = 0; i < chanlen && cnt < cfg_lavapool.maxclients; ++i) {
	    if (ch[i].common.type == TYPE_CLIENT) {
		switch ((int)ch[i].common.curstate) {
		    /* TYPE_CLIENT channels in this state effectively open */
		case OPEN:
		case READ:
		case GATHER:
		case WRITE:
		    ++cnt;
		    break;
		}
	    }
	    if (cnt >= cfg_lavapool.maxclients) {
		too_many = TRUE;
		break;
	    }
	}
    }

    /*
     * look at each channel for I/O based select masking
     */
    for (i = 0; i < chanlen; ++i) {

	/*
	 * check for invalid states
	 */
	if (ch[i].common.curstate < 0 || ch[i].common.curstate > LAST_STATE) {
	    warn("chan_select", "chan[%d]: invalid current state: %d",
		 i, (int)ch[i].common.curstate);
	    halt_chan(&(ch[i]));
	    continue;
	}
	if (ch[i].common.nxtstate < 0 || ch[i].common.nxtstate > LAST_STATE) {
	    warn("chan_select", "chan[%d]: invalid next state: %d",
		 i, (int)ch[i].common.nxtstate);
	    halt_chan(&(ch[i]));
	    continue;
	}

	/*
	 * skip if not open
	 */
	if (ch[i].common.fd < 0) {
	    continue;
	}

	/*
	 * set masks based on chan type
	 */
	ret = -1;
	switch (ch[i].common.type) {

	case TYPE_LISTENER:
	    /*
	     * We only want to read select on a listener if we have less
	     * than the maximum number of allowed clients.  The only
	     * reason to read select on a listener is to pick up a new client.
	     * If we are already at (or above) the maximum client count,
	     * we have no reason to read select and potentially pick up
	     * another client.
	     */
	    if (too_many) {
		/* too many open clients, do not listen for new connections */
		ret = listener_select_mask(&(ch[i].listener), NULL, &wr, &ex);
	    } else {
		ret = listener_select_mask(&(ch[i].listener), &rd, &wr, &ex);
	    }
	    break;

	case TYPE_CLIENT:
	    /*
	     * We always should be willing to service a client channel.
	     */
	    ret = client_select_mask(&(ch[i].client), &rd, &wr, &ex);
	    break;

	case TYPE_CHAOS:
	    /*
	     * If we are ready and willing to frame dump, then we select
	     * this channel regardless of pool size and fill rates.
	     * This is because the frame that is dumped is never used
	     * in the entropy pool.  When the frame is dumped, the
	     * ch->fast_select is set to TRUE and on the NEXT (presumably)
	     * quick cycle we don't frame dump but instead consider
	     * the pool size and fill rate issue.
	     */
	    if (ready_to_frame_dump(&(ch[i].chaos))) {
		dbg(3, "chan_select",
		    "chan[%d]: frame dump select", ch[i].common.indx);
		ret = chaos_select_mask(&(ch[i].chaos), &rd, &wr, &ex);
		break;
	    }

	    /*
	     * The decision to select on a chaos channel mostly depends
	     * on how fast we want to fill the lava pool.
	     *
	     * When the lava pool is low, we want to fill it more quickly
	     * than when it nearly full.  And of course when the pool is
	     * full we do not want/need to fill it at all.
	     *
	     * The nature of a chaos channel is that it is almost always
	     * ready to deliver data (either from a co-process or from
	     * a driver such as a camera driver).  We simply cannot select
	     * a chaos channel on each channel cycle (call to this function).
	     * To be always willing to select a chaos channel would cause
	     * the lavapool daemon to fill the lava pool at maximum speed.
	     * The daemon would consume system resources at a maximum rate
	     * (limited only by the co-process or driver's ability to pump
	     * data) until pool was full.  Any withdrawal from the lava pool
	     * by a client channels would return the daemon back to a maximum
	     * consumption rate.
	     *
	     * A reasonable compromise is to fill the lava pool at the
	     * maximum possible rate when the pool is empty.  As the lava
	     * pool fills we slow the filling rate down.  When the pool
	     * reaches some reasonable level we bottom out at some slow
	     * fill rate.  This slow fill rate continues until the pool
	     * becomes full.
	     *
	     * We control the fill speed by how frequently we read from
	     * the chaos channel (assuming it it ready to read):
	     *
	     *    pool level <= fastpool octets         fast fill rate
	     *
	     *          Read form the chaos channel on each cycle.
	     *
	     *    pool level <= slowpool        fast to slow fill rate
	     *
	     *          50% of the cycles we always read form the chaos channel
	     *          50% of the cycles we "may" read from it.
	     *
	     *          The 50% "may" part depends the pool level.  At lower
	     *          pool levels "may" amounts to nearly all cycles.  At
	     *          high pool levels "may" amounts to only a few cycles.
	     *
	     *          Thus when the level is close to fastpool, we always
	     *          read.  When the level is closer to slowpool we read
	     *          about 1/2 of the time.
	     *
	     *    pool level < ~poolsize                slow fill fate
	     *
	     *          Read form the chaos channel on every other cycle.
	     *
	     *          We say ~poolsize because the pool stops
	     *          filling within 20 octets of the top.
	     *
	     *    pool level >= ~poolsize               no fill operations
	     *
	     *          The pool is full.  We do not fill on any cycle.
	     */
	    fill_speed = pool_rate_factor();
	    /* fast_select - always read select unless full */
	    if (ch[i].chaos.fast_select && fill_speed >= 0.0) {
		dbg(3, "chan_select",
		    "chan[%d]: fast select", ch[i].common.indx);
		ret = chaos_select_mask(&(ch[i].chaos), &rd, &wr, &ex);
		/* low pool level - fast fill rate - always read select */
	    } else if (fill_speed >= 1.0) {
		dbg(3, "chan_select",
		    "chan[%d]: fast fill speed: %.3f",
		    ch[i].common.indx, fill_speed);
		ret = chaos_select_mask(&(ch[i].chaos), &rd, &wr, &ex);
		/* med pool level - fast->slow fill rate - sometimes read select */
	    } else if (fill_speed > 0.0) {
		/* 50% of the time we always read select */
		if (toggle) {
		    dbg(3, "chan_select",
			"chan[%d]: med speed: %.3f toggle: 1",
			ch[i].common.indx, fill_speed);
		    ret = chaos_select_mask(&(ch[i].chaos), &rd, &wr, &ex);
		    /* 50% of the time we "may" select */
		} else {
		    /* Determine the "may" select odds */
		    fill_odds = fnv_seq();
		    /* select sometimes */
		    if (fill_speed > fill_odds) {
			dbg(3, "chan_select",
			    "chan[%d]: med speed: %.3f <= odds: %.3f",
			    ch[i].common.indx, fill_speed, fill_odds);
			ret = chaos_select_mask(&(ch[i].chaos), &rd, &wr, &ex);
			/* do not select other times */
		    } else {
			dbg(3, "chan_select",
			    "chan[%d]: med skip: %.3f > odds: %.3f",
			    ch[i].common.indx, fill_speed, fill_odds);
			ret = -1;
		    }
		}
		/* high pool level - fast->slow fill rate - read select 1/2 time */
	    } else if (fill_speed == 0.0) {
		if (toggle) {
		    dbg(3, "chan_select",
			"chan[%d]: slow fill speed: %.3f toggle: 1",
			ch[i].common.indx, fill_speed);
		    ret = chaos_select_mask(&(ch[i].chaos), &rd, &wr, &ex);
		} else {
		    dbg(3, "chan_select", "chan[%d]: slow skip: %.3f",
			ch[i].common.indx, fill_speed);
		    ret = -1;
		}
		/* full pool level - do not fill */
	    } else {
		dbg(3, "chan_select", "chan[%d]: full skip",
		    ch[i].common.indx);
		ret = -1;
	    }
	    break;

	    /*
	     * Ignore any other type of channel.
	     */
	default:
	    break;
	}

	/*
	 * keep track of the highest bit set
	 */
	if (ret + 1 > n) {
	    n = ret + 1;
	}
	if (ret >= 0 && dbg_lvl >= 4) {
	    dbg(4, "chan_select", "selecting on chan[%d]: fd: %d state: %s",
		ch[i].common.indx, ch[i].common.fd,
		STATE_NAME(ch[i].common.curstate));
	}
    }

    /*
     * setup for pre-select debug
     */
    if (dbg_lvl >= 2) {
	start = right_now();
	if (timelen == 0.0) {
	    dbg(2, "chan_select", "select with zero timeout");
	} else if (timelen < 0.0) {
	    dbg(2, "chan_select", "select with forever timeout");
	} else {
	    dbg(2, "chan_select", "select with %.3f timeout", timelen);
	}
    }

    /*
     * perform the select call
     */
    if (n <= 0) {

	/*
	 * case - nothing to select on
	 */
	struct timeval idle;	/* idle select timeout */
	struct timeval *pidle;	/* sidle elect timeout arg or NULL */

	idle.tv_sec = double_to_tv_sec(IDLE_TIMEOUT);
	idle.tv_usec = double_to_tv_usec(IDLE_TIMEOUT);
	pidle = &idle;
	dbg(2, "chan_select", "empty select, now %.3f timeout", IDLE_TIMEOUT);
	ret = select(0, NULL, NULL, NULL, pidle);

    } else {

	/*
	 * case - something to select on
	 */
	ret = select(n, &rd, &wr, &ex, ptimeout);
    }

    /*
     * setup for post-select debug
     */
    if (dbg_lvl >= 2) {
	stop = right_now();
	dbg(3, "chan_select", "select returned %d after %.3f seconds",
	    ret, stop - start);
    }

    /*
     * quick exit if nothing was found
     */
    if (ret <= 0) {
	return ret;
    }

    /*
     * perform operation on all selected channels in a circular fashion
     */
    looped = FALSE;
    for (i = 0, action = ret; i < chanindx_len && action > 0; ++i) {
	int index;	/* channel index to operate on */
	chancycle cycle;	/* type of select cycle we are processing */

	/* cycle thru descriptor index values starting with last one */
	j = (i + last_desc) % chanindx_len;
	if (j >= n) {
	    if (looped) {
		/* been here, done that, time to stop */
		break;
	    }
	    /* advance so that 'j' loops around end and comes up 0 */
	    i = ((chanindx_len - last_desc - 1) % chanindx_len);
	    looped = TRUE;
	    continue;
	}

	/*
	 * ignore if it does not belong to a channel
	 */
	index = chanindx[j];
	if (index < 0 || index >= chanlen) {
	    continue;
	}

	/*
	 * Determine if the select mask says we can perform an
	 * operation.
	 *
	 * If we have multiple bits for the same descriptor, we
	 * will process the execution over the write, and the write
	 * over the read.
	 */
	if (FD_ISSET(j, &ex)) {
	    cycle = CYCLE_SELECTEXECPT;
	} else if (FD_ISSET(j, &wr)) {
	    cycle = CYCLE_SELECTWRITE;
	} else if (FD_ISSET(j, &rd)) {
	    cycle = CYCLE_SELECTREAD;
	} else {
	    continue;
	}

	/*
	 * We will perform a select based operation on the channel
	 */
	chan_indx_op(index, cycle);
	--action;
    }

    /*
     * advance start position for next time
     */
    if (chanindx_len != 0) {
	last_desc = (last_desc + 1) % chanindx_len;
    }
    dbg(1, "chan_select", "pool level: (%.3f) %u", pool_frac(), pool_level());
    return ret;
}


/*
 * chan_cycle - perform a complete cycle on all potential channels
 *
 * given:
 *      timeout         max seconds to, 0 ==> immediate return, <0 ==> forever
 */
void
chan_cycle(double timeout)
{
    static int last_indx = 0;	/* last index searched */
    int i;
    int j;

    /*
     * Prepare for processing the channels
     */
    about_now = right_now();
    chan_cycle_timeout = timeout;

    /*
     * perform pre-select processing on channels that need it
     */
    dbg(2, "chan_cycle", "cycle: %lld, pre-select processing", op_cycle);
    for (i = 0; i < chanlen; ++i) {

	/* cycle thru channels starting with last index */
	j = (i + last_indx) % chanlen;

	/*
	 * check for invalid states
	 */
	if (ch[j].common.curstate < 0 || ch[j].common.curstate > LAST_STATE) {
	    warn("chan_cycle", "chan[%d]: invalid current state: %d",
		 j, (int)ch[j].common.curstate);
	    halt_chan(&(ch[j]));
	    continue;
	}
	if (ch[j].common.nxtstate < 0 || ch[j].common.nxtstate > LAST_STATE) {
	    warn("chan_cycle", "chan[%d]: invalid next state: %d",
		 j, (int)ch[j].common.nxtstate);
	    halt_chan(&(ch[j]));
	    continue;
	}

	/* cycle processing */
	switch (ch[j].common.type) {
	case TYPE_LISTENER:
	    listener_pre_select_op(&(ch[j].listener));
	    break;
	case TYPE_CLIENT:
	    client_pre_select_op(&(ch[j].client));
	    break;
	case TYPE_CHAOS:
	    chaos_pre_select_op(&(ch[j].chaos));
	    break;
	default:
	    break;
	}
    }

    /*
     * Update the timeout that may have been altered by calls
     * to need_cycle_before() during the above pre-select operations
     */
    if (timeout != chan_cycle_timeout && dbg_lvl >= 2) {
	if (timeout < 0.0 && chan_cycle_timeout < 0.0) {
	    dbg(2, "chan_cycle",
		"changing cycle timeout from: forever to: forever");
	} else if (timeout < 0.0) {
	    dbg(2, "chan_cycle",
		"changing cycle timeout from: forever to: %.3f",
		chan_cycle_timeout);
	} else if (chan_cycle_timeout < 0.0) {
	    dbg(2, "chan_cycle",
		"changing cycle timeout from: %.3f to: forever", timeout);
	} else {
	    dbg(2, "chan_cycle",
		"changing cycle timeout from: %.3f to: %.3f",
		timeout, chan_cycle_timeout);
	}
    }
    timeout = chan_cycle_timeout;

    /*
     * advance start position for next time
     */
    if (chanlen != 0) {
	last_indx = (last_indx + 1) % chanlen;
    }

    /*
     * perform select based processing
     */
    about_now = right_now();
    chan_select(timeout);
    if (dbg_lvl >= 4) {
	if (timeout < 0.0) {
	    dbg(4, "chan_cycle", " return time: no specific time");
	} else {
	    dbg(4, "chan_cycle", " return time: %.3f", right_now() - timeout);
	}
    }

    /*
     * count the channel cycle
     */
    ++op_cycle;
    return;
}


/*
 * set_chanindx - record the channel index
 *
 * given:
 *      indx    channel index that is using fd
 *      fd      descriptor of the channel
 *
 * NOTE: This function silently ignores out of range channels and
 *       descriptors.
 */
void
set_chanindx(int indx, int fd)
{
    /*
     * firewall
     */
    if (indx < 0) {
	warn("set_chanindx", "indx: %d < 0", indx);
	return;
    }
    if (indx >= chanlen) {
	warn("set_chanindx", "indx: %d >= %d", indx, chanlen);
	return;
    }
    if (fd < 0) {
	warn("set_chanindx", "fd: %d < 0", fd);
	return;
    }
    if (fd >= chanindx_len) {
	warn("set_chanindx", "fd: %d >= %d", fd, chanindx_len);
	return;
    }

    /*
     * set the channel index
     */
    chanindx[fd] = indx;
    return;
}


/*
 * clear_chanindx - clear a recorded the channel index
 *
 * given:
 *      indx    channel index that was using fd
 *      fd      descriptor of the channel
 *
 * NOTE: This function silently ignores out of range channels and
 *       descriptors or descriptors that are not assigned to them.
 */
void
clear_chanindx(int indx, int fd)
{
    /*
     * firewall
     */
    if (indx < 0) {
	warn("clear_chanindx", "indx: %d < 0", indx);
	return;
    }
    if (indx >= chanlen) {
	warn("clear_chanindx", "indx: %d >= %d", indx, chanlen);
	return;
    }
    if (fd < 0) {
	warn("clear_chanindx", "fd: %d < 0", fd);
	return;
    }
    if (fd >= chanindx_len) {
	warn("clear_chanindx", "fd: %d >= %d", fd, chanindx_len);
	return;
    }

    /*
     * set the channel index
     */
    if (chanindx[fd] == indx) {
	chanindx[fd] = -1;
    } else {
	warn("clear_chanindx", "indx: %d not assigned to fd: %d", indx, fd);
    }
    return;
}


/*
 * need_cycle_before - register a need to cycle before a given time
 *
 * given:
 *      indx    channel index that is registering a cycle time
 *      when    >  0.0 ==> cycle before this time,
 *              <= 0.0 ==> no special cycle time reqired
 *
 * The chan_cycle() set chan_cycle_timeout to its timeout request.
 * During the pre-select processing, chan[indx] can request a timeout
 * before a given time (when if > 0.0), or specify no preference
 * (when == 0.0).  This function compares the current timeout
 * plan with the chan request and changes it if needed.
 * Sometime after the pre-select processing, chan_cycle() will
 * pick up the (possibly modified) timeout plan and implement it.
 */
void
need_cycle_before(int indx, double when)
{
    /*
     * case: no special cycle time required - leave timeout alone
     */
    if (when <= 0.0) {
	dbg(5, "need_cycle_before", "chan[%d]: no cycle timeout request",
	    indx);

	/*
	 * case: current plan is to cycle now - we cannot cycle any sooner
	 */
    } else if (chan_cycle_timeout == 0.0) {
	dbg(5, "need_cycle_before",
	    "chan[%d]: already immediate timeout, no need to change it", indx);

	/*
	 * case: current plan is to wait forever - see if that needs to change
	 */
    } else if (chan_cycle_timeout < 0.0) {

	/*
	 * case: change wait forever to a definite timeout
	 */
	if (when <= about_now) {
	    dbg(4, "need_cycle_before",
		"chan[%d]: chaning timeout from forever to immedate", indx);
	    chan_cycle_timeout = 0.0;
	} else {
	    dbg(4, "need_cycle_before",
		"chan[%d]: chaning timeout from forever to %.3f",
		indx, when - about_now);
	    chan_cycle_timeout = when - about_now;
	}

	/*
	 * case: channel is requesting a shorter timeout - shorten it
	 */
    } else if (chan_cycle_timeout + about_now > when) {

	/*
	 * case: change from definite timeout to immedate
	 */
	if (when <= about_now) {
	    dbg(4, "need_cycle_before",
		"chan[%d]: chaning timeout from %.3f to immedate",
		indx, chan_cycle_timeout);
	    chan_cycle_timeout = 0.0;

	    /*
	     * case: change from definite timeout to shorter timeout
	     */
	} else {
	    dbg(4, "need_cycle_before",
		"chan[%d]: shortening timeout from %.3f to %.3f",
		indx, chan_cycle_timeout, when - about_now);
	    chan_cycle_timeout = when - about_now;
	}

	/*
	 * case: channel is requesting longer timeout - no change needed
	 */
    } else {
	dbg(5, "need_cycle_before",
	    "chan[%d]: timeout %.3f already shorter than channel need",
	    indx, chan_cycle_timeout);
    }
    return;
}


/*
 * chan_close - close down all channels
 */
void
chan_close(void)
{
    int j;

    /*
     * Prepare for processing the channels
     */
    about_now = right_now();

    /*
     * perform pre-select processing on channels that need it
     */
    dbg(1, "chan_close", "cycle: %lld, closing all channels", op_cycle);
    for (j = 0; j < chanlen; ++j) {

	/*
	 * check for invalid states
	 */
	if (ch[j].common.curstate < 0 || ch[j].common.curstate > LAST_STATE) {
	    warn("chan_cycle", "chan[%d]: invalid current state: %d",
		 j, (int)ch[j].common.curstate);
	    halt_chan(&(ch[j]));
	    continue;
	}
	if (ch[j].common.nxtstate < 0 || ch[j].common.nxtstate > LAST_STATE) {
	    warn("chan_cycle", "chan[%d]: invalid next state: %d",
		 j, (int)ch[j].common.nxtstate);
	    halt_chan(&(ch[j]));
	    continue;
	}

	/*
	 * ignore channels that are already closed
	 */
	if (ch[j].common.curstate == CLOSE) {
	    dbg(2, "chan_close", "chan[%d]: already closed", j);
	    continue;
	}

	/* cycle processing */
	switch (ch[j].common.type) {
	case TYPE_LISTENER:
	    dbg(1, "chan_close", "chan[%d]: will force listener closed", j);
	    close_listener(&(ch[j].listener));
	    break;
	case TYPE_CLIENT:
	    dbg(1, "chan_close", "chan[%d]: will force client closed", j);
	    close_client(&(ch[j].client));
	    break;
	case TYPE_CHAOS:
	    dbg(1, "chan_close", "chan[%d]: will force chaos closed", j);
	    close_chaos(&(ch[j].chaos));
	    break;
	default:
	    break;
	}
    }
    dbg(1, "chan_close", "cycle: %lld, forced all channels closed", op_cycle);

    /*
     * count the channel cycle
     */
    about_now = right_now();
    ++op_cycle;
    return;
}


/*
 * free_allchan - free all channels
 */
void
free_allchan(void)
{
    if (ch != NULL) {
	free(ch);
	ch = NULL;
    }
    if (chanindx != NULL) {
    	free(chanindx);
	chanindx = NULL;
	chanindx_len = 0;
    }
}
