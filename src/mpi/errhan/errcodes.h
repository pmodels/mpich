/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef ERRCODES_H_INCLUDED
#define ERRCODES_H_INCLUDED

/* Prototypes for internal routines for the errhandling module */
int MPIR_Err_set_msg( int, const char * );
int MPIR_Err_add_class( void );
int MPIR_Err_add_code( int );

/* 
   This file contains the definitions of the error code fields
   
   An error code is organized as

   is-dynamic? specific-msg-sequence# specific-msg-index 
                                             generic-code is-fatal? class

   where
   class: The MPI error class (including dynamically defined classes)
   is-dynamic?: Set if this is a dynamically created error code (using
      the routines to add error classes and codes at runtime).  This *must*
      be the top bit so that MPI_ERR_LASTCODE and MPI_LASTUSEDCODE can
      be set properly.  (MPI_ERR_LASTCODE must be the largest valid 
      error *code* from the predefined codes.  The standard text is poorly
      worded here, but users and the rationale will expect to be able to 
      perform (errcode <= MPI_ERR_LASTCODE).  See Section 8.5 in the MPI-2
      standard for MPI_LASTUSEDCODE.
   generic-code: Index into the array of generic messages
   specific-msg-index: index to the *buffer* containing recent 
   instance-specific error messages.
   specific-msg-sequence#: A sequence number used to check that the specific
   message text is still valid.
   is-fatal?: the error is fatal and should not be returned to the user

   Note that error codes must also be positive integers, so we lose one 
   bit (if they aren't positive, the comparisons agains MPI_ERR_LASTCODE 
   and the value of the attribute MPI_LASTUSEDCODE will fail).
 */

/* the error class bits are defined in mpierror.h, are 0x0000007f */
#define ERROR_CLASS_MASK          MPIR_ERR_CLASS_MASK  
#define ERROR_CLASS_SIZE          MPIR_ERR_CLASS_SIZE
#define ERROR_DYN_MASK            0x40000000
#define ERROR_DYN_SHIFT           30
#define ERROR_GENERIC_MASK        0x0007FF00
#define ERROR_GENERIC_SIZE        2048
#define ERROR_GENERIC_SHIFT       8
#define ERROR_SPECIFIC_INDEX_MASK 0x03F80000
#define ERROR_SPECIFIC_INDEX_SIZE 128
#define ERROR_SPECIFIC_INDEX_SHIFT 19
#define ERROR_SPECIFIC_SEQ_MASK   0x3C000000
/* Size is size of field as an integer, not the number of bits */
#define ERROR_SPECIFIC_SEQ_SIZE   16
#define ERROR_SPECIFIC_SEQ_SHIFT  26
#define ERROR_FATAL_MASK          0x00000080
#define ERROR_GET_CLASS(mpi_errno_) MPIR_ERR_GET_CLASS(mpi_errno_)

/* These must correspond to the masks defined above */
#define ERROR_MAX_NCLASS ERROR_CLASS_SIZE
#define ERROR_MAX_NCODE  8192
#endif
