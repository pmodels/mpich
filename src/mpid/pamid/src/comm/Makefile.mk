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


mpi_core_sources +=               \
    src/mpid/pamid/src/comm/mpid_comm.c         \
    src/mpid/pamid/src/comm/mpid_selectcolls.c  \
    src/mpid/pamid/src/comm/mpid_optcolls.c


endif BUILD_PAMID

