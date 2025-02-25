/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_op_util.h"

/* Note a subtlety in these two macros which avoids compiler warnings.
   The compiler complains about using == on floats, but the standard
   requires that we set loc to min of the locs if the two values are
   equal.  So we do "if a>b {} else if a>=b Y" which is the same as
   "if a>b X else if a==b Y" but avoids the warning. */

void MPIR_MINLOC(void *invec, void *inoutvec, MPI_Aint * Len, MPI_Datatype * type)
{
    MPIR_Assert(MPIR_Datatype_is_pairtype(*type));

    if (HANDLE_IS_BUILTIN(*type) && (*type & MPIR_TYPE_PAIR_MASK)) {
        MPI_Aint len2 = (*Len) * 2;
        /* bits mask magic: same MPIR_TYPE without PAIR_MASK, and cut the type size by half */
        MPI_Datatype value_type = 0x4c000000 | (*type & 0x8f0000) | ((*type & 0xff00) >> 1);
        switch (value_type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) \
            case (mpi_type_) : { \
                c_type_ * restrict a = inoutvec; \
                const c_type_ * restrict b = invec; \
                for (MPI_Aint i=0; i<len2; i+=2) { \
                    if (a[i] > b[i]) { \
                        a[i]   = b[i]; \
                        a[i+1] = b[i+1]; \
                    } else if (a[i] >= b[i]) { \
                        a[i+1] = MPL_MIN(a[i+1],b[i+1]); \
                    } \
                } \
                break; \
            }

                /* no semicolons by necessity */
                MPIR_OP_TYPE_GROUP(INTEGER)
                    MPIR_OP_TYPE_GROUP(FLOATING_POINT)
            default:
                MPIR_Assert(0);
                break;
        }
    } else {
        MPI_Datatype value_type = MPIR_Pairtype_get_value_type(*type);
        switch (value_type) {
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_, c_type_) \
            case (mpi_type_) : { \
                struct _pair { \
                    c_type_ value; \
                    int loc; \
                }; \
                struct _pair * restrict a = inoutvec; \
                const struct _pair * restrict b = invec; \
                for (MPI_Aint i=0; i<*Len; i++) { \
                    if (a[i].value > b[i].value) { \
                        a[i].value = b[i].value; \
                        a[i].loc   = b[i].loc; \
                    } else if (a[i].value >= b[i].value) { \
                        a[i].loc = MPL_MIN(a[i].loc,b[i].loc); \
                    } \
                } \
                break; \
            }

                /* no semicolons by necessity */
                MPIR_OP_TYPE_GROUP(INTEGER)
                    MPIR_OP_TYPE_GROUP(FLOATING_POINT)
            default:
                MPIR_Assert(0);
                break;
        }
    }
}

int MPIR_MINLOC_check_dtype(MPI_Datatype type)
{
    int mpi_errno = MPI_SUCCESS;

    if (!MPIR_Datatype_is_pairtype(type)) {
        MPIR_ERR_SET1(mpi_errno, MPI_ERR_OP, "**opundefined", "**opundefined %s", "MPI_MINLOC");
    }

    return mpi_errno;
}
