[#] start of __file__
dnl MPICH_SUBCFG_AFTER=src/mpid/ch3/channels/nemesis

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
    #check for NewMadeleine options 
    #AC_ARG_WITH(newmad, [--with-newmad=path - specify path where pm2 software can be found],
    #if test "${with_newmad}" != "yes" -a "${with_newmad}" != "no" ; then
    #    LDFLAGS="$LDFLAGS `${with_newmad}/bin/pm2-config  --flavor=$PM2_FLAVOR --libs`"
    #    CPPFLAGS="$CPPFLAGS `${with_newmad}/bin/pm2-config  --flavor=$PM2_FLAVOR --cflags`"
    #fi,)

    AM_COND_IF([BUILD_CH3_NEMESIS],[
        for net in $nemesis_networks ; do
            AS_CASE([$net],[newmad],[build_nemesis_netmod_newmad=yes])
        done
    ])
    AM_CONDITIONAL([BUILD_NEMESIS_NETMOD_NEWMAD],[test "X$build_nemesis_netmod_newmad" = "Xyes"])

])dnl

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[
AM_COND_IF([BUILD_NEMESIS_NETMOD_NEWMAD],[
    AC_MSG_NOTICE([RUNNING CONFIGURE FOR ch3:nemesis:newmad])
    LDFLAGS="$LDFLAGS `pkg-config nmad --libs`"
    CPPFLAGS="$CPPFLAGS `pkg-config nmad  --cflags`"
    WRAPPER_CFLAGS="$WRAPPER_CFLAGS `pkg-config nmad  --cflags`"
    AC_CHECK_HEADER([nm_public.h], , [
       AC_MSG_ERROR(['nm_public.h not found.  Did you specify --with-newmad= ?'])
    ])                                      
    AC_CHECK_HEADER([nm_sendrecv_interface.h], , [
       AC_MSG_ERROR(['nm_sendrecv_interface.h not found.  Did you specify --with-newmad= ?'])
    ])
    AC_CHECK_LIB(nmad,nm_core_init, , [
       AC_MSG_ERROR(['nmad library not found.  Did you specify --with-newmad= ?'])
    ])
    AC_ARG_ENABLE(newmad-multirail,
    [--enable-newmad-multirail -  enables multirail support in newmad module],,enable_multi=no)
    if test "$enable_multi" = "yes" ; then
        AC_DEFINE(MPID_MAD_MODULE_MULTIRAIL, 1, [Define to enable multirail support in newmad module])
    fi                 
    AC_DEFINE([ENABLE_COMM_OVERRIDES], 1, [define to add per-vc function pointers to override send and recv functions]) 
])dnl end AM_COND_IF(BUILD_NEMESIS_NETMOD_NEWMAD,...)
])dnl end _BODY
[#] end of __file__
