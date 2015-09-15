#!/bin/zsh -xe

WORKSPACE=""
compiler="gnu"
jenkins_configure="default"
queue="ib64"
netmod="default"
ofi_prov="sockets"
N_MAKE_JOBS=8
GIT_BRANCH=""
BUILD_MODE="per-commit"

#####################################################################
## Initialization
#####################################################################

while getopts ":h:c:o:q:m:n:b:" opt; do
    case "$opt" in
        h)
            WORKSPACE=$OPTARG ;;
        c)
            compiler=$OPTARG ;;
        o)
            jenkins_configure=$OPTARG ;;
        q)
            queue=$OPTARG ;;
        m)
            _netmod=${OPTARG%%,*}
            if [[ "$_netmod" == "ofi" ]]; then
                netmod=$_netmod
                ofi_prov=${OPTARG/$_netmod,}
            else
                netmod=$_netmod
            fi
            ;;
        n)
            N_MAKE_JOBS=$OPTARG ;;
        b)
            GIT_BRANCH=$OPTARG ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
    esac
done

cd $WORKSPACE

if test "$GIT_BRANCH" = "" ; then
    BUILD_MODE="nightly"
fi

case "$BUILD_MODE" in
    "nightly")
        if [[ -x mpich-master/maint/jenkins/skip_test.sh ]]; then
            ./mpich-master/maint/jenkins/skip_test.sh -j $JOB_NAME -c $compiler -o $jenkins_configure -q $queue -m $_netmod \
                -s mpich-master/test/mpi/summary.junit.xml
            if [[ -f mpich-master/test/mpi/summary.junit.xml ]]; then
                exit 0
            fi
        fi
        ;;
    "per-commit")
        if [[ -x maint/jenkins/skip_test.sh ]]; then
            ./maint/jenkins/skip_test.sh -j $JOB_NAME -c $compiler -o $jenkins_configure -q $queue -m $_netmod \
                -s test/mpi/summary.junit.xml
            if [[ -f test/mpi/summary.junit.xml ]]; then
                exit 0
            fi
        fi
        ;;
esac


TMP_WORKSPACE=$(mktemp -d /sandbox/jenkins.tmp.XXXXXXXX)
SRC=$WORKSPACE
TMP_SRC=$TMP_WORKSPACE

# Preparing the source
case "$BUILD_MODE" in
    "nightly")
        tar zxvf mpich-master.tar.gz
        SRC=$WORKSPACE/mpich-master
        TMP_SRC=$TMP_WORKSPACE/mpich-master
        cp -a $WORKSPACE/mpich-master $TMP_WORKSPACE/
        ;;
    "per-commit")
        git clean -x -d -f
        cp -a $WORKSPACE/* $TMP_WORKSPACE/
        ;;
    *)
        echo "Invalid BUILD_MODE $BUILD_MODE. Set by mistake?"
        exit 1
esac


#####################################################################
## Functions
#####################################################################

CollectResults() {
    # TODO: copy saved test binaries (for failed cases)
    find . \
        \( -name "filtered-make.txt" -o \
        -name "apply-xfail.sh" -o \
        -name "autogen.log" -o \
        -name "config.log" -o \
        -name "c.txt" -o \
        -name "m.txt" -o \
        -name "mi.txt" -o \
        -name "summary.junit.xml" \) \
        -exec cp {} $SRC/{} \;
}

#####################################################################
## Logic to generate random configure options
#####################################################################
RandArgs() {
    # Chosen *without* replacement.  If an option is chosen twice,
    # then there will be fewer options
    n_choice=$1
    array=(${(P)${2}})
    optname=$3
    negoptname=$4
    chosen=()
    args=""
    ret_args=""
    array_len=$#array
    idx=0

    for i in `seq $array_len`; do
        chosen[$i]=0
    done

    for i in `seq $n_choice`; do
        let idx=$[RANDOM % $array_len]+1
        if [ $chosen[$idx] -eq 1 ]; then continue; fi
        chosen[$idx]=1
        args=("${(s/;/)array[$idx]}")
        name=$args[1]
        if [ $#args -eq 1 ]; then
            # Only the name is provided.  Choose one of three
            # choices:
            #    No option (skip this one)
            #    just --$optname-$name
            #    just --$negoptname-$name
            let idx=$[RANDOM % 3]+1
            if [ $idx -eq 1 ]; then
                ret_args="$ret_args --$optname-$name"
            elif [ $idx -eq 2 ]; then
                ret_args="$ret_args --$negoptname-$name"
            fi
        else
            let idx=$[RANDOM % ($#args-1)]+2
            # Special cases
            if [ "$args[$idx]" = "ch3:sock" ]; then
                ret_args="$ret_args --disable-ft-tests --disable-comm-overlap-tests"
            elif [ "$args[$idx]" = "gforker" ]; then
                if [ $chosen[4] -eq 1 ]; then
                    continue
                else
                    ret_args="$ret_args --with-namepublisher=file"
                    chosen[4]=1
                fi
            elif [ "$name" = "namepublisher" -a "$args[$idx]" = "no" ]; then
                if [ $chosen[3] -eq 1 ]; then
                    continue
                fi
            elif [ "$args[$idx]" = "ndebug" -a "$CC" = "suncc" -a "$label" = "ubuntu32" ]; then
                # On ubuntu32, suncc has a bug whose workaround is to add -O flag (ticket #2105)
                CFLAGS="-O1"
                export CFLAGS
            fi
            ret_args="$ret_args --$optname-$name=$args[$idx]"
        fi
    done
    echo $ret_args
}

RandConfig() {
    # WARNING: If moving anything in the two following arrays, check the indices in "Special cases" above
    enable_array=(
        'error-checking;no;runtime;all'
        'error-messages;all;generic;class;none'
        'timer-type;linux86_cycle;clock_gettime;gettimeofday'
        'timing;none;all;runtime;log;log_detailed'
        'g;none;all;handle;dbg;log;meminit;handlealloc;instr;mem;mutex;mutexnesting'
        'fast;O0;O1;O2;O3;ndebug;all;yes;none'
        'fortran'
        'cxx'
        'romio'
        'check-compiler-flags'
        'strict;c99;posix'
        'debuginfo'
        'weak-symbols;no;yes'
        'threads;single;multiple;runtime'
        'thread-cs;global'
        'refcount;lock-free;none'
        'mutex-timing'
        'handle-allocation;tls;mutex'
        'multi-aliases'
        'predefined-refcount'
        'alloca'
        'yield;sched_yield;select'
        'runtimevalues'
    )
    with_array=(
        'logging;none'
        'pmi;simple'
        'pm;gforker'
        'namepublisher;no;file'
        'device;ch3;ch3:sock'
    )
    let n_enable=$#enable_array+1
    let n_with=$#with_array+1
    enable_args=$(RandArgs $n_enable "enable_array" "enable" "disable")
    with_args=$(RandArgs $n_with "with_array" "with" "without")
    echo "$enable_args $with_args"
}

PrepareEnv() {
    case "$queue" in
        "ubuntu32" | "ubuntu64" )
            source /software/common/adm/etc/softenv-aliases.sh
            source /software/common/adm/etc/softenv-load.sh
            soft add +intel
            soft add +pgi
            soft add +absoft
            soft add +nagfor
            soft add +solarisstudio-12.4
            soft add +ekopath
            ;;
        "freebsd64" | "freebsd32")
            PATH=/usr/local/bin:$PATH
            LD_LIBRARY_PATH=/usr/local/lib/gcc46:$LD_LIBRARY_PATH
            export LD_LIBRARY_PATH
            ;;
        "solaris")
            PATH=/usr/gcc/4.3/bin:/usr/gnu/bin:/usr/sbin:$PATH
            CONFIG_SHELL=/usr/gnu/bin/sh
            export CONFIG_SHELL
            ;;
    esac
    PATH=$HOME/software/autotools/bin:$PATH
    export PATH
    echo "$PATH"
}

SetCompiler() {
    case "$compiler" in
        "gnu")
            CC=gcc
            CXX=g++
            F77=gfortran
            FC=gfortran
            ;;
        "clang")
            CC=clang
            CXX=clang++
            F77=gfortran
            FC=gfortran
            ;;
        "intel")
            CC=icc
            CXX=icpc
            F77=ifort
            FC=ifort
            ;;
        "pgi")
            CC=pgcc
            CXX=pgcpp
            F77=pgf77
            FC=pgfortran
            ;;
        "absoft")
            CC=gcc
            CXX=g++
            F77=af77
            FC=af90
            ;;
        "nag")
            CC=gcc
            CXX=g++
            F77=nagfor
            FC=nagfor
            FFLAGS="-mismatch"
            FCFLAGS="-mismatch"
            export FFLAGS
            export FCFLAGS
            ;;
        "solstudio")
            CC=suncc
            CXX=sunCC
            F77=sunf77
            FC=sunf90
            ;;
        "sunstudio")
            CC=cc
            CXX=CC
            F77=f77
            FC=f90
            ;;
        "pathscale")
            CC=pathcc
            CXX=pathCC
            F77=pathf95
            FC=pathf95
            ;;
        *)
            echo "Unknown compiler suite"
            exit 1
    esac

    export CC
    export CXX
    export F77
    export FC

    which $CC
    which $CXX
    which $F77
    which $FC
}

SetNetmod() {
    netmod_opt="__NULL__"
    case "$netmod" in
        "default") # for solaris, may use with sock
            netmod_opt=
            ;;
        "mxm")
            netmod_opt="--with-device=ch3:nemesis:mxm --with-mxm=$HOME/software/mellanox/mxm --disable-spawn --disable-ft-tests"
            ;;
        "ofi")
            netmod_opt="--with-device=ch3:nemesis:ofi --with-ofi=$HOME/software/libfabric/$ofi_prov --disable-spawn --disable-ft-tests LD_LIBRARY_PATH=$HOME/software/libfabric/lib"
            ;;
        "portals4")
            netmod_opt="--with-device=ch3:nemesis:portals4 --with-portals4=$HOME/software/portals4 --disable-spawn --disable-ft-tests"
            ;;
        "sock")
            netmod_opt="--with-device=ch3:sock --disable-ft-tests --disable-comm-overlap-tests"
            ;;
        "tcp")
            netmod_opt=
            ;;
        *)
            echo "Unknown netmod type"
            exit 1
    esac
    export netmod_opt
    echo "$netmod_opt"
}

SetConfigOpt() {
    config_opt="__TO_BE_FILLED__"
    case "$jenkins_configure" in
        "default")
            config_opt=
            ;;
        "strict")
            config_opt="--enable-strict"
            ;;
        "fast")
            config_opt="--enable-fast=all"
            ;;
        "nofast")
            config_opt="--disable-fast"
            ;;
        "noshared")
            config_opt="--disable-shared"
            ;;
        "debug")
            config_opt="--enable-g=all"
            ;;
        "noweak")
            config_opt="--disable-weak-symbols"
            ;;
        "strictnoweak")
            config_opt="--enable-strict --disable-weak-symbols"
            ;;
        "nofortran")
            config_opt="--disable-fortran"
            ;;
        "nocxx")
            config_opt="--disable-cxx"
            ;;
        "multithread")
            config_opt="--enable-threads=multiple"
            ;;
        "debuginfo")
            config_opt="--enable-debuginfo"
            ;;
        "noerrorchecking")
            config_opt="--disable-error-checking"
            ;;
        "sock") # for solaris + sock
            config_opt="--with-device=ch3:sock --disable-ft-tests --disable-comm-overlap-tests"
            ;;
        "mpd")
            config_opt="--with-pm=mpd --with-namepublisher=file"
            ;;
        "gforker")
            config_opt="--with-pm=gforker --with-namepublisher=file"
            ;;
        "shmem")
            config_opt=
            ;;
        "async")
            config_opt=
            ;;
        "random")
            config_opt=$(RandArgs)
            ;;
        *)
            echo "Bad configure option: $jenkins_configure"
            exit 1
    esac

    if test "$jenkins_configure" != "shmem" ; then
      config_opt="$config_opt --enable-nemesis-dbg-localoddeven"
    fi

    if test "$queue" = "osx" -a "$FC" = "ifort"; then
        config_opt="$config_opt lv_cv_ld_force_load=no"
    fi

    export config_opt
    echo "$config_opt"
}

#####################################################################
## Main() { Setup Environment and Build
#####################################################################
# determine if this is a nightly job or a per-commit job
PrepareEnv

SetCompiler "$compiler"

pushd "$TMP_SRC"

if test "$BUILD_MODE" = "per-commit" ; then
    ./autogen.sh 2>&1 | tee autogen.log
fi

if [[ -x maint/jenkins/set-xfail.sh ]]; then
    ./maint/jenkins/set-xfail.sh -j $JOB_NAME -c $compiler -o $jenkins_configure -q $queue -m $_netmod
fi

./configure --prefix="$TMP_SRC/_inst" $(SetNetmod $netmod) $(SetConfigOpt $jenkins_configure) \
    --disable-perftest \
    2>&1 | tee c.txt
make -j$N_MAKE_JOBS 2>&1 | tee m.txt
if test "${pipestatus[-2]}" != "0"; then
    CollectResults
    exit 1
fi
make -j$N_MAKE_JOBS install 2>&1 | tee mi.txt
if test "${pipestatus[-2]}" != "0"; then
    CollectResults
    exit 1
fi
cat m.txt mi.txt | ./maint/clmake > filtered-make.txt 2>&1

#####################################################################
## Run tests
#####################################################################

# Preparation
case "$jenkins_configure" in
    "mpd")
        $TMP_SRC/_inst/bin/mpd &
        sleep 1
        ;;
    "async")
        MPIR_CVAR_ASYNC_PROGRESS=1
        export MPIR_CVAR_ASYNC_PROGRESS
        ;;
    "multithread")
        MPIR_CVAR_DEFAULT_THREAD_LEVEL=MPI_THREAD_MULTIPLE
        export MPIR_CVAR_DEFAULT_THREAD_LEVEL
        ;;
esac

case "$netmod" in
    "mxm")
        MXM_LOG_LEVEL=error
        export MXM_LOG_LEVEL
        MXM_SHM_KCOPY_MODE=off
        export MXM_SHM_KCOPY_MODE
        ;;
    "ofi" | "portals4")
        MXM_LOG_LEVEL=error
        export MXM_LOG_LEVEL
        ;;
esac

# run only the minimum level of datatype tests when it is per-commit job
if [[ "$BUILD_MODE" = "per-commit" ]]; then
    MPITEST_DATATYPE_TEST_LEVEL=min
    export MPITEST_DATATYPE_TEST_LEVEL
fi

make testing

# Cleanup
case "$jenkins_configure" in
    "mpd")
        $TMP_SRC/_inst/bin/mpdallexit
        ;;
    "async")
        unset MPIR_CVAR_ASYNC_PROGRESS
        ;;
esac

if killall -9 hydra_pmi_proxy; then
  echo "leftover hydra_pmi_proxy processes killed"
fi

#####################################################################
## Copy Test results and Cleanup
#####################################################################

CollectResults

popd
rm -rf $TMP_WORKSPACE
exit 0

