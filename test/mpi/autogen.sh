#! /bin/sh
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

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
