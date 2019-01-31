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

#undef FUNCNAME
#define FUNCNAME MPIR_Type_flatten
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Type_flatten(MPIR_Datatype * datatype_ptr, void **flattened_type, int *flattened_type_size)
{
    struct flatten_hdr flatten_hdr;
    void *flattened_dataloop;
    int flattened_dataloop_size;
    int mpi_errno = MPI_SUCCESS;

    flatten_hdr.size = datatype_ptr->size;
    flatten_hdr.extent = datatype_ptr->extent;
    flatten_hdr.ub = datatype_ptr->ub;
    flatten_hdr.lb = datatype_ptr->lb;
    flatten_hdr.true_ub = datatype_ptr->true_ub;
    flatten_hdr.true_lb = datatype_ptr->true_lb;
    flatten_hdr.has_sticky_ub = datatype_ptr->has_sticky_ub;
    flatten_hdr.has_sticky_lb = datatype_ptr->has_sticky_lb;
    flatten_hdr.is_contig = datatype_ptr->is_contig;
    flatten_hdr.basic_type = datatype_ptr->basic_type;
    flatten_hdr.max_contig_blocks = datatype_ptr->max_contig_blocks;

    MPIR_Dataloop_flatten(datatype_ptr, &flattened_dataloop, &flattened_dataloop_size);

    *flattened_type_size = sizeof(struct flatten_hdr) + flattened_dataloop_size;
    *flattened_type = MPL_malloc(*flattened_type_size, MPL_MEM_DATATYPE);

    MPIR_Memcpy(*flattened_type, &flatten_hdr, sizeof(struct flatten_hdr));
    MPIR_Memcpy((((char *) (*flattened_type)) + sizeof(struct flatten_hdr)), flattened_dataloop,
                flattened_dataloop_size);

    MPL_free(flattened_dataloop);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Type_unflatten
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Type_unflatten(void *flattened_type, int flattened_type_size,
                        MPIR_Datatype ** datatype_ptr)
{
    struct flatten_hdr *flatten_hdr = (struct flatten_hdr *) flattened_type;
    void *flattened_dataloop =
        (MPIR_Dataloop *) ((char *) flattened_type + sizeof(struct flatten_hdr));
    MPIR_Datatype *new_dtp;
    int mpi_errno = MPI_SUCCESS;

    new_dtp = (MPIR_Datatype *) MPIR_Handle_obj_alloc(&MPIR_Datatype_mem);
    if (!new_dtp) {
        MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                             "MPIR_Datatype_mem");
    }

    *datatype_ptr = new_dtp;

    /* Note: handle is filled in by MPIR_Handle_obj_alloc() */
    MPIR_Object_set_ref(new_dtp, 1);
    new_dtp->is_committed = 1;
    new_dtp->attributes = 0;
    new_dtp->name[0] = 0;
    new_dtp->is_contig = flatten_hdr->is_contig;
    new_dtp->max_contig_blocks = flatten_hdr->max_contig_blocks;
    new_dtp->size = flatten_hdr->size;
    new_dtp->extent = flatten_hdr->extent;
    new_dtp->basic_type = flatten_hdr->basic_type;
    new_dtp->ub = flatten_hdr->ub;
    new_dtp->lb = flatten_hdr->lb;
    new_dtp->true_ub = flatten_hdr->true_ub;
    new_dtp->true_lb = flatten_hdr->true_lb;
    new_dtp->has_sticky_ub = flatten_hdr->has_sticky_ub;
    new_dtp->has_sticky_lb = flatten_hdr->has_sticky_lb;
    new_dtp->contents = NULL;

    MPIR_Dataloop_unflatten(new_dtp, flattened_dataloop);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
