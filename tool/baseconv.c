/*
 * baseconv - Convert between ASCII 0's & 1's, ASCII hex, ASCII base64, binary
 *
 * @(#) $Revision: 10.1 $
 * @(#) $Id: baseconv.c,v 10.1 2003/08/18 06:44:37 lavarnd Exp $
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
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "LavaRnd/lavaerr.h"
#include "LavaRnd/rawio.h"


/*
 * truth - as we know it
 */
#if !defined(TRUE)
# define TRUE 1
#endif
#if !defined(FALSE)
# define FALSE 0
#endif


/*
 * minimum
 */
#define MIN(x,y) (((x) < (y)) ? (x) : (y))


/*
 * usage
 */
char *program;		/* our name */
const char *usage="usage: "
    "%s {b256|b64|b16|b2} {b256|b64|b16|b2}\n"
    "\n"
    "\tb256	input/output in binary format (alias: bin)\n"
    "\tb64	input/output in base64 format\n"
    "\tb16	input/output in hex format (alias: hex)\n"
    "\tb2	input/output in base 2 format\n";
char *program;

enum format {
    FMT_B256=0,	/* output in base 256 format */
    FMT_B64,	/* output in base 64 format */
    FMT_B16,	/* output in base 16 format */
    FMT_B2	/* output in base 2 format */
};


/*
 * I/O
 */
#define IO_SIZE (24*1024)		/* base 256 I/O size */
#define IO_B256_SIZE (IO_SIZE)		/* base 256 I/O size */
#define IO_B64_SIZE (IO_SIZE*8/6)	/* base 64 I/O size */
#define IO_B16_SIZE (IO_SIZE*8/4)	/* base 16 I/O size */
#define IO_B2_SIZE (IO_SIZE*8/1)	/* base 2 I/O size */
#define PAD (9)				/* padding for I/O & conv buffer */
static unsigned char *inbuf = NULL;	/* input buffer */
static int inbuf_max = 0;		/* maximum length of input buffer */
static int inbuf_len = 0;		/* current length of input buffer */
static unsigned char conv[IO_SIZE+PAD];	/* conversion buffer */
static unsigned char *outbuf = NULL;	/* output buffer */
static int outbuf_max = 0;		/* maximum length of output buffer */
static int outbuf_len = 0;		/* current length of output buffer */

#define IN (0)				/* input descriptor - stdin */
#define OUT (1)				/* output descriptor - stdout */


/*
 * conversion tables
 */
static const unsigned char *const b16digits = "0123456789abcdef";
static const int b16val[256] = {
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
};
static const int isb16[256] = {
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
};
static const unsigned char *const base64 =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const int b64val[256] = {
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  0,  0, 63,
52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,
 0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,
 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};
static const int isb64[256] = {
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  1,
 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,
 0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,
 0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};
static const unsigned char *const base2 = "01";
static const int b2val[256] = {
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};
static const int isb2[256] = {
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  1,
 1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  1,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  1,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  1,
 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};


/*
 * static declarations
 */
static enum format parse_fmt(char *str);
static void alloc_buf(enum format fmt, unsigned char **buf_p, int *len_p);
static int bitlen(enum format fmt, int len);
static int rounddown(enum format fmt, int len);
static int roundup(enum format fmt, int len);
static void convert(unsigned char *in, int *p_inlen, enum format infmt,
		 unsigned char *out, int *p_outlen, int outmax,
		 enum format outfmt);
static void gleam(unsigned char *in, int *p_inlen, enum format fmt);


int
main(int argc, char *argv[])
{
    enum format in_fmt;		/* input format */
    enum format out_fmt;	/* output format */
    int inret;			/* read function return value */
    int pad;			/* amount of padding needed to inret */
    int outret;			/* write function return value */
    int eof_read;		/* TRUE ==> we read EOF on input */

    /*
     * parse args
     */
    program = argv[0];
    if (argc != 3) {
	fprintf(stderr, usage, program);
	exit(1);
    }
    in_fmt = parse_fmt(argv[1]);
    out_fmt = parse_fmt(argv[2]);

    /*
     * allocate I/O buffers
     */
    alloc_buf(in_fmt, &inbuf, &inbuf_max);
    inbuf_len = 0;
    alloc_buf(out_fmt, &outbuf, &outbuf_max);
    outbuf_len = 0;

    /*
     * convert input to output
     */
    eof_read = FALSE;
    do {

	/*
	 * read to fill the buffer
	 */
	if (!eof_read && inbuf_len < inbuf_max) {

	    /* try to fill the buffer from input */
	    errno = 0;
	    inret = read_once(IN, inbuf+inbuf_len,
	    		      inbuf_max-inbuf_len, FALSE);

	    /* status checking and accounting */
	    if (inret == 0) {
		eof_read = TRUE;
	    } else if (inret == LAVAERR_NONBLOCK) {
		inret = 0;
	    } else if (inret < 0) {
		fprintf(stderr, "%s: read error: inret: %d\n", program, inret);
		exit(2);
	    } else {
		gleam(inbuf+inbuf_len, &inret, in_fmt);
		inbuf_len += inret;
	    }
	}

	/*
	 * if EOF, then expand the buffer out a full conversion chunk
	 */
	if (eof_read) {

	    /* determine padding length */
	    pad = roundup(in_fmt, inbuf_len) - inbuf_len;

	    /* pad 0 octets */
	    if (pad > 0) {
		memset(inbuf+inbuf_len, 0, pad);
		inbuf_len += pad;
	    }
	}

	/*
	 * convert as much input to output
	 */
	if (inbuf_len > 0 && outbuf_len < outbuf_max) {
	    convert(inbuf, &inbuf_len, in_fmt,
		    outbuf, &outbuf_len, outbuf_max, out_fmt);
	}

	/*
	 * output as much of the buffer as possible
	 */
	if (outbuf_len > 0) {

	    /* try to write as much of the buffer as we can */
	    errno = 0;
	    if (eof_read) {
		outret = raw_write(OUT, outbuf, outbuf_len, FALSE);
	    } else {
		outret = noblock_write(OUT, outbuf, outbuf_len, FALSE);
	    }

	    /* status checking and accounting */
	    if (outret == 0) {
		fprintf(stderr, "%s: early EOF on output\n", program);
		exit(3);
	    } else if (outret == LAVAERR_NONBLOCK) {
		outret = 0;
	    } else if (outret < 0) {
		fprintf(stderr, "%s: write error: outret: %d\n",
			program, outret);
		exit(4);
	    } else {
		if (outret < outbuf_len) {
		    memmove(outbuf, outbuf+outret, outbuf_len-outret);
		}
		outbuf_len -= outret;
	    }
	}
    } while(! (eof_read && inbuf_len == 0 && outbuf_len == 0));

    /*
     * cleanup - so things like valgrind will not complain about memory leaks
     */
    free(inbuf);
    free(outbuf);

    /*
     * all done
     */
    return 0;
}


/*
 * parse_fmt - parse a format name string
 *
 * given:
 *	str	name of a format {bin, hex, b64}
 *
 * returns:
 *	enum format
 *
 * NOTE: This function exits on error.
 */
static enum format
parse_fmt(char *str)
{
    /*
     * firewall
     */
    if (str == NULL) {
	fprintf(stderr, "%s: parse_fmt: arg is NULL\n", program);
	exit(5);
    }

    /*
     * parse format
     */
    if (strcasecmp(str, "b256") == 0 || strcasecmp(str, "bin") == 0) {
	return FMT_B256;
    } else if (strcasecmp(str, "b64") == 0) {
	return FMT_B64;
    } else if (strcasecmp(str, "b16") == 0 || strcasecmp(str, "hex") == 0) {
	return FMT_B16;
    } else if (strcasecmp(str, "b2") == 0) {
	return FMT_B2;
    }

    /*
     * unknown format error
     */
    fprintf(stderr, usage, program);
    exit(6);
}


/*
 * alloc_buf - allocate an I/O buffer
 *
 * given:
 *	fmt	enum format
 *	buf_p	pointer to buffer pointer
 *	len_p	pointer to length
 *
 * NOTE: This function exits on error.
 */
static void
alloc_buf(enum format fmt, unsigned char **buf_p, int *len_p)
{
    int len;		/* allocation size */
    unsigned char *buf;	/* allocated buffer */

    /*
     * firewall
     */
    if (buf_p == NULL || len_p == NULL) {
	fprintf(stderr, "%s: alloc_buf: NULL arg(s)\n", program);
	exit(7);
    }

    /*
     * determine size
     */
    switch (fmt) {
    case FMT_B256:
	len = IO_B256_SIZE;
	break;
    case FMT_B64:
	len = IO_B64_SIZE;
	break;
    case FMT_B16:
	len = IO_B16_SIZE;
	break;
    case FMT_B2:
	len = IO_B2_SIZE;
	break;
    default:
	fprintf(stderr, "%s: alloc_buf: unknown base format: %d\n",
		program, (int)fmt);
	exit(8);
    }

    /*
     * allocate
     */
    buf = (unsigned char *)malloc(len+PAD);
    if (buf == NULL) {
	fprintf(stderr, "%s: alloc_buf: malloc of %d octets failed",
		program, len+1);
	exit(9);
    }

    /*
     * return information
     */
    *buf_p = buf;
    *len_p = len;
    return;
}


/*
 * bitlen - determine the binary bit length of a formatted buffer
 *
 * given:
 *	fmt		format to determine binary length
 *	len		formatted buffer length
 *
 * returns:
 *	bit length if buffer is converted to binary
 *
 * NOTE: This function exits on error.
 */
static int
bitlen(enum format fmt, int len)
{
    switch (fmt) {
    case FMT_B256:
	return len*8;
    case FMT_B64:
	return len*6;
    case FMT_B16:
	return len*4;
    case FMT_B2:
	return len;
    }
    fprintf(stderr, "%s: bitlen: unknown base format: %d\n",
	    program, (int)fmt);
    exit(10);
    /*NOTREACHED*/
    return 0;
}


/*
 * rounddown - round down a length to a completer binary octet conversion chunk
 *
 * given:
 *	fmt		format to determine binary length
 *	len		formatted buffer length
 *
 * returns:
 *	rounded down octet length
 *
 * NOTE: This function exits on error.
 */
static int
rounddown(enum format fmt, int len)
{
    switch (fmt) {
    case FMT_B256:
	return len;
    case FMT_B64:
	return 4*(len/4);
    case FMT_B16:
	return 2*(len/2);
    case FMT_B2:
	return 8*(len/8);
    }
    fprintf(stderr, "%s: bitlen: unknown base format: %d\n",
	    program, (int)fmt);
    exit(10);
    /*NOTREACHED*/
    return 0;
}


/*
 * roundup - round up a length to a completer binary octet conversion chunk
 *
 * given:
 *	fmt		format to determine binary length
 *	len		formatted buffer length
 *
 * returns:
 *	rounded up octet length
 *
 * NOTE: This function exits on error.
 */
static int
roundup(enum format fmt, int len)
{
    switch (fmt) {
    case FMT_B256:
	return len;
    case FMT_B64:
	return 4*((len+3)/4);
    case FMT_B16:
	return 2*((len+1)/2);
    case FMT_B2:
	return 8*((len+7)/8);
    }
    fprintf(stderr, "%s: bitlen: unknown base format: %d\n", program,
	    (int)fmt);
    exit(10);
    /*NOTREACHED*/
    return 0;
}


/*
 * convert - convert data between buffers
 *
 * This function will convert as much data as possible that is found
 * in the input buffer and place it in the output buffer.  Conversion
 * is performed according to infmt and outfmt.  On exit, the *p_inlen
 * and *p_outlen will be updated and and any converted input buffer
 * data will have been moved to the start of the input buffer.
 *
 * given:
 *	in		input buffer
 *	p_inlen		pointer to amount of data in the input buffer
 *	infmt		input format
 *	out		output buffer
 *	p_outlen	pointer to amount of data in the output buffer
 *	outmax		maximum size of the output buffer
 *	outfmt		output format
 *
 * NOTE: This function exits on error.
 */
static void
convert(unsigned char *in, int *p_inlen, enum format infmt,
     unsigned char *out, int *p_outlen, int outmax, enum format outfmt)
{
    int inlen;		/* amount of data in the input buffer */
    int outlen;		/* amount of data already in the output buffer */
    int outfree;	/* amount of data free in output buffer */
    int convmax;	/* max amount of binary data we can convert */
    int i;
    int j;

    /*
     * firewall
     */
    if (in == NULL || p_inlen == NULL || out == NULL || p_outlen == NULL) {
	fprintf(stderr, "%s: conv: NULL arg(s)\n", program);
	exit(11);
    }
    inlen = *p_inlen;
    outlen = *p_outlen;
    if (inlen < 0 || outlen < 0 || outmax <= 0) {
	fprintf(stderr, "%s: conv: bad count(s)\n", program);
	exit(12);
    }
    outfree = outmax - outlen;
    if (inlen == 0 || outfree <= 0) {
	/* nothing can be done, quick return */
	return;
    }

    /*
     * case: input and output formats are the same, do a direct copy and return
     */
    if (infmt == outfmt) {

	/* determine how much we can convert (copy) */
	convmax = MIN(inlen, outfree);

	/* covert (copy) */
	memmove(out+outlen, in, convmax);

	/* accounting */
	if (convmax < inlen) {
	    memmove(in, in+inlen, inlen-convmax);
	}
	inlen -= convmax;
	outlen += convmax;
	*p_inlen = inlen;
	*p_outlen = outlen;
	return;
    }

    /*
     * determine how much data we can convert
     */
    i = bitlen(infmt, rounddown(infmt, inlen))/8;
    convmax = bitlen(outfmt, rounddown(outfmt, outfree))/8;
    convmax = MIN(i, convmax);

    /*
     * convert input format to binary format
     *
     * We unroll the loops below with the knowledge (or hope) that
     * the optimizer will improve them.
     */
    switch (infmt) {
    case FMT_B256:
	/* convert (copy) to binary */
	memmove(conv, in, convmax);

	/* accounting */
	if (convmax < inlen) {
	    memmove(in, in+inlen, inlen-convmax);
	}
	inlen -= convmax;
	break;

    case FMT_B64:
	/* convert */
	/* 4/3 bit pattern: 01234501 23450123 45012345 */
	for (i=0, j=0; j < convmax;) {
	    conv[j] = (b64val[in[i]] << 2);
	    ++i;
	    conv[j] |= (b64val[in[i]] >> 4);
	    ++j;
	    if (j >= convmax) {
		break;
	    }

	    conv[j] = (b64val[in[i]] << 4);
	    ++i;
	    conv[j] |= (b64val[in[i]] >> 2);
	    ++j;
	    if (j >= convmax) {
		break;
	    }

	    conv[j] = (b64val[in[i]] << 6);
	    ++i;
	    conv[j] |= b64val[in[i]];
	    ++i;
	    ++j;
	}

	/* accounting */
	if (i < inlen) {
	    memmove(in, in+inlen, inlen-i);
	}
	inlen -= i;
	break;

    case FMT_B16:
	/* convert */
	/* 2/1 bit pattern: 01230123 */
	for (i=0, j=0; j < convmax;) {
	    conv[j] = (b16val[in[i]] << 4);
	    ++i;
	    conv[j] |= b16val[in[i]];
	    ++i;
	    ++j;
	}

	/* accounting */
	if (i < inlen) {
	    memmove(in, in+inlen, inlen-i);
	}
	inlen -= i;
	break;

    case FMT_B2:
	/* convert */
	/* 8/1 bit pattern: 00000000 */
	for (i=0, j=0; j < convmax;) {
	    conv[j] = (b2val[in[i]] << 7);
	    ++i;
	    conv[j] |= (b2val[in[i]] << 6);
	    ++i;
	    conv[j] |= (b2val[in[i]] << 5);
	    ++i;
	    conv[j] |= (b2val[in[i]] << 4);
	    ++i;
	    conv[j] |= (b2val[in[i]] << 3);
	    ++i;
	    conv[j] |= (b2val[in[i]] << 2);
	    ++i;
	    conv[j] |= (b2val[in[i]] << 1);
	    ++i;
	    conv[j] |= b2val[in[i]];
	    ++i;
	    ++j;
	}

	/* accounting */
	if (i < inlen) {
	    memmove(in, in+inlen, inlen-i);
	}
	inlen -= i;
	break;

    default:
	fprintf(stderr, "%s: bitlen: unknown base format: %d\n",
		program, (int)infmt);
	exit(13);
    }
    *p_inlen = inlen;

    /*
     * convert binary format to output format
     *
     * We unroll the loops below with the knowledge (or hope) that
     * the optimizer will improve them.
     */
    switch (outfmt) {
    case FMT_B256:
	/* convert (copy) to binary */
	memmove(out+outlen, conv, convmax);
	outlen += convmax;
	break;

    case FMT_B64:
	/* convert */
	/* 4/3 bit pattern: 012345 670123 456701 234567 */
	for (i=0, j=0; j < convmax;) {
	    int k;

	    out[i] = base64[conv[j] >> 2];
	    ++i;

	    k = ((conv[j] & 0x3) << 4);
	    ++j;
	    out[i] = base64[k | (conv[j] >> 4)];
	    ++i;
	    if (j >= convmax) {
		break;
	    }

	    k = ((conv[j] & 0xf) << 2);
	    ++j;
	    if (j >= convmax) {
		break;
	    }
	    out[i] = base64[k | (conv[j] >> 6)];
	    ++i;

	    out[i] = base64[conv[j] & 0x3f];
	    ++j;
	    ++i;
	}
	outlen += i;
	break;

    case FMT_B16:
	/* convert */
	/* 2/1 bit pattern: 0123 4567 */
	for (i=0, j=0; j < convmax;) {
	    out[i] = b16digits[conv[j] >> 4];
	    ++i;

	    out[i] = b16digits[conv[j] & 0xf];
	    ++i;
	    ++j;
	}
	outlen += i;
	break;

    case FMT_B2:
	/* convert */
	/* 8/1 bit pattern: 0 1 2 3 4 5 6 7 */
	for (i=0, j=0; j < convmax;) {
	    out[i] = ((conv[j] & 0x80) ? '1' : '0');
	    ++i;

	    out[i] = ((conv[j] & 0x40) ? '1' : '0');
	    ++i;

	    out[i] = ((conv[j] & 0x20) ? '1' : '0');
	    ++i;

	    out[i] = ((conv[j] & 0x10) ? '1' : '0');
	    ++i;

	    out[i] = ((conv[j] & 0x08) ? '1' : '0');
	    ++i;

	    out[i] = ((conv[j] & 0x04) ? '1' : '0');
	    ++i;

	    out[i] = ((conv[j] & 0x02) ? '1' : '0');
	    ++i;

	    out[i] = ((conv[j] & 0x01) ? '1' : '0');
	    ++i;
	    ++j;
	}
	outlen += i;
	break;

    default:
	fprintf(stderr, "%s: bitlen: unknown base format: %d\n",
		program, (int)outfmt);
	exit(14);
    }
    *p_outlen = outlen;
    return;
}


/*
 * gleam - remove non-format based characters
 *
 * given:
 *	in		buffer
 *	p_inlen		pointer to amount of data in the input buffer
 *	infmt		buffer format
 */
static void
gleam(unsigned char *in, int *p_inlen, enum format fmt)
{
    int inlen;		/* amount of data in the input buffer */
    int i;
    int j;

    /*
     * firewall
     */
    if (in == NULL || p_inlen == NULL) {
	fprintf(stderr, "%s: gleam: NULL arg(s)\n", program);
	exit(15);
    }
    inlen = *p_inlen;

    /*
     * gleam
     */
    switch (fmt) {
    case FMT_B256:
	/* all chars are binary */
	break;

    case FMT_B64:
	for (i=0, j=0; i < inlen; ++i) {
	    if (isb64[in[i]]) {
		in[i] = in[j++];
	    }
	}
	*p_inlen = j;
	break;

    case FMT_B16:
	for (i=0, j=0; i < inlen; ++i) {
	    if (isb16[in[i]]) {
		in[i] = in[j++];
	    }
	}
	*p_inlen = j;
	break;

    case FMT_B2:
	for (i=0, j=0; i < inlen; ++i) {
	    if (isb2[in[i]]) {
		in[i] = in[j++];
	    }
	}
	*p_inlen = j;
	break;

    default:
	fprintf(stderr, "%s: bitlen: unknown base format: %d\n",
		program, (int)fmt);
	exit(16);
    }
    return;
}
