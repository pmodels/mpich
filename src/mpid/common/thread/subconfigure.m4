[#] start of __file__

dnl _PREREQ handles the former role of mpich2prereq, setup_device, etc
AC_DEFUN([PAC_SUBCFG_PREREQ_src_mpid_common_thread],[
AM_CONDITIONAL([BUILD_MPID_COMMON_THREAD],[test "X$build_mpid_common_thread" = "Xyes"])
])

AC_DEFUN([PAC_SUBCFG_BODY_src_mpid_common_thread],[
# this conditional must come here in the body because other body code and
# post-PREREQ main code will set its inputs 
AM_CONDITIONAL([THREAD_SERIALIZED_OR_MULTIPLE],
               [test "x$MPICH_THREAD_LEVEL" = "xMPI_THREAD_SERIALIZED" -o \
                     "x$MPICH_THREAD_LEVEL" = "xMPI_THREAD_MULTIPLE"])

AM_COND_IF([BUILD_MPID_COMMON_THREAD],[
    dnl nothing to do for now...
    :
])dnl end AM_COND_IF(BUILD_MPID_COMMON_THREAD,...)
])dnl end _BODY
[#] end of __file__
