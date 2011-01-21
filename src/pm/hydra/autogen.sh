#! /bin/sh

if [ -n "$MPICH2_AUTOTOOLS_DIR" ] ; then
    libtoolize=${MPICH2_AUTOTOOLS_DIR}/libtoolize
    autoreconf=${MPICH2_AUTOTOOLS_DIR}/autoreconf
else
    libtoolize=${LIBTOOLIZE:-libtoolize}
    autoreconf=${AUTORECONF:-autoreconf}
fi

(cd mpl && $autoreconf -vif)
(cd tools/bind/hwloc/hwloc && $autoreconf -vif)
(cd tools/bind/plpa/plpa && $autoreconf -vif)
$autoreconf -vif

# Remove the autom4te.cache folders for a release-like structure.
find . -name autom4te.cache | xargs rm -rf
