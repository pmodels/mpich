dnl **** ROMIO ************************************

AC_DEFUN([PAC_SUBDIR_ENABLE_ROMIO],[
    AC_ARG_ENABLE(romio,
	AC_HELP_STRING([--enable-romio], [Enable ROMIO MPI I/O implementation]),,
	enable_romio=yes)
])

dnl PAC_SUBDIR_ROMIO([embed_src_dir])
AC_DEFUN([PAC_SUBDIR_ROMIO],[
    AC_SUBST(HAVE_ROMIO)
    if test "$enable_romio" = "yes" ; then
        dnl expands @HAVE_ROMIO@ in mpi.h into #include "mpio.h"
        HAVE_ROMIO='#include "mpio.h"'
        if test -d $1 ; then
            subsystems="$subsystems $1"
            AC_DEFINE(HAVE_ROMIO,1,[Define if ROMIO is enabled])

            # make it possible to "#include" mpio.h at build time
            #
            # This ought to be sufficient, but there is also a symlink setup in
            # src/include to accomodate current mpicc limitations.  See
            # src/mpi/Makefile.mk for more info.
            # PAC_APPEND_FLAG([-I${master_top_builddir}/src/mpi/romio/include],[CPPFLAGS])

            # Set environment variables that the romio configure expects
            # export use_top_srcdir
            # top_build_dir=`pwd`
            # export top_build_dir
            # if there is no $top_build_dir/lib, romio puts lib in wrong place
            # This test used -e under Linux, but not all test programs understand
            # -e
            # if test ! -d lib ; then mkdir lib ; fi
            # tell mpi.h to include mpio.h
            dnl defined in confdb/aclocal_mpi.m4
            dnl really it should be here or it should be PAC_HAVE_MPIO
            PAC_HAVE_ROMIO
        else
            AC_MSG_WARN([ROMIO src directory is not available])
        fi
    fi

    AM_CONDITIONAL([BUILD_ROMIO], [test x$enable_romio = xyes])
])
