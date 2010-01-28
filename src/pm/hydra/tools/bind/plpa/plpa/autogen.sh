:
# This file is used to ensure that the rebuild of the autotools files
# when run from within MPICH2 by maint/updatefiles will use the 
# correct versions of the autotools (many systems have versions installed
# in /usr/bin or /usr/local/bin that are too old to permit successful
# building of the autotools)
# Allow the following choices for libtoolize (similarly for autoreconf):
# LIBTOOLIZE if set, else MPICH2_LIBTOOLIZE if set, else libtoolize
if [ "X$LIBTOOLIZE" != "X" ] ; then
    libtoolize=$LIBTOOLIZE
else
    libtoolize=${MPICH2_LIBTOOLIZE:-libtoolize}
fi
if [ "X$AUTORECONF" != "X" ] ; then
    autoreconf=$AUTORECONF
else
    autoreconf=${MPICH2_AUTORECONF:-autoreconf}
fi
$libtoolize
$autoreconf -vif
