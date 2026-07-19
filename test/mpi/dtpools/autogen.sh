#! /bin/sh
##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

if test -z "$AUTORECONF" ; then
    AUTORECONF="autoreconf"
fi

if test -n "$MPICH_CONFDB" ; then
    CONFARGS="-I $MPICH_CONFDB -vif"
    mkdir -p confdb
else
    if ! test -d confdb ; then
        echo "Missing confdb!"
        exit 1
    fi

    CONFARGS="-vif"
    # set it for mpl
    export MPICH_CONFDB=`realpath confdb`
fi

$AUTORECONF $CONFARGS || exit 1
