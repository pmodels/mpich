#include "mpiimpl.h"

/* Algorithm: Circulant graph queued variable ring bcast
 * This algorithm is based on the paper by Jesper Larsson Traff:
 * https://dl.acm.org/doi/full/10.1145/3735139, with additional optimizations.
 * It is optimal for both small and large message sizes.
 */

struct sched_args_t {
    int *skips;
    int *send_sched;
    int *next;
    int *prev;
    int *extra;
    int tree_depth;
    int comm_size;
};

struct queue_tracker_t {
    MPIR_Request *req;
    int chunk_id;
    int need_wait;
};

static int get_baseblock(int r, struct sched_args_t *args);
static int all_blocks(int r, int r_prime, int s, int e, int k, int *buffer,
                      struct sched_args_t *args);
static void gen_rsched(int r, int *buffer, struct sched_args_t *args);
static void gen_ssched(int r, struct sched_args_t *args);

int MPIR_Bcast_intra_circ_qvring(void *buffer,
                                 MPI_Aint count,
                                 MPI_Datatype datatype,
                                 int root, MPIR_Comm * comm,
                                 const int chunk_size, const int q_len, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;

    int comm_size = comm->local_size;
    int rank = comm->rank;

    if (comm_size < 2) {
        goto fn_exit;
    }

    int depth = 1;
    while (0x1 << depth < comm_size) {
        depth++;
    }

    /* Circulant Graph Queued Variable Ring Bcast:
     * This algorithm uses the circulant graph abstraction to create communication
     * rings of every "n" processes, where "n" values are called "skips".
     * We use a roughly doubling pattern to generate the skips, creating log2(p)
     * communication rounds, making the algorithm latency-optimal for small messages.
     *
     * The algorithm works by generating a schedule of sends and receives based on the
     * number of processes (see paper for algorithm). The schedule is then executed to
     * complete the broadcast. The message size can be broken into chunks and pipelined,
     * repeating the send schedule for each chunk, to optimize for bandwidth/large messages.
     *
     * This implementation also includes a queued send optimization, where "q_len" sends
     * can be outstanding simultaneously. Bcast is a one->many operation, so the root can
     * begin the algorithm with "q_len" non-blocking sends, and each receiving process then
     * does the same. New sends are introduced when previous sends complete using a FIFO
     * queue. This optimization further improves small-message performance.
     */

    MPIR_CHKLMEM_DECL();
    int *skips;
    int *recv_sched;
    int *send_sched;
    int *next;
    int *prev;
    int *extra;
    MPIR_CHKLMEM_MALLOC(skips, (depth + 1) * sizeof(int));
    MPIR_CHKLMEM_MALLOC(recv_sched, depth * sizeof(int));
    MPIR_CHKLMEM_MALLOC(send_sched, depth * sizeof(int));
    MPIR_CHKLMEM_MALLOC(next, (depth + 2) * sizeof(int));
    MPIR_CHKLMEM_MALLOC(prev, (depth + 2) * sizeof(int));
    MPIR_CHKLMEM_MALLOC(extra, depth * sizeof(int));

    // Precalculate skips (roughly doubling)
    skips[depth] = comm_size;
    for (int i = depth - 1; i >= 0; i--) {
        skips[i] = (skips[i + 1] / 2) + (skips[i + 1] & 0x1);
    }

    // Generate send and receive schedules
    struct sched_args_t args = {
        skips, send_sched, next + 1, prev + 1, extra,
        depth, comm_size
    };
    gen_rsched(rank, recv_sched, &args);
    gen_ssched(rank, &args);

    // Datatype Handling:
    MPI_Aint type_size;
    int is_contig;
    int buf_size;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    buf_size = count * type_size;

    if (buf_size == 0)
        goto dealloc;

    if (HANDLE_IS_BUILTIN(datatype))
        is_contig = 1;
    else {
        MPIR_Datatype_is_contig(datatype, &is_contig);
    }
    void *tmp_buf;
    if (is_contig) {
        tmp_buf = buffer;
    } else {
        MPIR_CHKLMEM_MALLOC(tmp_buf, buf_size);
        if (rank == root) {
            mpi_errno =
                MPIR_Localcopy(buffer, count, datatype, tmp_buf, buf_size, MPIR_BYTE_INTERNAL);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    // Handle pipeline chunks
    int n_chunk;
    int last_msg_size;

    if (chunk_size == 0) {
        n_chunk = 1;
        last_msg_size = buf_size;
    } else {
        n_chunk = (buf_size / chunk_size) + (buf_size % chunk_size != 0);
        last_msg_size = (buf_size % chunk_size == 0)
            ? chunk_size : buf_size % chunk_size;
    }

    char *can_send;
    MPIR_CHKLMEM_MALLOC(can_send, n_chunk * sizeof(char));
    for (int i = 0; i < n_chunk; i++) {
        can_send[i] = (rank == root);
    }

    // Run schedule
    int x = (((depth - ((n_chunk - 1) % depth)) % depth) + depth) % depth;
    int offset = -x;

    int tru_ql = q_len;
    if (tru_ql < 1)
        tru_ql = 1;

    struct queue_tracker_t *requests;
    MPIR_CHKLMEM_MALLOC(requests, tru_ql * sizeof(struct queue_tracker_t));
    for (int i = 0; i < tru_ql; i++) {
        requests[i].need_wait = 0;
    }

    int q_head = 0;
    int q_tail = 0;
    int q_used = 0;
    for (int i = x; i < n_chunk - 1 + depth + x; i++) {
        int k = i % depth;

        if (send_sched[k] + offset >= 0) {
            int peer = (rank + skips[k]) % comm_size;
            if (peer) {
                int send_block = send_sched[k] + offset;
                if (send_block >= n_chunk)
                    send_block = n_chunk - 1;
                int msg_size = (send_block != n_chunk - 1) ? chunk_size : last_msg_size;

                if (can_send[send_block] == 0) {
                    for (int j = 0; j < tru_ql; j++) {
                        if (requests[j].chunk_id == send_block) {
                            mpi_errno = MPIC_Wait(requests[j].req);
                            MPIR_ERR_CHECK(mpi_errno);
                            requests[j].need_wait = 0;
                            MPIR_Request_free(requests[j].req);

                            can_send[send_block] = 1;
                            break;
                        }
                    }
                }

                mpi_errno =
                    MPIC_Isend(((char *) tmp_buf) + (chunk_size * send_block), msg_size,
                               MPIR_BYTE_INTERNAL, peer, MPIR_BCAST_TAG, comm,
                               &(requests[q_head].req), coll_attr);
                MPIR_ERR_CHECK(mpi_errno);
                requests[q_head].chunk_id = -1;
                requests[q_head].need_wait = 1;

                q_head = (q_head + 1) % tru_ql;
                q_used = 1;
            }
        }

        if (q_used && q_head == q_tail) {
            if (requests[q_tail].need_wait) {
                mpi_errno = MPIC_Wait(requests[q_tail].req);
                MPIR_ERR_CHECK(mpi_errno);
                requests[q_tail].need_wait = 0;
                MPIR_Request_free(requests[q_tail].req);
            }

            if (requests[q_tail].chunk_id != -1) {
                can_send[requests[q_tail].chunk_id] = 1;
            }

            q_tail = (q_tail + 1) % tru_ql;
        }

        if (recv_sched[k] + offset >= 0 && (rank != root)) {
            int peer = (rank - skips[k] + comm_size) % comm_size;

            int recv_block = recv_sched[k] + offset;
            if (recv_block >= n_chunk)
                recv_block = n_chunk - 1;
            int msg_size = (recv_block != n_chunk - 1) ? chunk_size : last_msg_size;

            mpi_errno =
                MPIC_Irecv(((char *) tmp_buf) + (chunk_size * recv_block), msg_size,
                           MPIR_BYTE_INTERNAL, peer, MPIR_BCAST_TAG, comm, &(requests[q_head].req));
            MPIR_ERR_CHECK(mpi_errno);
            requests[q_head].chunk_id = recv_block;
            requests[q_head].need_wait = 1;

            q_head = (q_head + 1) % tru_ql;
            q_used = 1;
        }

        if (q_used && q_head == q_tail) {
            if (requests[q_tail].need_wait) {
                mpi_errno = MPIC_Wait(requests[q_tail].req);
                MPIR_ERR_CHECK(mpi_errno);
                requests[q_tail].need_wait = 0;
                MPIR_Request_free(requests[q_tail].req);
            }

            if (requests[q_tail].chunk_id != -1) {
                can_send[requests[q_tail].chunk_id] = 1;
            }

            q_tail = (q_tail + 1) % tru_ql;
        }

        if (k == depth - 1) {
            offset += depth;
        }
    }

    for (int i = 0; i < tru_ql; i++) {
        if (requests[i].need_wait) {
            mpi_errno = MPIC_Wait(requests[i].req);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_Request_free(requests[i].req);
        }
    }

    if (!is_contig) {
        mpi_errno = MPIR_Localcopy(tmp_buf, buf_size, MPIR_BYTE_INTERNAL, buffer, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

  dealloc:
    MPIR_CHKLMEM_FREEALL();

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Helper Functions:
 * get_baseblock(): calculates the baseblock (i.e., the first pipeline chunk a process will receive)
 * all_blocks(): stores candidate skip indices in linked list (helper for receive schedule)
 * gen_rsched(): generates the receive schedule
 * gen_ssched(): generates the send schedule
 */

/*
 * Variable glossary:
 * r = rank
 * r_prime = sum of chosen skips (i.e., largest processor reached so far)
*/
static int get_baseblock(int r, struct sched_args_t *args)
{
    int r_prime = 0;
    for (int i = args->tree_depth - 1; i >= 0; i--) {
        if (r_prime + args->skips[i] == r) {
            return i;
        } else if (r_prime + args->skips[i] < r) {
            r_prime += args->skips[i];
        }
    }
    return args->tree_depth;
}

/*
 * Variable glossary:
 * r = rank
 * r_prime = largest intermediate processor found so far in the current search
 * s = Accepted intermediate processor from previous search (to force strict decreasing)
 * e = current skip index
 * k = receive chunk index
*/
static int all_blocks(int r, int r_prime, int s, int e, int k, int *buffer,
                      struct sched_args_t *args)
{
    while (e != -1) {
        if ((r_prime + args->skips[e] <= r - args->skips[k])
            && (r_prime + args->skips[e] < s)) {
            if (r_prime + args->skips[e] <= r - args->skips[k + 1]) {
                k = all_blocks(r, r_prime + args->skips[e], s, e, k, buffer, args);
            }
            if (r_prime > r - args->skips[k + 1]) {
                return k;
            }
            s = r_prime + args->skips[e];
            buffer[k] = e;
            k += 1;
            args->next[args->prev[e]] = args->next[e];
            args->prev[args->next[e]] = args->prev[e];
        }
        e = args->next[e];
    }
    return k;
}

/*
 * Variable glossary:
 * Same as all_blocks()
*/
static void gen_rsched(int r, int *buffer, struct sched_args_t *args)
{
    for (int i = 0; i <= args->tree_depth; i++) {
        args->next[i] = i - 1;
        args->prev[i] = i + 1;
    }
    args->prev[args->tree_depth] = -1;
    args->next[-1] = args->tree_depth;
    args->prev[-1] = 0;

    int b = get_baseblock(r, args);

    args->next[args->prev[b]] = args->next[b];
    args->prev[args->next[b]] = args->prev[b];

    all_blocks(args->comm_size + r, 0, args->comm_size * 2, args->tree_depth, 0, buffer, args);

    for (int i = 0; i < args->tree_depth; i++) {
        if (buffer[i] == args->tree_depth) {
            buffer[i] = b;
        } else {
            buffer[i] = buffer[i] - args->tree_depth;
        }
    }
}

/*
 * Variable glossary:
 * r = rank
 * r_prime = virtual processor rank
 * b = base block
 * c = current candidate block to send
 * e = upper bound of current send range
*/
static void gen_ssched(int r, struct sched_args_t *args)
{
    if (r == 0) {
        for (int i = 0; i < args->tree_depth; i++) {
            args->send_sched[i] = i;
        }
        return;
    }

    int b = get_baseblock(r, args);

    int r_prime = r;
    int c = b;
    int e = args->comm_size;
    for (int i = args->tree_depth - 1; i > 0; i--) {
        if (r_prime < args->skips[i]) {
            if ((r_prime + args->skips[i] < e)
                || (e < args->skips[i - 1])
                || ((i == 1)
                    && (b > 0))) {
                args->send_sched[i] = c;
            } else {
                gen_rsched((r + args->skips[i]) % args->comm_size, args->extra, args);
                args->send_sched[i] = args->extra[i];
            }
            if (e > args->skips[i]) {
                e = args->skips[i];
            }
        } else {
            c = i - args->tree_depth;
            e = e - args->skips[i];
            if ((r_prime > args->skips[i])
                || (r_prime <= e)
                || (i == 1)
                || (e < args->skips[i - 1])) {
                args->send_sched[i] = c;
            } else {
                gen_rsched((r + args->skips[i]) % args->comm_size, args->extra, args);
                args->send_sched[i] = args->extra[i];
            }
            r_prime -= args->skips[i];
        }
    }
    args->send_sched[0] = b - args->tree_depth;
}
