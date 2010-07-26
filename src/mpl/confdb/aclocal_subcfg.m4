dnl PAC_RESET_ALL_FLAGS - Reset precious flags to those set by the user
AC_DEFUN([PAC_RESET_ALL_FLAGS],[
	if test "$FROM_MPICH2" = "yes" ; then
	   CFLAGS="$USER_CFLAGS"
	   CPPFLAGS="$USER_CPPFLAGS"
	   CXXFLAGS="$USER_CXXFLAGS"
	   FFLAGS="$USER_FFLAGS"
	   FCFLAGS="$USER_FCFLAGS"
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

dnl Sandbox configure with additional arguments
dnl Usage: PAC_CONFIG_SUBDIR_ARGS(subdir,configure-args,action-if-success,action-if-failure)
dnl
dnl Note: I suspect this DEFUN body is underquoted in places, but it does not
dnl seem to cause problems in practice yet. [goodell@ 2010-05-18]
AC_DEFUN([PAC_CONFIG_SUBDIR_ARGS],[
        AC_MSG_NOTICE([===== configuring $1 =====])

	PAC_MKDIRS($1)
	pac_abs_srcdir=`(cd $srcdir && pwd)`

	if test -f $pac_abs_srcdir/$1/setup ; then
	   . $pac_abs_srcdir/$1/setup
	fi

        pac_subconfigure_file="$pac_abs_srcdir/$1/configure"
	if test -x $pac_subconfigure_file ; then
	   pac_subconfig_args="$2"
	   prev_arg=""

	   # Strip off the args we need to update
	   for ac_arg in $ac_configure_args ; do

	       # HACK: Though each argument in ac_configure_args is
	       # quoted to account for spaces, when we get them, the
	       # argument list is treated as a space separated list
	       # and the quotes are treated as a part of the
	       # argument. So, we explicitly look for the quotes and
	       # recreate the arguments. If/when this is fixed by
	       # autoconf, this hack can be deleted.
	       end_char=`echo $ac_arg | sed -e 's/.*\(.\)$/\1/g'`

	       # If the previous argument was incomplete, we append
	       # this argument to that
	       if test "$prev_arg" != "" -o "$end_char" != "'" ; then
                  prev_arg="$prev_arg $ac_arg"
	       fi

	       # If the end character is a quote, we are at the end of
	       # an argument.
	       if test "$end_char" = "'" ; then
                  # If the previous argument just got completed, set
		  # the current argument to the entire argument value
		  if test "$prev_arg" != "" ; then ac_arg="$prev_arg" ; fi
		  prev_arg=
	       else
		  # If the previous argument hasn't completed yet,
		  # just continue on to the next argument
		  continue
	       fi

	       # Remove any quotes around the args (added by configure)
	       ac_narg=`echo $ac_arg | sed -e "s/^'\(.*\)'$/\1/g"`

	       case $ac_narg in
                   CFLAGS=*)
		       pac_subconfig_args="$pac_subconfig_args CFLAGS='$CFLAGS'"
		       ;;
		   CPPFLAGS=*)
		       pac_subconfig_args="$pac_subconfig_args CPPFLAGS='$CPPFLAGS'"
		       ;;
		   CXXFLAGS=*)
		       pac_subconfig_args="$pac_subconfig_args CXXFLAGS='$CXXFLAGS'"
		       ;;
		   FFLAGS=*)
		       pac_subconfig_args="$pac_subconfig_args FFLAGS='$FFLAGS'"
		       ;;
		   FCFLAGS=*)
		       pac_subconfig_args="$pac_subconfig_args FCFLAGS='$FCFLAGS'"
		       ;;
		   LDFLAGS=*)
		       pac_subconfig_args="$pac_subconfig_args LDFLAGS='$LDFLAGS'"
		       ;;
		   LIBS=*)
		       pac_subconfig_args="$pac_subconfig_args LIBS='$LIBS'"
		       ;;
		   *)
		       # We store ac_arg instead of ac_narg to make
		       # sure we retain the quotes as provided to us
		       # by autoconf
		       pac_subconfig_args="$pac_subconfig_args $ac_arg"
		       ;;
	       esac
	   done

           dnl Add option to disable configure options checking
           if test "$enable_option_checking" = no ; then
              pac_subconfig_args="$pac_subconfig_args --disable-option-checking"
           fi

	   AC_MSG_NOTICE([executing: $pac_subconfigure_file $pac_subconfig_args])
	   if (cd $1 && eval $pac_subconfigure_file $pac_subconfig_args) ; then
	      $3
	      :
	   else
	      $4
	      :
	   fi
        else
           if test -e $pac_subconfigure_file ; then
               AC_MSG_WARN([$pac_subconfigure_file exists but is not executable])
           else
               AC_MSG_WARN([$pac_subconfigure_file does not exist])
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

dnl Sandbox configure
dnl Usage: PAC_CONFIG_SUBDIR(subdir,action-if-success,action-if-failure)
AC_DEFUN([PAC_CONFIG_SUBDIR],[PAC_CONFIG_SUBDIR_ARGS([$1],[],[$2],[$3])])

