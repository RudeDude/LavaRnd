/*
 * cfg_lavapool - private lavapool configuration information
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: cfg_lavapool.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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

#if !defined(__CFG_LAVAPOOL_H__)
#define __CFG_LAVAPOOL_H__


/*
 * cfg.lavapool - lavapool daemon configuration information
 *
 * these values are loaded from a parsed cfg.lavapool file
 */
#define LAVA_LAVAPOOL_CFG "/etc/LavaRnd/cfg.lavapool"	/* default cfg file */
#define LAVA_DEF_FASTPOOL (1*1024*1024)	  /* def fast pool filling level */
#define LAVA_DEF_SLOWPOOL (4*1024*1024)	  /* def slow filling level */
#define LAVA_DEF_POOLSIZE (8*1024*1024)	  /* def random daemon pool size */
#define LAVA_DEF_FAST_CYCLE (0.033333)	  /* def fastest chan fill cycle */
#define LAVA_DEF_SLOW_CYCLE (20.0)	  /* def slowest chan fill cycle */
#define LAVA_DEF_MAXCLINETS (16)	  /* def max number of clients */
#define LAVA_DEF_TIMEOUT (6.0)		  /* client timeout in seconds */
#define LAVA_DEF_USE_PREFIX (1)	  	  /* def no system stuff prefix */
struct cfg_lavapool {
    char *chaos;		/* chaos source (command or driver) */
    int32_t fastpool;		/* pool level below which pool fills fast */
    int32_t slowpool;		/* pool level above which pool fills slowly */
    int32_t poolsize;		/* size of lava pool */
    double fast_cycle;		/* fastest chan fill cycle in seconds */
    double slow_cycle;		/* slowest chan fill cycle in seconds */
    int32_t maxclients;		/* max clients allowed, 0 => no limit */
    double timeout;		/* seconds to timeout if > 0.0 */
    int prefix;			/* 0==>no system stuff for URL content prefix */
};


/*
 * external cfg.lavapool configuration functions
 */
extern struct cfg_lavapool cfg_lavapool;   /* current cfg.lavapool cfg */
extern int config_priv(char *, struct cfg_lavapool *);
extern struct cfg_lavapool *dup_cfg_lavapool(struct cfg_lavapool *,
					     struct cfg_lavapool *);
extern void free_cfg_lavapool(struct cfg_lavapool *);


#endif /* __CFG_LAVAPOOL_H__ */
