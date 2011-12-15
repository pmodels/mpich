#!/bin/sh

if [ -n "$MPICH2_AUTOTOOLS_DIR" ] ; then
    autoreconf=${MPICH2_AUTOTOOLS_DIR}/autoreconf
else
    autoreconf=${AUTORECONF:-autoreconf}
fi

$autoreconf ${autoreconf_args:-"-vif"}

# fix depcomp to support pgcc correctly
if grep "pgcc)" confdb/depcomp 2>&1 >/dev/null ; then :
else
    echo 'patching "confdb/depcomp" to support pgcc'
    patch -f -p0 < confdb/depcomp_pgcc.patch
fi

