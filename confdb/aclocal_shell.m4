dnl PAC_CHECK_BASH - check whether bash syntax, esp. arrays, can be used in mpicc etc.
dnl                  Sets $pac_cv_bash_works
AC_DEFUN([PAC_CHECK_BASH],[
    AC_ARG_ENABLE([bash],[AS_HELP_STRING([--disable-bash],[Do not use bash for wrapper scripts (mpicc etc)])],
                  [],[enable_bash=yes])
    AC_PATH_PROG(BASH_SHELL,bash)
    pac_cv_bash_works=no
    if test -z "$pac_cv_bash_works" -a "$enable_bash" = "yes" ; then
        # Confirm that bash has working arrays.  We can use this to
        # build more robust versions of the scripts (particularly the
        # compilation scripts) by taking advantage of the array features in
        # bash.
        if test -x "$BASH_SHELL" ; then
        changequote(%%,::)dnl
            cat >>conftest <<EOF
#! $BASH_SHELL
A[0]="b"
A[1]="c"
rc=1
if test \${A[1]} != "c" ; then rc=2 ; else rc=0 ; fi
exit \$rc
EOF
        changequote([,])dnl
            AC_MSG_CHECKING([whether $BASH_SHELL supports arrays])
            chmod +x conftest
            if ./conftest 2>&1 >/dev/null ; then
                pac_cv_bash_works=yes
            else
                pac_cv_bash_works=no
            fi
            rm -f conftest*
            AC_MSG_RESULT($pac_cv_bash_works)
        fi
    fi
])
