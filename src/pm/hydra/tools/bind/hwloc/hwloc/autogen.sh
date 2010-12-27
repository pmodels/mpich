#! /bin/sh

if [ -n "$MPICH2_AUTOTOOLS_DIR" ] ; then
    libtoolize=${MPICH2_AUTOTOOLS_DIR}/libtoolize
    autoreconf=${MPICH2_AUTOTOOLS_DIR}/autoreconf
else
    libtoolize=${LIBTOOLIZE:-libtoolize}
    autoreconf=${AUTORECONF:-autoreconf}
fi

$autoreconf -vif
