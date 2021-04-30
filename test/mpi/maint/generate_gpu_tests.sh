#! /bin/bash
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# Generate testlist.gpu for cases not covered under gentests-dtop.sh
# and test_coll_algos.sh
# Reads input from the input file in format "testpath/testname:procs"
# and creates test combinations for (host, device) and (device, device).

srcdir=`dirname $0`
builddir=$PWD

while read -r line ; do
    if [ ! `echo $line | cut -c 1` = "#" ] ; then
        # the line is not a comment
        pathname=`echo $line | cut -f1 -d':'`
        procs=`echo $line | cut -f2 -d':'`
        count=`echo $line | cut -f3 -d':'`
        other_args=""
    else
        # the line is a comment
        continue
    fi

    if [ -z "$pathname" ] ; then
        echo "No pathname found"
        exit 1
    elif [ -z "$procs" ] ; then
        echo "$pathname has no proc count specified"
        exit 1
    fi

    # get test directory and name
    testdir=`dirname $pathname`
    testname=`basename $pathname`

    # if this is the first test in this directory, create a new
    # testlist.gpu
    found=0
    for dir in $dirs ; do
        if test "$testdir" = "$dir" ; then
            found=1
        fi
    done
    if test "$found" = "0" ; then
        dirs="$dirs $testdir"
        printf "" >> ${builddir}/${testdir}/testlist.gpu
    fi

    # prepare extra args
    for arg in $args ; do
        other_args="$other_args arg=$arg"
    done

    printf "generating tests for: ${testname}... "
    if [ ${testname} == 'allred' ]
    then
        echo "${testname} $procs arg=-count=$count arg=-evenmemtype=host arg=-oddmemtype=device" >> ${builddir}/${testdir}/testlist.gpu
        echo "${testname} $procs arg=-count=$count arg=-evenmemtype=reg_host arg=-oddmemtype=device" >> ${builddir}/${testdir}/testlist.gpu
        echo "${testname} $procs arg=-count=$count arg=-evenmemtype=device arg=-oddmemtype=device" >> ${builddir}/${testdir}/testlist.gpu
    else
        echo "${testname} $procs arg=-evenmemtype=host arg=-oddmemtype=device" >> ${builddir}/${testdir}/testlist.gpu
        echo "${testname} $procs arg=-evenmemtype=reg_host arg=-oddmemtype=device" >> ${builddir}/${testdir}/testlist.gpu
        echo "${testname} $procs arg=-evenmemtype=device arg=-oddmemtype=device" >> ${builddir}/${testdir}/testlist.gpu
    fi
    printf "done\n"
done < ${srcdir}/gpu-test-config.txt
