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

use strict;
use Test::More tests => 1621;
BEGIN { use_ok('LavaRnd::Return') };
use LavaRnd::Return ':all';

#########################

# Insert your test code below, the Test::More module is use()ed here so read
# its man page ( perldoc Test::More ) for help writing this test script.

# my values
#
my $trycnt=0;	# tests performed
my $failcnt=0;	# number of test failures
my $val;	# j-random value :-)
my $hold;	# a large buffer created via random_data() held during test
my $hold_len;	# expected length of the hold buffer
my @array;	# individual octets of a value
my @hold_1st_array;	# individual octets of $hold early in the test
my @hold_last_array;	# individual octets of $hold late in the test
my $test_cnt;	# number of times to call random_data() in the test middle
my $len;	# mid-test random_data() size
my $beyond;	# die sides
my $verbose=0;	# NOTE: SET THIS VALUE TO NON-ZERO TO SEE DEBUG OUTPUT
my $i;

# pre-check errno
#
ok( lavarnd_errno() == 0 );
ok( lastop_errno() == 0 );

# hold on to a random buffer
#
$hold_len = 1024;
$hold = random_data($hold_len);
ok( defined $hold && length($hold) == $hold_len );
if (defined $hold && length($hold) == $hold_len) {
    if ($verbose) {
	@array = split(//, substr($hold, 0, 9));
	print STDERR "hold's first 9 octets: ";
	foreach my $j (@array) {
	    print STDERR ord($j), " ",;
	}
	print STDERR "\n";
	@array = split(//, substr($hold, -9, 9));
	print STDERR "hold's final 9 octets: ";
	foreach my $j (@array) {
	    print STDERR ord($j), " ",;
	}
	print STDERR "\n";
    }
    @hold_1st_array = split(//, $hold);
}
ok( lastop_errno() == 0 );

# 8 bit random value
#
$test_cnt = 16;
for ($i = 0; $i < $test_cnt; ++$i) {
    $val = random8();
    ok( lastop_errno() == 0 );
    ok( defined($val) && $val >= 0 && $val <= 0xff );
    print STDERR "8 bit random value: $val\n" if $verbose;
}

# 16 bit random value
#
$val = random16();
ok( defined($val) && $val >= 0 && $val <= 0xffff );
ok( lastop_errno() == 0 );
print STDERR "16 bit random value: $val\n" if $verbose;

# 32 bit random value
#
$val = random32();
ok( defined($val) && $val >= 0 && $val <= 0xffffffff );
ok( lastop_errno() == 0 );
print STDERR "32 bit random value: $val\n" if $verbose;

# try to get a buffer of random octets several times
#
$test_cnt = 100;
for ($i=0; $i < $test_cnt; ++$i) {
    $len = 17;
    $val = random_data($len);
    ok( lastop_errno() == 0 );
    ok(defined $val && length($val) == $len);
    if (!defined $val || length($val) != $len) {
	print STDERR "random_data error\n" if $verbose;
	last;
    }
}
ok ($i == $test_cnt);
if ($i >= $test_cnt) {
    print STDERR "obtained $test_cnt buffers of $len octets\n" if $verbose;
}

# roll a die and return a value from [0,$beyond)  (32 bit max)
#
$test_cnt = 32;
$beyond = 6;
for ($i = 0; $i < $test_cnt; ++$i) {
    $val = random_val($beyond);
    ok( lastop_errno() == 0 );
    ok( defined($val) && $val >= 0 && $val < $beyond );
    print STDERR "$beyond sided die: ", $val+1, "\n" if $verbose;
}

# integer random range, half open
#
for ($i = -50; $i < 50; ++$i) {
    $beyond = 5 + $i;
    $val = range_random($i, $beyond);
    ok( lastop_errno() == 0 );
    ok( defined($val) && $val >= $i && $val < $beyond );
    print STDERR "integer half open [$i, $beyond): $val\n" if $verbose;
}

# integer random range, open
#
for ($i = -50; $i < 50; ++$i) {
    $beyond = 5 + $i;
    $val = range_crandom($i, $beyond);
    ok( lastop_errno() == 0 );
    ok( defined($val) && $val >= $i && $val <= $beyond );
    print STDERR "integer half open [$i, $beyond): $val\n" if $verbose;
}

# floating point in the open interval [0.0, 1.0)
#
$test_cnt = 16;
for ($i = 0; $i < $test_cnt; ++$i) {
    $val = drandom();
    ok( lastop_errno() == 0 );
    ok( defined($val) && $val >= 0.0 && $val < 1.0 );
    print STDERR "floating point [0.0, 1.0) random value: $val\n" if $verbose;
}

# floating point in the closed interval [0.0, 1.0]
#
$test_cnt = 16;
for ($i = 0; $i < $test_cnt; ++$i) {
    $val = dcrandom();
    ok( lastop_errno() == 0 );
    ok( defined($val) && $val >= 0.0 && $val <= 1.0 );
    print STDERR "floating point [0.0, 1.0] random value: $val\n" if $verbose;
}

# floating point random range, half open
#
for ($i = -50.5; $i < 50.5; $i += 1.1) {
    $beyond = 5 + $i;
    $val = range_drandom($i, $beyond);
    ok( lastop_errno() == 0 );
    ok( defined($val) && $val >= $i && $val < $beyond );
    print STDERR "floating point half open [$i, $beyond): $val\n" if $verbose;
}
for ($i = -50.5; $i < 50.5; $i += 1.1) {
    $beyond = 4.5 + $i;
    $val = range_drandom($i, $beyond);
    ok( lastop_errno() == 0 );
    ok( defined($val) && $val >= $i && $val < $beyond );
    print STDERR "floating point half open [$i, $beyond): $val\n" if $verbose;
}

# floating point random range, open
#
for ($i = -50.5; $i < 50.5; $i += 1.1) {
    $beyond = 5 + $i;
    $val = range_dcrandom($i, $beyond);
    ok( lastop_errno() == 0 );
    ok( defined($val) && $val >= $i && $val <= $beyond );
    print STDERR "floating point half open [$i, $beyond): $val\n" if $verbose;
}
for ($i = -50.5; $i < 50.5; $i += 1.1) {
    $beyond = 4.5 + $i;
    $val = range_dcrandom($i, $beyond);
    ok( lastop_errno() == 0 );
    ok( defined($val) && $val >= $i && $val <= $beyond );
    print STDERR "floating point half open [$i, $beyond): $val\n" if $verbose;
}

# truth or falseness
#
$val = random_coin();
ok( lastop_errno() == 0 );
ok( defined $val );
if (defined $val) {
    if ($val) {
	print STDERR "random coin: heads\n" if $verbose;
    } else {
	print STDERR "random coin: tails\n" if $verbose;
    }
}

# numeric testing
#
print STDERR "numeric testing\n" if $verbose;
#
ok( numeric_value(23) == 23 );
ok( numeric_value(+23) == 23 );
ok( numeric_value(23.) == 23 );
ok( numeric_value(23.0) == 23 );
ok( numeric_value(-23.0) == -23 );
ok( numeric_value(-23.01) == -23.01 );
ok( numeric_value(+23.01) == 23.01 );
ok( numeric_value("23") == 23 );
ok( numeric_value("+23") == 23 );
ok( numeric_value("23.") == 23 );
ok( numeric_value("23.0") == 23 );
ok( numeric_value("-23.0") == -23 );
ok( numeric_value("-23.01") == -23.01 );
ok( numeric_value("+23.01") == 23.01 );
ok( numeric_value(" 23\t") == 23 );
ok( numeric_value(" +23\t") == 23 );
ok( numeric_value(" 23.\t") == 23 );
ok( numeric_value(" 23.0\t") == 23 );
ok( numeric_value(" -23.0\t") == -23 );
ok( numeric_value(" -23.01\t") == -23.01 );
ok( numeric_value(" +23.01\t") == 23.01 );
ok( numeric_value(__LINE__) == __LINE__ );
ok( ! defined numeric_value("abc") );
ok( ! defined numeric_value("14 black birds") );
ok( ! defined numeric_value("0x123h") );
#
ok( is_numeric(23) );
ok( is_numeric(+23) );
ok( is_numeric(23.) );
ok( is_numeric(23.0) );
ok( is_numeric(-23.0) );
ok( is_numeric(-23.01) );
ok( is_numeric(+23.01) );
ok( is_numeric("23") );
ok( is_numeric("+23") );
ok( is_numeric("23.") );
ok( is_numeric("23.0") );
ok( is_numeric("-23.0") );
ok( is_numeric("-23.01") );
ok( is_numeric("+23.01") );
ok( is_numeric(" 23\t") );
ok( is_numeric(" +23\t") );
ok( is_numeric(" 23.\t") );
ok( is_numeric(" 23.0\t") );
ok( is_numeric(" -23.0\t") );
ok( is_numeric(" -23.01\t") );
ok( is_numeric(" +23.01\t") );
ok( is_numeric(__LINE__) );
ok( !is_numeric("abc") );
ok( !is_numeric("14 black birds") );
ok( !is_numeric("0x123h") );
#
ok( is_integer(23) );
ok( is_integer(+23) );
ok( is_integer(23.) );
ok( is_integer(23.0) );
ok( is_integer(-23.0) );
ok( !is_integer(-23.01) );
ok( !is_integer(+23.01) );
ok( is_integer("23") );
ok( is_integer("+23") );
ok( is_integer("23.") );
ok( is_integer("23.0") );
ok( is_integer("-23.0") );
ok( !is_integer("-23.01") );
ok( !is_integer("+23.01") );
ok( is_integer(" 23\t") );
ok( is_integer(" +23\t") );
ok( is_integer(" 23.\t") );
ok( is_integer(" 23.0\t") );
ok( is_integer(" -23.0\t") );
ok( !is_integer(" -23.01\t") );
ok( !is_integer(" +23.01\t") );
ok( is_integer(__LINE__) );
ok( !is_integer("abc") );
ok( !is_integer("14 black birds") );
ok( !is_integer("0x123h") );
#
ok( integer_value(23) == 23 );
ok( integer_value(+23) == 23 );
ok( integer_value(23.) == 23 );
ok( integer_value(23.0) == 23 );
ok( integer_value(-23.0) == -23 );
ok( ! defined integer_value(-23.01) );
ok( ! defined integer_value(+23.01) );
ok( integer_value("23") == 23 );
ok( integer_value("+23") == 23 );
ok( integer_value("23.") == 23 );
ok( integer_value("23.0") == 23 );
ok( integer_value("-23.0") == -23 );
ok( ! defined integer_value("-23.01") );
ok( ! defined integer_value("+23.01") );
ok( integer_value(" 23\t") == 23 );
ok( integer_value(" +23\t") == 23 );
ok( integer_value(" 23.\t") == 23 );
ok( integer_value(" 23.0\t") == 23 );
ok( integer_value(" -23.0\t") == -23 );
ok( ! defined integer_value(" -23.01\t") );
ok( ! defined integer_value(" +23.01\t") );
ok( integer_value(__LINE__) == __LINE__ );
ok( ! defined integer_value("abc") );
ok( ! defined integer_value("14 black birds") );
ok( ! defined integer_value("0x123h") );

# look at the previously held data and see if it changed on us
#
ok( defined $hold && length($hold) == $hold_len );
if (defined $hold && length($hold) == $hold_len) {
    if ($verbose) {
	@array = split(//, substr($hold, 0, 9));
	print STDERR "hold now has 9 octets: ";
	foreach my $j (@array) {
	    print STDERR ord($j), " ",;
	}
	print STDERR "\n";
	@array = split(//, substr($hold, -9, 9));
	print STDERR "hold last of 9 octets: ";
	foreach my $j (@array) {
	    print STDERR ord($j), " ",;
	}
	print STDERR "\n";
    }
    @hold_last_array = split(//, $hold);
    ok( scalar(@hold_1st_array) == scalar(@hold_last_array) );
    if (scalar(@hold_1st_array) != scalar(@hold_last_array)) {
        print STDERR "amount of hold random_data changed from ",
	      scalar(@hold_1st_array), " to ",
	      scalar(@hold_last_array), "\n" if $verbose;
    }
    for ($i=0; $i < $#hold_1st_array; ++$i) {
        last if $hold_1st_array[$i] != $hold_last_array[$i];
    }
    ok( $i == $#hold_1st_array );
    if ($i != $#hold_1st_array) {
        print STDERR "octet $i of hold data: ", $hold_1st_array[$i],
	      " != ", $hold_last_array[$i], "\n" if $verbose;
    }
}

# look at lavarnd_errno() and lastop_errno()
#
ok( lastop_errno() == 0 );
ok( lavarnd_errno() == 0 );
$i = lavarnd_errno();
ok( $i == 0 );
$i = lavarnd_errno(127);
ok( $i == 0 );
ok( lavarnd_errno() == 127 );
$i = lavarnd_errno(0);
ok( $i == 127 );
ok( lavarnd_errno() == 0 );
ok( lavarnd_errno(-1) == 0 );
ok( lavarnd_errno(0) == -1 );
ok( lastop_errno() == 0 );
