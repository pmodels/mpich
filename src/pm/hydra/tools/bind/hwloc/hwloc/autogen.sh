:
libtoolize=${MPICH2_LIBTOOLIZE:-libtoolize}
autoreconf=${MPICH2_AUTORECONF:-autoreconf}
$libtoolize
$autoreconf -vif
