/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "yaksa.h"
#include "typerep_internal.h"

static yaksa_type_t TYPEREP_YAKSA_TYPE__REAL16;
static yaksa_type_t TYPEREP_YAKSA_TYPE__COMPLEX32;
static yaksa_type_t TYPEREP_YAKSA_TYPE__INTEGER16;

yaksa_info_t MPII_yaksa_info_nogpu;

yaksa_type_t MPII_Typerep_get_yaksa_type(MPI_Datatype type)
{
    MPIR_FUNC_ENTER;

    yaksa_type_t yaksa_type = YAKSA_TYPE__NULL;

    if (type == MPI_DATATYPE_NULL) {
        yaksa_type = YAKSA_TYPE__NULL;
        goto fn_exit;
    }

    MPI_Aint basic_type_size = 0;
    if (HANDLE_IS_BUILTIN(type)) {
        MPIR_Datatype_get_size_macro(type, basic_type_size);
    }

    switch (type) {
        case MPI_CHAR:
        case MPI_SIGNED_CHAR:
            yaksa_type = YAKSA_TYPE__INT8_T;
            break;

        case MPI_UNSIGNED_CHAR:
            yaksa_type = YAKSA_TYPE__UINT8_T;
            break;

        case MPI_PACKED:
        case MPI_BYTE:
            yaksa_type = YAKSA_TYPE__BYTE;
            break;

        case MPI_WCHAR:
            yaksa_type = YAKSA_TYPE__WCHAR_T;
            break;

        case MPI_SHORT:
            yaksa_type = YAKSA_TYPE__SHORT;
            break;

        case MPI_UNSIGNED_SHORT:
            yaksa_type = YAKSA_TYPE__UNSIGNED_SHORT;
            break;

        case MPI_INT:
            yaksa_type = YAKSA_TYPE__INT;
            break;

        case MPI_UNSIGNED:
            yaksa_type = YAKSA_TYPE__UNSIGNED;
            break;

        case MPI_LONG:
            yaksa_type = YAKSA_TYPE__LONG;
            break;

        case MPI_UNSIGNED_LONG:
            yaksa_type = YAKSA_TYPE__UNSIGNED_LONG;
            break;

        case MPI_LONG_LONG:
            yaksa_type = YAKSA_TYPE__LONG_LONG;
            break;

        case MPI_UNSIGNED_LONG_LONG:
            yaksa_type = YAKSA_TYPE__UNSIGNED_LONG_LONG;
            break;

        case MPI_INT8_T:
            yaksa_type = YAKSA_TYPE__INT8_T;
            break;

        case MPI_UINT8_T:
            yaksa_type = YAKSA_TYPE__UINT8_T;
            break;

        case MPI_INT16_T:
            yaksa_type = YAKSA_TYPE__INT16_T;
            break;

        case MPI_UINT16_T:
            yaksa_type = YAKSA_TYPE__UINT16_T;
            break;

        case MPI_INT32_T:
            yaksa_type = YAKSA_TYPE__INT32_T;
            break;

        case MPI_UINT32_T:
            yaksa_type = YAKSA_TYPE__UINT32_T;
            break;

        case MPI_INT64_T:
            yaksa_type = YAKSA_TYPE__INT64_T;
            break;

        case MPI_UINT64_T:
            yaksa_type = YAKSA_TYPE__UINT64_T;
            break;

        case MPI_FLOAT:
            yaksa_type = YAKSA_TYPE__FLOAT;
            break;

        case MPI_DOUBLE:
            yaksa_type = YAKSA_TYPE__DOUBLE;
            break;

        case MPI_LONG_DOUBLE:
            yaksa_type = YAKSA_TYPE__LONG_DOUBLE;
            break;

        case MPI_C_COMPLEX:
            yaksa_type = YAKSA_TYPE__C_COMPLEX;
            break;

        case MPI_C_DOUBLE_COMPLEX:
            yaksa_type = YAKSA_TYPE__C_DOUBLE_COMPLEX;
            break;

        case MPI_C_LONG_DOUBLE_COMPLEX:
            yaksa_type = YAKSA_TYPE__C_LONG_DOUBLE_COMPLEX;
            break;

        case MPI_2INT:
            yaksa_type = YAKSA_TYPE__2INT;
            break;

        case MPI_AINT:
        case MPI_OFFSET:
        case MPI_COUNT:
        case MPI_C_BOOL:
#ifdef HAVE_FORTRAN_BINDING
        case MPI_LOGICAL:
        case MPI_CHARACTER:
        case MPI_INTEGER:
        case MPI_INTEGER1:
        case MPI_INTEGER2:
        case MPI_INTEGER4:
        case MPI_INTEGER8:
#endif /* HAVE_FORTRAN_BINDING */
#ifdef HAVE_CXX_BOOL
        case MPI_CXX_BOOL:
#endif /* HAVE_CXX_BOOL */
            switch (basic_type_size) {
                case 1:
                    yaksa_type = YAKSA_TYPE__INT8_T;
                    break;

                case 2:
                    yaksa_type = YAKSA_TYPE__INT16_T;
                    break;

                case 4:
                    yaksa_type = YAKSA_TYPE__INT32_T;
                    break;

                case 8:
                    yaksa_type = YAKSA_TYPE__INT64_T;
                    break;

                default:
                    assert(0);
            }
            break;

#ifdef HAVE_FORTRAN_BINDING
            /* Unfortunately, some of these are more like hacks with unsafe assumptions */
        case MPI_COMPLEX:
            yaksa_type = YAKSA_TYPE__C_COMPLEX;
            break;

        case MPI_DOUBLE_COMPLEX:
            yaksa_type = YAKSA_TYPE__C_DOUBLE_COMPLEX;
            break;

        case MPI_2INTEGER:
            yaksa_type = YAKSA_TYPE__2INT;
            break;

        case MPI_2REAL:
            if (basic_type_size / 2 == sizeof(float))
                yaksa_type = YAKSA_TYPE__C_COMPLEX;
            else if (basic_type_size / 2 == sizeof(double))
                yaksa_type = YAKSA_TYPE__C_DOUBLE_COMPLEX;
            else if (basic_type_size / 2 == sizeof(long double))
                yaksa_type = YAKSA_TYPE__C_LONG_DOUBLE_COMPLEX;
            else
                assert(0);
            break;

        case MPI_2DOUBLE_PRECISION:
            yaksa_type = YAKSA_TYPE__C_DOUBLE_COMPLEX;
            break;

        case MPI_DOUBLE_PRECISION:
        case MPI_REAL:
        case MPI_REAL4:
        case MPI_REAL8:
            switch (basic_type_size) {
                case 1:
                    if (sizeof(float) == 1) {
                        yaksa_type = YAKSA_TYPE__FLOAT;
                    } else if (sizeof(double) == 1) {
                        yaksa_type = YAKSA_TYPE__DOUBLE;
                    } else if (sizeof(long double) == 1) {
                        yaksa_type = YAKSA_TYPE__LONG_DOUBLE;
                    } else {
                        yaksa_type = YAKSA_TYPE__NULL;
                    }
                    break;

                case 2:
                    if (sizeof(float) == 2) {
                        yaksa_type = YAKSA_TYPE__FLOAT;
                    } else if (sizeof(double) == 2) {
                        yaksa_type = YAKSA_TYPE__DOUBLE;
                    } else if (sizeof(long double) == 2) {
                        yaksa_type = YAKSA_TYPE__LONG_DOUBLE;
                    } else {
                        yaksa_type = YAKSA_TYPE__NULL;
                    }
                    break;

                case 4:
                    if (sizeof(float) == 4) {
                        yaksa_type = YAKSA_TYPE__FLOAT;
                    } else if (sizeof(double) == 4) {
                        yaksa_type = YAKSA_TYPE__DOUBLE;
                    } else if (sizeof(long double) == 4) {
                        yaksa_type = YAKSA_TYPE__LONG_DOUBLE;
                    } else {
                        yaksa_type = YAKSA_TYPE__NULL;
                    }
                    break;

                case 8:
                    if (sizeof(float) == 8) {
                        yaksa_type = YAKSA_TYPE__FLOAT;
                    } else if (sizeof(double) == 8) {
                        yaksa_type = YAKSA_TYPE__DOUBLE;
                    } else if (sizeof(long double) == 8) {
                        yaksa_type = YAKSA_TYPE__LONG_DOUBLE;
                    } else {
                        yaksa_type = YAKSA_TYPE__NULL;
                    }
                    break;

                case 16:
                    if (sizeof(float) == 16) {
                        yaksa_type = YAKSA_TYPE__FLOAT;
                    } else if (sizeof(double) == 16) {
                        yaksa_type = YAKSA_TYPE__DOUBLE;
                    } else if (sizeof(long double) == 16) {
                        yaksa_type = YAKSA_TYPE__LONG_DOUBLE;
                    } else {
                        yaksa_type = YAKSA_TYPE__NULL;
                    }
                    break;

                default:
                    assert(0);
            }
            break;

        case MPI_COMPLEX8:
            if (sizeof(float) == 4)
                yaksa_type = YAKSA_TYPE__C_COMPLEX;
            else if (sizeof(double) == 4)
                yaksa_type = YAKSA_TYPE__C_DOUBLE_COMPLEX;
            else if (sizeof(long double) == 4)
                yaksa_type = YAKSA_TYPE__C_LONG_DOUBLE_COMPLEX;
            else
                assert(0);
            break;

        case MPI_COMPLEX16:
            if (sizeof(float) == 8)
                yaksa_type = YAKSA_TYPE__C_COMPLEX;
            else if (sizeof(double) == 8)
                yaksa_type = YAKSA_TYPE__C_DOUBLE_COMPLEX;
            else if (sizeof(long double) == 8)
                yaksa_type = YAKSA_TYPE__C_LONG_DOUBLE_COMPLEX;
            else
                assert(0);
            break;
#endif /* HAVE_FORTRAN_BINDING */

#ifdef HAVE_CXX_BINDING
        case MPI_CXX_FLOAT_COMPLEX:
            yaksa_type = YAKSA_TYPE__C_COMPLEX;
            break;

        case MPI_CXX_DOUBLE_COMPLEX:
            yaksa_type = YAKSA_TYPE__C_DOUBLE_COMPLEX;
            break;

        case MPI_CXX_LONG_DOUBLE_COMPLEX:
            yaksa_type = YAKSA_TYPE__C_LONG_DOUBLE_COMPLEX;
            break;
#endif /* HAVE_CXX_BINDING */

        default:
            {
                /* some types are handled with if-else branches,
                 * instead of a switch statement because MPICH might
                 * define them as "MPI_DATATYPE_NULL". */
                if (type == MPI_REAL16) {
                    yaksa_type = TYPEREP_YAKSA_TYPE__REAL16;
                } else if (type == MPI_COMPLEX32) {
                    yaksa_type = TYPEREP_YAKSA_TYPE__COMPLEX32;
                } else if (type == MPI_INTEGER16) {
                    yaksa_type = TYPEREP_YAKSA_TYPE__INTEGER16;
                } else {
                    MPIR_Datatype *typeptr;
                    MPIR_Datatype_get_ptr(type, typeptr);
                    MPIR_Assert(typeptr != NULL);
                    yaksa_type = typeptr->typerep.handle;
                }
            }
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

    MPI_Aint size;

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

    MPIR_Datatype_get_size_macro(MPI_REAL16, size);
    yaksa_type_create_contig(size, YAKSA_TYPE__BYTE, NULL, &TYPEREP_YAKSA_TYPE__REAL16);

    MPIR_Datatype_get_size_macro(MPI_COMPLEX32, size);
    yaksa_type_create_contig(size, YAKSA_TYPE__BYTE, NULL, &TYPEREP_YAKSA_TYPE__COMPLEX32);

    MPIR_Datatype_get_size_macro(MPI_INTEGER16, size);
    yaksa_type_create_contig(size, YAKSA_TYPE__BYTE, NULL, &TYPEREP_YAKSA_TYPE__INTEGER16);

    MPIR_FUNC_EXIT;
}

void MPIR_Typerep_finalize(void)
{
    MPIR_FUNC_ENTER;

    yaksa_type_free(TYPEREP_YAKSA_TYPE__REAL16);
    yaksa_type_free(TYPEREP_YAKSA_TYPE__COMPLEX32);
    yaksa_type_free(TYPEREP_YAKSA_TYPE__INTEGER16);

    yaksa_info_free(MPII_yaksa_info_nogpu);

    yaksa_finalize();

    MPIR_FUNC_EXIT;
}
