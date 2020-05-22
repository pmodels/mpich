[#] start of __file__

##### prereq code
AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[])

##### core content
AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

##########################################################################
##### capture user arguments
##########################################################################

##### allow selection of datatype engine
AC_ARG_WITH([datatype-engine],
            [AS_HELP_STRING([--with-datatype-engine={yaksa|dataloop|auto}],[controls datatype engine to use])],
            [],[with_datatype_engine=auto])
if test "${with_datatype_engine}" = "yaksa" ; then
    AC_DEFINE_UNQUOTED(MPICH_DATATYPE_ENGINE,MPICH_DATATYPE_ENGINE_YAKSA,[Datatype engine])
elif test "${with_datatype_engine}" = "dataloop" ; then
    AC_DEFINE_UNQUOTED(MPICH_DATATYPE_ENGINE,MPICH_DATATYPE_ENGINE_DATALOOP,[Datatype engine])
else
    if test "$device_name" = "ch4" ; then
        AC_DEFINE_UNQUOTED(MPICH_DATATYPE_ENGINE,MPICH_DATATYPE_ENGINE_YAKSA,[Datatype engine])
        with_datatype_engine=yaksa
    else
        AC_DEFINE_UNQUOTED(MPICH_DATATYPE_ENGINE,MPICH_DATATYPE_ENGINE_DATALOOP,[Datatype engine])
        with_datatype_engine=dataloop
    fi
fi
AM_CONDITIONAL([BUILD_YAKSA_ENGINE], [test "${with_datatype_engine}" = "yaksa"])
AM_CONDITIONAL([BUILD_DATALOOP_ENGINE], [test "${with_datatype_engine}" = "dataloop"])

])dnl end _BODY

[#] end of __file__
