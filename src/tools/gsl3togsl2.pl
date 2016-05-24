#!/usr/bin/perl -w
#
# Convert GSL3 scripts back to GSL2.
#
# Written by Ewen McNeill <ewen@imatix.com>, 2001/01/02
#
# This performs basic conversions from GSL3 to GSL2, so that scripts 
# written in a "GSL2 compatible" dialect of GSL3 (eg, ones that have 
# been converted from GSL2 to GSL3 at some point) can be translated back
# to a form that will be understood by a GSL2 interpreter.  It is
# primarily intended for back converting scripts maintained as GSL3 to
# clients who cannot (or will not) update to GSL3.
#
# Usage: perl gsl3togsl2.pl <GSL3SCRIPT.gsl >GSL2SCRIPT.gsl
#    or: perl gsl3togsl2.pl *.gsl
#
# In the latter form it will convert the files to ".gsl2" extensions.
#
# Conversions performed:
#
# gsl from "       ->  include "
#
# file.exists (    ->  exists (
#
# string.justify ( ->  justify (
# string.length (  ->  length (
# string.substr (  ->  substr (
# string.trim (    ->  trim (
#
# NOTE: Certain GSL3 constructs (eg, returning a value from a function)
# are not compatible with GSL2, but are not converted.  These constructs
# could be flagged later if required.
#
#---------------------------------------------------------------------------

require 5;
use strict;

sub convert_gsl3_to_gsl2
{
  $_[0] =~ s/gsl from "/include "/g;
  $_[0] =~ s/file.exists( *\()/exists$1/g;
  $_[0] =~ s/string.justify( *\()/justify$1/g;
  $_[0] =~ s/string.length( *\()/length$1/g;
  $_[0] =~ s/string.substr( *\()/substr$1/g;
  $_[0] =~ s/string.trim( *\()/trim$1/g;
}

if (defined($ARGV[0]))
{
  # Convert a list of files on command line from *.gsl to *.gsl2
  if ($ARGV[0] =~ /\*/)
  {
    my @restarray = @ARGV;
    shift @restarray;
    @ARGV = (<$ARGV[0]>, @restarray);         # Try to expand wildcards
  }

  my $filename;
  for $filename (@ARGV)
  {
    warn "Processing: ${filename} -> ${filename}2\n";
    open (INPUT, "<${filename}")   || die "Cannot read \"$filename\": $!\n";
    open (OUTPUT, ">${filename}2") || die "Cannot write \"${filename}2\": $!\n";
    
    while (<INPUT>)
    {
      convert_gsl3_to_gsl2($_);
      print OUTPUT;
    }

    close (OUTPUT);
    close (INPUT);
  }
}
else
{
  # Convert a single file from standard input to standard output
  while (<>)
  {
    convert_gsl3_to_gsl2($_);
    print;
  }
}
