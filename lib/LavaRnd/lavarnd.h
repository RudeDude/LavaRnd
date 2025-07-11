/*
 * LavaRnd - LavaRnd core algorithms
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: lavarnd.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


#if !defined(__LAVARND_LAVARND_LAVARND_H__)
#  define __LAVARND_LAVARND_LAVARND_H__


#  include "sha1.h"


/*
 * external functions - lavarnd.c
 */
extern void lavarnd_cleanup(void);
extern void *lava_turn(void *input_arg, int len, int nway, void *output_arg);
extern int lavarnd_turn_len(int inlen, int nway);
extern int lavarnd_blk_turn_len(int inlen, int nway);
extern void *lava_blk_turn(void *input, int len, int nway, void *output);
extern int lavarnd_salt_blk_turn_len(int salt_len, int inlen, int nway);
extern void *lava_salt_blk_turn(void *salt_arg, int salt_len,
				void *input, int len, int nway, void *output);
extern int lava_nway_value(int length, double rate);
extern int lavarnd_len(int inlen, double rate);
extern int lavarnd(int use_salt, void *input, int inlen, double rate,
		   void *output, int outlen);

#endif /* __LAVARND_LAVARND_LAVARND_H__ */
