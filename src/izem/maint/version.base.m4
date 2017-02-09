dnl
dnl this m4 file expects to be processed with "autom4te -l M4sugar"
dnl
m4_init

dnl get the real version values
m4_include([maint/version.m4])dnl

dnl The m4sugar langauage switches the default diversion to "KILL", and causes
dnl all normal output to be thrown away.  We switch to the default (0) diversion
dnl to restore output.
m4_divert_push([])dnl

dnl now provide shell versions so that simple scripts can still use
dnl $ZM_VERSION
ZM_VERSION=ZM_VERSION_m4
export ZM_VERSION

dnl balance our pushed diversion
m4_divert_pop([])dnl
