dnl PAC_RESET_ALL_FLAGS - Reset precious flags to those set by the user
AC_DEFUN([PAC_RESET_ALL_FLAGS],[
	if test "$FROM_MPICH2" = "yes" ; then
	   CFLAGS="$USER_CFLAGS"
	   CXXFLAGS="$USER_CXXFLAGS"
	   FFLAGS="$USER_FFLAGS"
	   F90FLAGS="$USER_F90FLAGS"
	   LDFLAGS="$USER_LDFLAGS"
	   LIBS="$USER_LIBS"
	fi
])

dnl PAC_RESET_LINK_FLAGS - Reset precious link flags to those set by the user
AC_DEFUN([PAC_RESET_LINK_FLAGS],[
	if test "$FROM_MPICH2" = "yes" ; then
	   LDFLAGS="$USER_LDFLAGS"
	   LIBS="$USER_LIBS"
	fi
])

dnl PAC_PREFIX_FLAGS - Save flags with a prefix
dnl Usage: PAC_PREFIX_FLAGS(WRAPPER, CFLAGS)
AC_DEFUN([PAC_PREFIX_FLAGS],[
	$1_$2=$$2
	export $1_$2
	AC_SUBST($1_$2)
])

dnl Sandbox configure
dnl Usage: PAC_CONFIG_SUBDIR(subdir,srcdir,config_args,action-if-success,action-if-failure)
AC_DEFUN([PAC_CONFIG_SUBDIR],[
        AC_MSG_NOTICE([===== configuring $1 =====])

	PAC_MKDIRS($1)
	pac_abs_srcdir=`(cd $srcdir && pwd)`

	if test -x $1/mpich2setup ; then
	   $subsys/mpich2setup
	elif test -x $pac_abs_srcdir/$1/mpich2setup ; then
	   $pac_abs_srcdir/$1/mpich2setup
	elif test -x $pac_abs_srcdir/$1/configure ; then
	   pac_subconfig_args=""
	   # Strip off the args we need to update
	   for ac_arg in $3 ; do
	       # Remove any quotes around the args (added by configure)
	       ac_narg=`expr x$ac_arg : 'x'"'"'\(.*\)'"'"`
	       pac_subconfig_args="$pac_subconfig_args $ac_narg"
	   done

	   if (cd $1 && eval $pac_abs_srcdir/$1/configure $pac_subconfig_args) ; then
	      $4
	      :
	   else
	      $5
	      :
	   fi
	fi

        AC_MSG_NOTICE([===== done with $1 configure =====])

	# Check for any localdefs files.  These may be created, so we
	# look in the local directory first.
	if test -f $1/localdefs ; then
	   . $1/localdefs
	elif test -f $pac_abs_srcdir/$1/localdefs ; then
	   . $pac_abs_srcdir/$1/localdefs
	fi
])
