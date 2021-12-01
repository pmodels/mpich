#! /bin/sh
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

check_copy() {
    name=$1
    orig=$2
    printf "Checking $name... "
    if test ! -e $name ; then
        if test -e $orig; then
            cp -a $orig $name
            echo "copy from $orig"
        else
            echo "missing"
            exit 1
        fi
    else
        echo "found"
    fi
}

check_copy version.m4     ../../maint/version.m4
check_copy confdb         ../../confdb
check_copy dtpools/confdb ../../confdb

echo "Running autoreconf in dtpools"
(cd dtpools && autoreconf -ivf)

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

echo "Running autoreconf in ."
autoreconf -ivf
