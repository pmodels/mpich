#! /bin/sh
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

autoreconf=${AUTORECONF:-autoreconf}
$autoreconf ${autoreconf_args:-"-vif"} || exit 1
