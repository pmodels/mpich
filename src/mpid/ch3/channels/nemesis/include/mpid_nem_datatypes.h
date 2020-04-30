/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPID_NEM_DATATYPES_H_INCLUDED
#define MPID_NEM_DATATYPES_H_INCLUDED

#include "mpid_nem_debug.h"
#include "mpid_nem_atomics.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
    #include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
    #include <unistd.h>
#endif
#include <string.h>
#include <limits.h>
#ifdef HAVE_SCHED_H
    #include <sched.h>
#endif
#include "mpichconf.h"

/* FIXME We are using this as an interprocess lock in the queue code, although
   it's not strictly guaranteed to work for this scenario.  These should really
   use the "process locks" code, but it's in such terrible shape that it doesn't
   really work for us right now.  Also, because it has some inline assembly it's
   not really a fair comparison for studying the impact of atomic instructions.
   [goodell@ 2009-01-16] */

#define MPID_NEM_OFFSETOF(struc, field) ((int)(&((struc *)0)->field))
#define MPID_NEM_CACHE_LINE_LEN MPL_CACHELINE_SIZE
#define MPID_NEM_NUM_CELLS      64
#define MPID_NEM_CELL_LEN       (64*1024)

/*
   The layout of the cell looks like this:

   --CELL------------------
   | next                 |
   | padding              |
   | --MPICH PKT-------- |
   | | packet headers   | |
   | | packet payload   | |
   | |   .              | |
   | |   .              | |
   | |   .              | |
   | |                  | |
   | -------------------- |
   ------------------------

   For optimization, we want the cell to start at a cacheline boundary
   and the cell length to be a multiple of cacheline size.  This will
   avoid false sharing.  We also want payload to start at an 8-byte
   boundary to to optimize memcpys and datatype operations on the
   payload.  To ensure payload is 8-byte aligned, we add aligned attribute
   to the pkt field.

   Forgive the misnomers of the macros.

   MPID_NEM_CELL_LEN size of the whole cell (currently 64K)
   
   MPID_NEM_CELL_HEAD_LEN is the size of the next pointer plus the
       padding.
   
   MPID_NEM_CELL_PAYLOAD_LEN is the maximum length of the packet.
       This is MPID_NEM_CELL_LEN minus the size of the next pointer
       and any padding.

   MPID_NEM_MPICH_HEAD_LEN is the length of the mpich packet header
       fields.

   MPID_NEM_MPICH_DATA_LEN is the maximum length of the mpich packet
       payload and is basically what's left after the next pointer,
       padding and packet header.  This is MPID_NEM_CELL_PAYLOAD_LEN -
       MPID_NEM_MPICH_HEAD_LEN.

   MPID_NEM_CALC_CELL_LEN is the amount of data plus headers in the
       cell.  I.e., how much of a cell would need to be sent over a
       network.
*/

#define MPID_NEM_CELL_HEAD_LEN    offsetof(MPID_nem_cell_t, pkt)
#define MPID_NEM_CELL_PAYLOAD_LEN (MPID_NEM_CELL_LEN - MPID_NEM_CELL_HEAD_LEN)

#define MPID_NEM_CALC_CELL_LEN(cellp) (MPID_NEM_CELL_HEAD_LEN + MPID_NEM_MPICH_HEAD_LEN + MPID_NEM_CELL_DLEN (cell))

#define MPID_NEM_ALIGNED(addr, bytes) ((((unsigned long)addr) & (((unsigned long)bytes)-1)) == 0)

#define MPID_NEM_PKT_UNKNOWN     0
#define MPID_NEM_PKT_MPICH      1
#define MPID_NEM_PKT_MPICH_HEAD 2

#define MPID_NEM_FBOX_SOURCE(cell) (MPID_nem_mem_region.local_procs[(cell)->pkt.header.source])
#define MPID_NEM_CELL_SOURCE(cell) ((cell)->pkt.header.source)
#define MPID_NEM_CELL_DEST(cell)   ((cell)->pkt.header.dest)
#define MPID_NEM_CELL_DLEN(cell)   ((cell)->pkt.header.datalen)
#define MPID_NEM_CELL_SEQN(cell)   ((cell)->pkt.header.seqno)

#define MPID_NEM_MPICH_HEAD_LEN sizeof(MPID_nem_pkt_header_t)
#define MPID_NEM_MPICH_DATA_LEN (MPID_NEM_CELL_PAYLOAD_LEN - MPID_NEM_MPICH_HEAD_LEN)

#define MPID_NEM_PKT_HEADER_FIELDS   	    \

typedef struct MPID_nem_pkt_header
{
    int source;
    int dest;
    intptr_t datalen;
    unsigned short seqno;
    unsigned short type; /* currently used only with checkpointing */
} MPID_nem_pkt_header_t;

typedef struct MPID_nem_pkt
{
    MPID_nem_pkt_header_t header;
    char payload[] MPL_ATTR_ALIGNED(8);   /* C99 flexible array member with 8-byte alignment */
} MPID_nem_pkt_t;

/* Nemesis cells which are to be used in shared memory need to use
 * "relative pointers" because the absolute pointers to a cell from
 * different processes may be different.  Relative pointers are
 * offsets from the beginning of the mmapped region where they live.
 * We use different types for relative and absolute pointers to help
 * catch errors.  Use MPID_NEM_REL_TO_ABS and MPID_NEM_ABS_TO_REL to
 * convert between relative and absolute pointers. */

/* This should always be exactly the size of a pointer */
typedef struct MPID_nem_cell_rel_ptr
{
    MPL_atomic_ptr_t p;
}
MPID_nem_cell_rel_ptr_t;

/* NOTE: for a queue local to a single process (e.g. a queue in a network module),
 * relative pointers are not needed, and we could use a direct pointer. This
 * optimization is left as todo when needed.
 */

typedef struct MPID_nem_cell
{
    MPID_nem_cell_rel_ptr_t next;
    volatile MPID_nem_pkt_t pkt MPL_ATTR_ALIGNED(8);
} MPID_nem_cell_t;
typedef MPID_nem_cell_t *MPID_nem_cell_ptr_t;

typedef struct MPID_nem_queue
{
    MPID_nem_cell_rel_ptr_t head;
    MPID_nem_cell_rel_ptr_t tail;
    char padding1[MPID_NEM_CACHE_LINE_LEN - 2 * sizeof(MPID_nem_cell_rel_ptr_t)];
    MPID_nem_cell_rel_ptr_t my_head;
    char padding2[MPID_NEM_CACHE_LINE_LEN - sizeof(MPID_nem_cell_rel_ptr_t)];
} MPID_nem_queue_t, *MPID_nem_queue_ptr_t;

/* Fast Boxes*/ 
typedef union
{
    MPL_atomic_int_t value;
#if MPID_NEM_CACHE_LINE_LEN != 0
    char padding[MPID_NEM_CACHE_LINE_LEN];
#endif
}
MPID_nem_opt_volint_t;

typedef struct MPID_nem_fbox_common
{
    MPID_nem_opt_volint_t  flag;
} MPID_nem_fbox_common_t, *MPID_nem_fbox_common_ptr_t;

typedef struct MPID_nem_fbox_mpich
{
    MPID_nem_opt_volint_t flag;
    MPID_nem_cell_t cell;
} MPID_nem_fbox_mpich_t;

#define MPID_NEM_FBOX_LEN     (MPID_NEM_CELL_LEN + offsetof(MPID_nem_fbox_mpich_t, cell))
#define MPID_NEM_FBOX_DATALEN MPID_NEM_MPICH_DATA_LEN

typedef union 
{
    MPID_nem_fbox_common_t common;
    MPID_nem_fbox_mpich_t mpich;
} MPID_nem_fastbox_t;


typedef struct MPID_nem_fbox_arrays
{
    MPID_nem_fastbox_t **in;
    MPID_nem_fastbox_t **out;
} MPID_nem_fbox_arrays_t;

#endif /* MPID_NEM_DATATYPES_H_INCLUDED */
