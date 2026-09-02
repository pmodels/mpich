#! /bin/sh
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if test -z "$AUTORECONF" ; then
    AUTORECONF="autoreconf"
fi

if test -n "$MPICH_CONFDB" ; then
    CONFARGS="-I $MPICH_CONFDB -vif"
    mkdir -p confdb
    cp -v $MPICH_CONFDB/config.rpath confdb/
else
    if ! test -d confdb ; then
        echo "Missing confdb!"
        exit 1
    fi

    CONFARGS="-vif"
fi


$AUTORECONF $CONFARGS || exit 1
