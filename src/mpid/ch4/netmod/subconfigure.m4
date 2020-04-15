[#] start of __file__

m4_include([src/mpid/ch4/netmod/ofi/subconfigure.m4])
m4_include([src/mpid/ch4/netmod/ucx/subconfigure.m4])
m4_include([src/mpid/ch4/netmod/stubnm/subconfigure.m4])

m4_define([PAC_SRC_MPID_CH4_NETMOD_SUBCFG_MODULE_LIST],
[PAC_SRC_MPID_CH4_NETMOD_OFI_SUBCFG_MODULE_LIST,
 PAC_SRC_MPID_CH4_NETMOD_UCX_SUBCFG_MODULE_LIST,
 PAC_SRC_MPID_CH4_NETMOD_STUBNM_SUBCFG_MODULE_LIST])

[#] end of __file__
