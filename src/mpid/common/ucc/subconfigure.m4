[#] start of __file__

UCC_ENABLED_DEVICES="ch4:ucx"

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    if test "${with_ucc}" = "no" ; then
        pac_have_ucc=no;
    else
        PAC_CHECK_HEADER_LIB_EXPLICIT(ucc,[ucc/api/ucc.h],[ucc],[ucc_init_version])
        if test "$pac_have_ucc" = "yes" ; then
            AC_DEFINE([HAVE_UCC],1,[Define if building ucc])
        fi
    fi
    AM_CONDITIONAL([BUILD_UCC],[test "$pac_have_ucc" = "yes"])
])dnl end PREREQ

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

AM_COND_IF([BUILD_UCC], [
# Check if selected device supports the UCC wrappers
AS_IF([test -n "${DEVICE}"],
[
    AC_MSG_CHECKING([if the device ${DEVICE} supports UCC])
    found_ucc_enabled_device="no"
    for dev in ${UCC_ENABLED_DEVICES}; do
        if test "${dev}" = "${DEVICE}" ; then
            found_ucc_enabled_device="yes"
            AC_MSG_RESULT(yes)
            break
        fi
    done
    if test "${found_ucc_enabled_device}" = "no"; then
        AC_MSG_RESULT(no)
        AC_MSG_ERROR([Device ${DEVICE} does not support UCC. UCC-enabled devices are: ${UCC_ENABLED_DEVICES}])
    fi
])

# Check if CUDA awareness is enabled, and if so, whether UCC is also CUDA-aware
AS_IF([test -n "${GPU_SUPPORT}" -a "x${GPU_SUPPORT}" = "xCUDA" && test -n "${with_ucc}" -a "x${with_ucc}" != "xno"],
[
    AC_PATH_PROG([ucc_info_path], [ucc_info], [no], [${with_ucc}/bin])
    AS_IF([test "x${ucc_info_path}" != "xno"],
    [
        AC_MSG_CHECKING([if UCC is CUDA-aware (HAVE_CUDA)])
        ucc_info_defs=$(${ucc_info_path} -b)
        AC_LANG_PUSH([C])
        AC_COMPILE_IFELSE(  [AC_LANG_PROGRAM(
                                    [[${ucc_info_defs}]],
                                    [[
                                        #if !defined(HAVE_CUDA) || HAVE_CUDA != 1
                                        #error macro not defined
                                        #endif
                                    ]]
                                    )
                            ],[
                                AC_MSG_RESULT(yes)
                            ],[
                                AC_MSG_RESULT(no)
                                AC_MSG_ERROR([CUDA support is requested but no CUDA-aware UCC could be found.])
                            ]
                         )
        AC_LANG_POP([C])
    ],[
        AC_MSG_WARN([ucc_info not found. Cannot check for CUDA awareness of UCC.])
    ])
])
])dnl end AM_COND_IF(BUILD_UCC)

])dnl end _BODY

[#] end of __file__
