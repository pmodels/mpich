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
