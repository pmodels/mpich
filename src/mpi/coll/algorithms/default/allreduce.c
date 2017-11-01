/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpiimpl.h"
#include "collutil.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 2048
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        the short message algorithm will be used if the send buffer size is <=
        this value (in bytes)

    - name        : MPIR_CVAR_ENABLE_SMP_COLLECTIVES
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Enable SMP aware collective communication.

    - name        : MPIR_CVAR_ENABLE_SMP_ALLREDUCE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Enable SMP aware allreduce.

    - name        : MPIR_CVAR_MAX_SMP_ALLREDUCE_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum message size for which SMP-aware allreduce is used.  A
        value of '0' uses SMP-aware allreduce for all message sizes.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#undef FUNCNAME
#define FUNCNAME MPIC_DEFAULT_Allreduce_recursive_doubling
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_DEFAULT_Allreduce_recursive_doubling(
    const void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    MPIR_Comm * comm_ptr,
    MPIR_Errflag_t * errflag)
{
    MPIR_CHKLMEM_DECL(1);
#ifdef MPID_HAS_HETERO
    int is_homogeneous;
    int rc;
#endif
    int comm_size, rank;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int mask, dst, is_commutative, pof2, newrank, rem, newdst;
    MPI_Aint true_extent, true_lb, extent;
    void *tmp_buf;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    is_commutative = MPIR_Op_is_commutative(op);

    /* need to allocate temporary buffer to store incoming data*/
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);

    MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));
    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count*(MPL_MAX(extent,true_extent)), mpi_errno, "temporary buffer");

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);

    /* copy local data into recvbuf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf,
                                   count, datatype);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    /* find nearest power-of-two less than or equal to comm_size */
    pof2 = 1;
    while (pof2 <= comm_size) pof2 <<= 1;
    pof2 >>=1;

    rem = comm_size - pof2;

    /* In the non-power-of-two case, all even-numbered
       processes of rank < 2*rem send their data to
       (rank+1). These even-numbered processes no longer
       participate in the algorithm until the very end. The
       remaining processes form a nice power-of-two. */

    if (rank < 2*rem) {
        if (rank % 2 == 0) { /* even */
            mpi_errno = MPIC_Send(recvbuf, count,
                                     datatype, rank+1,
                                     MPIR_ALLREDUCE_TAG, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            /* temporarily set the rank to -1 so that this
               process does not pariticipate in recursive
               doubling */
            newrank = -1;
        }
        else { /* odd */
            mpi_errno = MPIC_Recv(tmp_buf, count,
                                     datatype, rank-1,
                                     MPIR_ALLREDUCE_TAG, comm_ptr,
                                     MPI_STATUS_IGNORE, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            /* do the reduction on received data. since the
               ordering is right, it doesn't matter whether
               the operation is commutative or not. */
            mpi_errno = MPIR_Reduce_local_impl(tmp_buf, recvbuf, count, datatype, op);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            /* change the rank */
            newrank = rank / 2;
        }
    }
    else  /* rank >= 2*rem */
        newrank = rank - rem;

    /* If op is user-defined or count is less than pof2, use
       recursive doubling algorithm. Otherwise do a reduce-scatter
       followed by allgather. (If op is user-defined,
       derived datatypes are allowed and the user could pass basic
       datatypes on one process and derived on another as long as
       the type maps are the same. Breaking up derived
       datatypes to do the reduce-scatter is tricky, therefore
       using recursive doubling in that case.) */

    if (newrank != -1) {
      mask = 0x1;
      while (mask < pof2) {
          newdst = newrank ^ mask;
          /* find real rank of dest */
          dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;

          /* Send the most current data, which is in recvbuf. Recv
             into tmp_buf */
          mpi_errno = MPIC_Sendrecv(recvbuf, count, datatype,
                                       dst, MPIR_ALLREDUCE_TAG, tmp_buf,
                                       count, datatype, dst,
                                       MPIR_ALLREDUCE_TAG, comm_ptr,
                                       MPI_STATUS_IGNORE, errflag);
          if (mpi_errno) {
              /* for communication errors, just record the error but continue */
              *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
              MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
              MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
          }

          /* tmp_buf contains data received in this step.
             recvbuf contains data accumulated so far */

          if (is_commutative  || (dst < rank)) {
              /* op is commutative OR the order is already right */
              mpi_errno = MPIR_Reduce_local_impl(tmp_buf, recvbuf, count, datatype, op);
              if (mpi_errno) MPIR_ERR_POP(mpi_errno);
          }
          else {
              /* op is noncommutative and the order is not right */
              mpi_errno = MPIR_Reduce_local_impl(recvbuf, tmp_buf, count, datatype, op);
              if (mpi_errno) MPIR_ERR_POP(mpi_errno);

              /* copy result back into recvbuf */
              mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
                                         recvbuf, count, datatype);
              if (mpi_errno) MPIR_ERR_POP(mpi_errno);
          }
          mask <<= 1;
      }
    }
    /* In the non-power-of-two case, all odd-numbered
       processes of rank < 2*rem send the result to
       (rank-1), the ranks who didn't participate above. */
    if (rank < 2*rem) {
        if (rank % 2)  /* odd */
            mpi_errno = MPIC_Send(recvbuf, count,
                                     datatype, rank-1,
                                     MPIR_ALLREDUCE_TAG, comm_ptr, errflag);
        else  /* even */
            mpi_errno = MPIC_Recv(recvbuf, count,
                                     datatype, rank+1,
                                     MPIR_ALLREDUCE_TAG, comm_ptr,
                                     MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIC_DEFAULT_Allreduce_reduce_scatter_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_DEFAULT_Allreduce_reduce_scatter_allgather(
    const void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    MPIR_Comm * comm_ptr,
    MPIR_Errflag_t * errflag)
{
    MPIR_CHKLMEM_DECL(3);
#ifdef MPID_HAS_HETERO
    int is_homogeneous;
    int rc;
#endif
    int comm_size, rank;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int mask, dst, pof2, newrank, rem, newdst, i,
        send_idx, recv_idx, last_idx, send_cnt, recv_cnt, *cnts, *disps;
    MPI_Aint true_extent, true_lb, extent;
    void *tmp_buf;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* need to allocate temporary buffer to store incoming data*/
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);

    MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));
    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count*(MPL_MAX(extent,true_extent)), mpi_errno, "temporary buffer");

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);

    /* copy local data into recvbuf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf,
                                   count, datatype);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    /* find nearest power-of-two less than or equal to comm_size */
    pof2 = 1;
    while (pof2 <= comm_size) pof2 <<= 1;
    pof2 >>=1;

    rem = comm_size - pof2;

    /* In the non-power-of-two case, all even-numbered
       processes of rank < 2*rem send their data to
       (rank+1). These even-numbered processes no longer
       participate in the algorithm until the very end. The
       remaining processes form a nice power-of-two. */

    if (rank < 2*rem) {
        if (rank % 2 == 0) { /* even */
            mpi_errno = MPIC_Send(recvbuf, count,
                                     datatype, rank+1,
                                     MPIR_ALLREDUCE_TAG, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            /* temporarily set the rank to -1 so that this
               process does not pariticipate in recursive
               doubling */
            newrank = -1;
        }
        else { /* odd */
            mpi_errno = MPIC_Recv(tmp_buf, count,
                                     datatype, rank-1,
                                     MPIR_ALLREDUCE_TAG, comm_ptr,
                                     MPI_STATUS_IGNORE, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            /* do the reduction on received data. since the
               ordering is right, it doesn't matter whether
               the operation is commutative or not. */
            mpi_errno = MPIR_Reduce_local_impl(tmp_buf, recvbuf, count, datatype, op);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);

            /* change the rank */
            newrank = rank / 2;
        }
    }
    else  /* rank >= 2*rem */
        newrank = rank - rem;

    /* If op is user-defined or count is less than pof2, use
       recursive doubling algorithm. Otherwise do a reduce-scatter
       followed by allgather. (If op is user-defined,
       derived datatypes are allowed and the user could pass basic
       datatypes on one process and derived on another as long as
       the type maps are the same. Breaking up derived
       datatypes to do the reduce-scatter is tricky, therefore
       using recursive doubling in that case.) */

    if (newrank != -1) {
      MPIR_CHKLMEM_MALLOC(cnts, int *, pof2*sizeof(int), mpi_errno, "counts");
      MPIR_CHKLMEM_MALLOC(disps, int *, pof2*sizeof(int), mpi_errno, "displacements");

      for (i=0; i<pof2; i++)
          cnts[i] = count/pof2;
      if ((count % pof2) > 0) {
          for (i=0; i<(count % pof2); i++)
              cnts[i] += 1;
      }

      disps[0] = 0;
      for (i=1; i<pof2; i++)
          disps[i] = disps[i-1] + cnts[i-1];

      mask = 0x1;
      send_idx = recv_idx = 0;
      last_idx = pof2;
      while (mask < pof2) {
          newdst = newrank ^ mask;
          /* find real rank of dest */
          dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;

          send_cnt = recv_cnt = 0;
          if (newrank < newdst) {
              send_idx = recv_idx + pof2/(mask*2);
              for (i=send_idx; i<last_idx; i++)
                  send_cnt += cnts[i];
              for (i=recv_idx; i<send_idx; i++)
                  recv_cnt += cnts[i];
          }
          else {
              recv_idx = send_idx + pof2/(mask*2);
              for (i=send_idx; i<recv_idx; i++)
                  send_cnt += cnts[i];
              for (i=recv_idx; i<last_idx; i++)
                  recv_cnt += cnts[i];
          }

          /* Send data from recvbuf. Recv into tmp_buf */
          mpi_errno = MPIC_Sendrecv((char *) recvbuf +
                                       disps[send_idx]*extent,
                                       send_cnt, datatype,
                                       dst, MPIR_ALLREDUCE_TAG,
                                       (char *) tmp_buf +
                                       disps[recv_idx]*extent,
                                       recv_cnt, datatype, dst,
                                       MPIR_ALLREDUCE_TAG, comm_ptr,
                                       MPI_STATUS_IGNORE, errflag);
          if (mpi_errno) {
              /* for communication errors, just record the error but continue */
              *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
              MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
              MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
          }

          /* tmp_buf contains data received in this step.
             recvbuf contains data accumulated so far */

          /* This algorithm is used only for predefined ops
             and predefined ops are always commutative. */
          mpi_errno = MPIR_Reduce_local_impl(((char *) tmp_buf + disps[recv_idx]*extent),
                                             ((char *) recvbuf + disps[recv_idx]*extent),
                                             recv_cnt, datatype, op);
          if (mpi_errno) MPIR_ERR_POP(mpi_errno);

          /* update send_idx for next iteration */
          send_idx = recv_idx;
          mask <<= 1;

          /* update last_idx, but not in last iteration
             because the value is needed in the allgather
             step below. */
          if (mask < pof2)
              last_idx = recv_idx + pof2/mask;
      }

      /* now do the allgather */

      mask >>= 1;
      while (mask > 0) {
          newdst = newrank ^ mask;
          /* find real rank of dest */
          dst = (newdst < rem) ? newdst*2 + 1 : newdst + rem;

          send_cnt = recv_cnt = 0;
          if (newrank < newdst) {
              /* update last_idx except on first iteration */
              if (mask != pof2/2)
                  last_idx = last_idx + pof2/(mask*2);

              recv_idx = send_idx + pof2/(mask*2);
              for (i=send_idx; i<recv_idx; i++)
                  send_cnt += cnts[i];
              for (i=recv_idx; i<last_idx; i++)
                  recv_cnt += cnts[i];
          }
          else {
              recv_idx = send_idx - pof2/(mask*2);
              for (i=send_idx; i<last_idx; i++)
                  send_cnt += cnts[i];
              for (i=recv_idx; i<send_idx; i++)
                  recv_cnt += cnts[i];
          }

          mpi_errno = MPIC_Sendrecv((char *) recvbuf +
                                       disps[send_idx]*extent,
                                       send_cnt, datatype,
                                       dst, MPIR_ALLREDUCE_TAG,
                                       (char *) recvbuf +
                                       disps[recv_idx]*extent,
                                       recv_cnt, datatype, dst,
                                       MPIR_ALLREDUCE_TAG, comm_ptr,
                                       MPI_STATUS_IGNORE, errflag);
          if (mpi_errno) {
              /* for communication errors, just record the error but continue */
              *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
              MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
              MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
          }

          if (newrank > newdst) send_idx = recv_idx;

          mask >>= 1;
      }
    }
    /* In the non-power-of-two case, all odd-numbered
       processes of rank < 2*rem send the result to
       (rank-1), the ranks who didn't participate above. */
    if (rank < 2*rem) {
        if (rank % 2)  /* odd */
            mpi_errno = MPIC_Send(recvbuf, count,
                                     datatype, rank-1,
                                     MPIR_ALLREDUCE_TAG, comm_ptr, errflag);
        else  /* even */
            mpi_errno = MPIC_Recv(recvbuf, count,
                                     datatype, rank+1,
                                     MPIR_ALLREDUCE_TAG, comm_ptr,
                                     MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* not declared static because a machine-specific function may call this one
   in some cases */
#undef FUNCNAME
#define FUNCNAME MPIC_DEFAULT_Allreduce_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_DEFAULT_Allreduce_intra (
    const void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
#ifdef MPID_HAS_HETERO
    int is_homogeneous;
    int rc;
#endif
    MPI_Aint type_size;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int nbytes = 0;
    int is_commutative, pof2;

    if (count == 0) goto fn_exit;

    is_commutative = MPIR_Op_is_commutative(op);

    if (MPIR_CVAR_ENABLE_SMP_COLLECTIVES && MPIR_CVAR_ENABLE_SMP_ALLREDUCE) {
        /* is the op commutative? We do SMP optimizations only if it is. */
        MPIR_Datatype_get_size_macro(datatype, type_size);
        nbytes = MPIR_CVAR_MAX_SMP_ALLREDUCE_MSG_SIZE ? type_size * count : 0;
        if (MPIR_Comm_is_node_aware(comm_ptr) && is_commutative &&
            nbytes <= MPIR_CVAR_MAX_SMP_ALLREDUCE_MSG_SIZE) {
            /* on each node, do a reduce to the local root */
            if (comm_ptr->node_comm != NULL) {
                /* take care of the MPI_IN_PLACE case. For reduce,
                 * MPI_IN_PLACE is specified only on the root;
                 * for allreduce it is specified on all processes. */

                if ((sendbuf == MPI_IN_PLACE) && (comm_ptr->node_comm->rank != 0)) {
                    /* IN_PLACE and not root of reduce. Data supplied to this
                     * allreduce is in recvbuf. Pass that as the sendbuf to reduce. */

                    mpi_errno =
                        MPID_Reduce(recvbuf, NULL, count, datatype, op, 0, comm_ptr->node_comm,
                                    errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                } else {
                    mpi_errno =
                        MPID_Reduce(sendbuf, recvbuf, count, datatype, op, 0, comm_ptr->node_comm,
                                    errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
            } else {
                /* only one process on the node. copy sendbuf to recvbuf */
                if (sendbuf != MPI_IN_PLACE) {
                    mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
                    if (mpi_errno)
                        MPIR_ERR_POP(mpi_errno);
                }
            }

            /* now do an IN_PLACE allreduce among the local roots of all nodes */
            if (comm_ptr->node_roots_comm != NULL) {
                mpi_errno = MPIR_Allreduce(MPI_IN_PLACE, recvbuf, count, datatype, op,
                                           comm_ptr->node_roots_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }

            /* now broadcast the result among local processes */
            if (comm_ptr->node_comm != NULL) {
                mpi_errno = MPID_Bcast(recvbuf, count, datatype, 0, comm_ptr->node_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
            goto fn_exit;
        }
    }

#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
    else
        is_homogeneous = 1;
#endif

#ifdef MPID_HAS_HETERO
    if (!is_homogeneous) {
        /* heterogeneous. To get the same result on all processes, we
           do a reduce to 0 and then broadcast. */
        mpi_errno = MPID_Reduce( sendbuf, recvbuf, count, datatype,
                                       op, 0, comm_ptr, errflag );
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        mpi_errno = MPID_Bcast( recvbuf, count, datatype, 0, comm_ptr, errflag );
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
    else
#endif /* MPID_HAS_HETERO */
    {
        /* homogeneous */

        pof2 = MPIU_pof2(comm_ptr->local_size);
        if ((nbytes <= MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE) ||
            (HANDLE_GET_KIND(op) != HANDLE_KIND_BUILTIN) || (count < pof2)) {
            mpi_errno = MPIC_DEFAULT_Allreduce_recursive_doubling(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        } else {
            mpi_errno = MPIC_DEFAULT_Allreduce_reduce_scatter_allgather(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }

  fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    return (mpi_errno);

  fn_fail:
    goto fn_exit;
}

/* not declared static because a machine-specific function may call this one
   in some cases */
#undef FUNCNAME
#define FUNCNAME MPIC_DEFAULT_Allreduce_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_DEFAULT_Allreduce_inter(const void *sendbuf,
                                             void *recvbuf,
                                             int count,
                                             MPI_Datatype datatype,
                                             MPI_Op op,
                                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
/* Intercommunicator Allreduce.
   We first do intracommunicator reduces to rank 0 on left and right
   groups, then an exchange between left and right rank 0, and finally
   intracommunicator broadcasts from rank 0 on left and right group.
*/
    int mpi_errno;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint true_extent, true_lb, extent;
    void *tmp_buf = NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_CHKLMEM_DECL(1);

    if (comm_ptr->rank == 0) {
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);
        /* I think this is the worse case, so we can avoid an assert()
         * inside the for loop */
        /* Should MPIR_CHKLMEM_MALLOC do this? */
        MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));
        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count * (MPL_MAX(extent, true_extent)), mpi_errno,
                            "temporary buffer");
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - true_lb);
    }

    /* Get the local intracommunicator */
    if (!comm_ptr->local_comm)
        MPII_Setup_intercomm_localcomm(comm_ptr);

    newcomm_ptr = comm_ptr->local_comm;

    /* Do a local reduce on this intracommunicator */
    mpi_errno = MPIR_Reduce(sendbuf, tmp_buf, count, datatype, op, 0, newcomm_ptr, errflag);
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

    /* Do a exchange between local and remote rank 0 on this intercommunicator */
    if (comm_ptr->rank == 0) {
        mpi_errno = MPIC_Sendrecv(tmp_buf, count, datatype, 0, MPIR_REDUCE_TAG,
                                  recvbuf, count, datatype, 0, MPIR_REDUCE_TAG,
                                  comm_ptr, MPI_STATUS_IGNORE, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    /* Do a local broadcast on this intracommunicator */
    mpi_errno = MPID_Bcast(recvbuf, count, datatype, 0, newcomm_ptr, errflag);
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIC_DEFAULT_Allreduce
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_DEFAULT_Allreduce(const void *sendbuf, void *recvbuf, int count,
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                       MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        mpi_errno =
            MPIC_DEFAULT_Allreduce_intra(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    } else {
        /* intercommunicator */
        mpi_errno =
            MPIC_DEFAULT_Allreduce_inter(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FCNAME
#undef FUNCNAME
