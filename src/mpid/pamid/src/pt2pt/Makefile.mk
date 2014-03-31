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


noinst_HEADERS +=                                                    \
    src/mpid/pamid/src/pt2pt/mpid_send.h                             \
    src/mpid/pamid/src/pt2pt/mpidi_recv.h                            \
    src/mpid/pamid/src/pt2pt/mpidi_send.h                            \
    src/mpid/pamid/src/pt2pt/mpid_irecv.h                            \
    src/mpid/pamid/src/pt2pt/mpid_isend.h

include $(top_srcdir)/src/mpid/pamid/src/pt2pt/persistent/Makefile.mk

mpi_core_sources +=                                    \
  src/mpid/pamid/src/pt2pt/mpid_cancel.c                             \
  src/mpid/pamid/src/pt2pt/mpid_issend.c                             \
  src/mpid/pamid/src/pt2pt/mpid_recv.c                               \
  src/mpid/pamid/src/pt2pt/mpid_ssend.c                              \
  src/mpid/pamid/src/pt2pt/mpidi_callback_eager.c                    \
  src/mpid/pamid/src/pt2pt/mpidi_callback_rzv.c                      \
  src/mpid/pamid/src/pt2pt/mpidi_callback_short.c                    \
  src/mpid/pamid/src/pt2pt/mpidi_callback_util.c                     \
  src/mpid/pamid/src/pt2pt/mpidi_control.c                           \
  src/mpid/pamid/src/pt2pt/mpidi_done.c                              \
  src/mpid/pamid/src/pt2pt/mpidi_recvmsg.c                           \
  src/mpid/pamid/src/pt2pt/mpidi_rendezvous.c                        \
  src/mpid/pamid/src/pt2pt/mpidi_sendmsg.c


endif BUILD_PAMID

