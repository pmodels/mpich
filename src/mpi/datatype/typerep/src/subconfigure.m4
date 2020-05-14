[#] start of __file__

##### prereq code
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[])

##### core content
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

##########################################################################
##### capture user arguments
##########################################################################

# --with-typerep
AC_ARG_WITH([typerep],AS_HELP_STRING([--with-typerep={dataloop|yaksa}],[typerep backend]),,[with_typerep=dataloop])
if test ${with_typerep} != "dataloop" ; then
    AC_MSG_ERROR([no supported typerep backend specified])
fi
AM_CONDITIONAL([BUILD_TYPEREP_DATALOOP], [test x${with_typerep} = x"dataloop"])

])dnl end _BODY

[#] end of __file__
