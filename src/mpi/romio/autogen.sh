#!/bin/sh

${AUTORECONF:-autoreconf} ${autoreconf_args:-"-vif"} -I confdb

# fix depcomp to support pgcc correctly
if grep "pgcc)" confdb/depcomp 2>&1 >/dev/null ; then :
else
    echo 'patching "confdb/depcomp" to support pgcc'
    patch -f -p0 < confdb/depcomp_pgcc.patch
fi

