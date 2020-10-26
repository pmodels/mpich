/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "typerep_internal.h"
#include "yaksa.h"

int MPIR_Typerep_create_vector(int count, int blocklength, int stride, MPI_Datatype oldtype,
                               MPIR_Datatype * newtype)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_CREATE_VECTOR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_CREATE_VECTOR);

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_vector(count, blocklength, stride, type,
                                      NULL, (yaksa_type_t *) & newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    uintptr_t num_contig_blocks;
    rc = yaksa_iov_len(1, (yaksa_type_t) (intptr_t) newtype->typerep.handle, &num_contig_blocks);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    newtype->typerep.num_contig_blocks = (MPI_Aint) num_contig_blocks;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_CREATE_VECTOR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype,
                                MPIR_Datatype * newtype)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_CREATE_HVECTOR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_CREATE_HVECTOR);

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_hvector(count, blocklength, stride, type,
                                       NULL, (yaksa_type_t *) & newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    uintptr_t num_contig_blocks;
    rc = yaksa_iov_len(1, (yaksa_type_t) (intptr_t) newtype->typerep.handle, &num_contig_blocks);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    newtype->typerep.num_contig_blocks = (MPI_Aint) num_contig_blocks;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_CREATE_HVECTOR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_contig(int count, MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_CREATE_CONTIG);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_CREATE_CONTIG);

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc =
        yaksa_type_create_contig(count, type, NULL, (yaksa_type_t *) & newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    uintptr_t num_contig_blocks;
    rc = yaksa_iov_len(1, (yaksa_type_t) (intptr_t) newtype->typerep.handle, &num_contig_blocks);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    newtype->typerep.num_contig_blocks = (MPI_Aint) num_contig_blocks;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_CREATE_CONTIG);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_dup(MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_CREATE_DUP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_CREATE_DUP);

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_dup(type, NULL, (yaksa_type_t *) & newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    uintptr_t num_contig_blocks;
    rc = yaksa_iov_len(1, (yaksa_type_t) (intptr_t) newtype->typerep.handle, &num_contig_blocks);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    newtype->typerep.num_contig_blocks = (MPI_Aint) num_contig_blocks;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_CREATE_DUP);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_indexed_block(int count, int blocklength, const int *array_of_displacements,
                                      MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_CREATE_INDEXED_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_CREATE_INDEXED_BLOCK);

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_indexed_block(count, blocklength, array_of_displacements,
                                             type, NULL,
                                             (yaksa_type_t *) & newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    uintptr_t num_contig_blocks;
    rc = yaksa_iov_len(1, (yaksa_type_t) (intptr_t) newtype->typerep.handle, &num_contig_blocks);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    newtype->typerep.num_contig_blocks = (MPI_Aint) num_contig_blocks;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_CREATE_INDEXED_BLOCK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_hindexed_block(int count, int blocklength,
                                       const MPI_Aint * array_of_displacements,
                                       MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_CREATE_HINDEXED_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_CREATE_HINDEXED_BLOCK);

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_hindexed_block(count, blocklength, array_of_displacements,
                                              type, NULL,
                                              (yaksa_type_t *) & newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    uintptr_t num_contig_blocks;
    rc = yaksa_iov_len(1, (yaksa_type_t) (intptr_t) newtype->typerep.handle, &num_contig_blocks);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    newtype->typerep.num_contig_blocks = (MPI_Aint) num_contig_blocks;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_CREATE_HINDEXED_BLOCK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_indexed(int count, const int *array_of_blocklengths,
                                const int *array_of_displacements, MPI_Datatype oldtype,
                                MPIR_Datatype * newtype)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_CREATE_INDEXED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_CREATE_INDEXED);

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_indexed(count, array_of_blocklengths, array_of_displacements,
                                       type, NULL, (yaksa_type_t *) & newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    uintptr_t num_contig_blocks;
    rc = yaksa_iov_len(1, (yaksa_type_t) (intptr_t) newtype->typerep.handle, &num_contig_blocks);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    newtype->typerep.num_contig_blocks = (MPI_Aint) num_contig_blocks;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_CREATE_INDEXED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_hindexed(int count, const int *array_of_blocklengths,
                                 const MPI_Aint * array_of_displacements, MPI_Datatype oldtype,
                                 MPIR_Datatype * newtype)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_CREATE_HINDEXED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_CREATE_HINDEXED);

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_hindexed(count, array_of_blocklengths, array_of_displacements,
                                        type, NULL, (yaksa_type_t *) & newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    uintptr_t num_contig_blocks;
    rc = yaksa_iov_len(1, (yaksa_type_t) (intptr_t) newtype->typerep.handle, &num_contig_blocks);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    newtype->typerep.num_contig_blocks = (MPI_Aint) num_contig_blocks;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_CREATE_HINDEXED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                                MPIR_Datatype * newtype)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_CREATE_RESIZED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_CREATE_RESIZED);

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_resized(type, lb, extent, NULL,
                                       (yaksa_type_t *) & newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    uintptr_t num_contig_blocks;
    rc = yaksa_iov_len(1, (yaksa_type_t) (intptr_t) newtype->typerep.handle, &num_contig_blocks);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    newtype->typerep.num_contig_blocks = (MPI_Aint) num_contig_blocks;

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_CREATE_RESIZED);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_struct(int count, const int *array_of_blocklengths,
                               const MPI_Aint * array_of_displacements,
                               const MPI_Datatype * array_of_types, MPIR_Datatype * newtype)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_CREATE_STRUCT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_CREATE_STRUCT);

    int mpi_errno = MPI_SUCCESS;
    yaksa_type_t *array_of_yaksa_types = (yaksa_type_t *) MPL_malloc(count * sizeof(yaksa_type_t),
                                                                     MPL_MEM_DATATYPE);

    for (int i = 0; i < count; i++) {
        array_of_yaksa_types[i] = MPII_Typerep_get_yaksa_type(array_of_types[i]);
    }

    int rc = yaksa_type_create_struct(count, array_of_blocklengths, array_of_displacements,
                                      array_of_yaksa_types, NULL,
                                      (yaksa_type_t *) & newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    uintptr_t num_contig_blocks;
    rc = yaksa_iov_len(1, (yaksa_type_t) (intptr_t) newtype->typerep.handle, &num_contig_blocks);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    newtype->typerep.num_contig_blocks = (MPI_Aint) num_contig_blocks;

    MPL_free(array_of_yaksa_types);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_CREATE_STRUCT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_subarray(int ndims, const int *array_of_sizes, const int *array_of_subsizes,
                                 const int *array_of_starts, int order,
                                 MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_CREATE_SUBARRAY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_CREATE_SUBARRAY);

    /* MPICH breaks down subarrays into smaller types, so we don't
     * need to use yaksa subarray types */
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_CREATE_SUBARRAY);
    return MPI_SUCCESS;
}

int MPIR_Typerep_create_darray(int size, int rank, int ndims, const int *array_of_gsizes,
                               const int *array_of_distribs, const int *array_of_dargs,
                               const int *array_of_psizes, int order, MPI_Datatype oldtype,
                               MPIR_Datatype * newtype)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_CREATE_DARRAY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_CREATE_DARRAY);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_CREATE_DARRAY);
    return MPI_SUCCESS;
}
