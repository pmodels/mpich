/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "yaksa.h"
#include "typerep_internal.h"

/* yaksa fixed types - supports type size and alignment, communication only */
static yaksa_type_t TYPEREP_YAKSA_TYPE__FIXED1 = YAKSA_TYPE__NULL;
static yaksa_type_t TYPEREP_YAKSA_TYPE__FIXED2 = YAKSA_TYPE__NULL;
static yaksa_type_t TYPEREP_YAKSA_TYPE__FIXED4 = YAKSA_TYPE__NULL;
static yaksa_type_t TYPEREP_YAKSA_TYPE__FIXED8 = YAKSA_TYPE__NULL;
static yaksa_type_t TYPEREP_YAKSA_TYPE__FIXED16 = YAKSA_TYPE__NULL;
static yaksa_type_t TYPEREP_YAKSA_TYPE__FIXED32 = YAKSA_TYPE__NULL;

yaksa_info_t MPII_yaksa_info_nogpu;

yaksa_type_t MPII_Typerep_get_yaksa_type(MPI_Datatype type)
{
    MPIR_FUNC_ENTER;

    yaksa_type_t yaksa_type = YAKSA_TYPE__NULL;

    if (type == MPI_DATATYPE_NULL) {
        yaksa_type = YAKSA_TYPE__NULL;
        goto fn_exit;
    }

    switch (MPIR_DATATYPE_GET_RAW_INTERNAL(type)) {
        case MPIR_INT8:
            yaksa_type = YAKSA_TYPE__INT8_T;
            break;

        case MPIR_UINT8:
            yaksa_type = YAKSA_TYPE__UINT8_T;
            break;

        case MPIR_FIXED8:
        case MPIR_FLOAT8:
            yaksa_type = TYPEREP_YAKSA_TYPE__FIXED1;
            break;

        case MPIR_INT16:
            yaksa_type = YAKSA_TYPE__INT16_T;
            break;

        case MPIR_UINT16:
            yaksa_type = YAKSA_TYPE__UINT16_T;
            break;

        case MPIR_FIXED16:
        case MPIR_FLOAT16:
        case MPIR_BFLOAT16:
        case MPIR_COMPLEX8:
            yaksa_type = TYPEREP_YAKSA_TYPE__FIXED2;
            break;

        case MPIR_INT32:
            yaksa_type = YAKSA_TYPE__INT32_T;
            break;

        case MPIR_UINT32:
            yaksa_type = YAKSA_TYPE__UINT32_T;
            break;

        case MPIR_FIXED32:
        case MPIR_COMPLEX16:
            yaksa_type = TYPEREP_YAKSA_TYPE__FIXED4;
            break;

        case MPIR_INT64:
            yaksa_type = YAKSA_TYPE__INT64_T;
            break;

        case MPIR_UINT64:
            yaksa_type = YAKSA_TYPE__UINT64_T;
            break;

        case MPIR_FIXED64:
            yaksa_type = TYPEREP_YAKSA_TYPE__FIXED8;
            break;

        case MPIR_FLOAT32:
            yaksa_type = YAKSA_TYPE__FLOAT;
            break;

        case MPIR_FLOAT64:
            yaksa_type = YAKSA_TYPE__DOUBLE;
            break;

        case MPIR_COMPLEX32:
            yaksa_type = YAKSA_TYPE__C_COMPLEX;
            break;

        case MPIR_COMPLEX64:
            yaksa_type = YAKSA_TYPE__C_DOUBLE_COMPLEX;
            break;

        case MPIR_ALT_FLOAT96:
        case MPIR_ALT_FLOAT128:
            yaksa_type = YAKSA_TYPE__LONG_DOUBLE;
            break;

        case MPIR_ALT_COMPLEX96:
        case MPIR_ALT_COMPLEX128:
            yaksa_type = YAKSA_TYPE__C_LONG_DOUBLE_COMPLEX;
            break;

        case MPIR_FIXED128:
        case MPIR_INT128:
        case MPIR_UINT128:
        case MPIR_FLOAT128:
            yaksa_type = TYPEREP_YAKSA_TYPE__FIXED16;
            break;

        case MPIR_COMPLEX128:
            yaksa_type = TYPEREP_YAKSA_TYPE__FIXED32;
            break;

        case MPIR_2INT32:
            yaksa_type = YAKSA_TYPE__2INT;
            break;
        case MPIR_2FLOAT32:
            yaksa_type = YAKSA_TYPE__C_COMPLEX;
            break;
        case MPIR_2FLOAT64:
            yaksa_type = YAKSA_TYPE__C_DOUBLE_COMPLEX;
            break;
        case MPIR_2FLOAT128:
            yaksa_type = TYPEREP_YAKSA_TYPE__FIXED32;
            break;

        default:
            MPIR_Datatype * typeptr;
            MPIR_Datatype_get_ptr(type, typeptr);
            MPIR_Assert(typeptr != NULL);
            yaksa_type = typeptr->typerep.handle;
            break;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return yaksa_type;
}

yaksa_op_t MPII_Typerep_get_yaksa_op(MPI_Op op)
{
    MPIR_FUNC_ENTER;

    yaksa_op_t yaksa_op = YAKSA_OP__LAST;

    switch (op) {
        case MPI_SUM:
            yaksa_op = YAKSA_OP__SUM;
            break;
        case MPI_PROD:
            yaksa_op = YAKSA_OP__PROD;
            break;
        case MPI_MAX:
            yaksa_op = YAKSA_OP__MAX;
            break;
        case MPI_MIN:
            yaksa_op = YAKSA_OP__MIN;
            break;
        case MPI_LOR:
            yaksa_op = YAKSA_OP__LOR;
            break;
        case MPI_LAND:
            yaksa_op = YAKSA_OP__LAND;
            break;
        case MPI_LXOR:
            yaksa_op = YAKSA_OP__LXOR;
            break;
        case MPI_BOR:
            yaksa_op = YAKSA_OP__BOR;
            break;
        case MPI_BAND:
            yaksa_op = YAKSA_OP__BAND;
            break;
        case MPI_BXOR:
            yaksa_op = YAKSA_OP__BXOR;
            break;
        case MPI_REPLACE:
            yaksa_op = YAKSA_OP__REPLACE;
            break;
        default:
            yaksa_op = YAKSA_OP__LAST;
            break;
    }

    MPIR_FUNC_EXIT;
    return yaksa_op;
}

void MPIR_Typerep_init(void)
{
    MPIR_FUNC_ENTER;

    /* Yaksa API uses intptr_t, while we use MPI_Aint. They should be identical,
     * but lets assert to just make sure.
     */
    MPIR_Assert(sizeof(MPI_Aint) == sizeof(intptr_t));

    yaksa_info_create(&MPII_yaksa_info_nogpu);
    yaksa_info_keyval_append(MPII_yaksa_info_nogpu, "yaksa_gpu_driver", "nogpu", 6);

    if (MPIR_CVAR_ENABLE_GPU) {
        yaksa_info_t info = NULL;
        if (MPIR_CVAR_GPU_HAS_WAIT_KERNEL) {
            yaksa_info_create(&info);
            yaksa_info_keyval_append(info, "yaksa_has_wait_kernel", "1", 2);
        }

        yaksa_init(info);
    } else {
        /* prevent yaksa to query gpu devices, which can be very expensive */
        yaksa_init(MPII_yaksa_info_nogpu);
    }

    TYPEREP_YAKSA_TYPE__FIXED1 = YAKSA_TYPE__UINT8_T;
    TYPEREP_YAKSA_TYPE__FIXED2 = YAKSA_TYPE__UINT16_T;
    TYPEREP_YAKSA_TYPE__FIXED4 = YAKSA_TYPE__UINT32_T;
    TYPEREP_YAKSA_TYPE__FIXED8 = YAKSA_TYPE__UINT64_T;

    uintptr_t tmp_size;
    yaksa_type_get_size(YAKSA_TYPE__LONG_DOUBLE, &tmp_size);
    if (tmp_size == 16) {
        TYPEREP_YAKSA_TYPE__FIXED16 = YAKSA_TYPE__LONG_DOUBLE;
        TYPEREP_YAKSA_TYPE__FIXED32 = YAKSA_TYPE__C_LONG_DOUBLE_COMPLEX;
    }

    MPIR_FUNC_EXIT;
}

void MPIR_Typerep_finalize(void)
{
    MPIR_FUNC_ENTER;

    yaksa_info_free(MPII_yaksa_info_nogpu);

    yaksa_finalize();

    MPIR_FUNC_EXIT;
}
