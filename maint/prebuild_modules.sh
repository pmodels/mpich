#! /usr/bin/env bash
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# Prebuild modules into modules.tar.gz. Copy the tarball to a fresh git cloned repository,
# both autogen.sh and configure will skip them in the build process.
#
# This is to accelerate repeated CI testing.
#

git submodule update --init

make_it_lean () {
    rm -rf .git
    find . -name '*.o' | xargs rm -f
}

pushd modules/hwloc
extra_option=$HWLOC_EXTRA
./autogen.sh
./configure CFLAGS=-fvisibility=hidden \
    --enable-embedded-mode --enable-visibility=no \
    --disable-libxml2 --disable-nvml --disable-cuda --disable-opencl --disable-rsmi $extra_option
make
make_it_lean
popd

pushd modules/json-c
extra_option=$JSONC_EXTRA
./autogen.sh
./configure --enable-embedded --disable-werror $extra_option
make
make_it_lean
popd

pushd modules/yaksa
extra_option=$YAKSA_EXTRA
./autogen.sh
./configure --enable-embedded $extra_option
make
make_it_lean
popd

pushd modules/libfabric
extra_option=$LIBFABRIC_EXTRA
./autogen.sh
./configure --enable-embedded $extra_option
make
make_it_lean
popd

# ucx need make install to work, which need replace all hardcoded paths to work
pushd modules/ucx
extra_option=$UCX_EXTRA
./autogen.sh
if false ; then
    # skip for now
    ./configure --prefix=/MODPREFIX --disable-static $extra_option
    make
    find . -name '*.la' | xargs --verbose sed -i "s,$PWD,MODDIR,g"
fi
make_it_lean
popd

# Add a flag to mark modules as pre-built. Remove the flag file to allow configure
# reconfigure and rebuild modules.
touch modules/PREBUILT

tar czf modules.tar.gz modules
