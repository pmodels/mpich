dnl PAC_CHECK_PYTHON check for python 3, sets PYTHON variable or abort
dnl
AC_DEFUN([PAC_CHECK_PYTHON],[
    AC_ARG_VAR([PYTHON], [set to Python 3])
    if test -z "$PYTHON" ; then
        PYTHON=
        python_one_liner="import sys; print(sys.version_info[[0]])"

        dnl check command 'python'
        PYTHON_PATH=
        AC_PATH_PROG(PYTHON_PATH, python)
        if test "x$PYTHON_PATH" != x ; then
            py_version=`$PYTHON_PATH -c "$python_one_liner"`
            if test "x$py_version" = x3 ; then
                PYTHON=$PYTHON_PATH
            fi
        fi
        dnl PYTHON is still not set, check command 'python3'
        if test "x$PYTHON" = x ; then
            PYTHON3_PATH=
            AC_PATH_PROG(PYTHON3_PATH, python3)
            if test "x$PYTHON3_PATH" != x ; then
                py3_version=`$PYTHON3_PATH -c "$python_one_liner"`
                if test "x$py3_version" = x3 ; then
                    PYTHON=$PYTHON3_PATH
                fi
            fi
        fi
        if test -z "$PYTHON" ; then
            AC_MSG_WARN([Python version 3 not found! Bindings need to be generated before configure.])
        else
            AC_MSG_NOTICE([Python version 3 is $PYTHON])
        fi
    fi
])
