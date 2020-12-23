/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "uthash.h"
#include <string.h>

/*
 * This file contains the routines needed to implement the MPI routines that
 * can add error classes and codes during runtime.  This file is organized
 * so that applications that do not use the MPI-2 routines to create new
 * error codes will not load any of this code.
 *
 * ROMIO has been customized to provide error messages with the same tools
 * as the rest of MPICH and will not rely on the dynamically assigned
 * error classes.  This leaves all of the classes and codes for the user.
 *
 * Because we have customized ROMIO, we do not need to implement
 * instance-specific messages for the dynamic error codes.
 */

/* Local data structures.
   A message may be associated with each class and code.
   Since we limit the number of user-defined classes and code (no more
   than 256 of each), we allocate an array of pointers to the messages here.

   We *could* allow 256 codes with each class.  However, we don't expect
   any need for this many codes, so we simply allow 256 (actually
   ERROR_MAX_NCODE) codes, and distribute these among the error codes.

   A user-defined error code has the following format.  The ERROR_xxx
   is the macro that may be used to extract the data (usually a MASK and
   a (right)shift)

   [0-6] Class (same as predefined error classes);  ERROR_CLASS_MASK
   [7]   Is dynamic; ERROR_DYN_MASK and ERROR_DYN_SHIFT
   [8-18] Code index (for messages); ERROR_GENERIC_MASK and ERROR_GENERIC_SHIFT
   [19-31] Zero (unused but defined as zero)
*/

static int not_initialized = 1; /* This allows us to use atomic decr */
static const char *(user_class_msgs[ERROR_MAX_NCLASS]) = {
0};

static const char *(user_code_msgs[ERROR_MAX_NCODE]) = {
0};

/* a container for integer objects that can be used as a linked list
 * or as a hashmap */
struct intcnt {
    int val;

    struct intcnt *next;
    struct intcnt *prev;

    UT_hash_handle hh;
};

static struct {
    int next;
    struct intcnt *free;
    struct intcnt *used;
} err_class, err_code;

static const char empty_error_string[1] = { 0 };

/* Forward reference */
static const char *get_dynerr_string(int code);

/* This external allows this package to define the routine that converts
   dynamically assigned codes and classes to their corresponding strings.
   A cleaner implementation could replace this exposed global with a method
   defined in the error_string.c file that allowed this package to set
   the routine. */

static int MPIR_Dynerrcodes_finalize(void *);

/* Local routine to initialize the data structures for the dynamic
   error classes and codes.

   MPIR_Init_err_dyncodes is called if not_initialized is true.
   Because all of the routines in this file are called by the
   MPI_Add_error_xxx routines, and those routines use the SINGLE_CS
   when the implementation is multithreaded, these routines (until
   we implement finer-grain thread-synchronization) need not worry about
   multiple threads
 */
static void MPIR_Init_err_dyncodes(void)
{
    int i;

    /* FIXME: Does this need a thread-safe init? */
    not_initialized = 0;

    err_class.next = 1; /* class 0 is reserved */
    err_class.free = NULL;
    err_class.used = NULL;
    err_code.next = 1;  /* code 0 is reserved */
    err_code.free = NULL;
    err_code.used = NULL;

    for (i = 0; i < ERROR_MAX_NCLASS; i++) {
        user_class_msgs[i] = 0;
    }
    for (i = 0; i < ERROR_MAX_NCODE; i++) {
        user_code_msgs[i] = 0;
    }
    /* Set the routine to provides access to the dynamically created
     * error strings */
    MPIR_Process.errcode_to_string = get_dynerr_string;

    /* Add a finalize handler to free any allocated space */
    MPIR_Add_finalize(MPIR_Dynerrcodes_finalize, (void *) 0, 9);
}

/*
  MPIR_Add_error_string_impl - Change the message for an error code or class

  Notes:
  This routine is needed to implement 'MPI_Add_error_string'.
*/
int MPIR_Add_error_string_impl(int code, const char *msg_string)
{
    int errcode, errclass;
    size_t msg_len;
    char *str;

    /* --BEGIN ERROR HANDLING-- */
    if (not_initialized) {
        /* Just to keep the rest of the code more robust, we'll
         * initialize the dynamic error codes *anyway*, but this is
         * an error (see MPI_Add_error_string in the standard) */
        MPIR_Init_err_dyncodes();
        return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                    __func__, __LINE__,
                                    MPI_ERR_ARG, "**argerrcode", "**argerrcode %d", code);
    }
    /* --END ERROR HANDLING-- */

    /* Error strings are attached to a particular error code, not class.
     * As a special case, if the code is 0, we use the class message */
    errclass = code & ERROR_CLASS_MASK;
    errcode = (code & ERROR_GENERIC_MASK) >> ERROR_GENERIC_SHIFT;

    /* --BEGIN ERROR HANDLING-- */
    if (code & ~(ERROR_CLASS_MASK | ERROR_DYN_MASK | ERROR_GENERIC_MASK)) {
        /* Check for invalid error code */
        return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                    __func__, __LINE__,
                                    MPI_ERR_ARG, "**argerrcode", "**argerrcode %d", code);
    }
    /* --END ERROR HANDLING-- */

    /* --------------------------------------------------------------------- */
    msg_len = strlen(msg_string);
    str = (char *) MPL_malloc(msg_len + 1, MPL_MEM_BUFFER);
    /* --BEGIN ERROR HANDLING-- */
    if (!str) {
        return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                    __func__, __LINE__, MPI_ERR_OTHER,
                                    "**nomem", "**nomem %s %d", "error message string", msg_len);
    }
    /* --END ERROR HANDLING-- */

    /* --------------------------------------------------------------------- */
    MPL_strncpy(str, msg_string, msg_len + 1);

    if (errcode) {
        struct intcnt *s;
        HASH_FIND_INT(err_code.used, &errcode, s);
        if (s) {
            MPL_free((void *) (user_code_msgs[errcode]));
            user_code_msgs[errcode] = (const char *) str;
        } else {
            /* FIXME : Unallocated error code? */
            MPL_free(str);
        }
    } else {
        struct intcnt *s;
        HASH_FIND_INT(err_class.used, &errclass, s);
        if (s) {
            MPL_free((void *) (user_class_msgs[errclass]));
            user_class_msgs[errclass] = (const char *) str;
        } else {
            /* FIXME : Unallocated error code? */
            MPL_free(str);
        }
    }

    return MPI_SUCCESS;
}

/*
  MPIR_Delete_error_string_impl - Delete the string associated with an error code or class

  Notes:
  This is used to implement 'MPI_Delete_error_string'; it may also be used by a
  device to delete device-specific error strings.
*/
int MPIR_Delete_error_string_impl(int code)
{
    int mpi_errno = MPI_SUCCESS;
    int errcode = (int) (((unsigned int) code & ERROR_GENERIC_MASK) >> ERROR_GENERIC_SHIFT);
    int errclass = code & ERROR_CLASS_MASK;

    if (not_initialized)
        MPIR_Init_err_dyncodes();

    if (errcode) {
        struct intcnt *s;
        HASH_FIND_INT(err_code.used, &errcode, s);
        if (s) {
            MPL_free((void *) (user_code_msgs[errcode]));
            user_code_msgs[errcode] = NULL;
        } else {
            mpi_errno = MPI_ERR_OTHER;
            goto fn_fail;
        }
    } else {
        struct intcnt *s;
        HASH_FIND_INT(err_class.used, &errclass, s);
        if (s) {
            MPL_free((void *) (user_class_msgs[errclass]));
            user_class_msgs[errclass] = NULL;
        } else {
            mpi_errno = MPI_ERR_OTHER;
            goto fn_fail;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
  MPIR_Add_error_class_impl - Creata a new error class

  Notes:
  This is used to implement 'MPI_Add_error_class'; it may also be used by a
  device to add device-specific error classes.

  Predefined classes are handled directly; this routine is not used to
  initialize the predefined MPI error classes.  This is done to reduce the
  number of steps that must be executed when starting an MPI program.

  This routine should be run within a SINGLE_CS in the multithreaded case.
*/
int MPIR_Add_error_class_impl(int *errorclass)
{
    int mpi_errno = MPI_SUCCESS;
    int new_class;

    if (not_initialized)
        MPIR_Init_err_dyncodes();

    /* Get new class */
    struct intcnt *s;
    if (err_class.free) {
        s = err_class.free;
        DL_DELETE(err_class.free, s);
        HASH_ADD_INT(err_class.used, val, s, MPL_MEM_BUFFER);
    } else {
        s = (struct intcnt *) MPL_malloc(sizeof(struct intcnt), MPL_MEM_BUFFER);
        s->val = err_class.next++;
        HASH_ADD_INT(err_class.used, val, s, MPL_MEM_BUFFER);
    }
    new_class = s->val;

    /* Fail if out of classes */
    MPIR_ERR_CHKANDJUMP(new_class >= ERROR_MAX_NCLASS, mpi_errno, MPI_ERR_OTHER, "**noerrclasses");

    /* Note that the MPI interface always adds an error class without
     * a string.  */
    user_class_msgs[new_class] = 0;

    new_class |= ERROR_DYN_MASK;

    if (new_class > MPIR_Process.attrs.lastusedcode) {
        MPIR_Process.attrs.lastusedcode = new_class;
    }

    *errorclass = new_class;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
  MPIR_Delete_error_class_impl - Delete an error class

  Notes:
  This is used to implement 'MPI_Delete_error_class'; it may also be used by a
  device to delete device-specific error classes.

  Predefined classes are handled directly; this routine is not used to
  delete the predefined MPI error classes.  This is done to reduce the
  number of steps that must be executed when starting an MPI program.

  This routine should be run within a SINGLE_CS in the multithreaded case.
*/
int MPIR_Delete_error_class_impl(int user_errclass)
{
    int mpi_errno = MPI_SUCCESS;

    if (not_initialized)
        MPIR_Init_err_dyncodes();

    int errclass = user_errclass & ~ERROR_DYN_MASK;
    struct intcnt *s;
    HASH_FIND_INT(err_class.used, &errclass, s);

    MPIR_ERR_CHKANDJUMP(s == NULL, mpi_errno, MPI_ERR_OTHER, "**predeferrclass");

    HASH_DEL(err_class.used, s);
    DL_APPEND(err_class.free, s);
    MPL_free((char *) user_class_msgs[s->val]);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
  MPIR_Add_error_code_impl - Create a new error code that is associated with an
  existing error class

  Notes:
  This is used to implement 'MPI_Add_error_code'; it may also be used by a
  device to add device-specific error codes.

  */
int MPIR_Add_error_code_impl(int class, int *code)
{
    int mpi_errno = MPI_SUCCESS;
    int new_code;

    /* Note that we can add codes to existing classes, so we may
     * need to initialize the dynamic error routines in this function */
    if (not_initialized)
        MPIR_Init_err_dyncodes();

    /* Get the new code */
    struct intcnt *s;
    if (err_code.free) {
        s = err_code.free;
        DL_DELETE(err_code.free, s);
        HASH_ADD_INT(err_code.used, val, s, MPL_MEM_BUFFER);
    } else {
        s = (struct intcnt *) MPL_malloc(sizeof(struct intcnt), MPL_MEM_BUFFER);
        s->val = err_code.next++;
        HASH_ADD_INT(err_code.used, val, s, MPL_MEM_BUFFER);
    }
    new_code = s->val;

    /* Fail if out of codes */
    MPIR_ERR_CHKANDJUMP(new_code >= ERROR_MAX_NCODE, mpi_errno, MPI_ERR_OTHER, "**noerrcodes");

    /* Create the full error code */
    new_code = class | (new_code << ERROR_GENERIC_SHIFT);

    /* FIXME: For robustness, we should make sure that the associated string
     * is initialized to null */
    *code = new_code;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
  MPIR_Delete_error_code_impl - Delete an error code

  Notes:
  This is used to implement 'MPI_Delete_error_code'; it may also be used by a
  device to delete device-specific error codes.
*/
int MPIR_Delete_error_code_impl(int code)
{
    int mpi_errno = MPI_SUCCESS;
    int errcode = (int) (((unsigned int) code & ERROR_GENERIC_MASK) >> ERROR_GENERIC_SHIFT);

    if (not_initialized)
        MPIR_Init_err_dyncodes();

    struct intcnt *s;
    HASH_FIND_INT(err_code.used, &errcode, s);

    MPIR_ERR_CHKANDJUMP(s == NULL, mpi_errno, MPI_ERR_OTHER, "**predeferrcode");

    HASH_DEL(err_code.used, s);
    DL_APPEND(err_code.free, s);
    MPL_free((char *) user_code_msgs[s->val]);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
  get_dynerr_string - Get the message string that corresponds to a
  dynamically created error class or code

Input Parameters:
+ code - An error class or code.  If a code, it must have been created by
  'MPIR_Add_error_code_impl'.

  Return value:
  A pointer to a null-terminated text string with the corresponding error
  message.  A null return indicates an error; usually the value of 'code' is
  neither a valid error class or code.

  Notes:
  This routine is used to implement 'MPI_ERROR_STRING'.  It is only called
  for dynamic error codes.
  */
static const char *get_dynerr_string(int code)
{
    int errcode, errclass;
    const char *errstr = 0;

    /* Error strings are attached to a particular error code, not class.
     * As a special case, if the code is 0, we use the class message */
    errclass = code & ERROR_CLASS_MASK;
    errcode = (code & ERROR_GENERIC_MASK) >> ERROR_GENERIC_SHIFT;

    if (code & ~(ERROR_CLASS_MASK | ERROR_DYN_MASK | ERROR_GENERIC_MASK)) {
        /* Check for invalid error code */
        return 0;
    }

    if (errcode) {
        struct intcnt *s;
        HASH_FIND_INT(err_code.used, &errcode, s);
        if (s) {
            errstr = user_code_msgs[errcode];
            if (!errstr)
                errstr = empty_error_string;
        }
    } else {
        struct intcnt *s;
        HASH_FIND_INT(err_class.used, &errclass, s);
        if (s) {
            errstr = user_class_msgs[errclass];
            if (!errstr)
                errstr = empty_error_string;
        }
    }

    return errstr;
}


static int MPIR_Dynerrcodes_finalize(void *p ATTRIBUTE((unused)))
{
    MPL_UNREFERENCED_ARG(p);

    if (not_initialized == 0) {
        struct intcnt *s, *tmp;

        HASH_ITER(hh, err_class.used, s, tmp) {
            MPL_free((char *) user_class_msgs[s->val]);
            HASH_DEL(err_class.used, s);
            MPL_free(s);
        }

        DL_FOREACH_SAFE(err_class.free, s, tmp) {
            MPL_free((char *) user_class_msgs[s->val]);
            DL_DELETE(err_class.free, s);
            MPL_free(s);
        }

        HASH_ITER(hh, err_code.used, s, tmp) {
            MPL_free((char *) user_code_msgs[s->val]);
            HASH_DEL(err_code.used, s);
            MPL_free(s);
        }

        DL_FOREACH_SAFE(err_code.free, s, tmp) {
            MPL_free((char *) user_code_msgs[s->val]);
            DL_DELETE(err_code.free, s);
            MPL_free(s);
        }
    }
    return 0;
}
