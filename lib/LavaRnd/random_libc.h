/*
 * random_libc - high level random number libc interface overrides
 *
 * This is a high level interface to random number services.  These
 * functions conflict with random number services found in libc.
 * That is, these functions override common libc generator functions.
 *
 * One difference is that seed arguments are ignored.  While the
 * seed functions (e.g., srandom(), srand(), ...) will cause the
 * lava_preload() function to prepare for later use, the seed
 * arguments given to them are ignored.  And at the risk of stating
 * the obvious: the random number functions (e.g., random(), rand())
 * while they return random values in the same range as libc,
 * these functions returns different values than the libc functions.
 *
 * We presume that these differences are desired ... otherwise you
 * would not be seeking cryprogrpahically strong random numbers
 * derived from chaotic sources.   Besides the worst valid values
 * that this code can return is much better than the best that
 * libc dreams of returning!  :-)
 *
 * This code is placed in the liblava_libc.a library and not other
 * libraries so that one can choose to override libc functionality
 * or not.  This code requires functions from one of the other
 * liblava_XYZ.a libraries.
 *
 * See random.c for non-conflictive functions.
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: random_libc.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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


#if !defined(__LAVARND_RANDOM_LIBC_H__)
#  define __LAVARND_RANDOM_LIBC_H__


/*
 * external functions
 */
#  undef rand
extern int rand(void);

#  undef random
extern long int random(void);

#  undef drand48
extern double drand48(void);

#  undef erand48
extern double erand48(unsigned short int xsubi[]);

#  undef lrand48
extern long int lrand48(void);

#  undef nrand48
extern long int nrand48(unsigned short int xsubi[]);

#  undef mrand48
extern long int mrand48(void);

#  undef jrand48
extern long int jrand48(unsigned short int xsubi[]);

#  undef srand
extern void srand(unsigned int seed);

#  undef srandom
extern void srandom(unsigned int seed);

#  undef initstate
extern char *initstate(unsigned int seed, char *state, size_t n);

#  undef setstate
extern char *setstate(char *state);

#  undef srand48
extern void srand48(long int seedval);

#  undef seed48
extern unsigned short int *seed48(unsigned short int seed16v[]);

#  undef lcong48
extern void lcong48(unsigned short int param[]);


#endif /* __LAVARND_RANDOM_LIBC_H__ */
