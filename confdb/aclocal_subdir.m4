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
                    need_reset_flags=no
                    config_arg="FROM_MPICH=yes --with-mpl=../../mpl"
                    ;;
                src/pm/hydra)
                    config_arg="--with-mpl=../../mpl --with-hwloc=../../hwloc"
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
dnl PAC_SUBDIR_EMBED_REUSE([libname],[src_dir],[la_dir],[inc_dir])
AC_DEFUN([PAC_SUBDIR_EMBED_REUSE],[
    if test -e "$2" ; then
        dnl prevent linking libmpl.la twice
        m4_ifndef([INSIDE_ROMIO], [
            external_libs="$external_libs $3/lib$1.la"
        ])
        PAC_APPEND_FLAG([-I$4],[CPPFLAGS])
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

dnl **** wrapper for configure options with --with-[libname] ****
dnl allowed options are: embedded, embed_reuse (../path), and prefix
dnl uses macros from aclocal_libs.m4

dnl following macros are used together:
dnl PAC_SUBDIR_CONFIG([name],[header.h],[lib],[func])
AC_DEFUN([PAC_SUBDIR_CONFIG], [
    have_$1=no
    case $with_$1 in
        ../*)
            have_$1=embed_reuse
            ;;
        embedded)
            ;;
        *)
            dnl FIXME: exclue empty case to make embedded default
            PAC_CHECK_HEADER_LIB_ERROR([$1],[$2],[$3],[$4])
            ;;
    esac
])

dnl ---- always required, use embedded unless supplied with --with-xxx=prefix
dnl PAC_SUBDIR_CONFIG_EMBED([name],[la_name],[src_dir],[la_dir],[inc_dir])
AC_DEFUN([PAC_SUBDIR_CONFIG_EMBED],[
    # if have_$1 is yes, the library is already configured and added to LIBS
    # otherwise, use embedded. 
    if test $have_$1 = no ; then
        with_$1=embedded
    fi
    PAC_SUBDIR_CONFIG_EMBED_OPTIONAL([$1],[$2],[$3],[$4],[$5])
])

dnl ---- optional, used embedded only with --with-xxx=embedded
dnl PAC_SUBDIR_CONFIG_EMBED_OPTIONAL([name],[la_name],[src_dir],[la_dir],[inc_dir])
AC_DEFUN([PAC_SUBDIR_CONFIG_EMBED_OPTIONAL],[
    if test $have_$1 = embed_reuse ; then
        m4_ifval($4,[inc_dir=$with_$1/$4],[inc_dir=$with_$1])
        m4_ifval($5,[la_dir=$with_$1/$5],[la_dir=$with_$1])
        PAC_SUBDIR_EMBED_REUSE([$2],[$with_$1],[$inc_dir],[$la_dir])
        have_$1=yes
    elif test x$with_$1 = xembedded ; then
        m4_ifval($4,[inc_dir=$3/$4],[inc_dir=$3])
        m4_ifval($5,[la_dir=$3/$5],[la_dir=$3])
        PAC_SUBDIR_EMBED([$2],[$3],[$inc_dir],[$la_dir])
        have_$1=yes
    fi
])
