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
else
    CONFARGS="-vif"
fi

$AUTORECONF $CONFARGS || exit 1
