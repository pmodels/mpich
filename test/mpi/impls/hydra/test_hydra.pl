#!/usr/bin/perl
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

use strict;

our $debug = 0;
our $dir = "config";
our @test_configs;
our @test_list;


foreach my $a (@ARGV) {
    if ($a eq "-v") {
        $debug = 1;
    } elsif ($a eq "-h" or $a=~/--?help/) {
        print "Usage: $0 [-v] [-h]\n";
        print "\n";
        print "Run tests configured in $dir/proc_binding.txt and\n";
        print "$dir/slurm_nodelist.txt.\n";
        print "\nOptions:\n";
        print "  -v: turn on verbose mode.\n";
        print "  -h: print this message.\n";
        print "\n";
        exit(0);
    }
}

push @test_configs, "$dir/proc_binding.txt";

foreach my $config_txt (@test_configs) {
    my $tests = load_tests($config_txt);
    push @test_list, @$tests;
}

my $num_errors = 0;
foreach my $test (@test_list) {
    if ($test->{type} eq "binding") {
        $num_errors += run_binding_test($test);
    }
    elsif ($test->{type} eq "slurm") {
        $num_errors += run_slurm_test($test);
    }
}
if (!$num_errors) {
    print "No Errors.\n";
}

# ---- subroutines --------------------------------------------
sub load_tests {
    my ($f) = @_;
    my @tests;
    my $test;
    my $topo_file;
    open In, "$f" or die "Can't open $f: $!\n";
    while(<In>){
        if (/TOPO:\s*(\S+\.xml)/i) {
            $topo_file = $1;
        }
        elsif (/^binding:\s+(.*) -n (\d+)/) {
            $test = {type=>"binding", option=>$1, np=>$2, output=>[], topo=>$topo_file};
            push @tests, $test;
        }
        elsif (/^slurm:\s+(\d+)\s+(\S+)/) {
            $test = {type=>"slurm", nnodes=>$1, nodelist=>$2, output=>[]};
            push @tests, $test;
        }
        elsif (/^\s+(\S.+)/) {
            push @{$test->{output}}, $1;
        }
    }
    close In;
    return \@tests;
}

sub run_binding_test {
    my ($test) = @_;
    my $np = $test->{np};
    my $cmd = "mpiexec $test->{option} -n $np ./dummy";
    $ENV{HYDRA_TOPO_DEBUG} = 1;
    $ENV{HWLOC_XMLFILE} = "$dir/$test->{topo}";

    my @output;
    my @errors;
    open In, "$cmd |" or die "Can't open $cmd |: $!\n";
    while(<In>){
        if (/process (\d+) binding: (\d+)/) {
            $output[$1] = $2;
        }
    }
    close In;

    return check_output($cmd, \@output, $test->{output});
}

sub check_output {
    my ($cmd, $output, $exp_output) = @_;
    my @errors;
    for (my $i = 0; $i<@$exp_output; $i++) {
        if ($output->[$i] ne $exp_output->[$i]) {
            push @errors, "output->[$i]: expect $exp_output->[$i], got $output->[$i]";
        }
    }
    my $got_error = 0;
    if (@errors) {
        print "$cmd\n";
        foreach my $err (@errors) {
            print "    $err\n";
        }
        $got_error++;
    }
    elsif ($debug) {
        print "$cmd\n";
        foreach (@$output) {
            print "    $_\n";
        }
    }
    return $got_error;
}

