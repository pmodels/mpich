#!/bin/sh

reldir="`dirname $0`"
pgmdir=`(cd $reldir && pwd)`

(cd $pgmdir && \
rm -f config.log config.status && \
rm -f configure && \
rm -rf autom4te.cache)

(cd $pgmdir && \
# autoheader -I ../../confdb && \
autoconf -I ../../confdb && \
rm -fr config.log config.status autom4te.cache )

if [ -x ./configure ] ; then
    rm -f ./configure.old
    mv ./configure ./configure.old
    sed -e "s|'\(\./conftest\$.*\)'|'ssh mic1 \"cd \$PWD \&\& \1\"'|g" ./configure.old > ./configure
    # sed -e "s|'\(\./conftest\$.*\)'|'\${MPICH_CROSS_PRECMD}\1\${MPICH_CROSS_POSTCMD}'|g" ./configure.old > ./configure
    if [ ! -x ./configure ] ; then
        chmod u+x ./configure
    fi
fi
