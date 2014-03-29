[#] start of __file__
dnl This configure is used ONLY to determine the Fortran 90 features
dnl that are needed to implement the create_type_xxx routines.

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_CONDITIONAL([BUILD_FC_BINDING],[test "X$enable_fc" = "Xyes"])
])

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

AM_COND_IF([BUILD_FC_BINDING],[
# FIXME XXX DJG this code came from a sub-configure in src/binding/fortran/use_mpi.  Why
# isn't this just all up in the top-level configure?  Alternatively, why doesn't
# most/all of the f90 configure code from the top level configure.ac live here
# instead?  Is it because of the c/f77/f90 interplay?
AC_MSG_NOTICE([RUNNING CONFIGURE FOR F90 CODE])

AC_PROG_LN_S

# Determine the extension for Fortran 90 files (it isn't always .f90)
PAC_FC_EXT
FCEXT=$ac_fc_srcext
AC_SUBST(FCEXT)

# Determine the precision and range of the standard Fortran types.  This
# isn't quite enough for a full implementation of the Type_create_f90_xxx
# routines, but will handle most programs.  We can extend this by trying to
# find (through selected_real_kind and selected_int_kind) types with larger or
# smaller precisions and/or ranges than the basic types.
PAC_FC_SIMPLE_NUMBER_MODEL([the precision and range of reals],
                           [real aa],
                           [precision(aa), ",", range(aa)],
                           [FC_REAL_MODEL],
                           [$CROSS_F90_REAL_MODEL])
AC_SUBST(FC_REAL_MODEL)
#
PAC_FC_SIMPLE_NUMBER_MODEL([the precision and range of double precision],
                           [double precision aa],
                           [precision(aa), ",", range(aa)],
                           [FC_DOUBLE_MODEL],
                           [$CROSS_F90_DOUBLE_MODEL])
AC_SUBST(FC_DOUBLE_MODEL)
#
PAC_FC_SIMPLE_NUMBER_MODEL([the range of integer],
                           [integer aa],
                           [range(aa)],
                           [FC_INTEGER_MODEL],
                           [$CROSS_F90_INTEGER_MODEL])
AC_SUBST(FC_INTEGER_MODEL)

# Try to find the available integer kinds by using selected_int_kind
# This produces a table of range,kind
PAC_FC_AVAIL_INTEGER_MODELS([FC_ALL_INTEGER_MODELS],
                            [$CROSS_F90_ALL_INTEGER_MODELS])
AC_SUBST(FC_ALL_INTEGER_MODELS)
#
PAC_FC_INTEGER_MODEL_MAP([FC_INTEGER_MODEL_MAP],
                         [$CROSS_F90_INTEGER_MODEL_MAP])
AC_SUBST(FC_INTEGER_MODEL_MAP)    

AC_CONFIG_FILES([src/binding/fortran/use_mpi/mpif90model.h])

])dnl end AM_COND_IF(BUILD_FC_BINDING,...)
])dnl end _BODY

[#] end of __file__
