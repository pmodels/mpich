#!/bin/sh

#  example usage from Makefile.in
#  ${INSTALL} -m 4755 mpdroot ${bindir}/mpdroot ;\

if [ "$1" = "-c" ]
then
    shift    # get rid of it
fi

if [ "$1" != "-m" ]
then
    echo "-m option (permissions) required for this version of install.sh"
    exit -1
fi

perms=$2
src=$3
dest=$4

/bin/cp $src $dest
/bin/chmod $perms $dest
