#!/bin/zsh -xe

hostname

export PATH=$HOME/software/autotools/bin:$HOME/software/sowing/bin:$PATH
export DOCTEXT_PATH=$HOME/software/sowing/share/doctext
export CC=gcc
export CXX=g++
export FC=gfortran

cd $WORKSPACE
TMP_WORKSPACE=$(mktemp -d /sandbox/jenkins.tmp.XXXXXXXX)

mkdir build

cp -a $WORKSPACE/* $TMP_WORKSPACE/
pushd $TMP_WORKSPACE

git clean -x -d -f

pushd build

../maint/release.pl --branch=master --version=master --git-repo=git://git.mpich.org/mpich.git

popd

cp -a $TMP_WORKSPACE/build/mpich-master.tar.gz $WORKSPACE/build

popd

rm -rf $TMP_WORKSPACE

exit 0
