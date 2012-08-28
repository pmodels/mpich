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
    src/mpid/pamid/src/onesided/mpidi_onesided.h


lib_lib@MPILIBNAME@_la_SOURCES +=                                    \
  src/mpid/pamid/src/onesided/mpid_1s.c                              \
  src/mpid/pamid/src/onesided/mpid_win_accumulate.c                  \
  src/mpid/pamid/src/onesided/mpid_win_create.c                      \
  src/mpid/pamid/src/onesided/mpid_win_fence.c                       \
  src/mpid/pamid/src/onesided/mpid_win_free.c                        \
  src/mpid/pamid/src/onesided/mpid_win_get.c                         \
  src/mpid/pamid/src/onesided/mpid_win_lock.c                        \
  src/mpid/pamid/src/onesided/mpid_win_pscw.c                        \
  src/mpid/pamid/src/onesided/mpid_win_put.c                         \
  src/mpid/pamid/src/onesided/mpidi_win_control.c


endif BUILD_PAMID

