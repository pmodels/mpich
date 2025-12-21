#! /bin/sh
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

if test -z "$AUTORECONF" ; then
    AUTORECONF="autoreconf"
fi

if test -n "$MPICH_CONFDB" ; then
    CONFARGS="-I $MPICH_CONFDB -vif"
    mkdir -p confdb
    cp -v $MPICH_CONFDB/config.rpath confdb/
else
    if ! test -d confdb ; then
        echo "Missing confdb!"
        exit 1
    fi

    CONFARGS="-vif"
fi


$AUTORECONF $CONFARGS || exit 1

# need python3 to generate bindings

check_python3() {
    printf "Checking for python ..."
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

if test -z "$PYTHON" ; then
    check_python3
fi
