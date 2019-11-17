
dnl PAC_SET_HEADER_LIB_PATH(with_option)
dnl This macro looks for the --with-xxx=, --with-xxx-include and --with-xxx-lib=
dnl options and sets the library and include paths.
dnl
dnl TODO as written, this macro cannot handle a "with_option" arg that has "-"
dnl characters in it.  Use AS_TR_SH (and possibly AS_VAR_* macros) to handle
dnl this case if it ever arises.
AC_DEFUN([PAC_SET_HEADER_LIB_PATH],[
    PAC_SET_HEADER_LIB_PATH_ARG([$1])
    AC_ARG_WITH([$1-include],
                [AC_HELP_STRING([--with-$1-include=PATH],
                                [specify path where $1 include directory can be found])],
                [AS_CASE(["$withval"],
                         [yes|no|''],
                         [AC_MSG_WARN([--with[out]-$1-include=PATH expects a valid PATH])
                          with_$1_include=""])],
                [])
    AC_ARG_WITH([$1-lib],
                [AC_HELP_STRING([--with-$1-lib=PATH],
                                [specify path where $1 lib directory can be found])],
                [AS_CASE(["$withval"],
                         [yes|no|''],
                         [AC_MSG_WARN([--with[out]-$1-lib=PATH expects a valid PATH])
                          with_$1_lib=""])],
                [])

    # The args have been sanitized into empty/non-empty values above.
    # Now append -I/-L args to CPPFLAGS/LDFLAGS, with more specific options
    # taking priority

    AS_IF([test -n "${with_$1_include}"],
          [PAC_APPEND_FLAG([-I${with_$1_include}],[CPPFLAGS])],
          [AS_IF([test -n "${with_$1}"],
                 [PAC_APPEND_FLAG([-I${with_$1}/include],[CPPFLAGS])])])

    AS_IF([test -n "${with_$1_lib}"],
          [PAC_APPEND_FLAG([-L${with_$1_lib}],[LDFLAGS])],
          [AS_IF([test -n "${with_$1}"],
                 dnl is adding lib64 by default really the right thing to do?  What if
                 dnl we are on a 32-bit host that happens to have both lib dirs available?
                 [PAC_APPEND_FLAG([-L${with_$1}/lib],[LDFLAGS])
                  AS_IF([test -d "${with_$1}/lib64"],
		        [PAC_APPEND_FLAG([-L${with_$1}/lib64],[LDFLAGS])])
                 ])
          ])

    if test -z "$with_$1" ; then
        if test -n "$with_$1_include" || test -n "$with_$1_lib" ; then
            # User specified a path, potentially raise error if check failed
            with_$1=yes
        fi
    fi
])

dnl PAC_SET_HEADER_LIB_PATH_SIMPLE(with_option)
dnl similar to PAC_SET_HEADER_LIB_PATH, but without --with-xxx-include and --with--xxx-lib
AC_DEFUN([PAC_SET_HEADER_LIB_PATH_SIMPLE],[
    PAC_SET_HEADER_LIB_PATH_ARG([$1])
    if test -n "$with_$1" ; then
        PAC_APPEND_FLAG([-I${with_$1}/include],[CPPFLAGS])
        PAC_APPEND_FLAG([-L${with_$1}/lib],[LDFLAGS])
        if test -d "${with_$1}/lib64" ; then
            PAC_APPEND_FLAG([-L${with_$1}/lib64],[LDFLAGS])
        fi
    fi
])

dnl PAC_SET_HEADER_LIB_PATH_ARG(with_option)
dnl this one only does AC_ARG_WITH. Define this macro here to ensure consistent behavior.
AC_DEFUN([PAC_SET_HEADER_LIB_PATH_ARG],[
    AC_ARG_WITH([$1],
                [AC_HELP_STRING([--with-$1=[[PATH]]],
                                [specify path where $1 include directory and lib directory can be found])],,
                [with_$1=""])
])

dnl PAC_CHECK_HEADER_LIB(header.h, libname, function, action-if-yes, action-if-no)
dnl This macro checks for a header and lib.  It is assumed that the
dnl user can specify a path to the includes and libs using --with-xxx=.
dnl The xxx is specified in the "with_option" parameter.
dnl
dnl NOTE: This macro expects a corresponding PAC_SET_HEADER_LIB_PATH
dnl macro (or equivalent logic) to be used before this macro is used.
AC_DEFUN([PAC_CHECK_HEADER_LIB],[
    failure=no
    AC_CHECK_HEADER([$1],,failure=yes)
    dnl prepends -l[name] to LIBS and define HAVE_LIBNAME
    AC_CHECK_LIB($2,$3,,failure=yes)
    if test "$failure" = "no" ; then
       $4
    else
       $5
    fi
])

dnl PAC_CHECK_HEADER_LIB_ONLY(with_option, header.h, libname, function)
dnl Similar to PAC_CHECK_HEADER_LIB, but does not later LIBS
AC_DEFUN([PAC_CHECK_HEADER_LIB_ONLY],[
    PAC_SET_HEADER_LIB_PATH($1)
    failure=no
    AC_CHECK_HEADER([$2],,failure=yes)
    AC_CHECK_LIB($3,$4,:,failure=yes)
    if test "$failure" = "no" ; then
       have_$1=yes
    else
       have_$1=no
    fi
])

dnl PAC_CHECK_HEADER_LIB_FATAL(with_option, header.h, libname, function)
dnl Similar to PAC_CHECK_HEADER_LIB, but errors out on failure
AC_DEFUN([PAC_CHECK_HEADER_LIB_FATAL],[
	PAC_SET_HEADER_LIB_PATH($1)
	PAC_CHECK_HEADER_LIB($2,$3,$4,have_$1=yes,have_$1=no)
	if test "$have_$1" = "no" ; then
	   AC_MSG_ERROR(['$2 or lib$3 library not found. Did you specify --with-$1= or --with-$1-include= or --with-$1-lib=?'])
	fi
])

dnl PAC_CHECK_HEADER_LIB_ERROR(with_option, header.h, libname, function)
dnl Similar to PAC_CHECK_HEADER_LIB, but errors out on failure and when --with options is given
AC_DEFUN([PAC_CHECK_HEADER_LIB_ERROR],[
	PAC_SET_HEADER_LIB_PATH($1)
	PAC_CHECK_HEADER_LIB($2,$3,$4,have_$1=yes,have_$1=no)
        if test -n "$with_$1" && test have_$1 = no ; then
	   AC_MSG_ERROR([with-$1 specified but library was not found])
	fi
])

dnl PAC_CHECK_PREFIX(with_option,prefixvar)
AC_DEFUN([PAC_CHECK_PREFIX],[
	AC_ARG_WITH([$1-prefix],
            [AS_HELP_STRING([[--with-$1-prefix[=DIR]]], [use the $1
                            library installed in DIR, rather than the
                            one included in the distribution.  Pass
                            "embedded" to force usage of the included
                            $1 source.])],
            [if test "$withval" = "system" ; then
                 :
             elif test "$withval" = "embedded" ; then
                 :
             elif test "$withval" = "no" ; then
                 :
             else
                 PAC_APPEND_FLAG([-I${with_$1_prefix}/include],[CPPFLAGS])
                 if test -d "${with_$1_prefix}/lib64" ; then
                     PAC_APPEND_FLAG([-L${with_$1_prefix}/lib64],[LDFLAGS])
                 fi
                 PAC_APPEND_FLAG([-L${with_$1_prefix}/lib],[LDFLAGS])
             fi
             ],
            [with_$1_prefix="embedded"])
	]
)

dnl PAC_LIB_DEPS(library_name, library_pc_path)
dnl library_pc_path is the path to the library pkg-config directory
AC_DEFUN([PAC_LIB_DEPS],[
if test "x$2" != "x"; then
    ac_lib$1_deps=`pkg-config --static --libs $2/lib$1.pc 2>/dev/null`
    # remove the library itself in case it is embedded
    ac_lib$1_deps=`echo $ac_lib$1_deps | sed 's/-l$1//'`
else
    # use system default
    ac_lib$1_deps=`pkg-config --static --libs lib$1 2>/dev/null`
fi
])

