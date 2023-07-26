#! /bin/sh
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

# Update all of the derived files
# For best performance, execute this in the top-level directory.
# There are some experimental features to allow it to be executed in
# subdirectories
#
# Eventually, we want to allow this script to be executed anywhere in the
# mpich tree.  This is not yet implemented.


warn() {
    echo "===> WARNING: $@"
}

error() {
    echo "===> ERROR:   $@"
}

echo_n() {
    # "echo -n" isn't portable, must portably implement with printf
    printf "%s" "$*"
}

########################################################################
## Checks to make sure we are running from the correct location
########################################################################

echo_n "Verifying the location of autogen.sh... "
if [ ! -d maint -o ! -s maint/version.m4 ] ; then
    echo "must execute at top level directory for now"
    exit 1
fi
# Set the SRCROOTDIR to be used later and avoid "cd ../../"-like usage.
SRCROOTDIR=$PWD
echo "done"

########################################################################
## Initialize variables to default values (possibly from the environment)
########################################################################

# Default choices
do_fortran=yes  # if no, skips patching libtool for Fortran
do_errmsgs=yes
do_getcvars=yes
do_f77=yes
do_f90=no  # we generate f90 binding in configure
do_f08=yes
do_cxx=yes
do_build_configure=yes
do_atdir_check=no
do_atver_check=yes
do_subcfg_m4=yes
do_hwloc=yes
do_ofi=yes
do_ucx=yes
do_json=yes
do_yaksa=yes
do_test=yes
do_hydra=yes
do_romio=yes
do_pmi=yes
do_doc=no

yaksa_depth=

do_quick=no
# Check -quick option. When enabled, skip as much as we can.
for arg in "$@" ; do
    if test $arg = "-quick"; then
        do_quick=yes
        do_hwloc=no
        do_json=no
        do_ofi=no
        do_ucx=no
        do_yaksa=no
        do_test=yes
        do_hydra=yes
        do_romio=no

        if test -e 'modules.tar.gz' -a ! -e 'modules/PREBUILT' ; then
            echo_n "Untaring modules.tar.gz... "
            tar xf modules.tar.gz
            echo "done"
        fi
    fi
done

# Allow MAKE to be set from the environment
MAKE=${MAKE-make}

# amdirs are the directories that make use of autoreconf
amdirs=". src/mpl src/pmi"

autoreconf_args="-if"
export autoreconf_args

########################################################################
## Utility functions
########################################################################

recreate_tmp() {
    rm -rf .tmp
    mkdir .tmp 2>&1 >/dev/null
}

# Assume Program's install-dir is <install-dir>/bin/<prog>.
# Given program name as the 1st argument,
# the install-dir is returned is returned in 2nd argument.
# e.g., ProgHomeDir libtoolize libtooldir.
ProgHomeDir() {
    prog=$1
    progpath="`which $prog`"
    progbindir="`dirname $progpath`"
    proghome=`(cd $progbindir/.. && pwd)`
    eval $2=$proghome
}

# checking and patching submodules
check_submodule_presence() {
    if test ! -f "$SRCROOTDIR/$1/configure.ac"; then
        error "Submodule $1 is not checked out."
        error "if you just git cloned this repository, run"
        error "    git submodule update --init"
        error "to checkout the submodules."
        exit 1
    fi
}

########################################################################
## prerequisite functions
########################################################################

autoconf=
set_autotools() {
    if test -z "$autoconf" ; then
        if [ -n "$autotoolsdir" ] ; then
            if [ -x $autotoolsdir/autoconf -a -x $autotoolsdir/autoheader ] ; then
                autoconf=$autotoolsdir/autoconf
                autoheader=$autotoolsdir/autoheader
                autoreconf=$autotoolsdir/autoreconf
                automake=$autotoolsdir/automake
                autom4te=$autotoolsdir/autom4te
                aclocal=$autotoolsdir/aclocal
                if [ -x "$autotoolsdir/glibtoolize" ] ; then
                    libtoolize=$autotoolsdir/glibtoolize
                else
                    libtoolize=$autotoolsdir/libtoolize
                fi

                AUTOCONF=$autoconf
                AUTOHEADER=$autoheader
                AUTORECONF=$autoreconf
                AUTOMAKE=$automake
                AUTOM4TE=$autom4te
                ACLOCAL=$aclocal
                LIBTOOLIZE=$libtoolize

                export AUTOCONF
                export AUTOHEADER
                export AUTORECONF
                export AUTOM4TE
                export AUTOMAKE
                export ACLOCAL
                export LIBTOOLIZE
            else
                echo "could not find executable autoconf and autoheader in $autotoolsdir"
                exit 1
            fi
        else
            autoconf=${AUTOCONF:-autoconf}
            autoheader=${AUTOHEADER:-autoheader}
            autoreconf=${AUTORECONF:-autoreconf}
            autom4te=${AUTOM4TE:-autom4te}
            automake=${AUTOMAKE:-automake}
            aclocal=${ACLOCAL:-aclocal}
            if test -z "${LIBTOOLIZE+set}" && ( glibtoolize --version ) >/dev/null 2>&1 ; then
                libtoolize=glibtoolize
            else
                libtoolize=${LIBTOOLIZE:-libtoolize}
            fi
        fi
    fi
}

externals=
set_externals() {
    if test -z "$externals" ; then
        #TODO: if necessary, run: git submodule update --init

        # external packages that require autogen.sh to be run for each of them
        externals="test/mpi"

        if [ "yes" = "$do_hydra" ] ; then
            externals="${externals} src/pm/hydra"
        fi

        if [ "yes" = "$do_romio" ] ; then
            externals="${externals} src/mpi/romio"
        fi

        if [ "yes" = "$do_pmi" ] ; then
            externals="${externals} src/pmi"
        fi

        if [ "yes" = "$do_hwloc" ] ; then
            check_submodule_presence modules/hwloc
            externals="${externals} modules/hwloc"
        fi

        if [ "yes" = "$do_ucx" ] ; then
            check_submodule_presence modules/ucx
            externals="${externals} modules/ucx"
        fi

        if [ "yes" = "$do_ofi" ] ; then
            check_submodule_presence modules/libfabric
            externals="${externals} modules/libfabric"
        fi

        if [ "yes" = "$do_json" ] ; then
            check_submodule_presence "modules/json-c"
            externals="${externals} modules/json-c"
        fi

        if [ "yes" = "$do_yaksa" ] ; then
            check_submodule_presence "modules/yaksa"
            externals="${externals} modules/yaksa"
        fi
    fi
}

check_python3() {
    echo_n "Checking for Python 3... "
    PYTHON=
    if test 3 = `python -c 'import sys; print(sys.version_info[0])' 2> /dev/null || echo "0"`; then
        PYTHON=python
    fi

    if test -z "$PYTHON" -a 3 = `python3 -c 'import sys; print(sys.version_info[0])' 2> /dev/null || echo "0"`; then
        PYTHON=python3
    fi

    if test -z "$PYTHON" ; then
        echo "not found"
        exit 1
    else
        echo "$PYTHON"
    fi
}

set_PYTHON() {
    if test -z "$PYTHON" ; then
        check_python3
    fi
}

########################################################################
## Step functions
########################################################################

sync_external () {
    srcdir=$1
    destdir=$2

    echo "syncing '$srcdir' --> '$destdir'"

    # deletion prevents creating 'confdb/confdb' situation, also cleans
    # stray files that may have crept in somehow
    rm -rf "$destdir"
    cp -pPR "$srcdir" "$destdir"
}

fn_copy_confdb_etc() {
    # This used to be an optionally installed hook to help with git-svn
    # versions of the old SVN repo.  Now that we are using git, this is our
    # mechanism that replaces relative svn:externals paths, such as for
    # "confdb" and "mpl". The basic plan is to delete the destdir and then
    # copy all of the files, warts and all, from the source directory to the
    # destination directory.
    echo
    echo "####################################"
    echo "## Replicating confdb (and similar)"
    echo "####################################"
    echo

    confdb_dirs=
    confdb_dirs="${confdb_dirs} src/mpl/confdb"
    if test "$do_pmi" = "yes" ; then
        confdb_dirs="${confdb_dirs} src/pmi/confdb"
        if test "$do_quick" = "no" ; then
            sync_external src/mpl src/pmi/mpl
            confdb_dirs="${confdb_dirs} src/pmi/mpl/confdb"
        fi
    fi
    if test "$do_romio" = "yes" ; then
        confdb_dirs="${confdb_dirs} src/mpi/romio/confdb"
        if test "$do_quick" = "no" ; then
            sync_external src/mpl src/mpi/romio/mpl
            confdb_dirs="${confdb_dirs} src/mpi/romio/mpl/confdb"
        fi
    fi
    if test "$do_hydra" = "yes" ; then
        confdb_dirs="${confdb_dirs} src/pm/hydra/confdb"
        if test "$do_quick" = "no" ; then
            mkdir -p src/pm/hydra/modules
            sync_external src/mpl src/pm/hydra/modules/mpl
            sync_external src/pmi src/pm/hydra/modules/pmi
            sync_external modules/hwloc src/pm/hydra/modules/hwloc
            # remove .git directories to avoid confusing git clean
            rm -rf src/pm/hydra/modules/hwloc/.git
            confdb_dirs="${confdb_dirs} src/pm/hydra/modules/mpl/confdb"
            confdb_dirs="${confdb_dirs} src/pm/hydra/modules/pmi/confdb"
            confdb_dirs="${confdb_dirs} src/pm/hydra/modules/pmi/mpl/confdb"
        fi
    fi
    if test "$do_test" = "yes" ; then
        confdb_dirs="${confdb_dirs} test/mpi/confdb"
        confdb_dirs="${confdb_dirs} test/mpi/dtpools/confdb"
    fi

    # all the confdb directories, by various names
    for destdir in $confdb_dirs ; do
        sync_external confdb "$destdir"
    done

    # a couple of other random files
    if [ -f maint/version.m4 ] ; then
        cp -pPR maint/version.m4 src/pm/hydra/version.m4
        cp -pPR maint/version.m4 src/mpi/romio/version.m4
        cp -pPR maint/version.m4 src/pmi/version.m4
        cp -pPR maint/version.m4 test/mpi/version.m4
    fi

    # Now sanity check that some of the above sync was successful
    f="aclocal_cc.m4"
    for d in $confdb_dirs ; do
        if [ -f "$d/$f" ] ; then :
        else
            error "expected to find '$f' in '$d'"
            exit 1
        fi
    done
}

fn_maint_configure() {
    set_autotools
    echo
    echo "------------------------------------"
    echo "Initiating building required scripts"
    (cd maint && $autoconf && rm -rf autom4te*.cache && ./configure)
    echo "Done building required scripts"
    echo "------------------------------------"
    echo
}

fn_errmsgs() {
    if test ! -x maint/extracterrmsgs ; then
        fn_maint_configure
    fi

    echo_n "Extracting error messages... "
    rm -rf .tmp
    rm -f .err
    rm -f unusederr.txt
    maint/extracterrmsgs -careful=unusederr.txt \
        -skip=src/util/multichannel/mpi.c `cat maint/errmsgdirs` > \
        .tmp 2>.err
    # (error here is ok)
    echo "done"

    update_errdefs=yes
    if [ -s .err ] ; then
        cat .err
        rm -f .err2
        grep -v "Warning:" .err > .err2
        if [ -s .err2 ] ; then
            warn "Because of errors in extracting error messages, the file"
            warn "src/mpi/errhan/defmsg.h was not updated."
            error "Error message files in src/mpi/errhan were not updated."
            rm -f .tmp .err .err2
            exit 1
        fi
        rm -f .err .err2
    else
        # In case it exists but has zero size
        rm -f .err
    fi
    if [ -s unusederr.txt ] ; then
        warn "There are unused error message texts in src/mpi/errhan/errnames.txt"
        warn "See the file unusederr.txt for the complete list"
    fi
    if [ -s .tmp -a "$update_errdefs" = "yes" ] ; then
        mv .tmp src/mpi/errhan/defmsg.h
    fi
    if [ ! -s src/mpi/errhan/defmsg.h ] ; then
        error "Unable to extract error messages"
        exit 1
    fi
}

fn_getcvars() {
    echo_n "Extracting control variables (cvar) ... "
    if test ! -x maint/extractcvars ; then
        fn_maint_configure
    fi

    if ./maint/extractcvars ; then
        echo "done"
    else
        echo "failed"
        error "unable to extract control variables"
        exit 1
    fi
}

fn_maint_version() {
    set_autotools
    # build a substitute maint/Version script now that we store the single copy of
    # this information in an m4 file for autoconf's benefit
    echo_n "Generating a helper maint/Version... "
    if $autom4te -l M4sugar maint/Version.base.m4 > maint/Version ; then
        echo "done"
    else
        echo "error"
        error "unable to correctly generate maint/Version shell helper"
    fi
}

fn_update_README() {
    if test ! -f ./maint/Version ; then
        fn_maint_version
    fi

    echo_n "Updating the README... "

    # import MPICH_VERSION and LIBFABRIC_VERSION
    . ./maint/Version

    if [ -f README.vin ] ; then
        sed -e "s/%VERSION%/${MPICH_VERSION}/g" -e "s/%LIBFABRIC_VERSION%/${LIBFABRIC_VERSION}/g" README.vin > README
        echo "done"
    else
        echo "error"
        error "README.vin file not present, unable to update README version number (perhaps we are running in a release tarball source tree?)"
    fi
}

fn_gen_subcfg_m4() {
    echo_n "Creating subsys_include.m4... "
    ./maint/gen_subcfg_m4
    echo "done"
}

fn_romio_glue() {
    echo_n "Building ROMIO glue code... "
    ( cd src/glue/romio && chmod a+x ./all_romio_symbols && ./all_romio_symbols ../../mpi/romio/include/mpio.h.in )
    echo "done"
}

fn_f77() {
    set_PYTHON
    echo_n "Building Fortran 77 interface... "
    ( cd src/binding/fortran/mpif_h && chmod a+x ./buildiface && ./buildiface )
    $PYTHON maint/gen_binding_f77.py
    echo "done"
}

fn_f90() {
    echo_n "Building Fortran 90 interface... "
    $PYTHON maint/gen_binding_f90.py
    echo "done"
}

fn_f08() {
    echo_n "Building Fortran 08 interface... "
    # Top-level files
    ( cd src/binding/fortran/use_mpi_f08 && chmod a+x ./buildiface && ./buildiface )
    # in configure: $PYTHON maint/gen_binding_f08.py [options]
    echo "done"
}

fn_cxx() {
    echo_n "Building C++ interface... "
    ( cd src/binding/cxx && chmod a+x ./buildiface &&
        ./buildiface -nosep -initfile=./cxx.vlist )
    echo "done"
}

fn_ch4_api() {
    set_PYTHON
    echo_n "generating ch4 API boilerplates... "
    $PYTHON ./maint/gen_ch4_api.py
}

fn_gen_coll() {
    set_PYTHON
    echo_n "generating Collective functions..."
    $PYTHON maint/gen_coll.py
    echo "done"
}

fn_gen_binding_c() {
    set_PYTHON
    echo_n "generating MPI C functions..."
    _opt=
    if test "$do_doc" = "yes"; then
        _opt="$_opt -output-mansrc"
    fi
    $PYTHON maint/gen_binding_c.py $_opt
    echo "done"
}

fn_json_gen() {
    echo_n "generating json char arrays... "
    ./maint/tuning/coll/json_gen.sh
    echo "done"
}

# internal
_patch_libtool() {
    _file=$1
    _patch=$2
    if test -f "$_file" ; then
        echo_n "Patching $_file with $_patch..."
        patch -N -s -l $_file maint/patches/optional/confdb/$_patch && :
        if [ $? -eq 0 ] ; then
            # Remove possible leftovers, which don't imply a failure
            rm -f $_file.orig
            # updates _patch_successful for return
            _patch_successful=yes
            echo "done"
        else
            echo "failed"
        fi
    fi
}

autoreconf_amdir() {
    _dir=$1
    if [ -d "$_dir" -o -L "$_dir" ] ; then
        echo "------------------------------------------------------------------------"
        echo "running $autoreconf in $_dir"
        (cd $_dir && $autoreconf $autoreconf_args) || exit 1

        # Patching ltmain.sh and libtool.m4
        # This works with libtool versions 2.4 - 2.4.2.
        # Older versions are not supported to build mpich.
        # Newer versions should have this patch already included.

        _patch_successful=no

        _patch_libtool $_dir/confdb/ltmain.sh  intel-compiler.patch
        _patch_libtool $_dir/confdb/libtool.m4 sys_lib_dlsearch_path_spec.patch
        _patch_libtool $_dir/confdb/libtool.m4 big-sur.patch
        _patch_libtool $_dir/confdb/libtool.m4 hpc-sdk.patch
        if test "$do_fortran" = "yes" ; then
            _patch_libtool $_dir/confdb/libtool.m4 darwin-ifort.patch
            _patch_libtool $_dir/confdb/libtool.m4 oracle-fort.patch
            _patch_libtool $_dir/confdb/libtool.m4 flang.patch
            _patch_libtool $_dir/confdb/libtool.m4 arm-compiler.patch
            _patch_libtool $_dir/confdb/libtool.m4 ibm-xlf.patch
        fi

        if test "$_patch_successful" = "yes" ; then
            (cd $_dir && $autoconf -f) || exit 1
            # Reset timestamps to avoid confusing make
            touch -r $_dir/confdb/ltversion.m4 $_dir/confdb/ltmain.sh $_dir/confdb/libtool.m4
        fi
    fi
}

autogen_external() {
    _dir=$1
    if [ -d "$_dir" -o -L "$_dir" ] ; then
        echo "------------------------------------------------------------------------"
        echo "running third-party initialization in $_dir"
        if test "$_dir" = "modules/yaksa" -a -n "$yaksa_depth" ; then
            (cd $_dir && ./autogen.sh --pup-max-nesting=$yaksa_depth) || exit 1
        else
            (cd $_dir && ./autogen.sh) || exit 1
        fi
    else
        error "external directory $_dir missing"
        exit 1
    fi
}

fn_build_configure() {
    set_autotools
    if [ "$do_build_configure" = "yes" ] ; then
        set_externals
        for external in $externals ; do
            autogen_external $external
        done

        for amdir in $amdirs ; do
            autoreconf_amdir $amdir
        done
    fi
}

########################################################################
## functions for checking prerequisites
########################################################################

fn_check_autotools() {
    set_autotools
    ProgHomeDir $autoconf   autoconfdir
    ProgHomeDir $automake   automakedir
    ProgHomeDir $libtoolize libtooldir

    echo_n "Checking if autotools are in the same location... "
    if [ "$autoconfdir" = "$automakedir" -a "$autoconfdir" = "$libtooldir" ] ; then
        same_atdir=yes
        echo "yes, all in $autoconfdir"
    else
        same_atdir=no
        echo "no"
        echo "	autoconf is in $autoconfdir"
        echo "	automake is in $automakedir"
        echo "	libtool  is in $libtooldir"
        # Emit a big warning message if $same_atdir = no.
        warn "Autotools are in different locations. In rare occasion,"
        warn "resulting configure or makefile may fail in some unexpected ways."
    fi

    ########################################################################
    ## Check if autoreconf can be patched to work
    ## when autotools are not in the same location.
    ## This test needs to be done before individual tests of autotools
    ########################################################################

    # If autotools are not in the same location, override autoreconf appropriately.
    if [ "$same_atdir" != "yes" ] ; then
        if [ -z "$libtooldir" ] ; then
            ProgHomeDir $libtoolize libtooldir
        fi
        libtoolm4dir="$libtooldir/share/aclocal"
        echo_n "Checking if $autoreconf accepts -I $libtoolm4dir... "
        new_autoreconf_works=no
        if [ -d "$libtoolm4dir" -a -f "$libtoolm4dir/libtool.m4" ] ; then
            recreate_tmp
            cat >.tmp/configure.ac <<_EOF
AC_INIT(foo,1.0)
AC_PROG_LIBTOOL
AC_OUTPUT
_EOF
            AUTORECONF="$autoreconf -I $libtoolm4dir"
            if (cd .tmp && $AUTORECONF -ivf >/dev/null 2>&1) ; then
                new_autoreconf_works=yes
            fi
            rm -rf .tmp
        fi
        echo "$new_autoreconf_works"
        # If autoreconf accepts -I <libtool's m4 dir> correctly, use -I.
        # If not, run libtoolize before autoreconf (i.e. for autoconf <= 2.63)
        # This test is more general than checking the autoconf version.
        if [ "$new_autoreconf_works" != "yes" ] ; then
            echo_n "Checking if $autoreconf works after an additional $libtoolize step... "
            new_autoreconf_works=no
            recreate_tmp
            # Need AC_CONFIG_
            cat >.tmp/configure.ac <<_EOF
AC_INIT(foo,1.0)
AC_CONFIG_AUX_DIR([m4])
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL
AC_OUTPUT
_EOF
            cat >.tmp/Makefile.am <<_EOF
ACLOCAL_AMFLAGS = -I m4
_EOF
            AUTORECONF="eval $libtoolize && $autoreconf"
            if (cd .tmp && $AUTORECONF -ivf >u.txt 2>&1) ; then
                new_autoreconf_works=yes
            fi
            rm -rf .tmp
            echo "$new_autoreconf_works"
        fi
        if [ "$new_autoreconf_works" = "yes" ] ; then
            export AUTORECONF
            autoreconf="$AUTORECONF"
        else
            # Since all autoreconf workarounds do not work, we need
            # to require all autotools to be in the same directory.
            do_atdir_check=yes
            error "Since none of the autoreconf workaround works"
            error "and autotools are not in the same directory, aborting..."
            error "Updating autotools or putting all autotools in the same location"
            error "may resolve the issue."
            exit 1
        fi
    fi

    if test $do_quick = "yes" ; then
        : # skip autotool versions check in quick mode (since it is too slow)
    else
        ########################################################################
        ## Verify autoconf version
        ########################################################################

        echo_n "Checking for autoconf version... "
        recreate_tmp
        ver=2.69
        cat > .tmp/configure.ac<<EOF
AC_INIT
AC_PREREQ($ver)
AC_OUTPUT
EOF
        if (cd .tmp && $autoreconf $autoreconf_args >/dev/null 2>&1 ) ; then
            echo ">= $ver"
        else
            echo "bad autoconf installation"
            cat <<EOF
You either do not have autoconf in your path or it is too old (version
$ver or higher required). You may be able to use

    autoconf --version

Unfortunately, there is no standard format for the version output and
it changes between autotools versions.  In addition, some versions of
autoconf choose among many versions and provide incorrect output).
EOF
            exit 1
        fi


        ########################################################################
        ## Verify automake version
        ########################################################################

        echo_n "Checking for automake version... "
        recreate_tmp
        ver=1.15
        cat > .tmp/configure.ac<<EOF
AC_INIT(testver,1.0)
AC_CONFIG_AUX_DIR([m4])
AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_INIT_AUTOMAKE],,[m4_fatal([AM_INIT_AUTOMAKE not defined])])
AM_INIT_AUTOMAKE([$ver foreign])
AC_MSG_RESULT([A message])
AC_OUTPUT([Makefile])
EOF
    cat <<EOF >.tmp/Makefile.am
ACLOCAL_AMFLAGS = -I m4
EOF
        if [ ! -d .tmp/m4 ] ; then mkdir .tmp/m4 >/dev/null 2>&1 ; fi
        if (cd .tmp && $autoreconf $autoreconf_args >/dev/null 2>&1 ) ; then
            echo ">= $ver"
        else
            echo "bad automake installation"
            cat <<EOF
You either do not have automake in your path or it is too old (version
$ver or higher required). You may be able to use

    automake --version

Unfortunately, there is no standard format for the version output and
it changes between autotools versions.  In addition, some versions of
autoconf choose among many versions and provide incorrect output).
EOF
            exit 1
        fi


        ########################################################################
        ## Verify libtool version
        ########################################################################

        echo_n "Checking for libtool version... "
        recreate_tmp
        ver=2.4.4
        cat <<EOF >.tmp/configure.ac
AC_INIT(testver,1.0)
AC_CONFIG_AUX_DIR([m4])
AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([LT_PREREQ],,[m4_fatal([LT_PREREQ not defined])])
LT_PREREQ($ver)
LT_INIT()
AC_MSG_RESULT([A message])
EOF
        cat <<EOF >.tmp/Makefile.am
ACLOCAL_AMFLAGS = -I m4
EOF
        if [ ! -d .tmp/m4 ] ; then mkdir .tmp/m4 >/dev/null 2>&1 ; fi
        if (cd .tmp && $autoreconf $autoreconf_args >/dev/null 2>&1 ) ; then
            echo ">= $ver"
        else
            echo "bad libtool installation"
            cat <<EOF
You either do not have libtool in your path or it is too old
(version $ver or higher required). You may be able to use

    libtool --version

Unfortunately, there is no standard format for the version output and
it changes between autotools versions.  In addition, some versions of
autoconf choose among many versions and provide incorrect output).
EOF
            exit 1
        fi
    fi
}

fn_check_bash_find_patch_xargs() {
    echo_n "Checking for bash... "
    if test "`which bash 2>&1 > /dev/null ; echo $?`" = "0" ;then
        echo "done"
    else
        echo "bash not found" ;
        exit 1;
    fi

    echo_n "Checking for UNIX find... "
    find ./maint -name 'configure.ac' > /dev/null 2>&1
    if [ $? = 0 ] ; then
        echo "done"
    else
        echo "not found (error)"
        exit 1
    fi

    echo_n "Checking for UNIX patch... "
    patch -v > /dev/null 2>&1
    if [ $? = 0 ] ; then
        echo "done"
    else
        echo "not found (error)"
        exit 1
    fi

    echo_n "Checking if xargs rm -rf works... "
    if [ -d "`find ./maint -name __random_dir__`" ] ; then
        error "found a directory named __random_dir__"
        exit 1
    else
        mkdir ./maint/__random_dir__
        find ./maint -name __random_dir__ | xargs rm -rf > /dev/null 2>&1
        if [ $? = 0 ] ; then
            echo "yes"
        else
            echo "no (error)"
            rm -rf ./maint/__random_dir__
            exit 1
        fi
    fi
}

# end of utility functions
#-----------------------------------------------------------------------

########################################################################
## Read the command-line arguments
########################################################################

for arg in "$@" ; do
    case $arg in 
        -quick)
            ;;
	-echo)
	    set -x
	    ;;
	
	-atdircheck=*)
	    val=`echo X$arg | sed -e 's/^X-atdircheck=//'`
            case $val in
		yes|YES|true|TRUE|1) do_atdir_check=yes ;;
		no|NO|false|FALSE|0) do_atdir_check=no ;;
		*) warn "unknown option: $arg."
            esac
            ;;

	-atvercheck=*)
            val=`echo X$arg | sed -e 's/^X-atvercheck=//'`
            case $val in
		yes|YES|true|TRUE|1) do_atver_check=yes ;;
		no|NO|false|FALSE|0) do_atver_check=no ;;
		*) warn "unknown option: $arg."
            esac
            ;;

	-yaksa-depth=*)
            val=`echo X$arg | sed -e 's/^X-yaksa-depth=//'`
            yaksa_depth=$val
            ;;

	-do=*|--do=*)
	    opt=`echo A$arg | sed -e 's/^A--*do=//'`
            if type fn_$opt | grep -q 'function' ; then
                echo Running step $opt...
                fn_$opt
                exit 0
            else
		echo "-do=$opt is unrecognized"
		exit 1
            fi
	    ;;

        -verbose-autoreconf|--verbose-autoreconf)
            autoreconf_args="-vif"
            export autoreconf_args
            ;;

	-with-autotools=*|--with-autotools=*)
	    autotoolsdir=`echo "A$arg" | sed -e 's/.*=//'`
	    ;;

        -without-*|--without-*)
            opt=`echo A$arg | sed -e 's/^A--*without-//'`
            var=do_$opt
            eval $var=no
            case "$opt" in
                fortran)
                    do_f77=no
                    do_f90=no
                    do_f08=no
                    ;;
            esac
            ;;

        -with-*|--with-*)
            opt=`echo A$arg | sed -e 's/^A--*with-//'`
            var=do_$opt
            eval $var=yes
            case "$opt" in
                f77 | f90 | f08)
                    do_fortran=yes
                    ;;
            esac
            ;;

	-help|--help|-usage|--usage)
	    cat <<EOF
   ./autogen.sh [ --with-autotools=dir ] \\
                [ -atdircheck=[yes|no] ] \\
                [ -atvercheck=[yes|no] ] \\
                [ --verbose-autoreconf ] \\
                [ --do=stepname ] [ -distrib ]
    Update the files in the MPICH build tree.  This file builds the 
    configure files, creates the Makefile.in files, extracts the error
    messages.

    You can use --with-autotools=dir to specify a directory that
    contains alternate autotools.

    -atdircheck=[yes|no] specifies the enforcement of all autotools
    should be installed in the same directory.

    -atvercheck=[yes|no] specifies if the check for the version of 
    autotools should be carried out.

    -distrib creates a distribution version of the Makefile.in files (no
    targets for updating the Makefile.in from Makefile.sm or rebuilding the
    autotools targets).  This does not create the configure files because
    some of those depend on rules in the Makefile.in in that directory.  
    Thus, to build all of the files for a distribution, run autogen.sh
    twice, as in 
         autogen.sh && autogen.sh -distrib

    -quick skips most of the modules autoconf. This is used when modules
    are prebuilt and used specifically in CI testing to accelerate the build.
EOF
	    exit 1
	    ;;

	*)
	    echo "unknown argument $arg"
	    exit 1
	    ;;

    esac
done

########################################################################
## Check for the location of autotools
########################################################################

if test "$do_quick" = "no" ; then
    fn_check_autotools
else
    set_autotools
fi
fn_check_bash_find_patch_xargs
check_python3

########################################################################
## Setup external packages
########################################################################
set_externals

########################################################################
## duplicating confdb, mpl, version.m4 etc.
########################################################################
fn_copy_confdb_etc

########################################################################
## Building maint/Version
########################################################################
fn_maint_version

########################################################################
## Building the README
########################################################################
fn_update_README

set -e

########################################################################
## Building subsys_include.m4
########################################################################
if [ "X$do_subcfg_m4" = Xyes ] ; then
    fn_gen_subcfg_m4
fi


########################################################################
## Building ROMIO glue code
########################################################################
fn_romio_glue

########################################################################
## Building Collective top-level code
########################################################################
fn_gen_coll

########################################################################
## Building C interfaces
########################################################################
fn_gen_binding_c

########################################################################
## Building non-C interfaces
########################################################################

# Create the bindings if necessary 
if [ $do_f77 = "yes" ] ; then
    fn_f77
else
    touch src/binding/fortran/mpif_h/mpif.h.in
fi
if [ $do_f90 = "yes" ] ; then
    fn_f90
fi
if [ $do_f08 = "yes" ] ; then
    fn_f08
else
    touch src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.f90.in
fi

if [ $do_cxx = "yes" ] ; then
    fn_cxx
fi

########################################################################
## Extract error messages
########################################################################
if [ $do_errmsgs = "yes" ] ; then
    fn_errmsgs
fi


########################################################################
## Build required scripts
########################################################################
fn_maint_configure

# new parameter code
if test "$do_getcvars" = "yes" ; then
    fn_getcvars
fi

########################################################################
## Running autotools on non-simplemake directories
########################################################################
fn_build_configure

fn_ch4_api

fn_json_gen
