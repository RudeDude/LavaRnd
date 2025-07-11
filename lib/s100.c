/*
 * s100 - subtractive 100 shuffle generator
 *
 * These routines implement the subtractive 100 shuffle generator.
 * While you can call these routines directly, you may want to
 * also use the functions provided by random.c and random_libc.c.
 *
 *	This does NOT implement the LavaRnd algorithm!!!!!
 *
 *		Please READ the WARNINGS below!!!!
 *
 * @(#) $Revision: 10.2 $
 * @(#) $Id: s100.c,v 10.2 2003/11/09 22:37:03 lavarnd Exp $
 *
 * Copyright 1995,1999,2003 by Landon Curt Noll.  All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright, this permission notice, and the
 * disclaimer below appear in all of the following:
 *
 *      * supporting documentation
 *      * source copies
 *      * source works derived from this source
 *      * binaries derived from this source or from derived source
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Under source code control:	1995/01/07 09:45:25
 * File existed as early as:	1994
 *
 * chongo <was here> /\oo/\	http://www.isthe.com/chongo/
 * Share and enjoy!  :-)	http://www.isthe.com/chongo/tech/comp/calc/
 *
 * NOTE: This code was "borrowed from Calc".
 */

/*
 * NOTE: We use this generator when we cannot contact the random daemon
 *	 and the failure mode of the library is to output something
 *	 alway (instead of retrying (perhaps forever) or exiting).
 *
 *	 The output of this generator, while good if given reasonable
 *	 seed data, is NOT cryptographically strong.  This code is here
 *	 for those random interface functions that insist on returning
 *	 something no matter what ...
 *
 * NOTE: We really do not want to expose the internals of this generator
 *	 to calling functions.  This generator is intended as a last
 *	 resort to help random library calls return something.  It is
 *	 for this reason that we have only 1 single s100 generator
 *	 state as a hidden static structure and limit access to its
 *	 output.
 */

/*
 * WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW
 * AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
 * RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR
 * NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
 * IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII
 * NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN
 * GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
 *
 * Well done Pseudo-random number generators are a "black art".  Obscure and
 * subtle things abound in the code.  Small seemingly innocent changes can
 * have disastrous consequences.  This code is doing pseudo-random things.  :-)
 * It can be very difficult to determine if a code change results in better
 * or worse pseudo-random output!
 *
 * We will try to document to some level, but we cannot possibly hope to teach
 * you the theory needed to understand things.  We will point you at some
 * references and hand wave at some algorithms, but it must be up to you to
 * truly understand things before you touch this code.
 *
 * In other words:
 *
 *	If you think you understand this code, you probably don't.
 *	Study code some more until you get confused.  Next resolve your
 *	confusion by learning some theory.  Once you have learned the
 *	theory, see if you understand the code.  Then tread with care.  :-)
 */

/*
 * The subtractive 100 shuffle generator
 *
 * The subtractive 100 shuffle generator wrapped inside of a shuffle generator.
 * We refer to this generator compound generator as the s100 generator.
 *
 * References:
 *
 *	The subtractive 100 generator is described in Knuth's "The Art of
 *	Computer Programming - Seminumerical Algorithms", Vol 2, 3rd edition
 *	(1998), Section 3.6, page 186, formula (2).
 *
 *	The "use only the first 100 our of every 1009" is described in
 *	Knuth's "The Art of Computer Programming - Seminumerical Algorithms",
 *	Vol 2, 3rd edition (1998), Section 3.6, page 188".
 *
 *	The period and other properties of this generator make it very
 *	useful to 'seed' other generators.
 *
 *	The shuffle generator is described in Knuth's "The Art of Computer
 *	Programming - Seminumerical Algorithms", Vol 2, 3rd edition (1998),
 *	Section 3.2.2, page 34, Algorithm B.
 *
 *	The shuffle generator is fast and serves as a fairly good standard
 *	pseudo-random generator.   If you need a fast generator and do not
 *	need a cryptographicly strong one, this generator is likely to do
 *	the job.
 *
 *	The shuffle generator is feed values by the subtractive 100 process.
 *
 * To really understand what is going on, you should carefully review
 * starting at the above references.  Pay attention to other pointers
 * mention in the text and attempt to do some of the related exercises
 * found at the section ends.
 */

/*
 * subtractive 100 - a Hand wave overview
 *
 * Seeding:
 *
 *	Given an array of 1009 slots, fill the 1st 100 with high
 *	quality random values.  We number the slots 0 thru 1008.
 *
 *	Set the output pointer to slot 100.  Set the 1st pointer to
 *	slot 0;  Set the 2nd pointer to slot 53.
 *
 *	slot_type slot[1009];
 *	slot_type *j, *k;
 *
 *	for (i=0; i < 100; ++i) {
 *	    slot[i] = random_stuff();
 *	}
 *	out = &slot[100];
 *	j = &slot[0];
 *	k = &slot[63];
 *
 * Output:
 *
 *	Subtract the slot pointed at by the 1st pointer from the slot
 *	pointed at by the 2nd pointer.  Place that result into the
 *	slot pointed at by the output pointer.  Advance all pointers
 *	1 slot.  The output is the result of the subtraction.
 *
 *	See:
 *
 *	  Knuth: "The Art of Computer Programming - Seminumerical Algorithms",
 *	  Vol 2, 3rd edition (1998), Section 3.6, page 186, formula (2).
 *
 *	slot_type slot[1009];
 *	slot_type *j, *k, *output;
 *
 *	*output = *k++ - *j;
 *
 *	We may use only the 1st 100 out every 1009 operations.  Thus the
 *	output that can use is found in slots 100 thru 199.
 *
 *	See:
 *
 *	  Knuth: "The Art of Computer Programming - Seminumerical Algorithms",
 *	  Vol 2, 3rd edition (1998), Section 3.6, page 188".
 *
 *	slot_type slot[1009];
 *	slot_type *j, *k, *output;
 *	int i;
 *
 *	for (i=0; i < 1009; ++i) {
 *	    *output = *k++ - *j;
 *	}
 *
 *	We can optimize by shorting the slot buffer by 100 slots and
 *	wrapping the pointers around to the beginning when the fall
 *	past the 908th slot.  This will result the initial 100 slots
 *	being ready for the next 1009 cycle operation.
 *
 * Reseeding:
 *
 *	There is an open question of how many times one should go thru
 *	the 1009 cycles (100 outputs and 909 tossed values).  The is not
 *	as much a statistical question as a cryptologic question.  How
 *	output data does one require to be able to compute the internal
 *	100 slot array with some level of certainty and thus be able
 *	to predict all future output with some level of certainty?
 *	In other words how often should you reseed?
 *
 *	The answer to this question was open at the time this code
 *	was written.  A heuristic argument can be made for reseeding
 *	every 1009^3 cycles (i.e., 1009^2 * 100 returned non-skipped
 *	outputs) to make prediction difficult.  But is it difficult
 *	enough?  The answer to that question depends on your requirements,
 *	paranoia and the accuracy of the "1009^3 cycles" argument.
 *
 *	This is one place where adjustment of this code is not
 *	so hazardous.  Reseeding more frequently slows down the
 *	app and drains random daemon resources faster.  How much depends
 *	on how fast the application wants data and on how much
 *	the reseed threshold is lowered.
 *
 *	Reseeding less frequently may make output more predictable.
 *	How predictable is an open question and will, in part, depend
 *	on how far the reseed threshold is lowered.
 *
 *	Maybe it is better if you leave the threshold alone?  :-)
 */

/*
 * shuffle generator
 *
 * Seeding:
 *
 *	Start with a array of slots filled with random values.  For ease
 *	of description, assume that the array size is a power of 2, say
 *	32 slots.  Set something called output with a random value.
 *
 *	In our example, one needs 32+1 random values.  These 32+1 random
 *	values could come from the 32+1 output values of a generator such
 *	as the subtractive 100 generator.  They could come from a quality
 *	source such as LavaRnd as well.
 *
 * Output:
 *
 *	Given the previous output value (or what output was set during
 *	seeding), use its lower bits (in our case 8 bits) to select a
 *	shuffle slot.  Obtain the output from another generator (say
 *	the output value of a subtractive 100 generator) and swap it
 *	with the selected slot.  Output the swaped slot value.
 *
 *	See:
 *
 *	  Knuth: "The Art of Computer Programming - Seminumerical Algorithms",
 *	  Vol 2, 3rd edition (1998), Section 3.2.2, page 34, Algorithm B.
 *
 *	#define SLOT_POWER 5
 *	slot_type slot[1<<SLOT_POWER];
 *	int indx;
 *
 *	indx = output & ((1<<SLOT_POWER)-1);
 *	output = slot[indx];
 *	slot[indx] = other_generator();
 *	return output;
 *
 * Reseeding:
 *
 *	There is no advantage to reseed the shuffle generator.
 *	There is no need to refill the shuffle slot and previous output
 *	value when the subtractive 100 generator is reseeded, for example.
 *	On the other hand if one is reseeding the subtractive 100 generator
 *	from a high quality source, one might want to pump a little more
 *	high quality data into the shuffle slots and output value.
 */


#include "LavaRnd/sysstuff.h"
#include "LavaRnd/fnv1.h"
#include "LavaRnd/s100.h"
#include "LavaRnd/s100_internal.h"
#include "LavaRnd/lavaerr.h"

#if defined(DMALLOC)
#include <dmalloc.h>
#endif


/*
 * subtractive 100 shuffle generator default state
 *
 * If we cannot contact random daemon (and our lib interface does not permit
 * us to fail), we will be forced to use this generator instead.
 *
 * IMPORTANT NOTE:
 *
 *	 The output of this generator, while good if given
 *	 reasonable seed data, is NOT cryptographically strong!
 *
 * This pre-compiled initial state was produced from the lavadump v1.9
 * tool which directly read Luminance values from a Logitech QuickCam
 * 3000 Pro camera on 15-Apr-2002 21:38:07 PDT (using the lavacam driver
 * v1.23 pwc sub-component v1.30 with flags -L -T 3, PWC 8.6 kernel
 * module, PWCX 8.2.1 module) and feed the Luminance values
 * into the LavaRnd process v2.4 to produce cryptographically strong
 * random numbers.  The binary random numbers were converted to hex
 * values by the Linux hexdump(1) utility and placed in the table below
 * with the help of the Linux vim(1) editor.
 *
 * You are welcome to try and replace the constants found below
 * with your own ... provided that you KNOW that you constants
 * are the output of a cryptographically strong random source.
 * The trick is knowing if you truly know.  :-)
 */
static struct s100_seed def_seed = {

  /* previous subtractive 100 output */
  0xa4b04eeabd051692ULL,

  /* shuffle table slots */
  {
    0xd2d6ee9381c4ba7fULL, 0x3b2e12296c69e68eULL, 0x5f27f86f3e43051eULL,
    0xe99e18643620fc1dULL, 0xb913dbd92e9c13f6ULL, 0x88d3912a2f68133bULL,
    0xd0072d344731c9deULL, 0xff6c5717b05d854fULL, 0xee1fb3cf9694bbe4ULL,
    0x7dca8708c29d91b6ULL, 0xfa8511bfba4114adULL, 0xfbe33cc3dc65e65bULL,
    0x09d67a2e93246323ULL, 0xe1473fc55e7c1d8bULL, 0x7fe85768d958b062ULL,
    0x901bf47bb5ab214eULL, 0x6829f2daee8debe0ULL, 0xd3d0a6651fd497b7ULL,
    0x54a05c2f5ad15c08ULL, 0xcfe8a69942e90e3aULL, 0x4a38e14115ed4edcULL,
    0xe100d64f12f65923ULL, 0x11c79fce3470c701ULL, 0x0c9d8a7b760ae816ULL,
    0x2ae342a27a4609a7ULL, 0x403d57f741392934ULL, 0xc23b30b860383c98ULL,
    0xf4f153f444629bbdULL, 0x42d93a2226e1b824ULL, 0x8ede000c3d78ba45ULL,
    0x81c8bceb8e6f6d75ULL, 0xfbeac25e5163ed9bULL, 0x3869fb481b78ae86ULL,
    0x2452fba714a381b8ULL, 0x8adb483ead3a79f2ULL, 0x17e69879db9b21feULL,
    0x8598237b42b94818ULL, 0x60604a3f8313603bULL, 0x54f17d3788e189e8ULL,
    0x4371106635c80607ULL, 0x8de5b4c48aa84eb4ULL, 0xde1891849efd781fULL,
    0x1631dff373d258a3ULL, 0xadb288b3d420c7c3ULL, 0x1a61e68a31a6650bULL,
    0xe3ff618c2859452eULL, 0x02716b7efcfb789cULL, 0xf354eaca5231514fULL,
    0x5ad593b52cd71ab8ULL, 0xac31032e226d67e4ULL, 0x8d04b3d85162d198ULL,
    0x39c2c21c558e311dULL, 0x2483bda8cb7f5563ULL, 0x0c1076bc3ddcca9cULL,
    0x912ae7aedd803f2dULL, 0xedec6fb7338419f6ULL, 0xc93a76136d0728e3ULL,
    0x5121643bd2efb0d6ULL, 0xb512fe47f085b363ULL, 0x0b03f565c7277d6fULL,
    0x6dda452a17a12966ULL, 0x7ecab1638c1c2b0fULL, 0xde487d3f2f68b7c4ULL,
    0x681b8d7b55efe676ULL
  },

  /* subtractive 100 table slots */
  {
    0x2c2b94f22e0ec732ULL, 0x1fe71be91e39bf76ULL, 0xc42612d4dd131b6aULL,
    0xa00d20cab85f6be9ULL, 0xf332803fd2df8256ULL, 0x8133782596659153ULL,
    0x33f73ae9f214d495ULL, 0xa6a8294b4328f9ddULL, 0x4b10dcb6c4a19b54ULL,
    0xb3ea82c6c523c5ffULL, 0x62e3711d2aadf4bdULL, 0x10f043290a6ac937ULL,
    0x64fed2d89c2cf347ULL, 0x3165098180c5bb9fULL, 0xa257b87f884c3759ULL,
    0x6e52236528645e05ULL, 0xcd1d24fc4296bf09ULL, 0x9636dcec52ece57bULL,
    0x017f307f42d93c2cULL, 0xcea5ff072991e5e0ULL, 0x96b25fdfd3b08950ULL,
    0x35457ef5f9a2ed46ULL, 0x72566e363b785e2bULL, 0xf0deab3d61953345ULL,
    0x3f6459914d7f7ec9ULL, 0x30efdb99564a7ff5ULL, 0x5db59863a8732022ULL,
    0x24644dc44384b8dbULL, 0x27bcfe8c28e041acULL, 0x961af58ca61b4118ULL,
    0xb3ab5af0328456d9ULL, 0x7ffee4b0f9b7f218ULL, 0xa6471285d4fe3b87ULL,
    0xe9264c5862e9ced2ULL, 0x56eb08b062400b9fULL, 0xd593d9e50580b789ULL,
    0x0c381ccf918a5c43ULL, 0x4257dbca60aa2f73ULL, 0x23d2452555cd9b9dULL,
    0xa5ca503d7ab33fb4ULL, 0x70dcf82af9e1ec3bULL, 0xcb7fc8683211bbffULL,
    0xedbb5f5acba695e0ULL, 0xbc161d85e891dc7cULL, 0x2bb739361be8a811ULL,
    0xd72d86f6f34d75bdULL, 0xc486b4fd9e8f491aULL, 0x6d586d46e9355c39ULL,
    0x29bfa9527005273fULL, 0xab5c08291c3494b7ULL, 0x29c83105f9bb7815ULL,
    0xc9b632fd4f7cdaabULL, 0x6084bd6a1a1d6a3dULL, 0x8b260be42e065548ULL,
    0x74a235b92d27ff1cULL, 0x35a2bca28c8f3917ULL, 0x173f96ae810554adULL,
    0x68bd8e3b229d01e0ULL, 0x646bafacc4d73140ULL, 0x94d0fb37a701c759ULL,
    0x2133a22705600a60ULL, 0x43dea0f3b5815fd5ULL, 0x9eb02ce18879c5f0ULL,
    0x140fcb73cd470016ULL, 0xb25772b0ebdd9a13ULL, 0x908f52f327b4e3c6ULL,
    0x0b075e42de4fd0f1ULL, 0x33616d981c027bb2ULL, 0xc5b01c4049688b86ULL,
    0xfac6bc466586f4c0ULL, 0x054b3f5bfe2566ceULL, 0x8e6599f90f3e46fdULL,
    0x7b50d1f1b45c1ecbULL, 0x1123f1f3ca759831ULL, 0x9e32174c2b23e6b8ULL,
    0x28cbcdc50d86082fULL, 0x616e1261816c03daULL, 0xb6dc4f1675f9079eULL,
    0x249e69816617a13fULL, 0x25bfcaa3923909d1ULL, 0x41e256b17d4eed33ULL,
    0x3ca5c11e90a15abbULL, 0xe2dc3c2998286911ULL, 0xa44c15692c277452ULL,
    0xfab7c6b6333b411fULL, 0x5e41bc84931e9a3bULL, 0xdd874c628bfa2c6eULL,
    0x1315a8a4c92dbf0fULL, 0x3a895cbeb8d69b74ULL, 0xcac5753dcd65799bULL,
    0x42c40644a272afd5ULL, 0xd81d31a2081b4ceaULL, 0xb095ab4e51082fd3ULL,
    0x417195d28a0549f3ULL, 0x5b76246d349d2469ULL, 0xcec75a410190a32fULL,
    0x8a149e0ab909f083ULL, 0x20afa9d950ed5705ULL, 0x521ca7a9b6d441c4ULL,
    0x861ec53b7ca91307ULL
  }
};


/*
 * s100 state - internal subtractive 100 shuffle generator state
 *
 * This is the state kept for the internal subtractive 100 shuffle generator.
 * We return this data when we cannot obtain data from random daemon and our
 * library interface forces us to return something anyway.
 */
static s100shuf s100_internal = {	/* internal state when state is NULL */
    0,			/* not seeded */
    0,			/* need to seed now */
    0,			/* no seed length */
    0,			/* no octets in output buffer */
    LAVA_QUAL_NONE,	/* no initial quality */
    NULL,		/* no initialized output buffer */
};


/*
 * seed_hash_mix - mix SHA-1 hashes of a buffer with the current s100 state
 *
 * given:
 *	buf	we will mix SHA-1 hashes of this buffer with the s100 state
 *	len	length of buf
 *
 * We will add SHA-1 hash value with slots count in the s100 state.
 * Since a SHA-1 hash is much smaller than the s100 state, we need a
 * way to "extend" the SHA-1 hash.
 *
 * To "extend" the SHA-1 hash we will repeatedly xor SHA-1 hashes
 * to successive 20 octet chunks of the s100 state.  The 1st SHA-1 hash
 * will be the hash of the buffer.  Subsequent SHA-1 hashes will be formed
 * by taking the SHA-1 hash of previously xored 20 octet chunk and
 * the next un-xored 20 octet chunk.  These subsequent SHA-1 hashes will
 * use the previous SHA-1 hash state.
 *
 * The effect of the subsequent SHA-1 hashes is a cypher feedback-like mode.
 *
 * NOTE: The s100 overlay is, by design, a multiple of SHA-1 hash sizes.
 *	 It may extended beyond end of the s100 state, but it will not
 *	 extended beyond the end of the s100_seed structure.
 *
 * NOTE: For performance reasons we assume that SHA_DIGESTLONG is 5.
 */
static void
seed_hash_mix(s100shuf *s100, u_int8_t *buf, int len)
{
    SHA_INFO sha1;	/* SHA-1 hash and hash state */
    int i;
    int j;

    /*
     * use internal state if s100 is NULL
     */
    if (s100 == NULL) {
	s100 = &s100_internal;
    }

    /*
     * we start off by SHA-1 hashing the supplied buffer
     */
    lava_sha_init(&sha1);
    lava_sha_update(&sha1, buf, len);
    lava_sha_final2(&sha1);

    /*
     * xor in each SHA-1's sized chunk in the s100 overlay
     */
    for (i=0, j=0; i < SEED_SHA1-1; ++i) {

	/*
	 * xor in the next SHA_DIGESTLONG (5) s100 overlay words
	 */
	s100->s.overlay[j++] ^= sha1.digest[0];
	s100->s.overlay[j++] ^= sha1.digest[1];
	s100->s.overlay[j++] ^= sha1.digest[2];
	s100->s.overlay[j++] ^= sha1.digest[3];
	s100->s.overlay[j++] ^= sha1.digest[4];

	/*
	 * hash the previous xored and the next un-xored chunk
	 */
        lava_sha_update(&sha1,
			(u_int8_t *)(s100->s.overlay+j-5),
			SHA_DIGESTSIZE*2);
	lava_sha_final2(&sha1);
    }

    /*
     * xor in the final SHA_DIGESTLONG (5) s100 overlay words
     */
    s100->s.overlay[j++] ^= sha1.digest[0];
    s100->s.overlay[j++] ^= sha1.digest[1];
    s100->s.overlay[j++] ^= sha1.digest[2];
    s100->s.overlay[j++] ^= sha1.digest[3];
    s100->s.overlay[j++] ^= sha1.digest[4];
    return;
}


/*
 * s100_load_size - random octets needed to fully seed the s100 generator
 *
 * The reason why this function returns the size of struct s100_seed
 * and not struct s100_state is that s100_turn() only assumes that
 * the first 100 (S100) slots have valid data.  The s100_turn()
 * function fills in the remainer of the slot entries and wraps
 * around to the first 100 slots for the complete 1009 (SPIN_CYCLE)
 * cycle making the first 100 slots ready for the next s100_turn() call.
 */
int
s100_load_size(void)
{
    return sizeof(struct s100_seed);
}


/*
 * s100_load - load random data into the s100 initial state and seed
 *
 * given:
 * 	s100	pointer to s100 state (NULL ==> use default internal state)
 *	buf	random buffer (or NULL ==> no buffer)
 *	len	length, in octets, of random buffer (or 0 ==> no buffer)
 *		   (try to send in at least s100_load_size() octets)
 *
 * This function will use the random buffer data to load into the
 * s100 generator state.  If the buffer is too small, then the
 * remainder of the generator state is initialized to the
 * default values and is mixed with system stuff.
 *
 * If the buffer is large enough to completely load into the
 * s100 generator seed, the default state is ignored and
 * no system stuff is mixed in.
 *
 * A side effect of loading is to clear out the current rndbuf buffered data.
 * This allows a caller, if it does not like the quality of the buffered
 * data to load with a higher quality seed and have it take immediate effect.
 *
 * NOTE: It is critical that the random buffer that is passed
 *	 be filled with cryptographic quality random data such as data
 *	 from random daemon.
 *
 * NOTE: If you don't have access to any cryptographic quality data,
 *	 pass a NULL ptr and a 0 length.  If you have only a little
 *	 (but not enough) data, then provide that data.  In either case
 *	 the default + system stuff mixing process will be used and
 *	 we will hope of the best.
 *
 * NOTE: If enough data of cryptographic quality is given to this
 *	 function, then the output of this s100 generator will be
 *	 of very high quality, but not cryptographicly strong.
 *	 If you pass in bogus data, you will get bogus output.
 *
 * NOTE: The s100_load_size() will return the amount of data needed
 *	 to avoid using the default + system stuff mixing process.
 *	 If you pass more data that is needed, the unused portion
 *	 will be ignored.
 *
 * NOTE: DO NOT pass s100 generator data into this function.  That won't
 *	 improve things.  You should feed in random data from a source
 *	 that is of higher quality than the s100 generator.
 */
void
s100_load(s100shuf *s100, u_int8_t *buf, int len)
{
    static struct system_stuff stuff;	/* quasi-bogus seed */
    static int stuff_set = FALSE;	/* TRUE ==> fixed stuff already set */

    /*
     * use internal state if s100 is NULL
     */
    if (s100 == NULL) {
	s100 = &s100_internal;
    }

    /*
     * note that we are not seeded
     */
    s100->seeded = 0;

    /*
     * case: we have enough random data to directly load
     */
    if (buf != NULL && len >= (int)sizeof(s100->s.seed)) {

	/* no default or system stuff mixing */
	memcpy((void *)&s100->s.seed, buf, sizeof(s100->s.seed));
	s100->seed_len = sizeof(s100->s.seed);

    /*
     * case: we need to use default state and system stuff mixing
     */
    } else {

	/* setup seed to default values */
	s100->s.seed = def_seed;

	/*
	 * load in any random data that we might have been given
	 *
	 * NOTE: We do not clear out any existing data in the buffer.
	 *	 We consider any previous data part of the 'system stuff'.
	 *	 This small-seed processing is not designed to be repeatable.
	 *	 For repeatable seeds, use a seed that is >= the
	 *	 size returned by s100_load_size.
	 */
	if (buf != NULL && len > 0) {
	    if (len >= (int)sizeof(s100->s.seed)) {
		memcpy((void *)&s100->s.seed, buf, sizeof(s100->s.seed));
		s100->seed_len = sizeof(s100->s.seed);
	    } else {
		memcpy((void *)&s100->s.seed, buf, len);
		s100->seed_len = len;
	    }
	} else {
	    s100->seed_len = 0;
	}

	/* collect some system stuff */
	system_stuff(&stuff, stuff_set);
	stuff_set = TRUE;

	/* mix the system stuff with the s100 state */
	seed_hash_mix(s100, (u_int8_t *)&stuff, (int)sizeof(stuff));
    }

    /*
     * initialize the required counters and pointers
     */
    s100->nextspin = S100_RESEED_CYCLES;
    s100->nxt_rnd = &(s100->rndbuf[0]);
    s100->rndbuf_len = 0;

    /*
     * note that we are seeded
     */
    s100->seeded = 1;
    return;
}


/*
 * s100_unload - clear out any s100 state so the next call will reseed
 *
 * given:
 * 	s100	pointer to s100 state (NULL ==> use default internal state)
 *
 * Calling this function as the effect of removing the current s100 seed,
 * flushing any buffered state, and clearing the current quality level.
 *
 * One might want to call this function if, due to a previous lavapool
 * failure, that the s100 quality as degraded below an acceptable level
 * and you want to force an attempt to reseed from a (hopefully now working)
 * lavapool daemon.
 */
void
s100_unload(s100shuf *s100)
{
    /*
     * use internal state if s100 is NULL
     */
    if (s100 == NULL) {
	s100 = &s100_internal;
    }

    /*
     * note that we are not seeded
     */
    s100->seeded = 0;

    /*
     * zero out all s100 state
     */
    memset((void *)&s100->s.overlay, 0, sizeof(s100->s.overlay));

    /*
     * initialize the required counters and pointers
     */
    s100->nxt_rnd = &(s100->rndbuf[0]);
    s100->rndbuf_len = 0;

    /*
     * clear the quality state
     */
    s100->bufqual = LAVA_QUAL_NONE;
}


/*
 * s100_turn - output 100 values, skip 909 more from the s100 generator
 *
 * given:
 * 	s100	pointer to s100 state (NULL ==> use default internal state)
 *	ptr	pointer to 100 u_int64_t values
 *
 * returns:
 *	0 ==> no need to call s100_load(), 100 u_int64_t values copied out
 *	1 ==> need to call s100_load(), 100 u_int64_t values copied out
 *
 * NOTE: If this function is called without first calling s100_load(),
 *	 then the default pseudo-random number sequence will be produced.
 *
 * NOTE: This function will not attempt to re-load the s100 generator.
 *	 If an excessive amount of data is asked for, it is possible
 *	 that the s100 generator will be used for a period that is longer
 *	 than is desirable.
 *
 *	 Running the generator too long without a refill is not fatal.
 *	 Indeed there is an open question as to much often the generator
 *	 should be refilled.  It is probably a good idea to check the
 *	 return of this function.  If the return is non-zero, then the
 *	 s100_load() function should be called with a buffer filled with
 *	 random data.  DON'T feed this generator data back into s100_load().
 *	 That won't help.  Give s100_load() data from a higher quality
 *	 source.  Use s100_load_size() to determine how much data is needed.
 *	 If you don't have that much quality data, ten give as much as
 *	 you can to s100_load().
 *
 * NOTE: Use s100_loadleft() to determine how much data can be returned
 *	 before a call to s100_load() is recommended to avoid running
 *	 the generator too long without a refill.
 *
 * NOTE: This function does NOT check for a NULL output pointer.
 * 	 It is assumed that the caller will pass a valid pointer to
 * 	 a buffer of 100 u_int64_t values.
 */
int
s100_turn(s100shuf *s100, u_int64_t *ptr)
{
    u_int64_t *j;	/* 1st slot pointer */
    u_int64_t *k;	/* 2nd slot pointer */
    u_int64_t *out;	/* output slot pointer */
    u_int64_t *beyond;	/* beyond the end of the slot buffer */
    int indx;		/* shuffle index */

    /*
     * use internal state if s100 is NULL
     */
    if (s100 == NULL) {
	s100 = &s100_internal;
    }

    /*
     * Spin a full set of 1009 cycles.
     *
     * We need only a 909 slot buffer because we wrap the pointers around
     * from the 908th slot to the 0th slot.  This results in the subtractive
     * 100 output slots (that need to be sent to the shuffle generator)
     * being found in the 100th thru the 199th slots.  Also the next 100
     * slots (for use in the next s100_turn()) being placed into the
     * 1st 100 slots.
     */
    beyond = &(s100->s.state.slot[SPIN_CYCLE-S100]);
    out = &(s100->s.state.slot[S100]);
    j = out-S100;
    k = out-S100_PTR_OFFSET;
    while(out < beyond) {
	*out++ = *j++ - *k++;
    }
    out = &(s100->s.state.slot[0]);
    while(k < beyond) {
	*out++ = *j++ - *k++;
    }
    k = &(s100->s.state.slot[0]);
    while(j < beyond) {
	*out++ = *j++ - *k++;
    }

    /*
     * Feed the 1st 100 outputs (slots 100 thru 199) into the
     * shuffle generator and deposit the output in the ptr argument.
     */
    out = &(s100->s.state.slot[S100]);
    beyond = out + S100;
    while (out < beyond) {
	indx = *out & S100_SHUF_MASK;
	*ptr++ = s100->s.state.shuf[indx];
	s100->s.state.shuf[indx] = *out++;
    }

    /*
     * Note that we have completed a 1009 cycle of the the subtractive
     * 100 shuffle generator.
     */
    if (s100->nextspin > 0) {
	--s100->nextspin;
	return 0;
    }
    /* need to reseed */
    return 1;
}


/*
 * s100_randomcpy - copy out s100 random data
 *
 * given:
 * 	s100	pointer to s100 state (NULL ==> use default internal state)
 *	ptr	where to place random data (NULL ==> probe only, copy nothing)
 *	len	octets to copy (<= 0 ==> probe only, copy nothing)
 *
 * return:
 *	0 ==> no need to call s100_load()
 *	1 ==> need to call s100_load()
 *
 * NOTE: If this function is called without first calling s100_load(),
 *	 then the default pseudo-random number sequence will be produced.
 *
 * NOTE: This function will not attempt to re-load the s100 generator.
 *	 If an excessive amount of data is asked for, it is possible
 *	 that the s100 generator will be used for a period that is longer
 *	 than is desirable.
 *
 *	 Running the generator too long without a refill is not fatal.
 *	 Indeed there is an open question as to much often the generator
 *	 should be refilled.  It is probably a good idea to check the
 *	 return of this function.  If the return is non-zero, then the
 *	 s100_load() function should be called with a buffer filled with
 *	 random data.  DON'T feed this generator data back into s100_load().
 *	 That won't help.  Give s100_load() data from a higher quality
 *	 source.  Use s100_load_size() to determine how much data is needed.
 *	 If you don't have that much quality data, ten give as much as
 *	 you can to s100_load().
 *
 * NOTE: Use s100_loadleft() to determine how much data can be returned
 *	 before a call to s100_load() is recommended to avoid running
 *	 the generator too long without a refill.
 *
 * The bufqual value, the quality if the buffered data, may be modified by
 * this function.  It is done in the event that this function leaves some
 * of it behind.  If some buffered data is left behind, then s100_quality()
 * will use the bufqual value to return a true quality value of the next
 * data that this function will return.
 *
 * The quality returned by this function will remain in effect for
 * the output of s100_loadleft() octets, if s100_loadleft() returns > 0.
 * If s100_loadleft() returns 0, then the quality returned by this function
 * will remain until the generator is reloaded via s100_load().
 */
int
s100_randomcpy(s100shuf *s100, u_int8_t *ptr, int len)
{
    int ret;		/* s100_fill() return indicator */

    /*
     * use internal state if s100 is NULL
     */
    if (s100 == NULL) {
	s100 = &s100_internal;
    }

    /*
     * firewall
     */
    ret = (s100->nextspin > 0) ? 0 : 1;
    if (ptr == NULL || len <= 0) {
	return ret;
    }

    /*
     * fill the internal buffer if it is empty
     */
    if (s100->rndbuf_len <= 0) {

	/* note the quality data we are about to buffer */
	s100->bufqual = s100_quality(s100);

	/* fill */
	ret = s100_turn(s100, (u_int64_t *)&(s100->rndbuf[0]));

	/* buffer accounting */
	s100->nxt_rnd = &(s100->rndbuf[0]);
	s100->rndbuf_len = S100_BUF;

    /*
     * firewall - NULL nxt_rnd pointer
     */
    } else if (s100->nxt_rnd == NULL) {
	s100->nxt_rnd = &(s100->rndbuf[0]);
    }

    /*
     * case: buffer can fulfill our request, copy out as much we need
     *	     and then return
     */
    if (len <= s100->rndbuf_len) {

	/* copy out all that we need */
	memcpy(ptr, s100->nxt_rnd, len);

	/* accounting */
	s100->rndbuf_len -= len;
	s100->nxt_rnd += len;
	return ret;

    /*
     * case: request is larger than our buffer, copy out all that we have
     *	     and enter into a fill buffer / copyout cycle
     */
    } else {

	/* copy out all that we have */
	memcpy(ptr, s100->nxt_rnd, s100->rndbuf_len);

	/* accounting */
	len -= s100->rndbuf_len;
	ptr += s100->rndbuf_len;
	s100->nxt_rnd = &(s100->rndbuf[0]);
	s100->rndbuf_len = 0;
    }

    /*
     * fill the internal buffer and copy out until we are done
     *
     * At this point we know that the internal buffer is empty and
     * that we need more data to fulfill the request.
     */
    do {

	/* note the quality data we are about to buffer */
	s100->bufqual = s100_quality(s100);

	/*
	 * fill the internal buffer
	 */
	ret = s100_turn(s100, (u_int64_t *)&(s100->rndbuf[0]));
	s100->rndbuf_len = S100_BUF;

	/*
	 * case: copy a full buffer
	 */
	if (len >= S100_BUF) {

	    /* copy out the entire buffer */
	    memcpy(ptr, s100->nxt_rnd, S100_BUF);

	    /* accounting */
	    len -= S100_BUF;
	    ptr += S100_BUF;
	    s100->rndbuf_len = 0;

	/*
	 * case: copy out a partial buffer and return
	 */
	} else {

	    /* copy out all that we need */
	    memcpy(ptr, s100->nxt_rnd, len);

	    /* accounting */
	    s100->rndbuf_len -= len;
	    s100->nxt_rnd += len;
	    len = 0;
	}
    } while (len > 0);
    return ret;
}


/*
 * s100_loadleft - number of octets before s100_load() should be called
 *
 * given:
 * 	s100	pointer to s100 state (NULL ==> use default internal state)
 *
 * return:
 *	number of octets that can be output before a reseed is suggested
 *
 * NOTE: The number of octets that can be output are LAVA_QUAL_S100HIGH
 *	 fully seeded octets.
 */
int
s100_loadleft(s100shuf *s100)
{
    /*
     * use internal state if s100 is NULL
     */
    if (s100 == NULL) {
	s100 = &s100_internal;
    }

    /*
     * return the amount of data left before a reseed is needed
     */
    if (s100->seeded) {
	return s100->nextspin*S100*sizeof(u_int64_t) + s100->rndbuf_len;
    } else {
        return 0;
    }
}


/*
 * s100_quality - return the quality of the s100 generator's next data
 *
 * given:
 * 	s100	pointer to s100 state (NULL ==> use default internal state)
 *
 * return:
 *	LAVA_QUAL_S100HIGH (s100 loaded and ready)
 *	or LAVA_QUAL_S100MED (s100 seeded but needs loading partial seed)
 *	or LAVA_QUAL_S100LOW (s100 not seeded)
 *
 * The quality returned by this function will remain in effect for
 * the output of s100_loadleft() octets, if s100_loadleft() returns > 0.
 * If s100_loadleft() returns 0, then the quality returned by this function
 * will remain until the generator is reloaded via s100_load().
 *
 * Reloading the generator (via s100_load()) may effect the quality of
 * the next data.  One should call this function after reloading and
 * before new data is obtained (via s100_randomcpy()) to obtain an accurate
 * next quality.
 *
 * NOTE: A current seed that was not s100_load_size() in size is considered
 *	 as good as a stale seed that was at least s100_load_size() in size.
 *	 If a seed is not s100_load_size() in size, then some default state
 *	 perturbed by the current system state was used.
 *
 * NOTE: A stale partial seed is as good as no seed.  In the no seed case,
 *	 only the default seed perturbed by the current system state is used.
 */
lavaqual
s100_quality(s100shuf *s100)
{
    lavaqual quality = LAVA_QUAL_NONE;		/* next s100 data quality */

    /*
     * use internal state if s100 is NULL
     */
    if (s100 == NULL) {
	s100 = &s100_internal;
    }

    /*
     * case: we seeded at least once
     */
    if (s100->seeded) {

	/*
	 * case: we are not in need of reseeding
	 */
    	if (s100->nextspin > 0) {
	    if (s100->seed_len >= (int)sizeof(s100->s.seed)) {
		quality = LAVA_QUAL_S100HIGH;
	    } else if (s100->seed_len > 0) {
		quality = LAVA_QUAL_S100MED;
	    } else {
		quality = LAVA_QUAL_S100LOW;
	    }

	/*
	 * case:  we need to reseed
	 */
	} else {
	    if (s100->seed_len >= (int)sizeof(s100->s.seed)) {
		quality = LAVA_QUAL_S100MED;
	    } else {
		quality = LAVA_QUAL_S100LOW;
	    }
	}

    /*
     * case: we have never seeded
     */
    } else {
	quality = LAVA_QUAL_S100LOW;
    }

    /*
     * if we have buffered data, it may lower our current quality
     */
    if (s100->rndbuf_len > 0 && (int)(s100->bufqual) < (int)quality) {
	quality = s100->bufqual;
    }

    /*
     * firewall - we should never output LAVA_QUAL_NONE, the worst
     *		  s100 can so is LAVA_QUAL_S100LOW
     */
    if ((int)quality <= (int)LAVA_QUAL_S100LOW) {
	quality = LAVA_QUAL_S100LOW;
    }

    /*
     * return the quality of the next data that the s100 generator will return
     */
    return quality;
}
