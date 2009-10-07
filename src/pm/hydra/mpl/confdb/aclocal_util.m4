dnl Nesting safe macros for saving variables
dnl Usage: PAC_PUSH_VAR(CFLAGS)
AC_DEFUN([PAC_PUSH_VAR],[
	if test -z "${pac_save_$1_nesting}" ; then
	   pac_save_$1_nesting=0
	fi
	eval pac_save_$1_${pac_save_$1_nesting}='"$$1"'
	pac_save_$1_nesting=`expr ${pac_save_$1_nesting} + 1`
])

dnl Usage: PAC_POP_VAR(CFLAGS)
AC_DEFUN([PAC_POP_VAR],[
	pac_save_$1_nesting=`expr ${pac_save_$1_nesting} - 1`
	eval $1="\$pac_save_$1_${pac_save_$1_nesting}"
	eval pac_save_$1_${pac_save_$1_nesting}=""
])

dnl Usage: PAC_SAVE_FLAGS
AC_DEFUN([PAC_SAVE_FLAGS],[
	PAC_PUSH_VAR(CFLAGS)
	PAC_PUSH_VAR(CXXFLAGS)
	PAC_PUSH_VAR(FFLAGS)
	PAC_PUSH_VAR(F90FLAGS)
	PAC_PUSH_VAR(LDFLAGS)
	PAC_PUSH_VAR(LIBS)
])

dnl Usage: PAC_RESTORE_FLAGS
AC_DEFUN([PAC_RESTORE_FLAGS],[
	PAC_POP_VAR(CFLAGS)
	PAC_POP_VAR(CXXFLAGS)
	PAC_POP_VAR(FFLAGS)
	PAC_POP_VAR(F90FLAGS)
	PAC_POP_VAR(LDFLAGS)
	PAC_POP_VAR(LIBS)
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
