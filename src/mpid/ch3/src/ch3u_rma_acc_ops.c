/* -*- Mode: C; c-basic-offset:4 ; -*- */
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
    int              mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t   data_sz;
    int              rank, origin_predefined, result_predefined, target_predefined;
    int              dt_contig ATTRIBUTE((unused));
    MPI_Aint         dt_true_lb ATTRIBUTE((unused));
    MPID_Datatype   *dtp;
    MPIU_CHKLMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_GET_ACCUMULATE);
    
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_MPIDI_GET_ACCUMULATE);

    MPIDI_Datatype_get_info(origin_count, origin_datatype, dt_contig, data_sz,
                            dtp, dt_true_lb);  
    
    if ((data_sz == 0) || (target_rank == MPI_PROC_NULL)) {
        goto fn_exit;
    }

    rank = win_ptr->myrank;
    
    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(origin_datatype, origin_predefined);
    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(result_datatype, result_predefined);
    MPIDI_CH3I_DATATYPE_IS_PREDEFINED(target_datatype, target_predefined);

    /* Do =! rank first (most likely branch?) */
    if (target_rank == rank) {
        MPI_User_function *uop;
       
        /* Perform the local get first, then the accumulate */
        mpi_errno = MPIR_Localcopy((char *) win_ptr->base + win_ptr->disp_unit *
                                   target_disp, target_count, target_datatype,
                                   result_addr, result_count, result_datatype); 
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

        if (op == MPI_REPLACE) {
            mpi_errno = MPIR_Localcopy(origin_addr, origin_count, origin_datatype,
                                (char *) win_ptr->base + win_ptr->disp_unit *
                                target_disp, target_count, target_datatype); 

            if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
            goto fn_exit;
        }
        
        MPIU_ERR_CHKANDJUMP1((HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN), 
                             mpi_errno, MPI_ERR_OP, "**opnotpredefined",
                             "**opnotpredefined %d", op );
        
        /* get the function by indexing into the op table */
        uop = MPIR_Op_table[((op)&0xf) - 1];
        
        if (origin_predefined && target_predefined) {    
            /* Cast away const'ness for origin_address in order to
             * avoid changing the prototype for MPI_User_function */
            (*uop)((void *) origin_addr, (char *) win_ptr->base + win_ptr->disp_unit *
                   target_disp, &target_count, &target_datatype);
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

                (*uop)(tmp_buf, (char *) win_ptr->base + win_ptr->disp_unit *
                   target_disp, &target_count, &target_datatype);
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
                target_buf = (char *) win_ptr->base + 
                    win_ptr->disp_unit * target_disp;
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
    }
    else {
        /* TODO: Inter-process get_accumulate isn't implemented yet */
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**notimpl");
    }

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_MPIDI_GET_ACCUMULATE);
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
