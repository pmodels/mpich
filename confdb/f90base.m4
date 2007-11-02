m4_define([AC_LANG(Fortran 90)],
[ac_ext=${ac_f90ext-f}
ac_compile='$F90 -c $F90FLAGS conftest.$ac_ext >&AS_MESSAGE_LOG_FD'
ac_link='$F90 -o conftest$ac_exeext $F90FLAGS $LDFLAGS conftest.$ac_ext $LIBS >&AS_MESSAGE_LOG_FD'
ac_compiler_gnu=$ac_cv_f90_compiler_gnu
])
m4_define([_AC_LANG_ABBREV(Fortran 90)], [f90])
m4_define([_AC_LANG_PREFIX(Fortran 90)], [F90])
m4_define([AC_LANG_SOURCE(Fortran 90)],
[$1])
m4_define([AC_LANG_PROGRAM(Fortran 90)],
[m4_ifval([$1],
       [m4_warn([syntax], [$0: ignoring PROLOGUE: $1])])dnl
      program main
$2
      end])

AC_DEFUN([PAC_PROG_F90],[
AC_LANG_PUSH(Fortran 90)dnl
AC_CHECK_TOOLS(F90,
      [m4_default([$1],
                  [f90 xlf90 pgf90 ifort epcf90 f95 fort xlf95 lf95 pathf90 g95 gfortran ifc efc])])

# once we find the compiler, confirm the extension 
AC_MSG_CHECKING([that $ac_ext works as the extension for Fortran 90 program])
cat > conftest.$ac_ext <<EOF
      program conftest
      end
EOF
if AC_TRY_EVAL(ac_compile) ; then
    AC_MSG_RESULT(yes)
else
    AC_MSG_RESULT(no)
    AC_MSG_CHECKING([for extension for Fortran 90 programs])
    ac_ext="f90"
    cat > conftest.$ac_ext <<EOF
      program conftest
      end
EOF
    if AC_TRY_EVAL(ac_compile) ; then
        AC_MSG_RESULT([f90])
    else
        rm -f conftest*
        ac_ext="f"
        cat > conftest.$ac_ext <<EOF
      program conftest
      end
EOF
        if AC_TRY_EVAL(ac_compile) ; then
            AC_MSG_RESULT([f])
        else
            AC_MSG_RESULT([unknown!])
        fi
    fi
    ac_f90ext=$ac_ext
    if test "$ac_ext" = "f90" ; then
        pac_cv_f90_ext_f90=yes
    else 
        pac_cv_f90_ext_f90=no
    fi
    pac_cv_f90_ext=$ac_ext
    rm -f conftest*
fi
# Provide some information about the compiler.
echo "$as_me:__oline__:" \
     "checking for _AC_LANG compiler version" >&AS_MESSAGE_LOG_FD
ac_compiler=`set X $ac_compile; echo $[2]`
_AC_EVAL([$ac_compiler --version </dev/null >&AS_MESSAGE_LOG_FD])
_AC_EVAL([$ac_compiler -v </dev/null >&AS_MESSAGE_LOG_FD])
_AC_EVAL([$ac_compiler -V </dev/null >&AS_MESSAGE_LOG_FD])

m4_expand_once([_AC_COMPILER_EXEEXT])[]dnl
m4_expand_once([_AC_COMPILER_OBJEXT])[]dnl
# If we don't use `.F' as extension, the preprocessor is not run on the
# input file.
if test -n "$F90" ; then
    ac_save_ext=$ac_ext
    ac_ext=F
    _AC_LANG_COMPILER_GNU
    ac_ext=$ac_save_ext
    G90=`test $ac_compiler_gnu = yes && echo yes`
dnl    _AC_PROG_F90_G
fi
AC_LANG_POP(Fortran 90)dnl
])
AC_DEFUN([PAC_LANG_FORTRAN90],[AC_LANG_PUSH(Fortran 90)])
