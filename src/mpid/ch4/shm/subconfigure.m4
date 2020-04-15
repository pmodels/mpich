[#] start of __file__

m4_include([src/mpid/ch4/shm/posix/subconfigure.m4])
m4_include([src/mpid/ch4/shm/xpmem/subconfigure.m4])
m4_include([src/mpid/ch4/shm/stubshm/subconfigure.m4])

m4_define([PAC_SRC_MPID_CH4_SHM_SUBCFG_MODULE_LIST],
[PAC_SRC_MPID_CH4_SHM_POSIX_SUBCFG_MODULE_LIST,
 PAC_SRC_MPID_CH4_SHM_XPMEM_SUBCFG_MODULE_LIST,
 PAC_SRC_MPID_CH4_SHM_STUBSHM_SUBCFG_MODULE_LIST])

[#] end of __file__
