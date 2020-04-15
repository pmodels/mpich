[#] start of __file__

m4_include([src/mpid/subconfigure.m4])
m4_include([src/binding/subconfigure.m4])
m4_include([src/pmi/subconfigure.m4])
m4_include([src/util/subconfigure.m4])
m4_include([src/pm/subconfigure.m4])

m4_define([PAC_SRC_SUBCFG_MODULE_LIST],
[PAC_SRC_MPID_SUBCFG_MODULE_LIST,
 PAC_SRC_BINDING_SUBCFG_MODULE_LIST,
 PAC_SRC_PMI_SUBCFG_MODULE_LIST,
 PAC_SRC_UTIL_SUBCFG_MODULE_LIST,
 PAC_SRC_PM_SUBCFG_MODULE_LIST])

[#] end of __file__
