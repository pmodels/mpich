[#] start of __file__

m4_include([src/mpid/common/hcoll/subconfigure.m4])
m4_include([src/mpid/common/sched/subconfigure.m4])
m4_include([src/mpid/common/thread/subconfigure.m4])
m4_include([src/mpid/common/bc/subconfigure.m4])
m4_include([src/mpid/common/shm/subconfigure.m4])

m4_define([PAC_SRC_MPID_COMMON_SUBCFG_MODULE_LIST],
[PAC_SRC_MPID_COMMON_HCOLL_SUBCFG_MODULE_LIST,
 PAC_SRC_MPID_COMMON_SCHED_SUBCFG_MODULE_LIST,
 PAC_SRC_MPID_COMMON_THREAD_SUBCFG_MODULE_LIST,
 PAC_SRC_MPID_COMMON_BC_SUBCFG_MODULE_LIST,
 PAC_SRC_MPID_COMMON_SHM_SUBCFG_MODULE_LIST])

[#] end of __file__
