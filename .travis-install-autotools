#!/bin/sh

set -e
set -x

os=`uname`
TOP="$1"

case "$os" in
    Darwin)
        brew update
        brew info autoconf automake libtool
        #brew install autoconf automake libtool
        brew upgrade autoconf automake libtool
        which glibtool
        which glibtoolize
        glibtool --version
        ln -s `which glibtool` ${TOP}/bin/libtool
        ln -s `which glibtoolize` ${TOP}/bin/libtoolize
        ;;
    Linux)
        MAKE_JNUM=2
        M4_VERSION=1.4.17
        LIBTOOL_VERSION=2.4.4
        AUTOCONF_VERSION=2.69
        AUTOMAKE_VERSION=1.15

        cd ${TOP}
        TOOL=m4
        TDIR=${TOOL}-${M4_VERSION}
        FILE=${TDIR}.tar.gz
        BIN=${TOP}/bin/${TOOL}
        if [ -f ${FILE} ] ; then
          echo ${FILE} already exists! Using existing copy.
        else
          wget http://ftp.gnu.org/gnu/${TOOL}/${FILE}
        fi
        if [ -d ${TDIR} ] ; then
          echo ${TDIR} already exists! Using existing copy.
        else
          echo Unpacking ${FILE}
          tar -xzf ${FILE}
        fi
        if [ -f ${BIN} ] ; then
          echo ${BIN} already exists! Skipping build.
        else
          cd ${TOP}/${TDIR}
          ./configure --prefix=${TOP} && make -j ${MAKE_JNUM} && make install
          if [ "x$?" != "x0" ] ; then
            echo FAILURE 1
            exit
          fi
        fi

        cd ${TOP}
        TOOL=libtool
        TDIR=${TOOL}-${LIBTOOL_VERSION}
        FILE=${TDIR}.tar.gz
        BIN=${TOP}/bin/${TOOL}
        if [ ! -f ${FILE} ] ; then
          wget http://ftp.gnu.org/gnu/${TOOL}/${FILE}
        else
          echo ${FILE} already exists! Using existing copy.
        fi
        if [ ! -d ${TDIR} ] ; then
          echo Unpacking ${FILE}
          tar -xzf ${FILE}
        else
          echo ${TDIR} already exists! Using existing copy.
        fi
        if [ -f ${BIN} ] ; then
          echo ${BIN} already exists! Skipping build.
        else
          cd ${TOP}/${TDIR}
          ./configure --prefix=${TOP} M4=${TOP}/bin/m4 && make -j ${MAKE_JNUM} && make install
          if [ "x$?" != "x0" ] ; then
            echo FAILURE 2
            exit
          fi
        fi

        cd ${TOP}
        TOOL=autoconf
        TDIR=${TOOL}-${AUTOCONF_VERSION}
        FILE=${TDIR}.tar.gz
        BIN=${TOP}/bin/${TOOL}
        if [ ! -f ${FILE} ] ; then
          wget http://ftp.gnu.org/gnu/${TOOL}/${FILE}
        else
          echo ${FILE} already exists! Using existing copy.
        fi
        if [ ! -d ${TDIR} ] ; then
          echo Unpacking ${FILE}
          tar -xzf ${FILE}
        else
          echo ${TDIR} already exists! Using existing copy.
        fi
        if [ -f ${BIN} ] ; then
          echo ${BIN} already exists! Skipping build.
        else
          cd ${TOP}/${TDIR}
          ./configure --prefix=${TOP} M4=${TOP}/bin/m4 && make -j ${MAKE_JNUM} && make install
          if [ "x$?" != "x0" ] ; then
            echo FAILURE 3
            exit
          fi
        fi

        cd ${TOP}
        TOOL=automake
        TDIR=${TOOL}-${AUTOMAKE_VERSION}
        FILE=${TDIR}.tar.gz
        BIN=${TOP}/bin/${TOOL}
        if [ ! -f ${FILE} ] ; then
          wget http://ftp.gnu.org/gnu/${TOOL}/${FILE}
        else
          echo ${FILE} already exists! Using existing copy.
        fi
        if [ ! -d ${TDIR} ] ; then
          echo Unpacking ${FILE}
          tar -xzf ${FILE}
        else
          echo ${TDIR} already exists! Using existing copy.
        fi
        if [ -f ${BIN} ] ; then
          echo ${BIN} already exists! Skipping build.
        else
          cd ${TOP}/${TDIR}
          ./configure --prefix=${TOP} M4=${TOP}/bin/m4 && make -j ${MAKE_JNUM} && make install
          if [ "x$?" != "x0" ] ; then
            echo FAILURE 4
            exit
          fi
        fi
        ;;
esac

