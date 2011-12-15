:
autoreconf ${autoreconf_args:-"-vif"}

# fix depcomp to support pgcc correctly
if grep "pgcc)" config/depcomp 2>&1 >/dev/null ; then :
else
    echo 'patching "config/depcomp" to support pgcc'
    patch -f -p0 < config/depcomp_pgcc.patch
fi

