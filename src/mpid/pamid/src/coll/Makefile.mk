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

include $(top_srcdir)/src/mpid/pamid/src/coll/barrier/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/bcast/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/allreduce/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/allgather/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/allgatherv/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/scatterv/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/scatter/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/gather/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/alltoall/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/alltoallv/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/gatherv/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/scan/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/reduce/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/alltoallw/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/exscan/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/ired_scat_block/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/ired_scat/Makefile.mk
include $(top_srcdir)/src/mpid/pamid/src/coll/red_scat/Makefile.mk

mpi_core_sources +=               \
    src/mpid/pamid/src/coll/coll_utils.c


endif BUILD_PAMID

