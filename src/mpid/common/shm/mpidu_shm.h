/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDU_SHM_H_INCLUDED
#define MPIDU_SHM_H_INCLUDED

/* MPIDU_shm_get_mapsize
 *
 * - size:  size of memory to be rounded to page size
 * - psz :  page size in the system
 *
 * The function returns the page rounded value for argument `size` and also
 * the size of the page in the system in the `psz` pointer argument. */
size_t MPIDU_shm_get_mapsize(size_t size, size_t * psz);

/* MPIDU_shm_alloc_symm_all - collectively allocate and return symmetric shared memory
 *
 * - comm_ptr :  communicator pointer used for the collective call
 * - len      :  total length of memory requested by all processes in a node
 * - offset:  :  number of bytes used to shift the shared memory segment base pointer
 *               in a node with multiple processes.  The offset argument is different
 *               for every process in the node and depends on the location of process
 *               memory inside the segment. For example, in a node with 4 processes
 *               rank0 will pass `0`, rank1 will pass `len0`, rank2 will pass `len0 +
 *               len1`, and rank3 will pass `len0 + len1 + len2`.   As a result rank0
 *               gets `ptr=base_ptr`, rank1 gets `ptr=base_ptr - len0`, and so on.
 *               This way every process's memory inside the segment maps to the same
 *               virtual address, i.e., `rank0 = base_ptr`, `rank1 = base_ptr - len0 +
 *               len0`, and so on
 * - ptr      :  pointer to allocated memory. Memory can be either shared if the node
 *               has more than one process or private if the node has only one process.
 *               Either way, the value of ptr will be such that the base address of
 *               process memory will be symmetric across all processes in comm_ptr
 * - fail_flag:  if symmetric shared memory could not be allocated this flag is set to
 *               `true`, otherwise it is set to `false`
 *
 * The function returns MPI_SUCCESS if no error has happened while allocating memory.
 * Failure allocating symmetric memory is not considered an error. The function returns
 * MPI_ERR_OTHER if any other error occurred. */
int MPIDU_shm_alloc_symm_all(MPIR_Comm * comm_ptr, size_t len, size_t offset, void **ptr,
                             bool * fail_flag);

/* MPIDU_shm_alloc - collectively allocate and return shared memory
 *
 * - shm_comm_ptr:  node communicator used for the collective call
 * - len         :  total length of memory requested by all processes in a node
 * - ptr         :  pointer to allocated shared memory
 * - fail_flag   :  if shared memory could not be allocated this flag is set to `true`,
 *                  otherwise it is set to `false`
 *
 * The function returns MPI_SUCCESS if no error has happened while allocating memory.
 * Failure allocating shared memory is not considered an error. The function returns
 * MPI_ERR_OTHER if any other error occurred. */
int MPIDU_shm_alloc(MPIR_Comm * shm_comm_ptr, size_t len, void **ptr, bool * fail_flag);

/* MPIDU_shm_free - free memory allocated using either of the previous two methods
 *
 * - ptr: pointer to memory allocated using either `MPIDU_shm_alloc_symm_all` or
 *        `MPIDU_shm_alloc`
 *
 * The function returns MPI_SUCCESS if memory was freed correctly error otherwise. If
 * ptr points to memory that was not allocated using either `MPIDU_shm_alloc_symm_all`
 * or `MPIDU_shm_alloc` the function will assert and cause the MPI application to crash. */
int MPIDU_shm_free(void *ptr);

#endif /* MPIDU_SHM_H_INCLUDED */
