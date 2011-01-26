dnl /*D
dnl PAC_F77_WORKS_WITH_CPP
dnl
dnl Checks if Fortran 77 compiler works with C preprocessor
dnl
dnl Most systems allow the Fortran compiler to process .F and .F90 files
dnl using the C preprocessor.  However, some systems either do not
dnl allow this or have serious bugs (OSF Fortran compilers have a bug
dnl that generates an error message from cpp).  The following test
dnl checks to see if .F works, and if not, whether "cpp -P -C" can be used
dnl D*/
AC_DEFUN([PAC_F77_WORKS_WITH_CPP],[
AC_REQUIRE([AC_PROG_CPP])
AC_MSG_CHECKING([whether Fortran 77 compiler processes .F files with C preprocessor])
AC_LANG_PUSH([Fortran 77])
saved_f77_ext=${ac_ext}
ac_ext="F"
saved_FFLAGS="$FFLAGS"
FFLAGS="$FFLAGS $CPPFLAGS"
AC_LANG_CONFTEST([
    AC_LANG_SOURCE([
        program main
#define ASIZE 10
        integer a(ASIZE)
        end
    ])
])
AC_COMPILE_IFELSE([],[
    pac_cv_f77_accepts_F=yes
    ifelse([$1],[],[],[$1=""])
],[
    pac_cv_f77_accepts_F=no
    ifelse([$1],[],[:],[$1="false"])
])
# Restore Fortran 77's ac_ext but not FFLAGS
ac_ext="$saved_f77_ext"

if test "$pac_cv_f77_accepts_F" != "yes" ; then
    pac_cpp_f77="$ac_cpp -C -P conftest.F > conftest.$ac_ext"
    PAC_RUNLOG_IFELSE([$pac_cpp_f77],[
        if test -s conftest.${ac_ext} ; then
            AC_COMPILE_IFELSE([],[
                pac_cv_f77_accepts_F="no, use cpp"
                ifelse([$1],[],[],[$1="$CPP -C -P"])
            ],[])
            rm -f conftest.${ac_ext}
        fi
    ],[])
fi
FFLAGS="$saved_FFLAGS"
rm -f conftest.F
AC_LANG_POP([Fortran 77])
AC_MSG_RESULT([$pac_cv_f77_accepts_F])
])


dnl /*D 
dnl PAC_PROG_F77_HAS_POINTER - Determine if Fortran allows pointer type
dnl
dnl Synopsis:
dnl   PAC_PROG_F77_HAS_POINTER(action-if-true,action-if-false)
dnl D*/
AC_DEFUN([PAC_PROG_F77_HAS_POINTER],[
AC_CACHE_CHECK([whether Fortran 77 supports Cray-style pointer],
pac_cv_prog_f77_has_pointer,[
AC_LANG_PUSH([Fortran 77])
AC_LANG_CONFTEST([
    AC_LANG_PROGRAM([],[
        integer M
        pointer (MPTR,M)
        data MPTR/0/
    ])
])
saved_FFLAGS="$FFLAGS"
pac_cv_prog_f77_has_pointer=""
for ptrflag in '' '-fcray-pointer' ; do
    FFLAGS="$saved_FFLAGS $ptrflag"
    AC_COMPILE_IFELSE([], [pac_cv_prog_f77_has_pointer="yes";break], [])
done
if test "$pac_cv_prog_f77_has_pointer" = "yes" -a "X$ptrflag" != "X" ; then
    pac_cv_prog_f77_has_pointer="with $ptrflag"
fi
AC_LANG_POP([Fortran 77])
])
if test "X$pac_cv_prog_f77_has_pointer" != "X" ; then
    ifelse([$1],,:,[$1])
else
    ifelse([$2],,:,[$2])
fi
])
