/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#ifndef HAVE_ARMCI_INTERNALS_H
#define HAVE_ARMCI_INTERNALS_H

#include <armci.h>
#include <armciconf.h>

#if   HAVE_STDINT_H
#  include <stdint.h>
#elif HAVE_INTTYPES_H
#  include <inttypes.h>
#endif

/* Likely/Unlikely macros borrowed from MPICH2:
 */

/* These likely/unlikely macros provide static branch prediction hints to the
 * compiler, if such hints are available.  Simply wrap the relevant expression in
 * the macro, like this:
 *
 * if (unlikely(ptr == NULL)) {
 *     // ... some unlikely code path ...
 * }
 *
 * They should be used sparingly, especially in upper-level code.  It's easy to
 * incorrectly estimate branching likelihood, while the compiler can often do a
 * decent job if left to its own devices.
 *
 * These macros are not namespaced because the namespacing is cumbersome.
 */
/* safety guard for now, add a configure check in the future */
#if defined(__GNUC__) && (__GNUC__ >= 3)
#  define unlikely(x_) __builtin_expect(!!(x_),0)
#  define likely(x_)   __builtin_expect(!!(x_),1)
#else
#  define unlikely(x_) (x_)
#  define likely(x_)   (x_)
#endif


/* Disable safety checks if the user asks for it */

#ifdef NO_SEATBELTS
#define NO_CHECK_OVERLAP /* Disable checks for overlapping IOV operations */
//#define NO_USE_CTREE     /* Use the slower O(N) check instead of the conflict tree */
#define NO_CHECK_BUFFERS /* Disable checking for shared origin buffers    */

#else
#endif

/* Internal types */

enum ARMCII_Op_e { ARMCII_OP_PUT, ARMCII_OP_GET, ARMCII_OP_ACC };

enum ARMCII_Strided_methods_e { ARMCII_STRIDED_IOV, ARMCII_STRIDED_DIRECT };

enum ARMCII_Iov_methods_e { ARMCII_IOV_AUTO, ARMCII_IOV_CONSRV,
                            ARMCII_IOV_BATCHED, ARMCII_IOV_DIRECT };

enum ARMCII_Shr_buf_methods_e { ARMCII_SHR_BUF_COPY, ARMCII_SHR_BUF_NOGUARD };

extern char ARMCII_Strided_methods_str[][10];
extern char ARMCII_Iov_methods_str[][10];
extern char ARMCII_Shr_buf_methods_str[][10];

typedef struct {
  int           init_count;             /* Number of times ARMCI_Init has been called                           */
  int           debug_alloc;            /* Do extra debuggin on memory allocation                               */
  int           debug_flush_barriers;   /* Flush all windows on a barrier                                       */
  int           iov_checks;             /* Disable IOV same allocation and overlapping checks                   */
  int           iov_batched_limit;      /* Max number of ops per epoch for BATCHED IOV method                   */
  int           noncollective_groups;   /* Use noncollective group creation algorithm                           */
  int           cache_rank_translation; /* Enable caching of translation between absolute and group ranks       */
  int           verbose;                /* ARMCI should produce extra status output                             */

  enum ARMCII_Strided_methods_e strided_method; /* Strided transfer method              */
  enum ARMCII_Iov_methods_e     iov_method;     /* IOV transfer method                  */
  enum ARMCII_Shr_buf_methods_e shr_buf_method; /* Shared buffer management method      */
} global_state_t;


/* Global data */

extern ARMCI_Group    ARMCI_GROUP_WORLD;
extern ARMCI_Group    ARMCI_GROUP_DEFAULT;
extern MPI_Op         MPI_ABSMIN_OP;
extern MPI_Op         MPI_ABSMAX_OP;
extern MPI_Op         MPI_SELMIN_OP;
extern MPI_Op         MPI_SELMAX_OP;
extern global_state_t ARMCII_GLOBAL_STATE;
  

/* Utility functions */

void  ARMCII_Bzero(void *buf, int size);
int   ARMCII_Log2(unsigned int val);
char *ARMCII_Getenv(char *varname);
int   ARMCII_Getenv_bool(char *varname, int default_value);
int   ARMCII_Getenv_int(char *varname, int default_value);

/* Synchronization */

void ARMCII_Flush_local(void);

/* GOP Operators */

void ARMCII_Absmin_op(void *invec, void *inoutvec, int *len, MPI_Datatype *datatype);
void ARMCII_Absmax_op(void *invec, void *inoutvec, int *len, MPI_Datatype *datatype);
void ARMCII_Absv_op(void *invec, void *inoutvec, int *len, MPI_Datatype *datatype);
void ARMCII_Msg_sel_min_op(void *data_in, void *data_inout, int *len, MPI_Datatype *datatype);
void ARMCII_Msg_sel_max_op(void *data_in, void *data_inout, int *len, MPI_Datatype *datatype);


/* Translate between ARMCI and MPI ranks */

int  ARMCII_Translate_absolute_to_group(ARMCI_Group *group, int world_rank);


/* I/O Vector data management and implementation */

/** ARMCI IOV Iterator 
  */
typedef struct {
  /* Strided Representation */
  void *src;
  void *dst;
  int   stride_levels;

  int  *base_ptr;
  int  *src_stride_ar;
  int  *dst_stride_ar;
  int  *count;

  /* Iterator State */
  int   was_contiguous;
  int  *idx;
} armcii_iov_iter_t;

void ARMCII_Acc_type_translate(int armci_datatype, MPI_Datatype *type, int *type_size);

int  ARMCII_Iov_check_overlap(void **ptrs, int count, int size);
int  ARMCII_Iov_check_same_allocation(void **ptrs, int count, int proc);

void ARMCII_Strided_to_iov(armci_giov_t *iov,
               void *src_ptr, int src_stride_ar[/*stride_levels*/],
               void *dst_ptr, int dst_stride_ar[/*stride_levels*/], 
               int count[/*stride_levels+1*/], int stride_levels);

int ARMCII_Iov_op_dispatch(enum ARMCII_Op_e op, void **src, void **dst, int count, int size,
    int datatype, int overlapping, int same_alloc, int proc);

int ARMCII_Iov_op_safe(enum ARMCII_Op_e op, void **src, void **dst, int count, int elem_count,
    MPI_Datatype type, int proc);
int ARMCII_Iov_op_batched(enum ARMCII_Op_e op, void **src, void **dst, int count, int elem_count,
    MPI_Datatype type, int proc);
int ARMCII_Iov_op_datatype(enum ARMCII_Op_e op, void **src, void **dst, int count, int elem_count,
    MPI_Datatype type, int proc);

armcii_iov_iter_t *ARMCII_Strided_to_iov_iter(
               void *src_ptr, int src_stride_ar[/*stride_levels*/],
               void *dst_ptr, int dst_stride_ar[/*stride_levels*/], 
               int count[/*stride_levels+1*/], int stride_levels);
void ARMCII_Iov_iter_free(armcii_iov_iter_t *it);
int  ARMCII_Iov_iter_has_next(armcii_iov_iter_t *it);
int  ARMCII_Iov_iter_next(armcii_iov_iter_t *it, void **src, void **dst);


/* Shared to private buffer management routines */

int  ARMCII_Buf_prepare_read_vec(void **orig_bufs, void ***new_bufs_ptr, int count, int size);
void ARMCII_Buf_finish_read_vec(void **orig_bufs, void **new_bufs, int count, int size);
int  ARMCII_Buf_prepare_acc_vec(void **orig_bufs, void ***new_bufs_ptr, int count, int size,
                            int datatype, void *scale);
void ARMCII_Buf_finish_acc_vec(void **orig_bufs, void **new_bufs, int count, int size);
int  ARMCII_Buf_prepare_write_vec(void **orig_bufs, void ***new_bufs_ptr, int count, int size);
void ARMCII_Buf_finish_write_vec(void **orig_bufs, void **new_bufs, int count, int size);

int  ARMCII_Buf_acc_is_scaled(int datatype, void *scale);
void ARMCII_Buf_acc_scale(void *buf_in, void *buf_out, int size, int datatype, void *scale);

#endif /* HAVE_ARMCI_INTERNALS_H */
