#!/bin/sh

if [ -n "$MPICH_AUTOTOOLS_DIR" ] ; then
    autoreconf=${MPICH_AUTOTOOLS_DIR}/autoreconf
else
    autoreconf=${AUTORECONF:-autoreconf}
fi

$autoreconf ${autoreconf_args:-"-ivf"}
