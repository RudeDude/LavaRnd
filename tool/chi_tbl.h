/* $Id: chi_tbl.h,v 10.1 2003/08/18 06:44:37 lavarnd Exp $  */
/*
 * chi_tbl - chi square table
 *
 * Given:
 *
 *      freedom - degrees of freedom
 *      chi - chi_square value
 *
 * then for chi <= CHI_TBL:
 *
 *      chi_tbl[freedom][x] >= chi > chi_tbl[freedom[x+1]
 *
 * occurs with a probability of at least chiprob[x] if the sample taken
 * was large enough.
 *
 * Another way of looking at this is if our sample size was
 * significant, then a table value of chi_tbl[f][x] means:
 *
 *         our chi will be        with a probability of about
 *         ---------------        ---------------------------
 *       chi >  chi_tbl[f][x]             chiprob[x]
 *       chi <= chi_tbl[f][x]          1.0 - chiprob[x]
 *
 * given 'f' degrees of freedom.  We can further refine the probability
 * result by interpolating between chiprob[x] and chiprob[x+1].
 *
 * For degrees of CHI_TBL < freedom <= CHI_LARGE, we have chi
 * values for freedom levels in multiples of 10 in:
 *
 *      chi_tbl10[i][x]
 *
 * where freedom = 30+(i*10).  We will interpolate for freedom
 * values that are not directly found in the table.
 *
 * For degrees of CHI_LARGE < freedom, the following formula
 * allows one to construct a 'chi_tblx[freedom][i]'-like value:
 *
 *   chi_tblx[freedom][i] = freedom + sqrt(2*freedom)*chi_x[x] +
 *                          (2/3)*chi_x[x]*chi_x[x] - 2/3 + CHI_T/sqrt(freedom)
 *
 * where CHI_T seems to be the value 1.0 for degrees freedom near 100.
 * We apply the same interpolation methods above on this constructed
 * line to compute a probability value.
 *
 * See Knuth's "Art of Computer Programming - 2nd edition",
 * Volume 2 ("Seminumerical Algorithms"), Section 3.3.1.
 * See p. 41.
 *
 * Also see "Handbook of Mathematical Functions", M. Abromowitz &
 * I. A. Stegun; National Bureau of Standards, U.S. Government Printing
 * Office, Washington D.C., 9th printing, Nov 1970; Table 26.8,
 * pages 984-985.
 */
/*
 * Copyright 1987,1988,1989,2003 by Landon Curt Noll.  All Rights Reserved.
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
 */

/*
 * chi_tbl.h constants
 *
 * Because chi_tbl10 is a multiple of 10 table, CHI_TBL and CHI_LARGE
 * must also be multiples of 10.
 */
#define CHI_PROB 24		/* # of slots in chi_tbl[], length of chiprob[] */
#define CHI_PROB10 19		/* # of slots in chi_tbl[], length of chiprob10[] */
#define CHI_PROBX 27		/* # of slots in chi_x[], length of chiprobx[] */
#define CHI_TBL 30		/* max degree of freedom listed in chi_tbl[] */
#define CHI_LARGE 100		/* freedom condition for large approximation */
#define CHI_T (double)1.0)	/* O(1/sqrt(freedom)) for chi_x[] */
#define CHI_BIG ((double)250.0)	/* bigger than life chi value */
#define CHI_BIG10 ((double)750.0)	/* bigger than life chi10 value */
#define CHI_BIGX ((double)7.5)	/* bigger than line chi X value */

/*
 * chiprob - probability that chi will be < chi_tbl[f][x] is chiprob[x]
 */
static double chiprob[CHI_PROB] = {
    1.000, 0.995, 0.990, 0.980, 0.975,
    0.950, 0.900, 0.800, 0.750, 0.700,
    0.500, 0.300, 0.250, 0.200, 0.100,
    0.050, 0.025, 0.020, 0.010, 0.005,
    0.001, 0.0005, 0.0001,
    0.000
};

/*
 * chi_tbl - chi square table
 */
static double chi_tbl[CHI_TBL + 1][CHI_PROB] = {
    /* just a place holder */
    {0.0, 0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0, 0.0, 0.0,
     0.0, 0.0, 0.0,
     CHI_BIG},

    /* freedom = 1 */
    {0.0, 0.0000392704, 0.000157088, 0.000628, 0.000982069,
     0.00393214, 0.0157908, 0.0642, 0.101531, 0.148,
     0.454937, 1.074, 1.32330, 1.642, 2.70554,
     3.84146, 5.02389, 5.412, 6.63490, 7.87944,
     10.828, 12.116, 15.137,
     CHI_BIG},

    /* freedom = 2 */
    {0.0, 0.010, 0.0201007, 0.0404, 0.0506356,
     0.102587, 0.210720, 0.446, 0.575364, 0.713,
     1.38629, 2.408, 2.77259, 3.219, 4.60517,
     5.99147, 7.37776, 7.824, 9.21034, 10.5966,
     13.816, 15.202, 18.421,
     CHI_BIG},

    /* freedom = 3 */
    {0.0, 0.0717212, 0.114832, 0.185, 0.215795,
     0.351846, 0.584375, 1.005, 1.212534, 1.424,
     2.376597, 3.665, 4.10835, 4.642, 6.25139,
     7.81473, 9.34840, 9.837, 11.3449, 12.8381,
     16.266, 17.730, 21.108,
     CHI_BIG},

    /* freedom = 4 */
    {0.0, 0.206990, 0.297110, 0.429, 0.484419,
     0.710721, 1.06623, 1.649, 1.92255, 2.195,
     3.35670, 4.878, 5.38527, 5.989, 7.77944,
     9.48773, 11.1433, 11.668, 13.2767, 14.8602,
     18.467, 19.997, 23.513,
     CHI_BIG},

    /* freedom = 5 */
    {0.0, 0.411740, 0.554300, 0.752, 0.831221,
     1.145476, 1.61031, 2.343, 2.67460, 3.000,
     4.35146, 6.064, 6.62568, 7.289, 9.23635,
     11.0705, 12.8325, 13.388, 15.0863, 16.7496,
     20.515, 22.105, 25.745,
     CHI_BIG},

    /* freedom = 6 */
    {0.0, 0.675727, 0.87085, 1.134, 1.237347,
     1.63539, 2.20413, 3.070, 3.45460, 3.828,
     5.34812, 7.231, 7.84080, 8.558, 10.6446,
     12.5916, 14.4494, 15.033, 16.8119, 18.5476,
     22.458, 24.103, 27.856,
     CHI_BIG},

    /* freedom = 7 */
    {0.0, 0.989265, 1.239043, 1.564, 1.68987,
     2.16735, 2.83311, 3.822, 4.25485, 4.671,
     6.34581, 8.383, 9.03715, 9.803, 12.0170,
     14.0671, 16.0128, 16.622, 18.4753, 20.2777,
     24.322, 26.018, 29.877,
     CHI_BIG},

    /* freedom = 8 */
    {0.0, 1.344419, 1.646482, 2.032, 2.17973,
     2.73264, 3.48954, 4.594, 5.07064, 5.527,
     7.34412, 9.524, 10.2188, 11.030, 13.3616,
     15.5073, 17.5346, 18.168, 20.0902, 21.9550,
     26.125, 27.868, 31.828,
     CHI_BIG},

    /* freedom = 9 */
    {0.0, 1.734926, 2.087912, 2.532, 2.70039,
     3.32511, 4.16816, 5.380, 5.89883, 6.393,
     8.34283, 10.656, 11.3887, 12.242, 14.6837,
     16.9190, 19.0228, 19.679, 21.6660, 23.5893,
     27.887, 29.666, 33.270,
     CHI_BIG},

    /* freedom = 10 */
    {0.0, 2.15585, 2.55821, 3.059, 3.24697,
     3.94030, 4.86518, 6.179, 6.73720, 7.267,
     9.34182, 11.781, 12.5489, 13.442, 15.9871,
     18.3070, 20.4831, 21.161, 23.2093, 25.1882,
     29.588, 31.420, 35.564,
     CHI_BIG},

    /* freedom = 11 */
    {0.0, 2.60321, 3.0547, 3.609, 3.81575,
     4.57481, 5.57779, 6.989, 7.58412, 8.148,
     10.3410, 12.899, 13.7007, 14.631, 17.2750,
     19.6751, 21.9200, 22.618, 24.7250, 26.7569,
     31.264, 33.137, 37.367,
     CHI_BIG},

    /* freedom = 12 */
    {0.0, 3.07382, 3.57056, 4.178, 4.40379,
     5.22603, 6.30380, 7.804, 8.43842, 9.034,
     11.3403, 14.011, 14.8454, 15.812, 18.5494,
     21.0261, 23.3367, 24.054, 26.2170, 28.2995,
     32.909, 34.821, 39.134,
     CHI_BIG},

    /* freedom = 13 */
    {0.0, 3.56503, 4.10691, 4.765, 5.00874,
     5.89186, 7.04150, 8.634, 9.29906, 9.926,
     12.3398, 15.119, 15.9839, 16.985, 19.8119,
     22.3621, 24.7356, 25.472, 27.6883, 29.8194,
     34.528, 36.478, 40.871,
     CHI_BIG},

    /* freedom = 14 */
    {0.0, 4.07468, 4.66043, 5.368, 5.62872,
     6.57063, 7.78953, 9.467, 10.1653, 10.821,
     13.3393, 16.222, 17.1170, 18.151, 21.0642,
     23.6848, 26.1190, 26.873, 29.1413, 31.3193,
     36.123, 38.109, 42.579,
     CHI_BIG},

    /* freedom = 15 */
    {0.0, 4.60094, 5.22935, 5.985, 6.26214,
     7.26094, 8.54675, 10.307, 11.0365, 11.721,
     14.3389, 17.322, 18.2451, 19.311, 22.3072,
     24.9958, 27.4884, 28.259, 30.5779, 32.8013,
     37.697, 39.719, 44.263,
     CHI_BIG},

    /* freedom = 16 */
    {0.0, 5.14224, 5.81221, 6.614, 6.90766,
     7.96164, 9.31223, 11.152, 11.9122, 12.624,
     15.3385, 18.418, 19.3688, 20.465, 23.5418,
     26.2962, 28.8454, 29.633, 31.9999, 34.2672,
     39.252, 41.308, 45.925,
     CHI_BIG},

    /* freedom = 17 */
    {0.0, 5.69724, 6.407761, 7.255, 7.56418,
     8.67176, 10.0852, 12.002, 12.7919, 13.531,
     16.3381, 19.511, 20.4887, 21.615, 24.7690,
     27.5871, 30.1910, 30.995, 33.4087, 35.7185,
     40.790, 42.879, 47.566,
     CHI_BIG},

    /* freedom = 18 */
    {0.0, 6.26481, 7.01491, 7.906, 8.23075,
     9.39046, 10.8649, 12.857, 13.6753, 14.440,
     17.3379, 20.601, 21.6049, 22.760, 25.9894,
     28.8693, 31.5264, 32.346, 34.8053, 37.1564,
     42.312, 44.434, 49.189,
     CHI_BIG},

    /* freedom = 19 */
    {0.0, 6.84398, 7.63273, 8.567, 8.90655,
     10.1170, 11.6509, 13.716, 14.5620, 15.352,
     18.3376, 21.689, 22.7178, 23.900, 27.2036,
     30.1435, 32.8523, 33.687, 36.1908, 38.5822,
     43.820, 45.973, 50.796,
     CHI_BIG},

    /* freedom = 20 */
    {0.0, 7.43386, 8.26040, 9.237, 9.59083,
     10.8508, 12.4426, 14.578, 15.4518, 16.266,
     19.3374, 22.775, 23.8277, 25.038, 28.4120,
     31.4104, 34.1696, 35.020, 37.5662, 39.9968,
     45.315, 47.498, 52.386,
     CHI_BIG},

    /* freedom = 21 */
    {0.0, 8.03366, 8.89720, 9.915, 10.2893,
     11.5913, 13.2396, 15.445, 16.3444, 17.182,
     20.3372, 23.858, 24.9348, 26.171, 29.6151,
     32.6705, 35.4789, 36.343, 38.9321, 41.4010,
     46.797, 49.011, 53.962,
     CHI_BIG},

    /* freedom = 22 */
    {0.0, 8.64272, 9.54249, 10.600, 10.9823,
     12.3380, 14.0415, 16.314, 17.2396, 18.101,
     21.3370, 24.939, 26.0393, 27.301, 30.8133,
     33.9244, 36.7807, 37.659, 40.2894, 42.7956,
     CHI_BIG},

    /* freedom = 23 */
    {0.0, 9.26042, 10.19567, 11.293, 11.6885,
     13.0905, 14.8479, 17.187, 18.1373, 19.021,
     22.3369, 26.081, 27.1413, 28.429, 32.0069,
     35.1725, 38.0757, 38.968, 41.6384, 44.1813,
     49.728, 52.000, 57.075,
     CHI_BIG},

    /* freedom = 24 */
    {0.0, 9.88623, 10.8564, 11.992, 12.4011,
     13.8484, 15.6587, 18.062, 19.0372, 19.943,
     23.3367, 27.096, 28.2412, 29.553, 33.1963,
     36.4151, 39.3641, 40.270, 42.9798, 45.5585,
     51.179, 53.479, 58.613,
     CHI_BIG},

    /* freedom = 25 */
    {0.0, 10.5197, 11.5240, 12.697, 13.1197,
     14.6114, 16.4734, 18.940, 19.9393, 20.867,
     24.3366, 28.172, 29.3389, 30.675, 34.3816,
     37.6525, 40.6465, 41.566, 44.3141, 46.9278,
     52.620, 54.947, 60.140,
     CHI_BIG},

    /* freedom = 26 */
    {0.0, 11.1603, 12.1981, 13.409, 13.8439,
     15.3791, 17.2919, 19.820, 20.8434, 21.792,
     25.3364, 29.246, 30.4345, 31.795, 35.5631,
     38.8852, 41.9232, 42.856, 45.6417, 48.2899,
     54.052, 56.407, 61.657,
     CHI_BIG},

    /* freedom = 27 */
    {0.0, 11.8076, 12.8786, 14.125, 14.5733,
     16.1513, 18.1138, 20.703, 21.7494, 22.719,
     26.3363, 30.319, 31.5284, 32.912, 36.7412,
     40.1133, 43.1944, 44.140, 46.9630, 49.6449,
     55.476, 57.858, 63.164,
     CHI_BIG},

    /* freedom = 28 */
    {0.0, 12.4613, 13.5648, 14.847, 15.3079,
     16.9279, 18.9392, 21.588, 22.6572, 23.647,
     27.3363, 31.391, 32.6205, 34.027, 37.9159,
     41.3372, 44.4607, 45.419, 48.2782, 50.9933,
     56.892, 59.300, 64.662,
     CHI_BIG},

    /* freedom = 29 */
    {0.0, 13.1221, 14.2565, 15.574, 16.0417,
     17.7083, 19.7677, 22.475, 23.5666, 24.577,
     28.3362, 32.461, 33.7109, 35.139, 39.0875,
     42.5569, 45.7222, 46.693, 49.5879, 52.3356,
     58.302, 60.735, 66.152,
     CHI_BIG},

    /* freedom = 30 */
    {0.0, 13.7867, 14.9535, 16.306, 16.7908,
     18.4926, 20.5992, 23.364, 24.4776, 25.508,
     29.3360, 33.530, 34.7998, 36.250, 40.2560,
     43.7729, 46.9792, 47.962, 50.8922, 53.6720,
     59.703, 62.162, 67.633,
     CHI_BIG}
};

/*
 * chiprob10 - probability that chi will be < chi_tbl10[f][x] is chiprob10[x]
 */
static double chiprob10[CHI_PROB10] = {
    1.000, 0.995, 0.990, 0.975,
    0.950, 0.900, 0.750,
    0.500, 0.250, 0.100,
    0.050, 0.025, 0.010, 0.005,
    0.001, 0.0005, 0.0001,
    0.000
};

/*
 * chi_tbl10 - multiple of 10 freedom table from CHI_TBL to CHI_LARGE
 */
static double chi_tbl10[1 + (CHI_LARGE - CHI_TBL) / 10][CHI_PROB10] = {
    /* freedom = 30 */
    {0.0, 13.7867, 14.9535, 16.7908,
     18.4926, 20.5992, 24.4776,
     29.3360, 34.7998, 40.2560,
     43.7729, 46.9792, 50.8922, 53.6720,
     59.703, 62.162, 67.633,
     CHI_BIG10},

    /* freedom = 40 */
    {0.0, 20.7065, 22.1643, 24.4331,
     26.5093, 29.0505, 33.6603,
     39.3354, 45.6160, 51.8050,
     55.3417, 59.3417, 63.6907, 66.7659,
     73.402, 76.095, 82.062,
     CHI_BIG10},

    /* freedom = 50 */
    {0.0, 27.9907, 29.7067, 32.3574,
     34.7642, 37.6886, 42.9421,
     49.3349, 56.3336, 63.1671,
     67.5048, 71.4202, 76.1539, 79.4900,
     86.661, 89.560, 95.969,
     CHI_BIG10},

    /* freedom = 60 */
    {0.0, 35.5346, 37.4848, 40.4817,
     43.1879, 46.4589, 52.2938,
     59.3347, 66.9814, 74.3970,
     79.0819, 83.2976, 88.3794, 91.9517,
     99.607, 102.695, 109.503,
     CHI_BIG10},

    /* freedom = 70 */
    {0.0, 43.2752, 45.4418, 48.7576,
     51.7393, 55.3290, 61.6983,
     69.3344, 77.5766, 85.5271,
     90.5312, 95.0231, 100.425, 104.425,
     112.317, 115.578, 122.755,
     CHI_BIG10},

    /* freedom = 80 */
    {0.0, 51.1720, 53.5400, 57.1532,
     60.3915, 64.2778, 71.1445,
     79.3343, 88.1303, 96.5782,
     101.879, 106.629, 112.329, 116.321,
     124.839, 128.261, 135.783,
     CHI_BIG10},

    /* freedom = 90 */
    {0.0, 59.1963, 61.7541, 65.6466,
     69.1260, 73.2912, 80.6247,
     89.3342, 98.6499, 107.565,
     113.145, 118.136, 124.116, 128.299,
     137.208, 140.782, 148.627,
     CHI_BIG10},

    /* freedom = 100 */
    {0.0, 67.3276, 70.0648, 74.2219,
     77.9295, 82.3581, 90.1332,
     99.3341, 109.141, 118.498,
     124.342, 129.561, 135.807, 140.169,
     149.449, 153.167, 161.319,
     CHI_BIG10}
};

/*
 * chiprobx - probability table for matching on chi_x
 */
static double chiprobx[CHI_PROBX] = {
    1.000,
    0.9999, 0.9995, 0.9990,
    0.995, 0.990, 0.980, 0.975,
    0.950, 0.900, 0.800, 0.750, 0.700,
    0.500,
    0.300, 0.250, 0.200, 0.100, 0.050,
    0.025, 0.020, 0.010, 0.005,
    0.0010, 0.0005, 0.0001,
    0.000
};

/*
 * chi_x - chi square x(p) values for approximating large freedom chi values
 *
 * Calculated by:
 *
 *      chi_x[x] = ((chi_tbl10[FREEDOM_100]/100)^(1/3) - (1 - 2/(9*100))) /
 *                 sqrt(2/(9*100))
 *
 * where FREEDOM_100 is the array index for degree 100 from chi_tbl10.
 *
 * It should be noted that the chi_x values for a probability of P
 * is the same as -chi_x for a probability of 1-P.  The chi_x values
 * were further processed using this fact by doing an 'average' of
 * the 'chi_x' values for P and 1-P.  (i.e., the 0.25 and the negative
 * of the 0.75 were averaged to yield the 0.25 value; the 0.75 was
 * taken to be the negative of the new 0.25 value)
 *
 * The values for the 0.9999, 0.9995, 0.9990 probabilities were
 * estimated to be the negative of the 0.0001, 0.0005, 0.0010
 * probability values using the fact noted above.
 *
 * The values for 0.02, 0.20 and 0.30 were taken from the 12th
 * edition CRC reference found below.  (later editions such as the
 * 25th edition omit some of these values)  The 0.98, 0.80 and
 * 0.70 were calculated using the fact noted above.
 *
 * See "Handbook of Mathematical Functions", M. Abromowitz &
 * I. A. Stegun; National Bureau of Standards, U.S. Government Printing
 * Office, Washington D.C., 9th printing, Nov 1970;
 * Table 26.8 (pages 984-985) and 26.4.14 (page 941).
 *
 * Also see "Standard Math Tables", Chemical Rubber Publishing Company,
 * 12th edition (1959), page 252.
 */
static double chi_x[CHI_PROBX] = {
    -CHI_BIGX,
    -3.71304477, -3.28670491, -3.08722514,
    -2.57402256, -2.05068851, -2.32528040 - 1.95963039,
    -1.64492435, -1.28186787, -0.84293762, -0.67479839, -0.52547090,
    0.0,
    0.52547090, 0.67479839, 0.84293762, 1.28186787, 1.64492435,
    1.95963039, 2.05068851, 2.32528040, 2.57402256,
    3.08722514, 3.28670491, 3.71304477,
    CHI_BIGX
};
