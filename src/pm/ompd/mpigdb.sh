#!/bin/sh
# This is a simple version of mpigdb, which has the restriction that none of the 
# arguments to mpdrun are usable except -n <nprocs> .  In the long run, we expect to 
# replace this with a version of mpiexec that accepts all mpiexec arguments.
if [ "$1" = "-a" ] ; then
    jobid=$2
    shift  # -a
    shift  # jobid
    exec mpdrun -ga $jobid
else if [ "$1" = "-n" ] ; then
    nprocs=$2
    pgm=$3
    shift  # -n
    shift  # nprocs
    shift  # pgm
    exec mpdrun -g -n $nprocs $pgm $*
else
    echo "usage:"
    echo "    mpigdb -a jobid # to attach to a running job"
    echo "    mpigdb -n nprocs pgm [pgmars]   # to use gdb on a new job"
fi
fi
