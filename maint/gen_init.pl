#!/usr/bin/env perl
#
# (C) 2019 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

# Scan C source files to search for component init functions and
# and aggregate into src/include/autogen.h

use strict;
use warnings;

my $srcdir = ".";

# read in template
my @lines;
my $template_file = "$srcdir/src/include/autogen.h.in";
open In, $template_file or die "Failed to open template $template_file\n";
while (<In>) {
    push @lines, $_;
}
close In;

# collect source file list
my $t = `find $srcdir -name '*.c' |xargs grep -l 'AUTOGEN-'`;
my @all_files = split /\n/, $t;

my %placeholders;
$placeholders{DECLARE} = [];
foreach my $f (@all_files) {
    my %func_hash;
    open In, $f or die "Failed to open source file $f\n";
    while (<In>) {
        if (/^\/\*\s*AUTOGEN-(\w+)/) {
            if(!$placeholders{$1}) {
                $placeholders{$1} = [];
            }
            my $place = $placeholders{$1};

            # prototype next line
            my $t = <In>;
            if ($t =~ /^(void|int)\s+(\w+)\(void\)/) {
                push @{$placeholders{DECLARE}}, "$1 $2(void);";
                if ($1 eq "void") {
                    push @{$place}, "$2();"
                }
                else {
                    push @{$place}, "mpi_errno = $2();";
                    push @{$place}, "MPIR_ERR_CHECK(mpi_errno);";
                }
            }
        }
    }
    close In;
}

# generate autogen.h

my $autogen_file = "$srcdir/src/include/autogen.h";
open Out, ">$autogen_file" or die "Can't write $autogen_file\n";
foreach my $l (@lines) {
    if ($l=~/^\/\*\s* DISCLAIMER/){
        print Out "/* Automatically generated. Do not edit. */\n";
    }
    elsif ($l=~/^(\s*)\/\*\s*PLACEHOLDER-(\w+)/) {
        my ($sp, $name) = ($1, $2);
        my $place = $placeholders{$name};
        if ($place) {
            foreach $t (@$place) {
                print Out "$sp$t\n";
            }
        }
    }
    else {
        print Out $l;
    }
}
close Out;
