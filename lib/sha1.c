/*
 * This section was 'borrowed' from the Digest-MD5-2.09 module.
 * In particular it is based on the file Digest-MD5-2.09/SHA1/SHA1.xs.
 *
 * Why is it here?  Well:
 *
 *	1) For speed we do not want to jump in/out of the C/Perl
 *	   interface and carry the turned buffers for very long
 *	   in main memory.
 *
 *	2) Landon Curt Noll has some SHA-1 performance improvements
 *	   which are not found in the original source code.
 *
 *	3) To make some minor changes to the code such as to remove
 *	   a function that was only used by the Digest::SHA1 object
 *	   constructor which we do not need.
 *
 * In all other respects the code is the same.  In particular this
 * section of code implements the standard SHA-1 algorithm as well
 * as the Digest::SHA1 module, only a little faster.
 *
 * NOTE: We put lava_ in front of the functions so that they will
 *	 not conflict with someone's else SHA-1 code or libs.
 *	 We make these external in case someone wants to use
 *	 a good implementation of SHA-1.
 */

/*
 * NIST Secure Hash Algorithm
 * heavily modified by Uwe Hollerbach <uh@alumni.caltech edu>
 * from Peter C. Gutmann's implementation as found in
 * Applied Cryptography by Bruce Schneier
 *
 * Further modifications to include the "UNRAVEL" stuff, below and
 * additional mods by chongo:
 *
 *	http://www.isthe.com/chongo/index.html
 */

/* This code is in the public domain */

#include <string.h>

#include "LavaRnd/sha1.h"
#include "LavaRnd/sha1_internal.h"
#include "LavaRnd/have/endian.h"

#if defined(DMALLOC)
#include <dmalloc.h>
#endif


/*
 * lava_sha_transform - transform one block of data inside SHA_INFO
 *
 * NOTE: This function is identical to the common sha_transform().
 */
void
lava_sha_transform(SHA_INFO *sha_info)
{
    int i;
    u_int8_t *dp;
    u_int32_t T, A, B, C, D, E, W[80], *WP;

    dp = sha_info->data;

    /*
     * The following makes sure that at least one code block below is
     * traversed or an error is reported, without the necessity for nested
     * preprocessor if/else/endif blocks, which are a great pain in the
     * nether regions of the anatomy...
     */
#if CALC_BYTE_ORDER == LITTLE_ENDIAN
    for (i = 0; i < 16; ++i) {
	T = *((u_int32_t *) dp);
	dp += 4;
	W[i] =  ((T << 24) & 0xff000000) | ((T <<  8) & 0x00ff0000) |
		((T >>  8) & 0x0000ff00) | ((T >> 24) & 0x000000ff);
    }
#else
    for (i = 0; i < 16; ++i) {
	T = *((u_int32_t *) dp);
	dp += 4;
	W[i] = T32(T);
    }
#endif

    for (i = 16; i < 80; ++i) {
	W[i] = W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16];
#if (SHA_VERSION == 1)
	W[i] = R32(W[i], 1);
#endif /* SHA_VERSION */
    }
    A = sha_info->digest[0];
    B = sha_info->digest[1];
    C = sha_info->digest[2];
    D = sha_info->digest[3];
    E = sha_info->digest[4];
    WP = W;
#ifdef UNRAVEL
    FA(1); FB(1); FC(1); FD(1); FE(1); FT(1); FA(1); FB(1); FC(1); FD(1);
    FE(1); FT(1); FA(1); FB(1); FC(1); FD(1); FE(1); FT(1); FA(1); FB(1);
    FC(2); FD(2); FE(2); FT(2); FA(2); FB(2); FC(2); FD(2); FE(2); FT(2);
    FA(2); FB(2); FC(2); FD(2); FE(2); FT(2); FA(2); FB(2); FC(2); FD(2);
    FE(3); FT(3); FA(3); FB(3); FC(3); FD(3); FE(3); FT(3); FA(3); FB(3);
    FC(3); FD(3); FE(3); FT(3); FA(3); FB(3); FC(3); FD(3); FE(3); FT(3);
    FA(4); FB(4); FC(4); FD(4); FE(4); FT(4); FA(4); FB(4); FC(4); FD(4);
    FE(4); FT(4); FA(4); FB(4); FC(4); FD(4); FE(4); FT(4); FA(4); FB(4);
    sha_info->digest[0] = T32(sha_info->digest[0] + E);
    sha_info->digest[1] = T32(sha_info->digest[1] + T);
    sha_info->digest[2] = T32(sha_info->digest[2] + A);
    sha_info->digest[3] = T32(sha_info->digest[3] + B);
    sha_info->digest[4] = T32(sha_info->digest[4] + C);
#else /* !UNRAVEL */
#ifdef UNROLL_LOOPS
    FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1);
    FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1); FG(1);
    FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2);
    FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2); FG(2);
    FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3);
    FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3); FG(3);
    FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4);
    FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4); FG(4);
#else /* !UNROLL_LOOPS */
    for (i =  0; i < 20; ++i) { FG(1); }
    for (i = 20; i < 40; ++i) { FG(2); }
    for (i = 40; i < 60; ++i) { FG(3); }
    for (i = 60; i < 80; ++i) { FG(4); }
#endif /* !UNROLL_LOOPS */
    sha_info->digest[0] = T32(sha_info->digest[0] + A);
    sha_info->digest[1] = T32(sha_info->digest[1] + B);
    sha_info->digest[2] = T32(sha_info->digest[2] + C);
    sha_info->digest[3] = T32(sha_info->digest[3] + D);
    sha_info->digest[4] = T32(sha_info->digest[4] + E);
#endif /* !UNRAVEL */
}


/*
 * lava_sha_init - initialize the SHA digest
 *
 * NOTE: This function is identical to the common sha_init().
 */
void
lava_sha_init(SHA_INFO *sha_info)
{
    sha_info->digest[0] = 0x67452301L;
    sha_info->digest[1] = 0xefcdab89L;
    sha_info->digest[2] = 0x98badcfeL;
    sha_info->digest[3] = 0x10325476L;
    sha_info->digest[4] = 0xc3d2e1f0L;
    sha_info->count_lo = 0L;
    sha_info->count_hi = 0L;
    sha_info->local = 0;
}


/*
 * lava_sha_update - transform whole block of data, leave partial for later
 *
 * NOTE: This function is identical to the common sha_update().
 */
void
lava_sha_update(SHA_INFO *sha_info, u_int8_t *buffer, int count)
{
    int i;
    u_int32_t clo;

    clo = T32(sha_info->count_lo + ((u_int32_t) count << 3));
    if (clo < sha_info->count_lo) {
	++sha_info->count_hi;
    }
    sha_info->count_lo = clo;
    sha_info->count_hi += (u_int32_t) count >> 29;
    if (sha_info->local) {
	i = SHA_BLOCKSIZE - sha_info->local;
	if (i > count) {
	    i = count;
	}
	memcpy(((u_int8_t *) sha_info->data) + sha_info->local, buffer, i);
	count -= i;
	buffer += i;
	sha_info->local += i;
	if (sha_info->local == SHA_BLOCKSIZE) {
	    lava_sha_transform(sha_info);
	} else {
	    return;
	}
    }
    while (count >= SHA_BLOCKSIZE) {
	memcpy(sha_info->data, buffer, SHA_BLOCKSIZE);
	buffer += SHA_BLOCKSIZE;
	count -= SHA_BLOCKSIZE;
	lava_sha_transform(sha_info);
    }
    memcpy(sha_info->data, buffer, count);
    sha_info->local = count;
}


/*
 * lava_sha_final - finish computing the SHA digest
 *
 * NOTE: digest is assumed to be at least SHA_DIGESTSIZE octets long.
 *
 * NOTE: This function is identical to the common sha_final().
 */
void
lava_sha_final(u_int8_t *digest, SHA_INFO *sha_info)
{
    int count;
    u_int32_t lo_bit_count, hi_bit_count;

    lo_bit_count = sha_info->count_lo;
    hi_bit_count = sha_info->count_hi;
    count = (int) ((lo_bit_count >> 3) & 0x3f);
    ((u_int8_t *) sha_info->data)[count++] = 0x80;
    if (count > SHA_BLOCKSIZE - 8) {
	memset(((u_int8_t *) sha_info->data) + count, 0, SHA_BLOCKSIZE - count);
	lava_sha_transform(sha_info);
	memset((u_int8_t *) sha_info->data, 0, SHA_BLOCKSIZE - 8);
    } else {
	memset(((u_int8_t *) sha_info->data) + count, 0,
	    SHA_BLOCKSIZE - 8 - count);
    }
    sha_info->data[56] = (hi_bit_count >> 24) & 0xff;
    sha_info->data[57] = (hi_bit_count >> 16) & 0xff;
    sha_info->data[58] = (hi_bit_count >>  8) & 0xff;
    sha_info->data[59] = (hi_bit_count >>  0) & 0xff;
    sha_info->data[60] = (lo_bit_count >> 24) & 0xff;
    sha_info->data[61] = (lo_bit_count >> 16) & 0xff;
    sha_info->data[62] = (lo_bit_count >>  8) & 0xff;
    sha_info->data[63] = (lo_bit_count >>  0) & 0xff;
    lava_sha_transform(sha_info);
    digest[ 0] = (unsigned char) ((sha_info->digest[0] >> 24) & 0xff);
    digest[ 1] = (unsigned char) ((sha_info->digest[0] >> 16) & 0xff);
    digest[ 2] = (unsigned char) ((sha_info->digest[0] >>  8) & 0xff);
    digest[ 3] = (unsigned char) ((sha_info->digest[0]      ) & 0xff);
    digest[ 4] = (unsigned char) ((sha_info->digest[1] >> 24) & 0xff);
    digest[ 5] = (unsigned char) ((sha_info->digest[1] >> 16) & 0xff);
    digest[ 6] = (unsigned char) ((sha_info->digest[1] >>  8) & 0xff);
    digest[ 7] = (unsigned char) ((sha_info->digest[1]      ) & 0xff);
    digest[ 8] = (unsigned char) ((sha_info->digest[2] >> 24) & 0xff);
    digest[ 9] = (unsigned char) ((sha_info->digest[2] >> 16) & 0xff);
    digest[10] = (unsigned char) ((sha_info->digest[2] >>  8) & 0xff);
    digest[11] = (unsigned char) ((sha_info->digest[2]      ) & 0xff);
    digest[12] = (unsigned char) ((sha_info->digest[3] >> 24) & 0xff);
    digest[13] = (unsigned char) ((sha_info->digest[3] >> 16) & 0xff);
    digest[14] = (unsigned char) ((sha_info->digest[3] >>  8) & 0xff);
    digest[15] = (unsigned char) ((sha_info->digest[3]      ) & 0xff);
    digest[16] = (unsigned char) ((sha_info->digest[4] >> 24) & 0xff);
    digest[17] = (unsigned char) ((sha_info->digest[4] >> 16) & 0xff);
    digest[18] = (unsigned char) ((sha_info->digest[4] >>  8) & 0xff);
    digest[19] = (unsigned char) ((sha_info->digest[4]      ) & 0xff);
}


/*
 * lava_sha_final2 - finish computing the SHA digest with no output
 *
 * NOTE: digest is assumed to be at least SHA_DIGESTSIZE octets long.
 *
 * NOTE: This function is similar to the common sha_final() except that
 *	 it does not perform a final byte for byte copyout of the
 *	 final digest.  The final digest can be accessed within the
 *	 SHA_INFO by accessing the digest sub-structure elements as
 *	 unsigned 32 bit words.
 */
void
lava_sha_final2(SHA_INFO *sha_info)
{
    int count;
    u_int32_t lo_bit_count, hi_bit_count;

    lo_bit_count = sha_info->count_lo;
    hi_bit_count = sha_info->count_hi;
    count = (int) ((lo_bit_count >> 3) & 0x3f);
    ((u_int8_t *) sha_info->data)[count++] = 0x80;
    if (count > SHA_BLOCKSIZE - 8) {
	memset(((u_int8_t *) sha_info->data) + count, 0, SHA_BLOCKSIZE - count);
	lava_sha_transform(sha_info);
	memset((u_int8_t *) sha_info->data, 0, SHA_BLOCKSIZE - 8);
    } else {
	memset(((u_int8_t *) sha_info->data) + count, 0,
	    SHA_BLOCKSIZE - 8 - count);
    }
    sha_info->data[56] = (hi_bit_count >> 24) & 0xff;
    sha_info->data[57] = (hi_bit_count >> 16) & 0xff;
    sha_info->data[58] = (hi_bit_count >>  8) & 0xff;
    sha_info->data[59] = (hi_bit_count >>  0) & 0xff;
    sha_info->data[60] = (lo_bit_count >> 24) & 0xff;
    sha_info->data[61] = (lo_bit_count >> 16) & 0xff;
    sha_info->data[62] = (lo_bit_count >>  8) & 0xff;
    sha_info->data[63] = (lo_bit_count >>  0) & 0xff;
    lava_sha_transform(sha_info);
}


/*
 * lava_sha1_buf - perform a SHA-1 hash on a buffer
 *
 * given:
 *	buf	start of data to hash
 *	int	length of buf
 *	hash	where to place the 160 bit hash
 */
void
lava_sha1_buf(void *buf, int len, void *hash)
{
    SHA_INFO sha1;	/* internal SHA-1 state */

    /*
     * initialize the SHA-1 transform
     */
    lava_sha_init(&sha1);

    /*
     * update the SHA-1 hash with the buffer
     */
    lava_sha_update(&sha1, (u_int8_t *)buf, len);

    /*
     * complete the SHA-1 hash
     */
    lava_sha_final((u_int8_t *)hash, &sha1);
    return;
}
