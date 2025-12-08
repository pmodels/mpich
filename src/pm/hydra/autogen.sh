#! /bin/sh
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if test -z "$AUTORECONF" ; then
    AUTORECONF="autoreconf"
fi

if test -d "modules/mpl" ; then
    echo "=== running autogen.sh in 'modules/mpl' ==="
    (cd modules/mpl && ./autogen.sh) || exit 1
fi

if test -d "modules/pmi" ; then
    echo "=== running autogen.sh in 'modules/pmi' ==="
    (cd modules/pmi && ./autogen.sh) || exit 1
fi

if test -d "modules/hwloc" ; then
    echo "=== running autogen.sh in 'modules/hwloc' ==="
    (cd modules/hwloc && ./autogen.sh) || exit 1
fi

echo "=== running autoreconf in '.' ==="
$AUTORECONF -vif || exit 1

# Remove the autom4te.cache folders for a release-like structure.
find . -name autom4te.cache | xargs rm -rf
