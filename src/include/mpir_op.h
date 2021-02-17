/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_OP_H_INCLUDED
#define MPIR_OP_H_INCLUDED

/*E
  MPIR_Op_kind - Enumerates types of MPI_Op types

  Notes:
  These are needed for implementing 'MPI_Accumulate', since only predefined
  operations are allowed for that operation.

  A gap in the enum values was made allow additional predefined operations
  to be inserted.  This might include future additions to MPI or experimental
  extensions (such as a Read-Modify-Write operation).

  Module:
  Collective-DS
  E*/
typedef enum MPIR_Op_kind {
    MPIR_OP_KIND__NULL = 0,
    MPIR_OP_KIND__MAX = 1,
    MPIR_OP_KIND__MIN = 2,
    MPIR_OP_KIND__SUM = 3,
    MPIR_OP_KIND__PROD = 4,
    MPIR_OP_KIND__LAND = 5,
    MPIR_OP_KIND__BAND = 6,
    MPIR_OP_KIND__LOR = 7,
    MPIR_OP_KIND__BOR = 8,
    MPIR_OP_KIND__LXOR = 9,
    MPIR_OP_KIND__BXOR = 10,
    MPIR_OP_KIND__MAXLOC = 11,
    MPIR_OP_KIND__MINLOC = 12,
    MPIR_OP_KIND__REPLACE = 13,
    MPIR_OP_KIND__NO_OP = 14,
    MPIR_OP_KIND__EQUAL = 15,
    MPIR_OP_KIND__USER_NONCOMMUTE = 32,
    MPIR_OP_KIND__USER = 33
} MPIR_Op_kind;

/*S
  MPIR_User_function - Definition of a user function for MPI_Op types.

  Notes:
  This includes a 'const' to make clear which is the 'in' argument and
  which the 'inout' argument, and to indicate that the 'count' and 'datatype'
  arguments are unchanged (they are addresses in an attempt to allow
  interoperation with Fortran).  It includes 'restrict' to emphasize that
  no overlapping operations are allowed.

  We need to include a Fortran version, since those arguments will
  have type 'MPI_Fint *' instead.  We also need to add a test to the
  test suite for this case; in fact, we need tests for each of the handle
  types to ensure that the transfered handle works correctly.

  This is part of the collective module because user-defined operations
  are valid only for the collective computation routines and not for
  RMA accumulate.

  Yes, the 'restrict' is in the correct location.  C compilers that
  support 'restrict' should be able to generate code that is as good as a
  Fortran compiler would for these functions.

  We should note on the manual pages for user-defined operations that
  'restrict' should be used when available, and that a cast may be
  required when passing such a function to 'MPI_Op_create'.

  Question:
  Should each of these function types have an associated typedef?

  Should there be a C++ function here?

  Module:
  Collective-DS
  S*/
typedef union MPIR_User_function {
    void (*c_function) (const void *, void *, const int *, const MPI_Datatype *);
    void (*f77_function) (const void *, void *, const MPI_Fint *, const MPI_Fint *);
} MPIR_User_function;
/* FIXME: Should there be "restrict" in the definitions above, e.g.,
   (*c_function)(const void restrict * , void restrict *, ...)? */

/*S
  MPIR_Op - MPI_Op structure

  Notes:
  All of the predefined functions are commutative.  Only user functions may
  be noncummutative, so there are two separate op types for commutative and
  non-commutative user-defined operations.

  Operations do not require reference counts because there are no nonblocking
  operations that accept user-defined operations.  Thus, there is no way that
  a valid program can free an 'MPI_Op' while it is in use.

  Module:
  Collective-DS
  S*/
typedef struct MPIR_Op {
    MPIR_OBJECT_HEADER;         /* adds handle and ref_count fields */
    MPIR_Op_kind kind;
    MPIR_Lang_t language;
    MPIR_User_function function;
#ifdef MPID_DEV_OP_DECL
     MPID_DEV_OP_DECL
#endif
} MPIR_Op;
#define MPIR_OP_N_BUILTIN 15
extern MPIR_Op MPIR_Op_builtin[MPIR_OP_N_BUILTIN];
extern MPIR_Op MPIR_Op_direct[];
extern MPIR_Object_alloc_t MPIR_Op_mem;

#define MPIR_Op_ptr_add_ref(op_p_) \
    do { MPIR_Object_add_ref(op_p_); } while (0)
#define MPIR_Op_ptr_release_ref(op_p_, inuse_) \
    do { MPIR_Object_release_ref(op_p_, inuse_); } while (0)

/* release and free-if-not-in-use helper */
#define MPIR_Op_ptr_release(op_p_)                       \
    do {                                                 \
        int in_use_;                                     \
        MPIR_Op_ptr_release_ref((op_p_), &in_use_);      \
        if (!in_use_) {                                  \
            MPIR_Handle_obj_free(&MPIR_Op_mem, (op_p_)); \
        }                                                \
    } while (0)


/* Query index of builtin op */
MPL_STATIC_INLINE_PREFIX int MPIR_Op_builtin_get_index(MPI_Op op)
{
    MPIR_Assert(HANDLE_IS_BUILTIN(op));
    return (0x000000ff & op) - 1;       /* index 1 to 14 in handle. */
}

/* Query builtin op by using index (from 0 to MPIR_OP_N_BUILTIN-1) */
MPL_STATIC_INLINE_PREFIX MPI_Op MPIR_Op_builtin_get_op(int index)
{
    MPIR_Assert(index >= 0 && index < MPIR_OP_N_BUILTIN);
    return (MPI_Op) (0x58000000 | (index + 1)); /* index 1 to 14 in handle */
}

MPI_Datatype MPIR_Op_builtin_search_by_shortname(const char *short_name);
const char *MPIR_Op_builtin_get_shortname(MPI_Op op);

void MPIR_MAXF(void *, void *, int *, MPI_Datatype *);
void MPIR_MINF(void *, void *, int *, MPI_Datatype *);
void MPIR_SUM(void *, void *, int *, MPI_Datatype *);
void MPIR_PROD(void *, void *, int *, MPI_Datatype *);
void MPIR_LAND(void *, void *, int *, MPI_Datatype *);
void MPIR_BAND(void *, void *, int *, MPI_Datatype *);
void MPIR_LOR(void *, void *, int *, MPI_Datatype *);
void MPIR_BOR(void *, void *, int *, MPI_Datatype *);
void MPIR_LXOR(void *, void *, int *, MPI_Datatype *);
void MPIR_BXOR(void *, void *, int *, MPI_Datatype *);
void MPIR_MAXLOC(void *, void *, int *, MPI_Datatype *);
void MPIR_MINLOC(void *, void *, int *, MPI_Datatype *);
void MPIR_REPLACE(void *, void *, int *, MPI_Datatype *);
void MPIR_NO_OP(void *, void *, int *, MPI_Datatype *);
void MPIR_EQUAL(void *, void *, int *, MPI_Datatype *);

int MPIR_MAXF_check_dtype(MPI_Datatype);
int MPIR_MINF_check_dtype(MPI_Datatype);
int MPIR_SUM_check_dtype(MPI_Datatype);
int MPIR_PROD_check_dtype(MPI_Datatype);
int MPIR_LAND_check_dtype(MPI_Datatype);
int MPIR_BAND_check_dtype(MPI_Datatype);
int MPIR_LOR_check_dtype(MPI_Datatype);
int MPIR_BOR_check_dtype(MPI_Datatype);
int MPIR_LXOR_check_dtype(MPI_Datatype);
int MPIR_BXOR_check_dtype(MPI_Datatype);
int MPIR_MAXLOC_check_dtype(MPI_Datatype);
int MPIR_MINLOC_check_dtype(MPI_Datatype);
int MPIR_REPLACE_check_dtype(MPI_Datatype);
int MPIR_NO_OP_check_dtype(MPI_Datatype);
int MPIR_EQUAL_check_dtype(MPI_Datatype);

#define MPIR_Op_add_ref_if_not_builtin(op)               \
    do {                                                 \
        if (!HANDLE_IS_BUILTIN((op))) {\
            MPIR_Op *op_ptr = NULL;                      \
            MPIR_Op_get_ptr(op, op_ptr);                 \
            MPIR_Assert(op_ptr != NULL);                 \
            MPIR_Op_ptr_add_ref(op_ptr);                 \
        }                                                \
    } while (0)                                          \


#define MPIR_Op_release_if_not_builtin(op)               \
    do {                                                 \
        if (!HANDLE_IS_BUILTIN((op))) {\
            MPIR_Op *op_ptr = NULL;                      \
            MPIR_Op_get_ptr(op, op_ptr);                 \
            MPIR_Assert(op_ptr != NULL);                 \
            MPIR_Op_ptr_release(op_ptr);                 \
        }                                                \
    } while (0)                                          \

extern MPI_User_function *MPIR_Op_table[];

typedef int (MPIR_Op_check_dtype_fn) (MPI_Datatype);
extern MPIR_Op_check_dtype_fn *MPIR_Op_check_dtype_table[];

#define MPIR_OP_HDL_TO_FN(op) MPIR_Op_table[((op)&0xf)]
#define MPIR_OP_HDL_TO_DTYPE_FN(op) MPIR_Op_check_dtype_table[((op)&0xf)]

int MPIR_Op_commutative(MPIR_Op * op_ptr, int *commute);

int MPIR_Op_is_commutative(MPI_Op);

int MPIR_Op_create_impl(MPI_User_function * user_fn, int commute, MPI_Op * op);
void MPIR_Op_free_impl(MPI_Op * op);
#endif /* MPIR_OP_H_INCLUDED */
