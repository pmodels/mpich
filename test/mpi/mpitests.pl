##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

my $mpiexec = "mpiexec";
my $test_count = 0;

system "rm -f mpitests.run";
system "rm -f mpitests.lastrun";
system "touch mpitests.lastrun";

for my $np (2) {
  re_run:
    my $last_count = $test_count;
    open LOG, "> mpitests.run";
    open MPIOUT, "$mpiexec $np run_mpitests 2>&1 |" ||
        die "Could not launch $mpiexec $np run_mpitests!\n";

    read_MPIOUT();
    my $rc = close(MPIOUT);

    if ($test_count == $last_count) {
        die "run_mpitests didn't make progress\n";
    }

    if ($rc == 0) {
        # something wrong
        if ($!) {
            die "Pipe Failure! OS_ERROR: $!\n";
        }
        # rerun run_mpitests, it should continue from the next test
        goto re_run;
    }
}

sub read_MPIOUT {
    my ($test_name, $test_time, @test_output);
    while (<MPIOUT>) {
    }
}
