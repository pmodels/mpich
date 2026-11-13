/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_BINE_H
#define MPIR_BINE_H

#define BINE_MAX_STEPS 20

#include "mpiimpl.h"

static const int rhos[BINE_MAX_STEPS] = {
    1, -1, 3, -5, 11, -21, 43, -85, 171, -341,
    683, -1365, 2731, -5461, 10923, -21845, 43691, -87381, 174763, -349525
};

static const int smallest_negabinary[BINE_MAX_STEPS] = {
    0, 0, -2, -2, -10, -10, -42, -42, -170, -170,
    -682, -682, -2730, -2730, -10922, -10922, -43690, -43690, -174762, -174762
};

static const int largest_negabinary[BINE_MAX_STEPS] = {
    0, 1, 1, 5, 5, 21, 21, 85, 85, 341,
    341, 1365, 1365, 5461, 5461, 21845, 21845, 87381, 87381, 349525
};

static inline int MPII_Bine_mod(int a, int b)
{
#ifdef MPL_HAVE_BUILTIN_POPCOUNT
    if (MPL_is_pof2(b)) {
        return a & (b - 1);
    }
#endif
    int r = a % b;
    return r < 0 ? r + b : r;
}

/**
 * @brief Computes the destination rank for a given process in a bine
 * algorithm step.
 *
 * This function calculates the rank to which a process will communicate
 * based on the bine algorithm, ensuring the result is within the valid
 * range of ranks.
 *
 * @param rank The rank of the current process.
 * @param step The current step in the bine algorithm.
 * @param comm_sz The total number of processes in the communicator.
 * @return The destination rank after applying the bine algorithm, a
 *         value in [0, comm_sz - 1].
 */
static inline int MPII_Bine_pi(int rank, int step, int comm_sz)
{
    int dest;

    if ((rank & 1) == 0) {
        dest = MPII_Bine_mod(rank + rhos[step], comm_sz);       /* Even rank */
    } else {
        dest = MPII_Bine_mod(rank - rhos[step], comm_sz);       /* Odd rank */
    }

    if (dest < 0) {
        dest += comm_sz;        /* Adjust for negative ranks */
    }

    return dest;
}

static inline void MPII_Bine_get_permutation_aux(int rank, int step, const int n_steps,
                                                 const int adj_size, int *bitmap, int offset)
{
    *(bitmap + rank) = offset;
    if (step >= n_steps)
        return;

    int peer;

    for (int s = step; s < n_steps; s++) {
        peer = MPII_Bine_pi(rank, s, adj_size);
        MPII_Bine_get_permutation_aux(peer, s + 1, n_steps, adj_size, bitmap,
                                      offset + (1 << (n_steps - s - 1)));
    }
}

static inline void MPII_Bine_get_permutation(int rank, int step, const int n_steps,
                                             const int adj_size, int *bitmap, int offset)
{
    if (step >= n_steps)
        return;

    int peer = MPII_Bine_pi(rank, step, adj_size);
    MPII_Bine_get_permutation_aux(peer, step + 1, n_steps, adj_size, bitmap, offset);
}

static inline void MPII_Bine_get_indexes_aux(int rank, int step, const int n_steps,
                                             const int adj_size, int *bitmap)
{
    if (step >= n_steps)
        return;

    int peer;

    for (int s = step; s < n_steps; s++) {
        peer = MPII_Bine_pi(rank, s, adj_size);
        *(bitmap + peer) = 0x1;
        MPII_Bine_get_indexes_aux(peer, s + 1, n_steps, adj_size, bitmap);
    }
}

static inline void MPII_Bine_get_indexes(int rank, int step, const int n_steps,
                                         const int adj_size, int *bitmap)
{
    if (step >= n_steps)
        return;

    int peer = MPII_Bine_pi(rank, step, adj_size);
    *(bitmap + peer) = 0x1;
    MPII_Bine_get_indexes_aux(peer, step + 1, n_steps, adj_size, bitmap);
}

/**
 * @brief Returns log_2(value). Value must be a positive integer.
 *        If value is not a power of two, returns ceil(log2(value)).
 *
 * @param value The **POSITIVE** integer value to return its log_2.
 *
 * @returns The log_2 of value or -1 for negative value and 0.
 */
static inline int MPII_Bine_log2(int value)
{
    if (unlikely(value <= 0)) {
        return -1;
    }
    int log = MPL_log2(value);
    if (!MPL_is_pof2(value)) {
        log += 1;
    }
    return log;
}

/**
 * Calculates the highest bit in an integer
 *
 * @param value The integer value to examine
 * @param start Position to start looking
 *
 * @returns pos Position of highest-set integer or -1 if none are set.
 *
 * Look at the integer "value" starting at position "start", and move
 * to the right.  Return the index of the highest bit that is set to
 * 1.
 *
 * WARNING: *NO* error checking is performed.  This is meant to be a
 * fast inline function.
 */
static inline int MPII_Bine_hibit(int value, int start)
{
    unsigned int mask;

    /* Only look at the part that the caller wanted looking at */
    mask = value & ((1 << start) - 1);

    if (0 == mask) {
        return -1;
    }

    return MPL_log2(mask);
}

/**
 * @brief Reorders blocks in a buffer according to a given permutation.
 *
 * @param buffer The buffer containing the blocks to reorder.
 * @param block_size The size of each block in bytes.
 * @param block_permutation The permutation of the blocks.
 * @param num_blocks The number of blocks in the buffer.
 *
 * @return MPI_SUCCESS on success, or an error code.
 */
static inline int MPII_Bine_reorder_blocks(void *buffer, MPI_Aint block_size,
                                           MPI_Datatype dtype, int *block_permutation,
                                           int num_blocks)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint extent;
    char *buf = (char *) buffer;
    char *visited = NULL;
    void *temp = NULL;

    MPIR_CHKLMEM_DECL();

    MPIR_ERR_CHKANDJUMP(unlikely(buffer == NULL || block_permutation == NULL ||
                                 num_blocks <= 0), mpi_errno, MPI_ERR_ARG, "**arg");

    MPIR_Datatype_get_extent_macro(dtype, extent);
    MPIR_CHKLMEM_MALLOC(temp, block_size * extent);

    MPIR_CHKLMEM_MALLOC(visited, num_blocks * sizeof(char));
    memset(visited, 0, num_blocks * sizeof(char));

    for (int i = 0; i < num_blocks; ++i) {
        /* Skip if the block is already in its correct position or visited */
        if (visited[i] == 1 || block_permutation[i] == i) {
            continue;
        }

        int current = i;
        /* Save the current block to temp (start of the cycle) */
        mpi_errno = MPIR_Localcopy(buf + current * block_size * extent, block_size, dtype,
                                   temp, block_size, dtype);
        MPIR_ERR_CHECK(mpi_errno);

        /* Follow the cycle and place each block in its final position */
        while (visited[block_permutation[current]] != 1) {
            int next = block_permutation[current];
            mpi_errno = MPIR_Localcopy(buf + next * block_size * extent, block_size, dtype,
                                       buf + current * block_size * extent, block_size, dtype);
            MPIR_ERR_CHECK(mpi_errno);
            visited[current] = 1;
            current = next;
        }

        /* Place the saved block in its final position */
        mpi_errno = MPIR_Localcopy(temp, block_size, dtype,
                                   buf + current * block_size * extent, block_size, dtype);
        MPIR_ERR_CHECK(mpi_errno);
        /* Mark the last block as visited */
        visited[current] = 1;
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline uint32_t MPII_Bine_binary_to_negabinary(int32_t bin)
{
    if (unlikely(bin > 0x55555555))
        return -1;
    const uint32_t mask = 0xAAAAAAAA;
    return (mask + bin) ^ mask;
}

static inline int32_t MPII_Bine_negabinary_to_binary(uint32_t neg)
{
    const uint32_t mask = 0xAAAAAAAA;
    return (mask ^ neg) - mask;
}

static inline int MPII_Bine_in_range(int x, uint32_t nbits)
{
    return x >= smallest_negabinary[nbits] && x <= largest_negabinary[nbits];
}

static inline uint32_t MPII_Bine_reverse(uint32_t x)
{
    x = ((x >> 1) & 0x55555555u) | ((x & 0x55555555u) << 1);
    x = ((x >> 2) & 0x33333333u) | ((x & 0x33333333u) << 2);
    x = ((x >> 4) & 0x0f0f0f0fu) | ((x & 0x0f0f0f0fu) << 4);
    x = ((x >> 8) & 0x00ff00ffu) | ((x & 0x00ff00ffu) << 8);
    x = ((x >> 16) & 0xffffu) | ((x & 0xffffu) << 16);
    return x;
}

static inline uint32_t MPII_Bine_get_rank_negabinary_representation(int num_ranks, int rank)
{

    uint32_t nba = UINT32_MAX, nbb = UINT32_MAX;

    if (num_ranks == 1) {
        return 0;
    }

    MPI_Aint num_bits = MPII_Bine_log2(num_ranks);
    if (rank % 2) {
        if (MPII_Bine_in_range(rank, num_bits)) {
            nba = MPII_Bine_binary_to_negabinary(rank);
        }
        if (MPII_Bine_in_range(rank - num_ranks, num_bits)) {
            nbb = MPII_Bine_binary_to_negabinary(rank - num_ranks);
        }
    } else {
        if (MPII_Bine_in_range(-rank, num_bits)) {
            nba = MPII_Bine_binary_to_negabinary(-rank);
        }
        if (MPII_Bine_in_range(-rank + num_ranks, num_bits)) {
            nbb = MPII_Bine_binary_to_negabinary(-rank + num_ranks);
        }
    }

    MPIR_Assert(nba != UINT32_MAX || nbb != UINT32_MAX);

    if (nba == UINT32_MAX && nbb != UINT32_MAX) {
        return nbb;
    } else if (nba != UINT32_MAX && nbb == UINT32_MAX) {
        return nba;
    } else {    /* Check MSB */
        if (nba & (0x80000000 >> (32 - num_bits))) {
            return nba;
        } else {
            return nbb;
        }
    }
}

static inline int MPII_Bine_remap_rank(int num_ranks, int rank)
{
    if (num_ranks == 1)
        return 0;
    uint32_t remap_rank = MPII_Bine_get_rank_negabinary_representation(num_ranks, rank);
    remap_rank = remap_rank ^ (remap_rank >> 1);
    int num_bits = MPII_Bine_log2(num_ranks);
    remap_rank = MPII_Bine_reverse(remap_rank) >> (32 - num_bits);
    return remap_rank;
}

static inline int MPII_Bine_get_sender_aux(int num_ranks, int rank, int root)
{
    int remap = MPII_Bine_remap_rank(num_ranks, rank);

    if (remap == root)
        return rank;
    else
        return MPII_Bine_get_sender_aux(num_ranks, remap, root);
}

static inline int MPII_Bine_get_sender_rec(int num_ranks, int rank)
{
    return MPII_Bine_get_sender_aux(num_ranks, rank, rank);
}

/* Function to calculate a Mersenne number (2^n - 1) */
static inline uint32_t MPII_Bine_mersenne(int n)
{
    return (1UL << (n + 1)) - 1;
}


static inline int MPII_Bine_remap_distance_doubling(uint32_t num)
{
    int remapped = 0;
    while (num > 0) {
        int k = -1;
/* TODO: Maybe this should be replaced by MPL_log2() */
#ifdef MPL_HAVE_BUILTIN_CLZ
        k = 31 - __builtin_clz(num);    /* Find the position of the highest set bit */
#else
        int n = num;
        while (n > 0) {
            n >>= 1;
            k++;
        }
#endif
        remapped ^= (0x1 << k); /* Set the k-th bit in the remapped number */
        num ^= MPII_Bine_mersenne(k);   /* XOR the Mersenne number with the remaining number */
    }
    return remapped;
}

static inline uint32_t MPII_Bine_nb_to_nu(uint32_t nb, uint32_t size)
{
    return MPII_Bine_reverse(nb ^ (nb >> 1)) >> (32 - MPII_Bine_log2(size));
}

static inline uint32_t MPII_Bine_get_nu(uint32_t rank, uint32_t size)
{
    uint32_t nba = UINT32_MAX, nbb = UINT32_MAX;
    MPI_Aint num_bits = MPII_Bine_log2(size);
    if (rank % 2) {
        if (MPII_Bine_in_range(rank, num_bits)) {
            nba = MPII_Bine_binary_to_negabinary(rank);
        }
        if (MPII_Bine_in_range(rank - size, num_bits)) {
            nbb = MPII_Bine_binary_to_negabinary(rank - size);
        }
    } else {
        if (MPII_Bine_in_range(-rank, num_bits)) {
            nba = MPII_Bine_binary_to_negabinary(-rank);
        }
        if (MPII_Bine_in_range(-rank + size, num_bits)) {
            nbb = MPII_Bine_binary_to_negabinary(-rank + size);
        }
    }
    MPIR_Assert(nba != UINT32_MAX || nbb != UINT32_MAX);

    if (nba == UINT32_MAX && nbb != UINT32_MAX) {
        return MPII_Bine_nb_to_nu(nbb, size);
    } else if (nba != UINT32_MAX && nbb == UINT32_MAX) {
        return MPII_Bine_nb_to_nu(nba, size);
    } else {    /* Check MSB */
        int nu_a = MPII_Bine_nb_to_nu(nba, size);
        int nu_b = MPII_Bine_nb_to_nu(nbb, size);
        if (nu_a < nu_b) {
            return nu_a;
        } else {
            return nu_b;
        }
    }
}

static inline unsigned int MPII_Bine_ffs(int x)
{
#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
    return __builtin_ffs(x);
#else
    int pos;
    if (x == 0) {
        return 0;
    }
    pos = 1;
    unsigned int ux = (unsigned int) x;
    while ((ux & 1) == 0) {
        pos++;
        ux >>= 1;
    }
    return pos;
#endif
}

#endif /* MPIR_BINE_H */
