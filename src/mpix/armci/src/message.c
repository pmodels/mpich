/*
 * Copyright (C) 2010. See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include <debug.h>
#include <armci.h>
#include <armci_internals.h>

/** Query process rank from messaging (MPI) layer.
  */
int armci_msg_me(void) {
  int me;
  MPI_Comm_rank(ARMCI_GROUP_WORLD.comm, &me);
  return me;
}


/** Query number of processes.
  */
int armci_msg_nproc(void) {
  int nproc;
  MPI_Comm_size(ARMCI_GROUP_WORLD.comm, &nproc);
  return nproc;
}


/** Abort the application.
  *
  * @param[in] code Exit error code
  */
void armci_msg_abort(int code) {
  MPI_Abort(ARMCI_GROUP_WORLD.comm, code);
}


/** Get the wall clock time.
  *
  * @return Wall clock time
  */
double armci_timer(void) {
  return MPI_Wtime();
}


/** Broadcast a message.  Collective.
  *
  * @param[in] buffer Source buffer on root, destination elsewhere.
  * @param[in] len    Length of the message in bytes.
  * @param[in] root   Rank of the root process.
  */
void armci_msg_bcast(void *buffer, int len, int root) {
  MPI_Bcast(buffer, len, MPI_BYTE, root, ARMCI_GROUP_WORLD.comm);
}


/** Broadcast a message.  Collective.
  *
  * @param[in] buffer Source buffer on root, destination elsewhere.
  * @param[in] len    Length of the message in bytes.
  * @param[in] root   Rank of the root process.
  */
void armci_msg_brdcst(void *buffer, int len, int root) {
  MPI_Bcast(buffer, len, MPI_BYTE, root, ARMCI_GROUP_WORLD.comm);
}


/** Broadcast a message on the given scope.  Collective.
  *
  * @param[in] scope  Scope for the broadcast
  * @param[in] buffer Source buffer on root, destination elsewhere.
  * @param[in] len    Length of the message in bytes.
  * @param[in] root   Rank of the root process.
  */
void armci_msg_bcast_scope(int scope, void *buffer, int len, int root) {
  armci_msg_group_bcast_scope(scope, buffer, len, root, &ARMCI_GROUP_WORLD);
}


/** Barrier from the messaging layer.
  */
void armci_msg_barrier(void) {
  MPI_Barrier(ARMCI_GROUP_WORLD.comm);
}


/** Message barrier on a group.
  *
  * @param[in] group Group on which to perform barrier
  */
void armci_msg_group_barrier(ARMCI_Group *group) {
  MPI_Barrier(group->comm);
}


/** Broadcast on a group. Collective.
  *
  * @param[in]    scope ARMCI scope
  * @param[inout] buf   Input on the root, output on all other processes
  * @param[in]    len   Number of bytes in the message
  * @param[in]    abs_root Absolute rank of the process at the root of the broadcast
  * @param[in]    group ARMCI group on which to perform communication
  */
void armci_msg_group_bcast_scope(int scope, void *buf, int len, int abs_root, ARMCI_Group *group) {
  int grp_root;

  if (scope == SCOPE_ALL || scope == SCOPE_MASTERS) {
    grp_root = ARMCII_Translate_absolute_to_group(group->comm, abs_root);
    ARMCII_Assert(grp_root >= 0 && grp_root < group->size);

    MPI_Bcast(buf, len, MPI_BYTE, grp_root, group->comm);

  } else /* SCOPE_NODE */ {
    // grp_root = ARMCII_Translate_absolute_to_group(MPI_COMM_SELF, abs_root);
    // ARMCII_Assert(grp_root >= 0 && grp_root < group->size);
    grp_root = 0;

    MPI_Bcast(buf, len, MPI_BYTE, grp_root, MPI_COMM_SELF);
  }
}


/** Send a two-sided message.
  *
  * @param[in] tag    Message tag (must match on sender and receiver)
  * @param[in] buf    Buffer containing the message
  * @param[in] nbytes Length of the message in bytes
  * @param[in] dest   Destination process id
  */
void armci_msg_snd(int tag, void *buf, int nbytes, int dest) {
  MPI_Send(buf, nbytes, MPI_BYTE, dest, tag, ARMCI_GROUP_WORLD.comm);
}


/** Receive a two-sided message.
  *
  * @param[in]  tag    Message tag (must match on sender and receiver)
  * @param[in]  buf    Buffer containing the message
  * @param[in]  nbytes_buf Size of the buffer in bytes
  * @param[out] nbytes_msg Length of the message received in bytes (NULL to ignore)
  * @param[in]  src    Source process id
  */
void armci_msg_rcv(int tag, void *buf, int nbytes_buf, int *nbytes_msg, int src) {
  MPI_Status status;
  MPI_Recv(buf, nbytes_buf, MPI_BYTE, src, tag, ARMCI_GROUP_WORLD.comm, &status);

  if (nbytes_msg != NULL)
    MPI_Get_count(&status, MPI_BYTE, nbytes_msg);
}


/** Receive a two-sided message from any source.
  *
  * @param[in]  tag    Message tag (must match on sender and receiver)
  * @param[in]  buf    Buffer containing the message
  * @param[in]  nbytes_buf Size of the buffer in bytes
  * @param[out] nbytes_msg Length of the message received in bytes (NULL to ignore)
  * @return            Rank of the message source
  */
int armci_msg_rcvany(int tag, void *buf, int nbytes_buf, int *nbytes_msg) {
  MPI_Status status;
  MPI_Recv(buf, nbytes_buf, MPI_BYTE, MPI_ANY_SOURCE, tag, ARMCI_GROUP_WORLD.comm, &status);

  if (nbytes_msg != NULL)
    MPI_Get_count(&status, MPI_BYTE, nbytes_msg);

  return status.MPI_SOURCE;
}


void armci_msg_reduce(void *x, int n, char *op, int type) {
  armci_msg_reduce_scope(SCOPE_ALL, x, n, op, type);
}


void armci_msg_reduce_scope(int scope, void *x, int n, char *op, int type) {
  ARMCII_Error("unimplemented"); // TODO
}


/** Map process IDs onto a binary tree.
  *
  * @param[in]  scope Scope of processes involved
  * @param[out] root  Process id of the root
  * @param[out] up    Process id of my parent
  * @param[out] left  Process id of my left child
  * @param[out] right Process if of my right child
  */
void armci_msg_bintree(int scope, int *root, int *up, int *left, int *right) {
  int me, nproc;

  if (scope == SCOPE_NODE) {
    *root  = 0;
    *left  = -1;
    *right = -1;
   
    return;
  }

  me    = armci_msg_me();
  nproc = armci_msg_nproc();

  *root = 0;
  *up   =  (me == 0) ? -1 : (me - 1) / 2;

  *left = 2*me + 1;
  if (*left >= nproc) *left = -1;

  *right = 2*me + 2;
  if (*right >= nproc) *right = -1;
}


/** Data packet for a select operation.  Data entry is a struct from GA's
  * runtime where the first element is a value and later parts are indices of
  * the element in the GA.
  */
typedef struct {
  int     contribute;
  int     type;
  uint8_t data[1];
} sel_data_t;


/** Select operations to be used in allreduce.
  */
MPI_Op MPI_SELMIN_OP;
MPI_Op MPI_SELMAX_OP;


/** Min operator for armci_msg_sel
  */
void ARMCII_Msg_sel_min_op(void *data_in, void *data_inout, int *len, MPI_Datatype *datatype) {
  sel_data_t *sd_1, *sd_2;

  sd_1 = (sel_data_t*) data_in;
  sd_2 = (sel_data_t*) data_inout;

  if (sd_1->contribute && !sd_2->contribute) {
    ARMCI_Copy(data_in, data_inout, *len);
  }
  
  else if (sd_1->contribute && sd_2->contribute) {

#define MSG_SEL_MIN_OP(X,Y,LEN,TYPE)                                      \
  do {                                                                    \
    if (*(TYPE*)((sel_data_t*)X)->data < *(TYPE*)((sel_data_t*)Y)->data)  \
      ARMCI_Copy(X, Y, LEN);                                              \
  } while (0)

    switch (sd_1->type) {
      case ARMCI_INT:
        MSG_SEL_MIN_OP(data_in, data_inout, *len, int);
        break;
      case ARMCI_LONG:
        MSG_SEL_MIN_OP(data_in, data_inout, *len, long);
        break;
      case ARMCI_LONG_LONG:
        MSG_SEL_MIN_OP(data_in, data_inout, *len, long long);
        break;
      case ARMCI_FLOAT:
        MSG_SEL_MIN_OP(data_in, data_inout, *len, float);
        break;
      case ARMCI_DOUBLE:
        MSG_SEL_MIN_OP(data_in, data_inout, *len, double);
        break;
      default:
        ARMCII_Error("Invalid data type (%d)", sd_1->type);
    }

#undef MSG_SEL_MIN_OP
  }

  /* else: no need to copy, data_inout already contains what we want to return */
}


/** Min operator for armci_msg_sel
  */
void ARMCII_Msg_sel_max_op(void *data_in, void *data_inout, int *len, MPI_Datatype *datatype) {
  sel_data_t *sd_1, *sd_2;

  sd_1 = (sel_data_t*) data_in;
  sd_2 = (sel_data_t*) data_inout;

  if (sd_1->contribute && !sd_2->contribute) {
    ARMCI_Copy(data_in, data_inout, *len);
  }
  
  else if (sd_1->contribute && sd_2->contribute) {

#define MSG_SEL_MAX_OP(X,Y,LEN,TYPE)                                      \
  do {                                                                    \
    if (*(TYPE*)((sel_data_t*)X)->data > *(TYPE*)((sel_data_t*)Y)->data)  \
      ARMCI_Copy(X, Y, LEN);                                              \
  } while (0)

    switch (sd_1->type) {
      case ARMCI_INT:
        MSG_SEL_MAX_OP(data_in, data_inout, *len, int);
        break;
      case ARMCI_LONG:
        MSG_SEL_MAX_OP(data_in, data_inout, *len, long);
        break;
      case ARMCI_LONG_LONG:
        MSG_SEL_MAX_OP(data_in, data_inout, *len, long long);
        break;
      case ARMCI_FLOAT:
        MSG_SEL_MAX_OP(data_in, data_inout, *len, float);
        break;
      case ARMCI_DOUBLE:
        MSG_SEL_MAX_OP(data_in, data_inout, *len, double);
        break;
      default:
        ARMCII_Error("Invalid data type (%d)", sd_1->type);
    }

#undef MSG_SEL_MIN_OP
  }

  /* else: no need to copy, data_inout already contains what we want to return */
}


/** Collective index selection reduce operation.
  */
void armci_msg_sel(void *x, int n, char *op, int type, int contribute) {
  armci_msg_sel_scope(SCOPE_ALL, x, n, op, type, contribute);
}


/** Collective index selection reduce operation (scoped).
  */
void armci_msg_sel_scope(int scope, void *x, int n, char* op, int type, int contribute) {
  MPI_Comm    sel_comm;
  sel_data_t *data_in, *data_out;

  /*
  printf("[%d] armci_msg_sel_scope(scope=%d, x=%p, n=%d, op=%s, type=%d, contribute=%d)\n",
      ARMCI_GROUP_WORLD.rank, scope, x, n, op, type, contribute);
  */

  /* Determine the scope of the collective operation */
  if (scope == SCOPE_ALL || scope == SCOPE_MASTERS)
    sel_comm = ARMCI_GROUP_WORLD.comm;
  else
    sel_comm = MPI_COMM_SELF;

  data_in  = malloc(sizeof(sel_data_t)+n-1);
  data_out = malloc(sizeof(sel_data_t)+n-1);

  ARMCII_Assert(data_in != NULL && data_out != NULL);

  data_in->contribute = contribute;
  data_in->type       = type;

  if (contribute)
    ARMCI_Copy(x, data_in->data, n);

  if (strncmp(op, "min", 3) == 0) {
    MPI_Allreduce(data_in, data_out, sizeof(sel_data_t)+n-1, MPI_BYTE, MPI_SELMIN_OP, sel_comm);
  } else if (strncmp(op, "max", 3) == 0) {
    MPI_Allreduce(data_in, data_out, sizeof(sel_data_t)+n-1, MPI_BYTE, MPI_SELMAX_OP, sel_comm);
  } else {
      ARMCII_Error("Invalid operation (%s)", op);
  }

  ARMCI_Copy(data_out->data, x, n);

  free(data_in);
  free(data_out);
}
