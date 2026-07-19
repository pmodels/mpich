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
    CONFARGS="-vif"
fi

$AUTORECONF $CONFARGS || exit 1
