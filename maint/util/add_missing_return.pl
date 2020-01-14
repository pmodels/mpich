#!/usr/bin/perl
use strict;

my $grep_cmd="find . -name '*.[ch]' |xargs grep -l 'fn_exit:'";
my @files;
open In, "$grep_cmd |" or die "Can't open $grep_cmd |: $!\n";
while(<In>){
    chomp;
    push @files, $_;
}
close In;

our $cnt=0;
foreach my $f (@files){
    if($f){
        $cnt += patch($f);
    }
}
print "Added missing return for $cnt functions\n";

# ---- subroutines --------------------------------------------
sub patch {
    my ($f) = @_;
    my @lines;
    {
        open In, "$f" or die "Can't open $f.\n";
        @lines=<In>;
        close In;
    }
    my $cnt;

    my $i=0;
    my $n_lines = @lines;
    while($i<$n_lines){
        if($lines[$i]=~/^\s+fn_exit:\s*$/ && $lines[$i+1]=~/^}/){
            $lines[$i] = [$lines[$i], "    return;\n"];
            $cnt++;
        }
        $i++;
    }
    if($cnt>0){
        open Out, ">$f" or die "Can't write $f: $!\n";
        foreach my $l (@lines){
            if(ref($l) eq "ARRAY"){
                foreach my $ll (@$l){
                    print Out $ll;
                }
            }
            elsif($l ne "-DELETE"){
                print Out $l;
            }
        }
        close Out;
    }
    return $cnt;
}

