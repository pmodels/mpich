#!/bin/sh
#
# (C) 2018 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#
# Generate datatype testing build instructions for every line in
# configuration file basictypetest.txt and structtypetest.txt

types=""     # basic MPI datatypes for type signature
counts=""    # counts for type signature
pathname=""  # source file pathname
source=""    # base name of source file - extension
ext=""       # extension of source file
dir=""       # directory in which source is located
dirs=""      # all directories containing DTPools tests
macros=""    # additional control macros (user defined)
timelimit="" # time limit for testlist.in
procs=""     # number of processes to use in testlist.in
other_macros=""

# Read basic MPI datatypes
while read -r line
do
    if [ ! `echo $line | head -c 1` = "#" ] ; then
        types="$types $line "
    fi
done < basictypelist.txt

ln=1 # line number
while read -r line
do
    if [ ! `echo $line | head -c 1` = "#" ] ; then
        # the line is not a comment
        pathname=`echo $line | sed -E 's|(.+):(.*):(.+):(.*):(.+)|\1|'`
        macros=`echo $line | sed -E 's|(.+):(.*):(.+):(.*):(.+)|\2|'`
        counts=`echo $line | sed -E 's|(.+):(.*):(.+):(.*):(.+)|\3|'`
        timelimit=`echo $line | sed -E 's|(.+):(.*):(.+):(.*):(.+)|\4|'`
        procs=`echo $line | sed -E 's|(.+):(.*):(.+):(.*):(.+)|\5|'`
        other_macros=""
    else
        # the line is a comment
        pathname=`echo $line | sed -E 's|(.+):(.*):(.+):(.*):(.+)|\1|'`
        ln=$((ln+1)) # increment line number
        continue
    fi

    dir=`dirname $pathname`
    source=`echo $(basename $pathname) | sed -E "s|(.+)\.c[xx]*|\1|"`
    ext=`echo $(basename $pathname) | sed -E "s|.+\.(c[xx]*)|\1|"`

    # check whether .dtp and .in files already exist in dir ...
    exists=0
    for i in $dirs
    do
        if [ $i = $dir ] ; then
            exists=1
        fi
    done

    # ... if not create them
    if [ $exists = 0 ] ; then
        printf "" > ${dir}/Makefile.dtp
        printf "" > ${dir}/testlist.dtp
        printf "" > ${dir}/testlist.in
        dirs="$dirs $dir "
    fi

    # prepare user defined macros
    for macro in $macros
    do
        other_macros="$other_macros -D$macro "
    done

    printf "basictypetest.txt:${ln}: Generate tests for: ${source}.${ext} ... "

    # generate Makefile.dtp
    echo "noinst_PROGRAMS += ${source}__BASIC__L${ln}" >> ${dir}/Makefile.dtp
    echo "${source}__BASIC__L${ln}_CPPFLAGS = $other_macros \$(AM_CPPFLAGS)" >> ${dir}/Makefile.dtp
    echo "${source}__BASIC__L${ln}_SOURCES = ${source}.${ext}" >> ${dir}/Makefile.dtp

    sendcounts=$counts

    for type in $types
    do
        recvcounts=$sendcounts # reset recv counts to send counts

        for sendcount in $sendcounts
        do
            if [ $dir = "pt2pt" ] ; then # only send/recv comm can use types from different pools
                # do combination of different send recv count where recv count >= send count
                for recvcount in $recvcounts
                do
                    echo "${source}__BASIC__L${ln} $procs arg=-type=${type} arg=-sendcnt=${sendcount} arg=-recvcnt=${recvcount} $timelimit" >> ${dir}/testlist.dtp
                     # limit the mixed pool case to only one
                     # TODO: this should be defined in the config file
                    if [ $recvcount -gt $sendcount ]; then
                        break
                    fi
                done
                recvcounts=`echo $recvcounts | sed -e "s|$sendcount||"` # update recv counts
            else
                echo "${source}__BASIC__L${ln} $procs arg=-type=${type} arg=-count=${sendcount} $timelimit" >> ${dir}/testlist.dtp
            fi
        done
    done
    ln=$((ln+1)) # increment line count
    printf "done\n"
done < basictypetest.txt

ln=1 # line number
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
        ln=$((ln+1)) # increment line number
        continue
    fi

    dir=`dirname $pathname`
    source=`echo $(basename $pathname) | sed -E "s|(.+)\.c[xx]*|\1|"`
    ext=`echo $(basename $pathname) | sed -E "s|.+\.(c[xx]*)|\1|"`

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
        printf "" > ${dir}/Makefile.dtp
        printf "" > ${dir}/testlist.dtp
        printf "" > ${dir}/testlist.in
        dirs="$dirs $dir "
    fi

    # prepare user defined macros
    for macro in $macros
    do
        other_macros="$other_macros -D$macro "
    done

    printf "structtypetest.txt:${ln}: Generate tests for: ${source}.${ext} ... "
    echo "noinst_PROGRAMS += ${source}__STRUCT__L${ln}" >> ${dir}/Makefile.dtp
    echo "${source}__STRUCT__L${ln}_CPPFLAGS = $other_macros \$(AM_CPPFLAGS)" >> ${dir}/Makefile.dtp
    echo "${source}__STRUCT__L${ln}_SOURCES = ${source}.${ext}" >> ${dir}/Makefile.dtp
    echo "${source}__STRUCT__L${ln} $procs arg=-numtypes=$numtypes arg=-types=$types arg=-counts=$counts $timelimit" >> ${dir}/testlist.dtp
    printf "done\n"
    ln=$((ln+1))
done < structtypetest.txt

for dir in $dirs
do
    printf "Generate testlist in dir: ${dir} ... "
    cat ${dir}/testlist.def >> ${dir}/testlist.in
    cat ${dir}/testlist.dtp >> ${dir}/testlist.in
    rm -f ${dir}/testlist.dtp
    printf "done\n"
done

printf "Generate basictypelist.txt for dtpools ... "
printf "" > dtpools/basictypelist.txt
while read -r line; do
    if [ ! `echo $line | head -c 1` = "#" ]; then
        echo "$line," >> dtpools/basictypelist.txt
    fi
done < basictypelist.txt
echo "MPI_DATATYPE_NULL" >> dtpools/basictypelist.txt
printf "done\n"

# Create and/or update the f90 tests
printf "Create or update the Fortran 90 tests derived from the Fortran 77 tests... "
for dir in f77/* ; do
    if [ ! -d $dir ] ; then continue ; fi
    leafDir=`basename $dir`
    if [ ! -d f90/$leafDir ] ; then
	mkdir f90/$leafDir
    fi
    if maint/f77tof90 $dir f90/$leafDir Makefile.am Makefile.ap ; then
        echo "timestamp" > f90/$leafDir/Makefile.am-stamp
    else
        echo "failed"
        error "maint/f77tof90 $dir failed!"
        exit 1
    fi
done
for dir in errors/f77/* ; do
    if [ ! -d $dir ] ; then continue ; fi
    leafDir=`basename $dir`
    if [ ! -d errors/f90/$leafDir ] ; then
	mkdir errors/f90/$leafDir
    fi
    if maint/f77tof90 $dir errors/f90/$leafDir Makefile.am Makefile.ap ; then
        echo "timestamp" > errors/f90/$leafDir/Makefile.am-stamp
    else
        echo "failed"
        error "maint/f77tof90 $dir failed!"
        exit 1
    fi
done
echo "done"
