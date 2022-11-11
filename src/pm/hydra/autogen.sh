#! /bin/sh
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

autoreconf=${AUTORECONF:-autoreconf}

if test -d "modules/mpl" ; then
    echo "=== running autoreconf in 'modules/mpl' ==="
    (cd modules/mpl && $autoreconf ${autoreconf_args:-"-vif"}) || exit 1
fi

if test -d "modules/pmi" ; then
    echo "=== running autoreconf in 'modules/pmi' ==="
    (cd modules/pmi && ./autogen.sh) || exit 1
fi

if test -d "modules/hwloc" ; then
    echo "=== running autoreconf in 'modules/hwloc' ==="
    (cd modules/hwloc && ./autogen.sh) || exit 1
fi

echo "=== running autoreconf in '.' ==="
$autoreconf ${autoreconf_args:-"-vif"} || exit 1

# Remove the autom4te.cache folders for a release-like structure.
find . -name autom4te.cache | xargs rm -rf
