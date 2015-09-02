#!/bin/zsh -xe

export PATH=$HOME/software/autotools/bin:$HOME/software/sowing/bin:$PATH
export DOCTEXT_PATH=$HOME/software/sowing/share/doctext
export CC=gcc
export CXX=g++
export FC=gfortran

cd $WORKSPACE
git clean -x -d -f

mkdir build
cd build

../maint/release.pl --branch=master --version=master --git-repo=git://git.mpich.org/mpich.git

exit 0
