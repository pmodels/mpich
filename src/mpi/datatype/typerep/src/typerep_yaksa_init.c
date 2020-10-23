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

yaksa_type_t MPII_Typerep_get_yaksa_type(MPI_Datatype type)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPII_TYPEREP_GET_YAKSA_TYPE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPII_TYPEREP_GET_YAKSA_TYPE);

    yaksa_type_t yaksa_type;

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
            yaksa_type = YAKSA_TYPE__CHAR;
            break;

        case MPI_UNSIGNED_CHAR:
            yaksa_type = YAKSA_TYPE__UNSIGNED_CHAR;
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
#endif /* HAVE_FORTRAN_BINDING */
#ifdef HAVE_CXX_BINDING
        case MPI_CXX_BOOL:
#endif /* HAVE_CXX_BINDING */
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
        case MPI_CHARACTER:
            yaksa_type = YAKSA_TYPE__CHAR;
            break;

        case MPI_INTEGER:
            yaksa_type = YAKSA_TYPE__INT;
            break;

        case MPI_INTEGER1:
            yaksa_type = YAKSA_TYPE__INT8_T;
            break;

        case MPI_INTEGER2:
            yaksa_type = YAKSA_TYPE__INT16_T;
            break;

        case MPI_INTEGER4:
            yaksa_type = YAKSA_TYPE__INT32_T;
            break;

        case MPI_INTEGER8:
            yaksa_type = YAKSA_TYPE__INT64_T;
            break;

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
            if (sizeof(float) == 8)
                yaksa_type = YAKSA_TYPE__C_COMPLEX;
            else if (sizeof(double) == 8)
                yaksa_type = YAKSA_TYPE__C_DOUBLE_COMPLEX;
            else if (sizeof(long double) == 8)
                yaksa_type = YAKSA_TYPE__C_LONG_DOUBLE_COMPLEX;
            else
                assert(0);
            break;

        case MPI_COMPLEX16:
            if (sizeof(float) == 16)
                yaksa_type = YAKSA_TYPE__C_COMPLEX;
            else if (sizeof(double) == 16)
                yaksa_type = YAKSA_TYPE__C_DOUBLE_COMPLEX;
            else if (sizeof(long double) == 16)
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
                    yaksa_type = ((yaksa_type_t) (intptr_t) typeptr->typerep.handle);
                }
            }
            break;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPII_TYPEREP_GET_YAKSA_TYPE);
    return yaksa_type;
}

void MPIR_Typerep_init(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_INIT);

    MPI_Aint size;

    yaksa_init(NULL);

    MPIR_Datatype_get_size_macro(MPI_REAL16, size);
    yaksa_type_create_contig(size, YAKSA_TYPE__BYTE, NULL, &TYPEREP_YAKSA_TYPE__REAL16);

    MPIR_Datatype_get_size_macro(MPI_COMPLEX32, size);
    yaksa_type_create_contig(size, YAKSA_TYPE__BYTE, NULL, &TYPEREP_YAKSA_TYPE__COMPLEX32);

    MPIR_Datatype_get_size_macro(MPI_INTEGER16, size);
    yaksa_type_create_contig(size, YAKSA_TYPE__BYTE, NULL, &TYPEREP_YAKSA_TYPE__INTEGER16);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_INIT);
}

void MPIR_Typerep_finalize(void)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TYPEREP_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TYPEREP_FINALIZE);

    yaksa_type_free(TYPEREP_YAKSA_TYPE__REAL16);
    yaksa_type_free(TYPEREP_YAKSA_TYPE__COMPLEX32);
    yaksa_type_free(TYPEREP_YAKSA_TYPE__INTEGER16);

    yaksa_finalize();

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TYPEREP_FINALIZE);
}
