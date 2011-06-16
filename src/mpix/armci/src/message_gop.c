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

/* MPI Operations, registered in Init */
MPI_Op MPI_ABSMIN_OP;
MPI_Op MPI_ABSMAX_OP;

#define IABS(X)  (((X) > 0  ) ? X : -X)
#define FABS(X)  (((X) > 0.0) ? X : -X)
#define MIN(X,Y) (((X) < (Y)) ? X : Y)
#define MAX(X,Y) (((X) > (Y)) ? X : Y)

#define ABSMIN(IN,INOUT,COUNT,DTYPE,ABSOP)      \
      do {                                      \
        int i;                                  \
        DTYPE *in = (DTYPE *)IN;                \
        DTYPE *io = (DTYPE *)INOUT;             \
        for (i = 0; i < COUNT; i++) {           \
          const DTYPE x = ABSOP(in[i]);         \
          const DTYPE y = ABSOP(io[i]);         \
          io[i] = MIN(x,y);                     \
        }                                       \
      } while (0)

/** MPI reduction operator that computes the minimum absolute value.
  */
void ARMCII_Absmin_op(void *invec, void *inoutvec, int *len, MPI_Datatype *datatype) {
  const int    count = *len;
  MPI_Datatype dt    = *datatype;

  if (dt == MPI_INT) {
      ABSMIN(invec, inoutvec, count, int, IABS);
  } else if (dt == MPI_LONG) {
      ABSMIN(invec, inoutvec, count, long, IABS);
  } else if (dt == MPI_LONG_LONG) {
      ABSMIN(invec, inoutvec, count, long long, IABS);
  } else if (dt == MPI_FLOAT) {
      ABSMIN(invec, inoutvec, count, float, FABS);
  } else if (dt == MPI_DOUBLE) {
      ABSMIN(invec, inoutvec, count, double, FABS);
  } else {
      ARMCII_Error("unknown type (%d)", *datatype);
  }
}

#undef ABSMIN


#define ABSMAX(IN,INOUT,COUNT,DTYPE,ABSOP)      \
      do {                                      \
        int i;                                  \
        DTYPE *in = (DTYPE *)IN;                \
        DTYPE *io = (DTYPE *)INOUT;             \
        for (i = 0; i < COUNT; i++) {           \
          const DTYPE x = ABSOP(in[i]);         \
          const DTYPE y = ABSOP(io[i]);         \
          io[i] = MAX(x,y);                     \
        }                                       \
      } while (0)

/** MPI reduction operator that computes the maximum absolute value.
  */
void ARMCII_Absmax_op(void *invec, void *inoutvec, int *len, MPI_Datatype *datatype) {
  const int    count = *len;
  MPI_Datatype dt    = *datatype;

  if (dt == MPI_INT) {
      ABSMAX(invec, inoutvec, count, int, IABS);
  } else if (dt == MPI_LONG) {
      ABSMAX(invec, inoutvec, count, long, IABS);
  } else if (dt == MPI_LONG_LONG) {
      ABSMAX(invec, inoutvec, count, long long, IABS);
  } else if (dt == MPI_FLOAT) {
      ABSMAX(invec, inoutvec, count, float, FABS);
  } else if (dt == MPI_DOUBLE) {
      ABSMAX(invec, inoutvec, count, double, FABS);
  } else {
      ARMCII_Error("unknown type (%d)", *datatype);
  }
}

#undef ABSMAX


#define ABSV(IN,INOUT,COUNT,DTYPE,ABSOP)        \
      do {                                      \
        int i;                                  \
        DTYPE *in = (DTYPE *)IN;                \
        DTYPE *io = (DTYPE *)INOUT;             \
        for (i = 0; i < COUNT; i++)             \
          io[i] = ABSOP(in[i]);                 \
      } while (0)

/** Compute the absolute value.
  */
void ARMCII_Absv_op(void *invec, void *inoutvec, int *len, MPI_Datatype *datatype) {
  const int    count = *len;
  MPI_Datatype dt    = *datatype;

  if (dt == MPI_INT) {
      ABSV(invec, inoutvec, count, int, IABS);
  } else if (dt == MPI_LONG) {
      ABSV(invec, inoutvec, count, long, IABS);
  } else if (dt == MPI_LONG_LONG) {
      ABSV(invec, inoutvec, count, long long, IABS);
  } else if (dt == MPI_FLOAT) {
      ABSV(invec, inoutvec, count, float, FABS);
  } else if (dt == MPI_DOUBLE) {
      ABSV(invec, inoutvec, count, double, FABS);
  } else {
      ARMCII_Error("unknown type (%d)", *datatype);
  }
}

#undef ABSV


/** General ARMCI global operation (reduction).  Collective on group.
  *
  * @param[in]    scope Scope in which to perform the GOP (only SCOPE_ALL is supported)
  * @param[inout] x     Vector of n doubles, contains input and will contain output.
  * @param[in]    n     Length of x
  * @param[in]    op    One of '+', '*', 'max', 'min', 'absmax', 'absmin'
  * @param[in]    type  Data type of x
  * @param[in]    group Group on which to perform the GOP
  */
void armci_msg_group_gop_scope(int scope, void *x, int n, char *op, int type, ARMCI_Group *group) {
  void        *out;
  MPI_Op       mpi_op;
  MPI_Datatype mpi_type;
  MPI_Comm     comm;
  int          mpi_type_size;

  if (scope == SCOPE_ALL || scope == SCOPE_MASTERS)
    comm = group->comm;
  else
    comm = MPI_COMM_SELF;

  if (op[0] == '+') {
    mpi_op = MPI_SUM;
  } else if (op[0] == '*') {
    mpi_op = MPI_PROD;
  } else if (strncmp(op, "max", 3) == 0) {
    mpi_op = MPI_MAX;
  } else if (strncmp(op, "min", 3) == 0) {
    mpi_op = MPI_MIN;
  } else if (strncmp(op, "or", 2) == 0) {
    mpi_op = MPI_BOR;
  } else if (strncmp(op, "absmax", 6) == 0) {
    mpi_op = MPI_ABSMAX_OP;
  } else if (strncmp(op, "absmin", 6) == 0) {
    mpi_op = MPI_ABSMIN_OP;
  } else {
    ARMCII_Error("unknown operation \'%s\'", op);
    return;
  }

  switch(type) {
    case ARMCI_INT:
      mpi_type = MPI_INT;
      break;
    case ARMCI_LONG:
      mpi_type = MPI_LONG;
      break;
    case ARMCI_LONG_LONG:
      mpi_type = MPI_LONG_LONG;
      break;
    case ARMCI_FLOAT:
      mpi_type = MPI_FLOAT;
      break;
    case ARMCI_DOUBLE:
      mpi_type = MPI_DOUBLE;
      break;
    default:
      ARMCII_Error("unknown type (%d)", type);
      return;
  }

  // ABS MAX/MIN are unary as well as binary.  We need to also apply abs in the
  // single processor case when reduce would normally just be a no-op.
  if (group->size == 1 && (mpi_op == MPI_ABSMAX_OP || mpi_op == MPI_ABSMIN_OP)) {
    ARMCII_Absv_op(x, x, &n, &mpi_type);
    return;
  }

  MPI_Type_size(mpi_type, &mpi_type_size);

  out = malloc(n*mpi_type_size);
  ARMCII_Assert(out != NULL);

  MPI_Allreduce(x, out, n, mpi_type, mpi_op, group->comm);

  ARMCI_Copy(out, x, n*mpi_type_size);
  free(out);
}

void armci_msg_group_igop(int *x, int n, char *op, ARMCI_Group *group) {
  armci_msg_group_gop_scope(SCOPE_ALL, x, n, op, ARMCI_INT, group);
}

void armci_msg_group_lgop(long *x, int n, char *op, ARMCI_Group *group) {
  armci_msg_group_gop_scope(SCOPE_ALL, x, n, op, ARMCI_LONG, group);
}

void armci_msg_group_llgop(long long *x, int n, char *op, ARMCI_Group *group) {
  armci_msg_group_gop_scope(SCOPE_ALL, x, n, op, ARMCI_LONG_LONG, group);
}

void armci_msg_group_fgop(float *x, int n, char *op, ARMCI_Group *group) {
  armci_msg_group_gop_scope(SCOPE_ALL, x, n, op, ARMCI_FLOAT, group);
}

void armci_msg_group_dgop(double *x, int n, char *op, ARMCI_Group *group) {
  armci_msg_group_gop_scope(SCOPE_ALL, x, n, op, ARMCI_DOUBLE, group);
}

void armci_msg_gop_scope(int scope, void *x, int n, char *op, int type) {
  armci_msg_group_gop_scope(scope, x, n, op, type, &ARMCI_GROUP_WORLD);
}

void armci_msg_igop(int *x, int n, char *op) {
  armci_msg_gop_scope(SCOPE_ALL, x, n, op, ARMCI_INT);
}

void armci_msg_lgop(long *x, int n, char *op) {
  armci_msg_gop_scope(SCOPE_ALL, x, n, op, ARMCI_LONG);
}

void armci_msg_llgop(long long *x, int n, char *op) {
  armci_msg_gop_scope(SCOPE_ALL, x, n, op, ARMCI_LONG_LONG);
}

void armci_msg_fgop(float *x, int n, char *op) {
  armci_msg_gop_scope(SCOPE_ALL, x, n, op, ARMCI_FLOAT);
}

void armci_msg_dgop(double *x, int n, char *op) {
  armci_msg_gop_scope(SCOPE_ALL, x, n, op, ARMCI_DOUBLE);
}

