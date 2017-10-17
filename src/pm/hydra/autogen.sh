#! /bin/sh

autoreconf=${AUTORECONF:-autoreconf}

echo "=== running autoreconf in 'mpl' ==="
(cd mpl && $autoreconf ${autoreconf_args:-"-vif"}) || exit 1

echo "=== running autoreconf in 'tools/topo/hwloc/hwloc' ==="
(cd tools/topo/hwloc/hwloc && ./autogen.sh) || exit 1

echo "=== running autoreconf in '.' ==="
$autoreconf ${autoreconf_args:-"-vif"} || exit 1

# Remove the autom4te.cache folders for a release-like structure.
find . -name autom4te.cache | xargs rm -rf
