[#] start of __file__

AC_DEFUN([PAC_SUBCFG_PREREQ_]PAC_SUBCFG_AUTO_SUFFIX,[
])

AC_DEFUN([PAC_SUBCFG_BODY_]PAC_SUBCFG_AUTO_SUFFIX,[

# common ARG_ENABLE, shared by "simple" and "pmi2"
AC_ARG_ENABLE(pmiport,
[--enable-pmiport - Allow PMI interface to use a host-port pair to contact
                   for PMI services],,enable_pmiport=default)

])dnl end BODY macro

m4_include([src/pmi/bgq/subconfigure.m4])
m4_include([src/pmi/pmi2/subconfigure.m4])
m4_include([src/pmi/slurm/subconfigure.m4])
m4_include([src/pmi/cray/subconfigure.m4])
m4_include([src/pmi/simple/subconfigure.m4])

m4_define([PAC_SRC_PMI_SUBCFG_MODULE_LIST],
[PAC_SRC_PMI_BGQ_SUBCFG_MODULE_LIST,
 PAC_SRC_PMI_PMI2_SUBCFG_MODULE_LIST,
 PAC_SRC_PMI_SLURM_SUBCFG_MODULE_LIST,
 PAC_SRC_PMI_CRAY_SUBCFG_MODULE_LIST,
 PAC_SRC_PMI_PMI2_SUBCFG_MODULE_LIST,
 PAC_SRC_PMI_SIMPLE_SUBCFG_MODULE_LIST])

[#] end of __file__
