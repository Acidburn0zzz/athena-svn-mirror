#!/usr/bin/perl -w

package Math::BigInt::Subclass;

require 5.005_02;
use strict;

use Exporter;
use Math::BigInt(1.56);
use vars qw($VERSION @ISA $PACKAGE @EXPORT_OK
            $accuracy $precision $round_mode $div_scale);

@ISA = qw(Exporter Math::BigInt);
@EXPORT_OK = qw(bgcd objectify);

$VERSION = 0.03;

use overload;	# inherit overload from BigInt

# Globals
$accuracy = $precision = undef;
$round_mode = 'even';
$div_scale = 40;

sub new
{
        my $proto  = shift;
        my $class  = ref($proto) || $proto;

        my $value       = shift;
	my $a = $accuracy; $a = $_[0] if defined $_[0];
	my $p = $precision; $p = $_[1] if defined $_[1];
        my $self = Math::BigInt->new($value,$a,$p,$round_mode);
	bless $self,$class;
        $self->{'_custom'} = 1; # make sure this never goes away
        return $self;
}

sub bgcd
  {
  Math::BigInt::bgcd(@_);
  }

sub blcm
  {
  Math::BigInt::blcm(@_);
  }

BEGIN
  {
  *objectify = \&Math::BigInt::objectify;

  # these are called by AUTOLOAD from BigFloat, so we need at least these.
  # We cheat, of course..
  *bneg = \&Math::BigInt::bneg;
  *babs = \&Math::BigInt::babs;
  *bnan = \&Math::BigInt::bnan;
  *binf = \&Math::BigInt::binf;
  *bzero = \&Math::BigInt::bzero;
  *bone = \&Math::BigInt::bone;
  }

sub import
  {
  my $self = shift;

  my @a; my $t = 0;
  foreach (@_)
    {
    $t = 0, next if $t == 1;
    if ($_ eq 'lib')
      {
      $t = 1; next;
      }
    push @a,$_;
    }
  $self->SUPER::import(@a);			# need it for subclasses
  $self->export_to_level(1,$self,@a);		# need this ?
  }

1;
