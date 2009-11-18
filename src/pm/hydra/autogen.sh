#! /bin/sh

(cd tools/bind/hwloc/hwloc && ./autogen.sh)
libtoolize=${MPICH2_LIBTOOLIZE:-libtoolize}
autoreconf=${MPICH2_AUTORECONF:-autoreconf}
$libtoolize
$autoreconf -vif
