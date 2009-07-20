dnl PAC_MPICH2_INIT - Initialization routine for top-level mpich2 configure.in.
dnl                   Call before invocation of mpich2's subpackage configure
AC_DEFUN([PAC_MPICH2_INIT],[
# Exporting the MPICH2_INTERNAL_xFLAGS with modified xFLAGS
# before calling subconfigure.
    MPICH2_INTERNAL_CFLAGS="$CFLAGS"
    MPICH2_INTERNAL_CXXFLAGS="$CXXFLAGS"
    MPICH2_INTERNAL_FFLAGS="$FFLAGS"
    MPICH2_INTERNAL_F90FLAGS="$F90FLAGS"
    export MPICH2_INTERNAL_CFLAGS
    export MPICH2_INTERNAL_CXXFLAGS
    export MPICH2_INTERNAL_FFLAGS
    export MPICH2_INTERNAL_F90FLAGS
# Not sure if we need AC_SUBST(MPICH2_INTERNAL_xFLAGS)
])


dnl PAC_SUBCONFIG_INIT - Initialization routine for subpackge configure.in
dnl                      Called after AC_INIT before any of xFLAGS is accessed.
AC_DEFUN([PAC_SUBCONFIG_INIT],[
# Initialize xFLAGS with MPICH2_INTERNAL_xFLAGS.
  if test "$FROM_MPICH2" = "yes" ; then
    CFLAGS="$MPICH2_INTERNAL_CFLAGS"
    CXXFLAGS="$MPICH2_INTERNAL_CXXFLAGS"
    FFLAGS="$MPICH2_INTERNAL_FFLAGS"
    F90FLAGS="$MPICH2_INTERNAL_F90FLAGS"
  fi
])


AC_DEFUN([PAC_SUBCONFIG_FINALIZE],[
])

AC_DEFUN([PAC_MPICH2_FINALIZE],[
])
