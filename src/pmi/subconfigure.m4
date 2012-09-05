[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
])

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

# common ARG_ENABLE, shared by "simple" and "pmi2"
AC_ARG_ENABLE(pmiport,
[--enable-pmiport - Allow PMI interface to use a host-port pair to contact
                   for PMI services],,enable_pmiport=default)

])dnl end BODY macro
[#] end of __file__
