# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl 1.t'

# Copyright (c) 2000-2003 by Landon Curt Noll and Simon Cooper.
# All Rights Reserved.
#
# This is open software; you can redistribute it and/or modify it under
# the terms of the version 2.1 of the GNU Lesser General Public License
# as published by the Free Software Foundation.
#
# This software is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General
# Public License for more details.
#
# The file COPYING contains important info about Licenses and Copyrights.
# Please read the COPYING file for details about this open software.
#
# A copy of version 2.1 of the GNU Lesser General Public License is
# distributed with calc under the filename COPYING-LGPL.  You should have
# received a copy with calc; if not, write to Free Software Foundation, Inc.
# 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
#
# For more information on LavaRnd: http://www.LavaRnd.org
#
# Share and enjoy! :-)

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

use Test::More tests => 61;
BEGIN { use_ok('LavaRnd::Util') };
use LavaRnd::Util ':all';

#########################

# Insert your test code below, the Test::More module is use()ed here so read
# its man page ( perldoc Test::More ) for help writing this test script.

my $ret;	# return value from a LavaRnd::Util function
my $salt;	# pre-turned salt argument
my $input;	# pre-turned input argument
my $turned;	# turned input value
my $output;	# binary output
my @dataset;	# array of 32 bit word used to pack into a scalar value
my $data;	# dataset packed as scalar value
my $verbose=0;	# NOTE: SET THIS VALUE TO NON-ZERO TO SEE DEBUG OUTPUT

# pre-check errno
#
ok( lavarnd_errno() == 0 );
ok( lastop_errno() == 0 );

# lavarnd_len testing
#
ok( lavarnd_len(19200, 4.0) == 700 );
ok( lavarnd_len(19200, 3.0) == 620 );
ok( lavarnd_len(19200, 2.0) == 500 );
ok( lavarnd_len(19200, 1.5) == 460 );
ok( lavarnd_len(19200, 1.0) == 340 );
ok( lavarnd_len(19200, 0.5) == 260 );
ok( lavarnd_len(19200, 0.25) == 220 );
#
ok( lavarnd_len(350000, 4.0) == 2980 );
ok( lavarnd_len(350000, 3.0) == 2620 );
ok( lavarnd_len(350000, 2.0) == 2140 );
ok( lavarnd_len(350000, 1.5) == 1820 );
ok( lavarnd_len(350000, 1.0) == 1460 );
ok( lavarnd_len(350000, 0.5) == 1060 );
ok( lavarnd_len(350000, 0.25) == 740 );

# lava_nway_value testing
#
ok( lava_nway_value(350000, 4.0) == 149 );
ok( lava_nway_value(350000, 3.0) == 131 );
ok( lava_nway_value(350000, 2.0) == 107 );
ok( lava_nway_value(350000, 1.5) == 91 );
ok( lava_nway_value(350000, 1.0) == 73 );
ok( lava_nway_value(350000, 0.5) == 53 );
ok( lava_nway_value(350000, 0.25) == 37 );


# test lava_turn
#
$input = "abcdefghijklmnopqrstuvwxyz";
$turned = "aeimquy" . "bfjnrvz" . "cgkosw\0" . "dhlptx\0";
$ret = lava_turn($input, 4);
print STDERR "expected turned: (($turned))\n" if $verbose;
print STDERR "returned turned: (($ret))\n" if $verbose;
ok( lastop_errno() == 0 );
ok( defined($ret) );
ok( $ret eq $turned );
$input = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMN" .
	 "OPQRSTUVWXYZ0123456789abcdefghijklmnopqr" .
	 "stuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012345" .
	 "67890abcdefghijklmnopqrstuvwxyzABCDEFGHI" .
	 "JKLMNOPQRSTUVWXYZ0123456789abcdefghijklm";
$turned = "afkpuzEJOTY38dinsxCHMRW16afkpuzEJOTY38di" .
	  "bglqvAFKPUZ49ejotyDINSX27bglqvAFKPUZ49ej" .
	  "chmrwBGLQV05afkpuzEJOTY38chmrwBGLQV05afk" .
	  "dinsxCHMRW16bglqvAFKPUZ49dinsxCHMRW16bgl" .
	  "ejotyDINSX27chmrwBGLQV050ejotyDINSX27chm";
$ret = lava_turn($input, 5);
print STDERR "expected turned: (($turned))\n" if $verbose;
print STDERR "returned turned: (($ret))\n" if $verbose;
ok( lastop_errno() == 0 );
ok( defined($ret) );
ok( $ret eq $turned );

# test lava_blk_turn
#
$input = "abcdefghijklmnopqrstuvwxyz";
$turned = "aeimquy" . "\0"x13 .
	  "bfjnrvz" . "\0"x13 .
	  "cgkosw\0" . "\0"x13 .
	  "dhlptx\0" . "\0"x13;
$ret = lava_blk_turn($input, 4);
print STDERR "expected turned: (($turned))\n" if $verbose;
print STDERR "returned turned: (($ret))\n" if $verbose;
ok( lastop_errno() == 0 );
ok( defined($ret) );
ok( $ret eq $turned );
$input = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMN" .
	 "OPQRSTUVWXYZ0123456789abcdefghijklmnopqr" .
	 "stuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012345" .
	 "67890abcdefghijklmnopqrstuvwxyzABCDEFGHI" .
	 "JKLMNOPQRSTUVWXYZ0123456789abcdefghijklm";
$turned = "afkpuzEJOTY38dinsxCHMRW16afkpuzEJOTY38di" .
	  "bglqvAFKPUZ49ejotyDINSX27bglqvAFKPUZ49ej" .
	  "chmrwBGLQV05afkpuzEJOTY38chmrwBGLQV05afk" .
	  "dinsxCHMRW16bglqvAFKPUZ49dinsxCHMRW16bgl" .
	  "ejotyDINSX27chmrwBGLQV050ejotyDINSX27chm";
$ret = lava_blk_turn($input, 5);
print STDERR "expected turned: (($turned))\n" if $verbose;
print STDERR "returned turned: (($ret))\n" if $verbose;
ok( lastop_errno() == 0 );
ok( defined($ret) );
ok( $ret eq $turned );

# test lava_salt_blk_turn
#
$salt = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
$input = "abcdefghijklmnopqrstuvwxyz";
$turned = "AEIMQUYaeimquy" . "\0"x6 .
	  "BFJNRVZbfjnrvz" . "\0"x6 .
	  "CGKOSW\0cgkosw\0" . "\0"x6 .
	  "DHLPTX\0dhlptx\0" . "\0"x6;
$ret = lava_salt_blk_turn($salt, $input, 4);
print STDERR "expected turned: (($turned))\n" if $verbose;
print STDERR "returned turned: (($ret))\n" if $verbose;
ok( lastop_errno() == 0 );
ok( defined($ret) );
ok( $ret eq $turned );

# test system_stuff
#
$ret = system_stuff();
ok( lastop_errno() == 0 );
ok( defined($ret) );
ok( length($ret) > 0 );
ok( system_stuff() ne $ret );

# test lavarnd()
#
$input = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMN" .
	 "OPQRSTUVWXYZ0123456789abcdefghijklmnopqr" .
	 "stuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012345" .
	 "67890abcdefghijklmnopqrstuvwxyzABCDEFGHI" .
	 "JKLMNOPQRSTUVWXYZ0123456789abcdefghijklm";
$ret = lavarnd($input, 2.0, 0);
ok( lastop_errno() == 0 );
ok( defined($ret) );
ok( length($ret) > 0 );
print STDERR "lavarnd return: ", join(', ', unpack("I*", $ret)), "\n"
    if $verbose;
@dataset = (
    1076401684, 197533180, 3718027889, 1975299194, 3703973107, 1551630766,
    1717725347, 2414162518, 3576815072, 2932257586, 3501165063, 233293518,
    3768925882, 2304104756, 4055971731, 2903735188, 3640200381, 1238032090,
    2887794397, 2501626040, 1082398978, 2699976426, 867988793, 3371907591,
    2810238180
);
$data = pack("I*", @dataset);
print STDERR "test data: ", join(', ', unpack("I*", $data)), "\n" if $verbose;
ok( $ret eq $data );
#
$ret = lavarnd($input, 1.0, 0);
ok( lastop_errno() == 0 );
ok( defined($ret) );
ok( length($ret) > 0 );
print STDERR "lavarnd return: ", join(', ', unpack("I*", $ret)), "\n"
    if $verbose;
@dataset = (
    2662758592, 50781024, 2740041913, 1519848053, 2745740331
);
$data = pack("I*", @dataset);
print STDERR "test data: ", join(', ', unpack("I*", $data)), "\n" if $verbose;
ok( $ret eq $data );
#
$ret = lavarnd($input, 1.0, 1);
ok( lastop_errno() == 0 );
ok( defined($ret) );
ok( length($ret) > 0 );
print STDERR "lavarnd return: ", join(', ', unpack("I*", $ret)), "\n"
    if $verbose;
ok( $ret ne $data );
#
@dataset = (
    2092237683, 2664988773, 1179892291, 520437219,
    3362625537, 2343274412, 3887616368, 1816046888,
    3117678670, 3788808964, 2580021566, 637359808,
    1403466047, 3092473604, 1468112498, 588472695,
    2498698566, 3528844600, 621628210, 3944552444,
    473438215, 1819730812, 2672478402, 4129867282,
    4186986144, 2777143954, 1787503112, 998758830,
    2019474334, 42753833, 358060967, 2135514472,
    336251171, 15324038, 3142023996, 2363289206,
    2869867265, 103233821, 3179028168, 621632306,
    3216376934, 573392574, 3206181467, 700729866,
    4034225011, 2875409197, 2761701908, 3930530260,
    2085088764, 148222951, 3257739498, 3569702001,
    1400700466, 1859025986, 2489797198, 3916625503,
    4096772840, 2781176577, 1242417228, 4093556799,
    3759748915, 1993788026, 1496477257, 2997236408,
    2659971733, 3449348037, 857188720, 395386141,
    3477065998, 2951018099, 3987460104, 766399205,
    156173934, 4202862274, 3640925171, 1499081503,
    1639181921, 503166500, 1674792068, 4122549761,
    2392724535, 3258813043, 3134433901, 2005760030,
    3513540149, 3036720284, 1075314585, 95007552,
    3419908046, 3927123209, 95767594, 1154603340,
    114385985, 3485116595, 3560625934, 3518222073,
    3931129714, 626959949, 2933298583, 969654665,
    3646505542, 3283751964, 1233581575, 1886238445,
    1093380650, 466275066, 3446342970, 668119299,
    3963167409, 3308473277, 269371710, 554662714,
    2599460661, 533929202, 819726159, 3741251800,
    4077364135, 3127581562, 4283808566, 2143344713,
    2328492196, 701245291, 2942722596, 3880272924,
    469735806, 447956999, 734237752, 3346167506,
    3457498910, 128041821, 3200402475, 1090377759,
    134622798, 3151242068, 411802527, 2446108452,
    3005118453, 1742218070, 3470736013, 2897105897,
    768971524, 1725158420, 859361820, 586359139,
    1073477698, 1305179111, 840967751, 2559255723,
    2506594092, 3275430987, 2780462346, 2256310761,
    4130803826, 876402710, 583205978, 252863464,
    483944643, 3782614874, 4015766913, 4197607068,
    441052296, 3663389575, 917087198, 1751382886,
    2089473769, 214452129, 1153542056, 1087624825,
    595737249, 267656078, 2648152637, 3318657691,
    2694377020, 3901929761, 1780016655, 1645201807,
    478439818, 4177180815, 444956904, 1401133788,
    204688987, 3083722542, 88351334, 1078777893,
    225540176, 3880911001, 2058388793, 2149230201,
    3494820415, 1140245001, 2681473534, 1648837906,
    2254278176, 4048053164, 2769615827, 4052713888,
    1950201249, 4221788213, 3445758877, 4220306511,
    3618003382, 667957346, 1495139719, 1344843621,
    1664738789, 2092590111, 3136900178, 1717976070,
    2504955188, 887932553, 3971384595, 2345736307,
    3418210839, 2185160830, 2848089464, 29349735,
    3026356421, 747982803, 4250405615, 2543327444,
    2906351727, 2243215785, 3487256764, 941794804,
    970921985, 4215321595, 776000230, 4149974826,
    3483096204, 2223060826, 2201061731, 3786051682,
    3250415764, 3774051897, 4092337112, 908614366,
    1755069811, 3364790938, 372054231, 2995345843,
    2817800998, 4255205994, 557179681, 3154959916,
    2289213214, 2601239588, 2447711908, 3533648329,
    1034082533, 4086249541, 1063022073, 2856956546,
    1654049429, 2322352270, 2981442368, 1555568318,
);
$input = pack("I*", @dataset);
$ret = lavarnd($input, 1.0, 0);
ok( lastop_errno() == 0 );
ok( defined($ret) );
ok( length($ret) > 0 );
@dataset = (
    258743470, 1412101959, 1091886953, 3923158312, 1601418798, 2113361316,
    459080111, 1579987703, 314668105, 461626009, 1201701723, 3211053925,
    3161401487, 864298198, 2493951277, 4038019512, 680003877, 3360440518,
    92083590, 540020092, 404227106, 2903665866, 3510558771, 2448405597,
    960599752
);
print STDERR "lavarnd return: ", join(', ', unpack("I*", $ret)), "\n"
    if $verbose;
$data = pack("I*", @dataset);
print STDERR "test data: ", join(', ', unpack("I*", $data)), "\n" if $verbose;
ok( $ret eq $data );

# post-check errno
#
ok( lavarnd_errno() == 0 );
ok( lastop_errno() == 0 );
