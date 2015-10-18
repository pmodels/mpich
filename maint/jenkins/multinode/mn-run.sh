#!/bin/zsh -xe
#
# (C) 2015 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

# This script only run on the first node. The mpiexec will automatically spawned
if test ! $SLURM_NODEID -eq 0; then
    exit 0
fi

hostname

WORKSPACE=""
TMP_WORKSPACE=""
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

while getopts ":h:c:o:q:m:n:b:t:" opt; do
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
        t)
            TMP_WORKSPACE=$OPTARG ;;
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


if test -d "$TMP_WORKSPACE"; then
    rm -rf "$TMP_WORKSPACE"
fi
SRC=$WORKSPACE
TMP_SRC=$TMP_WORKSPACE

# Preparing the source
case "$BUILD_MODE" in
    "nightly")
        SRC=$WORKSPACE/mpich-master
        TMP_SRC=$TMP_WORKSPACE/mpich-master
        ;;
    "per-commit")
        ;;
    *)
        echo "Invalid BUILD_MODE $BUILD_MODE. Set by mistake?"
        exit 1
esac

srun mkdir -p $TMP_SRC

# distribute source and binary
pushd $WORKSPACE
srun cp $WORKSPACE/mpich-test-pack.tar $TMP_SRC
srun tar xf $TMP_SRC/mpich-test-pack.tar -C $TMP_SRC
popd


#####################################################################
## Functions
#####################################################################

CollectResults() {
    # TODO: copy saved test binaries (for failed cases)
    if [[ "$BUILD_MODE" != "per-commit" ]]; then
        find . \
            \( -name "filtered-make.txt" \
            -o -name "apply-xfail.sh" \
            -o -name "autogen.log" \
            -o -name "config.log" \
            -o -name "c.txt" \
            -o -name "m.txt" \
            -o -name "mi.txt" \
            -o -name "summary.junit.xml" \) \
            | while read -r line; do
                mkdir -p "$SRC/$(dirname $line)"
            done
    fi

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


#####################################################################
## Main() { Run tests
## Multi-node tests are running from Jenkins workspace
#####################################################################

pushd "$TMP_SRC"

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

# hack for passing -iface ib0
if test "$netmod" = "tcp"; then
    sed -i 's+/bin/mpiexec+/bin/mpiexec -iface ib0+g' test/mpi/runtests
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

#####################################################################
## Copy Test results and Cleanup
#####################################################################

CollectResults

popd
srun rm -rf $TMP_WORKSPACE
exit 0

