dnl PAC_RUNLOG_IFELSE(COMMMAND,[ACTION-IF-RUN-OK],[ACTION-IF-RUN-FAIL])
AC_DEFUN([PAC_RUNLOG_IFELSE],[AS_IF([AC_RUN_LOG([$1])],[$2],[$3])])

dnl PAC_COMPILE_IFELSE_LOG is a wrapper around AC_COMPILE_IFELSE with the
dnl output of ac_compile to a specified logfile instead of AS_MESSAGE_LOG_FD
dnl
dnl PAC_COMPILE_IFELSE_LOG(logfilename, input,
dnl                        [action-if-true], [action-if-false])
dnl
AC_DEFUN([PAC_COMPILE_IFELSE_LOG],[
PAC_PUSH_FLAG([ac_compile])
ac_compile="`echo $ac_compile | sed -e 's|>.*$|> $1 2>\&1|g'`"
AC_COMPILE_IFELSE([$2],[$3],[$4])
PAC_POP_FLAG([ac_compile])
])

dnl PAC_LINK_IFELSE_LOG is a wrapper around AC_LINK_IFELSE with the
dnl output of ac_link to a specified logfile instead of AS_MESSAGE_LOG_FD
dnl
dnl PAC_LINK_IFELSE_LOG(logfilename, input,
dnl                     [action-if-true], [action-if-false])
dnl
AC_DEFUN([PAC_LINK_IFELSE_LOG],[
PAC_PUSH_FLAG([ac_link])
ac_link="`echo $ac_link | sed -e 's|>.*$|> $1 2>\&1|g'`"
AC_LINK_IFELSE([$2],[$3],[$4])
PAC_POP_FLAG([ac_link])
])

dnl PAC_COMPLINK_IFELSE (input1, input2, [action-if-true], [action-if-false])
dnl
dnl where input1 and input2 are either AC_LANG_SOURCE or AC_LANG_PROGRAM
dnl enclosed input programs.
dnl
dnl The macro first compiles input1 and uses the object file created
dnl as part of LIBS during linking.
dnl
AC_DEFUN([PAC_COMPLINK_IFELSE],[
AC_COMPILE_IFELSE([$1],[
    AC_RUN_LOG([mv conftest.$OBJEXT pac_conftest.$OBJEXT])
    PAC_PUSH_FLAG([LIBS])
    LIBS="pac_conftest.$OBJEXT $LIBS"
    AC_LINK_IFELSE([$2],[$3],[$4])
    PAC_POP_FLAG([LIBS])
    rm -f pac_conftest.$OBJEXT]),[$4]
])
