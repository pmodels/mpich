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

# YAKSA
AC_ARG_VAR([YAKSALIBNAME],[can be used to override the name of the YAKSA library (default: "yaksa")])
YAKSALIBNAME=${YAKSALIBNAME:-"yaksa"}
export YAKSALIBNAME
AC_SUBST(YAKSALIBNAME)
yaksasrcdir=""
AC_SUBST([yaksasrcdir])
yaksalibdir=""
AC_SUBST([yaksalibdir])
yaksalib=""
AC_SUBST([yaksalib])

AM_COND_IF([BUILD_YAKSA_ENGINE], [
m4_define([yaksa_embedded_dir],[modules/yaksa])
PAC_CHECK_HEADER_LIB_EXPLICIT([yaksa],[yaksa.h],[$YAKSALIBNAME],[yaksa_init])
if test "$with_yaksa" = "embedded" ; then
    yaksalib="modules/yaksa/lib${YAKSALIBNAME}.la"
    if test ! -e "${use_top_srcdir}/modules/PREBUILT" -o ! -e "$yaksalib"; then
        PAC_PUSH_ALL_FLAGS()
        PAC_RESET_ALL_FLAGS()
        # no need for libtool versioning when embedding YAKSA
        yaksa_subdir_args="--enable-embedded"
        PAC_CONFIG_SUBDIR_ARGS([modules/yaksa],[$yaksa_subdir_args],[],[AC_MSG_ERROR(YAKSA configure failed)])
        PAC_POP_ALL_FLAGS()
        yaksasrcdir="modules/yaksa"
    fi
    PAC_APPEND_FLAG([-I${main_top_builddir}/modules/yaksa/src/frontend/include], [CPPFLAGS])
    PAC_APPEND_FLAG([-I${use_top_srcdir}/modules/yaksa/src/frontend/include], [CPPFLAGS])
fi
])

])dnl end _BODY

[#] end of __file__
