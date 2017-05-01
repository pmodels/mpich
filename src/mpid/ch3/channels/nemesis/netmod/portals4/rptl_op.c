/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "ptl_impl.h"
#include "rptl.h"

#undef FUNCNAME
#define FUNCNAME rptli_op_alloc
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int rptli_op_alloc(struct rptl_op **op, struct rptl_target *target)
{
    int ret = PTL_OK;
    struct rptl_op_pool_segment *op_segment;
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIR_CHKPMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_RPTLI_OP_ALLOC);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_RPTLI_OP_ALLOC);

    assert(target);

    if (target->op_pool == NULL) {
        MPIR_CHKPMEM_MALLOC(op_segment, struct rptl_op_pool_segment *, sizeof(struct rptl_op_pool_segment),
                            mpi_errno, "op pool segment", MPL_MEM_OTHER);
        DL_APPEND(target->op_segment_list, op_segment);

        for (i = 0; i < RPTL_OP_POOL_SEGMENT_COUNT; i++)
            DL_APPEND(target->op_pool, &op_segment->op[i]);
    }

    *op = target->op_pool;
    DL_DELETE(target->op_pool, *op);

  fn_exit:
    MPIR_CHKPMEM_COMMIT();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_RPTLI_OP_ALLOC);
    return ret;

  fn_fail:
    if (mpi_errno)
        ret = PTL_FAIL;
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME rptli_op_free
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
void rptli_op_free(struct rptl_op *op)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_RPTLI_OP_FREE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_RPTLI_OP_FREE);

    DL_APPEND(op->target->op_pool, op);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_RPTLI_OP_FREE);
}
