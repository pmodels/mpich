# begin_generated_IBM_copyright_prolog                             
#                                                                  
# This is an automatically generated copyright prolog.             
# After initializing,  DO NOT MODIFY OR MOVE                       
#  --------------------------------------------------------------- 
# Licensed Materials - Property of IBM                             
# Blue Gene/Q 5765-PER 5765-PRP                                    
#                                                                  
# (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           
# US Government Users Restricted Rights -                          
# Use, duplication, or disclosure restricted                       
# by GSA ADP Schedule Contract with IBM Corp.                      
#                                                                  
#  --------------------------------------------------------------- 
#                                                                  
# end_generated_IBM_copyright_prolog                               
# -*- mode: makefile-gmake; -*-

# note that the includes always happen but the effects of their contents are
# affected by "if BUILD_PAMID"
if BUILD_PAMID

#AM_CPPFLAGS += -I$(top_srcdir)/src/mpid/pamid/src/include   \
#               -I$(top_builddir)/src/mpid/pamid/src/include

noinst_HEADERS +=                                                    \
    src/mpid/pamid/src/mpid_request.h                                \
    src/mpid/pamid/src/mpid_progress.h                               \
    src/mpid/pamid/src/mpid_recvq.h


include $(top_srcdir)/src/mpid/pamid/src/coll/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/comm/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/misc/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/mpix/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/onesided/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/pamix/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/pt2pt/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/dyntask/Makefile.mk


mpi_core_sources +=               \
    src/mpid/pamid/src/mpid_buffer.c            \
    src/mpid/pamid/src/mpidi_bufmm.c            \
    src/mpid/pamid/src/mpid_finalize.c          \
    src/mpid/pamid/src/mpid_init.c              \
    src/mpid/pamid/src/mpid_iprobe.c            \
    src/mpid/pamid/src/mpid_probe.c             \
    src/mpid/pamid/src/mpid_progress.c          \
    src/mpid/pamid/src/mpid_recvq.c             \
    src/mpid/pamid/src/mpid_request.c           \
    src/mpid/pamid/src/mpid_time.c              \
    src/mpid/pamid/src/mpid_vc.c                \
    src/mpid/pamid/src/mpidi_env.c              \
    src/mpid/pamid/src/mpidi_util.c             \
    src/mpid/pamid/src/mpidi_mutex.c            \
    src/mpid/pamid/src/mpid_mrecv.c             \
    src/mpid/pamid/src/mpid_mprobe.c            \
    src/mpid/pamid/src/mpid_imrecv.c            \
    src/mpid/pamid/src/mpid_improbe.c           \
    src/mpid/pamid/src/mpid_aint.c              \
    src/mpid/pamid/src/mpidi_nbc_sched.c        \
    src/mpid/pamid/src/mpidi_pami_datatype.c

if QUEUE_BINARY_SEARCH_SUPPORT
mpi_core_sources +=                             \
    src/mpid/pamid/src/mpid_recvq_mmap.cpp
endif QUEUE_BINARY_SEARCH_SUPPORT



endif BUILD_PAMID

