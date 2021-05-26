#! /bin/bash
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# Generate datatype testing build instructions for every line in
# configuration file dtp-test-config.txt

types=""     # basic MPI datatypes for type signature
counts=""    # counts for type signature
pathname=""  # test file pathname
testdir=""   # name of test directory
testname=""  # name of test
builddir=""  # directory in which executable will be located
dirs=""      # all directories containing DTPools tests
args=""      # extra args
timelimit="" # time limit for testlist.in
procs=""     # number of processes to use in testlist.in
srcdir=""    # relative directory in which this script is located
other_args=""

# help message
show_help() {
cat << EOF
Usage: $0 [options]
options:
  --help    show this message and exit
  --with-dtpools-datatypes=[typelist]
            comma separated list of MPI Datatypes to use for
            generating DTPools tests
EOF
}

while [ true ] ; do
    if test -z "$1" ; then break ; fi
    case $(echo $1 | cut -d= -f1) in
        --help)
            show_help
            exit 1
            ;;
        --with-dtpools-datatypes)
            types=$(echo $1 | cut -d= -f2)
            shift
            ;;
        *)
            echo Unrecognized option [$1]
            exit 1
            ;;
    esac
done

# get source and build directories
srcdir=`dirname $0`
builddir=$PWD

# reformat type list
types=$(echo $types | tr "," " ")

# NOTE: generate seeds for tests starting from '1'
#       as srand() considers 'seed=0' and 'seed=1'
#       equivalent: http://sourceware.org/git/?p=glibc.git;a=blob;f=stdlib/random_r.c;h=51a2e8c812aee78783bd6d38c1b6269d41c8e47e;hb=HEAD#l181
seed=1

while read -r line ; do
    if [ ! `echo $line | cut -c 1` = "#" ] ; then
        # the line is not a comment
        pathname=`echo $line | cut -f1 -d':'`
        args=`echo $line | cut -f2 -d':'`
        counts=`echo $line | cut -f3 -d':'`
        timelimit=`echo $line | cut -f4 -d':'`
        procs=`echo $line | cut -f5 -d':'`
        mintestsize=`echo $line | cut -f6 -d':'`
        maxtestsize=`echo $line | cut -f7 -d':'`
        gacc=`echo $line | cut -f8 -d':'`
        other_args=""
    else
        # the line is a comment
        continue
    fi

    if [ -z "$pathname" ] ; then
        echo "No pathname found"
        exit 1
    elif [ -z "$counts" ] ; then
        echo "$pathname has no counts specified"
        exit 1
    elif [ -z "$procs" ] ; then
        echo "$pathname has no proc count specified"
        exit 1
    elif [ -z "$mintestsize" ] ; then
        echo "$pathname has no minimum test size specified"
        exit 1
    fi

    # set default maxtestsize
    if [ -z $maxtestsize ] ; then
        maxtestsize=$mintestsize
    fi

    # get test directory and name
    testdir=`dirname $pathname`
    testname=`basename $pathname`

    # if this is the first test in this directory, create a new
    # testlist.dtp
    found=0
    for dir in $dirs ; do
        if test "$testdir" = "$dir" ; then
            found=1
        fi
    done
    if test "$found" = "0" ; then
        dirs="$dirs $testdir"
        printf "" > ${builddir}/${testdir}/testlist.dtp
    fi

    # prepare extra args
    for arg in $args ; do
        other_args="$other_args arg=$arg"
    done

    printf "generating tests for: ${testname}... "

    sendcounts=$counts

    for type in $types ; do
        testsize=0
        count=0
        for sendcount in $sendcounts ; do
            # decrease the testsize by the amount the count went up
            # by, but make sure it doesn't fall below 8
            if test "$testsize" = "0" ; then
                testsize=$maxtestsize
                count=$sendcount
            else
                testsize=$((testsize * count / sendcount))
                if [ $testsize -lt $mintestsize ] ; then testsize=$mintestsize; fi
                if [ $testsize -gt $maxtestsize ] ; then testsize=$maxtestsize; fi
            fi

            if [ $testdir = "pt2pt" ] || [ $testdir = "part" ]; then # only send/recv or partitioned comm can use types from different pools
                # do combination of different send recv count where recv count >= send count
                # limit the mixed pool case to only one
                # TODO: this should be defined in the config file
                for recvcount in $sendcount $((sendcount * 2)) ; do
                    echo "${testname} $procs arg=-type=${type} arg=-sendcnt=${sendcount} arg=-recvcnt=${recvcount} arg=-seed=$seed arg=-testsize=${testsize} ${other_args} $timelimit" >> ${builddir}/${testdir}/testlist.dtp
                    seed=$((seed + 1))
                done
            elif [ $testdir = "rma" ] ; then
                if [ $gacc = "1" ] ; then # specify resultmem for MPI_GET_ACCUMULATE tests
                    echo "${testname} $procs arg=-type=${type} arg=-count=${sendcount} arg=-seed=$seed arg=-testsize=${testsize} ${other_args} $timelimit" >> ${builddir}/${testdir}/testlist.dtp
                    seed=$((seed + 1))
                else
                    echo "${testname} $procs arg=-type=${type} arg=-count=${sendcount} arg=-seed=$seed arg=-testsize=${testsize} ${other_args} $timelimit" >> ${builddir}/${testdir}/testlist.dtp
                    seed=$((seed + 1))
                fi
            elif [ $testdir = "coll" ] ; then
                echo "${testname} $procs arg=-type=${type} arg=-count=${sendcount} arg=-seed=$seed arg=-testsize=${testsize} ${other_args} $timelimit" >> ${builddir}/${testdir}/testlist.dtp
                seed=$((seed + 1))
            else
                echo "${testname} $procs arg=-type=${type} arg=-count=${sendcount} arg=-seed=$seed arg=-testsize=${testsize} ${other_args} $timelimit" >> ${builddir}/${testdir}/testlist.dtp
                seed=$((seed + 1))
            fi
        done
    done
    printf "done\n"
done < ${srcdir}/dtp-test-config.txt
