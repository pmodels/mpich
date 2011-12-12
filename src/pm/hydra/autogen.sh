#! /bin/sh

if [ -n "$MPICH2_AUTOTOOLS_DIR" ] ; then
    libtoolize=${MPICH2_AUTOTOOLS_DIR}/libtoolize
    autoreconf=${MPICH2_AUTOTOOLS_DIR}/autoreconf
else
    libtoolize=${LIBTOOLIZE:-libtoolize}
    autoreconf=${AUTORECONF:-autoreconf}
fi

echo "=== running autoreconf in 'mpl' ==="
(cd mpl && $autoreconf ${autoreconf_args:-"-vif"})
echo "=== running autoreconf in 'tools/topo/hwloc/hwloc' ==="
(cd tools/topo/hwloc/hwloc && $autoreconf ${autoreconf_args:-"-vif"})
$autoreconf ${autoreconf_args:-"-vif"}

# Remove the autom4te.cache folders for a release-like structure.
find . -name autom4te.cache | xargs rm -rf
