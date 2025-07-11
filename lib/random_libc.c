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
 * @(#) $Id: random_libc.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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

#define MAX_RAND 0x7fffffff
#define MAX_RANDOM 0x7fffffff

#include "LavaRnd/lavaerr.h"
#include "LavaRnd/fetchlava.h"
#include "LavaRnd/random.h"
#include "LavaRnd/random_libc.h"

#if defined(DMALLOC)
#include <dmalloc.h>
#endif


/*
 * silly state - initstate() and setstate() want to return something,
 *		 so we have then return a pointer to this silly data.
 */
#define SILLY_SIZE 256
static char silly[SILLY_SIZE] =
/*          1         2         3         4         5         6     */
/* 123456789 123456789 123456789 123456789 123456789 123456789 1234 */
  "This is not a state buffer.  This fake state will satisfy libc's"
  " return requirement. This is just some silly meaningless static "
  "data. chongo was here. 2^23209-1 is prime. Fizzbin!!! Check out "
  "http://www.isthe.com/chongo and http://www.LavaRnd.org someday.\n";


/*
 * seed48 - internal buffer required by seed48() functionality
 */
static unsigned short int seed48_return[] = { 19937, 21701, 23209 };


/*
 * other static data
 */
static int preload_once = 0;	/* 1 ==> already pre_loaded */


/*
 * fake_seed - fake a seed process by preloading on the first call
 */
static void
fake_seed(void)
{
    /*
     * preload as a way of 'seeding'
     *
     * We will always configure the LavaRnd library system.
     *
     * We only want to preload the private s100 generator in cases
     * where we might need it.  The LAVACALL_S100_* callbacks always
     * use the private s100 generator so we might as well preload it
     * from the beginning.
     *
     * In all other cases we simply setup the initial lavapool buffer
     * with LavaRnd data.  Because the LavaRnd library system is
     * also configured, the s100_preseed_amt configuration rules
     * about when to later preload the private s100 generator apply.
     */
    switch ((int)lava_callback) {
    case CASE_LAVACALL_S100_ANY:
    case CASE_LAVACALL_S100_HIGH:
    case CASE_LAVACALL_S100_MED:
	(void) lava_preload(-1, TRUE);
	break;
    default:
	(void) lava_preload(0, FALSE);
	break;
    }
    preload_once = 1;
    return;
}


/*
 * rand - libc interface to return a [0,2^31) random value
 *
 * NOTE: We do not need to touch lastop_errno because random32()
 * 	 will do it for us.
 */
int
rand(void)
{
    /*
     * return a 31 bit random value
     */
    return ((int)random32() & MAX_RAND);
}


/*
 * random - libc interface to return a [0,2^31) random value
 *
 * NOTE: We do not need to touch lastop_errno because random32()
 * 	 will do it for us.
 */
long int
random(void)
{
    /*
     * return a 31 bit random value
     */
    return ((long int)random32() & MAX_RANDOM);
}


/*
 * drand48 - libc interface to return a [0.0, 1.0) random value
 *
 * NOTE: We do not need to touch lastop_errno because drandom()
 * 	 (via random64()) will do it for us.
 */
double
drand48(void)
{
    /*
     * return a random double
     */
    return drandom();
}


/*
 * erand48 - libc interface to return a [0.0, 1.0) random value
 *
 * We ignore the generator state argument and just preload if our first call.
 *
 * NOTE: We do not need to touch lastop_errno because drandom()
 * 	 (via random64()) will do it for us.
 */
/*ARGSUSED*/
double
erand48(unsigned short int xsubi[3])
{
    /*
     * preload if first call
     */
    if (! preload_once) {
	fake_seed();
    }

    /*
     * return a random double
     */
    return drandom();
}


/*
 * lrand48 - libc interface to return a [0,2^31) random value
 *
 * NOTE: We do not need to touch lastop_errno because random32()
 * 	 will do it for us.
 */
long int
lrand48(void)
{
    /*
     * return a 31 bit random value
     */
    return ((long int)random32() & MAX_RANDOM);
}


/*
 * nrand48 - libc interface to return a [0,2^31) random value
 *
 * We ignore the generator state argument and just preload if our first call.
 *
 * NOTE: We do not need to touch lastop_errno because random32()
 * 	 will do it for us.
 */
/*ARGSUSED*/
long int
nrand48(unsigned short int xsubi[3])
{
    /*
     * preload if first call
     */
    if (! preload_once) {
	fake_seed();
    }

    /*
     * return a 31 bit random value
     */
    return ((long int)random32() & MAX_RANDOM);
}


/*
 * mrand48 - libc interface to return a [-2^31,2^31) random value
 *
 * NOTE: We do not need to touch lastop_errno because random32()
 * 	 will do it for us.
 */
long int
mrand48(void)
{
    /*
     * return a signed 32 bit random value
     */
    return (long int)random32();
}


/*
 * jrand48 - libc interface to return a [-2^31,2^31) random value
 *
 * We ignore the generator state argument and just preload if our first call.
 *
 * NOTE: We do not need to touch lastop_errno because random32()
 * 	 will do it for us.
 */
/*ARGSUSED*/
long int
jrand48(unsigned short int xsubi[3])
{
    /*
     * preload if first call
     */
    if (! preload_once) {
	fake_seed();
    }

    /*
     * return a signed 32 bit random value
     */
    return (long int)random32();
}


/*
 * srand - stub for libc srand
 *
 * We ignore the seed argument.  We ensure that everything is setup and full.
 */
void
srand(unsigned int seed)
{
    if (! preload_once) {
	fake_seed();
    }
    return;
}


/*
 * srandom - stub for libc srandom
 *
 * We ignore the seed argument and just preload if our first call.
 */
/*ARGSUSED*/
void
srandom(unsigned int seed)
{
    if (! preload_once) {
	fake_seed();
    }
    return;
}


/*
 * initstate - stub for libc initstate
 *
 * We ignore the args.  We ensure that everything is setup and full.
 * Since this function must return a pointer, we return a pointer
 * to static data containing silly things.
 */
/*ARGSUSED*/
char *
initstate(unsigned int seed, char *state, size_t n)
{
    if (! preload_once) {
	fake_seed();
    }
    return silly;
}


/*
 * setstate - stub for libc setstate
 *
 * We ignore the args.  We ensure that everything is setup and full.
 * Since this function must return a pointer, we return a pointer
 * to static data containing silly things.
 */
/*ARGSUSED*/
char *
setstate(char *state)
{
    if (! preload_once) {
	fake_seed();
    }
    return silly;
}


/*
 * srand48 - stub for libc srand48
 *
 * We ignore the seed argument.  We ensure that everything is setup and full.
 */
/*ARGSUSED*/
void
srand48(long int seedval)
{
    if (! preload_once) {
	fake_seed();
    }
    return;
}


/*
 * seed48 - stub for libc seed48
 *
 * We ignore the seed argument.  We ensure that everything is setup and full.
 */
/*ARGSUSED*/
unsigned short int *
seed48(unsigned short int seed16v[3])
{
    if (! preload_once) {
	fake_seed();
    }
    return &(seed48_return[0]);
}


/*
 * lcong48 - stub for libc lcong48
 *
 * We ignore the seed argument.  We ensure that everything is setup and full.
 */
/*ARGSUSED*/
void
lcong48(unsigned short int param[7])
{
    if (! preload_once) {
	fake_seed();
    }
    return;
}
