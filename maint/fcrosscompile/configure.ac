AC_INIT

AC_ARG_WITH(aint-size,
    AC_HELP_STRING([--with-aint-size],
                   [Override the size of MPI_AINT (in bytes)]),,
    with_aint_size=0)

AC_PROG_CPP
AC_PROG_CC
AC_CHECK_SIZEOF([char])
AC_CHECK_SIZEOF([short])
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([long long])
AC_CHECK_SIZEOF([void *])

PAC_PROG_F77
AC_F77_LIBRARY_LDFLAGS
PAC_PROG_F77_FLIBS_VALID
PAC_PROG_F77_CHECK_FLIBS
PAC_PROG_F77_OBJ_LINKS_WITH_C
PAC_PROG_F77_LINKER_WITH_C
PAC_PROG_F77_NAME_MANGLE
PAC_PROG_F77_AND_C_STDIO_LIBS

CROSS_F77_SIZEOF_INTEGER=0
CROSS_F77_SIZEOF_REAL=0
CROSS_F77_SIZEOF_DOUBLE_PRECISION=0
PAC_PROG_F77_CHECK_SIZEOF_EXT([integer],[$CROSS_F77_SIZEOF_INTEGER])
PAC_PROG_F77_CHECK_SIZEOF_EXT([real],[$CROSS_F77_SIZEOF_REAL])
PAC_PROG_F77_CHECK_SIZEOF_EXT([double precision],[$CROSS_F77_SIZEOF_DOUBLE_PRECISION])
AC_SUBST(pac_cv_f77_sizeof_integer)
AC_SUBST(pac_cv_f77_sizeof_real)
AC_SUBST(pac_cv_f77_sizeof_double_precision)

# MPI_FINT is required for logical-to-C translation.
    noval="yes"
    AC_MSG_CHECKING([for C type matching Fortran integer])
    for c_type in char short int long "long_long" ; do
        eval ctypelen=\$"ac_cv_sizeof_$c_type"
        if test "$pac_cv_f77_sizeof_integer" = "$ctypelen" -a \
          "$ctypelen" -gt 0 ; then
            c_type=`echo $c_type | sed -e 's/_/ /g'`
            AC_MSG_RESULT($c_type)
            MPI_FINT=$c_type
            noval="no"
            break
        fi
    done
    if test "$noval" = "yes" ; then
        # Set a default
        MPI_FINT="int"
        AC_MSG_RESULT([unavailable])
    fi
PAC_F77_LOGICALS_IN_C([$MPI_FINT])
AC_SUBST([true_val])
AC_SUBST([false_val])

AC_PROG_FC
PAC_PROG_FC_WORKS

if test "$with_aint_size" -gt 0 -a \
        "$with_aint_size" -gt "$ac_cv_sizeof_void_p" ; then
    testsize=$with_aint_size
else
    testsize=$ac_cv_sizeof_void_p
fi
if test "$testsize" = 0 ; then
    # Set a default
    testsize=4
fi
dnl Using the {} around testsize helps the comments work correctly
CROSS_F90_ADDRESS_KIND=0
CROSS_F90_OFFSET_KIND=0
CROSS_F90_INTEGER_KIND=0
PAC_PROG_FC_INT_KIND([ADDRESS_KIND], [${testsize}], [$CROSS_F90_ADDRESS_KIND])
if test "$testsize" = 8 ; then
    OFFSET_KIND=$ADDRESS_KIND
else
    PAC_PROG_FC_INT_KIND([OFFSET_KIND],[8],[$CROSS_F90_OFFSET_KIND])
fi
PAC_PROG_FC_INT_KIND([INTEGER_KIND],[$pac_cv_f77_sizeof_integer],
                     [$CROSS_F90_INTEGER_KIND])
AC_SUBST(ADDRESS_KIND)
AC_SUBST(OFFSET_KIND)
AC_SUBST(INTEGER_KIND)


CROSS_F90_REAL_MODEL=NIL
CROSS_F90_DOUBLE_MODEL=NIL
CROSS_F90_INTEGER_MODEL=NIL
CROSS_F90_ALL_INTEGER_MODELS=NIL
CROSS_F90_INTEGER_MODEL_MAP=NIL
PAC_FC_SIMPLE_NUMBER_MODEL([the precision and range of reals],
                           [real aa],
                           [precision(aa), ",", range(aa)],
                           [FC_REAL_MODEL],
                           [$CROSS_F90_REAL_MODEL])
PAC_FC_SIMPLE_NUMBER_MODEL([the precision and range of double precision],
                           [double precision aa],
                           [precision(aa), ",", range(aa)],
                           [FC_DOUBLE_MODEL],
                           [$CROSS_F90_DOUBLE_MODEL])
PAC_FC_SIMPLE_NUMBER_MODEL([the range of integer],
                           [integer aa],
                           [range(aa)],
                           [FC_INTEGER_MODEL],
                           [$CROSS_F90_INTEGER_MODEL])
PAC_FC_AVAIL_INTEGER_MODELS([FC_ALL_INTEGER_MODELS],
                            [$CROSS_F90_ALL_INTEGER_MODELS])
PAC_FC_INTEGER_MODEL_MAP([FC_INTEGER_MODEL_MAP],
                         [$CROSS_F90_INTEGER_MODEL_MAP])
AC_SUBST(FC_REAL_MODEL)
AC_SUBST(FC_DOUBLE_MODEL)
AC_SUBST(FC_INTEGER_MODEL)
AC_SUBST(FC_ALL_INTEGER_MODELS)
AC_SUBST(FC_INTEGER_MODEL_MAP)

AC_OUTPUT(cross_values.txt)
