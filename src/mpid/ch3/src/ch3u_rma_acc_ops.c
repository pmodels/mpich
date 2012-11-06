/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"
#include "mpidrma.h"

#ifdef USE_MPIU_INSTR
MPIU_INSTR_DURATION_EXTERN_DECL(rmaqueue_alloc);
MPIU_INSTR_DURATION_EXTERN_DECL(rmaqueue_set);
extern void MPIDI_CH3_RMA_InitInstr(void);
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_Get_accumulate
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Get_accumulate(const void *origin_addr, int origin_count,
                         MPI_Datatype origin_datatype, void *result_addr, int result_count,
                         MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp,
                         int target_count, MPI_Datatype target_datatype, MPI_Op op, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t data_sz;
    int rank, origin_predefined, result_predefined, target_predefined;
    int shm_locked = 0;
    int dt_contig ATTRIBUTE((unused));
    MPI_Aint dt_true_lb ATTRIBUTE((unused));
    MPID_Datatype *dtp;
    MPIU_CHKLMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_GET_ACCUMULATE);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_GET_ACCUMULATE);

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    if (win_ptr->epoch_state == MPIDI_EPOCH_NONE && win_ptr->fence_issued) {
        win_ptr->epoch_state = MPIDI_EPOCH_FENCE;
    }

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state == MPIDI_EPOCH_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    MPIDI_Datatype_get_info(target_count, target_datatype, dt_contig, data_sz,
                            dtp, dt_true_lb);

    if (data_sz == 0) {
        goto fn_exit;
    }

    rank = win_ptr->myrank;

    origin_predefined = TRUE; /* quiet uninitialized warnings (b/c goto) */
    if (op != MPI_NO_OP) {
        MPIDI_CH3I_DATATYPE_IS_PREDEFINED(origin_datatype, origin_predefined);
    }
    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(result_datatype, result_predefined);
    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(target_datatype, target_predefined);

    /* Do =! rank first (most likely branch?) */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED)
    {
        MPI_User_function *uop;
        void *base;
        int disp_unit;

        if (win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED) {
            base = win_ptr->shm_base_addrs[target_rank];
            disp_unit = win_ptr->disp_units[target_rank];
            MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
            shm_locked = 1;
        }
        else {
            base = win_ptr->base;
            disp_unit = win_ptr->disp_unit;
        }

        /* Perform the local get first, then the accumulate */
        mpi_errno = MPIR_Localcopy((char *) base + disp_unit * target_disp,
                                   target_count, target_datatype,
                                   result_addr, result_count, result_datatype);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        /* NO_OP: Don't perform the accumulate */
        if (op == MPI_NO_OP) {
            if (shm_locked) {
                MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
                shm_locked = 0;
            }

            goto fn_exit;
        }

        if (op == MPI_REPLACE) {
            mpi_errno = MPIR_Localcopy(origin_addr, origin_count, origin_datatype,
                                (char *) base + disp_unit * target_disp,
                                target_count, target_datatype);

            if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

            if (shm_locked) {
                MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
                shm_locked = 0;
            }

            goto fn_exit;
        }

        MPIU_ERR_CHKANDJUMP1((HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN),
                             mpi_errno, MPI_ERR_OP, "**opnotpredefined",
                             "**opnotpredefined %d", op );

        /* get the function by indexing into the op table */
        uop = MPIR_OP_HDL_TO_FN(op);

        if (origin_predefined && target_predefined) {
            /* Cast away const'ness for origin_address in order to
             * avoid changing the prototype for MPI_User_function */
            (*uop)((void *) origin_addr, (char *) base + disp_unit*target_disp,
                   &target_count, &target_datatype);
        }
        else {
            /* derived datatype */

            MPID_Segment *segp;
            DLOOP_VECTOR *dloop_vec;
            MPI_Aint first, last;
            int vec_len, i, type_size, count;
            MPI_Datatype type;
            MPI_Aint true_lb, true_extent, extent;
            void *tmp_buf=NULL, *target_buf;
            const void *source_buf;

            if (origin_datatype != target_datatype) {
                /* first copy the data into a temporary buffer with
                   the same datatype as the target. Then do the
                   accumulate operation. */

                MPIR_Type_get_true_extent_impl(target_datatype, &true_lb, &true_extent);
                MPID_Datatype_get_extent_macro(target_datatype, extent);

                MPIU_CHKLMEM_MALLOC(tmp_buf, void *,
                                    target_count * (MPIR_MAX(extent,true_extent)),
                                    mpi_errno, "temporary buffer");
                /* adjust for potential negative lower bound in datatype */
                tmp_buf = (void *)((char*)tmp_buf - true_lb);

                mpi_errno = MPIR_Localcopy(origin_addr, origin_count,
                                           origin_datatype, tmp_buf,
                                           target_count, target_datatype);
                if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
            }

            if (target_predefined) {
                /* target predefined type, origin derived datatype */

                (*uop)(tmp_buf, (char *) base + disp_unit * target_disp,
                       &target_count, &target_datatype);
            }
            else {

                segp = MPID_Segment_alloc();
                MPIU_ERR_CHKANDJUMP1((!segp), mpi_errno, MPI_ERR_OTHER,
                                     "**nomem","**nomem %s","MPID_Segment_alloc");
                MPID_Segment_init(NULL, target_count, target_datatype, segp, 0);
                first = 0;
                last  = SEGMENT_IGNORE_LAST;

                MPID_Datatype_get_ptr(target_datatype, dtp);
                vec_len = dtp->max_contig_blocks * target_count + 1;
                /* +1 needed because Rob says so */
                MPIU_CHKLMEM_MALLOC(dloop_vec, DLOOP_VECTOR *,
                                    vec_len * sizeof(DLOOP_VECTOR),
                                    mpi_errno, "dloop vector");

                MPID_Segment_pack_vector(segp, first, &last, dloop_vec, &vec_len);

                source_buf = (tmp_buf != NULL) ? tmp_buf : origin_addr;
                target_buf = (char *) base + disp_unit * target_disp;
                type = dtp->eltype;
                type_size = MPID_Datatype_get_basic_size(type);

                for (i=0; i<vec_len; i++) {
                    count = (dloop_vec[i].DLOOP_VECTOR_LEN)/type_size;
                    (*uop)((char *)source_buf + MPIU_PtrToAint(dloop_vec[i].DLOOP_VECTOR_BUF),
                           (char *)target_buf + MPIU_PtrToAint(dloop_vec[i].DLOOP_VECTOR_BUF),
                           &count, &type);
                }

                MPID_Segment_free(segp);
            }
        }

        if (shm_locked) {
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
            shm_locked = 0;
        }
    }
    else {
        MPIDI_RMA_Ops_list_t *ops_list = MPIDI_CH3I_RMA_Get_ops_list(win_ptr, target_rank);
        MPIDI_RMA_Op_t *new_ptr = NULL;

        /* Append the operation to the window's RMA ops queue */
        MPIU_INSTR_DURATION_START(rmaqueue_alloc);
        mpi_errno = MPIDI_CH3I_RMA_Ops_alloc_tail(ops_list, &new_ptr);
        MPIU_INSTR_DURATION_END(rmaqueue_alloc);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        /* TODO: Can we use the MPIDI_RMA_ACC_CONTIG optimization? */

        MPIU_INSTR_DURATION_START(rmaqueue_set);
        new_ptr->type = MPIDI_RMA_GET_ACCUMULATE;
        /* Cast away const'ness for origin_address as MPIDI_RMA_Op_t
         * contain both PUT and GET like ops */
        new_ptr->origin_addr = (void *) origin_addr;
        new_ptr->origin_count = origin_count;
        new_ptr->origin_datatype = origin_datatype;
        new_ptr->result_addr = result_addr;
        new_ptr->result_count = result_count;
        new_ptr->result_datatype = result_datatype;
        new_ptr->target_rank = target_rank;
        new_ptr->target_disp = target_disp;
        new_ptr->target_count = target_count;
        new_ptr->target_datatype = target_datatype;
        new_ptr->op = op;
        MPIU_INSTR_DURATION_END(rmaqueue_set);

        /* if source or target datatypes are derived, increment their
           reference counts */
        if (!origin_predefined) {
            MPID_Datatype_get_ptr(origin_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
        }
        if (!result_predefined) {
            MPID_Datatype_get_ptr(result_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
        }
        if (!target_predefined) {
            MPID_Datatype_get_ptr(target_datatype, dtp);
            MPID_Datatype_add_ref(dtp);
        }
    }

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_GET_ACCUMULATE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Compare_and_swap
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Compare_and_swap(const void *origin_addr, const void *compare_addr,
                          void *result_addr, MPI_Datatype datatype, int target_rank,
                          MPI_Aint target_disp, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int shm_locked = 0;
    int rank;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_COMPARE_AND_SWAP);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_COMPARE_AND_SWAP);

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    if (win_ptr->epoch_state == MPIDI_EPOCH_NONE && win_ptr->fence_issued) {
        win_ptr->epoch_state = MPIDI_EPOCH_FENCE;
    }

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state == MPIDI_EPOCH_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    rank = win_ptr->myrank;

    /* The datatype must be predefined, and one of: C integer, Fortran integer,
     * Logical, Multi-language types, or Byte.  This is checked above the ADI,
     * so there's no need to check it again here. */

    /* FIXME: For shared memory windows, we should provide an implementation
     * that uses a processor atomic operation. */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED)
    {
        void *base, *dest_addr;
        int disp_unit;
        int len;

        if (win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED) {
            base = win_ptr->shm_base_addrs[target_rank];
            disp_unit = win_ptr->disp_units[target_rank];

            MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
            shm_locked = 1;
        }
        else {
            base = win_ptr->base;
            disp_unit = win_ptr->disp_unit;
        }

        dest_addr = (char *) base + disp_unit * target_disp;

        MPID_Datatype_get_size_macro(datatype, len);
        MPIU_Memcpy(result_addr, dest_addr, len);

        if (MPIR_Compare_equal(compare_addr, dest_addr, datatype)) {
            MPIU_Memcpy(dest_addr, origin_addr, len);
        }

        if (shm_locked) {
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
            shm_locked = 0;
        }
    }
    else {
        MPIDI_RMA_Ops_list_t *ops_list = MPIDI_CH3I_RMA_Get_ops_list(win_ptr, target_rank);
        MPIDI_RMA_Op_t *new_ptr = NULL;

        /* Append this operation to the RMA ops queue */
        MPIU_INSTR_DURATION_START(rmaqueue_alloc);
        mpi_errno = MPIDI_CH3I_RMA_Ops_alloc_tail(ops_list, &new_ptr);
        MPIU_INSTR_DURATION_END(rmaqueue_alloc);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        MPIU_INSTR_DURATION_START(rmaqueue_set);
        new_ptr->type = MPIDI_RMA_COMPARE_AND_SWAP;
        new_ptr->origin_addr = (void *) origin_addr;
        new_ptr->origin_count = 1;
        new_ptr->origin_datatype = datatype;
        new_ptr->target_rank = target_rank;
        new_ptr->target_disp = target_disp;
        new_ptr->target_count = 1;
        new_ptr->target_datatype = datatype;
        new_ptr->result_addr = result_addr;
        new_ptr->result_count = 1;
        new_ptr->result_datatype = datatype;
        new_ptr->compare_addr = (void *) compare_addr;
        new_ptr->compare_count = 1;
        new_ptr->compare_datatype = datatype;
        MPIU_INSTR_DURATION_END(rmaqueue_set);
    }

fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_COMPARE_AND_SWAP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_Fetch_and_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_Fetch_and_op(const void *origin_addr, void *result_addr,
                       MPI_Datatype datatype, int target_rank,
                       MPI_Aint target_disp, MPI_Op op, MPID_Win *win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int shm_locked = 0;
    int rank;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_FETCH_AND_OP);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_FETCH_AND_OP);

    if (target_rank == MPI_PROC_NULL) {
        goto fn_exit;
    }

    if (win_ptr->epoch_state == MPIDI_EPOCH_NONE && win_ptr->fence_issued) {
        win_ptr->epoch_state = MPIDI_EPOCH_FENCE;
    }

    MPIU_ERR_CHKANDJUMP(win_ptr->epoch_state == MPIDI_EPOCH_NONE,
                        mpi_errno, MPI_ERR_RMA_SYNC, "**rmasync");

    rank = win_ptr->myrank;

    /* The datatype and op must be predefined.  This is checked above the ADI,
     * so there's no need to check it again here. */

    /* FIXME: For shared memory windows, we should provide an implementation
     * that uses a processor atomic operation. */
    if (target_rank == rank || win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED)
    {
        MPI_User_function *uop;
        void *base, *dest_addr;
        int disp_unit;
        int len, one;

        if (win_ptr->create_flavor == MPI_WIN_FLAVOR_SHARED) {
            base = win_ptr->shm_base_addrs[target_rank];
            disp_unit = win_ptr->disp_units[target_rank];

            MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
            shm_locked = 1;
        }
        else {
            base = win_ptr->base;
            disp_unit = win_ptr->disp_unit;
        }

        dest_addr = (char *) base + disp_unit * target_disp;

        MPID_Datatype_get_size_macro(datatype, len);
        MPIU_Memcpy(result_addr, dest_addr, len);

        uop = MPIR_OP_HDL_TO_FN(op);
        one = 1;

        (*uop)((void *) origin_addr, dest_addr, &one, &datatype);

        if (shm_locked) {
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
            shm_locked = 0;
        }
    }
    else {
        MPIDI_RMA_Ops_list_t *ops_list = MPIDI_CH3I_RMA_Get_ops_list(win_ptr, target_rank);
        MPIDI_RMA_Op_t *new_ptr = NULL;

        /* Append this operation to the RMA ops queue */
        MPIU_INSTR_DURATION_START(rmaqueue_alloc);
        mpi_errno = MPIDI_CH3I_RMA_Ops_alloc_tail(ops_list, &new_ptr);
        MPIU_INSTR_DURATION_END(rmaqueue_alloc);
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        MPIU_INSTR_DURATION_START(rmaqueue_set);
        new_ptr->type = MPIDI_RMA_FETCH_AND_OP;
        new_ptr->origin_addr = (void *) origin_addr;
        new_ptr->origin_count = 1;
        new_ptr->origin_datatype = datatype;
        new_ptr->target_rank = target_rank;
        new_ptr->target_disp = target_disp;
        new_ptr->target_count = 1;
        new_ptr->target_datatype = datatype;
        new_ptr->result_addr = result_addr;
        new_ptr->result_count = 1;
        new_ptr->result_datatype = datatype;
        new_ptr->op = op;
        MPIU_INSTR_DURATION_END(rmaqueue_set);
    }

fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_FETCH_AND_OP);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
fn_fail:
    if (shm_locked) {
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    }
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
