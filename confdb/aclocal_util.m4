dnl NOTE: do not use these macros recursively, you will be very sad
dnl FIXME this probably should be modified to take a namespacing parameter
AC_DEFUN([PAC_SAVE_FLAGS],[
	pac_save_CFLAGS=$CFLAGS
	pac_save_CXXFLAGS=$CXXFLAGS
	pac_save_FFLAGS=$FFLAGS
	pac_save_F90FLAGS=$F90FLAGS
	pac_save_LDFLAGS=$LDFLAGS
])

dnl NOTE: do not use these macros recursively, you will be very sad
dnl FIXME this probably should be modified to take a namespacing parameter
AC_DEFUN([PAC_RESTORE_FLAGS],[
	CFLAGS=$pac_save_CFLAGS
	CXXFLAGS=$pac_save_CXXFLAGS
	FFLAGS=$pac_save_FFLAGS
	F90FLAGS=$pac_save_F90FLAGS
	LDFLAGS=$pac_save_LDFLAGS
])

dnl Usage: PAC_APPEND_FLAG([-02], [$CFLAGS])
dnl need a clearer explanation and definition of how this is called
AC_DEFUN([PAC_APPEND_FLAG],[
	AC_REQUIRE([AC_PROG_FGREP])
	AS_IF(
		[echo "$$2" | $FGREP -e '$1' >/dev/null 2>&1],
		[echo "$2(='$$2') contains '$1', not appending" >&AS_MESSAGE_LOG_FD],
		[echo "$2(='$$2') does not contain '$1', appending" >&AS_MESSAGE_LOG_FD
		$2="$$2 $1"]
	)
])
