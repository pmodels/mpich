#! /bin/sh

if [ -n "$MPICH_AUTOTOOLS_DIR" ] ; then
    autoreconf=${MPICH_AUTOTOOLS_DIR}/autoreconf
else
    autoreconf=${AUTORECONF:-autoreconf}
fi

echo "=== running autoreconf in 'mpl' ==="
(cd mpl && $autoreconf ${autoreconf_args:-"-vif"}) || exit 1

echo "=== running autoreconf in 'libhydra/topo/hwloc/hwloc' ==="
(cd libhydra/topo/hwloc/hwloc && ./autogen.sh) || exit 1

echo "=== running autoreconf in '.' ==="
$autoreconf ${autoreconf_args:-"-vif"} || exit 1

# Remove the autom4te.cache folders for a release-like structure.
find . -name autom4te.cache | xargs rm -rf
