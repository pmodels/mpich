dnl This is parallel to aclocal_subcfg.m4.
dnl `aclocal_subcfg.m4` executes subcfg inside top configure's space.
dnl `aclocal_subdir.m4`(this file) runs separate `configure` scripts.
dnl Both files have distinct logic despite similar sematics, therefore separate m4.
dnl FIXME: PAC_CONFIG_SUBDIR_ARGS and PAC_CONFIG_SUBDIR need be moved here.
dnl        Also: PAC_SUBDIR_CACHE_CLEANUP

dnl In MPICH, there are: 
dnl     src/util/logging/[logging_name]
dnl     src/mpi/romio
dnl     [devicedir] # none left right?
dnl     src/pm/[pm_name]
dnl     src/nameserv/[namepublisher]
dnl     what about bindingsubsystems

dnl call this right after AC_INIT (not must, but a good convention).
AC_DEFUN([PAC_SUBDIR_INIT],[
    dnl automake will define all AC_SUBST's
    AC_SUBST([external_subdirs])
    AC_SUBST([external_dist_subdirs])
    AC_SUBST([external_libs])
])

dnl PAC_SUBDIR_ADD(subdir)
AC_DEFUN([PAC_SUBDIR_ADD],[
    subsystems="$subsystems $1"
])

dnl call this at the end of configure.ac
dnl This will inflate the configure for hydra and romio, unfortunately
AC_DEFUN([PAC_CONFIG_ALL_SUBDIRS],[
    all="m4_default([$1], [$subsystems])"
    if test -n "$all" ; then
        echo '------------------------------------------'
        echo Configure all subsystems: $all
        echo '------------------------------------------'
        for subsys in $all ; do 
            dnl may need reset
            dnl   CFLAGS, CPPFLAGS, CXXFLAGS, FFLAGS, FCFLAGS, LDFLAGS, LIBS
            need_reset_flags=yes
            case $subsys in
                mpl|*/mpl)
                    need_reset_flags=no
                    PAC_CONFIG_ARG_MPL
                    ;;
                */openpa)
                    need_reset_flags=no
                    PAC_CONFIG_ARG_OPA
                    ;;
                */izem)
                    PAC_CONFIG_ARG_IZEM
                    ;;
                */hwloc)
                    PAC_CONFIG_ARG_HWLOC
                    ;;
                */libfabric)
                    PAC_CONFIG_ARG_OFI
                    ;;
                */ucx)
                    PAC_CONFIG_ARG_UCX
                    ;;
                src/mpi/romio)
                    config_arg="FROM_MPICH=yes --with-mpl-prefix=../../mpl"
                    ;;
                src/pm/hydra)
                    config_arg="--with-mpl-prefix=../../mpl --with-hwloc-prefix=../../hwloc"
                    ;;
                *)
                    config_arg=""
                    ;;
            esac

            if test need_reset_flags = yes ; then
                PAC_PUSH_ALL_FLAGS
                PAC_RESET_ALL_FLAGS
            fi
            PAC_CONFIG_SUBDIR_ARGS([$subsys],[$config_arg],[],[AC_MSG_ERROR([$subsys configure failed])])
            if test need_reset_flags = yes ; then
                PAC_POP_ALL_FLAGS
            fi
        done 
    fi

    dnl FIXME: Not sure why we need following. Comment out to find out the consequence...
    dnl if test "$DEBUG_SUBDIR_CACHE" = yes -a "$enable_echo" != yes ; then 
    dnl     set +x
    dnl fi
    dnl PAC_SUBDIR_CACHE_CLEANUP

    dnl # Make subsystems available to makefiles.
    dnl # FIXME does the makefile actually need this?
    dnl subsystems="$devsubsystems $subsystems $bindingsubsystems"
])

dnl ****************************************
dnl Apparently we need allow the possiblity of library under custom name.
dnl PAC_SUBDIR_LIBNAME([var], [default])
AC_DEFUN([PAC_SUBDIR_LIBNAME],[
    AC_ARG_VAR([$1],[can be used to override the name of the $2 library (default: "$2")])
    $1=${$1:-"$2"}
    export $1
    AC_SUBST($1)
])

dnl **** embed ************************************
dnl PAC_SUBDIR_EMBED([libname],[src_dir],[la_dir],[inc_dir])
AC_DEFUN([PAC_SUBDIR_EMBED],[
    if test -e "$2" ; then
        subsystems="$subsystems $2"
        external_subdirs="$external_subdirs $2"
        external_dist_subdirs="$external_dist_subdirs $2"
        external_libs="$external_libs $3/lib$1.la"
        PAC_APPEND_FLAG([-I./$4],[CPPFLAGS])
        if test $srcdir != . ; then
            PAC_APPEND_FLAG([-I${srcdir}/$4],[CPPFLAGS])
        fi
    else
        dnl FIXME: if may fail, then should fail here.
        AC_MSG_WARN([Attempted to use the embedded $1 source tree in "$2", but it is missing.  Configuration or compilation may fail later.])
    fi
])

dnl Reuse the convenience library, skip subsystems
dnl PAC_SUBDIR_EMBED([libname],[src_dir],[la_dir],[inc_dir])
AC_DEFUN([PAC_SUBDIR_EMBED_REUSE],[
    if test -e "$2" ; then
        dnl prevent linking libmpl.la twice
        m4_ifndef([INSIDE_ROMIO], [
            external_libs="$external_libs $3/lib$1.la"
        ])
        PAC_APPEND_FLAG([-I./$4],[CPPFLAGS])
        if test $srcdir != . ; then
            PAC_APPEND_FLAG([-I${srcdir}/$4],[CPPFLAGS])
        fi
    else
        dnl FIXME: if it may fail later, then it should fail here.
        AC_MSG_WARN([Attempted to reuse the embedded $1 source tree in "$2", but it is missing.  Configuration or compilation may fail later.])
    fi
])

dnl **** system ************************************
dnl PAC_SUBDIR_SYSTEM(libname)
AC_DEFUN([PAC_SUBDIR_SYSTEM],[
    PAC_PREPEND_FLAG([-l$1],[LIBS])
    m4_ifset([INSIDE_MPICH],[
            PAC_PREPEND_FLAG([-l$1],[WRAPPER_LIBS])
        ],[])
])

dnl **** prefix ************************************
dnl FIXME: unify with PAC_SET_HEADER_LIB_PATH
dnl PAC_ARG_WITH_PREFIX(libname)
AC_DEFUN([PAC_ARG_WITH_PREFIX],[
	AC_ARG_WITH([$1-prefix],
            [AS_HELP_STRING([[--with-$1-prefix[=DIR]]], [use the $1
                            library installed in DIR, rather than the
                            one included in the distribution.  Pass
                            "embedded" to force usage of the included
                            $1 source.])],
            [],dnl action-if-given
            [with_$1_prefix="embedded"])
])

dnl PAC_SUBDIR_PREFIX([libname],[prefix_dir])
AC_DEFUN([PAC_SUBDIR_PREFIX],[
    PAC_APPEND_FLAG([-I$2/include],[CPPFLAGS])
    if test -d "$2/lib64"; then
        pac_libdir="$2/lib64"
    else
        pac_libdir="$2/lib"
    fi
    PAC_PREPEND_FLAG([-L${pac_libdir}],[LDFLAGS])
    m4_ifset([INSIDE_MPICH],[
        PAC_PREPEND_FLAG([-L${pac_libdir}],[WRAPPER_LDFLAGS])
        ],[])

    if "$3"x == x ; then
        PAC_SUBDIR_SYSTEM([$1])
    fi
])

dnl .internal.
dnl PAC_SUBDIR_PREFIX_CHECK([name],[dir],[file])
AC_DEFUN([PAC_SUBDIR_PREFIX_CHECK],[
    AS_IF([test -s "$2/$3"],
        [:],[AC_MSG_ERROR([the $1 installation in "$2" appears broken])])
])

