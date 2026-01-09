#include "mpiimpl.h"

/* Algorithm: Circulant graph queued variable ring bcast
 * This algorithm is based on the paper by Jesper Larsson Traff:
 * https://dl.acm.org/doi/full/10.1145/3735139, with additional optimizations.
 */

/* The circulant graph algorithm is a generalization of the binomial algorithm, with arbitrary number
 * of processes and multiple number of blocks of data.
 *
 * In order to follow the algorithm, we use the following variable names (in sync with the paper):
 *
 *     p - number of processes, comm_size
 *     r - the effective rank of the process
 *     q - ceil(log2(p)).
 *     n - total number of chunks (or blocks).
 *     k - the i-th round is referred as round k, where k = i % q. k = 0, 1, ..., q-1.
 *
 *     Skip[q+1] - each round process r send to r+Skip[k] nd receive from r-Skip[k], with a "MOD p" assumption.
 *     S[q] - in round k, this process sends the block S[k] (to r+Skip[k])
 *     R[q] - in round k, this process receives the block R[k] (from r-Skip[k])
 *
 * Because one need keep these terms in his head in order to follow the algorithm, we use these short variable
 * names so
 *  1. Be consistent with the paper.
 *  2. Easier to fit into one's working memory.
 */

/* Example send/receive schedule with p=17, q=5.
 *
 *   Skip[6] = {1, 2, 3, 5, 9, 17}
 *
 *           0 |  1 |  2 |  3   4 |  5   6   7   8 |  9  10  11  12  13  14  15  16
 *   -------------------------------------------------------------------------------
 *   R[0]:  -4 | [0]| -5 | -4  -3 | -5  -2  -5  -4 | -3  -1  -5  -4  -3  -5  -2  -5
 *   R[1]:  -5 | -4 | [1]| -5  -4 | -3  -3  -2  -5 | -4  -3  -1  -5  -4  -3  -3  -2
 *   R[2]:  -2 | -2 | -2 | [2] [0]| -4  -4  -3  -2 | -2  -4  -3  -1  -1  -4  -4  -3
 *   R[3]:  -1 | -3 | -3 | -2  -2 | [3] [0] [1] [2]| -5  -2  -2  -2  -2  -1  -1  -1
 *   R[4]:  -3 | -1 | -1 | -1  -1 | -1  -1  -1  -1 | [4] [0] [1] [2] [0] [3] [0] [1]
 *   -------------------------------------------------------------------------------
 *   S[0]:  [0]| -5 | -4 | -3  -5 | -2  -5  -4  -3 | -1  -5  -4  -3  -5  -2  -5  -4
 *   S[1]:  [1]| -5 | -4 | -3  -3 | -2  -5  -4  -3 | -1  -5  -4  -3  -3  -2  -5  -4
 *   S[2]:  [2]| [0]| -4 | -4  -3 | -2  -2  -4  -3 | -1  -1  -4  -4  -3  -2  -2  -2
 *   S[3]:  [3]| [0]| [1]| [2] -5 | -2  -2  -2  -2 | -1  -1  -1  -1  -3  -3  -2  -2
 *   S[4]:  [4]| [0]| [1]| [2] [0]| [3] [0] [1] -3 | -1  -1  -1  -1  -1  -1  -1  -1
 *   -------------------------------------------------------------------------------
 *
 * If there is only a single block, all the negative blocks are ignored, all the non-negative blocks are collapsed
 * to 0. This reconstruts the binomial algorithm for the power-of-2 case.
 *
 * If there are n blocks, blocks are shifted by q every q rounds, so negative blocks become non-negative blocks in
 * later rounds. In the last q rounds, blocks > n is collapsed to n-1, so the last q rounds completes the bcast
 * without sending any extra blocks.
 *
 * Send to root is ignored.
 *
 * Correctness condition:
 * 1. S(r)[k] = R(r+Skip[k])[k], sends and receives must agree.
 *
 * 2. Over any q rounds, each rank must receive q different blocks. More concretely -
 *       {-1,-2,...,-q}\{b-q} + b (with a block offset), where b is the baseblock for that process.
 *
 * 3. S[k] is either b-q (baseblock from last q rounds), or S[k]=R[j], where j<k. Process only can
 *    send the blocks it received from earlier rounds.
 *
 * Consequently, we observe -
 * 1. S(0)[k] = k. Root send out its blocks sequentially.
 *
 * 2. R[K] = b. Each non-root rank receives its baseblock in round K, where Skip[K] <= r < Skip[K+1].
 *
 * 3. For k > K and r+Skip[k] < Skip[k+1], S[k] = b. Essentially the same rule as 2.
 *
 * 4. S[0] = b-q. The first round only can send its baseblock from the last q round.
 *    Because Skip[0]=1, R[0] is also fixed.
 */

/* schedule generation routines */
static void gen_skips(int Skip[], int p, int q);
static void gen_rsched(int r, int R[], int p, int q, int Skip[],
                       int stack[], int next[], int prev[]);
static void gen_ssched(int r, int S[], int p, int q, int Skip[],
                       int stack[], int next[], int prev[], int extra[]);

/* math routines */
static int calc_log_p(int p);
static int calc_chunks(MPI_Aint data_size, MPI_Aint chunk_size, int *last_msg_size_out);

/* routines that manage non-blocking send/recv */
static int init_request_queue(int q_len, void *buf, int n, int chunk_size, int last_chunk_size,
                              MPIR_Comm * comm, int coll_attr, bool is_root, void **queue_out);
static int issue_send(int block, int peer_rank, void *queue_in);
static int issue_recv(int block, int peer_rank, void *queue_in);
static int complete_the_queue(void *queue_in);

/* The Circulant Graph Algorithm */
int MPIR_Bcast_intra_circ_graph(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                int root, MPIR_Comm * comm,
                                int chunk_size, int q_len, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL();

    int rank, comm_size;
    MPIR_COMM_RANK_SIZE(comm, rank, comm_size);

    if (comm_size < 2) {
        goto fn_exit;
    }
    if (q_len < 1) {
        q_len = 1;
    }

    int p = comm_size;
    int q = calc_log_p(p);
    int r = (rank - root + comm_size) % comm_size;

    /* First, we need some memory */
    int *mem;
    MPIR_CHKLMEM_MALLOC(mem, (q * 7 + 4) * sizeof(int));

#define SET_MEM(V, size) int *V; V = mem; mem += size

    /* schedule: in round k, send to r + Skip[k] block S[k], and receive from r - Skip[k] block R[k] */
    SET_MEM(Skip, q + 1);
    SET_MEM(R, q);
    SET_MEM(S, q);
    /* working memory needed by gen_rsched and gen_ssched */
    SET_MEM(stack, q + 1);
    SET_MEM(next, q + 1);
    SET_MEM(prev, q + 1);
    SET_MEM(extra, q);

    /* Generate the send and receive schedule */
    gen_skips(Skip, p, q);
    gen_rsched(r, R, p, q, Skip, stack, next, prev);
    gen_ssched(r, S, p, q, Skip, stack, next, prev, extra);

    /* Prepare the data blocks */
    MPI_Aint type_size;
    int is_contig;
    int buf_size;

    MPIR_Datatype_get_size_macro(datatype, type_size);
    buf_size = count * type_size;

    if (buf_size == 0)
        goto dealloc;

    MPIR_Datatype_is_contig(datatype, &is_contig);

    /* if necessary, pack data in tmp_buf */
    void *tmp_buf;
    if (is_contig) {
        tmp_buf = buffer;
    } else {
        MPIR_CHKLMEM_MALLOC(tmp_buf, buf_size);
        if (rank == root) {
            MPI_Aint actual_pack_bytes;
            MPIR_Typerep_pack(buffer, count, datatype, 0,
                              tmp_buf, buf_size, &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
            MPIR_Assert(actual_pack_bytes == buf_size);
        }
    }

    /* Handle pipeline chunks */
    int last_msg_size;
    int n = calc_chunks(buf_size, chunk_size, &last_msg_size);

    /* Run schedule */
    void *queue;
    mpi_errno = init_request_queue(q_len, tmp_buf, n, chunk_size, last_msg_size,
                                   comm, coll_attr, (rank == root), &queue);
    MPIR_ERR_CHECK(mpi_errno);

    int x = (q - ((n - 1) % q)) % q;
    int offset = -x;

    for (int i = x; i < n - 1 + q + x; i++) {
        int k = i % q;

        int send_block = S[k] + offset;
        if (send_block >= 0) {
            int peer = (rank + Skip[k]) % p;
            if (peer != root) {
                if (send_block >= n) {
                    send_block = n - 1;
                }

                mpi_errno = issue_send(send_block, peer, queue);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }

        int recv_block = R[k] + offset;
        if (recv_block >= 0) {
            int peer = (rank - Skip[k] + p) % p;
            if (rank != root) {
                if (recv_block >= n) {
                    recv_block = n - 1;
                }

                mpi_errno = issue_recv(recv_block, peer, queue);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }

        if (k == q - 1) {
            offset += q;
        }
    }

    mpi_errno = complete_the_queue(queue);
    MPIR_ERR_CHECK(mpi_errno);

    if (!is_contig) {
        MPI_Aint actual_unpack_bytes;
        MPIR_Typerep_unpack(tmp_buf, buf_size, buffer, count, datatype,
                            0, &actual_unpack_bytes, MPIR_TYPEREP_FLAG_NONE);
        MPIR_Assert(actual_unpack_bytes == buf_size);
    }

  dealloc:
    MPIR_CHKLMEM_FREEALL();

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- routines for creating message schedules ---- */

/* Skip[] - a q-rounds of (approximately) doubling steps. This is the key for O(q) algorithm efficiency.
 * Specifically, it is mathematically determined as
 *      Skip[0] = 1, Skip[q] = p, Skip[k] = ceil(Skip[k+1]/2)
 *
 * If p is power-of-2, the Skip[] is identical to binomial, i.e {1, 2, 4, ..., 2^q}.
 */
static void gen_skips(int Skip[], int p, int q)
{
    Skip[q] = p;
    for (int k = q - 1; k >= 0; k--) {
        Skip[k] = (Skip[k + 1] / 2) + (Skip[k + 1] & 0x1);
    }
}

/* Baseblock is the single non-negative block each rank receives during the first q rounds.
 *
 * If rank r is in region k, then it is sent from rank r-Skip[k], and shares the same
 * baseblock as r-Skip[k]. Thus we can recursively reduce r' until it reaches 0. In math -
 *    r = Skip[e1] + Skip[e2] + ... Skip[b], where e1 > e2 > ... >b, and b is the baseblock.
 */
static int get_baseblock(int r, int *Skip, int q)
{
    if (r == 0) {
        return q;
    }

    int rt = r;
    for (int k = q - 1; k >= 0; k--) {
        if (rt >= Skip[k]) {
            rt = rt - Skip[k];
            if (rt == 0) {
                return k;
            }
        }
    }
    /* can't happen */
    MPIR_Assert(0);
    return -1;
}

/* receive schedule:
 * for k = 0 to q-1, find the unique maximum baseblock from r-Skip[k+1] to r-Skip[k].
 * Use a doubly-linked list to track to avoid reusing the same baseblock.
 *
 * Instead of repeatedly call get_baseblock() for every rank to find the maximum baseblock,
 * recall that -
 *        r = Skip[e1] + Skip[e2] + ... + Skip[b], where e1>e2>...>b
 * we will try all e1, e2, ..., b in decreasing order (e1, e2, ..., b alphabetically), and when
 *        r' = Skip[e1] + Skip[e2] + ... + Skip[b], and r' falls inside [r-Skip[k+1], r-Skip[k]],
 * we know b is the largest baseblock within that range.
 *
 * There is an implied MOD(p) for expression such as r-Skip[k]. To avoid the MOD,
 * we shift r to r+p.
 *
 * However, as a result of including Skip[q], q will always be selected in place of the
 * baseblock. To see this, if r is in region i, then rank 0 (or p) is always in the range of
 * (r-Skip[i+1], r-Skip[i]], thus will be selected in R[i].
 *
 * TODO: show this rule to generate receive schedule is consistent with rules used to generate
 *       send schedule, and it fulfills the correctness conditions.
 */

static void list_init(int q, int *next, int *prev);
static void list_del(int e, int *next, int *prev);

#define PUSH \
    do { \
        rt += Skip[e]; \
        stack[sp++] = e; \
    } while (0)

#define POP \
    do { \
        sp--; \
        rt -= Skip[stack[sp]]; \
    } while (0)

static void gen_rsched(int r, int R[], int p, int q, int Skip[],
                       int stack[], int next[], int prev[])
{
    int b = get_baseblock(r, Skip, q);
    /* initialize the doubly linked list as -1->q->q-1->...->1->0->-1, "-1" marks the end */
    list_init(q, next, prev);
    list_del(b, next, prev);

    int sp = 0;                 /* stack pointer */
    int k = 0;                  /* round index */
    int rt = 0;                 /* r' */
    /* condition: low < rt <= high */
    int low = r + p - Skip[k + 1];
    int high = r + p - Skip[k];
    /* e is the last skip rt is taking. Every time we push rt, e is tracked in the stack */
    int e = q;
    PUSH;

    while (k < q) {
        /* we lied about picking between r-Skip[k+1] to r-Skip[k] *inclusive*.
         * We only consider r-Skip[k+1] if we exhausted all possible e (due to excluding b).
         */
        if (rt < low || (rt == low && next[e] > -1)) {
            e = next[e];
            PUSH;
        } else if (rt > high) {
            POP;
        } else {
            /* low < rt <= high */
            /* e may not reflect the baseblock for rt especially after a POP, thus we need reload it from the stack. */
            e = stack[sp - 1];
            R[k++] = e;
            high = low;
            low = r + p - Skip[k + 1];
            list_del(e, next, prev);
            POP;
        }
    }

    /* post process */
    for (int i = 0; i < q; i++) {
        if (R[i] == q) {
            R[i] = b;
        } else {
            R[i] = R[i] - q;
        }
    }
}

/* a doubly-linked list to avoid repeats */
static void list_init(int q, int *next, int *prev)
{
    /* initialize the list as [q]->[q-1]->...->[0]. -1 marks the ends */
    for (int i = 0; i <= q; i++) {
        next[i] = i - 1;
        prev[i] = i + 1;
    }
    prev[q] = -1;
}

static void list_del(int e, int *next, int *prev)
{
    if (prev[e] != -1) {
        next[prev[e]] = next[e];
    }
    if (next[e] != -1) {
        prev[next[e]] = prev[e];
    }
}

/* Send schedules are generated as following. It is determined by the receive schedules.
 * 1. for r = 0, S[k] = k, for k = 0:q-1.
 *    - root sends out each block sequentially to Skip[k]. R(Skip[k])[k] = k = b(Skip[k]).
 *
 * 2. for k = 0, S[0] = b - q
 *    - Skip[0] is always 1, and R(r+1)[0] = b(r) - q
 *
 * 3. for k > b and 0 < r < Skip[k], S[k] = b unless r + Skip[k] == Skip[k+1]
 *    - send base block once it is received unless the target is outside the next region
 *    - power-of-2 will never have this case, thus the entire schedules can be trivially determined.
 *    - let's call this exception entry a "violation"
 *
 * Now start with r'(q-1) = r, recursively set r'(k - 1) = (r'(k) >= Skip[k]) ? (r'(k) - Skip[k]) : r'(k)
 *
 *    4. if r'(k) > Skip[k] (upper), S[k] = k - q
 *       - recall R(r)[k] is the maximum b in (r-Skip[k+1], r-Skip[k]], which includes Skip[k], thus R(r)[k] = k.
 *
 *    5. if r'(k) < Skip[k] (lower) and r'[k] + Skip[k] < Skip[k+1], we are sending to the next region
 *         * if r=r', k>b, S[k] = b. Because r + Skip[k] receives its baseblock in this round.
 *         * otherwise, S[k]=S[k-1], which follows the same logic in rule 4.
 *
 *    6. Look up the remaining case from receive schedule: S(r)[k] = R(r+Skip[k])[k]
 *       - r'(k) = Skip[k]
 *       - r'(k) = Skip[k]-1 and Skip[k+1] is odd.
 */

/* determine send schedule according to above rules. The rules leaves the "violation" cases unresolved,
 * we simply call gen_rsched() to look it up. The "violation" cases are limited to less than 4.
 */

#define FROM_RSCHED() \
    do { \
        int *R = extra; \
        int t = (r + Skip[k]) % p; \
        gen_rsched(t, R, p, q, Skip, stack, next, prev); \
        S[k] = R[k]; \
    } while (0)

static void gen_ssched(int r, int S[], int p, int q, int Skip[],
                       int stack[], int next[], int prev[], int extra[])
{
    if (r == 0) {
        for (int k = 0; k < q; k++) {
            S[k] = k;
        }
    } else {
        int b = get_baseblock(r, Skip, q);
        /* recursively map r to rt into region k */
        int rt = r;
        /* Rule 5 - set cur_block to S[k+1], starting as the baseblock */
        int cur_block = b;
        for (int k = q - 1; k > 0; k--) {
            if (rt < Skip[k]) {
                /* lower part */
                if (rt + Skip[k] < Skip[k + 1]) {
                    S[k] = cur_block;
                } else {
                    FROM_RSCHED();
                }
            } else {
                /* upper part */
                cur_block = k - q;
                if (rt > Skip[k]) {
                    S[k] = cur_block;
                } else {
                    FROM_RSCHED();
                }
                rt -= Skip[k];
            }
        }
        S[0] = b - q;
    }
}

/* ---- routines that manage non-blocking send/recv ---- */

struct request_queue {
    int q_len;
    void *buf;
    int n;
    int chunk_size;
    int last_chunk_size;
    MPIR_Comm *comm;
    int coll_attr;

    bool *can_send;             /* can_send[n], set to true once the block is received */
    struct {
        MPIR_Request *req;
        int chunk_id;
    } *requests;                /* requests[q_len] */
    int q_head;
    int q_tail;
};

static int wait_if_full(struct request_queue *queue);
static int wait_for_request(int i, struct request_queue *queue);

static int init_request_queue(int q_len, void *buf, int n, int chunk_size, int last_chunk_size,
                              MPIR_Comm * comm, int coll_attr, bool is_root, void **queue_out)
{
    int mpi_errno = MPI_SUCCESS;

    struct request_queue *queue = MPL_malloc(sizeof(struct request_queue), MPL_MEM_OTHER);
    MPIR_ERR_CHKANDJUMP(!queue, mpi_errno, MPI_ERR_OTHER, "**nomem");

    queue->q_len = q_len;
    queue->buf = buf;
    queue->n = n;
    queue->chunk_size = chunk_size;
    queue->last_chunk_size = last_chunk_size;
    queue->comm = comm;
    queue->coll_attr = coll_attr;

    queue->q_head = 0;
    queue->q_tail = 0;

    queue->can_send = MPL_malloc(n * sizeof(*queue->can_send), MPL_MEM_OTHER);
    if (!queue->can_send) {
        MPL_free(queue);
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    queue->requests = MPL_malloc(q_len * sizeof(*queue->requests), MPL_MEM_OTHER);
    if (!queue->requests) {
        MPL_free(queue);
        MPL_free(queue->can_send);
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    for (int i = 0; i < n; i++) {
        queue->can_send[i] = is_root;
    }

    for (int i = 0; i < q_len; i++) {
        queue->requests[i].req = NULL;
        queue->requests[i].chunk_id = -1;
    }

    *queue_out = queue;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int issue_send(int block, int peer_rank, void *queue_in)
{
    int mpi_errno = MPI_SUCCESS;
    struct request_queue *queue = queue_in;

    if (queue->can_send[block] == 0) {
        for (int i = 0; i < queue->q_len; i++) {
            if (queue->requests[i].chunk_id == block) {
                mpi_errno = wait_for_request(i, queue);
                MPIR_ERR_CHECK(mpi_errno);
                break;
            }
        }
    }
    MPIR_Assert(queue->can_send[block]);

    void *buf = (char *) queue->buf + block * queue->chunk_size;
    MPI_Aint msg_size = (block == queue->n - 1) ? queue->last_chunk_size : queue->chunk_size;

    mpi_errno = MPIC_Isend(buf, msg_size, MPIR_BYTE_INTERNAL,
                           peer_rank, MPIR_BCAST_TAG, queue->comm,
                           &(queue->requests[queue->q_head].req), queue->coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

    queue->requests[queue->q_head].chunk_id = -1;

    queue->q_head = (queue->q_head + 1) % queue->q_len;

    mpi_errno = wait_if_full(queue);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int issue_recv(int block, int peer_rank, void *queue_in)
{
    int mpi_errno = MPI_SUCCESS;
    struct request_queue *queue = queue_in;

    void *buf = (char *) queue->buf + block * queue->chunk_size;
    MPI_Aint msg_size = (block == queue->n - 1) ? queue->last_chunk_size : queue->chunk_size;

    mpi_errno = MPIC_Irecv(buf, msg_size, MPIR_BYTE_INTERNAL,
                           peer_rank, MPIR_BCAST_TAG, queue->comm,
                           &(queue->requests[queue->q_head].req));
    MPIR_ERR_CHECK(mpi_errno);

    queue->requests[queue->q_head].chunk_id = block;

    queue->q_head = (queue->q_head + 1) % queue->q_len;

    mpi_errno = wait_if_full(queue);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int complete_the_queue(void *queue_in)
{
    int mpi_errno = MPI_SUCCESS;
    struct request_queue *queue = queue_in;

    for (int i = 0; i < queue->q_len; i++) {
        if (queue->requests[i].req) {
            mpi_errno = MPIC_Wait(queue->requests[i].req);
            MPIR_ERR_CHECK(mpi_errno);

            MPIR_Request_free(queue->requests[i].req);
        }
    }
    MPL_free(queue->can_send);
    MPL_free(queue->requests);
    MPL_free(queue);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int wait_if_full(struct request_queue *queue)
{
    int mpi_errno = MPI_SUCCESS;

    if (queue->q_head == queue->q_tail) {
        if (queue->requests[queue->q_tail].req) {
            mpi_errno = wait_for_request(queue->q_tail, queue);
            MPIR_ERR_CHECK(mpi_errno);
        }

        queue->q_tail = (queue->q_tail + 1) % queue->q_len;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int wait_for_request(int i, struct request_queue *queue)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIC_Wait(queue->requests[i].req);
    MPIR_ERR_CHECK(mpi_errno);

    if (queue->requests[i].chunk_id != -1) {
        /* it's a recv, update can_send */
        queue->can_send[queue->requests[i].chunk_id] = 1;
    }
    MPIR_Request_free(queue->requests[i].req);
    queue->requests[i].req = NULL;
    queue->requests[i].chunk_id = -1;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* -- math routines -- */

static int calc_log_p(int p)
{
    /* q = ceil(log2(p)) */
    int q = 1;
    while ((0x1 << q) < p) {
        q++;
    }
    return q;
}

static int calc_chunks(MPI_Aint buf_size, MPI_Aint chunk_size, int *last_msg_size_out)
{
    int n;
    int last_msg_size;

    if (chunk_size == 0) {
        n = 1;
        last_msg_size = buf_size;
    } else {
        n = (buf_size / chunk_size);
        if (buf_size % chunk_size == 0) {
            last_msg_size = chunk_size;
        } else {
            n++;
            last_msg_size = buf_size % chunk_size;
        }
    }
    *last_msg_size_out = last_msg_size;
    return n;
}
