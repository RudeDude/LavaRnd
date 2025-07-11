/*
 * This section was 'borrowed' from the Digest-MD5-2.09 module.
 * In particular it is based on the file Digest-MD5-2.09/SHA1/SHA1.xs.
 *
 * Why is it here?  Well:
 *
 *      1) For speed we do not want to jump in/out of the C/Perl
 *         interface and carry the turned buffers for very long
 *         in main memory.
 *
 *      2) Landon Curt Noll has some SHA-1 performance improvements
 *         which are not found in the original source code.
 *
 *      3) To make some minor changes to the code such as to remove
 *         a function that was only used by the Digest::SHA1 object
 *         constructor which we do not need.
 *
 * In all other respects the code is the same.  In particular this
 * section of code implements the standard SHA-1 algorithm as well
 * as the Digest::SHA1 module, only a little faster.
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


#if !defined(__LAVARND_SHA1_H__)
#  define __LAVARND_SHA1_H__


#  include <sys/types.h>


#  define SHA_BLOCKSIZE		64
/*
 * NOTE: SHA_DIGESTLONG must be 5 and SHA_DIGESTSIZE bust 20 or xor
 *       loops in s100.c and lavarnd.c will fail.
 */
#  define SHA_DIGESTSIZE		20
#  define SHA_DIGESTLONG		(SHA_DIGESTSIZE / sizeof(u_int32_t))

typedef struct {
    u_int32_t digest[SHA_DIGESTLONG];	/* message digest */
    u_int32_t count_lo, count_hi;	/* 64-bit bit count */
    u_int8_t data[SHA_BLOCKSIZE];	/* SHA data buffer */
    int local;	/* unprocessed amount in data */
} SHA_INFO;


/*
 * external functions
 *
 * NOTE: These SHA-1 functions are identical to the common SHA-1 functions
 * without the lava_ prefix.
 */
extern void lava_sha_init(SHA_INFO *sha_info);
extern void lava_sha_update(SHA_INFO *sha_info, u_int8_t * buffer, int count);
extern void lava_sha_final(u_int8_t * digest, SHA_INFO *sha_info);

/*
 * non-common SHA-1 functions
 */
extern void lava_sha_final2(SHA_INFO *sha_info);
extern void lava_sha1_buf(void *buf, int len, void *hash);


#endif /* __LAVARND_SHA1_H__ */
