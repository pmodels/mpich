[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
])
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
])

dnl The following code is migrated from configure.ac
dnl The rlog is currently broken, therefore it is parked here.
dnl FIXME:
dnl     1. fix maint/genstates.in. It cannot rely on FUNCNAME macros as they are being removed.
dnl     2. fix rlog build.
dnl     3. move the following macros into PAC_SUBCFG_BODY_

dnl parking. PAC_CFG_LOGGING is not being used.
AC_DEFUN([PAC_CFG_LOGGING], [
# ----------------------------------------------------------------------------
# enable-timing and with-logging
#
# Still to do: add subsets: e.g., class=pt2pt,class=coll.  See mpich doc
#
# Logging and timing are intertwined.  If you select logging, you
# may also need to select a timing level.  If no timing is selected
# but logging with rlog is selected, make "all" the default timing level.
#
# FIXME: make timing and logging options work more cleanly together,
# particularly when other logging options are selected (e.g., logging is not
# rlog).
# ----------------------------------------------------------------------------
AM_CONDITIONAL([BUILD_LOGGING_RLOG],[test "X$with_logging" = "Xrlog"])
collect_stats=false
logging_required=false
if test "$enable_timing" = "default" ; then
    if test "$with_logging" = "rlog" ; then
        enable_timing=all
    fi
fi
timing_name=$enable_timing
case "$enable_timing" in
    no)
    timing_name=none
    ;;
    time)
    collect_stats=true
    ;;
    log|log_detailed)
    logging_required=true
    ;;
    yes)
    timing_name=all
    collect_stats=true
    logging_required=true
    ;;
    all|runtime)
    collect_stats=true
    logging_required=true
    ;;
    none|default)
    timing_name=none
    ;;
    *)
    AC_MSG_WARN([Unknown value $enable_timing for enable-timing])
    enable_timing=no
    timing_name=none
    ;;
esac
#
# The default logging package is rlog; you can get it by
# specifying --with-logging or --with-logging=rlog
#
case $with_logging in
    yes)
    logging_name=rlog
    ;;
    no|none)
    logging_name=none
    ;;
    default)
    if test "$logging_required" = "true" ; then
        logging_name=rlog
    else
        logging_name=none
    fi
    ;;
    *)
    logging_name=$with_logging
    ;;
esac
#
# Include the selected logging subsystem
#
# Choices:
# 1) A subdir of src/util/logging
#     This directory must contain a configure which will be executed
#     to build the
# 2) An external directory
#     This directory must contain
#          a mpilogging.h file
#     It may contain
#          a setup_logging script
#          a configure
#
#
logging_subsystems=
if test "$logging_name" != "none" ; then
    # Check for an external name (directory containing a /)
    hasSlash=`echo A$logging_name | sed -e 's%[[^/]]%%g'`
    if test -n "$hasSlash" ; then
        # Check that the external logging system is complete.
	# Any failure will cause configure to abort
        if test ! -d $logging_name ; then
	    AC_MSG_ERROR([External logging directory $logging_name not found.  Configure aborted])
	    logging_name=none
        elif test ! -s $logging_name/mpilogging.h ; then
	    AC_MSG_ERROR([External logging header $logging_name/mpilogging.h not found.  Configure aborted])
	    logging_name=none
        fi

        logdir=$logging_name
	# Force the logdir to be absolute
	logdir=`cd $logdir && pwd`
	# Switch name to "external" because that is how the MPICH
	# code will know it
	logging_name=external
	# Add the dir to the include paths
	#CPPFLAGS="$CPPFLAGS -I$logdir"
	CPPFLAGS="$CPPFLAGS -I$logdir"
	# Add to the list of external modules to setup
	if test -x $logdir/setup_logging ; then
	     EXTERNAL_SETUPS="$EXTERNAL_SETUPS $logdir/setup_logging"
	fi
    else
        logdir=$srcdir/src/util/logging
        logreldir=src/util/logging/$logging_name
        logging_subsystems="$logging_subsystems $logreldir"
        for dir in $logging_name ; do
            if test ! -d $logdir/$dir ; then
	        AC_MSG_ERROR([$logdir/$dir does not exist.  Configure aborted])
	        logging_name=none
            fi
        done
        for dir in $logging_subsystems ; do
            if test ! -x $srcdir/$dir/configure ; then
	        AC_MSG_ERROR([$srcdir/$dir has no configure (required).  Configure aborted])
	        logging_name=none
            fi
        done
    fi
fi
#
# FIXME: Logging doesn't necessarily require timing (e.g., simply logging the
# sequence of routines).
if test "$logging_name" != "none" ; then
    if test "$enable_timing" != "no" ; then
	if test "$enable_timing" = "default" -o "$enable_timing" = "none" ; then
	    enable_timing=log
	    timing_name=log
    	fi
	subsystems="$subsystems $logging_subsystems"
    else
	AC_MSG_WARN([Timing was disabled.  Logging has been disabled as well.])
	with_logging=no
	logging_name=none
    fi
else
    if test "$logging_required" = "true" ; then
	AC_MSG_WARN([Timing was enabled with log option but no logging library is available.  Timing has been disabled.])
	enable_timing=no
	timing_name=none
    fi
fi
if test "$timing_name" != "none" ; then
    timing_kind=`echo $timing_name | \
       tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
    timing_kind=MPICH_TIMING_KIND__$timing_kind
    AC_DEFINE_UNQUOTED(HAVE_TIMING,$timing_kind,[define to enable timing collection])
    if test "$collect_stats" = "true" ; then
        AC_DEFINE(COLLECT_STATS,1,[define to enable collection of statistics])
    fi
fi

use_logging_variable="MPICH_LOGGING__`echo $logging_name | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`"
AC_DEFINE_UNQUOTED(USE_LOGGING,$use_logging_variable,[define to choose logging library])
# ----------------------------------------------------------------------------
# End of logging tests
# ----------------------------------------------------------------------------

])
