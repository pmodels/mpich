#! /bin/sh
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

########################################################################
## Utility functions
########################################################################

echo_n() {
    # "echo -n" isn't portable, must portably implement with printf
    printf "%s" "$*"
}

error() {
    echo "===> ERROR:   $@"
    exit
}


########################################################################
## Parse user environment and arguments
########################################################################

genpup_args=
gentests_args=

if test -n "$YAKSA_AUTOGEN_PUP_NESTING" ; then
    genpup_args=$YAKSA_AUTOGEN_PUP_NESTING
fi

for arg in "$@" ; do
    case $arg in
        -pup-max-nesting=*|--pup-max-nesting=*)
            genpup_args="$genpup_args $arg"
            ;;
        -skip-test-complex)
            gentests_args="$gentests_args $arg"
            ;;
        *)
            error "unknown argument $arg"
            ;;
    esac
done

########################################################################
## Generating required files
########################################################################

# backend pup functions
for x in seq cuda ze hip ; do
    echo_n "generating backend pup functions for ${x}... "
    ./src/backend/${x}/genpup.py ${genpup_args}
    if test "$?" = "0" ; then
	echo "done"
    else
	echo "failed"
	exit 1
    fi
done

# tests
./maint/gentests.py ${gentests_args}
if test "$?" != "0" ; then
    echo "test generation failed"
    exit 1
fi


########################################################################
## Autotools
########################################################################

# generate configure files
echo
echo "=== generating configure files in main directory ==="
autoreconf -vif
if test "$?" = "0" ; then
    echo "=== done === "
else
    echo "=== failed === "
    exit 1
fi
echo


########################################################################
## Building maint/Version
########################################################################

# build a substitute maint/Version script now that we store the single copy of
# this information in an m4 file for autoconf's benefit
echo_n "Generating a helper maint/Version... "
if autom4te -l M4sugar maint/Version.base.m4 > maint/Version ; then
    echo "done"
else
    echo "error"
    error "unable to correctly generate maint/Version shell helper"
fi
