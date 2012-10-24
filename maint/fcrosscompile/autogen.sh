#!/bin/sh

reldir="`dirname $0`"
pgmdir=`(cd $reldir && pwd)`

(cd $pgmdir && \
rm -f config.log config.status && \
rm -f configure && \
rm -rf autom4te.cache)

(cd $pgmdir && \
# autoconf -I $HOME/mpich_work/mpich/confdb && \
# autoheader -I ../../confdb && \
autoconf -I ../../confdb && \
rm -fr config.log config.status autom4te.cache)

# Modify the configure to launch ./conftest on the remote host
# and fetch the result back.
# Currently this is a hack, needs to look for a more elegant way
# to do this within autoconf.
if [ -x ./configure ] ; then
    rm -f ./configure.old
    mv ./configure ./configure.old
    # Instead running ./conftest in configure:
    # Replace './conftest*' by './cross_run ./conftest*'
    # Where ./cross_run will take ./conftest as an argument and
    # Run it remotely on the backend.
    sed -e "s|'\(\./conftest\$.*\)'|'./cross_run \1'|g" ./configure.old > ./configure
    if [ ! -x ./configure ] ; then
        chmod u+x ./configure
    fi
fi
