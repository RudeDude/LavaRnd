/*
 * lavaquality - LavaRnd data quality
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: lavaquality.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


#if !defined(__LAVARND_LAVAQUALITY_H__)
#  define __LAVARND_LAVAQUALITY_H__


/*
 * LavaRnd quality
 */
enum e_lavaqual {
    LAVA_QUAL_NONE = 0,	/* no data, or request not completely filled */
    LAVA_QUAL_S100LOW,		/* s100 PRNG without LavaRnd seed */
    LAVA_QUAL_S100MED,		/* s100 PRNG, LavaRnd seeded, but stale seed */
    LAVA_QUAL_S100HIGH,		/* s100 PRNG, LavaRnd seeded, current seed */
    LAVA_QUAL_LAVARND		/* LavaRnd cryptographically strong data */
};
typedef enum e_lavaqual lavaqual;


#endif /* __LAVARND_LAVAQUALITY_H__ */
