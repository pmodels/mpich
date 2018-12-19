#include "hcoll/api/hcoll_dte.h"
#include "mpiimpl.h"
#include "hcoll_dtypes.h"

extern int hcoll_initialized;
extern int hcoll_enable;

static dte_data_representation_t mpi_predefined_derived_2_hcoll(MPI_Datatype datatype)
{
    MPI_Aint size;

    switch (datatype) {
        case MPI_FLOAT_INT:
            return DTE_FLOAT_INT;
        case MPI_DOUBLE_INT:
            return DTE_DOUBLE_INT;
        case MPI_LONG_INT:
            return DTE_LONG_INT;
        case MPI_SHORT_INT:
            return DTE_SHORT_INT;
        case MPI_LONG_DOUBLE_INT:
            return DTE_LONG_DOUBLE_INT;
        case MPI_2INT:
            return DTE_2INT;
#ifdef HAVE_FORTRAN_BINDING
#if HCOLL_API >= HCOLL_VERSION(3,7)
        case MPI_2INTEGER:
            MPIR_Datatype_get_size_macro(datatype, size);
            switch (size) {
                case 4:
                    return DTE_2INT;
                case 8:
                    return DTE_2INT64;
                default:
                    return DTE_ZERO;
            }
        case MPI_2REAL:
            MPIR_Datatype_get_size_macro(datatype, size);
            switch (size) {
                case 4:
                    return DTE_2FLOAT32;
                case 8:
                    return DTE_2FLOAT64;
                default:
                    return DTE_ZERO;
            }
        case MPI_2DOUBLE_PRECISION:
            MPIR_Datatype_get_size_macro(datatype, size);
            switch (size) {
                case 4:
                    return DTE_2FLOAT32;
                case 8:
                    return DTE_2FLOAT64;
                default:
                    return DTE_ZERO;
            }
#endif
#endif
        default:
            break;
    }
    return DTE_ZERO;
}

dte_data_representation_t mpi_dtype_2_hcoll_dtype(MPI_Datatype datatype, int count, const int mode)
{
    dte_data_representation_t dte_data_rep = DTE_ZERO;

    if (HANDLE_GET_KIND((datatype)) == HANDLE_KIND_BUILTIN) {
        /* Built-in type */
        dte_data_rep = mpi_dtype_2_dte_dtype(datatype);
    }
#if HCOLL_API >= HCOLL_VERSION(3,6)
    else if (TRY_FIND_DERIVED == mode) {

        /* Check for predefined derived types */
        dte_data_rep = mpi_predefined_derived_2_hcoll(datatype);
        if (HCOL_DTE_IS_ZERO(dte_data_rep)) {
            MPIR_Datatype *dt_ptr;

            /* Must be a non-predefined derived mapping, get it */
            MPIR_Datatype_get_ptr(datatype, dt_ptr);
            dte_data_rep = (dte_data_representation_t) dt_ptr->dev.hcoll_datatype;
        }
    }
#endif

    /* We always fall back, don't even think about forcing it! */
    /* XXX Fix me
     * if (HCOL_DTE_IS_ZERO(dte_data_rep) && TRY_FIND_DERIVED == mode
     * && !mca_coll_hcoll_component.hcoll_datatype_fallback) {
     * dte_data_rep = DTE_ZERO;
     * dte_data_rep.rep.in_line_rep.data_handle.in_line.in_line = 0;
     * dte_data_rep.rep.in_line_rep.data_handle.pointer_to_handle = (uint64_t) &datatype;
     * }
     */
    return dte_data_rep;
}

/* This will only get called once */
int hcoll_type_commit_hook(MPIR_Datatype * dtype_p)
{
    int mpi_errno, ret;

    if (0 == hcoll_initialized) {
        mpi_errno = hcoll_initialize();
        if (mpi_errno)
            return MPI_ERR_OTHER;
    }

    if (0 == hcoll_enable) {
        return MPI_SUCCESS;
    }

    dtype_p->dev.hcoll_datatype = mpi_predefined_derived_2_hcoll(dtype_p->handle);
    if (!HCOL_DTE_IS_ZERO(dtype_p->dev.hcoll_datatype)) {
        return MPI_SUCCESS;
    }

    dtype_p->dev.hcoll_datatype = DTE_ZERO;

    ret = hcoll_create_mpi_type((void *) (intptr_t) dtype_p->handle, &dtype_p->dev.hcoll_datatype);
    if (HCOLL_SUCCESS != ret) {
        return MPI_ERR_OTHER;
    }

    if (HCOL_DTE_IS_ZERO(dtype_p->dev.hcoll_datatype))
        MPIR_Datatype_add_ref_if_not_builtin(dtype_p->handle);

    return MPI_SUCCESS;
}

int hcoll_type_free_hook(MPIR_Datatype * dtype_p)
{
    if (0 == hcoll_enable) {
        return MPI_SUCCESS;
    }

    if (HCOL_DTE_IS_ZERO(dtype_p->dev.hcoll_datatype))
        MPIR_Datatype_release_if_not_builtin(dtype_p->handle);

    int rc = hcoll_dt_destroy(dtype_p->dev.hcoll_datatype);
    if (HCOLL_SUCCESS != rc) {
        return MPI_ERR_OTHER;
    }

    dtype_p->dev.hcoll_datatype = DTE_ZERO;

    return MPI_SUCCESS;
}
