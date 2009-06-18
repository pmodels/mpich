dnl
dnl PAC_MPICH2_INIT - Initialization routine for top-level mpich2 configure.in.
dnl                   Call before invocation of mpich2's subpackage configure
dnl
AC_DEFUN([PAC_MPICH2_INIT],[
# Exporting the MPICH2_INTERNAL_xFLAGS with modified xFLAGS
# before calling subconfigure.
# Check if the env variable MPICH2_UNIQ_XFLAGS is set to no.
# MPICH2_UNIQ_XFLAGS is default to yes. It is a way to check
# if uniq'ed MPICH2_EXTRA_xFLAGS messes up xFLAGS.
pac_replace=${MPICH2_UNIQ_XFLAGS:-yes}
# Use user-supplied flags, WRAPPER_xFLAGS, and uniq'ed MPICH2_EXTRA_xFLAGS.
    if test "$pac_replace" = "yes" ; then
        CFLAGS="$WRAPPER_CFLAGS $MPICH2_EXTRA_CFLAGS"
        CXXFLAGS="$WRAPPER_CXXFLAGS $MPICH2_EXTRA_CXXFLAGS"
        FFLAGS="$WRAPPER_FFLAGS $MPICH2_EXTRA_FFLAGS"
        F90FLAGS="$WRAPPER_F90FLAGS $MPICH2_EXTRA_F90FLAGS"
    fi
    MPICH2_INTERNAL_CFLAGS="$CFLAGS"
    MPICH2_INTERNAL_CXXFLAGS="$CXXFLAGS"
    MPICH2_INTERNAL_FFLAGS="$FFLAGS"
    MPICH2_INTERNAL_F90FLAGS="$F90FLAGS"
    export MPICH2_INTERNAL_CFLAGS
    export MPICH2_INTERNAL_CXXFLAGS
    export MPICH2_INTERNAL_FFLAGS
    export MPICH2_INTERNAL_F90FLAGS
# Not sure if we need AC_SUBST(MPICH2_INTERNAL_xFLAGS)
])dnl
dnl
dnl
dnl PAC_SUBCONFIG_INIT - Initialization routine for subpackge configure.in
dnl                      Called after AC_INIT before any of xFLAGS is accessed.
dnl
AC_DEFUN([PAC_SUBCONFIG_INIT],[
# Initialize xFLAGS with MPICH2_INTERNAL_xFLAGS.
  if test "$FROM_MPICH2" = "yes" ; then
    CFLAGS="$MPICH2_INTERNAL_CFLAGS"
    CXXFLAGS="$MPICH2_INTERNAL_CXXFLAGS"
    FFLAGS="$MPICH2_INTERNAL_FFLAGS"
    F90FLAGS="$MPICH2_INTERNAL_F90FLAGS"
  fi
])dnl
dnl
dnl Do we need PAC_SUBCONFIG_FINALIZE or PAC_MPICH2_FINALIZE ?
dnl 
AC_DEFUN([PAC_SUBCONFIG_FINALIZE],[
])dnl
AC_DEFUN([PAC_MPICH2_FINALIZE],[
])dnl
