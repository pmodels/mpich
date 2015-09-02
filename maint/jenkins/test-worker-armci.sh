#!/bin/zsh -xe

WORKSPACE=""
GIT_BRANCH=""
BUILD_MODE="per-commit"

#####################################################################
## Initialization
#####################################################################

while getopts ":h:b:" opt; do
    case "$opt" in
        h)
            WORKSPACE=$OPTARG ;;
        b)
            GIT_BRANCH=$OPTARG ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
    esac
done

cd $WORKSPACE

TMP_WORKSPACE=$(mktemp -d /sandbox/jenkins.tmp.XXXXXXXX)
SRC=$WORKSPACE
TMP_SRC=$TMP_WORKSPACE

if test "$GIT_BRANCH" = "" ; then
    BUILD_MODE="nightly"
fi

# Preparing the source
case "$BUILD_MODE" in
    "nightly")
        tar zxvf mpich-master.tar.gz
        SRC=$WORKSPACE/mpich-master
        TMP_SRC=$TMP_WORKSPACE/mpich-master
        cp -a $WORKSPACE/mpich-master $TMP_WORKSPACE/
        ;;
    "per-commit")
        cp -a $WORKSPACE/* $TMP_WORKSPACE/
        ;;
    *)
        echo "Invalid BUILD_MODE $BUILD_MODE. Set by mistake?"
        exit 1
esac

PATH=$HOME/software/autotools/bin:$PATH
export PATH

#####################################################################
## Main() { Setup Environment and Build
#####################################################################

pushd "$TMP_SRC"

if test "$BUILD_MODE" = "per-commit" ; then
    ./autogen.sh 2>&1 | tee autogen.log
fi

./configure --prefix="$TMP_SRC/_inst" CC=gcc CXX=g++ FC=gfortran 2>&1 | tee c.txt
make -j16 install 2>&1 | tee mi.txt

#####################################################################
## Run tests
#####################################################################

rm -rf ./armci-mpi
git clone -b mpi3rma git://git.mpich.org/armci-mpi

export PATH=$TMP_SRC/_inst/bin:$PATH

pushd $TMP_SRC/armci-mpi

./autogen.sh
./configure
make -j8
make check

popd
popd

rm -rf $TMP_WORKSPACE
exit 0

