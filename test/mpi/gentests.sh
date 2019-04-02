#!/bin/sh
#
# (C) 2018 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#
# Generate datatype testing build instructions for every line in
# configuration file basictypetest.txt

types=""     # basic MPI datatypes for type signature
counts=""    # counts for type signature
pathname=""  # source file pathname
source=""    # base name of source file - extension
dir=""       # directory in which source is located
dirs=""      # all directories containing DTPools tests
args=""      # extra args
timelimit="" # time limit for testlist.in
procs=""     # number of processes to use in testlist.in
srcdir=""    # relative directory in which this script is located
other_args=""

# extract src directory from configure
srcdir=`dirname $0`

# Read basic MPI datatypes
while read -r line
do
    if [ ! `echo $line | head -c 1` = "#" ] ; then
        types="$types $line "
    fi
done < ${srcdir}/basictypelist.txt

while read -r line
do
    if [ ! `echo $line | head -c 1` = "#" ] ; then
        # the line is not a comment
        pathname=`echo $line | sed -E 's|(.+):(.*):(.+):(.*):(.+)|\1|'`
        args=`echo $line | sed -E 's|(.+):(.*):(.+):(.*):(.+)|\2|'`
        counts=`echo $line | sed -E 's|(.+):(.*):(.+):(.*):(.+)|\3|'`
        timelimit=`echo $line | sed -E 's|(.+):(.*):(.+):(.*):(.+)|\4|'`
        procs=`echo $line | sed -E 's|(.+):(.*):(.+):(.*):(.+)|\5|'`
        other_args=""
    else
        # the line is a comment
        continue
    fi

    dir=${srcdir}/`dirname $pathname`
    source=`echo $(basename $pathname)`

    # check whether .dtp files already exist in dir ...
    exists=0
    for i in $dirs
    do
        if [ $i = $dir ] ; then
            exists=1
        fi
    done

    # ... if not create them
    if [ $exists = 0 ] ; then
        printf "" > ${dir}/testlist.dtp
        printf "" > ${dir}/testlist.in
        dirs="$dirs $dir "
    fi

    # prepare extra args
    for arg in $args
    do
        other_args="$other_args arg=$arg"
    done

    printf "generating tests for: ${source}... "

    sendcounts=$counts

    for type in $types
    do
        recvcounts=$sendcounts # reset recv counts to send counts

        for sendcount in $sendcounts
        do
            if [ $(basename $dir) = "pt2pt" ] ; then # only send/recv comm can use types from different pools
                # do combination of different send recv count where recv count >= send count
                for recvcount in $recvcounts
                do
                    echo "${source} $procs arg=-type=${type} arg=-sendcnt=${sendcount} arg=-recvcnt=${recvcount} ${other_args} $timelimit" >> ${dir}/testlist.dtp
                     # limit the mixed pool case to only one
                     # TODO: this should be defined in the config file
                    if [ $recvcount -gt $sendcount ]; then
                        break
                    fi
                done
                recvcounts=`echo $recvcounts | sed -e "s|$sendcount||"` # update recv counts
            else
                echo "${source} $procs arg=-type=${type} arg=-count=${sendcount} ${other_args} $timelimit" >> ${dir}/testlist.dtp
            fi
        done
    done
    printf "done\n"
done < ${srcdir}/basictypetest.txt

while read -r line
do
    if [ ! `echo $line | head -c 1` = "#" ] ; then
        # the line is not a comment
        pathname=`echo $line | sed -E 's|(.+):(.*):(.+):(.+):(.+):(.*):(.+)|\1|'`
        macros=`echo $line | sed -E 's|(.+):(.*):(.+):(.+):(.+):(.*):(.+)|\2|'`
        numtypes=`echo $line | sed -E 's|(.+):(.*):(.+):(.+):(.+):(.*):(.+)|\3|'`
        types=`echo $line | sed -E 's|(.+):(.*):(.+):(.+):(.+):(.*):(.+)|\4|'`
        counts=`echo $line | sed -E 's|(.+):(.*):(.+):(.+):(.+):(.*):(.+)|\5|'`
        timelimit=`echo $line | sed -E 's|(.+):(.*):(.+):(.+):(.+):(.*):(.+)|\6|'`
        procs=`echo $line | sed -E 's|(.+):(.*):(.+):(.+):(.+):(.*):(.+)|\7|'`
        other_macros=""
    else
        # the line is a comment
        continue
    fi

    dir=${srcdir}/`dirname $pathname`
    source=`echo $(basename $pathname)`

    # NOTE: .dtp and .in files should already exist at this point ...
    exists=0
    for i in $dirs
    do
        if [ $i = $dir ] ; then
            exists=1
        fi
    done

    # ... if they don't create them
    if [ $exists = 0 ] ; then
        printf "" > ${dir}/testlist.dtp
        printf "" > ${dir}/testlist.in
        dirs="$dirs $dir "
    fi

    # prepare extra args
    for arg in $args
    do
        other_args="$other_args arg=$arg"
    done

    printf "generating tests for: ${source}... "
    echo "${source} $procs arg=-numtypes=$numtypes arg=-types=$types arg=-counts=$counts ${other_args} $timelimit" >> ${dir}/testlist.dtp
    printf "done\n"
done < ${srcdir}/structtypetest.txt

for dir in $dirs
do
    printf "generating testlist for: ${dir}... "
    cat ${dir}/testlist.def >> ${dir}/testlist.in
    cat ${dir}/testlist.dtp >> ${dir}/testlist.in
    rm -f ${dir}/testlist.dtp
    printf "done\n"
done

printf "generating basictypelist.txt for dtpools... "
printf "" > ${srcdir}/dtpools/basictypelist.txt
while read -r line; do
    if [ ! `echo $line | head -c 1` = "#" ]; then
        echo "$line," >> ${srcdir}/dtpools/basictypelist.txt
    fi
done < ${srcdir}/basictypelist.txt
echo "MPI_DATATYPE_NULL" >> ${srcdir}/dtpools/basictypelist.txt
printf "done\n"
