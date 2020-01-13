#!/usr/bin/perl
use strict;

my ($root, %opts);
foreach my $a (@ARGV) {
    if ($a=~/^-(\w+)=(.*)/) {
        $opts{$1} = $2;
    }
    elsif ($a=~/^-(\w+)$/) {
        $opts{$1} = 1;
    }
    elsif (-d $a) {
        $root = $a;
    }
}

if($root){
    chdir $root or die;
}

my $grep_cmd="find . -name '*.[ch]' |xargs grep -l 'MPIR_FUNC_'";
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
print "Removed function tracing logs from $cnt functions\n";

# ---- subroutines --------------------------------------------
sub patch {
    my ($f) = @_;
    my @lines;
    {
        open In, "$f" or die "Can't open $f.\n";
        @lines=<In>;
        close In;
    }
    my ($cnt, $cnt_enter);

    my $i=0;
    my $n_lines = @lines;
    while($i<$n_lines){
        if($lines[$i]=~/^\s*MPIR_FUNC_(VERBOSE|TERSE)_\w*(STATE_DECL|ENTER|EXIT)(?:_FRONT|_BACK|_BOTH)?\s*(.*)/){
            my ($verbose, $type, $tail)=($1, $2, $3);
            if($tail=~/^\(\w+\);(\s*\\)?/){
                $lines[$i] = "-DELETE";
            }
            elsif($tail=~/^$/ && $lines[$i+1]=~/^\s*\(\w+\);\s*$/){
                $lines[$i] = "-DELETE";
                $lines[$i+1] = "-DELETE";
            }
            else{
                warn "mismatch: $lines[$i]";
            }
            $cnt++;
            if($type eq "ENTER"){
                $cnt_enter++;
            }
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
    return $cnt_enter;
}

