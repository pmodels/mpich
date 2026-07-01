#! /bin/sh
##
## Copyright 2026 Argonne National Laboratory
## SPDX-License-Identifier: Apache-2.0
##

ticks=`find test/mpi -name testlist | xargs grep xfail | \
    sed -e 's/.*xfail=ticket//g' -e 's/ .*//g' | sort -n | uniq`

for tt in $ticks ; do
    if test -z "`wget -O - http://trac.mpich.org/projects/mpich/query?status=closed\&id=$tt \
                 2> /dev/null | html2text | grep 'No tickets found'`" ; then
	echo "ticket $tt does not seem to be open"
    fi
done
