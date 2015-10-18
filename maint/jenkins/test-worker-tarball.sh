#!/bin/zsh -xe
#
# (C) 2015 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

hostname

export PATH=$HOME/software/autotools/bin:$HOME/software/sowing/bin:$PATH
export DOCTEXT_PATH=$HOME/software/sowing/share/doctext
export CC=gcc
export CXX=g++
export FC=gfortran

cd $WORKSPACE

git clean -x -d -f

TMP_WORKSPACE=$(mktemp -d /sandbox/jenkins.tmp.XXXXXXXX)

if test ! -d $WORKSPACE/build ; then
    mkdir -p $WORKSPACE/build
fi

cp -a $WORKSPACE/* $TMP_WORKSPACE/
pushd $TMP_WORKSPACE
pushd build

../maint/release.pl --branch=master --version=master --git-repo=git://git.mpich.org/mpich.git

popd

cp -a $TMP_WORKSPACE/build/mpich-master.tar.gz $WORKSPACE/build

popd

rm -rf $TMP_WORKSPACE

exit 0
