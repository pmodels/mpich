##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

use strict;

our $debug = 0;
our $dir = "binding_reference";
our $dummy = "./dummy";

if (!-x $dummy) {
    $dummy = "true";
}

my $test_list = load_tests("$dir/proc_binding.txt");

my $num_errors = 0;
foreach my $test (@$test_list) {
    $num_errors += run_bind_test($test);
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
        elsif (/^mpiexec\s+(.*) -n (\d+)/) {
            $test = {option=>$1, np=>$2, output=>[], topo=>$topo_file};
            push @tests, $test;
        }
        elsif (/^\s+([01]+)/) {
            push @{$test->{output}}, $1;
        }
    }
    close In;
    return \@tests;
}

sub run_bind_test {
    my ($test) = @_;
    my $np = $test->{np};
    my $cmd = "mpiexec $test->{option} -n $np $dummy";
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
    for (my $i = 0; $i<$np; $i++) {
        if ($output[$i] ne $test->{output}->[$i]) {
            push @errors, "process $i expect $test->{output}->[$i], got $output[$i]";
        }
    }

    my $got_error = 0;
    if (@errors) {
        print "$test->{topo}\n";
        print "$cmd\n";
        foreach my $err (@errors) {
            print "    $err\n";
        }
        $got_error++;
    }
    elsif ($debug) {
        print "$test->{topo}\n";
        print "$cmd\n";
        foreach (@output) {
            print "    $_\n";
        }
    }

    return $got_error;
}

