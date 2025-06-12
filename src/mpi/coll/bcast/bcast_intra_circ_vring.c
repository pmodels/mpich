#include "mpiimpl.h"

int all_blocks(int r, int r_, int s, int e, int k, int** args);
void gen_rsched(int* sched, int r, int q, int p, int bb, int** args);
void gen_ssched(int* sched, int r, int q, int p, int bb, int** args);

int MPIR_Bcast_intra_circ_vring(void *buffer,
                                MPI_Aint count,
                                MPI_Datatype datatype,
                                int root, MPIR_Comm* comm,
                                const int n_chunk, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;

    int comm_size = comm->local_size;
    int rank = comm->rank;
    // int rel_rank = (rank - root + comm_size) % comm_size;

    if (comm_size < 2) {
        goto fn_exit;
    }
    
    // Depth of ~~doubling tree ("n skips" or "q")
    int depth = 1;
    while (0x1<<depth <= comm_size) {
        depth++;
    }

    /////////////////////////////////////////////////////////////////////
    // IMPORTANT: if `MPIR_CHKLMEM_MALLOC` ordering is changed, it may //
    //   be necessary to update helper various helper functions        //
    //                                                                 //
    //   `gen_rsched` expects:                                         //
    //   - mpiu_chklmem_stk_[0] = next_                                //
    //   - mpiu_chklmem_stk_[1] = prev_                                //
    //   - mpiu_chklmem_stk_[2] = skips                                //
    //   - mpiu_chklmem_stk_[3] = recv_sched                           //
    //                                                                 //
    //   `all_blocks` expects:                                         //
    //   - mpiu_chklmem_stk_[0] = next_                                //
    //   - mpiu_chklmem_stk_[1] = prev_                                //
    //   - mpiu_chklmem_stk_[2] = skips                                //
    //   - mpiu_chklmem_stk_[3] = recv_sched                           //
    //                                                                 //
    //   `gen_ssched` expects:                                         //
    //   - mpiu_chklmem_stk_[2] = skips                                //
    //   - mpiu_chklmem_stk_[3] = recv_sched                           //
    //   - mpiu_chklmem_stk_[5] = scratch_                             //
    /////////////////////////////////////////////////////////////////////
    MPIR_CHKLMEM_DECL();
    {
        int* next_; int* prev_;
        MPIR_CHKLMEM_MALLOC(next_, (depth + 2) * sizeof(int));
        MPIR_CHKLMEM_MALLOC(prev_, (depth + 2) * sizeof(int));
    }
    int* skips; int* recv_sched; int* send_sched;
    MPIR_CHKLMEM_MALLOC(skips, (depth + 1) * sizeof(int));
    MPIR_CHKLMEM_MALLOC(recv_sched, depth * sizeof(int));
    MPIR_CHKLMEM_MALLOC(send_sched, depth * sizeof(int));
    {
        int* scratch_;
        MPIR_CHKLMEM_MALLOC(scratch_, depth * sizeof(int));
    }
    
    // Precalculate skips
    //   (hard to do at runtime during the `all_blocks` subroutine)
    skips[depth] = comm_size;
    for (int i = depth - 1; i >= 0; i--) {
        skips[i] = (skips[i+1] / 2) + (skips[i+1] & 0x1);
    }

    // Get baseblock
    int baseblock = -1;
    {
        int r_ = 0;
        for (int i = depth - 1; i > 0; i--) {
            if (r_ + skips[i] == rank) {
                baseblock = i;
                break;
            } else if (r_ + skips[i] < rank) {
                r_ += skips[i];
            }

            if (i == 1) baseblock = depth;
        }
    }

    // Generate schedules
    gen_rsched(recv_sched, rank, depth, comm_size, baseblock, (int**) mpiu_chklmem_stk_);
    gen_ssched(send_sched, rank, depth, comm_size, baseblock, (int**) mpiu_chklmem_stk_);
    

    // Loop variables
    int x = (depth - ((n_chunk - 1) % depth)) % depth;
    int offset = -x;
    int current_skip = (comm_size - 1) % (comm_size & 0x1);

    // Run Schedule
    MPIR_Request* requests[1];
    for (int i = x; i < n_chunk - 1 + depth + x; i++) {
        int k = i % depth;

        if (send_sched[k] + offset >= 0) {
            int peer = (rank + current_skip) % comm_size;
            mpi_errno = MPIC_Isend(buffer, count, datatype, peer, MPIR_BCAST_TAG, comm, &requests[0], errflag);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIC_Waitall(1, requests, MPI_STATUSES_IGNORE);
            MPIR_ERR_CHECK(mpi_errno);
        } else if (recv_sched[k] + offset >= 0) {
            int peer = (rank - current_skip + comm_size) % comm_size;
            mpi_errno = MPIC_Irecv(buffer, count, datatype, peer, MPIR_BCAST_TAG, comm, &requests[0]);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIC_Waitall(1, requests, MPI_STATUSES_IGNORE);
            MPIR_ERR_CHECK(mpi_errno);
        }

        if (k == depth - 1) {
            offset += depth;
        }
    }

fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

//////// HELPER FUNCTIONS ////////

int all_blocks(int r, int r_, int s, int e, int k, int** args) {
    // If you change how `args` gets indexed,
    //   you MUST update `MPIR_Bcast_intra_circ_vring`
    int* next = args[0] + 1;
    int* prev = args[1] + 1;
    int* skips = args[2];
    int* recv_sched = args[3];

    while (e != -1) {
        if ((r_ + skips[e] <= r - skips[e]) && (r_ + skips[e] < s)) {
            if (r_ + skips[e] <= r - skips[k+1]) {
                k = all_blocks(r, r_ + skips[e], s, e, k, args);
            }
            if (r_ > r - skips[k+1]) {
                return k;
            }
            s = r_ + skips[e];
            recv_sched[k++] = e;
            next[prev[e]] = next[e];
            prev[next[e]] = prev[e];
        }
        e = next[e];
    }
    return k;
}

void gen_rsched(int* sched, int r, int q, int p, int bb, int** args) {
    // If you change how `args` gets indexed,
    //   you MUST update `MPIR_Bcast_intra_circ_vring`
    //               AND `gen_ssched`
    int* next = ((int*) args[0]) + 1;
    int* prev = ((int*) args[1]) + 1;

    for (int i = 0; i <= q; i++) {
        next[i] = i - 1;
        prev[i] = i + 1;
    }
    prev[q] = -1;
    next[-1] = q;
    prev[-1] = 0;
    next[prev[bb]] = next[bb];
    prev[next[bb]] = prev[bb];

    all_blocks(p + r, 0, p + p, q, 0, args);

    for (int i = 0; i < q; i++) {
        if (sched[i] == q) {
            sched[i] = bb;
        } else {
            sched[i] = sched[i] - q;
        }
    }
}

void gen_ssched(int* sched, int r, int q, int p, int bb, int** args) {
    int* skips = args[2];
    int* temp_sched = args[5];

    if (r == 0) {
        for (int i = 0; i < q; i++) {
            sched[i] = i;
        }
        return;
    }

    int r_ = r;
    int c = bb;
    int e = p;
    for (int i = q - 1; i > 0; i--) {
        if (r_ < skips[i]) {
            if ((r_ + skips[i] < e)
             || (e < skips[i-1])
             || ((i == 1)
             && (bb  > 0))) {
                sched[i] = c;
            } else {
                int* x[4] = { args[0], args[1], args[2], temp_sched };
                gen_rsched(temp_sched, (r + skips[i]) % p, q, p, bb, x);
                sched[i] = temp_sched[i];
            }
            if (e > skips[i]) {
                e = skips[i];
            }
        } else {
            c = i - q;
            e = e - skips[i];
            if ((r_ > skips[i])
             || (r_ <= e)
             || (i == 1)
             || (e < skips[i-1])) {
                sched[i] = c;
            } else {
                int* x[4] = { args[0], args[1], args[2], temp_sched };
                gen_rsched(temp_sched, (r + skips[i]) % p, q, p, bb, x);
                sched[i] = temp_sched[i];
            }
            r_ -= skips[i];
        }
    }
    sched[0] = bb - q;
}
