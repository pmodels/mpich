#!/usr/bin/perl
use strict;

our $subdir = ".";
our $verbose = "VERBOSE";
our %opts;


parse_args();
if($opts{subdir}){
    $subdir = $opts{subdir};
}

if($opts{verbose} eq "terse" or $opts{terse}){
    $verbose = "TERSE";
}
my @files;
open In, "find $subdir -name '*.[ch]' |" or die "Can't open find $subdir -name '*.[ch]' |.\n";
while(<In>){
    chomp;
    push @files, $_;
}
close In;
my $count = 0;
foreach my $f (@files){
    my @lines;
    my @temp;
    my ($got_function, $stage);
    open In, "$f" or die "Can't open $f.\n";
    while(<In>){
        if(/^[^#\s{]/){
            undef $got_function;
        }

        if(/^[^#\s].*\s+(\w+)\s*\(.*\)\s*$/){
            $got_function = $1;
        }
        elsif(/^[^#\s].*\s+(\w+)\s*\(.*$/){
            $got_function = $1;
        }

        if(/^{/ && $got_function){
            my $l = ["{\n"];
            my $ID = uc($got_function);
            push @$l, "    MPIR_FUNC_VERBOSE_STATE_DECL($ID);\n";
            push @$l, "    MPIR_FUNC_VERBOSE_ENTER($ID);\n";
            push @lines, $l;
            $count++;
        }
        elsif(/^(\s+)return/ && $got_function){
            my $ID = uc($got_function);
            my $l = [$1."MPIR_FUNC_VERBOSE_EXIT($ID);\n"];
            push @$l, $_;
            push @lines, $l;
        }
        else{
            push @lines, $_;
        }
    }
    close In;

    open Out, ">$f" or die "Can't write $f.\n";
    foreach my $l (@lines){
        if(ref($l) eq "ARRAY"){
            foreach my $ll (@$l){
                print Out $ll;
            }
        }
        else{
            print Out $l;
        }
    }
    close Out;
}

print "Added tracing logs to $count functions.\n";

# ---- subroutines --------------------------------------------
sub parse_args {
    my @un_recognized;
    foreach my $a (@ARGV){
        if($a=~/-(\w+)=(.*)/){
            $opts{$1} = $2;
        }
        elsif($a=~/-(\w+)/){
            $opts{$1} = 1;
        }
        else{
            push @un_recognized, $a;
        }
    }
    if(@un_recognized){
        usage();
        die "Unrecognized options: @un_recognized\n";
    }
}

sub usage {
    print "Usage: $0 [-subdir=folder] [-terse]\n";
}

