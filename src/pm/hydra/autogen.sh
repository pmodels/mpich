#! /bin/sh

(cd tools/bind/hwloc/hwloc && ./autogen.sh)
(cd tools/bind/plpa/plpa && ./autogen.sh)
libtoolize=${MPICH2_LIBTOOLIZE:-libtoolize}
autoreconf=${MPICH2_AUTORECONF:-autoreconf}
$libtoolize
$autoreconf -vif
