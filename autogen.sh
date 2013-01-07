#!/bin/sh

if [ -n "$MPICH2_AUTOTOOLS_DIR" ] ; then
    autoreconf=${MPICH2_AUTOTOOLS_DIR}/autoreconf
else
    autoreconf=${AUTORECONF:-autoreconf}
fi

$autoreconf ${autoreconf_args:-"-vif"} || exit 1
