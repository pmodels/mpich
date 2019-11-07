/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ft=c.mpich : */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_SHM_H_INCLUDED
#define MPIDU_SHM_H_INCLUDED

size_t MPIDU_shm_get_mapsize(size_t size, size_t * psz);

/* This is a collective function: every process in the communicator comm must
 * call it. The function expects the amount of memory to be allocated per node.
 *
 * If the node contains multiple processes the function tries to allocate symmetric
 * shared memory (i.e., base address is the same on all processes).  If the node
 * contains only one process the function allocates private memory for the process
 * but still attempts to get the base address equal across all processes. If none
 * of the processes in the node requests any memory they behave as if symmetric
 * memory was allocated successfully.
 *
 * The function returns MPI_SUCCESS and fail_flag = true if successful; MPI_SUCCESS
 * and fail_flag = true if symmetric shared memory could not be allocated; and MPI_ERR_OTHER
 * if any other error occurred.
 *
 * The offset argument is used to shift the base address returned by the function
 * so that the base address for all processes in different nodes is the same.
 * This optimization is useful in RMA operations as it allows processes to perform
 * data transfers without querying the target for the address of the buffer.
 *
 * It is up to the caller to map the process to the right position in shared memory
 * or try another allocation mechanism if an error code is returned. */
int MPIDU_shm_alloc_symm_all(MPIR_Comm * comm_ptr, size_t len, size_t off, void **ptr,
                             bool * fail_flag);

/* This is a collective function: every process in the communicator shm_comm must
 * call it. The function expects the amount of memory to be allocated per node.
 *
 * If the node contains multiple processes the function tries to allocate shared
 * memory (symmetry is not checked). If the node contains only one process the
 * function allocates private memory for it.
 *
 * The function returns MPI_SUCCESS and fail_flag = false if successful; MPI_SUCCESS and
 * fail_flag = true if shared memory could not be allocated; and MPI_ERR_OTHER if any
 * other error occurred.
 *
 * It is up to the caller to map the process to the right position in shared memory
 * or try another allocation mechanism if an error code is returned. */
int MPIDU_shm_alloc(MPIR_Comm * shm_comm_ptr, size_t len, void **ptr, bool * fail_flag);

/* This function frees the memory pointed to by ptr. Memory has to be allocated with
 * either MPIDU_shm_alloc_symm_all() or MPIDU_shm_alloc() to be freed correctly. If
 * malloc'd memory is passed instead the function will fail with an assert. */
int MPIDU_shm_free(void *ptr);

#endif /* MPIDU_SHM_H_INCLUDED */
