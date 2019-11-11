#! /bin/sh
# 
# (C) 2006 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#
# Update all of the derived files
# For best performance, execute this in the top-level directory.
# There are some experimental features to allow it to be executed in
# subdirectories
#
# Eventually, we want to allow this script to be executed anywhere in the
# mpich tree.  This is not yet implemented.


########################################################################
## Utility functions
########################################################################

recreate_tmp() {
    rm -rf .tmp
    mkdir .tmp 2>&1 >/dev/null
}

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
        error "Submodule $1 is not checked out"
        exit 1
    fi
}

sync_external () {
    srcdir=$1
    destdir=$2

    echo "syncing '$srcdir' --> '$destdir'"

    # deletion prevents creating 'confdb/confdb' situation, also cleans
    # stray files that may have crept in somehow
    rm -rf "$destdir"
    cp -pPR "$srcdir" "$destdir"
}

########################################################################
echo
echo "####################################"
echo "## Checking user environment"
echo "####################################"
echo

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
do_bindings=yes
do_geterrmsgs=yes
do_getcvars=yes
do_f77=yes
do_build_configure=yes
do_genstates=yes
do_atdir_check=no
do_atver_check=yes
do_subcfg_m4=yes
do_izem=yes
do_ofi=yes
do_ucx=yes

export do_build_configure

# Allow MAKE to be set from the environment
MAKE=${MAKE-make}

# amdirs are the directories that make use of autoreconf
amdirs=". src/mpl src/util/logging/rlog"

autoreconf_args="-if"
export autoreconf_args

########################################################################
## Read the command-line arguments
########################################################################

# List of steps that we will consider (We do not include depend
# because the values for depend are not just yes/no)
AllSteps="geterrmsgs bindings f77 build_configure genstates getparms"
stepsCleared=no

for arg in "$@" ; do
    case $arg in 
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

	-do=*|--do=*)
	    opt=`echo A$arg | sed -e 's/^A--*do=//'`
	    case $opt in 
		build-configure|configure) opt=build_configure ;;
	    esac
	    var=do_$opt

	    # Check that this opt is known
	    eval oldval=\$"$var"
	    if [ -z "$oldval" ] ; then
		echo "-do=$opt is unrecognized"
		exit 1
	    else
		if [ $stepsCleared = no ] ; then
		    for step in $AllSteps ; do
			var=do_$step
			eval $var=no
		    done
		    stepsCleared=yes
		fi
		var=do_$opt
		eval $var=yes
	    fi
	    ;;

        -verbose-autoreconf|--verbose-autoreconf)
            autoreconf_args="-vif"
            export autoreconf_args
            ;;

	-with-genstates|--with-genstates)
	    do_genstates=yes
	    ;;

	-without-genstates|--without-genstates)
	    do_genstates=no
	    ;;
 
	-with-errmsgs|--with-errmsgs)
	    do_geterrmsgs=yes
	    ;;

	-without-errmsgs|--without-errmsgs)
	    do_geterrmsgs=no
	    ;;

	-with-bindings|--with-bindings)
	    do_bindings=yes
	    ;;

	-without-bindings|--without-bindings)
	    do_bindings=no
	    ;;

	-with-f77|--with-f77)
	    do_f77=yes
	    ;;

	-without-f77|--without-f77)
	    do_f77=no
	    ;;

	-with-autotools=*|--with-autotools=*)
	    autotoolsdir=`echo "A$arg" | sed -e 's/.*=//'`
	    ;;

    -without-izem|--without-izem)
        do_izem=no
        ;;

    -without-ofi|--without-ofi|-without-libfabric|--without-libfabric)
        do_ofi=no
        ;;

    -without-ucx|--without-ucx)
        do_ucx=no
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

    Use --do=stepname to update only a single step.  For example, 
    --do=build_configure only updates the configure scripts.  The available
    steps are:
EOF
	    for step in $AllSteps ; do
		echo "        $step"
	    done
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

########################################################################
## Verify autoconf version
########################################################################

echo_n "Checking for autoconf version... "
recreate_tmp
ver=2.67
# petsc.mcs.anl.gov's /usr/bin/autoreconf is version 2.65 which returns OK
# if configure.ac has AC_PREREQ() withOUT AC_INIT.
#
# ~/> hostname
# petsc
# ~> /usr/bin/autoconf --version
# autoconf (GNU Autoconf) 2.65
# ....
# ~/> cat configure.ac
# AC_PREREQ(2.68)
# ~/> /usr/bin/autoconf ; echo "rc=$?"
# configure.ac:1: error: Autoconf version 2.68 or higher is required
# configure.ac:1: the top level
# autom4te: /usr/bin/m4 failed with exit status: 63
# rc=63
# ~/> /usr/bin/autoreconf ; echo "rc=$?"
# rc=0
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


########################################################################
## Checking for UNIX find
########################################################################

echo_n "Checking for UNIX find... "
find ./maint -name 'configure.ac' > /dev/null 2>&1
if [ $? = 0 ] ; then
    echo "done"
else
    echo "not found (error)"
    exit 1
fi


########################################################################
## Checking if xargs rm -rf works
########################################################################

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

########################################################################
## Setup external packages
########################################################################

echo
echo "###########################################################"
echo "## Checking submodules"
echo "###########################################################"
echo

# hwloc is always required
check_submodule_presence src/hwloc

# external packages that require autogen.sh to be run for each of them
externals="src/pm/hydra src/pm/hydra2 src/mpi/romio src/openpa src/hwloc test/mpi"

if [ "yes" = "$do_izem" ] ; then
    check_submodule_presence src/izem
    externals="${externals} src/izem"
fi

if [ "yes" = "$do_ucx" ] ; then
    check_submodule_presence src/mpid/ch4/netmod/ucx/ucx
    externals="${externals} src/mpid/ch4/netmod/ucx/ucx"
fi

if [ "yes" = "$do_ofi" ] ; then
    check_submodule_presence src/mpid/ch4/netmod/ofi/libfabric
    externals="${externals} src/mpid/ch4/netmod/ofi/libfabric"
fi

########################################################################
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
confdb_dirs="${confdb_dirs} src/mpi/romio/confdb"
confdb_dirs="${confdb_dirs} src/mpi/romio/mpl/confdb"
confdb_dirs="${confdb_dirs} src/mpl/confdb"
confdb_dirs="${confdb_dirs} src/openpa/confdb"
confdb_dirs="${confdb_dirs} src/pm/hydra/confdb"
confdb_dirs="${confdb_dirs} src/pm/hydra2/confdb"
confdb_dirs="${confdb_dirs} src/pm/hydra/mpl/confdb"
confdb_dirs="${confdb_dirs} src/pm/hydra2/mpl/confdb"
confdb_dirs="${confdb_dirs} test/mpi/confdb"

# hydra's copies of mpl and hwloc
sync_external src/mpl src/pm/hydra/mpl
sync_external src/mpl src/pm/hydra2/mpl

# ROMIO's copy of mpl
sync_external src/mpl src/mpi/romio/mpl

# all the confdb directories, by various names
for destdir in $confdb_dirs ; do
    sync_external confdb "$destdir"
done

# Copying hwloc to hydra
sync_external src/hwloc src/pm/hydra/tools/topo/hwloc/hwloc
sync_external src/hwloc src/pm/hydra2/libhydra/topo/hwloc/hwloc
# remove .git directories to avoid confusing git clean
rm -rf src/pm/hydra/tools/topo/hwloc/hwloc/.git
rm -rf src/pm/hydra2/libhydra/topo/hwloc/hwloc/.git

# a couple of other random files
if [ -f maint/version.m4 ] ; then
    cp -pPR maint/version.m4 src/pm/hydra/version.m4
    cp -pPR maint/version.m4 src/pm/hydra2/version.m4
    cp -pPR maint/version.m4 src/mpi/romio/version.m4
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

echo
echo
echo "###########################################################"
echo "## Autogenerating required files"
echo "###########################################################"
echo

########################################################################
## Building maint/Version
########################################################################

# build a substitute maint/Version script now that we store the single copy of
# this information in an m4 file for autoconf's benefit
echo_n "Generating a helper maint/Version... "
if $autom4te -l M4sugar maint/Version.base.m4 > maint/Version ; then
    echo "done"
else
    echo "error"
    error "unable to correctly generate maint/Version shell helper"
fi

########################################################################
## Building the README
########################################################################

echo_n "Updating the README... "
. ./maint/Version
if [ -f README.vin ] ; then
    sed -e "s/%VERSION%/${MPICH_VERSION}/g" README.vin > README
    echo "done"
else
    echo "error"
    error "README.vin file not present, unable to update README version number (perhaps we are running in a release tarball source tree?)"
fi


########################################################################
## Building subsys_include.m4
########################################################################
if [ "X$do_subcfg_m4" = Xyes ] ; then
    echo_n "Creating subsys_include.m4... "
    ./maint/gen_subcfg_m4
    echo "done"
fi


########################################################################
## Building ROMIO glue code
########################################################################
echo_n "Building ROMIO glue code... "
( cd src/glue/romio && chmod a+x ./all_romio_symbols && ./all_romio_symbols ../../mpi/romio/include/mpio.h.in )
echo "done"


########################################################################
## Building non-C interfaces
########################################################################

# Create the bindings if necessary 
if [ $do_bindings = "yes" ] ; then
    build_f77=no
    build_f90=no
    build_cxx=no
    if [ $do_f77 = "yes" ] ; then
        if [ ! -s src/binding/fortran/mpif_h/abortf.c ] ; then
	    build_f77=yes
        elif find src/binding/fortran/mpif_h -name 'buildiface' -newer 'src/binding/fortran/mpif_h/abortf.c' >/dev/null 2>&1 ; then
	    build_f77=yes
        fi
        if [ ! -s src/binding/fortran/use_mpi/mpi_base.f90 ] ; then
 	    build_f90=yes
        elif find src/binding/fortran/use_mpi -name 'buildiface' -newer 'src/binding/fortran/use_mpi/mpi_base.f90' >/dev/null 2>&1 ; then
	    build_f90=yes
        fi
        if [ ! -s src/binding/fortran/use_mpi_f08/wrappers_c/cdesc.c ] ; then
	    build_f08=yes
        elif find src/binding/fortran/use_mpi_f08 -name 'buildiface' -newer 'src/binding/fortran/use_mpi_f08/wrappers_c/cdesc.c' >/dev/null 2>&1 ; then
	    build_f08=yes
        fi
    fi

    if [ $build_f77 = "yes" ] ; then
	echo_n "Building Fortran 77 interface... "
	( cd src/binding/fortran/mpif_h && chmod a+x ./buildiface && ./buildiface )
	echo "done"
    fi
    if [ $build_f90 = "yes" ] ; then
	echo_n "Building Fortran 90 interface... "
	# Remove any copy of mpi_base.f90 (this is used to handle the
	# Double precision vs. Real*8 option
	rm -f src/binding/fortran/use_mpi/mpi_base.f90.orig
	( cd src/binding/fortran/use_mpi && chmod a+x ./buildiface && ./buildiface )
	( cd src/binding/fortran/use_mpi && ../mpif_h/buildiface -infile=cf90t.h -deffile=./cf90tdefs)
	echo "done"
    fi
    if [ $build_f08 = "yes" ] ; then
	echo_n "Building Fortran 08 interface... "
	# Top-level files
	( cd src/binding/fortran/use_mpi_f08 && chmod a+x ./buildiface && ./buildiface )
        # Delete the old Makefile.mk
        ( rm -f src/binding/fortran/use_mpi_f08/wrappers_c/Makefile.mk )
        # Execute once for mpi.h.in ...
	( cd src/binding/fortran/use_mpi_f08/wrappers_c && chmod a+x ./buildiface && ./buildiface ../../../../include/mpi.h.in )
        # ... and once for mpio.h.in
	( cd src/binding/fortran/use_mpi_f08/wrappers_c && chmod a+x ./buildiface && ./buildiface ../../../../mpi/romio/include/mpio.h.in )
	echo "done"
    fi

    if [ ! -s src/binding/cxx/mpicxx.h ] ; then 
	build_cxx=yes
    elif find src/binding/cxx -name 'buildiface' -newer 'src/binding/cxx/mpicxx.h' >/dev/null 2>&1 ; then
	build_cxx=yes
    fi
    if [ $build_cxx = "yes" ] ; then
	echo_n "Building C++ interface... "
	( cd src/binding/cxx && chmod a+x ./buildiface &&
	  ./buildiface -nosep -initfile=./cxx.vlist )
	echo "done"
    fi
fi


########################################################################
## Extract error messages
########################################################################

# Capture the error messages
if [ $do_geterrmsgs = "yes" ] ; then
    if [ -x maint/extracterrmsgs ] ; then
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
            # Incase it exists but has zero size
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
            echo_n "Creating a dummy defmsg.h file... "
	    cat > src/mpi/errhan/defmsg.h <<EOF
typedef struct { const unsigned int sentinal1; const char *short_name, *long_name; const unsigned int sentinal2; } msgpair;
static const int generic_msgs_len = 0;
static msgpair generic_err_msgs[] = { {0xacebad03, 0, "no error catalog", 0xcb0bfa11}, };
static const int specific_msgs_len = 0;
static msgpair specific_err_msgs[] = {  {0xacebad03,0,0,0xcb0bfa11}, };
#if MPICH_ERROR_MSG_LEVEL > MPICH_ERROR_MSG__NONE
#define MPIR_MAX_ERROR_CLASS_INDEX 54
static int class_to_index[] = {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0 };
#endif
EOF
	    echo "done"
        fi
    fi
fi  # do_geterrmsgs


########################################################################
## Build required scripts
########################################################################

echo
echo "------------------------------------"
echo "Initiating building required scripts"
# Build scripts such as genstates if necessary
ran_maint_configure=no
run_configure=no
# The information that autoconf uses is saved in the autom4te*.cache
# file; since this cache is not accurate, we delete it.
if [ ! -x maint/configure ] ; then
    (cd maint && $autoconf && rm -rf autom4te*.cache )
elif find maint -name 'configure.ac' -newer 'maint/configure' >/dev/null 2>&1 ; then
    # The above relies on the Unix find command
    (cd maint && $autoconf && rm -rf autom4te*.cache)
fi
if [ ! -x maint/genstates ] ; then
    run_configure=yes
fi

# The following relies on the Unix find command
if [ -s maint/genstates ] ; then 
    if find maint -name 'genstates.in' -newer 'maint/genstates' >/dev/null 2>&1 ; then
        run_configure=yes
    fi
else
    run_configure=yes
fi

if [ "$run_configure" = "yes" ] ; then
    (cd maint && ./configure)
    ran_maint_configure=yes
fi
echo "Done building required scripts"
echo "------------------------------------"
echo

# Run some of the simple codes
echo_n "Creating the enumeration of logging states into src/include/mpiallstates.h... "
if [ -x maint/extractstates -a $do_genstates = "yes" ] ; then
    ./maint/extractstates
fi
echo "done"

# new parameter code
echo_n "Extracting control variables (cvar) ... "
if test -x maint/extractcvars -a "$do_getcvars" = "yes" ; then
    if ./maint/extractcvars --dirs="`cat maint/cvardirs`"; then
        echo "done"
    else
        echo "failed"
        error "unable to extract control variables"
        exit 1
    fi
else
    echo "skipped"
fi

echo
echo
echo "###########################################################"
echo "## Generating configure files"
echo "###########################################################"
echo

########################################################################
## Running autotools on non-simplemake directories
########################################################################

if [ "$do_build_configure" = "yes" ] ; then
    for external in $externals ; do
       if [ -d "$external" -o -L "$external" ] ; then
           echo "------------------------------------------------------------------------"
           echo "running third-party initialization in $external"
           (cd $external && ./autogen.sh) || exit 1
       fi
    done

    for amdir in $amdirs ; do
	if [ -d "$amdir" -o -L "$amdir" ] ; then
	    echo "------------------------------------------------------------------------"
	    echo "running $autoreconf in $amdir"
            (cd $amdir && $autoreconf $autoreconf_args) || exit 1
            # Patching ltmain.sh
            if [ -f $amdir/confdb/ltmain.sh ] ; then
                echo_n "Patching ltmain.sh for compatibility with Intel compiler options... "
                patch -N -s -l $amdir/confdb/ltmain.sh maint/patches/optional/confdb/intel-compiler.patch
                if [ $? -eq 0 ] ; then
                    # Remove possible leftovers, which don't imply a failure
                    rm -f $amdir/confdb/ltmain.sh.orig
                    echo "done"
                else
                    echo "failed"
                fi
                # Rebuild configure
                (cd $amdir && $autoconf -f) || exit 1
                # Reset ltmain.sh timestamps to avoid confusing make
                touch -r $amdir/confdb/ltversion.m4 $amdir/confdb/ltmain.sh
            fi
            # Patching libtool.m4
            # This works with libtool versions 2.4 - 2.4.2.
            # Older versions are not supported to build mpich.
            # Newer versions should have this patch already included.
            if [ -f $amdir/confdb/libtool.m4 ] ; then
                # There is no need to patch if we're not going to use Fortran.
                ifort_patch_requires_rebuild=no
                oracle_patch_requires_rebuild=no
                flang_patch_requires_rebuild=no
                arm_patch_requires_rebuild=no
                ibm_patch_requires_rebuild=no
                sys_lib_dlsearch_path_patch_requires_rebuild=no
                echo_n "Patching libtool.m4 for system dynamic library search path..."
                patch -N -s -l $amdir/confdb/libtool.m4 maint/patches/optional/confdb/sys_lib_dlsearch_path_spec.patch
                if [ $? -eq 0 ] ; then
                    sys_lib_dlsearch_path_patch_requires_rebuild=yes
                    # Remove possible leftovers, which don't imply a failure
                    rm -f $amdir/confdb/libtool.m4.orig
                    echo "done"
                else
                    echo "failed"
                fi
                if [ $do_bindings = "yes" ] ; then
                    echo_n "Patching libtool.m4 for compatibility with ifort on OSX... "
                    patch -N -s -l $amdir/confdb/libtool.m4 maint/patches/optional/confdb/darwin-ifort.patch
                    if [ $? -eq 0 ] ; then
                        ifort_patch_requires_rebuild=yes
                        # Remove possible leftovers, which don't imply a failure
                        rm -f $amdir/confdb/libtool.m4.orig
                        echo "done"
                    else
                        echo "failed"
                    fi
                    echo_n "Patching libtool.m4 for fort compatibility with Oracle Dev Studio 12.6..."
                    patch -N -s -l $amdir/confdb/libtool.m4 maint/patches/optional/confdb/oracle-fort.patch
                    if [ $? -eq 0 ] ; then
                        oracle_patch_requires_rebuild=yes
                        # Remove possible leftovers, which don't imply a failure
                        rm -f $amdir/confdb/libtool.m4.orig
                        echo "done"
                    else
                        echo "failed"
                    fi
                    echo_n "Patching libtool.m4 for compatibility with Flang..."
                    patch -N -s -l $amdir/confdb/libtool.m4 maint/patches/optional/confdb/flang.patch
                    if [ $? -eq 0 ] ; then
                        flang_patch_requires_rebuild=yes
                        # Remove possible leftovers, which don't imply a failure
                        rm -f $amdir/confdb/libtool.m4.orig
                        echo "done"
                    else
                        echo "failed"
                    fi
                    echo_n "Patching libtool.m4 for compatibility with Arm LLVM compilers..."
                    patch -N -s -l $amdir/confdb/libtool.m4 maint/patches/optional/confdb/arm-compiler.patch
                    if [ $? -eq 0 ] ; then
                        arm_patch_requires_rebuild=yes
                        # Remove possible leftovers, which don't imply a failure
                        rm -f $amdir/confdb/libtool.m4.orig
                        echo "done"
                    else
                        echo "failed"
                    fi
                    echo_n "Patching libtool.m4 for compatibility with IBM XL Fortran compilers..."
                    patch -N -s -l $amdir/confdb/libtool.m4 maint/patches/optional/confdb/ibm-xlf.patch
                    if [ $? -eq 0 ] ; then
                        ibm_patch_requires_rebuild=yes
                        # Remove possible leftovers, which don't imply a failure
                        rm -f $amdir/confdb/libtool.m4.orig
                        echo "done"
                    else
                        echo "failed"
                    fi
                fi

                if [ $ifort_patch_requires_rebuild = "yes" ] || [ $oracle_patch_requires_rebuild = "yes" ] \
                    || [ $arm_patch_requires_rebuild = "yes" ] || [ $ibm_patch_requires_rebuild = "yes" ] \
                    || [ $sys_lib_dlsearch_path_patch_requires_rebuild = "yes" ] || [ $flang_patch_requires_rebuild = "yes" ]; then
                    # Rebuild configure
                    (cd $amdir && $autoconf -f) || exit 1
                    # Reset libtool.m4 timestamps to avoid confusing make
                    touch -r $amdir/confdb/ltversion.m4 $amdir/confdb/libtool.m4
                fi
            fi
	fi
    done
fi
