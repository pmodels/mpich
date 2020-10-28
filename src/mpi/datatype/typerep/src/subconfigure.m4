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
AC_ARG_WITH([yaksa-prefix],
            [AS_HELP_STRING([[--with-yaksa-prefix[=DIR]]],
                            [use the YAKSA library installed in DIR,
                             rather than the one included in modules/yaksa.  Pass
                             "embedded" to force usage of the YAKSA source
                             distributed with MPICH.])],
            [],dnl action-if-given
            [with_yaksa_prefix=embedded]) dnl action-if-not-given

yaksasrcdir=""
AC_SUBST([yaksasrcdir])
yaksalibdir=""
AC_SUBST([yaksalibdir])
yaksalib=""
AC_SUBST([yaksalib])

if test "$with_yaksa_prefix" = "embedded" ; then
    # no need for libtool versioning when embedding YAKSA
    yaksa_subdir_args="--enable-embedded"
    PAC_CONFIG_SUBDIR_ARGS([modules/yaksa],[$yaksa_subdir_args],[],[AC_MSG_ERROR(YAKSA configure failed)])
    PAC_APPEND_FLAG([-I${main_top_builddir}/modules/yaksa/src/frontend/include], [CPPFLAGS])
    PAC_APPEND_FLAG([-I${use_top_srcdir}/modules/yaksa/src/frontend/include], [CPPFLAGS])

    yaksasrcdir="modules/yaksa"
    yaksalib="modules/yaksa/lib${YAKSALIBNAME}.la"
else
    # The user specified an already-installed YAKSA; just sanity check, don't
    # subconfigure it
    AS_IF([test -s "${with_yaksa_prefix}/include/yaksa.h"],
          [:],[AC_MSG_ERROR([the YAKSA installation in "${with_yaksa_prefix}" appears broken])])
    PAC_APPEND_FLAG([-I${with_yaksa_prefix}/include],[CPPFLAGS])
    PAC_PREPEND_FLAG([-l${YAKSALIBNAME}],[WRAPPER_LIBS])
    PAC_APPEND_FLAG([-L${with_yaksa_prefix}/lib],[WRAPPER_LDFLAGS])
    yaksalibdir="-L${with_yaksa_prefix}/lib"
fi

])dnl end _BODY

[#] end of __file__
