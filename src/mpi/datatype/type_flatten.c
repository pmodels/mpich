/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpiimpl.h>
#include <mpir_dataloop.h>
#include <stdlib.h>

struct flatten_hdr {
    MPI_Aint size;
    MPI_Aint extent, ub, lb, true_ub, true_lb;
    int has_sticky_ub, has_sticky_lb;
    int is_contig;
    int basic_type;
    MPI_Aint max_contig_blocks;
};

/*
 * MPIR_Type_flatten_size
 *
 * Parameters:
 * datatype_ptr        - (IN)  datatype to flatten
 * flattened_type_size - (OUT) buffer size needed for the flattened representation
 */
int MPIR_Type_flatten_size(MPIR_Datatype * datatype_ptr, int *flattened_type_size)
{
    int flattened_loop_size;
    int mpi_errno = MPI_SUCCESS;

    MPIR_Dataloop_flatten_size(datatype_ptr, &flattened_loop_size);

    *flattened_type_size = flattened_loop_size + sizeof(struct flatten_hdr);

    return mpi_errno;
}

/*
 * MPIR_Type_flatten
 *
 * Parameters:
 * datatype_ptr   - (IN)  datatype to flatten
 * flattened_type - (OUT) buffer that will contain the flattened representation
 */
int MPIR_Type_flatten(MPIR_Datatype * datatype_ptr, void *flattened_type)
{
    struct flatten_hdr *flatten_hdr = (struct flatten_hdr *) flattened_type;
    void *flattened_dataloop = (void *) ((char *) flattened_type + sizeof(struct flatten_hdr));
    int mpi_errno = MPI_SUCCESS;

    flatten_hdr->size = datatype_ptr->size;
    flatten_hdr->extent = datatype_ptr->extent;
    flatten_hdr->ub = datatype_ptr->ub;
    flatten_hdr->lb = datatype_ptr->lb;
    flatten_hdr->true_ub = datatype_ptr->true_ub;
    flatten_hdr->true_lb = datatype_ptr->true_lb;
    flatten_hdr->has_sticky_ub = datatype_ptr->has_sticky_ub;
    flatten_hdr->has_sticky_lb = datatype_ptr->has_sticky_lb;
    flatten_hdr->is_contig = datatype_ptr->is_contig;
    flatten_hdr->basic_type = datatype_ptr->basic_type;
    flatten_hdr->max_contig_blocks = datatype_ptr->max_contig_blocks;

    mpi_errno = MPIR_Dataloop_flatten(datatype_ptr, flattened_dataloop);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/*
 * MPIR_Type_unflatten
 *
 * Parameters:
 * datatype_ptr   - (OUT) datatype into which the buffer will be unflattened
 * flattened_type - (IN)  buffer that contains the flattened representation
 */
int MPIR_Type_unflatten(MPIR_Datatype * datatype_ptr, void *flattened_type)
{
    struct flatten_hdr *flatten_hdr = (struct flatten_hdr *) flattened_type;
    void *flattened_dataloop =
        (MPIR_Dataloop *) ((char *) flattened_type + sizeof(struct flatten_hdr));
    int mpi_errno = MPI_SUCCESS;

    datatype_ptr->is_committed = 1;
    datatype_ptr->attributes = 0;
    datatype_ptr->name[0] = 0;
    datatype_ptr->is_contig = flatten_hdr->is_contig;
    datatype_ptr->max_contig_blocks = flatten_hdr->max_contig_blocks;
    datatype_ptr->size = flatten_hdr->size;
    datatype_ptr->extent = flatten_hdr->extent;
    datatype_ptr->basic_type = flatten_hdr->basic_type;
    datatype_ptr->ub = flatten_hdr->ub;
    datatype_ptr->lb = flatten_hdr->lb;
    datatype_ptr->true_ub = flatten_hdr->true_ub;
    datatype_ptr->true_lb = flatten_hdr->true_lb;
    datatype_ptr->has_sticky_ub = flatten_hdr->has_sticky_ub;
    datatype_ptr->has_sticky_lb = flatten_hdr->has_sticky_lb;
    datatype_ptr->contents = NULL;

    mpi_errno = MPIR_Dataloop_unflatten(datatype_ptr, flattened_dataloop);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
