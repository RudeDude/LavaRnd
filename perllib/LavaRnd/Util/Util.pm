package LavaRnd::Util;

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

# This allows declaration	use LavaRnd::Util ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our %EXPORT_TAGS = ( 'all' => [ qw(
    lavarnd lavarnd_len lava_nway_value
    lava_turn lava_blk_turn lava_salt_blk_turn
    system_stuff
    lavarnd_errno lastop_errno
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(

);

our $VERSION = '0.21';

require XSLoader;
XSLoader::load('LavaRnd::Util', $VERSION);

# Preloaded methods go here.

# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__

=head1 NAME

LavaRnd::Util - miscellaneous low level LavaRnd utility functions


=head1 SYNOPSIS

  use LavaRnd::Util qw(lavarnd lavarnd_len lavarnd_errno lastop_errno);

  # lavarnd - Perform the lavarnd process in a scalar
  #
  # given:
  #	input	  input value (should be from a chaotic source)
  #	rate	  increase (>1.0) or decrease (<1.0) output amount
  #			if not given, then 1.0 is assumed
  #	use_salt  1 ==> use system_stuff for salt, 0 ==> no salting
  #			if not given, then 1 is assumed
  #
  # returns:
  #	$rnddata	LavaRnd octets if defined, or undef if error
  #
  $rnddata = lavarnd($input [, $rate [, $use_salt]]);

  # lava_turn - perform a nway turn on a value
  # lava_blk_turn - nway a value with ways turn SHA-1 digest aligned
  #
  # given:
  #	salt	salt value
  #	input	input value
  #	nway	turn step value
  #
  # returns:
  #	$turned		nway turned input value or undef if error
  #
  $turned = lava_turn($input, $nway);
  $turned = lava_blk_turn($input, $nway);
  $turned = lava_salt_blk_turn($salt, $input, $nway);

  # lavarnd_len - expected lavarnd() return size
  #
  # given:
  #	$inlen	length of the input buffer (not counting any salt)
  #	$rate	increase (>1.0) or decrease (<1.0) output amount
  #
  # returns:
  #	$len	length of output that would returned by lavarnd()
  #
  # Rate values that are >= 0.0625 and <= 16.0 produce perfectly
  # reasonable values.  If in doubt, use a rate of 1.0.
  #
  $len = lavarnd_len($inlen, $rate);

  # lava_nway_value - determine a nway value given length and rate
  #
  # given:
  #	$inlen	length of the input buffer (not counting any salt)
  #	$rate	increase (>1.0) or decrease (<1.0) output amount
  #
  # return:
  #	$nway	nway value recommened for the length and rate
  #
  # Rate values that are >= 0.0625 and <= 16.0 produce perfectly
  # reasonable values.  If in doubt, use a rate of 1.0.
  #
  $nway = lava_nway_value($inlen, $rate);

  # system_stuff - collect system state related stuff
  #
  # return:
  #	A bunch of stuff based on system and process information or undef
  #
  # This data is suitable to use as a salt, however it is NOT a
  # good source of chaotic data!
  #
  $stuff = system_stuff();

  ####
  # Error checking
  #
  # NOTE: This LavaRnd::Util module will exit instead of returning
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

=head1 ABSTRACT

  The LavaRnd::Util module contains miscellaneous low level LavaRnd
  utility functions.  These functions are not normally needed by users,
  but are provided for those who which to experiment with some of the
  fundimental LavaRnd algorithms.

=head1 DESCRIPTION

This module provides access to miscellaneous low level LavaRnd
functions.
These functions are presented for demonstration and internal
testing purposes only.
You should look at other LavaRnd modules for random number facilities.

=head2 lava_turn

The lava_turn() function performs an nway turn on a value.
If the input value is not a multiple of nway octets, then
the turn operation will return as if the input value
was NUL octet padded to an multiple of nway octets.
The nway turn is one of the core operations in the LavaRnd algorithm.

=head2 lava_blk_turn

The lava_blk_turn() function also performs an nway turn on a value,
but each nway value is SHA-1 digest aligned for efficency in
performing the SHA-1 digest within the LavaRnd algorithm.
NUL octets may be added within the output value so that
each nway value is SHA-1 digest aligned.
A SHA-1 digest is 20 octets.

=head2 lava_salt_blk_turn

The lava_salt_blk_turn() function performs an nway turn on a salt
and a string.
The salt is nway turned first.
Like lava_turn(),
if the salt is not a multiple of nway then the turned result
will be NUL padded.
Then the input value is nway turned in the same way.
Last, NUL octets may be added within the output value so that
each nway value is SHA-1 digest aligned.
A SHA-1 digest is 20 octets.

=head2 lavarnd

The lavarnd() function represents the fundimental LavaRnd algorithm
that can turn a digitized chaotic source into cryptographically
strong random numbers.
lavarnd_errno() will remain unchanged.

=head2 lavarnd_len

The lavarnd_len() function can be used to predict the length
of the lavarnd() function return, when defined.

=head2 lava_nway_value

The lava_nway_value() may be used to determine the nway value
used for a given length and rate factor.

=head2 system_stuff

The system_stuff() function simply returns some system state
stuff that is suitable as a salt.

B<PLEASE NOTE: The system_stuff() function does not return
random data!!!
The system_stuff() function is not a source very chaotic data!!!!>.
There exist some software implementations that mix (via
cryprographic hashes and/or block ciphers) system state
in what they claim to be an entropy pool to produce
what they claim to be random data.
Just because these software implementations are foolish enough
to use such poor chatoic sources does B<NOT MEAN YOU SHOULD!>
The data returned by system_stuff() function is suitable
to be used as a salt, and that is about all.

=head2 misc notes

Many of these functions return undef on error.
Both lavarnd_errno() and lastop_errno() will return the
any error code produced by called function.
If the function call was successful, then
lastop_errno() will return 0 and

In lavarnd(), lavarnd_len(), and lava_nway_value() one supplies a rate.
A rate of >1.0 will increase the amount of data from the
normal rate.
A rate of <1.0 will increase the amount of data produced.
The rate value should be >= 0.0625 and <= 16.0.
If in doubt, use a rate of 1.0.

The LavaRnd algorithm xor, fold and rotates an nway turned
digitization of a chaotic source.
There if I<$input> is the digitization of a chaotic source, then
I<$data> represents what needs to be ''I<xor fold rotated>
to produce cryptographically strong random numbers:

    $data = lava_salt_blk_turn(system_stuff(), $input,
			       lava_nway_value(length($input), $rate));

=head2 EXPORT

None by default.

=head1 SEE ALSO

http://www.LavaRnd.org/

=head1 AUTHOR

Landon Curt Noll (http://www.isthe.com/chongo/index.html)

=head1 COPYRIGHT AND LICENSE

Copyright 2003 by Landon Curt Noll

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut
