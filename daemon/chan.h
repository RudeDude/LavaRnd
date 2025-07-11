/*
 * chan - channel state
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: chan.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


#if !defined(__CHAN_H__)
#  define __CHAN_H__

#  include <sys/types.h>
#  include <sys/time.h>
#  include <unistd.h>

#  include "LavaRnd/lavacam.h"


/*
 * channel type
 *
 * A channel can be a client requesting LavaRnd data, or
 * a listening socket accepting connections and creating new clients, or
 * a socket communicating with a chaotic source via HTTP/1.0 protocol.
 */
enum chantype_t {
    TYPE_NONE = 0,	/* not initialized or file descriptor unassigned */
    TYPE_LISTENER,		/* listening socket that accepts new CLIENTs */
    TYPE_CLIENT,		/* client data requests via count and binary return */
    TYPE_CHAOS,			/* chaotic source socket via HTTP/1.0 protocol */
    TYPE_SYSTEM			/* system related descriptor */
};
typedef enum chantype_t chantype;


/*
 * channel op cycle
 *
 * When performing an operation, we give the operation function a condition
 * that explains where in the operation call it is being called.
 */
enum chancycle_t {
    CYCLE_NONE = 0,	/* not within the channel cycle */
    CYCLE_PRESELECT,		/* before select mask processing */
    CYCLE_SELECTEXECPT,		/* select ready on exception */
    CYCLE_SELECTREAD,		/* select ready on read */
    CYCLE_SELECTWRITE		/* select ready on write */
};
typedef enum chancycle_t chancycle;


/*
 * current channel state
 *
 * NOTE: We depend on this enum array starting allocation (ALLOCED) with
 *       the numeric value of 0.  We depend on the highest enum value
 *       to be #defined to LAST_STATE.
 *
 * NOTE: One should not re-order these enums.  The listener.c, client.c,
 *       chaos.c and chan.c files have state arrays that depend on this
 *       enum sequence.  The enum sequence should not have any gaps.
 *
 * NOTE: In a state change, the channel state is usually last item to change.
 *       For example when opening a CLOSE or ALLOCED channel, the last
 *       thing that is performed is the state is changed to OPEN.  Typically
 *       the OPEN state is selectable for READ or WRITE.  However the channel
 *       state will remain in OPEN until the first read or write operation
 *       is performed.  Think of the state as the state of the last
 *       completed operation, not the current in-process operation.
 */
enum chanstate_e {
    ALLOCED = 0,	/* allocated and zeroed but not initialized */
    ACCEPT,			/* socket is accepting new connections */
    OPEN,			/* socket created/opened and channel initialized */
    READ,			/* start of or in the middle of reading data */
    GATHER,			/* gathering data to write */
    WRITE,			/* start of or in the middle of writing data */
    CLOSE,			/* closed socket, ready to re-initialize */
    HALT			/* error state unable to process further */
      /* NOTE: LAST_STATE must #define to the last enum value above */
};
typedef enum chanstate_e chanstate;

#  define LAST_STATE ((int)(HALT))	/* highest enum value */
#  define VALID_STATE(x) ((int)(x) >= 0 && (int)(x) <= LAST_STATE)
#  define STATE_NAME(x) (VALID_STATE(x) ? state_name[(int)(x)] : "UNKNOWN_STATE")


/*
 * listener - listen and accept new client connections
 */
struct listener_s {
    /* must be first 5 - must match common typedef */
    int indx;		/* channel index */
    chantype type;	/* should be TYPE_LISTENER */
    chanstate curstate;	/* current state of this client */
    chanstate nxtstate;	/* next state of this client */
    int fd;		/* open socket descriptor if >= 0 */
    /**/
    u_int64_t count;	/* clients accepted on the channel */
    double last_op;	/* time of last successful accept operation */
};
typedef struct listener_s listener;


/*
 * client - request for LavaRnd data
 */
struct client_s {
    /* must be first 5 - must match common typedef */
    int indx;		/* channel index */
    chantype type;	/* should be TYPE_CLIENT */
    chanstate curstate;	/* current state of this client */
    chanstate nxtstate;	/* next state of this client */
    int fd;		/* open socket descriptor if >= 0 */
    /**/
    double open_op;	/* time of when client was opened */
    double last_op;	/* time of last successful accept operation */
    double timelimit;	/* READ+WRITE time in sec if > 0.0 */
    double timeout;	/* timeout time if opened and > 0.0 */
    long request;	/* request size if GATHER or WRITE */
    long readcnt;	/* total characters read if READ */
    long gathercnt;	/* random octets gathered if GATHER */
    long writecnt;	/* total characters written if WRITE */
    char readbuf[LAVA_REQBUFLEN + 1];	/* read length request input buffer */
    u_int8_t *random;	/* if != NULL, LavaRnd data to deliver */
};
typedef struct client_s client;


/*
 * chaos - request and obtain chaotic data via HTTP/1.0 protocol
 */
struct chaos_s {
    /* must be first 5 - must match common typedef */
    int indx;		/* channel index */
    chantype type;	/* should be TYPE_CHAOS */
    chanstate curstate;	/* current state of this client */
    chanstate nxtstate;	/* next state of this client */
    int fd;		/* open socket descriptor if >= 0 */
    /**/
    u_int64_t count;	/* LavaRnd octets processed by the channel */
    double last_op;	/* time of last successful read operation */
    pid_t pid;		/* pid of process, 0 ==> driver, no process */
    int fast_select;	/* 1 ==> force select on next chan cycle */
    /* driver related state */
    int driver;		/* TRUE ==> using driver, FALSE ==> command */
    int driver_type;	/* driver type or LAVACAM_ERR_TYPE */
    union lavacam cam;	/* device state and settings */
    struct opsize siz;	/* how and where to read from device */
    struct lavacam_flag flag;	/* flags set via lavacam_argv() */
    double next_file;	/* >0 ==> time of next savefile */
};
typedef struct chaos_s chaos;

#  define MAX_ARG_CNT (32)	/* max args in lavaurl command string */


/*
 * common - common initial elements to all types of channels
 */
struct common_s {
    /* must match first 5 of struct listener_s, client_s and chaos_s */
    int indx;	/* channel index */
    chantype type;	/* channel type */
    chanstate curstate;	/* current state of this client */
    chanstate nxtstate;	/* next state of this client */
    int fd;	/* open socket descriptor if >= 0 */
     /**/
};
typedef struct common_s common;


/*
 * channel
 */
union chan_u {
    common common;	/* elements common to call channel types */
    listener listener;	/* listen and accept new client connections */
    client client;	/* request for LavaRnd data */
    chaos chaos;	/* chaotic data via HTTP/1.0 protocol */
};
typedef union chan_u chan;


/*
 * channel descriptor index
 */
#  define LAVA_UNUSED_FD (-1)	/* no channel associated with descriptor */
#  define LAVA_SYS_FD (-2)	/* descriptor in use by system, not channel */


/*
 * chan.c - external functions
 */
extern u_int32_t chanindx_len;
extern const char *const state_name[];
extern double about_now;
extern void alloc_chanindx(void);
extern chan *mk_chan(chantype type);
extern void halt_chan(chan *ch);
extern chan *find_chan(chantype type, chanstate state);
extern void chan_cycle(double timeout);
extern void set_chanindx(int indx, int fd);
extern void clear_chanindx(int indx, int fd);
extern void need_cycle_before(int indx, double when);
extern void chan_close(void);
extern void free_allchan(void);


/*
 * listener.c - external functions
 */
extern void do_listener_op(listener *ch, chancycle cycle);
extern int listener_select_mask(listener *ch, fd_set * rd, fd_set * wr,
				fd_set * ex);
extern void listener_pre_select_op(listener *ch);
extern chan *mk_listener(listener *ch);
extern chan *mk_open_listener(void);
extern void accept_listener(listener *ch);
extern void close_listener(listener *ch);


/*
 * client.c - external functions
 */
extern void do_client_op(client *ch, chancycle cycle);
extern int client_select_mask(client *ch, fd_set * rd, fd_set * wr,
			      fd_set * ex);
extern void client_pre_select_op(client *ch);
extern chan *mk_client(client *ch);
extern chan *mk_open_client(int fd);
extern void close_client(client *ch);


/*
 * chaos.c - external functions
 */
extern void do_chaos_op(chaos *ch, chancycle cycle);
extern int chaos_select_mask(chaos *ch, fd_set * rd, fd_set * wr, fd_set * ex);
extern void chaos_pre_select_op(chaos *ch);
extern chan *mk_chaos(chaos *ch);
extern chan *mk_open_chaos(void);
extern void close_chaos(chaos *ch);
extern int ready_to_frame_dump(chaos *ch);


#endif
