#!/bin/zsh -x
#
# (C) 2015 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

hostname

WORKSPACE=""
build_mpi="mpich-master"
compiler="gnu"
run_conf="default"
run_mpi="mpich-master"

#####################################################################
## Initialization
#####################################################################

while getopts ":h:c:o:b:r:" opt; do
    case "$opt" in
        h)
            WORKSPACE=$OPTARG ;;
        c)
            compiler=$OPTARG ;;
        o)
            run_conf=$OPTARG ;;
        b)
            build_mpi=$OPTARG ;;
        r)
            run_mpi=$OPTARG ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
    esac
done

cd $WORKSPACE
TMP_WORKSPACE=$(mktemp -d /sandbox/jenkins.tmp.XXXXXXXX)

# $build_mpi -- MPI libraries used to build the tests
# $run_mpi   -- MPI libraries used to run the tests
# $compiler  -- gcc or icc. Compilers used to build $build_mpi _and_ $run_mpi.
#               Intel MPI provides mpigcc, mpiicc etc.
# $run_conf  -- debug or default. Options used to build $build_mpi _and_ $run_mpi

#####################################################################
## Setup the paths for the compilers
#####################################################################
source /software/common/adm/etc/softenv-aliases.sh
source /software/common/adm/etc/softenv-load.sh
soft add +intel

if test "$compiler" = "gnu" ; then
  CC=gcc
  CXX=g++
  F77=gfortran
  FC=gfortran
elif test "$compiler" = "intel" ; then
  CC=icc
  CXX=icpc
  F77=ifort
  FC=ifort
else
  echo "Unknown compiler suite"
  exit 1
fi

export CC
export CXX
export F77
export FC

which $CC
which $CXX
which $F77
which $FC

## Setup tmp directory
mkdir ${TMP_WORKSPACE}/_tmp

## Setup binaries in MPICH
if test "$build_mpi" != "impi-5.0"; then
  cp -a ${HOME}/mpi/installs/${build_mpi}/${compiler}/${run_conf} ${TMP_WORKSPACE}/_tmp/${build_mpi}-inst
  MPICH_BUILD_PATH=${TMP_WORKSPACE}/_tmp/${build_mpi}-inst
  export MPICH_BUILD_PATH
  cd ${MPICH_BUILD_PATH}/bin
  for x in mpif77 mpif90 mpicc mpicxx ; do
    sed -i "s+^libdir=.*+libdir=${MPICH_BUILD_PATH}/lib+g" $x
    sed -i "s+^prefix=.*+prefix=${MPICH_BUILD_PATH}+g" $x
    sed -i "s+^includedir=.*+includedir=${MPICH_BUILD_PATH}/include+g" $x
  done
fi
if test "$run_mpi" != "impi-5.0"; then
  if test "$build_mpi" != "$run_mpi"; then
    cp -a ${HOME}/mpi/installs/${run_mpi}/${compiler}/${run_conf} ${TMP_WORKSPACE}/_tmp/${run_mpi}-inst
  fi
  MPICH_RUN_PATH=${TMP_WORKSPACE}/_tmp/${run_mpi}-inst
  export MPICH_RUN_PATH
fi

## Setup binaries in IMPI
cp -a /home/autotest/intel/impi/5.0.1.035 ${TMP_WORKSPACE}/_tmp/impi-inst
IMPI_PATH=${TMP_WORKSPACE}/_tmp/impi-inst
export IMPI_PATH
cd ${IMPI_PATH}/intel64/bin
for x in mpif77 mpif90 mpigcc mpigxx mpiicc mpiicpc mpiifort ; do
  sed -i "s+^prefix=.*+prefix=${IMPI_PATH}+g" $x
done


## Setup compiler paths
if test "$build_mpi" = "mpich-master" -o "$build_mpi" = "mpich-3.1" ; then
  MPICC=${MPICH_BUILD_PATH}/bin/mpicc
  MPICXX=${MPICH_BUILD_PATH}/bin/mpicxx
  MPIF77=${MPICH_BUILD_PATH}/bin/mpif77
  MPIFC=${MPICH_BUILD_PATH}/bin/mpif90
elif test "$build_mpi" = "impi-5.0" ; then
  if test "$compiler" = "gnu" ; then
    MPICC=${IMPI_PATH}/intel64/bin/mpigcc
    MPICXX=${IMPI_PATH}/intel64/bin/mpigxx
    MPIF77=${IMPI_PATH}/intel64/bin/mpif77
    MPIFC=${IMPI_PATH}/intel64/bin/mpif90
  elif test "$compiler" = "intel" ; then
    MPICC=${IMPI_PATH}/intel64/bin/mpiicc
    MPICXX=${IMPI_PATH}/intel64/bin/mpiicpc
    MPIF77=${IMPI_PATH}/intel64/bin/mpiifort
    MPIFC=${IMPI_PATH}/intel64/bin/mpiifort
  fi
fi
export MPICC
export MPICXX
export MPIF77
export MPIFC

if test "$run_mpi" = "mpich-master" -o "$run_mpi" = "mpich-3.1" ; then
  MPIEXEC=${MPICH_RUN_PATH}/bin/mpiexec
elif test "$run_mpi" = "impi-5.0"; then
  MPIEXEC=${IMPI_PATH}/intel64/bin/mpiexec.hydra
fi
export MPIEXEC

cp -a /home/autotest/mpi/tests/mpich-tests ${TMP_WORKSPACE}/_tmp

MPICH_TESTS_PATH=${TMP_WORKSPACE}/_tmp/mpich-tests/test/mpi
export MPICH_TESTS_PATH

cd ${MPICH_TESTS_PATH}
./configure --disable-ft-tests --disable-dependency-tracking
make -j16 --keep-going

## Some tests require special compilation
cd f77/ext
make allocmemf
cd ${MPICH_TESTS_PATH}

cd f90/ext
make allocmemf90
cd ${MPICH_TESTS_PATH}

cd f90/profile
make profile1f90
cd ${MPICH_TESTS_PATH}

cd impls/mpich
make
cd ${MPICH_TESTS_PATH}

# these tests will compile with old MPIs, but produce the wrong result at runtime
if test "$run_mpi" = "impi-5.0" -o "$run_mpi" = "mpich-3.1" ; then
  sed -i "s+\(^gather_big .*\)+\1 xfail=ticket0+g" coll/testlist
  sed -i "s+\(^comm_idup_comm .*\)+\1 xfail=ticket0+g" comm/testlist
  sed -i "s+\(^comm_idup_comm2 .*\)+\1 xfail=ticket0+g" comm/testlist
  sed -i "s+\(^comm_idup_iallreduce .*\)+\1 xfail=ticket0+g" comm/testlist
  sed -i "s+\(^comm_idup_isend .*\)+\1 xfail=ticket0+g" comm/testlist
  sed -i "s+\(^comm_idup_mul .*\)+\1 xfail=ticket0+g" comm/testlist
  sed -i "s+\(^comm_idup_nb .*\)+\1 xfail=ticket0+g" comm/testlist
  sed -i "s+\(^comm_idup_overlap .*\)+\1 xfail=ticket0+g" comm/testlist
  sed -i "s+\(^dynamic_errcode_predefined_errclass .*\)+\1 xfail=ticket0+g" errhan/testlist
  sed -i "s+\(^errstring2 .*\)+\1 xfail=ticket0+g" errhan/testlist
  sed -i "s+\(^noalias2 .*\)+\1 xfail=ticket0+g" errors/coll/testlist
  sed -i "s+\(^noalias3 .*\)+\1 xfail=ticket0+g" errors/coll/testlist
  sed -i "s+\(^too_many_icomms .*\)+\1 xfail=ticket0+g" errors/comm/testlist
  sed -i "s+\(^too_many_icomms2 .*\)+\1 xfail=ticket0+g" errors/comm/testlist
  sed -i "s+\(^file_errhdl .*\)+\1 xfail=ticket0+g" errors/io/testlist
  sed -i "s+\(^cas_type_check .*\)+\1 xfail=ticket0+g" errors/rma/testlist
  sed -i "s+\(^win_sync_lock_fence .*\)+\1 xfail=ticket0+g" errors/rma/testlist
  sed -i "s+\(^unpub .*\)+\1 xfail=ticket0+g" errors/spawn/testlist
  sed -i "s+\(^bottom .*\)+\1 xfail=ticket0+g" f77/datatype/testlist
  sed -i "s+\(^bottom .*\)+\1 xfail=ticket0+g" f90/datatype/testlist
  sed -i "s+\(^createf90types .*\)+\1 xfail=ticket0+g" f90/f90types/testlist
  sed -i "s+\(^infoget .*\)+\1 xfail=ticket0+g" info/testlist
  sed -i "s+\(^bigtype .*\)+\1 xfail=ticket0+g" io/testlist
  sed -i "s+\(^hindexed_io .*\)+\1 xfail=ticket0+g" io/testlist
  sed -i "s+\(^resized2 .*\)+\1 xfail=ticket0+g" io/testlist
  sed -i "s+\(^acc-pairtype .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^accfence1 .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^accpscw1 .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^atomic_get .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^epochtest .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^large-acc-flush_local .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^lock_contention_dt .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^lock_dt .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^lock_dt_flush .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^lock_dt_flushlocal .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^lockall_dt .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^lockall_dt_flush .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^lockall_dt_flushall .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^lockall_dt_flushlocal .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^lockall_dt_flushlocalall .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^putfence1 .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^putpscw1 .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^win_info .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^win_shared_cas_flush_load .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^win_shared_gacc_flush_load .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^win_shared_zerobyte .*\)+\1 xfail=ticket0+g" rma/testlist

  # regression tests added after mpich-3.1
  sed -i "s+\(^manyrma2_shm .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^nullpscw_shm .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^pscw_ordering_shm .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^test2_am_shm .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^test3_am_shm .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^test2_shm .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^test3_shm .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^transpose3_shm .*\)+\1 xfail=ticket0+g" rma/testlist
  sed -i "s+\(^wintest_shm .*\)+\1 xfail=ticket0+g" rma/testlist
fi

# MPI-3.1 version
sed -i "s+\(^version .*\)+\1 xfail=ticket0+g" init/testlist
sed -i "s+\(^baseenvf .*\)+\1 xfail=ticket0+g" f77/init/testlist
sed -i "s+\(^baseenv .*\)+\1 xfail=ticket0+g" cxx/init/testlist
sed -i "s+\(^baseenvf90 .*\)+\1 xfail=ticket0+g" f90/init/testlist

## Remove the MPI library that is not being used for running
if test "$build_mpi" = "mpich-master" -a "$run_mpi" = "mpich-master" ; then
  rm -rf ${IMPI_PATH}

  LD_LIBRARY_PATH=${MPICH_RUN_PATH}/lib:$LD_LIBRARY_PATH
  export LD_LIBRARY_PATH

elif test "$build_mpi" = "mpich-master" -a "$run_mpi" = "mpich-3.1" ; then
  rm -rf ${MPICH_BUILD_PATH}
  rm -rf ${IMPI_PATH}

  cd ${MPICH_RUN_PATH}/lib

  ln -s libmpich.so.12.0.0 libmpi.so.0
  ln -s libmpichcxx.so.12.0.0 libmpicxx.so.0
  ln -s libmpichf90.so.12.0.0 libmpifort.so.0

  LD_LIBRARY_PATH=${MPICH_RUN_PATH}/lib:$LD_LIBRARY_PATH
  export LD_LIBRARY_PATH

  LD_PRELOAD=${MPICH_RUN_PATH}/lib/libmpich.so.12:${MPICH_RUN_PATH}/lib/libmpichf90.so.12
  export LD_PRELOAD

elif test "$build_mpi" = "mpich-master" -a "$run_mpi" = "impi-5.0"; then
  rm -rf ${MPICH_BUILD_PATH}

  cd ${IMPI_PATH}/intel64/lib

  if test "$run_conf" = "debug" ; then
    ln -s debug/libmpi.so.12.0 libmpi.so.0
  else
    ln -s release/libmpi.so.12.0 libmpi.so.0
  fi
  ln -s libmpicxx.so.12.0 libmpicxx.so.0
  ln -s libmpifort.so.12.0 libmpifort.so.0

  LD_LIBRARY_PATH=${IMPI_PATH}/intel64/lib:$LD_LIBRARY_PATH
  export LD_LIBRARY_PATH

  LD_PRELOAD=${IMPI_PATH}/intel64/lib/libmpi.so.12:${IMPI_PATH}/intel64/lib/libmpifort.so.12
  export LD_PRELOAD

elif test "$build_mpi" = "impi-5.0" -a "$run_mpi" = "mpich-master" ; then
  rm -rf ${IMPI_PATH}

  cd ${MPICH_RUN_PATH}/lib

  ln -s libmpi.so.0.0.0 libmpi.so.12
  mkdir release
  ln -s libmpi.so.0.0.0 release/libmpi.so.12
  mkdir debug
  ln -s libmpi.so.0.0.0 debug/libmpi.so.12
  ln -s libmpicxx.so.0.0.0 libmpicxx.so.12
  ln -s libmpifort.so.0.0.0 libmpifort.so.12

  LD_LIBRARY_PATH=${MPICH_RUN_PATH}/lib:$LD_LIBRARY_PATH
  export LD_LIBRARY_PATH

elif test "$build_mpi" = "impi-5.0" -a "$run_mpi" = "mpich-3.1" ; then
  rm -rf ${IMPI_PATH}

  cd ${MPICH_RUN_PATH}/lib

  ln -s libmpich.so.12.0.0 libmpi.so.12
  ln -s libmpichcxx.so.12.0.0 libmpicxx.so.12
  ln -s libmpichf90.so.12.0.0 libmpifort.so.12

  LD_LIBRARY_PATH=${MPICH_RUN_PATH}/lib:$LD_LIBRARY_PATH
  export LD_LIBRARY_PATH

elif test "$build_mpi" = "impi-5.0" -a "$run_mpi" = "impi-5.0" ; then
  LD_LIBRARY_PATH=${IMPI_PATH}/intel64/lib:$LD_LIBRARY_PATH
  export LD_LIBRARY_PATH

elif test "$build_mpi" = "mpich-3.1" -a "$run_mpi" = "mpich-master" ; then
  rm -rf ${IMPI_PATH}
  rm -rf ${MPICH_BUILD_PATH}

  cd ${MPICH_RUN_PATH}/lib

  ln -s libmpi.so.0.0.0 libmpich.so.12
  ln -s libmpicxx.so.0.0.0 libmpichcxx.so.12
  ln -s libmpifort.so.0.0.0 libmpichf90.so.12
  ln -s libmpi.so.0.0.0 libmpl.so.1
  ln -s libmpi.so.0.0.0 libopa.so.1

  LD_LIBRARY_PATH=${MPICH_RUN_PATH}/lib:$LD_LIBRARY_PATH
  export LD_LIBRARY_PATH

  LD_PRELOAD=${MPICH_RUN_PATH}/lib/libmpich.so.12:${MPICH_RUN_PATH}/lib/libmpichf90.so.12
  export LD_PRELOAD

elif test "$build_mpi" = "mpich-3.1" -a "$run_mpi" = "mpich-3.1" ; then
  rm -rf ${IMPI_PATH}

  LD_LIBRARY_PATH=${MPICH_RUN_PATH}/lib:$LD_LIBRARY_PATH
  export LD_LIBRARY_PATH

elif test "$build_mpi" = "mpich-3.1" -a "$run_mpi" = "impi-5.0" ; then
  rm -rf ${MPICH_BUILD_PATH}

  cd ${IMPI_PATH}/intel64/lib

  if test "$run_conf" = "debug" ; then
    ln -s debug/libmpi.so.12.0 libmpich.so.12
  else
    ln -s release/libmpi.so.12.0 libmpich.so.12
  fi
  ln -s libmpicxx.so.12.0 libmpichcxx.so.12
  ln -s libmpifort.so.12.0 libmpichf90.so.12
  ln -s libmpich.so.12 libmpl.so.1
  ln -s libmpich.so.12 libopa.so.1

  LD_LIBRARY_PATH=${IMPI_PATH}/intel64/lib:$LD_LIBRARY_PATH
  export LD_LIBRARY_PATH

  LD_PRELOAD=${IMPI_PATH}/intel64/lib/libmpich.so.12:${IMPI_PATH}/intel64/lib/libmpichf90.so.12
  export LD_PRELOAD

else
  echo "Unknown combination"
  exit 1
fi

## Run tests for MPI version 3.0
cd ${MPICH_TESTS_PATH}
./runtests -mpiversion=3.0 -srcdir=. -tests=testlist -junitfile=summary.junit.xml -tapfile=summary.tap -xmlfile=summary.xml

mv summary.junit.xml ${WORKSPACE}
mv summary.tap ${WORKSPACE}
mv summary.xml ${WORKSPACE}
cd ${WORKSPACE}
rm -rf ${TMP_WORKSPACE}
exit 0

