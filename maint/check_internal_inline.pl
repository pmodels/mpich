#!/usr/bin/perl
use strict;

our %opts;
our %functions;

foreach my $a (@ARGV) {
    if ($a=~/^-(\w+)$/) {
        $opts{$1}=1;
    }
    elsif ($a=~/^-(\w+)=(.*)/) {
        $opts{$1}=$2;
    }
}
my @files;
foreach my $dir (qw(mpi mpi_t nameserv util include mpid pmi)) {
    open In, "find src/$dir -name '*.[ch]' |" or die "Can't open find src/$dir -name '*.[ch]' |: $!\n";
    while(<In>){
        chomp;
        push @files, $_;
    }
    close In;
}

foreach my $f (@files) {
    open In, "$f" or die "Can't open $f: $!\n";
    while(<In>){
        if (/^MPL_STATIC_INLINE_PREFIX\s.*\s(\w+)\(/) {
            if ($functions{$1}) {
                $functions{$1}->{external_count}++;
            }
            else {
                $functions{$1} = {file=>$f};
            }
        }
        elsif (/^INTERNAL_STATIC_INLINE\s.*\s(\w+)\(/) {
            $functions{$1} = {file=>$f, internal=>1};
        }
    }
    close In;
}
foreach my $f (@files) {
    open In, "$f" or die "Can't open $f: $!\n";
    while(<In>){
        if (/(\w+)\(/ && $functions{$1}) {
            if ($f eq $functions{$1}->{file}) {
                $functions{$1}->{internal_count}++;
            }
            else {
                $functions{$1}->{external_count}++;
            }
        }
    }
    close In;
}
my %files_need_correct;
foreach my $name (sort keys %functions) {
    my $count = $functions{$name}->{external_count};
    my $file = $functions{$name}->{file};
    if ($functions{$name}->{internal}) {
        if ($count>0) {
            print "INTERNAL $name\texternally used\n";
            $functions{$name}->{need_correct}=1;
            $files_need_correct{$file}++;
        }
    }
    else {
        if ($count==0) {
            my $internal_count = $functions{$name}->{internal_count};
            if ($name=~/^MPIDI_STUBSHM_/ or $internal_count==1) {
                next;
            }
            print "EXTERNAL $name\tonly used internally ($internal_count)\n";
            $functions{$name}->{need_correct}=1;
            $files_need_correct{$file}++;
        }
    }
}
if ($opts{correct}) {
    foreach my $f (sort keys %files_need_correct) {
        my @lines;
        open In, "$f" or die "Can't open $f: $!\n";
        while(<In>){
            if (/^MPL_STATIC_INLINE_PREFIX\s.*\s(\w+)\(/) {
                if ($functions{$1}->{need_correct}) {
                    s/MPL_STATIC_INLINE_PREFIX/INTERNAL_STATIC_INLINE/;
                }
            }
            elsif (/^INTERNAL_STATIC_INLINE\s.*\s(\w+)\(/) {
                if ($functions{$1}->{need_correct}) {
                    s/INTERNAL_STATIC_INLINE/MPL_STATIC_INLINE_PREFIX/;
                }
            }
            push @lines, $_;
        }
        close In;
        open Out, ">$f" or die "Can't write $f: $!\n";
        print "  --> [$f]\n";
        foreach my $l (@lines) {
            print Out $l;
        }
        close Out;
    }
}
