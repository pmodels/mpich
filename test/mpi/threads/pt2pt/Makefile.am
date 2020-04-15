##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

include $(top_srcdir)/Makefile_threads.mtest

EXTRA_DIST = testlist

noinst_PROGRAMS = threads threaded_sr alltoall sendselfth greq_wait greq_test \
                  multisend multisend2 multisend3 multisend4 ibsend \
                  mt_sendrecv mt_bsendrecv mt_ssendrecv \
                  mt_isendirecv mt_ibsendirecv mt_issendirecv \
                  mt_sendrecv_huge mt_bsendrecv_huge mt_ssendrecv_huge \
                  mt_isendirecv_huge mt_ibsendirecv_huge mt_issendirecv_huge \
                  mt_probe_isendrecv mt_probe_sendrecv \
                  mt_iprobe_isendrecv mt_iprobe_sendrecv \
                  mt_mprobe_isendrecv mt_mprobe_sendrecv mt_mprobe_sendirecv \
                  mt_improbe_isendrecv mt_improbe_sendrecv \
                  mt_probe_sendrecv_huge mt_iprobe_sendrecv_huge \
                  mt_mprobe_sendrecv_huge mt_improbe_sendrecv_huge \
                  mt_sendrecv_pers_huge mt_bsendrecv_pers_huge mt_ssendrecv_pers_huge \
                  mt_mpicancel

# MT Send/Recv tests with small messages

mt_sendrecv_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_sendrecv_CPPFLAGS = -DSEND $(AM_CPPFLAGS)

mt_bsendrecv_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_bsendrecv_CPPFLAGS = -DBSEND $(AM_CPPFLAGS)

mt_ssendrecv_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_ssendrecv_CPPFLAGS = -DSSEND $(AM_CPPFLAGS)

mt_isendirecv_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_isendirecv_CPPFLAGS = -DNONBLOCKING -DISEND $(AM_CPPFLAGS)

mt_ibsendirecv_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_ibsendirecv_CPPFLAGS = -DNONBLOCKING -DIBSEND $(AM_CPPFLAGS)

mt_issendirecv_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_issendirecv_CPPFLAGS = -DNONBLOCKING -DISSEND $(AM_CPPFLAGS)

# MT Send/Recv tests with large messages

mt_sendrecv_huge_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_sendrecv_huge_CPPFLAGS = -DHUGE_COUNT -DSEND $(AM_CPPFLAGS)

mt_bsendrecv_huge_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_bsendrecv_huge_CPPFLAGS = -DHUGE_COUNT -DBSEND $(AM_CPPFLAGS)

mt_ssendrecv_huge_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_ssendrecv_huge_CPPFLAGS = -DHUGE_COUNT -DSSEND $(AM_CPPFLAGS)

mt_isendirecv_huge_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_isendirecv_huge_CPPFLAGS = -DHUGE_COUNT -DNONBLOCKING -DISEND $(AM_CPPFLAGS)

mt_ibsendirecv_huge_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_ibsendirecv_huge_CPPFLAGS = -DHUGE_COUNT -DNONBLOCKING -DIBSEND $(AM_CPPFLAGS)

mt_issendirecv_huge_SOURCES = mt_pt2pt_common.c mt_sendrecv_impl.c
mt_issendirecv_huge_CPPFLAGS = -DHUGE_COUNT -DNONBLOCKING -DISSEND $(AM_CPPFLAGS)

# MT Probe family tests with small data send/recv

mt_probe_isendrecv_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_probe_isendrecv_CPPFLAGS = -DPROBE -DNBLOCKINGSEND -DISEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_probe_sendrecv_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_probe_sendrecv_CPPFLAGS = -DPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_iprobe_isendrecv_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_iprobe_isendrecv_CPPFLAGS = -DIPROBE -DNBLOCKINGSEND -DISEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_iprobe_sendrecv_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_iprobe_sendrecv_CPPFLAGS = -DIPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_mprobe_isendrecv_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_mprobe_isendrecv_CPPFLAGS = -DMPROBE -DNBLOCKINGSEND -DISEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_mprobe_sendrecv_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_mprobe_sendrecv_CPPFLAGS = -DMPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_mprobe_sendirecv_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_mprobe_sendirecv_CPPFLAGS = -DMPROBE -DBLOCKINGSEND -DSEND -DNBLOCKINGRECV $(AM_CPPFLAGS)

mt_improbe_isendrecv_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_improbe_isendrecv_CPPFLAGS = -DIMPROBE -DNBLOCKINGSEND -DISEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_improbe_sendrecv_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_improbe_sendrecv_CPPFLAGS = -DIMPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

# MT Probe family tests with large data send/recv

mt_probe_sendrecv_huge_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_probe_sendrecv_huge_CPPFLAGS = -DHUGE_COUNT -DPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_iprobe_sendrecv_huge_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_iprobe_sendrecv_huge_CPPFLAGS = -DHUGE_COUNT -DIPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_mprobe_sendrecv_huge_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_mprobe_sendrecv_huge_CPPFLAGS = -DHUGE_COUNT -DMPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

mt_improbe_sendrecv_huge_SOURCES =  mt_pt2pt_common.c mt_probe_impl.c
mt_improbe_sendrecv_huge_CPPFLAGS = -DHUGE_COUNT -DIMPROBE -DBLOCKINGSEND -DSEND -DBLOCKINGRECV $(AM_CPPFLAGS)

# MT Persistent Send/Recv tests with large messages
# Persistent test cases don't have the "small" versions

mt_sendrecv_pers_huge_SOURCES = mt_pt2pt_common.c mt_pers_sendrecv_impl.c
mt_sendrecv_pers_huge_CPPFLAGS = -DHUGE_COUNT -DSEND $(AM_CPPFLAGS)

mt_bsendrecv_pers_huge_SOURCES = mt_pt2pt_common.c mt_pers_sendrecv_impl.c
mt_bsendrecv_pers_huge_CPPFLAGS = -DHUGE_COUNT -DBSEND $(AM_CPPFLAGS)

mt_ssendrecv_pers_huge_SOURCES = mt_pt2pt_common.c mt_pers_sendrecv_impl.c
mt_ssendrecv_pers_huge_CPPFLAGS = -DHUGE_COUNT -DSSEND $(AM_CPPFLAGS)

mt_mpicancel_SOURCES = mt_mpicancel.c
