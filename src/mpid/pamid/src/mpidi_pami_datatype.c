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
 * \file src/mpidi_pami_datatype.c
 * \brief pami_type_t datatype hooks
 */

#include <pami.h>
#include <mpidimpl.h>

/**
 * \brief Create PAMI datatype representation of MPI Datatype during commit.
 *
 * Signifcant performance improvements can be realized for one-sided communication
 * utilizing the PAMI_Rput_typed and PAMI_Rget_typed interface which requires a
 * PAMI representation of the MPI Datatype.
 */
void MPIDI_PAMI_datatype_commit_hook (MPI_Datatype *ptr)
{

    /* If the PAMID optimization to utilize the PAMI_Rput_typed / PAMI_Rget_typed call for
     * one-sided comm for derived types is enabled then we need to create the PAMI datatype.
     */
    if (MPIDI_Process.typed_onesided == 1) {

      MPID_Datatype *datatype_ptr;
      MPID_Datatype_get_ptr(*ptr, datatype_ptr);

      pami_result_t pami_dtop_result;
      datatype_ptr->device_datatype = (pami_type_t *) MPIU_Malloc(sizeof(pami_type_t));
      pami_dtop_result = PAMI_Type_create ((pami_type_t *)datatype_ptr->device_datatype);
      MPIU_Assert(pami_dtop_result == PAMI_SUCCESS);

      /* Flatten the non-contiguous data type into arrays describing the contiguous chunks.
       */
      MPI_Aint *dt_offset_array = (MPI_Aint *) MPIU_Malloc(datatype_ptr->max_contig_blocks * sizeof(MPI_Aint));
      MPI_Aint *dt_size_array = (MPI_Aint *) MPIU_Malloc(datatype_ptr->max_contig_blocks * sizeof(MPI_Aint));
      MPI_Aint dt_array_len = datatype_ptr->max_contig_blocks;
      int rc = MPIR_Type_flatten(*ptr, dt_offset_array, dt_size_array, &dt_array_len);

      /* Build the PAMI datatype adding one contiguous chunk at a time with the PAMI_Type_add_simple
       * interface.
       */
      int i;

      for (i=0;i<dt_array_len;i++) {
        size_t num_bytes_this_entry = dt_size_array[i];
        size_t cursor_offset;
        if (i == 0)
          cursor_offset = (size_t) dt_offset_array[i];
        else
          cursor_offset = (size_t) dt_offset_array[i] - (size_t)dt_offset_array[i-1];
        pami_dtop_result = PAMI_Type_add_simple (*(pami_type_t*)(datatype_ptr->device_datatype), num_bytes_this_entry, cursor_offset,  1, 0);
        MPIU_Assert(pami_dtop_result == PAMI_SUCCESS);
      }

      /* Complete the PAMI datatype and free arrays.
       */
      pami_dtop_result = PAMI_Type_complete (*(pami_type_t*)(datatype_ptr->device_datatype),1);
      MPIU_Assert(pami_dtop_result == PAMI_SUCCESS);
      MPIU_Free(dt_offset_array);
      MPIU_Free(dt_size_array);
    }
  return;
}

/**
 * \brief Destroy PAMI datatype representation of MPI Datatype.
 *
 */
void MPIDI_PAMI_datatype_destroy_hook (MPID_Datatype *ptr)
{
    /* If a PAMI datatype was created, destroy it if this is the
     * last reference to the MPID_Datatype ptr.
     */
    if ((MPIDI_Process.typed_onesided == 1) && (ptr->is_committed)) {
      if (ptr->device_datatype) {
        pami_result_t pami_dtop_result;
        pami_dtop_result = PAMI_Type_destroy ((pami_type_t *)ptr->device_datatype);
        MPIU_Assert(pami_dtop_result == PAMI_SUCCESS);
        MPIU_Free(ptr->device_datatype);
      }
    }
}

/**
 * \brief Create PAMI datatype representation of MPI Datatype during dup.
 *
 * Signifcant performance improvements can be realized for one-sided communication
 * utilizing the PAMI_Rput_typed and PAMI_Rget_typed interface which requires a
 * PAMI representation of the MPI Datatype.
 */
void MPIDI_PAMI_datatype_dup_hook (MPI_Datatype *ptr)
{

    /* If the PAMID optimization to utilize the PAMI_Rput_typed / PAMI_Rget_typed call for
     * one-sided comm for derived types is enabled then we need to create the PAMI datatype.
     */
    if (MPIDI_Process.typed_onesided == 1) {

      MPID_Datatype *datatype_ptr;
      MPID_Datatype_get_ptr(*ptr, datatype_ptr);

      pami_result_t pami_dtop_result;
      datatype_ptr->device_datatype = (pami_type_t *) MPIU_Malloc(sizeof(pami_type_t));
      pami_dtop_result = PAMI_Type_create ((pami_type_t *)datatype_ptr->device_datatype);
      MPIU_Assert(pami_dtop_result == PAMI_SUCCESS);

      /* Flatten the non-contiguous data type into arrays describing the contiguous chunks.
       */
      MPI_Aint *dt_offset_array = (MPI_Aint *) MPIU_Malloc(datatype_ptr->max_contig_blocks * sizeof(MPI_Aint));
      MPI_Aint *dt_size_array = (MPI_Aint *) MPIU_Malloc(datatype_ptr->max_contig_blocks * sizeof(MPI_Aint));
      MPI_Aint dt_array_len = datatype_ptr->max_contig_blocks;
      int rc = MPIR_Type_flatten(*ptr, dt_offset_array, dt_size_array, &dt_array_len);

      /* Build the PAMI datatype adding one contiguous chunk at a time with the PAMI_Type_add_simple
       * interface.
       */
      int i;

      for (i=0;i<dt_array_len;i++) {
        size_t num_bytes_this_entry = dt_size_array[i];
        size_t cursor_offset;
        if (i == 0)
          cursor_offset = (size_t) dt_offset_array[i];
        else
          cursor_offset = (size_t) dt_offset_array[i] - (size_t)dt_offset_array[i-1];
        pami_dtop_result = PAMI_Type_add_simple (*(pami_type_t*)(datatype_ptr->device_datatype), num_bytes_this_entry, cursor_offset,  1, 0);
        MPIU_Assert(pami_dtop_result == PAMI_SUCCESS);
      }

      /* Complete the PAMI datatype and free arrays.
       */
      pami_dtop_result = PAMI_Type_complete (*(pami_type_t*)(datatype_ptr->device_datatype),1);
      MPIU_Assert(pami_dtop_result == PAMI_SUCCESS);
      MPIU_Free(dt_offset_array);
      MPIU_Free(dt_size_array);
    }
  return;
}
