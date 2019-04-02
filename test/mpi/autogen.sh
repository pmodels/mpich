#!/bin/sh
#
# (C) 2018 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

pathname=""
dirs=""

# We need to create testlist.in place holders otherwise autoconf will fail.
while read -r line
do
    if [ ! `echo $line | head -c 1` = "#" ] ; then
        # the line is not a comment
        pathname=`echo $line | cut -d: -f1`
    else
        # the line is a comment
        continue
    fi

    dir=`dirname $pathname`

    # check whether testlist.in files already exist in dir ...
    exists=0
    for i in $dirs
    do
        if [ $i = $dir ] ; then
            exists=1
        fi
    done

    # ... if not create them
    if [ $exists = 0 ] ; then
        printf "" > ${dir}/testlist.in
        dirs="$dirs $dir "
    fi
done < basictypetest.txt

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
