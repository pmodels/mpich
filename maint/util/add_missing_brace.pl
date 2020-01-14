#!/usr/bin/perl
use strict;

my $grep_cmd="find . -name '*.[ch]'";
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
        if($lines[$i-1]=~/\\$/){
            # NOOP
        }
        elsif($lines[$i]=~/^\s+return\b/ && $lines[$i-1]=~/^\s*(if|else|\} else)\b/ && $lines[$i-1]!~/{$/){
            chomp $lines[$i-1];
            $lines[$i-1] .= " {\n";
            if ($lines[$i+1]=~/^(\s*)(else.*)/) {
                $lines[$i+1]="$1\} $2\n"
            }
            elsif ($lines[$i-1]=~/^(\s*)/) {
                my $sp = $1;
                if ($lines[$i+1]=~/^$/ and $lines[$i+2]=~/^#/) {
                    $lines[$i+1] = "$sp}\n";
                }
                else {
                    $lines[$i] = [$lines[$i], "$sp}\n"];
                }
            }
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

