/*
 * lavarnd - core LavaRnd algorithm
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: lavarnd.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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

/*
 * WARNING: Subtle magic!!!
 *
 * This is code is at the core of the LavaRnd algorithm.  Tread with care
 * if you change anything here.  A seemingly innocent change may result
 * in bogosity that is difficult to detect.  Be sure you REALLY know
 * what you are doing before you touch anything in this file.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "LavaRnd/sha1.h"
#include "LavaRnd/lavarnd.h"
#include "LavaRnd/lavaerr.h"
#include "LavaRnd/sysstuff.h"

#if defined(DMALLOC)
#include <dmalloc.h>
#endif


/*
 * firewall
 */
#if SHA_DIGESTSIZE != 20
     NOTE: The loops inside lava_xor_fold_rot() and lavarnd()
           requires SHA_DIGESTLONG == 5 and SHA_DIGESTSIZE == 20 !!!

	   If you see an Syntax error for these lines, it is because
	   SHA_DIGESTSIZE is not 20.
#endif


/*
 * internal LavaRnd algorithm  macros
 *
 * LAVA_DIVUP(x, y)
 *
 *	Compute x/y rounded up to the next integer.  If 'x' is already a
 *	multiple of 'y' then x/y is returned.
 *
 * LAVA_DIVDOWN(x, y)
 *
 *	Compute x/y rounded down to the lower integer.  If 'x' is already a
 *	multiple of 'y' then x/y is returned.
 *
 * LAVA_ROUNDUP(x, y)
 *
 *	Round 'x' up to a multiple of 'y'.  If 'x' is already multiple of
 *	'y' then 'x' is returned.
 *
 * LAVA_ROUNDDOWN(x, y)
 *
 *	Round 'x' down to a multiple of 'y'.  If 'x' is already multiple of
 *	'y' then 'x' is returned.
 *
 ******
 *
 * LAVA_TURN_SUBDIFF(len, nway)
 *
 *	Compute the difference between starting positions of successive
 *	'nway' sub-buffers produced by an 'nway' turn of an input buffer
 *	of 'len' octets.
 *
 *	This is the entire length of each of the sub-buffers produced by
 *	lava_turn().  This length may include some of the NUL
 *	octet padding due to rounding.
 *
 * LAVA_BLK_TURN_SUBDIFF(len, nway)
 *
 *	Compute the difference between starting positions of successive
 *	'nway' sub-buffers with SHA-1 digest blocking produced by an
 *	'nway' turn of an input buffer of 'len' octets.
 *
 *	Unlike LAVA_TURN_SUBLEN(x,y), this macro assumes that each of the
 *	'nway' sub-buffers will be a multiple of SHA-1 digest in length.
 *
 *	This is the entire length of each of the sub-buffers produced by
 *	lava_blk_turn().  This length may include some of the NUL
 *	octet padding due to rounding.
 *
 * LAVA_SALT_BLK_TURN_SUBDIFF(salt_len, len, nway)
 *
 *	Compute the difference between starting positions of successive
 *	'nway' sub-buffers with SHA-1 digest blocking produced by an
 *	'nway' turn of an input buffer of 'salt_len' salt prefix octets,
 *	NUL bytes needed to round out the 'salt_len' to an nway multiple,
 *	and the 'len' octets.
 *
 *	Unlike LAVA_TURN_SUBLEN(x,y), this macro assumes that each of the
 *	'nway' sub-buffers will be a multiple of SHA-1 digest in length.
 *
 *	This is the entire length of each of the sub-buffers produced by
 *	lava_blk_turn().  This length may include some of the NUL
 *	octet padding due to rounding.
 *
 ******
 *
 * LAVA_TURN_OFFSET(len, nway, n)
 *
 *	Compute the starting offset of the 'n'-th sub-buffer within
 *	the output buffer produced by an 'nway' turn of an input
 *	buffer of 'len' octets.
 *
 * LAVA_BLK_TURN_OFFSET(len, nway, n)
 *
 *	Compute the starting offset of the 'n'-th sub-buffer within
 *	the output buffer produced by an 'nway' turn of an input
 *	buffer of 'len' octets.
 *
 * LAVA_SALT_BLK_TURN_OFFSET(salt_len, len, nway, n)
 *
 *	Compute the starting offset of the 'n'-th sub-buffer within
 *	the output buffer produced by an 'nway' turn of an input
 *	'nway' turn of an input buffer of 'salt_len' salt prefix octets,
 *	NUL bytes needed to round out the 'salt_len' to an nway multiple,
 *	and the 'len' octets.
 *
 ******
 *
 * LAVA_TURN_SUBLEN(len, nway, n)
 *
 *	Compute the actual length of the 'n'-th sub-buffer of an
 *	'nway' turn of an input buffer of 'len' octets.
 *
 *	This is the amount of turned data placed in the 'n'-th sub-buffer
 *	by lava_turn().
 *
 *	This length does not include any of the NUL octet padding due
 *	to rounding.
 *
 * LAVA_BLK_TURN_SUBLEN(len, nway, n)
 *
 *	Compute the actual length of the 'n'-th sub-buffer of an
 *	'nway' turn of an input buffer of 'len' octets.
 *
 *	This is the amount of turned data placed in the 'n'-th sub-buffer
 *	by lava_blk_turn().
 *
 *	This length does not include any of the NUL octet padding due
 *	to rounding.
 *
 * LAVA_SALT_BLK_TURN_SUBLEN(salt_len, len, nway, n)
 *
 *	Compute the actual length of the 'n'-th sub-buffer of an
 *	'nway' turn of an input buffer of 'salt_len' salt prefix octets,
 *	NUL bytes needed to round out the 'salt_len' to an nway multiple,
 *	and the 'len' octets.
 *
 *	This is the amount of turned data placed in the 'n'-th sub-buffer
 *	by lava_salt_blk_turn().
 *
 *	This length does not include any of the NUL octet padding due
 *	to rounding.
 *
 ******
 *
 * LAVA_TURN_LEN(len, nway)
 *
 *	Compute the length of the length of an output buffer needed for
 *	'nway' turn of input buffer that is 'len' octets in length.
 *
 *	This is the output buffer length needed for lava_turn().
 *
 * LAVA_BLK_TURN_LEN(len, nway)
 *
 *	Compute the length of the length of an needed for 'nway' turn of
 *	input buffer that is 'len' octets in length where each of
 *	the 'nway' sub-buffers are a multiple of a SHA-1 digest
 *	in length.
 *
 *	This is the output buffer length needed for lava_blk_turn().
 *
 * LAVA_SALT_BLK_TURN_LEN(len, nway)
 *
 *	Compute the length of the length of an needed for 'nway' turn of
 *	an input buffer of 'salt_len' salt prefix octets, NUL bytes needed
 *	to round out the 'salt_len' to an nway multiple, input buffer that
 *	is 'len' octets in length where each of the 'nway' sub-buffers are
 *	a multiple of a SHA-1 digest in length.
 *
 *	This is the output buffer length needed for lava_salt_blk_turn().
 *
 ******
 *
 * LAVA_INDXSTEP(len, nway)
 * LAVA_BLK_INDXSTEP(len, nway)
 * LAVA_SALT_BLK_INDXSTEP(salt_len, len, nway)
 *
 *	Compute the sub-buffer index where the the true length drops
 *	by 1 due to rounding ... or return 0 if there is no length drop.
 *
 *	For example, LAVA_INDXSTEP(26,4) will return 2 because sub-buffers
 *	0 and 1 have length 7 whereas 2 and 3 have length 6.  On the other
 *	hand LAVA_INDXSTEP(28,4) will return 0 because all of the sub-buffers
 *	(0,1,2,3) will have length 7.
 *
 *	Think of LAVA_INDXSTEP(x,y) as the lowest index of the sub-buffers
 *	with a minimum length.  If all sub-buffers are the same length
 *	then 0 is that 1st index minimum length.
 *
 *	NOTE: This LAVA_INDXSTEP(len, nway) formula for BLK processing
 *	      (rounding output sub-buffers to a SHA_DIGESTSIZE) and salting
 *	      (adding a salt, padded with NULs to an nway multiple).
 *
 *	      The BLK buffering is a NUL padding after the turn and
 *	      thus does not change which sub-buffers has the step.
 *	      The salting is padded with NULs to an nway multiple so
 *	      it too will not change which sub-buffers has the step.
 */
#define LAVA_DIVUP(x, y) ( (int)((x)-1+(y)) / (int)(y) )
#define LAVA_DIVDOWN(x, y) ( (int)(x) / (int)(y) )
#define LAVA_ROUNDUP(x, y) ((y) * LAVA_DIVUP(x,y))
#define LAVA_ROUNDDOWN(x, y) ((y) * LAVA_DIVDOWN(x,y))
/**/
#define LAVA_TURN_SUBDIFF(len, nway) \
	LAVA_DIVUP(len, nway)
#define LAVA_BLK_TURN_SUBDIFF(len, nway) \
	LAVA_ROUNDUP(LAVA_TURN_SUBDIFF(len, nway), SHA_DIGESTSIZE)
#define LAVA_SALT_BLK_TURN_SUBDIFF(salt_len, len, nway) \
	LAVA_BLK_TURN_SUBDIFF(LAVA_ROUNDUP((salt_len),(nway))+(len), (nway))
/**/
#define LAVA_TURN_OFFSET(len, nway, n) ((n) * LAVA_TURN_SUBDIFF((len), (nway)))
#define LAVA_BLK_TURN_OFFSET(len, nway, n) \
	((n) * LAVA_BLK_TURN_SUBDIFF((len), (nway)))
#define LAVA_SALT_BLK_TURN_OFFSET(salt_len, len, nway, n) \
	((n) * LAVA_SALT_BLK_TURN_SUBDIFF((salt_len), (len), (nway)))
/**/
#define LAVA_TURN_SUBLEN(len, nway, n) \
	(LAVA_DIVDOWN((len), (nway)) + ((n) < LAVA_INDXSTEP((len), (nway))))
#define LAVA_BLK_TURN_SUBLEN(len, nway, n) \
	LAVA_TURN_SUBLEN((len), (nway), (n))
#define LAVA_SALT_BLK_TURN_SUBLEN(salt_len, len, nway, n) \
	LAVA_TURN_SUBLEN(LAVA_ROUNDUP((salt_len),(nway))+(len), (nway), (n))
/**/
#define LAVA_TURN_LEN(len, nway) \
	((nway) * LAVA_TURN_SUBDIFF((len), (nway)))
#define LAVA_BLK_TURN_LEN(len, nway) \
	((nway) * LAVA_BLK_TURN_SUBDIFF((len), (nway)))
#define LAVA_SALT_BLK_TURN_LEN(salt_len, len, nway) \
	((nway) * LAVA_SALT_BLK_TURN_SUBDIFF((salt_len), (len), (nway)))
/**/
#define LAVA_INDXSTEP(len, nway) ((int)(len) % (int)(nway))
#define LAVA_BLK_INDXSTEP(len, nway) LAVA_INDXSTEP(len, nway)
#define LAVA_SALT_BLK_INDXSTEP(salt_len, len, nway) LAVA_INDXSTEP(len, nway)


/*
 * static declarations
 */
static void lava_xor_fold_rot(u_int32_t *input, int words, u_int32_t *output);


/*
 * static internal state
 *
 * Buffers and sockets needed to perform the simple LavaRnd operation.
 * Part of the simple / naive nature of this code is that there is
 * a single instance of this data.  But that is all that is really
 * needed here anyway ...
 */
static u_int32_t *turned = NULL;	/* n-way turn buffer */
static int turned_maxlen = 0;		/* malloced length of turned */
static int turned_len = 0;		/* length of turned used/needed */
static struct system_stuff salt;	/* salt of the system stuff */
static int stuff_set = FALSE;		/* TRUE ==> fixed stuff already set */


/*
 * lava_turn - turn a buffer into the shortest space
 *
 * given:
 *	input		input buffer to turn
 *	len		length of 'input' and 'output'
 *	nway		number of buffers to turn into
 *	output		turned buffer of LAVA_TURN_LEN(len,nway) octets
 *			NULL ==> malloc it
 *
 * returns:
 *	pointer to the turned buffer or NULL ==> error
 *
 * We turn an 'input' buffer into an 'output' buffer.  Starting positions
 * within the 'output' buffer differ by LAVA_TURN_SUBDIFF(len, nway)
 * octets.  Due rounding, an 'nway' sub-buffer may be followed by a NUL octet.
 *
 * If 'output' is NULL, it is malloced.  The return value, if non-NULL
 * may be used to obtain the malloced buffer.  If 'output' is non-NULL, then
 * 'output' must point to a buffer of at least LAVA_TURN_LEN(len,nway)
 * octets.
 *
 * The 'input' buffer need not be a string.  It may contain NUL octets.
 * The 'output' buffer is likely to contain NUL octets if the 'input' buffer
 * is not a multiple of 'nway' in length (or obviously if 'input' contains
 * NUL octets as well!).
 *
 * The call:
 *
 *	lava_turn("abcdefghijklmnopqrstuvwxyz", 26, 4, turned)
 *
 * will put into the turned buffer:
 *
 *	turned = "aeimquy"
 *		 "bfjnrvz"
 *		 "cgkosw\0"
 *		 "dhlptx\0";
 *
 * I.e.:
 *
 *	turned = "aeimquybfjnrvzcgkosw\0dhlptx\0";
 *		  ^      ^      ^       ^
 *		  +------+------+-------+--- row starts
 *
 *
 * NOTE: If 'nway' is 1, the effect of this routine is a memcpy().
 *
 * NOTE: As implied by the comments above, 'input' need not be a C-style
 *	 string, 'output' may have internal NUL octets and/or 'output'
 *	 may not be NUL terminated.
 *
 * NOTE: In row/col terminology, a sub-buffer is a single row.
 */
void *
lava_turn(void *input_arg, int len, int nway, void *output_arg)
{
    u_int8_t *input = input_arg;	/* input_arg cast as a octet ptr */
    u_int8_t *output = output_arg;	/* output_arg cast as a octet ptr */
    int col;		/* turn column being worked on */
    int row;		/* turn row being worked on */
    int offset;		/* offset between rows in output */
    u_int8_t *beyond_bulk;	/* beyond when to stop bulk turning */
    u_int8_t *beyond;		/* beyond the end of the input buffer */
    u_int8_t *p;

    /*
     * firewall
     */
    if (input == NULL) {
	return NULL;
    }
    if (output == NULL) {
	output = (u_int8_t *)malloc(LAVA_TURN_LEN(len, nway));
	if (output == NULL) {
	    return NULL;
	}
    }

    /*
     * case: 1-way turn
     */
    if (nway <= 1) {
	/* 1-way turn is just a copy */
	memcpy(output, input, len);
	return (void *)output;
    }

    /*
     * determine the first input octet that will not be bulk turned
     * determine the first octet that is beyond the end of input
     */
    beyond_bulk = input + (nway * LAVA_DIVDOWN(len, nway));
    beyond = input + len;

    /*
     * turn in bulk for complete rows
     */
    col = 0;
    offset = LAVA_TURN_SUBDIFF(len, nway);
    while (input < beyond_bulk) {
	for (p=output+col, row=0; row < nway; ++row) {
	    *p = *input++;
	    p += offset;
	}
	++col;
    }

    /*
     * process partial column, if it exists
     */
    if (beyond_bulk < beyond) {

	/*
	 * turn the final partial column
	 */
	p = output+col;
	row = 0;
	while (input < beyond) {
	    *p = *input++;
	    p += offset;
	    ++row;
	}

	/*
	 * NUL fill the rest of the final column
	 */
	while (row++ < nway) {
	    *p = '\0';
	    p += offset;
	}
    }

    /*
     * return the turned input buffer
     */
    return (void *)output;
}


/*
 * lavarnd_turn_len - size of buffer needed by lava_turn()
 *
 * given:
 *	inlen		length of the input buffer
 *	nway		n-way LavaRnd processing value
 *
 * returns:
 * 	length of output buffer needed by lava_turn()
 */
int
lavarnd_turn_len(int inlen, int nway)
{
    return LAVA_TURN_LEN(inlen, nway);
}


/*
 * lava_blk_turn - turn a buffer into a SHA-1 digest aligned buffer set
 *
 * given:
 *	input		input buffer to turn
 *	len		length of 'input' and 'output'
 *	nway		number of buffers to turn into
 *	output		turned buffer of LAVA_BLK_TURN_LEN(len,nway) octets
 *			NULL ==> malloc it
 *
 * returns:
 *	pointer to the turned buffer or NULL ==> error
 *
 * We turn an 'input' buffer into an 'output' buffer.  Starting positions
 * within the 'output' buffer differ by:
 *
 * 	lavarnd_turn_len(len, nway) / nway
 *
 * octets, which returns the LAVA_BLK_TURN_SUBDIFF(len, nway) value.
 *
 * This function differs from lava_turn() in that each of the 'nway'
 * sub-buffers start on an SHA-1 digest aligned (20 octet) position.
 * Any extra octets that follow the end of an 'nway' sub-buffer are NUL
 * filled.  Each of the sub-buffers has a length that is a multiple of
 * the SHA-1 digest size.
 *
 * If 'output' is NULL, it is malloced.  The return value, if non-NULL
 * may be used to obtain the malloced buffer.  If 'output' is non-NULL, then
 * 'output' must point to a buffer of at least:
 *
 * 	lavarnd_turn_len(len, nway)
 *
 * octets, which turns the LAVA_BLK_TURN_LEN(len, nway) value.
 *
 * The 'input' buffer need not be a string.  It may contain NUL octets.
 * The 'output' buffer is likely to contain NUL octets if the 'input' buffer
 * is not a multiple of 'SHA_DIGESTSIZE*nway' in length (or obviously if
 * 'input' contains NUL octets as well!).
 *
 * See the comments in lava_blk() for details of the turn process.  The
 * difference here is that lava_blk_turn() will extend each of the turned
 * sub-buffer to a SHA-1 digest multiple length by NUL filling them.
 *
 * NOTE: If 'nway' is 1, the effect of this routine is a memcpy() followed
 *	 by padding of NUL octets out to a SHA-1 digest length.
 *
 * NOTE: As implied by the comments above, 'input' need not be a C-style
 *	 string, 'output' may have internal NUL octets and/or 'output'
 *	 may not be NUL terminated.
 *
 * NOTE: In row/col terminology, a sub-buffer is a single row.
 */
void *
lava_blk_turn(void *input_arg, int len, int nway, void *output_arg)
{
    u_int8_t *input = input_arg;	/* input_arg cast as a octet ptr */
    u_int8_t *output = output_arg;	/* output_arg cast as a octet ptr */
    int col;		/* turn column being worked on */
    int row;		/* turn row being worked on */
    int offset;		/* offset between rows in output */
    u_int8_t *beyond_bulk;	/* beyond when to stop bulk turning */
    u_int8_t *beyond;	/* beyond the end of the input buffer */
    int extra;		/* extra octets beyond longest sub-buffer to NUL fill */
    int sublen;		/* non-filled 1st sub-buffer length */
    u_int8_t *p;

    /*
     * firewall
     */
    if (input == NULL) {
	return NULL;
    }
    if (output == NULL) {
	output = (u_int8_t *)malloc(LAVA_BLK_TURN_LEN(len, nway));
	if (output == NULL) {
	    return NULL;
	}
    }

    /*
     * case: 1-way turn
     */
    if (nway <= 1) {
	/* 1-way turn is just a copy */
	memcpy(output, input, len);
	/* NUL pad out to a SHA-1 digest multiple length */
	extra = LAVA_ROUNDUP(len, SHA_DIGESTSIZE) - len;
	if (extra > 0) {
	    memset(output+len, 0, extra);
	}
	return output;
    }

    /*
     * determine the first input octet that will not be bulk turned
     * determine the first octet that is beyond the end of input
     */
    beyond_bulk = input + (nway * LAVA_DIVDOWN(len, nway));
    beyond = input + len;

    /*
     * turn in bulk for complete rows
     */
    col = 0;
    offset = LAVA_BLK_TURN_SUBDIFF(len, nway);
    while (input < beyond_bulk) {
	for (p=output+col, row=0; row < nway; ++row) {
	    *p = *input++;
	    p += offset;
	}
	++col;
    }

    /*
     * process partial column, if it exists
     */
    if (beyond_bulk < beyond) {

	/*
	 * turn the final partial column
	 */
	p = output+col;
	row = 0;
	while (input < beyond) {
	    *p = *input++;
	    p += offset;
	    ++row;
	}

	/*
	 * NUL fill those sub-buffers that are short, if any
	 */
	while (row++ < nway) {
	    *p = '\0';
	    p += offset;
	}
    }

    /*
     * now NUL fill out to the end of the SHA-1 digest length multiple
     * if we were not already at a SHA-1 digest length multiple
     *
     * extra is how much data we need to NUL fill on each sub-buffer.
     */
    sublen = LAVA_BLK_TURN_SUBLEN(len, nway, 0);
    extra = offset - sublen;
    if (extra > 0) {

	/* final fill to the SHA-1 digest length multiple */
	for (p=output+sublen, row=0; row < nway; ++row, p += offset) {
	    memset(p, 0, extra);
	}
    }

    /*
     * return the turned input buffer
     */
    return output;
}


/*
 * lavarnd_blk_turn_len - size of buffer needed by lava_blk_turn()
 *
 * given:
 *	inlen		length of the input buffer
 *	nway		n-way LavaRnd processing value
 *
 * returns:
 * 	length of output buffer needed by lava_blk_turn()
 */
int
lavarnd_blk_turn_len(int inlen, int nway)
{
    return LAVA_BLK_TURN_LEN(inlen, nway);
}


/*
 * lava_salt_blk_turn - turn salt & buffer into a SHA-1 digest aligned buf set
 *
 * given:
 *	salt		prefix salt to process before processing input buffer
 *	salt_len	length of the salt in octets
 *	input		input buffer to turn
 *	len		length of 'input' and 'output'
 *	nway		number of buffers to turn into
 *	output		turned buffer of LAVA_BLK_TURN_LEN(len,nway) octets
 *			NULL ==> malloc it
 *
 * returns:
 *	pointer to the turned buffer or NULL ==> error
 *
 * This function is like the lava_blk_turn() function with the addition of
 * the salt prefix.  The output is as if one concatenated the salt prefix,
 * some NUL octets needed to round out the salt prefix length to a multiple
 * of nway octets in length, * and the input buffer and then call
 * lava_blk_turn().
 *
 * We turn an 'salt'+NULs+'input' buffer into an 'output' buffer.  Starting
 * positions within the 'output' buffer differ by:
 *
 * 	lavarnd_salt_blk_len(salt_len, len, nway) / nway
 *
 * octets, which returns LAVA_SALT_BLK_TURN_SUBDIFF(salt_len, len, nway).
 *
 * This function differs from lava_turn() in that each of the 'nway'
 * sub-buffers start on an SHA-1 digest aligned (20 octet) position.
 * Any extra octets that follow the end of an 'nway' sub-buffer are NUL
 * filled.  Each of the sub-buffers has a length that is a multiple of
 * the SHA-1 digest size.
 *
 * If 'output' is NULL, it is malloced.  The return value, if non-NULL
 * may be used to obtain the malloced buffer.  If 'output' is non-NULL, then
 * 'output' must point to a buffer of at least:
 *
 * 	lavarnd_salt_blk_len(salt_len, len, nway)
 *
 * octets, which turns the LAVA_SALT_BLK_TURN_LEN(salt_len, len, nway) value.
 *
 * The 'input' buffer need not be a string.  It may contain NUL octets.
 * The 'output' buffer is likely to contain NUL octets if the 'input' buffer
 * is not a multiple of 'SHA_DIGESTSIZE*nway' in length (or obviously if
 * 'input' contains NUL octets as well!).
 *
 * See the comments in lava_blk() for details of the turn process.  The
 * difference here is that lava_blk_turn() will extend each of the turned
 * sub-buffer to a SHA-1 digest multiple length by NUL filling them.
 *
 * NOTE: If 'nway' is 1, the effect of this routine is a memcpy() followed
 *	 by padding of NUL octets out to a SHA-1 digest length.
 *
 * NOTE: As implied by the comments above, 'input' need not be a C-style
 *	 string, 'output' may have internal NUL octets and/or 'output'
 *	 may not be NUL terminated.
 *
 * NOTE: In row/col terminology, a sub-buffer is a single row.
 */
void *
lava_salt_blk_turn(void *salt_arg, int salt_len,
		   void *input_arg, int len, int nway, void *output_arg)
{
    u_int8_t *salt = salt_arg;		/* salt_arg cast as a octet ptr */
    u_int8_t *input = input_arg;	/* input_arg cast as a octet ptr */
    u_int8_t *output = output_arg;	/* output_arg cast as a octet ptr */
    int col;		/* turn column being worked on */
    int row;		/* turn row being worked on */
    int offset;		/* offset between rows in output */
    u_int8_t *beyond_bulk;	/* beyond when to stop bulk turning */
    u_int8_t *beyond;	/* beyond the end of the input buffer */
    int extra;		/* extra octets beyond longest sub-buffer to NUL fill */
    int salt_sub_off;	/* offset within sub-buffer to load turned input data */
    int sublen;		/* non-filled 1st sub-buffer length */
    u_int8_t *p;

    /*
     * firewall
     */
    if (input == NULL || len < 0 || salt_len < 0 || (salt_len+len) <= 0) {
	return NULL;
    }
    if (output == NULL) {
	output =
	  (u_int8_t *) malloc(LAVA_SALT_BLK_TURN_LEN(salt_len, len, nway));
	if (output == NULL) {
	    return NULL;
	}
    }

    /*
     * case: 1-way turn
     */
    if (nway <= 1) {
	/* 1-way turn is just a copy */
	memcpy(output, salt, salt_len);
	memcpy(output+salt_len, input, len);
	/* NUL pad out to a SHA-1 digest multiple length */
	extra = LAVA_ROUNDUP(salt_len+len, SHA_DIGESTSIZE) - salt_len - len;
	if (extra > 0) {
	    memset(output+len, 0, extra);
	}
	return output;
    }

    /*
     * We first will turn our salt
     */

    /*
     * determine the first salt octet that will not be bulk turned
     * determine the first octet that is beyond the end of the salt
     */
    beyond_bulk = salt + (nway * LAVA_DIVDOWN(salt_len, nway));
    beyond = salt + salt_len;

    /*
     * turn in bulk for complete rows
     */
    col = 0;
    offset = LAVA_SALT_BLK_TURN_SUBDIFF(salt_len, len, nway);
    while (salt < beyond_bulk) {
	for (p=output+col, row=0; row < nway; ++row) {
	    *p = *salt++;
	    p += offset;
	}
	++col;
    }

    /*
     * process partial column, if it exists
     */
    if (beyond_bulk < beyond) {

	/*
	 * turn the final partial column
	 */
	p = output + col;
	row = 0;
	while (salt < beyond) {
	    *p = *salt++;
	    p += offset;
	    ++row;
	}

	/*
	 * NUL fill those sub-buffers that are short, if any
	 */
	while (row++ < nway) {
	    *p = '\0';
	    p += offset;
	}
    }

    /*
     * We will now turn our input buffer
     */

    /*
     * Because we effectually padded the salt with NULs so that
     * we processed an multiple of nway octets, each output
     * sub-buffer of the turned input will start at the
     * same offset within the given sub-buffer.
     */
    salt_sub_off = LAVA_DIVDOWN(LAVA_ROUNDUP(salt_len, nway), nway);

    /*
     * determine the first input octet that will not be bulk turned
     * determine the first octet that is beyond the end of the input
     */
    beyond_bulk = input + (nway * LAVA_DIVDOWN(len, nway));
    beyond = input + len;

    /*
     * turn in bulk for complete rows
     */
    col = 0;
    while (input < beyond_bulk) {
	for (p=output+col+salt_sub_off, row=0; row < nway; ++row) {
	    *p = *input++;
	    p += offset;
	}
	++col;
    }

    /*
     * process partial column, if it exists
     */
    if (beyond_bulk < beyond) {

	/*
	 * turn the final partial column
	 */
	p = output + col + salt_sub_off;
	row = 0;
	while (input < beyond) {
	    *p = *input++;
	    p += offset;
	    ++row;
	}

	/*
	 * NUL fill those sub-buffers that are short, if any
	 */
	while (row++ < nway) {
	    *p = '\0';
	    p += offset;
	}
    }

    /*
     * now NUL fill out to the end of the SHA-1 digest length multiple
     * if we were not already at a SHA-1 digest length multiple
     *
     * extra is how much data we need to NUL fill on each sub-buffer.
     */
    sublen = LAVA_SALT_BLK_TURN_SUBLEN(salt_len, len, nway, 0);
    extra = offset - sublen;
    if (extra > 0) {

	/* final fill to the SHA-1 digest length multiple */
	for (p=output+sublen, row=0; row < nway; ++row, p += offset) {
	    memset(p, 0, extra);
	}
    }

    /*
     * return the turned input buffer
     */
    return output;
}


/*
 * lavarnd_salt_blk_turn_len - size of buf needed by lava_salt_blk_turn()
 *
 * given:
 *	salt_len	length of the salt in octets
 *	inlen		length of the input buffer
 *	nway		n-way LavaRnd processing value
 *
 * returns:
 * 	length of output buffer needed by lava_salt_blk_turn()
 */
int
lavarnd_salt_blk_turn_len(int salt_len, int inlen, int nway)
{
    return LAVA_SALT_BLK_TURN_LEN(salt_len, inlen, nway);
}


/*
 * lava_nway_value - determine a good nway value given length and rate factor
 *
 * given:
 *	length		length of data to process in octets
 *	rate		output rate factor (normal: 1.0,<1.0 slower,>1.0 faster)
 *
 * returns
 *	recommended nway value
 *
 * This function returns a good nway value given input length and rate factor.
 * The normal rate of 1.0 produces a typical nway value.  Rates that are > 0
 * and < 1 result in smaller nway values and return less data.  Rates that are
 * > 1 result in larger nway values and return more data.
 *
 * The value returned is a recommended value but is by no means a
 * restriction.  One may use values as high as double this return value
 * and obtain perfectly reasonable results.  One may use smaller values
 * (as small as 1) as well.  This function returns odd numbers, the truly
 * paranoid might want to use odd numbers as well.
 *
 * Rate values that are >= 0.0625 and <= 16.0 produce perfectly reasonable
 * nway values.  If in doubt, use a rate of 1.0.
 *
 * The nway value controls the degree of buffer turning as well
 * as the amount of data that the lavarnd*() and lavaurl*()
 * functions return.  The idea behind the selection of the nway
 * value is to spread our SHA-1 hash strings throughout the input
 * data and to produce a reasonable amount of SHA-1 cryptographic
 * data for our efforts.
 *
 * We do not want our nway value to be too large with respect
 * to the size of the input.  If we have too many nway buffers then
 * each nway buffer will take only a few octets from our input.
 *
 * We do not want our nway value to be too small.  A small nway
 * value will not produce very much SHA-1 cryptographic data.
 *
 * As a firewall, a length <= 0 is treated as a length of 1.
 * As a firewall, a rate <= 0.0 is treated as a rate value of 1.0.
 *
 * The basic nway value is:
 *
 *	nway = sqrt(length*rate / SHA_BLOCKSIZE)
 *
 * However we do not want an nway value that is a multiple of 2 or 3.
 * We increase nway until it is either 1 or 5 mod 6.
 *
 * Consider a 160x120 pixel raw image that is 19200 octets in size.
 * The following shows the effect of rate on this 19200 octet image:
 *
 *	image			output
 *	octets	rate    nway	octets
 *	------	----	----	------
 *	19200	16.0	71	1420
 *	19200	12.0	61	1220
 *	19200	8.0	49	 980
 *	19200	6.0	43	 860
 *	19200	4.0	35	 700
 *	19200	3.0	31	 620
 *	19200	2.0	25	 500
 *	19200	1.5	23	 460
 *	19200	1.0	17	 340
 *	19200	0.5	13	 260
 *	19200	0.25	11	 220
 *	19200	0.125	 7	 140
 *	19200	0.0625	 5	 100
 *
 * NOTE: The size of the salt does not effect the nway value.   The nway
 * 	 value is only dependent on the input length.  Therefore the length
 * 	 should not include any salt size.
 */
int
lava_nway_value(int length, double rate)
{
    int nway;		/* nway value to return */
    static int mod6[6] = { 1, 0, 3, 2, 1, 0 }; /* add to be 1 or 5 mod 6 */

    /* compute a nway value without regard to mod 6 value */
    nway = (int)sqrt(((double)(length<=0) ? 1 : length) *
    		     ((rate<=0) ? 1.0 : rate) / SHA_BLOCKSIZE);

    /* force nway to be 1 or 5 mod 6 */
    nway += mod6[nway % 6];

    /* return optimal nway value */
    return nway;
}


/*
 * lava_xor_fold_rot - xor, fold and rotate in SHA-1 sized chunks
 *
 * given:
 *	input		data used to produce the xor rotation
 *	wordlen		length of input in 32bits (must be >= 5)
 *	output		xor rotation result (SHA_DIGESTSIZE sized buffer)
 *
 * NOTE: wordlen must be at least SHA_DIGESTLONG and should be a multiple
 *	 of SHA_DIGESTLONG.
 *
 * NOTE: For performance reasons we assume that SHA_DIGESTLONG is 5.
 *
 * NOTE: This function silently does nothing if input or output NULL or
 *	 if wordlen is < SHA_DIGESTLONG.
 */
static void
lava_xor_fold_rot(u_int32_t *input, int words, u_int32_t *output)
{
    int inwords;	/* number u_int32_t words in input */
    int32_t xor[SHA_DIGESTLONG];	/* temp xor buffer */
    int i;

    /*
     * firewall
     */
    if (input == NULL || words < SHA_DIGESTLONG || output == NULL) {
	return;
    }

    /*
     * perform xor rotations
     */
    memset(output, 0, SHA_DIGESTSIZE);
    inwords = LAVA_ROUNDDOWN(words, SHA_DIGESTLONG);
    i = 0;
    while (i < inwords) {

	/* xor the next SHA-1 digest chunk */
	xor[0] = input[i++] ^ output[0];
	xor[1] = input[i++] ^ output[1];
	xor[2] = input[i++] ^ output[2];
	xor[3] = input[i++] ^ output[3];
	xor[4] = input[i++] ^ output[4];

	/* circular shift left by 1 bit, the xor */
	output[0] = ((xor[0]<<1) | (xor[4] < 0));
	output[1] = ((xor[1]<<1) | (xor[0] < 0));
	output[2] = ((xor[2]<<1) | (xor[1] < 0));
	output[3] = ((xor[3]<<1) | (xor[2] < 0));
	output[4] = ((xor[4]<<1) | (xor[3] < 0));
    }
    return;
}


/*
 * lavarnd_len - determine the size of output buffer needed by lavarnd
 *
 * given:
 *	inlen		length of the input buffer (not counting any salt)
 *	rate		increase (>1.0) or decrease (<1.0) output amount
 *
 * returns:
 * 	length of output buffer needed by lavarnd()
 *
 * NOTE: The size of the salt does not effect the nway value.   The nway
 * 	 value is only dependent on the input length.  Therefore the inlen
 * 	 should not include any salt size.
 */
/*ARGSUSED*/
int
lavarnd_len(int inlen, double rate)
{
    return lava_nway_value(inlen,rate) * SHA_DIGESTSIZE;
}


/*
 * lavarnd - Perform the lavarnd process
 *
 * Produce cryptographically strong output given chaotic input.  The
 * amount of output depends on the input length, the rate and the
 * output buffer length (outlen).  The output will not exceed the
 * length of the output buffer.
 *
 * given:
 *	use_salt	1 ==> use system_stuff for salt, 0 ==> no salting
 *	input		input buffer
 *	inlen		length of the input buffer
 *			    must be at least 1 to return anything
 *	rate		increase (>1.0) or decrease (<1.0) output amount
 *			    must be > 0.0 to return anything
 *	output		where to put the lavarnd result
 *	outlen		maximum length of the output buffer available
 *			    must be at least SHA_DIGESTSIZE to return anything
 *
 * returns:
 *	LavaRnd octets written to output if >0, <0 ==> error
 *
 * NOTE: We will also treat any negative outlen value as an indication that
 * 	 nothing should be some because the output buffer is full.
 *
 * NOTE: This function will produce the following number of octets:
 *
 *		min(outlen, lava_nway_value(inlen, rate))
 *
 *	 The amount of data produced will never exceed outlen.  If outlen
 *	 is not large enough, the nway value will be reduced until it fits
 *	 into output.  However, if the output buffer is too large, then
 *	 the lava_nway_value(inlen, rate) limit still applies and the
 *	 extra output space will be unused.
 *
 * NOTE: The return value, if >0, is always a multiple of SHA_DIGESTSIZE.
 *	 It is reasonable (but not required) make the output buffer (and
 *	 the maxoutput value) a multiple of SHA_DIGESTSIZE in length as well.
 *
 * NOTE: Typically a calling function might want to:
 *
 *	#include "LavaRnd/rawlava.h"
 *	#include "LavaRnd/lavarnd.h"
 *
 *	u_int8_t *chaotic_data;		(* given: chaotic data to process *)
 *	int chaotic_len;		(* given: length of chaotic_data *)
 *
 *	char *url;			(* URL of chaotic source *)
 *	double rate;			(* 0.0625 to 16.0, so perhaps 1.0 *)
 *	u_int8_t *output = NULL;	(* LavaRnd output *)
 *	int ret;			(* lavarnd() output len or <0==>err *)
 *
 *	if (chaotic_len > 0) {
 *	    u_int32_t nway;		(* computed optimal nway value *)
 *	    u_int32_t outlen;		(* where output length is placed *)
 *
 *	    (* pick optimal nway *)
 *	    nway = lava_nway_value(chaotic_len, rate);
 *
 *	    (* setup the LavaRnd output buffer *)
 *	    outlen = lavarnd_len(ret, nway);
 *	    output = malloc(outlen);
 *	    if (output == NULL) {
 *		 return LAVAERR_MALLOC;
 *	    }
 *
 *	    (* do the LavaRnd thing :-) *)
 *	    ret = lavarnd(1, chaotic_data, chaotic_len, rate, output, outlen);
 *	    if (ret < 0) {
 *	        if (output != NULL) {
 *	             free(output);
 *	        }
 *	        return ret;
 *	    }
 *
 *	    (* The LavaRnd starts at output and goes for 'ret' octets *)
 *	}
 *
 *	...
 *
 *	(* later, to free internal storage *)
 *	lavarnd_cleanup();
 *
 * NOTE: The creation of chaotic_data, chaotic_len might be as follows:
 *
 * 	#include "LavaRnd/rawlava.h"
 * 	#include "LavaRnd/simple_url.h"
 * 	#include "LavaRnd/lavarnd.h"
 *
 *	struct lava_retry *lava_retry;	(* fill in or NULL for no retrying *)
 *	u_int8_t *chaotic_data;		(* given: chaotic data to process *)
 *	int chaotic_len;		(* given: length of chaotic_data *)
 *	int minlen;			(* min URL content desired *)
 *
 * 	chaotic_len = lava_get_url(url, minlen, lava_retry);
 *
 * 	... code from previous NOTE ...
 *
 * 	lava_url_cleanup();
 */
int
lavarnd(int use_salt, void *input_arg, int inlen, double rate,
	void *output_arg, int outlen)
{
    u_int32_t *input = input_arg;	/* input_arg cast as a 32bit ptr */
    u_int32_t *output = output_arg;	/* output_arg cast as a 32bit ptr */
    int nway;				/* nway turn level */
    int subwords;	/* sub-buffer 32bit word length including padding */
    int sublen0;	/* longest sub-buffer length */
    int indxjump;	/* 32bit turn index of shortest sub-buffer */
    int sublen1;	/* shortest sub-buffer length, may == sublen0 */
    u_int32_t fold[SHA_DIGESTLONG];	/* previous sub-buffer xor rot folded */
    u_int32_t hash[SHA_DIGESTLONG];	/* current SHA-1 sub-buffer hash */
    int turnedwords;			/* words used in turned buffer */
    u_int32_t *p;	/* temp pointer for realloc return */
    int salt_len;	/* 0 if not salting, system stuff size if salting */
    int i;		/* turned 32bit word index */
    int j;		/* output 32bit word index */

    /*
     * firewall
     */
    if (input == NULL || inlen < 0 || rate < 0.0 || output == NULL) {
	return LAVAERR_BADARG;
    }
    if (inlen <= 0 || rate <= 0.0 || outlen < SHA_DIGESTSIZE) {
	/* nothing to do */
	return 0;
    }

    /*
     * generate the salt if first use_salt request
     */
    if (use_salt) {
	system_stuff(&salt, stuff_set);
	stuff_set = TRUE;
    }
    salt_len = (use_salt ? sizeof(salt) : 0);

    /*
     * determine the nway value
     *
     * We deal with the case where outlen is too small by reducing
     * the nway value until it fits and nway == 1 or 5 mod 6.
     *
     * NOTE: We do not allow the size of the salt to effect the nway value.
     * 	     The nway value is only dependent on the input length.
     */
    nway = lava_nway_value(inlen, rate);
    if (nway*SHA_DIGESTSIZE > outlen) {
	static int mod6[6] = { 1, 0, 1, 2, 3, 0 }; /* sub be 1 or 5 mod 6 */

	/* nway cannot exceed this value or it will overflow output */
	nway = LAVA_DIVDOWN(outlen, SHA_DIGESTSIZE);
	/* force nway to be 1 or 5 mod 6 by reducing it */
	nway -= mod6[nway % 6];
    }
    /* firewall */
    if (nway < 1) {
	return LAVAERR_IMPOSSIBLE;
    }

    /*
     * prep for the nway turn of the input data
     *
     * We will either successfully malloc the turned buffer (with a
     * new turned_maxlen size) or we will return LAVAERR_MALLOC (with
     * turned and turned_maxlen unchanged).
     */
    turned_len = LAVA_SALT_BLK_TURN_LEN(salt_len, inlen, nway);
    if (turned == NULL) {
	turned = (u_int32_t *)malloc(turned_len);
	if (turned == NULL) {
	    /* out of memory */
	    return LAVAERR_MALLOC;
	}
	turned_maxlen = nway*SHA_DIGESTSIZE;
    } else if (turned_maxlen < turned_len) {
	p = (u_int32_t *)realloc(turned, turned_len);
	if (p == NULL) {
	    /* out of memory */
	    return LAVAERR_MALLOC;
	}
	turned = p;
	turned_maxlen = turned_len;
    }

    /*
     * perform a SHA-1 digest size blocked nway turn of the input data
     *
     * The first nway sets of SHA_DIGESTSIZE chunks will have nway
     * sub-buffers, aligned on SHA-1 digest sized chunks and NUL
     * padded as needed.
     */
    /* firewall */
    if (nway*SHA_DIGESTSIZE > outlen) {
	return LAVAERR_IMPOSSIBLE;
    }
    if (use_salt) {
	p = lava_salt_blk_turn((void *)&salt, salt_len,
			       input_arg, inlen, nway, (void *)turned);
    } else {
	p = lava_blk_turn(input_arg, inlen, nway, (void *)turned);
    }
    /* firewall */
    if (p == NULL) {
	return LAVAERR_IMPOSSIBLE;
    }

    /*
     * SHA-1 hash each nway sub-buffer and xor it with the xor fold rotate of
     * of the previous sub-buffer.
     *
     * This is the code of the LavaRnd algorithm.
     */

    /*
     * LavaRnd algorithm setup
     */
    turnedwords = turned_len / sizeof(u_int32_t);
    subwords = LAVA_SALT_BLK_TURN_SUBDIFF(salt_len, inlen, nway) /
		  sizeof(u_int32_t);
    sublen0 = LAVA_SALT_BLK_TURN_SUBLEN(salt_len, inlen, nway, 0);
    indxjump = LAVA_SALT_BLK_INDXSTEP(salt_len, inlen, nway) * subwords;
    sublen1 = LAVA_SALT_BLK_TURN_SUBLEN(salt_len, inlen, nway, nway-1);

    /*
     * xor fold rotate the last sub-buffer for our loop start
     */
    lava_xor_fold_rot(turned+turnedwords-subwords, subwords, fold);

    /*
     * LavaRnd process the longer sub-buffers, or none if they are all the same
     */
    for (i=0, j=0; i < indxjump; i += subwords) {

	/* SHA-1 hash the sub-buffer */
	lava_sha1_buf((void *)(turned+i), sublen0, (void *)hash);

	/* store the xor of the previous sub-buffer xor fold and this hash */
	output[j++] = fold[0] ^ hash[0];
	output[j++] = fold[1] ^ hash[1];
	output[j++] = fold[2] ^ hash[2];
	output[j++] = fold[3] ^ hash[3];
	output[j++] = fold[4] ^ hash[4];

	/* xor fold rotate this sub-buffer for the next operation */
	lava_xor_fold_rot(turned+i, subwords, fold);
    }

    /*
     * LavaRnd process the shorter sub-buffers
     * up to but not including the last
     */
    for ( ; i < turnedwords-subwords; i += subwords) {

	/* SHA-1 hash the sub-buffer */
	lava_sha1_buf((void *)(turned+i), sublen1, (void *)hash);

	/* store the xor of the previous sub-buffer xor fold and this hash */
	output[j++] = fold[0] ^ hash[0];
	output[j++] = fold[1] ^ hash[1];
	output[j++] = fold[2] ^ hash[2];
	output[j++] = fold[3] ^ hash[3];
	output[j++] = fold[4] ^ hash[4];

	/* xor fold rotate this sub-buffer for the next operation */
	lava_xor_fold_rot(turned+i, subwords, fold);
    }

    /*
     * process the last sub-buffers - no need to xor fold it again
     */

    /* SHA-1 hash the sub-buffer */
    lava_sha1_buf((void *)(turned+i), sublen1, (void *)hash);

    /* store the xor of the previous sub-buffer xor fold and this hash */
    output[j++] = fold[0] ^ hash[0];
    output[j++] = fold[1] ^ hash[1];
    output[j++] = fold[2] ^ hash[2];
    output[j++] = fold[3] ^ hash[3];
    output[j++] = fold[4] ^ hash[4];

    /*
     * return output length
     */
    return j * sizeof(u_int32_t);
}


/*
 * lavarnd_cleanup - cleanup to make memory leak checkers happy
 *
 * NOTE: This function is not called from lava_dormant() because this file
 * is not part of the ${COMMON_LAVA_OBS} set.
 */
void
lavarnd_cleanup(void)
{
    /*
     * free the malloced buffers used by this module
     */
    if (turned != NULL) {
	free(turned);
	turned = NULL;
    }
    turned_maxlen = 0;
}
