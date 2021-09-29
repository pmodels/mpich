#! /bin/sh
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

autoreconf=${AUTORECONF:-autoreconf}

if test -d "mpl" ; then
    echo "=== running autoreconf in 'mpl' ==="
    (cd mpl && $autoreconf ${autoreconf_args:-"-vif"}) || exit 1
fi

if test -d "tools/topo/hwloc/hwloc" ; then
    echo "=== running autoreconf in 'tools/topo/hwloc/hwloc' ==="
    (cd tools/topo/hwloc/hwloc && ./autogen.sh) || exit 1
fi

echo "=== running autoreconf in '.' ==="
$autoreconf ${autoreconf_args:-"-vif"} || exit 1

# Remove the autom4te.cache folders for a release-like structure.
find . -name autom4te.cache | xargs rm -rf
