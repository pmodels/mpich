## -*- Mode: Makefile; -*-
## vim: set ft=automake :
##
## (C) 2011 by Argonne National Laboratory.
##     See COPYRIGHT in top-level directory.
##

include $(top_srcdir)/threads/Makefile_threads.mtest

EXTRA_DIST = testlist

noinst_PROGRAMS = threads threaded_sr alltoall sendselfth greq_wait greq_test \
                  multisend multisend2 multisend3 multisend4 ibsend \
                  mt_sendrecv mt_bsendrecv mt_ssendrecv mt_rsendrecv \
                  mt_isendirecv mt_ibsendirecv mt_issendirecv mt_irsendirecv \
                  mt_sendrecv_huge mt_bsendrecv_huge mt_ssendrecv_huge mt_rsendrecv_huge \
                  mt_isendirecv_huge mt_ibsendirecv_huge mt_issendirecv_huge mt_irsendirecv_huge \
                  mt_sendrecv_medium mt_bsendrecv_medium mt_ssendrecv_medium mt_rsendrecv_medium \
                  mt_isendirecv_medium mt_ibsendirecv_medium mt_issendirecv_medium mt_irsendirecv_medium \
                  mt_probe_isendrecv_small mt_probe_sendrecv_small \
                  mt_iprobe_isendrecv_small mt_iprobe_sendrecv_small \
                  mt_mprobe_isendrecv_small mt_mprobe_sendrecv_small mt_mprobe_sendirecv_small \
                  mt_improbe_isendrecv_small mt_improbe_sendrecv_small \
                  mt_probe_sendrecv_huge mt_iprobe_sendrecv_huge \
                  mt_mprobe_sendrecv_huge mt_improbe_sendrecv_huge \
                  mt_probe_sendrecv_medium mt_mprobe_sendrecv_medium \
                  mt_sendrecv_pers mt_bsendrecv_pers mt_ssendrecv_pers mt_rsendrecv_pers \
                  mt_sendrecv_pers_huge mt_bsendrecv_pers_huge mt_ssendrecv_pers_huge mt_rsendrecv_pers_huge \
                  mt_sendrecv_pers_medium mt_bsendrecv_pers_medium mt_ssendrecv_pers_medium mt_rsendrecv_pers_medium

# MT Send/Recv tests with small messages

mt_sendrecv_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_sendrecv_CPPFLAGS = -DSEND $(AM_CPPFLAGS)

mt_bsendrecv_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_bsendrecv_CPPFLAGS = -DBSEND $(AM_CPPFLAGS)

mt_ssendrecv_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_ssendrecv_CPPFLAGS = -DSSEND $(AM_CPPFLAGS)

mt_rsendrecv_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_rsendrecv_CPPFLAGS = -DRSEND $(AM_CPPFLAGS)

mt_isendirecv_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_isendirecv_CPPFLAGS = -DNONBLOCKING -DISEND $(AM_CPPFLAGS)

mt_ibsendirecv_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_ibsendirecv_CPPFLAGS = -DNONBLOCKING -DIBSEND $(AM_CPPFLAGS)

mt_issendirecv_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_issendirecv_CPPFLAGS = -DNONBLOCKING -DISSEND $(AM_CPPFLAGS)

mt_irsendirecv_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_irsendirecv_CPPFLAGS = -DNONBLOCKING -DIRSEND $(AM_CPPFLAGS)

# MT Send/Recv tests with large messages

mt_sendrecv_huge_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_sendrecv_huge_CPPFLAGS = -DHUGE_COUNT -DSEND $(AM_CPPFLAGS)

mt_bsendrecv_huge_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_bsendrecv_huge_CPPFLAGS = -DHUGE_COUNT -DBSEND $(AM_CPPFLAGS)

mt_ssendrecv_huge_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_ssendrecv_huge_CPPFLAGS = -DHUGE_COUNT -DSSEND $(AM_CPPFLAGS)

mt_rsendrecv_huge_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_rsendrecv_huge_CPPFLAGS = -DHUGE_COUNT -DRSEND $(AM_CPPFLAGS)

mt_isendirecv_huge_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_isendirecv_huge_CPPFLAGS = -DHUGE_COUNT -DNONBLOCKING -DISEND $(AM_CPPFLAGS)

mt_ibsendirecv_huge_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_ibsendirecv_huge_CPPFLAGS = -DHUGE_COUNT -DNONBLOCKING -DIBSEND $(AM_CPPFLAGS)

mt_issendirecv_huge_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_issendirecv_huge_CPPFLAGS = -DHUGE_COUNT -DNONBLOCKING -DISSEND $(AM_CPPFLAGS)

mt_irsendirecv_huge_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_irsendirecv_huge_CPPFLAGS = -DHUGE_COUNT -DNONBLOCKING -DIRSEND $(AM_CPPFLAGS)

# MT Send/Recv tests with medium messages

mt_sendrecv_medium_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_sendrecv_medium_CPPFLAGS = -DMEDIUM_COUNT -DSEND $(AM_CPPFLAGS)

mt_bsendrecv_medium_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_bsendrecv_medium_CPPFLAGS = -DMEDIUM_COUNT -DBSEND $(AM_CPPFLAGS)

mt_ssendrecv_medium_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_ssendrecv_medium_CPPFLAGS = -DMEDIUM_COUNT -DSSEND $(AM_CPPFLAGS)

mt_rsendrecv_medium_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_rsendrecv_medium_CPPFLAGS = -DMEDIUM_COUNT -DRSEND $(AM_CPPFLAGS)

mt_isendirecv_medium_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_isendirecv_medium_CPPFLAGS = -DMEDIUM_COUNT -DNONBLOCKING -DISEND $(AM_CPPFLAGS)

mt_ibsendirecv_medium_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_ibsendirecv_medium_CPPFLAGS = -DMEDIUM_COUNT -DNONBLOCKING -DIBSEND $(AM_CPPFLAGS)

mt_issendirecv_medium_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_issendirecv_medium_CPPFLAGS = -DMEDIUM_COUNT -DNONBLOCKING -DISSEND $(AM_CPPFLAGS)

mt_irsendirecv_medium_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_irsendirecv_medium_CPPFLAGS = -DMEDIUM_COUNT -DNONBLOCKING -DIRSEND $(AM_CPPFLAGS)


# MT Probe family tests with small data send/recv

mt_probe_isendrecv_small_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_probe_isendrecv_small_CPPFLAGS = -DPROBE -DNBLOCKINGSEND -DISEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_probe_sendrecv_small_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_probe_sendrecv_small_CPPFLAGS = -DPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_iprobe_isendrecv_small_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_iprobe_isendrecv_small_CPPFLAGS = -DIPROBE -DNBLOCKINGSEND -DISEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_iprobe_sendrecv_small_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_iprobe_sendrecv_small_CPPFLAGS = -DIPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_mprobe_isendrecv_small_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_mprobe_isendrecv_small_CPPFLAGS = -DMPROBE -DNBLOCKINGSEND -DISEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_mprobe_sendrecv_small_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_mprobe_sendrecv_small_CPPFLAGS = -DMPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_mprobe_sendirecv_small_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_mprobe_sendirecv_small_CPPFLAGS = -DMPROBE -DBLOCKINGSEND -DSEND -DNBLOCKINGRECV $(AM_CPPFLAGS)

mt_improbe_isendrecv_small_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_improbe_isendrecv_small_CPPFLAGS = -DIMPROBE -DNBLOCKINGSEND -DISEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_improbe_sendrecv_small_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_improbe_sendrecv_small_CPPFLAGS = -DIMPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

# MT Probe family tests with large data send/recv

mt_probe_sendrecv_huge_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_probe_sendrecv_huge_CPPFLAGS = -DHUGE_COUNT -DPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_iprobe_sendrecv_huge_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_iprobe_sendrecv_huge_CPPFLAGS = -DHUGE_COUNT -DIPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_mprobe_sendrecv_huge_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_mprobe_sendrecv_huge_CPPFLAGS = -DHUGE_COUNT -DMPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_improbe_sendrecv_huge_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_improbe_sendrecv_huge_CPPFLAGS = -DHUGE_COUNT -DIMPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

# MT Probe family tests with medium data send/recv

mt_probe_sendrecv_medium_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_probe_sendrecv_medium_CPPFLAGS = -DMEDIUM_COUNT -DPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_mprobe_sendrecv_medium_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_mprobe_sendrecv_medium_CPPFLAGS = -DMEDIUM_COUNT -DMPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

# MT Persistent Send/Recv tests with small messages

mt_sendrecv_pers_SOURCES = mt_pt2pt_common.c mt_pers_sendrecv_impl.c
mt_sendrecv_pers_CPPFLAGS = -DSEND $(AM_CPPFLAGS)

mt_bsendrecv_pers_SOURCES = mt_pt2pt_common.c mt_pers_sendrecv_impl.c
mt_bsendrecv_pers_CPPFLAGS = -DBSEND $(AM_CPPFLAGS)

mt_ssendrecv_pers_SOURCES = mt_pt2pt_common.c mt_pers_sendrecv_impl.c
mt_ssendrecv_pers_CPPFLAGS = -DSSEND $(AM_CPPFLAGS)

mt_rsendrecv_pers_SOURCES = mt_pt2pt_common.c mt_pers_sendrecv_impl.c
mt_rsendrecv_pers_CPPFLAGS = -DRSEND $(AM_CPPFLAGS)

# MT Persistent Send/Recv tests with large messages

mt_sendrecv_pers_huge_SOURCES = mt_pt2pt_common.c mt_pers_sendrecv_impl.c
mt_sendrecv_pers_huge_CPPFLAGS = -DHUGE_COUNT -DSEND $(AM_CPPFLAGS)

mt_bsendrecv_pers_huge_SOURCES = mt_pt2pt_common.c mt_pers_sendrecv_impl.c
mt_bsendrecv_pers_huge_CPPFLAGS = -DHUGE_COUNT -DBSEND $(AM_CPPFLAGS)

mt_ssendrecv_pers_huge_SOURCES = mt_pt2pt_common.c mt_pers_sendrecv_impl.c
mt_ssendrecv_pers_huge_CPPFLAGS = -DHUGE_COUNT -DSSEND $(AM_CPPFLAGS)

mt_rsendrecv_pers_huge_SOURCES = mt_pt2pt_common.c mt_pers_sendrecv_impl.c
mt_rsendrecv_pers_huge_CPPFLAGS = -DHUGE_COUNT -DRSEND $(AM_CPPFLAGS)

# MT Persistent Send/Recv tests with medium messages

mt_sendrecv_pers_medium_SOURCES = mt_pt2pt_common.c mt_pers_sendrecv_impl.c
mt_sendrecv_pers_medium_CPPFLAGS = -DMEDIUM_COUNT -DSEND $(AM_CPPFLAGS)

mt_bsendrecv_pers_medium_SOURCES = mt_pt2pt_common.c mt_pers_sendrecv_impl.c
mt_bsendrecv_pers_medium_CPPFLAGS = -DMEDIUM_COUNT -DBSEND $(AM_CPPFLAGS)

mt_ssendrecv_pers_medium_SOURCES = mt_pt2pt_common.c mt_pers_sendrecv_impl.c
mt_ssendrecv_pers_medium_CPPFLAGS = -DMEDIUM_COUNT -DSSEND $(AM_CPPFLAGS)

mt_rsendrecv_pers_medium_SOURCES = mt_pt2pt_common.c mt_pers_sendrecv_impl.c
mt_rsendrecv_pers_medium_CPPFLAGS = -DMEDIUM_COUNT -DRSEND $(AM_CPPFLAGS)
