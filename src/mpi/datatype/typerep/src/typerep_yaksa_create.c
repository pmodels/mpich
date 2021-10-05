/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "typerep_internal.h"
#include "yaksa.h"

/* yaksa may disagree with mpich on some internal fields. To ensure consistency,
 * we update these fields here by querying yaksa */

/* internal helper function
 * Note oldtype is a datatype handle
 *      count is the count of oldtype as a unit
 */
static int update_yaksa_type(MPIR_Datatype * newtype, MPI_Datatype oldtype, MPI_Aint count)
{
    int mpi_errno = MPI_SUCCESS;
    int rc;
    yaksa_type_t dt = newtype->typerep.handle;

    uintptr_t num_contig_blocks;
    int cnt = 2;
    rc = yaksa_iov_len(cnt, dt, &num_contig_blocks);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    if (num_contig_blocks == 1) {
        newtype->is_contig = 1;
        newtype->typerep.num_contig_blocks = 1;
    } else {
        newtype->is_contig = 0;
        newtype->typerep.num_contig_blocks = (MPI_Aint) num_contig_blocks / cnt;
    }

    rc = yaksa_type_get_size(dt, (uintptr_t *) & newtype->size);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    rc = yaksa_type_get_extent(dt, (intptr_t *) & newtype->lb, (intptr_t *) & newtype->extent);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");
    newtype->ub = newtype->lb + newtype->extent;
    MPI_Aint true_extent;
    rc = yaksa_type_get_true_extent(dt, (intptr_t *) & newtype->true_lb,
                                    (intptr_t *) & true_extent);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    newtype->true_ub = newtype->true_lb + true_extent;

    if (count == 0) {
        /* this is a struct, deal with it in MPIR_Typerep_create_struct */
    } else if (HANDLE_IS_BUILTIN(oldtype)) {
        MPI_Aint el_sz = (MPI_Aint) MPIR_Datatype_get_basic_size(oldtype);
        newtype->n_builtin_elements = count;
        newtype->builtin_element_size = el_sz;
        newtype->basic_type = oldtype;
    } else {
        MPIR_Datatype *old_dtp;
        MPIR_Datatype_get_ptr(oldtype, old_dtp);

        newtype->n_builtin_elements = count * old_dtp->n_builtin_elements;
        newtype->builtin_element_size = old_dtp->builtin_element_size;
        newtype->basic_type = old_dtp->basic_type;
    }
  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_vector(MPI_Aint count, MPI_Aint blocklength, MPI_Aint stride,
                               MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_vector(count, blocklength, stride, type,
                                      NULL, &newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    mpi_errno = update_yaksa_type(newtype, oldtype, count * blocklength);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_hvector(MPI_Aint count, MPI_Aint blocklength, MPI_Aint stride,
                                MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_hvector(count, blocklength, stride, type,
                                       NULL, &newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    mpi_errno = update_yaksa_type(newtype, oldtype, count * blocklength);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_contig(MPI_Aint count, MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_contig(count, type,
                                      NULL, &newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    mpi_errno = update_yaksa_type(newtype, oldtype, count);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_dup(MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_dup(type, NULL, &newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    mpi_errno = update_yaksa_type(newtype, oldtype, 1);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_indexed_block(MPI_Aint count, MPI_Aint blocklength,
                                      const MPI_Aint * array_of_displacements,
                                      MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_indexed_block(count, blocklength,
                                             (const intptr_t *) array_of_displacements,
                                             type, NULL,
                                             &newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    mpi_errno = update_yaksa_type(newtype, oldtype, count * blocklength);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_hindexed_block(MPI_Aint count, MPI_Aint blocklength,
                                       const MPI_Aint * array_of_displacements,
                                       MPI_Datatype oldtype, MPIR_Datatype * newtype)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_hindexed_block(count, blocklength,
                                              (const intptr_t *) array_of_displacements, type, NULL,
                                              &newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    mpi_errno = update_yaksa_type(newtype, oldtype, count * blocklength);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_indexed(MPI_Aint count, const MPI_Aint * array_of_blocklengths,
                                const MPI_Aint * array_of_displacements, MPI_Datatype oldtype,
                                MPIR_Datatype * newtype)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_indexed(count, (const intptr_t *) array_of_blocklengths,
                                       (const intptr_t *) array_of_displacements,
                                       type, NULL, &newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    MPI_Aint old_ct = 0;
    for (MPI_Aint i = 0; i < count; i++) {
        old_ct += array_of_blocklengths[i];
    }
    mpi_errno = update_yaksa_type(newtype, oldtype, old_ct);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_hindexed(MPI_Aint count, const MPI_Aint * array_of_blocklengths,
                                 const MPI_Aint * array_of_displacements, MPI_Datatype oldtype,
                                 MPIR_Datatype * newtype)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_hindexed(count, (const intptr_t *) array_of_blocklengths,
                                        (const intptr_t *) array_of_displacements,
                                        type, NULL, &newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    MPI_Aint old_ct = 0;
    for (MPI_Aint i = 0; i < count; i++) {
        old_ct += array_of_blocklengths[i];
    }
    mpi_errno = update_yaksa_type(newtype, oldtype, old_ct);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent,
                                MPIR_Datatype * newtype)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;

    yaksa_type_t type = MPII_Typerep_get_yaksa_type(oldtype);

    int rc = yaksa_type_create_resized(type, lb, extent, NULL,
                                       &newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    mpi_errno = update_yaksa_type(newtype, oldtype, 1);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_struct(MPI_Aint count, const MPI_Aint * array_of_blocklengths,
                               const MPI_Aint * array_of_displacements,
                               const MPI_Datatype * array_of_types, MPIR_Datatype * newtype)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    yaksa_type_t *array_of_yaksa_types = (yaksa_type_t *) MPL_malloc(count * sizeof(yaksa_type_t),
                                                                     MPL_MEM_DATATYPE);

    for (MPI_Aint i = 0; i < count; i++) {
        array_of_yaksa_types[i] = MPII_Typerep_get_yaksa_type(array_of_types[i]);
    }

    int rc = yaksa_type_create_struct(count, (const intptr_t *) array_of_blocklengths,
                                      (const intptr_t *) array_of_displacements,
                                      array_of_yaksa_types, NULL,
                                      &newtype->typerep.handle);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_INTERN, "**yaksa");

    mpi_errno = update_yaksa_type(newtype, MPI_DATATYPE_NULL, 0);
    MPIR_ERR_CHECK(mpi_errno);

    MPI_Aint el_sz = 0;
    MPI_Datatype el_type = MPI_DATATYPE_NULL;
    int found_el_type = 0;
    for (int i = 0; i < count; i++) {
        MPI_Aint tmp_el_sz;
        MPI_Datatype tmp_el_type;
        MPIR_Datatype *old_dtp = NULL;

        if (array_of_blocklengths[i] == 0)
            continue;

        if (HANDLE_IS_BUILTIN(array_of_types[i])) {
            tmp_el_sz = MPIR_Datatype_get_basic_size(array_of_types[i]);
            tmp_el_type = array_of_types[i];
        } else {
            MPIR_Datatype_get_ptr(array_of_types[i], old_dtp);
            tmp_el_sz = old_dtp->builtin_element_size;
            tmp_el_type = old_dtp->basic_type;
        }

        if (found_el_type == 0) {
            el_sz = tmp_el_sz;
            el_type = tmp_el_type;
            found_el_type = 1;
        } else if (el_sz != tmp_el_sz) {
            el_sz = -1;
            el_type = MPI_DATATYPE_NULL;
        } else if (el_type != tmp_el_type) {
            /* Q: should we set el_sz = -1 even though the same? */
            el_type = MPI_DATATYPE_NULL;
        }
    }
    newtype->n_builtin_elements = -1;   /* TODO */
    newtype->builtin_element_size = el_sz;
    newtype->basic_type = el_type;

    MPL_free(array_of_yaksa_types);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Typerep_create_pairtype(MPI_Datatype type, MPIR_Datatype * newtype)
{
    MPIR_FUNC_ENTER;

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}
