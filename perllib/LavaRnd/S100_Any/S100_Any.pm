package LavaRnd::S100_Any;

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

use 5.008;
use strict;
use warnings;
use POSIX qw(strtod);

require Exporter;
use AutoLoader qw(AUTOLOAD);

our @ISA = qw(Exporter);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use LavaRnd::S100_Any ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our %EXPORT_TAGS = ( 'all' => [ qw(
    random8 random16 random32 random_data
    random_val random_coin
    range_drandom range_dcrandom
    drandom dcrandom
    range_random range_crandom
    numeric_value is_numeric integer_value is_integer
    lavarnd_errno lastop_errno
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(

);

our $VERSION = '0.21';

require XSLoader;
XSLoader::load('LavaRnd::S100_Any', $VERSION);

# Preloaded methods go here.

# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__

=head1 NAME

LavaRnd::S100_Any - local s100 psuedo-random generator, seeded from LavaRnd

=head1 SYNOPSIS

  use LavaRnd::S100_Any qw(
    random8 random16 random32 random_data
    random_val random_coin
    range_drandom range_dcrandom
    drandom dcrandom
    range_random range_crandom
    numeric_value is_numeric integer_value is_integer
    lavarnd_errno
  );

  # ... or if you want everything:

  use LavaRnd::S100_Any ':all';

  ####
  # Integer random values
  ####

  # return a 8 bit random value
  #
  $integer = random8();

  # return a 16 bit random value
  #
  $integer = random16();

  # return a 32 bit random value
  #
  $integer = random32();

  # return random data, $len octets (bytes) long
  #
  # The $len size must be an integer > 0 or
  # undef is returned.
  #
  $data = random_data($len);

  # return an integer value >= 0 and < $beyond.
  #
  # The $beyond value must fit into a 32 bit integer.
  # Also $beyond >= 0 or 0 is returned.
  #
  $integer = random_val($beyond);

  # return integer value >= $low and < $beyond
  #
  # The $low and $beyond must be 32 bit integers.
  # Also $low < $beyond or $low is returned.
  #
  $integer = range_random($low, $beyond);

  # return integer value value >= $low and <= $high
  #
  # The $low and $high must be 32 bit integers.
  # Also $low <= $high or $low is returned.
  #
  $integer = range_crandom($low, $high);

  # return a true (1) or false (0) value
  #
  $boolean = random_coin();

  ####
  # Floating point random values
  ####

  # return a random floating point value >= 0.0 and < 1.0.
  #
  $float = drandom();

  # return a random floating point value >= 0.0 and <= 1.0.
  #
  $float = dcrandom();

  # return floating point value >= $low and < $beyond
  #
  # Note that: $low < $beyond or $low is returned.
  #
  $float = range_drandom($low, $beyond);

  # return floating point value value >= $low and <= $high
  #
  # Note that: $low <= $high or $low is returned.
  #
  $float = range_dcrandom($low, $high);

  ####
  # Error checking
  #
  # NOTE: This LavaRnd::S100_Any module will exit instead of returning
  #	  bad data, so this call is less useful than it is in
  #	  other LavaRnd modules.  The lavarnd_errno() and
  #	  lastop_errno() interfaces are provided to make this
  #	  module's interface similar to the other LavaRnd modules.
  ####

  # Determine if some previous LavaRnd call encountered an error
  #
  if (lavarnd_errno() < 0) {
    die "some previous LavaRnd call failed";
  }

  # Clear previous error condition
  #
  $old_code = lavarnd_errno(0);

  # Determine if the most recent LavaRnd call encountered an error
  #
  if (lastop_errno() < 0) {
    die "the most recent LavaRnd call failed";
  }

  ####
  # misc utility functions used by the interface
  ####

  # if scalar only is numeric (integer, floating point), then
  # return the numeric value or
  # return undef (scalar contains non-numeric data)
  #
  $number_or_undef = numeric_value($value);

  # TRUE if scalar is numeric (integer, floating point),
  # FALSE otherwise
  #
  $true_if_numeric = is_numeric($value);

  # if scalar only is an integer, then return the integer value or
  # return undef (scalar contains non-integer data)
  #
  $integer_or_undef = integer_value($value);

  # TRUE if scalar is integer (integer, floating point),
  # FALSE otherwise
  #
  $true_if_integer = is_integer($value);

=head1 DESCRIPTION

This module uses the local s100 pseudo-random generator
that has been potentially seeded from LavaRnd's lavapool
daemon.

The s100 pseudo-random generator does not produce
cryptographically strong random numbers.
However, when seeded with data from LavaRnd data
(via the lavapool daemon), this generator
produces very high quality pseudo-random data.

By default, this module attempts to seed the s100 pseudo-random generator
with lavapool data.
The s100 pseudo-random generator is also reseeded with
the lavapool data after it has
been used to produce a lot of data.
In this mode, the s100 pseudo-random generator yields very
high quality psuedo-random, approaching (if not reaching)
cryptographic strength.

If during a reseed operation, any call fails to
communicate with the lavapool daemon, then
the s100 generator will continue to use its
current state.
This module try to reseed from the lavapool daemon
on a periodic basis.
In this mode, the pseudo-random generator yields medium
quality psuedo-random that is generally better than
most standard / language-library provided pseudo-random facilities.

If during the initial seed operation, any call fails to
communicate with the lavapool daemon, then
the s100 generator will use a default initial seed
permuted by information obtained from the caller's
environment.
This module will try to seed from the lavapool daemon
on a periodic basis.
In this mode, the pseudo-random generator yields low
quality psuedo-random.
Even in this low quality mode, this pseudo-random generator
compares favorably with nearly all
standard / language-library provided pseudo-random facilities.

If your application requires cryptographically strong random
data, you should use one of the other LavaRnd modules
such as LavaRnd::Exit, LavaRnd::Retry, LavaRnd::TryOnce_Any,
or LavaRnd::Try_Any.

This module uses the LavaRnd liblava_s100_any.a library.

=head1 AUTHOR

Landon Curt Noll (http://www.isthe.com/chongo/index.html)

=head1 SEE ALSO

http://www.LavaRnd.org/

If you have a mailing list set up for your module, mention it here.

If you have a web site set up for your module, mention it here.

=head1 COPYRIGHT AND LICENSE

Copyright 2003 by Landon Curt Noll

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

# numeric_value - get the numeric value of a scalar
#
# given:
#	$value	scalar whose numeric value will be extracted, if only numeric
#
# returns:
#	numeric value or
#	undef ==> scalar contained some non-numeric data
#
sub numeric_value($)
{
    my ($str) = @_;	# get arg
    my $num;		# numeric part of $str, if any
    my $unparsed;	# non-numeric part of $str, if any

    # firewall
    #
    return undef if ! defined $str;

    # strip leading and trailing whitespace
    #
    $str =~ s/^\s+//;
    $str =~ s/\s+$//;
    return undef if ($str eq '');

    # try to convert to a number
    #
    $! = 0;
    ($num, $unparsed) = strtod($str);

    # return numeric value or undef
    #
    if (($unparsed != 0) || $!) {
        return undef;
    }
    return $num;
}


# is_numeric - determine if a scalar is just a numeric value
#
# given:
#	$value	scalar to test for numerical value
#
# returns:
#	TRUE ==> scalar is numeric
#	FALSE ==> scalar is NOT numeric
#
sub is_numeric($)
{
    my ($str) = @_;	# get arg
    my $num;		# numeric part of $str, if any
    my $unparsed;	# non-numeric part of $str, if any

    # firewall
    #
    return undef if ! defined $str;

    # strip leading and trailing whitespace
    #
    $str =~ s/^\s+//;
    $str =~ s/\s+$//;
    return undef if ($str eq '');

    # try to convert to a number
    #
    $! = 0;
    ($num, $unparsed) = strtod($str);

    # determine of the arg was numeric
    #
    return (($unparsed == 0) && $! == 0);
}


# range_drandom - return a floating value in the open interval [$low, $beyond)
#
# given:
#	$low		lowest possible random value
#	$beyond		return up to but not including this value
#
# returns:
#	[$low, $beyond)
#
# NOTE: If $low >= $beyond, then the $low value is always returned.
#
sub range_drandom($$)
{
    my ($low, $beyond) = @_;	# get args
    my $bottom;			# bottom of range
    my $range;			# size of range to return in

    # firewall - args must be defined
    #
    if (!defined $low || !defined $beyond) {
       return $low;
    }

    # firewall - args must be numeric
    #
    $bottom = numeric_value($low);
    $beyond = numeric_value($beyond);
    if (!defined $bottom) {
       return $low;
    }
    if (!defined $beyond) {
       return $bottom;
    }

    # firewall - bottom of range must be below beyond range
    #
    $range = $beyond - $bottom;
    if ($range <= 0) {
       return $bottom;
    }

    # return value in open interval
    #
    return $bottom + ($range * drandom());
}


# range_dcrandom - return a floating value in the closed interval [$low, $high]
#
# given:
#	$low		lowest possible random value
#	$high		do not return beyond this value
#
# returns:
#	[$low, $high]
#
# NOTE: If $low >= $high, then the $low value is always returned.
#
sub range_dcrandom($$)
{
    my ($low, $high) = @_;	# get args
    my $bottom;			# bottom of range
    my $range;			# size of range to return in

    # firewall - args must be defined
    #
    if (!defined $low || !defined $high) {
       return $low;
    }

    # firewall - args must be numeric
    #
    $bottom = numeric_value($low);
    $high = numeric_value($high);
    if (!defined $bottom) {
       return $low;
    }
    if (!defined $high) {
       return $bottom;
    }

    # firewall - bottom of range must be below beyond range
    #
    $range = $high - $bottom;
    if ($range <= 0) {
       return $bottom;
    }

    # return value in closed interval
    #
    return $bottom + ($range * dcrandom());
}


# integer_value - get the integer value of a scalar
#
# given:
#	$value	scalar whose integer value will be extracted, if only integer
#
# returns:
#	integer value or
#	undef ==> scalar contained some non-integer
#
sub integer_value($)
{
    my ($str) = @_;	# get arg

    # firewall
    #
    return undef if ! defined $str;

    # strip leading and trailing whitespace and a trailing .0's
    #
    $str =~ s/^\s+//;
    $str =~ s/\s+$//;
    $str =~ s/\.0*$//;
    return undef if ($str eq '');

    # return integer value or undef
    #
    if ($str =~ /^[+-]?\d+$/) {
	return $str;
    }
    return undef;
}


# is_integer - determine if a scalar is just an integer value
#
# given:
#	$value	scalar to test for integer value
#
# returns:
#	TRUE ==> scalar is an integer
#	FALSE ==> scalar is NOT an integer
#
sub is_integer($)
{
    my ($str) = @_;	# get arg

    # firewall
    #
    return undef if ! defined $str;

    # strip leading and trailing whitespace and a trailing .0's
    #
    $str =~ s/^\s+//;
    $str =~ s/\s+$//;
    $str =~ s/\.0*$//;

    # determine of the arg an integer
    #
    return (($str ne '') && ($str =~ /^[+-]?\d+$/));
}


# range_random - return an integer value in the open interval [$low, $beyond)
#
# given:
#	$low		lowest possible random value
#	$beyond		return up to but not including this value
#
# returns:
#	[$low, $beyond)
#
# NOTE: If $low >= $beyond, then the $low value is always returned.
#
sub range_random($$)
{
    my ($low, $beyond) = @_;	# get args
    my $bottom;			# bottom of range
    my $range;			# size of range to return in

    # firewall - args must be defined
    #
    if (!defined $low || !defined $beyond) {
       return $low;
    }

    # firewall - args must be numeric
    #
    $bottom = integer_value($low);
    $beyond = integer_value($beyond);
    if (!defined $bottom) {
       return $low;
    }
    if (!defined $beyond) {
       return $bottom;
    }

    # firewall - bottom of range must be below beyond range
    #
    $range = $beyond - $bottom;
    if ($range <= 0) {
       return $bottom;
    }

    # return value in open interval
    #
    return $bottom + random_val($range);
}


# range_crandom - return a integer value in the closed interval [$low, $high]
#
# given:
#	$low		lowest possible random value
#	$high		do not return beyond this value
#
# returns:
#	[$low, $high]
#
# NOTE: If $low >= $high, then the $low value is always returned.
#
sub range_crandom($$)
{
    my ($low, $high) = @_;	# get args
    my $bottom;			# bottom of range
    my $range;			# size of range to return in

    # firewall - args must be defined
    #
    if (!defined $low || !defined $high) {
       return $low;
    }

    # firewall - args must be numeric
    #
    $bottom = integer_value($low);
    $high = integer_value($high);
    if (!defined $bottom) {
       return $low;
    }
    if (!defined $high) {
       return $bottom;
    }

    # firewall - bottom of range must be below beyond range
    #
    $range = $high - $bottom + 1;
    if ($range <= 0) {
       return $bottom;
    }

    # return value in closed interval
    #
    return $bottom + random_val($range);
}
