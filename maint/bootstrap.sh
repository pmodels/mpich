#!/usr/bin/env bash
#
# Fetches MPICH dependencies, builds them from source, and installs them
# in the path stored in the environment variable MPICH_DEPS_PATH.

# Dependencies to install:
LIBTOOL=libtool-2.4.6
AUTOCONF=autoconf-2.69
AUTOMAKE=automake-1.15

# Check that the environment variable is set:
MPICH_DEPS_PREFIX=${MPICH_DEPS_PREFIX:?"bootstrap.sh installs MPICH dependencies \
in the path stored in the environment variable MPICH_DEPS_PREFIX but this \
variable is not set. Please set it to the path in which you want to install \
MPICH dependencies and try again."}

echo "Bootstrapping MPICH dependencies..."

# The dependencies will be downloaded and compiled in this directory
MPICH_DEPS_SRC=/tmp/mpich_deps_src

set -e # Trap on error

mkdir -p ${MPICH_DEPS_PREFIX}
mkdir    ${MPICH_DEPS_SRC}    # Errors out if the directory already exists

# File used to log errors (will be cleaned up later)
ERROR_LOG="mpich_bootstrap.log"

# Dumps the file in the ERROR_LOG environment variable to stderr and exits
function dump_ERROR_LOG {
    if [ -z ${ERROR_LOG+x} ]; then
        exit 1
    else
        cat ${ERROR_LOG} >&2
        exit 1
    fi
}

# On error, dump the log
trap 'dump_ERROR_LOG' ERR

cd ${MPICH_DEPS_SRC}

echo -ne "Downloading dependencies... libtool  [0%]\r"
wget http://ftpmirror.gnu.org/libtool/${LIBTOOL}.tar.gz >${ERROR_LOG} 2>&1
echo -ne "Downloading dependencies... autoconf [33%]\r"
wget http://ftp.gnu.org/gnu/autoconf/${AUTOCONF}.tar.gz >${ERROR_LOG} 2>&1
echo -ne "Downloading dependencies... automake [66%]\r"
wget http://ftp.gnu.org/gnu/automake/${AUTOMAKE}.tar.gz >${ERROR_LOG} 2>&1
echo     "Downloading dependencies... done.           "

echo -ne "Extracting dependencies... libtool  [0%]\r"
tar -xzf ${LIBTOOL}.tar.gz >${ERROR_LOG} 2>&1
echo -ne "Extracting dependencies... autoconf [33%]\r"
tar -xzf ${AUTOCONF}.tar.gz >${ERROR_LOG} 2>&1
echo -ne "Extracting dependencies... automake [66%]\r"
tar -xzf ${AUTOMAKE}.tar.gz >${ERROR_LOG} 2>&1
echo     "Extracting dependencies... done.           "

echo -ne "Installing dependencies... libtool  [0%]\r"
cd ${LIBTOOL}
./configure --prefix=${MPICH_DEPS_PREFIX} >${ERROR_LOG} 2>&1
make -j >${ERROR_LOG} 2>&1
make install -j >${ERROR_LOG} 2>&1
echo -ne "Installing dependencies... autoconf [33%]\r"
cd ../${AUTOCONF}
./configure --prefix=${MPICH_DEPS_PREFIX} >${ERROR_LOG} 2>&1
make install -j >${ERROR_LOG} 2>&1
make -j >${ERROR_LOG} 2>&1
echo -ne "Installing dependencies... automake [66%]\r"
cd ../${AUTOMAKE}
./configure --prefix=${MPICH_DEPS_PREFIX} >${ERROR_LOG} 2>&1
make -j >${ERROR_LOG} 2>&1
make install -j >${ERROR_LOG} 2>&1
echo     "Installing dependencies... done.          "
cd ../..

rm -rf ${MPICH_DEPS_SRC} # Cleanup the source directory.

echo     "MPICH bootstrapping finished. All dependencies have been installed to ${MPICH_DEPS_PREFIX}."
echo     "Please make sure that the directory is in your PATH before continuing configuring MPICH."
