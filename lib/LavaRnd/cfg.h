/*
 * cfg - lavapool configuration information
 *
 * @(#) $Revision: 10.2 $
 * @(#) $Id: cfg.h,v 10.2 2003/11/09 22:37:03 lavarnd Exp $
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


#if !defined(__LAVARND_CFG_H__)
#define __LAVARND_CFG_H__


/*
 * default interface values
 */
#define LAVA_RANDOM_CFG "/etc/LavaRnd/cfg.random" /* def cfg.random file */
/**/
#define LAVA_DEF_LAVAPOOL "127.0.0.1:23209" /* default lavapool socket */
#define LAVA_DEF_MALLOCED_STR 0		/* lavapool string is a constant */
#define LAVA_DEF_MAXREQ 65536		/* def max request allowed */
#define LAVA_MAX_LEN_ARG 1048576	/* largest lavapool transfer allowed */
#define LAVA_DEF_EXIT_RETRIES 3		/* def retry connections if exit */
#define LAVA_DEF_EXIT_MIN_WAIT 2.0	/* def min retry timeout if exit */
#define LAVA_DEF_EXIT_MAX_WAIT 6.0	/* def max retry timeout if exit */
#define LAVA_DEF_RETRY_MIN_WAIT 2.0	/* def min retry timeout if retry */
#define LAVA_DEF_RETRY_INC_WAIT 2.0	/* def timeout increment if retry */
#define LAVA_DEF_RETRY_MAX_WAIT 6.0	/* def max retry timeout if retry */
#define LAVA_DEF_RETURN_RETRIES 3	/* def retry connections if return */
#define LAVA_DEF_RETURN_MIN_WAIT 2.0	/* def min retry timeout if return */
#define LAVA_DEF_RETURN_MAX_WAIT 6.0	/* def max retry timeout if return */
#define LAVA_DEF_S100_RETRIES 3		/* def retry connections seeding s100 */
#define LAVA_DEF_S100_PRESEED_AMT 1025	/* when to s100 seed if fallback used */
#define LAVA_DEF_S100_MIN_WAIT 2.0	/* def min retry timeout seeding s100 */
#define LAVA_DEF_S100_MAX_WAIT 6.0	/* def max retry timeout seeding s100 */
#define LAVA_DEF_CALLBACK_WAIT 4.0	/* def initial timeout if callback */
#define LAVA_DEF_PRELOAD 2.0		/* timeout's in lava_preload() */

/*
 * cfg.random - random number interface to the lavapool daemon
 */
struct cfg_random {
    char *lavapool;	   	/* name of lavapool socket */
    int malloced_str;		/* 1 ==> malloced strings (i.e., lavapool) */
    int32_t maxrequest; 	/* maximum size allowed allowed */
    int32_t exit_retries;	/* retry connection if LAVACALL_LAVA_EXIT */
    double exit_min_wait;	/* min retry timeout if LAVACALL_LAVA_EXIT */
    double exit_max_wait;	/* max retry timeout if LAVACALL_LAVA_EXIT */
    double retry_min_wait;	/* min retry timeout if LAVACALL_LAVA_RETRY */
    double retry_inc_wait;	/* inc retry timeout if LAVACALL_LAVA_RETRY */
    double retry_max_wait;	/* max retry timeout if LAVACALL_LAVA_RETRY */
    int32_t return_retries;	/* retry connection if LAVACALL_LAVA_RETURN */
    double return_min_wait;	/* min retry timeout if LAVACALL_LAVA_RETURN */
    double return_max_wait;	/* max retry timeout if LAVACALL_LAVA_RETURN */
    int32_t s100_retries;	/* retry connection if seeding s100 */
    int32_t s100_preseed_amt;	/* output amt before seeding s100 fallback */
    double s100_min_wait;	/* min retry timeout if seeding s100 */
    double s100_max_wait;	/* max retry timeout if seeding s100 */
    double def_callback_wait;	/* default initial timeout if callback */
    double preload_wait;	/* time to wait during lava_preload() */
};


/*
 * master interface configuration structure
 */
extern struct cfg_random cfg_random;


#endif /* __LAVARND_CFG_H__ */
