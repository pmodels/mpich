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
    src/mpid/pamid/include/mpidi_thread.h                            \
    src/mpid/pamid/include/mpidimpl.h                                \
    src/mpid/pamid/include/mpidi_mutex.h                             \
    src/mpid/pamid/include/mpidpost.h                                \
    src/mpid/pamid/include/mpidi_constants.h                         \
    src/mpid/pamid/include/mpidi_externs.h                           \
    src/mpid/pamid/include/mpidi_util.h                              \
    src/mpid/pamid/include/mpidi_hooks.h                             \
    src/mpid/pamid/include/mpidi_macros.h                            \
    src/mpid/pamid/include/mpidi_datatypes.h                         \
    src/mpid/pamid/include/mpidi_prototypes.h                        \
    src/mpid/pamid/include/pamix.h                                   \
    src/mpid/pamid/include/mpidpre.h                                 \
    src/mpid/pamid/include/mpidi_platform.h

include_HEADERS += src/mpid/pamid/include/mpix.h

include $(top_srcdir)/src/mpid/pamid/src/Makefile.mk

endif BUILD_PAMID
