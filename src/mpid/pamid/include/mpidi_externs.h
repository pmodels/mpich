/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file include/mpidi_externs.h
 * \brief ???
 */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#ifndef __include_mpidi_externs_h__
#define __include_mpidi_externs_h__


extern pami_client_t  MPIDI_Client;
extern pami_context_t MPIDI_Context[];

extern MPIDI_Process_t MPIDI_Process;

extern advisor_table_t  MPIDI_Collsel_advisor_table;
extern pami_extension_t MPIDI_Collsel_extension;
extern advisor_params_t MPIDI_Collsel_advisor_params;
extern char            *MPIDI_Collsel_output_file;
extern pami_extension_collsel_advise MPIDI_Pamix_collsel_advise;

#endif
