dnl
dnl Caching is usually WRONG for systems with cross-mounted file systems
dnl (the cache file may correspond to a different system).  Since configure
dnl is not on a performance-critical path, go for robustness over speed.
dnl
define([AC_CACHE_LOAD], )dnl
define([AC_CACHE_SAVE], )dnl
dnl
dnl
AC_INIT()
AC_PREREQ(2.52)
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_HEADER(mpe_conf.h)
#
echo "Configuring MPE Profiling System with $ac_configure_args"
dnl
dnl  Set top_srcdir_abs to be the ABSOLUTE path to the home directory of MPE
dnl  top_srcdir is in relative path.
if test "X$srcdir" != "X." -a -s $srcdir/include/mpe.h ; then
    top_srcdir_abs=$srcdir
else
    PAC_GETWD( top_srcdir_abs, include/mpe.h )
fi
AC_SUBST(top_srcdir_abs)

# Determine this package's name based on its directory name
changequote(,)dnl
tmpname="`echo $top_srcdir_abs | sed -e 's%\(.*\)/\([^/]*\)%\2%'`"
pkgname="`echo $tmpname | sed -e 'y%abcdefghijklmnopqrstuvwxyz/%ABCDEFGHIJKLMNOPQRSTUVWXYZ_%'`"
changequote([,])dnl
if test "x$pkgname" = "x" ; then
    pkgname="MPE-2"
fi

AC_ARG_ENABLE( echo,
[--enable-echo                      - Turn on strong echoing.
                                     The default is enable=no.],
set -x )

AC_ARG_ENABLE( softerror,
[--enable-softerror                 - Turn on soft error, i.e. instead of
                                     issuing MSG_ERROR, MSG_WARN + exit 0
                                     are used.  The default is enable=no.],
, enable_softerror=${mpe_enable_softerror:-no} )

dnl Don't preset F77 when MPE is configured with MPICH2 
dnl otherwise AC_PROG_F77 or other PAC F77 tests could be messed up below.
dnl This is especially true when no fortran compiler in PATH.
VERSION=""
if test "$FROM_MPICH2" != "yes" ; then
    CC=${CC:-cc}
    F77=${F77:-f77}
    CFLAGS=${CFLAGS}
    FFLAGS=${FFLAGS}
fi
MPE_LIBNAME=""

dnl Read FROM_MPICHx variables from the environment
MAKE=${MAKE:-make}
FROM_MPICH=${FROM_MPICH:-no}
FROM_MPICH2=${FROM_MPICH2:-no}
FROM_LAM=${FROM_LAM:-no}


dnl Fixup for make
PAC_MAKE_IS_GNUMAKE
PAC_MAKE_IS_BSD44
PAC_MAKE_IS_OSF
PAC_MAKE_VPATH
AC_SUBST(MAKE)
 
dnl If we are relying on vpath and no vpath is set, then we must exit
if test ! -s include/mpe.h -a -z "$VPATH" ; then
    AC_MSG_ERROR( [No virtual MAKE path command found.
	You may need to set your make command
	The GNU make (sometimes available as gnumake) can be used.] )
fi

dnl CPRP is the version of cp that accepts -r and -p arguments.
MKDIR=mkdir
CPRP=cp
RM=rm
MV=mv
AC_SUBST(MKDIR)
AC_SUBST(CPRP)
AC_SUBST(RM)
AC_SUBST(MV)

AC_ARG_ENABLE( strict,
[--enable-strict                    - Turn on strict compilation testing
                                     when using gcc],,enable_strict=no )
dnl
AC_ARG_ENABLE( g,
[--enable-g                         - Turn on/off the compilation flag -g.
                                     The default is enable=no.],
[if test "$enable_g" != "no" ; then enable_g=yes ; fi],
[enable_g=no] )

dnl MPE_BUILD_FORTRAN2C=yes means that ALL subpackages will be building
dnl the fortran2c wrapper libraries.  This does not necessarily mean that
dnl specific subpackage has to build fortran2c wrapper library.  One example:
dnl src/wrappers may have --with-f2cmpilibs set by user, so src/wrappers's
dnl own libmpe_f2cmpi.a will not be built but MPE_BUILD_FORTRAN2C could
dnl still be set to yes because it means other subpackages like src/graphics
dnl and src/logging will build their fortran2c wrapper objects.  So treat
dnl MPE_BUILD_FORTRAN2C as a directive, i.e. generally a necessary but
dnl insufficient condition to build fortran2c wrapper library.
AC_ARG_ENABLE( f77,
[--enable-f77                       - Compile routines that require a Fortran
                                     compiler.  If configuring with MPICH, the
                                     configure in the top-level MPICH directory
                                     will choose the appropriate value for you.
                                     The default is enable=yes.], ,
enable_f77=yes )
if test "$enable_f77" = "yes" ; then
    MPE_BUILD_FORTRAN2C=yes
else
    MPE_BUILD_FORTRAN2C=no
fi

AC_ARG_ENABLE( mpich,
[--enable-mpich                     - Indicates if this MPE will be built as
                                     part of the MPICH build.  The default is
                                     enable=no.], ,
enable_mpich=no )

dnl  Translating FROM_MPICHx into MPI_IMPL.  So don't pass FROM_MPICHx down
dnl  to subpackages.  All subpackages will use MPI_IMPL instead.
dnl  BUILDING_MPI=yes disables the linkage test during building, i.e. make
dnl  BUILDING_MPI=yes when MPI_IMPL=MPICH or MPI_IMPL=MPICH2
dnl  MPE_KNOWN_MPI_IMPL can be tested by testing MPI_IMPL = MPICH, MPICH2
if test "$enable_mpich" = "yes" -o "$FROM_MPICH" = "yes" ; then
    AC_MSG_RESULT([This MPE build is part of the MPICH build])
    MPI_IMPL="MPICH"
    enable_softerror=yes
    enable_checkMPI=no
dnl BUILDING_MPI=yes & MPE_KNOWN_MPI_IMPL=yes
elif test "$FROM_MPICH2" = "yes" ; then
    AC_MSG_RESULT([This MPE build is part of the MPICH2 build])
    MPI_IMPL="MPICH2"
    if test -d "$includebuild_dir" ; then
        MPICPPFLAGS="$MPICPPFLAGS -I$includebuild_dir"
    else
        MPICPPFLAGS="$MPICPPFLAGS -I$master_top_srcdir/src/include -I$master_top_builddir/src/include"
    fi
    enable_softerror=yes
    enable_checkMPI=no
dnl BUILDING_MPI=yes & MPE_KNOWN_MPI_IMPL=yes
dnl elif test "$FROM_LAM" = yes ; then
dnl    AC_MSG_RESULT([This MPE build is part of the LAM build])
dnl    MPI_IMPL="LAM"
dnl BUILDING_MPI=yes & MPE_KNOWN_MPI_IMPL=yes
else
    AC_MSG_RESULT([User supplied MPI implmentation (Good Luck!)])
    MPI_IMPL="UserSupplied"
    enable_checkMPI=yes
dnl BUILDING_MPI=no & MPE_KNOWN_MPI_IMPL=no
fi

AC_ARG_ENABLE( checkMPI,
[--enable-checkMPI                  - Turn on/off the linking test for the MPI
                                     include files, MPI libraries and MPI_CC
                                     compiler. The default is enable=yes.],
enable_checkMPI=${mpe_enable_checkMPI:-$enableval} )

AC_ARG_ENABLE( checkMPIwtime,
[--enable-checkMPIwtime             - Turn on/off the linking test for the
                                     MPI_Wtime(). The default is enable=no.], ,
enable_checkMPIwtime=no )

dnl
dnl  Set the MPE Libraries name
dnl
AC_ARG_WITH( mpelibname,
[--with-mpelibname=MPE_LIBNAME      - Set the MPE library name instead of the
                                     default 'mpe', e.g. if MPE_LIBNAME=mpe_new,
                                     then the libraries generated will be
                                     libmpe_new.a, liblmpe_new.a, libtmpe_new.a
                                     and libampe_new.a.],
MPE_LIBNAME="$withval" )

dnl
dnl  Set the Fortran Include Library Leading Characters
dnl
AC_ARG_WITH( finc_path_leader,
[--with-finc_path_leader=FINC_PATH_LEADER - Specify a Fortran include path
                                     leader for compilation,
                                     default value is -I.],
FINC_PATH_LEADER="$withval" )
dnl
AC_ARG_WITH( flib_path_leader,
[--with-flib_path_leader=FLIB_PATH_LEADER - Specify a Fortran library path
                                     leader for linking, default value is -L.],
FLIB_PATH_LEADER="$withval" )
dnl

dnl
dnl Check if -g is needed to appended to CFLAGS and FFLAGS
dnl
dnl Append "-g" to xFLAGS and MPI_xFLAGS if enable_g=yes
if test "$enable_g" = "yes" ; then
    for flagname in CFLAGS FFLAGS MPI_CFLAGS MPI_FFLAGS ; do
        eval flagvalue=\$"$flagname"
        case " $flagvalue " in
           *" -g "*)
              ;;  # Avoid duplication, do nothing
           *)
              # Split eval command into 2 lines so
              # vi's hightlight mode won't get confused. 
              flagcmd="$flagname=\"$flagvalue -g\""
              eval ${flagcmd}
              ;;
        esac
    done
fi

# Set the various build directories and export them as environmental variables.
#
# [mpe_]includebuild_dir is for all user header files
# [mpe_]libbuild_dir is for building the libraries in before they are installed.
# [mpe_]binbuild_dir is for the scripts/programs
# [mpe_]sbinbuild_dir is for all system admin tools
# [mpe_]etcbuild_dir is for all system admin configuration settings
#
rootbuild_dir=`pwd`
for dir in include lib bin sbin etc ; do
    dirname=${dir}build_dir
    eval dirvalue=\$"$dirname"
    eval $dirname=$rootbuild_dir/$dir
    mpe_dirname=mpe_$dirname
    eval $mpe_dirname=\$"$dirname"
    export $mpe_dirname
done

srcbuild_dir=$rootbuild_dir/src
for dir in include lib bin sbin src etc ; do
    dirname=${dir}build_dir
    eval dirvalue=\$"$dirname"
    if test -n "$dirvalue" ; then
        if test ! -d $dirvalue ; then
            if mkdir -p $dirvalue ; then
                :
            else
                PAC_MSG_ERROR( $enable_softerror,
                               [Could not create directory $dirvalue] )
            fi
        fi
    fi
done

AC_SUBST(includebuild_dir)
AC_SUBST(libbuild_dir)
AC_SUBST(binbuild_dir)
AC_SUBST(sbinbuild_dir)
AC_SUBST(etcbuild_dir)


dnl
dnl  Unknown MPI implmentation, i.e. User defined MPI_LIBS and MPI_INC
dnl
AC_ARG_WITH( mpicc,
[--with-mpicc=MPI_CC                - C compiler for MPI program,
                                     e.g. mpixlc_r for BG/P MPI, or
                                     mpicc from MPICHx, Open-MPI, or LAM-MPI.
                                     Setting --with-mpicc to MPI wrapper
                                     means no need to set --with-mpiinc
                                     and --with-mpilibs.],
MPI_CC="$withval" )
 
AC_ARG_WITH( mpif77,
[--with-mpif77=MPI_F77              - Fortran compiler for MPI program,
                                     e.g. mpixlf77_r for BG/P MPI, or
                                     mpif77 from MPICHx, Open-MPI, or LAM-MPI.
                                     Setting --with-mpif77 to MPI wrapper
                                     means no need to set --with-mpiinc
                                     and --with-mpilibs.],
MPI_F77="$withval" )
 
AC_ARG_WITH( mpiinc,
[--with-mpiinc=MPI_INC              - MPI include path prefixed with -I.
                                     It specifies MPI header files location,
                                     where mpi.h and mpif.h are located,
                                     e.g. -I/pkgs/MPI/include. This flag will
                                     be added to MPI_CFLAGS and MPI_FFLAGS,
                                     i.e. the MPI include path can be
                                     explicitly set in MPI_CFLAGS and
                                     MPI_FFLAGS without setting --with-mpiinc.],
MPI_INC="$withval" )
 
AC_ARG_WITH( mpilibs,
[--with-mpilibs=MPI_LIBS            - MPI library flags with -L and -l.
                                     It specifies the location of the MPI
                                     libraries.  The flags contains library
                                     path prefixed with -L and MPI libraries
                                     name prefixed with -l,
                                     e.g. -L/opt/MPI/lib -lpmpich -lmpich.],
MPI_LIBS="$withval" )

dnl
misc_dirname="src/misc"
callstack_dirname="src/callstack"
logging_dirname="src/logging"
wrappers_dirname="src/wrappers"
graphics_dirname="src/graphics"
collchk_dirname="src/collchk"
slog2_dirname="src/slog2sdk"

dnl Cleanup the subpackages' Makefiles if they exist
AC_MSG_CHECKING( [for leftover Makefiles in subpackages] )
leftover_pkgs=""
for pkg in misc callstack logging wrappers graphics collchk slog2 ; do
    dirname=${pkg}_dirname
    eval dirvalue=\$"$dirname"
    if test -s $rootbuild_dir/$dirvalue/Makefile ; then
        leftover_pkgs="$leftover_pkgs \"$dirvalue\""
        ( cd $rootbuild_dir/$dirvalue && $MAKE distclean > /dev/null 2>&1 )
        dnl Just in case "make distclean" does not remove Makefile.
        if test -f $rootbuild_dir/$dirvalue/Makefile ; then
            $RM -f $rootbuild_dir/$dirvalue/Makefile
        fi
    fi
done
if test "X$leftover_pkgs" = "X" ; then
    AC_MSG_RESULT( [none] )
else
    AC_MSG_RESULT( [$leftover_pkgs cleaned] )
fi

dnl trial_pkgs="misc callstack logging wrappers graphics collchk"

dnl
AC_ARG_ENABLE( misc,
[--enable-misc                      - Build MPE miscellaneous routines, e.g.
                                     MPE_Errors_xxx(), MPE_Seq_xxx(),
                                     MPE_xxxTags() and MPE_GetHostName()....
                                     The default enable=yes.],
, enable_misc=yes )

dnl
AC_ARG_ENABLE( callstack,
[--enable-callstack                 - Build MPE callstack routines, i.e.
                                     MPE_CallStack_xxx()...
                                     The default enable=yes.],
, enable_callstack=yes )

dnl
AC_ARG_ENABLE( logging,
[--enable-logging                   - Build MPE logging routines for 
                                     CLOG-2, e.g.  MPE_Log_xxx()...
                                     The default enable=yes.],
, enable_logging=yes )

dnl
AC_ARG_ENABLE( wrappers,
[--enable-wrappers                  - Build various MPI wrapper libraries.
                                     MPI logging library, liblmpe.a;
                                     MPI tracing library, libtmpe.a;
                                     MPI Fortran to C library, libmpe_f2cmpi.a],
, enable_wrappers=yes )
dnl  src/wrappers requires at least src/logging
if test "$enable_wrappers" = "yes" ; then
    if test "$enable_logging" != "yes" ; then
        enable_logging=yes
        AC_MSG_WARN( [Enabling Logging which is required by Wrappers.] ); 
    fi
fi

dnl  Determine if the X graphics is going to be configured
AC_ARG_ENABLE( graphics,
[--enable-graphics                  - Build MPE graphics routines and MPI
                                     animation library, libampe.a.  If disabled
                                     then the MPE routines that make use of X11
                                     graphics will NOT be built.  This is
                                     appropriate for systems that either do not
                                     have the X11 include files or that do not
                                     support X11 graphics.
                                     The default enable=yes.],
, enable_graphics=yes )
dnl  src/graphics's libmpe_nompi.a requires mpehname.c defined in src/misc
dnl  but then libmpe_nompi.a is rarely used, tests in src/grahics will pass
dnl  without src/misc.  Let user decides.
if test "$enable_graphics" = "yes" -a "$enable_misc" != "yes" ; then
    AC_MSG_WARN( [libmpe_nompi.a in src/graphics needs mpehname.c in src/misc!
	If you need libmpe_nompi.a, reconfigure with --enable-misc.] )
fi

AC_ARG_ENABLE( collchk,
[--enable-collchk                   - Build MPI Collective and Datatype 
                                     checking library, libmpe_collchk.a],
, enable_collchk=yes )
dnl  src/collchk requires at least src/callstack
if test "$enable_collchk" = "yes" ; then
    if test "$enable_callstack" != "yes" ; then
        enable_callstack=yes
        AC_MSG_WARN( [Enabling CallStack which is required by CollChk.] ); 
    fi
fi

selected_pkgs=""

if test "$enable_misc" = "yes" -a -s $srcdir/src/misc/configure ; then
    selected_pkgs="$selected_pkgs misc"
    AC_CONFIG_SUBDIRS(src/misc)
fi
if test "$enable_callstack" = "yes" -a -s $srcdir/src/callstack/configure ; then
    selected_pkgs="$selected_pkgs callstack"
    AC_CONFIG_SUBDIRS(src/callstack)
fi
if test "$enable_logging" = "yes" -a -s $srcdir/src/logging/configure ; then
    selected_pkgs="$selected_pkgs logging"
    AC_CONFIG_SUBDIRS(src/logging)
fi
if test "$enable_wrappers" = "yes" -a -s $srcdir/src/wrappers/configure ; then
    selected_pkgs="$selected_pkgs wrappers"
    AC_CONFIG_SUBDIRS(src/wrappers)
fi
if test "$enable_graphics" = "yes" -a -s $srcdir/src/graphics/configure ; then
    selected_pkgs="$selected_pkgs graphics"
    AC_CONFIG_SUBDIRS(src/graphics)
fi
if test "$enable_collchk" = "yes" -a -s $srcdir/src/collchk/configure ; then
    selected_pkgs="$selected_pkgs collchk"
    AC_CONFIG_SUBDIRS(src/collchk)
fi

dnl  Determine if the slog2sdk directory is going to be configured.
AC_ARG_ENABLE( slog2,
[--enable-slog2{=build}             - Configure all the SLOG-2 tools.
                                     --enable-slog2 without any argument will
                                     use the prebuilt jar files.
                                     --enable-slog2=build will rebuild all the
                                     jar files from the source code.
                                     The default is enable=yes.], ,
enable_slog2=yes )


dnl if SLOG-2 is here, configure it
if test -s $srcdir/$slog2_dirname/configure ; then
    if test "$enable_slog2" = "yes" -o "$enable_slog2" = "build" ; then
        if test "$enable_slog2" = "build" ; then
            slog2_enable_source=yes
        else
            slog2_enable_source=no
        fi
        export slog2_enable_source
        dnl  Enable softerror in SLOG2 configure won't abort the entire process
        slog2_enable_softerror=yes
        export slog2_enable_softerror
        selected_pkgs="$selected_pkgs slog2"
        if test "$MPI_IMPL" = "MPICH" -o "$MPI_IMPL" = "MPICH2" ; then
            dnl  Disable the SAMPLE code in slog2sdk for MPICH2
            slog2_enable_sample=no
            export slog2_enable_sample
        fi
        AC_CONFIG_SUBDIRS(src/slog2sdk)
    fi
fi

dnl The following statement is no longer true as of 6/24/2007,
dnl a thread-specific MPE logging on/off switch has been added
dnl to mpe_log_thread[.h/.c] and MPI logging wrappper library
dnl has been updated accordingly.
dnl
dnl AC_ARG_ENABLE( threadlogging,
dnl [--enable-threadlogging             - Turn on/off the coarse-grained thread
dnl                                      support in MPE logging.  The default
dnl                                      is enable=no.], ,
dnl enable_threadlogging=no )
dnl
dnl When thread-logging is enabled in multi-thread environment,
dnl safePMPI guard can't be used, i.e. global variables is_mpelog_on
dnl can't be used in coarse-grained thread support,
dnl becasue these varaibles need to be thread-specific.  Otherwise
dnl turn off logging on 1 thread would mean turning it off for all
dnl threads.
dnl
dnl if test "$enable_threadlogging" = "yes" ; then
dnl     enable_safePMPI=no
dnl     export enable_safePMPI
dnl fi


AC_ARG_VAR( MPI_CC, [C compiler for MPI program, e.g. mpixlc_r for BG/P MPI, or mpicc from MPICHx, Open-MPI, or LAM-MPI.  Setting MPI_CC to MPI wrapper means no need to set MPI_INC and MPI_LIBS.] )
AC_ARG_VAR( MPI_F77, [Fortran compiler for MPI program, e.g. mpixlf77_r for BG/P MPI, or mpif77 from MPICHx, Open-MPI, or LAM-MPI.  Setting MPI_F77 to MPI wrapper means no need to set MPI_INC and MPI_LIBS.] )
AC_ARG_VAR( MPI_INC, [MPI include path prefixed with -I.  It specifies MPI header files location, where mpi.h and mpif.h are located, e.g. -I/pkgs/MPI/include. This flag will be added to MPI_CFLAGS and MPI_FFLAGS, i.e. the MPI include path can be explicitly set in MPI_CFLAGS and MPI_FFLAGS without setting MPI_INC.] )
AC_ARG_VAR( MPI_CFLAGS, [MPI C compiler flags to compile MPI program.] )
AC_ARG_VAR( MPI_FFLAGS, [MPI Fortran compiler flags to compile MPI Fortran program.] )
AC_ARG_VAR( MPI_LIBS, [MPI library flags with -L and -l. It specifies the location of the MPI libraries.  The flags contains library path prefixed with -L and MPI libraries name prefixed with -l, e.g. -L/opt/MPI/lib -lpmpich -lmpich.] )
dnl AC_ARG_VAR( MPI_LDFLAGS, [MPI linker flags used with MPI_CC or MPI_F77 during linking.] )
AC_ARG_VAR( MPERUN, [MPI application launch command.  This variable is necessary only if user wants to run "make check", "make installcheck-all" or "$sbindir/mpecheckinstall -run" when the default MPERUN value, "$bindir/mpiexec -n 4", is not correct.] )

# Prepend/Append '"' before/after MPERUN.
if test "x$MPERUN" != "x" ; then
    MPERUN="\"$MPERUN\""
fi

# Preppend MPI_INC to MPI_CFLAGS/MPI_FFLAGS
if test "x$MPI_INC" != "x" ; then
    MPI_CFLAGS="$MPI_INC $MPI_CFLAGS"
    MPI_FFLAGS="$MPI_INC $MPI_FFLAGS"
fi

dnl
AC_ARG_WITH( cross-spec,
[--with-cross-spec=file             - Specify the values of variables that
                                     configure cannot determine in a
                                     cross-compilation environment through
                                     a file], ,
with_cross_spec=no )
dnl
pac_cross_compiling=no
if test "$with_cross_spec" != "no" ; then
    if test -s "$with_cross_spec" ; then
        AC_MSG_RESULT([Reading the cross-compilation file $with_cross_spec])
        . $with_cross_spec
        # Autoconf 2.52 no longer sets cross_compiling except with the
        # awkward "targethost" options.
        pac_cross_compiling=yes
        cross_compiling=yes
        ac_cv_prog_cc_cross=yes
        ac_cv_prog_f77_cross=yes
        ac_cv_prog_f90_cross=yes
        ac_cv_prog_cxx_cross=yes
        export cross_compiling
        # Export all the variables in cross_spec/cross_spec.txt
        # i.e. CROSS_xxx, MPI_xxx and friends...
        cross_spec_vars=`grep -v '^#' $with_cross_spec | sed -e 's/=.*//g'`
        for var in $cross_spec_vars ; do
            eval value=\$"$var"
            echo "Exporting $var=$value ..."
            export $var
        done
    fi
fi


dnl
AC_ARG_WITH( config-args,
[--with-config-args=filename        - Specify configure argument file that
                                     contains the values of variables that
                                     configure reads, e.g. CC, CFLAGS, 
                                     F77, FFLAGS, MPI_CC, MPI_F77....
                                     If the filename does not begin with
                                     / (absolute path), . or .. (relative path),
                                     the filename will be assumed to be
                                     $top_srcdir/configargs/<filename>.cfg.], ,
with_config_args=no )
dnl
if test "$with_config_args" != "no" ; then
    case "$with_config_args" in
        /*|../*|./*)
            config_args_file="$with_config_args"
            ;;
        *)
            config_args_file="$srcdir/configargs/${with_config_args}.cfg"
            ;;
    esac
    if test -s "$config_args_file" ; then
        AC_MSG_RESULT([Reading the configure arguments in ${config_args_file}.])
        . $config_args_file
        # Export all the variables in $config_args_file
        # i.e. CC, F77, CFLAGS, FFLAGS and friends...
        config_args_vars=`grep -v '^#' $config_args_file | sed -e 's/=.*//g'`
        for var in $config_args_vars ; do
            eval value=\$"$var"
            echo "Exporting $var=$value ..."
            export $var
        done
    else
        AC_MSG_WARN([Non-existent ${config_args_file}.])
    fi
fi


AC_PROG_CC
dnl Check for broken handling of common symbols
dnl PAC_PROG_C_BROKEN_COMMON

dnl Adding --enable-strict flags to CFLAGS if needed.
dnl No space should be added after "(" and before ")", otherwise invalid
dnl statement, like 'CFLAGS  ="$CFLAGS ..."', will be resulted in configure.
dnl Also, PAC_GET_GCC_STRICT_FLAGS should be called after AC_PROG_CC.
if test "$FROM_MPICH2" != "yes" ; then
    MPE_COPTION=""
    PAC_GET_GCC_STRICT_FLAGS([MPE_COPTION])
    if test "x$MPE_COPTION" != "x" ; then
        CFLAGS="$CFLAGS $MPE_COPTION"
        MPI_CFLAGS="$MPI_CFLAGS $MPE_COPTION"
    fi
fi

dnl  MPI_CC is only use for MPI programs.  
dnl  CC should be the same compiler that is used within the MPI_CC
dnl  environment.  MPI_LIBS should also be set for linking MPI programs
AC_MSG_CHECKING( whether MPI_CC has been set )
if test -z "$MPI_CC" ; then
    MPI_CC="$CC $MPICPPFLAGS"
fi
AC_MSG_RESULT( $MPI_CC )

if test "$MPE_BUILD_FORTRAN2C" = "yes" ; then
    AC_PROG_F77
    if test "x$F77" = "x" ; then
        MPE_BUILD_FORTRAN2C=no
    fi
    AC_MSG_CHECKING( whether MPI_F77 has been set )
    if test -z "$MPI_F77" ; then
        # FIXME: Temporary for F77 accepting the same flags as CC for 
        # file includes
        MPI_F77="$F77 $MPICPPFLAGS"
    fi
    AC_MSG_RESULT( $MPI_F77 )
fi

# Determine the name of the various library being built
if test -z "$MPE_LIBNAME" ; then
    MPE_LIBNAME=mpe
fi



dnl
dnl  Check if MPI_CC, MPI_CFLAGS, and MPI_LIBS together provide
dnl  a linkable environment for a simple MPI program
dnl
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    if test "$enable_checkMPI" = "yes" ; then
        AC_MSG_CHECKING( [for the linkage of the supplied MPI C definitions] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, , , ,
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
        else
            AC_MSG_RESULT(no)
            PAC_MSG_ERROR( $enable_softerror,
                           [Cannot link with basic MPI C program!
	Check your MPI include paths, MPI libraries and MPI CC compiler] )
        fi
    fi
    if test "$enable_checkMPI" = "yes" -a "$MPE_BUILD_FORTRAN2C" = "yes" ; then
        AC_MSG_CHECKING( [for the linkage of the supplied MPI F77 definitions] )
        PAC_MPI_LINK_F77_FUNC( $MPI_F77, [$MPI_FFLAGS], $MPI_LIBS, [
        integer mytid
        integer ierr
        ], [
        call MPI_Comm_rank( MPI_COMM_WORLD, mytid, ierr )
        ],
        [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
        else
            AC_MSG_RESULT(no)
            PAC_MSG_ERROR( $enable_softerror,
                           [Cannot link with basic MPI F77 program!
	Check your MPI include paths, MPI libraries and MPI F77 compiler ] )
        fi
    fi

    dnl
    dnl Some MPI's seem to have trouble with
    dnl MPI_Wtime, PMPI_Wtime, and MPI_Attr_get
    dnl (Fujitsu for MPI_Wtime and MPI_Attr_get; IBM's PMPI_Wtime returned 0)
    dnl Check at least that we can link with MPI_Wtime and MPI_Attr_get
    dnl
    if test "$enable_checkMPI" = "yes" -a "$enable_checkMPIwtime" = "yes" ; then
        AC_MSG_CHECKING( [for the C version of MPI_Wtime()] )
        PAC_MPI_LINK_CC_FUNC( $MPI_CC, [$MPI_CFLAGS], $MPI_LIBS, ,
                              [ double d ],
                              [ d=MPI_Wtime() ],
                              [ mpe_link_ok=yes ], [ mpe_link_ok=no ] )
        if test "$mpe_link_ok" = "yes" ; then
            AC_MSG_RESULT(yes)
            AC_DEFINE( HAVE_MPI_WTIME,1,[Define if MPI_Wtime available] )
        else
            AC_MSG_RESULT(no)
            PAC_MSG_ERROR( $enable_softerror, [Cannot link with MPI_Wtime()!
	Check your MPI include paths, MPI libraries and MPI CC compiler ] )
        fi
    fi
fi

if test -z "$CC" -o -z "$MPI_CC"; then
    PAC_MSG_ERROR( $enable_softerror,
                   [You must configure MPE with a specific MPI] )
fi
 
if test -n "$F77" -a "$MPE_BUILD_FORTRAN2C" = "yes" ; then 
    FLIBS_save="$FLIBS"
    FLIBS=""
    AC_F77_LIBRARY_LDFLAGS
    # The autoconf macro for finding FLIBS sometimes makes mistakes
    # (particularly with the Fujitsu frt compiler).  This next step
    # first sees if the FLIBS is valid with the Fortran compiler
    PAC_PROG_F77_FLIBS_VALID
    # Now see if FLIBS works with the C compiler
    PAC_PROG_F77_CHECK_FLIBS

    PAC_PROG_F77_LINKER_WITH_C
    PAC_PROG_F77_OBJ_LINKS_WITH_C
    # For name mangle, we need the library flags
    PAC_PROG_F77_NAME_MANGLE
    # Check whether additional libraries are needed when linking with C
    PAC_PROG_F77_AND_C_STDIO_LIBS
    AC_SUBST(F77_OTHER_LIBS)

    PAC_PROG_F77_LIBRARY_DIR_FLAG
    if test -z "$FLIB_PATH_LEADER" ; then
        FLIB_PATH_LEADER=$F77_LIBDIR_LEADER
    fi
    PAC_PROG_F77_HAS_INCDIR(f77tmpdir)
    FINC_PATH_LEADER=$F77_INCDIR

    # Replace all -I in MPI_FFLAGS by $FINC_PATH_LEADER
    MPI_FFLAGS=`echo "$MPI_FFLAGS" | sed -e "s|-I|$FINC_PATH_LEADER|g"`
    # If there is MPI_FLIBS, to replace -L by $FLIB_PATH_LEADER
fi

dnl  We need to check that this has worked.  The autoconf macro is broken
AC_PROG_CPP
if test "$CPP" = "/lib/cpp" -a ! -x /lib/cpp ; then
    AC_MSG_ERROR( [configure could not find a working C preprocessor] )
fi

dnl  check for "ln -s" exists.  It is for install program, mpeinstall
AC_PROG_LN_S

dnl checking for working INSTALL
AC_PROG_INSTALL
PAC_PROG_CHECK_INSTALL_WORKS
PAC_PROG_INSTALL_BREAKS_LIBS

dnl
dnl MPE_CFLAGS/MPE_FFLAGS are used to pass MPE-modified CFLAGS/FFLAGS
dnl to other subpackages.  Can't use CFLAGS to pass this info because
dnl user may have set CFLAGS in the environment.  In that scenario,
dnl env variable CFLAGS can't be set! So subpackages cannot get the
dnl MPE-modified CFLAGS.
dnl
MPE_CFLAGS="$CFLAGS"
if test "$MPE_BUILD_FORTRAN2C" = "yes" ; then
    MPE_FFLAGS="$FFLAGS"
fi

dnl Determine if this is "UserSupplied" MPI implmentation.
dnl If so, build mpecc and mpefc.
dnl    MPE_CLINKER/MPE_FLINKER need to sync with sbin/mpeinstall
if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
    mpe_compilers=bin/mpecc
    if test "$MPE_BUILD_FORTRAN2C" = "yes" ; then
        mpe_compilers="$mpe_compilers bin/mpefc"
    fi
    MPE_CLINKER="$binbuild_dir/mpecc -"
    MPE_FLINKER="$binbuild_dir/mpefc -"
else dnl This is MPICH2.
    MPE_CLINKER="$MPI_CC -mpe="
    MPE_FLINKER="$MPI_F77 -mpe="
fi

dnl The following does not work as expected because
dnl AC_CONFIG_SUBDIRS sets the "precious" variables like CFLAGS from the
dnl settings recorded at the top of the configure. That means CFLAGS cannot
dnl be inherit from the mpich2's configure enviroment.
dnl
dnl Include --enable-strict option only if this isn't part of MPICHx build.
dnl MPE does not even need to check enable_strict_done passed down by MPICH2.
dnl if test "$MPI_IMPL" != "MPICH" -a "$MPI_IMPL" != "MPICH2" ; then
dnl     MPE_CFLAGS="$MPE_CFLAGS $STRICT_COPTS"
dnl fi
dnl
if test -z "$CLINKER" ; then CLINKER="$CC" ; fi
if test -z "$FLINKER" ; then FLINKER="$F77" ; fi
AC_SUBST(MPI_IMPL)
AC_SUBST(MPE_LIBNAME)
AC_SUBST(MPE_BUILD_FORTRAN2C)
dnl "precious" AC_SUBST(CC)
AC_SUBST(CLINKER)
AC_SUBST(MPE_CFLAGS)
dnl "precious" AC_SUBST(MPI_CC)
dnl "precious" AC_SUBST(MPI_CFLAGS)
AC_SUBST(MPE_CLINKER)
AC_SUBST(F77)
AC_SUBST(FLINKER)
AC_SUBST(MPE_FFLAGS)
dnl "precious" AC_SUBST(MPI_F77)
dnl "precious" AC_SUBST(MPI_FFLAGS)
AC_SUBST(MPE_FLINKER)
dnl "precious" AC_SUBST(MPI_LIBS)
dnl "precious" AC_SUBST(LDFLAGS)
dnl "precious" AC_SUBST(MPERUN)

dnl  Make the directories that do not contain Makefiles (should they?) but 
dnl  are needed for the subsequent directories.
$RM -f $srcdir/mpe_conf.h

export MPI_IMPL
export MPE_LIBNAME
export MPE_BUILD_FORTRAN2C
export CC
export CLINKER
export MPI_CC
export MPI_CFLAGS
export MPE_CLINKER
export MPE_CFLAGS
export F77
export FLINKER
export MPI_F77
export MPI_FFLAGS
export MPE_FLINKER
export MPE_FFLAGS
export MPI_LIBS
export LDFLAGS
export FINC_PATH_LEADER
export FLIB_PATH_LEADER

mpe_enable_softerror=yes
export mpe_enable_softerror




AC_OUTPUT_COMMANDS( [
if test ! -x ./sbin/mpeinstall -a -s ./sbin/mpeinstall ; then
    chmod u+x ./sbin/mpeinstall
fi
if test ! -x ./sbin/mpecheckinstall -a -s ./sbin/mpecheckinstall ; then
    chmod u+x ./sbin/mpecheckinstall
fi
if test ! -x ./sbin/mpecheckbuild -a -s ./sbin/mpecheckbuild ; then
    chmod u+x ./sbin/mpecheckbuild
fi
if test ! -x ./sbin/mpetestlink -a -s ./sbin/mpetestlink ; then
    chmod u+x ./sbin/mpetestlink
fi
if test ! -x ./sbin/mpetestexec -a -s ./sbin/mpetestexec ; then
    chmod u+x ./sbin/mpetestexec
fi
if test ! -x ./sbin/mpetestexeclog -a -s ./sbin/mpetestexeclog ; then
    chmod u+x ./sbin/mpetestexeclog
fi
if test ! -x ./bin/mpecc -a -s ./bin/mpecc ; then
    chmod a+x ./bin/mpecc
fi
if test ! -x ./bin/mpefc -a -s ./bin/mpefc ; then
    chmod a+x ./bin/mpefc
fi
] )
AC_OUTPUT( Makefile sbin/mpetestlink sbin/mpetestexec sbin/mpetestexeclog \
           sbin/mpeinstall sbin/mpecheckinstall sbin/mpecheckbuild \
           etc/mpe_help.conf etc/mpe_f77env.conf $mpe_compilers )

configured_pkgs=""
for pkg in $selected_pkgs ; do
    dirname=${pkg}_dirname
    eval dirvalue=\$"$dirname"
    if test -s $dirvalue/Makefile ; then
        configured_pkgs="$configured_pkgs \"$dirvalue\""
    fi
done

if test -z "$configured_pkgs" ; then
    AC_MSG_WARN([$pkgname: No sub-package has been configured])
else
    AC_MSG_RESULT([$pkgname: $configured_pkgs have been configured])
fi
