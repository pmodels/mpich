#!/usr/bin/perl
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

use strict;

our $debug = 0;
our $dir = "config";
our @test_list;

foreach my $a (@ARGV) {
    if ($a eq "-v") {
        $debug = 1;
    } elsif ($a =~ /^-slurm=(.*)/) {
        quick_test_slurm($1);
        exit(0);
    } elsif ($a eq "-h" or $a=~/--?help/) {
        print "Usage: $0 [-v] [-h]\n";
        print "\n";
        print "Run tests configured in $dir/proc_binding.txt and\n";
        print "$dir/slurm_nodelist.txt.\n";
        print "\nOptions:\n";
        print "  -v: turn on verbose mode.\n";
        print "  -h: print this message.\n";
        print "  -slurm=\"np host_pattern\"\n";
        print "      run quick test on slurm host pattern\n";
        print "\n";
        exit(0);
    }
}

if (!@test_list) {
    my @test_configs;
    push @test_configs, "$dir/proc_binding.txt";
    push @test_configs, "$dir/slurm_nodelist.txt";

    foreach my $config_txt (@test_configs) {
        my $tests = load_tests($config_txt);
        push @test_list, @$tests;
    }
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
        elsif ($test and /^\s+(\S.+)/) {
            push @{$test->{output}}, $1;
        } else {
            undef $test;
        }
    }
    close In;
    return \@tests;
}

sub run_binding_test {
    my ($test) = @_;
    my $np = $test->{np};
    my $cmd = "mpiexec $test->{option} -n $np";
    if (-x "dummy") {
        $cmd .= " ./dummy";
    } else {
        $cmd .= " true";
    }
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

sub run_slurm_test {
    my ($test) = @_;
    my $nnodes = $test->{nnodes};
    $ENV{SLURM_NNODES} = $nnodes;
    $ENV{SLURM_TASKS_PER_NODE} = "1(x$nnodes)";
    $ENV{SLURM_NODELIST} = $test->{nodelist};
    my $cmd = "mpiexec -rmk slurm -debug-nodelist true";
    my @output;
    open In, "$cmd |" or die "Can't open $cmd |: $!\n";
    while(<In>){
        if (/(\S.*)/) {
            push @output, $1;
        }
    }
    close In;

    my $testname = "nodelist: $test->{nodelist}";
    return check_output($testname, \@output, $test->{output});
}

sub quick_test_slurm {
    my ($t) = @_;
    if ($t=~/(\d+)\s+(.+)/) {
        $ENV{SLURM_NNODES} = $1;
        $ENV{SLURM_TASKS_PER_NODE} = "1(x$1)";
        $ENV{SLURM_NODELIST} = $2;
        my $cmd = "mpiexec -rmk slurm -debug-nodelist true";
        print "Pattern: $2\n";
        system $cmd;
    } else {
        die "Usage: $0 -slurm=\"np hostlist_pattern\"\n";
    }
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

