#!/bin/zsh -xe

WORKSPACE=""
compiler="gnu"
jenkins_configure="default"
queue="ib64"
netmod="default"
ofi_prov="sockets"
N_MAKE_JOBS=8

#####################################################################
## Initialization
#####################################################################

while getopts ":h:c:o:q:m:n:" opt; do
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
        \?)
            echo "Invalid option: -$OPTARG" >&2
            exit 1
    esac
done

cd $WORKSPACE

TMP_WORKSPACE=$(mktemp -d /sandbox/jenkins.tmp.XXXXXXXX)
SRC=$WORKSPACE
TMP_SRC=$TMP_WORKSPACE

tar zxvf mpich-master.tar.gz
SRC=$WORKSPACE/mpich-master
TMP_SRC=$TMP_WORKSPACE/mpich-master
cp -a $WORKSPACE/mpich-master $TMP_WORKSPACE/


#####################################################################
## Functions
#####################################################################

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
            config_opt="--disable-f77 --disable-fc"
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

rm -rf $HOME/mpi/installs/mpich-master/$compiler/$jenkins_configure

PrepareEnv

SetCompiler "$compiler"

pushd "$TMP_SRC"

./configure --prefix=$HOME/mpi/installs/mpich-master/$compiler/$jenkins_configure $(SetConfigOpt $jenkins_configure)
make -j8 install

popd

if test "$compiler" = "gnu" -a "$jenkins_configure" = "default" ; then
  cd /home/autotest/mpi/tests/mpich-tests
  git clean -x -d -f
  git pull
  ./autogen.sh
fi

exit 0
