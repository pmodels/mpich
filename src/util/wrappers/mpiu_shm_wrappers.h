/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIU_SHM_WRAPPERS_H_INCLUDED
#define MPIU_SHM_WRAPPERS_H_INCLUDED
/* FIXME: Add underscore to the end of funcs/macro names  not to 
 * be exposed to user 
 */
/* SHM Wrapper funcs defined in this header file */

#include "mpichconfconst.h"
#include "mpichconf.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef USE_SYSV_SHM
    #include <sys/stat.h>
    #include <sys/ipc.h>
    #include <sys/shm.h>
#elif defined USE_MMAP_SHM
    #include <fcntl.h>
    #include <sys/mman.h>
    #if defined (HAVE_MKSTEMP) && defined (NEEDS_MKSTEMP_DECL)
        extern int mkstemp(char *template);
    #endif
#elif defined USE_NT_SHM
    #include<winsock2.h>
    #include<windows.h>
#else
#   error "No shared memory mechanism specified"
#endif

#include "mpiu_os_wrappers_pre.h"
#include "mpiu_util_wrappers.h"

#if !(defined(MPISHARED_H_INCLUDED) || defined(MPIIMPL_H_INCLUDED))
#error "this header must be included after mpiimpl.h or mpishared.h"
#endif

/* FIXME: Reduce the conditional checks for wrapper-internal
 * utility funcs/macros.
 */

/* Allocate mem for references within the handle */
/* Returns 0 on success, -1 on error */
#define MPIU_SHMW_Hnd_ref_alloc(hnd)(                               \
    ((hnd)->ghnd = (MPIU_SHMW_Ghnd_t)                               \
                    MPIU_Malloc(MPIU_SHMW_GHND_SZ)) ? 0 : -1        \
)

#ifdef  USE_SYSV_SHM
/************************** Sys V shm **************************/
#define MPIU_SHMW_MAX_INT_STR_LEN   20
#define MPIU_SHMW_SEG_KEY_LEN    MPIU_SHMW_MAX_INT_STR_LEN
#define MPIU_SHMW_GHND_SZ MPIU_SHMW_SEG_KEY_LEN
#define MPIU_SHMW_LHND_INVALID    -1
#define MPIU_SHMW_LHND_INIT_VAL    -1

#define MPIU_SHMW_SEG_ALREADY_EXISTS EEXIST

/* These macros are the setters/getters for the shm handle */
#define MPIU_SHMW_Lhnd_get(hnd)   ((hnd)->lhnd)
#define MPIU_SHMW_Lhnd_set(hnd, val)  ((hnd)->lhnd=val)

#define MPIU_SHMW_Lhnd_is_valid(hnd) (                              \
    ((hnd)->lhnd != MPIU_SHMW_LHND_INVALID)                         \
)
#define MPIU_SHMW_Lhnd_is_init(hnd) 1
/* Nothing to be done at close */
#define MPIU_SHMW_Lhnd_close(hnd) 0

#elif defined USE_MMAP_SHM
/************************** MMAP shm **************************/
#define MPIU_SHMW_FNAME_LEN    50
#define MPIU_SHMW_GHND_SZ MPIU_SHMW_FNAME_LEN
#define MPIU_SHMW_LHND_INVALID    -1
#define MPIU_SHMW_LHND_INIT_VAL    -1

#define MPIU_SHMW_SEG_ALREADY_EXISTS EEXIST

/* These macros are the setters/getters for the shm handle */
#define MPIU_SHMW_Lhnd_get(hnd)   ((hnd)->lhnd)
#define MPIU_SHMW_Lhnd_set(hnd, val)  ((hnd)->lhnd=val)

#define MPIU_SHMW_Lhnd_is_valid(hnd) (                              \
    ((hnd)->lhnd != MPIU_SHMW_LHND_INVALID)                         \
)
#define MPIU_SHMW_Lhnd_is_init(hnd) 1

/* Returns 0 on success, -1 on error */
static inline int MPIU_SHMW_Lhnd_close(MPIU_SHMW_Hnd_t hnd)
{
    MPIU_SHMW_Lhnd_t lhnd = MPIU_SHMW_LHND_INVALID;
    lhnd = MPIU_SHMW_Lhnd_get(hnd);
    if(lhnd != MPIU_SHMW_LHND_INVALID) {
        if(close(lhnd) == 0){
            MPIU_SHMW_Lhnd_set(hnd, MPIU_SHMW_LHND_INIT_VAL);
        }
        else{
            /* close() failed */
            return -1;
        }
    }
    return 0;
}

#elif defined USE_NT_SHM
/************************** NT shm **************************/
#define MPIU_SHMW_SEG_NAME_LEN    70
#define MPIU_SHMW_GHND_SZ MPIU_SHMW_SEG_NAME_LEN
#define MPIU_SHMW_LHND_INVALID    INVALID_HANDLE_VALUE
#define MPIU_SHMW_LHND_INIT_VAL    INVALID_HANDLE_VALUE

#define MPIU_SHMW_SEG_ALREADY_EXISTS ERROR_ALREADY_EXISTS

/* These macros are the setters/getters for the shm handle */
#define MPIU_SHMW_Lhnd_get(hnd)   ((hnd)->lhnd)
#define MPIU_SHMW_Lhnd_set(hnd, val)  ((hnd)->lhnd=val)

#define MPIU_SHMW_Lhnd_is_valid(hnd) (                              \
    ((hnd)->lhnd != MPIU_SHMW_LHND_INVALID)                         \
)
#define MPIU_SHMW_Lhnd_is_init(hnd) 1

/* Returns 0 on success, -1 on error */
#define MPIU_SHMW_Lhnd_close(hnd)(                                  \
    (CloseHandle(MPIU_SHMW_Lhnd_get(hnd)) != 0) ? 0 : -1            \
)
/* Returns 0 on success, -1 on error */
static inline int MPIU_SHMW_Ghnd_set_uniq(MPIU_SHMW_Hnd_t hnd)
{
    if(MPIU_SHMW_Hnd_ref_alloc(hnd) == 0){
        if(MPIU_OSW_Get_uniq_str(hnd->ghnd, MPIU_SHMW_GHND_SZ) != 0){
            return -1;
        }
    }
    else{
        return -1;
    }
    return 0;
}
#endif /* USE_NT_SHM */

#define MPIU_SHMW_HND_SZ    (sizeof(MPIU_SHMW_LGhnd_t))
#define MPIU_SHMW_SER_HND_SZ MPIU_SHMW_GHND_SZ

/* These macros are the setters/getters for the shm handle */
#define MPIU_SHMW_Ghnd_get_by_ref(hnd)   ((hnd)->ghnd)

/* Returns -1 on error, 0 on success */
#define MPIU_SHMW_Ghnd_get_by_val(hnd, str, strlen)  (              \
    (MPIU_Snprintf(str, strlen, "%s",                               \
        MPIU_SHMW_Ghnd_get_by_ref(hnd))) ? 0 : -1                   \
)
#define MPIU_SHMW_Ghnd_set_by_ref(hnd, val) ((hnd)->ghnd = val)
/* Returns -1 on error, 0 on success */
/* FIXME: What if val is a non-null terminated string ? */
#define MPIU_SHMW_Ghnd_set_by_val(hnd, fmt, val) (                  \
    (MPIU_Snprintf(MPIU_SHMW_Ghnd_get_by_ref(hnd),                  \
        MPIU_SHMW_GHND_SZ, fmt, val)) ? 0 : -1                      \
)

#define MPIU_SHMW_Ghnd_is_valid(hnd) (                              \
    (((hnd)->ghnd == MPIU_SHMW_GHND_INVALID) ||                     \
        (strlen((hnd)->ghnd) == 0)) ? 0 : 1                         \
)
#define MPIU_SHMW_Ghnd_is_init(hnd) (                               \
    ((hnd)->flag & MPIU_SHMW_FLAG_GHND_STATIC) ?                    \
    1 :                                                             \
    (((hnd)->ghnd != MPIU_SHMW_GHND_INVALID) ? 1 : 0)               \
)

/* Allocate mem for global handle.
 * Returns 0 on success, -1 on failure 
 */
static inline int MPIU_SHMW_Ghnd_alloc(MPIU_SHMW_Hnd_t hnd)
{
    if(!(hnd->ghnd)){
        hnd->ghnd = (MPIU_SHMW_Ghnd_t)MPIU_Malloc(MPIU_SHMW_GHND_SZ);
        if(!(hnd->ghnd)){ return -1; }
    }
    /* Global handle is no longer static */
    hnd->flag &= ~MPIU_SHMW_FLAG_GHND_STATIC;
    return 0;
}

/* A Handle is valid if it is initialized/init and has a value 
 * different from the default/invalid value assigned during init
 */
#define MPIU_SHMW_Hnd_is_valid(hnd) (                               \
    ((hnd) &&                                                       \
        MPIU_SHMW_Lhnd_is_valid(hnd) &&                             \
        MPIU_SHMW_Ghnd_is_valid(hnd))                               \
)

/* With MMAP_SHM, NT_SHM & SYSV_SHM local handle is always init'ed */
#define MPIU_SHMW_Hnd_is_init(hnd) (                                \
    ((hnd) && /* MPIU_SHMW_Lhnd_is_init(hnd) && */                  \
        MPIU_SHMW_Ghnd_is_init(hnd))                                \
)


/* Allocate mem for handle. Lazy allocation for global handle */
/* Returns 0 on success, -1 on error */
static inline int MPIU_SHMW_Hnd_alloc(MPIU_SHMW_Hnd_t *hnd_ptr)
{
    MPIU_Assert(hnd_ptr);
    *hnd_ptr = (MPIU_SHMW_Hnd_t) MPIU_Malloc(MPIU_SHMW_HND_SZ);
    if(*hnd_ptr){
        (*hnd_ptr)->flag |= MPIU_SHMW_FLAG_GHND_STATIC;
    }
    else{
        return -1;
    }
    return 0;
}

/* Close Handle */
#define MPIU_SHMW_Hnd_close(hnd) MPIU_SHMW_Lhnd_close(hnd)

static inline void MPIU_SHMW_Hnd_reset_val(MPIU_SHMW_Hnd_t hnd)
{
    MPIU_Assert(hnd);
    MPIU_SHMW_Lhnd_set(hnd, MPIU_SHMW_LHND_INIT_VAL);
    if(hnd->flag & MPIU_SHMW_FLAG_GHND_STATIC){
        hnd->ghnd = MPIU_SHMW_GHND_INVALID;
    }
    else{
        MPIU_Assert(hnd->ghnd);
        (hnd->ghnd)[0] = MPIU_SHMW_GHND_INIT_VAL;
    }
}

static inline void MPIU_SHMW_Hnd_free(MPIU_SHMW_Hnd_t hnd)
{
    if(MPIU_SHMW_Hnd_is_init(hnd)){
        if(!(hnd->flag & MPIU_SHMW_FLAG_GHND_STATIC)){
            MPIU_Free(hnd->ghnd);
        }
        MPIU_Free(hnd);
    }
}

static inline int MPIU_SHMW_Seg_open(MPIU_SHMW_Hnd_t hnd, int seg_sz);
static inline int MPIU_SHMW_Hnd_deserialize_by_ref(MPIU_SHMW_Hnd_t hnd, char **ser_hnd_ptr);

/* FIXME : Don't print ENGLISH strings on error. Define the error
 * strings in errnames.txt
 */
/* Serialize a handle. A serialized handle is a string of
 * characters that can be persisted by the caller. The serialized
 * handle can be used to create another ref to the shared mem seg
 * by deserializing it.
 * str  : A string of chars of len, str_len.
 *          If the function succeeds the serialized handle is copied 
 *          into this user buffer
 * hnd  : Handle to shared memory
 */
#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Hnd_serialize
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIU_SHMW_Hnd_serialize(char *str, 
    MPIU_SHMW_Hnd_t hnd, int str_len)
{
    int mpi_errno = MPI_SUCCESS;
    int rc = -1;

    MPIU_Assert(MPIU_SHMW_Hnd_is_init(hnd));
    MPIU_Assert(str);
    MPIU_Assert(str_len >= MPIU_SHMW_GHND_SZ);

    rc = MPIU_SHMW_Ghnd_get_by_val(hnd, str, str_len);
    MPIU_ERR_CHKANDJUMP(rc != 0, mpi_errno, MPI_ERR_OTHER, "**shmw_gethnd");

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Deserialize a handle.
 * str_hnd  : A null-terminated string of len str_hnd_len that 
 *              contains the serialized handle. 
 * hnd      : If the call succeeds the user gets back a handle,hnd, to 
 *           shared mem - deserialized from strHnd. This handle
 *           will refer to the shm seg referred by the serialized
 *           handle.
 */
#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Hnd_deserialize
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIU_SHMW_Hnd_deserialize(
    MPIU_SHMW_Hnd_t hnd, const char *str_hnd, int str_hnd_len)
{
    int mpi_errno = MPI_SUCCESS;
    int rc = -1;

    MPIU_Assert(MPIU_SHMW_Hnd_is_init(hnd));
    MPIU_ERR_CHKINTERNAL(!str_hnd, mpi_errno, "ser hnd is null");
    MPIU_ERR_CHKANDJUMP(str_hnd_len>=MPIU_SHMW_GHND_SZ,
        mpi_errno, MPI_ERR_OTHER, "**shmw_deserbufbig");

    MPIU_SHMW_Hnd_reset_val(hnd);

    rc = MPIU_SHMW_Ghnd_alloc(hnd);
    MPIU_ERR_CHKANDJUMP1((rc != 0), mpi_errno, MPI_ERR_OTHER,
        "**nomem", "**nomem %s", "shared mem global handle");

    rc = MPIU_SHMW_Ghnd_set_by_val(hnd, "%s", str_hnd);
    MPIU_Assert(rc == 0);

    mpi_errno = MPIU_SHMW_Seg_open(hnd, 0);
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }
    
fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Get a serialized handle by reference. 
 * Rationale: The user might only want to read the serialized view
 * of the handle & hence not want to allocate a buffer for the ser view 
 * of the handle.
 * str_ptr  : Pointer to a string of chars to hold the serialized handle
 *           If the function succeeds, the pointer points to a
 *           serialized view of the handle.
 * hnd      : Handle to shm seg which has to be serialized
 */
 
#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Hnd_get_serialized_by_ref
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIU_SHMW_Hnd_get_serialized_by_ref(
    MPIU_SHMW_Hnd_t hnd, char **str_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_SHMW_Hnd_is_init(hnd));
    MPIU_Assert(str_ptr);

    *str_ptr = (char *)MPIU_SHMW_Ghnd_get_by_ref(hnd);
    MPIU_Assert(*str_ptr);

    return mpi_errno;
}

/* Deserialize a handle by reference.
 * Rationale : The user already has a serialized view of the handle.
 *            The user does not want to manage the view buffer any more
 *            & also needs to deserialize from the buffer.
 * ser_hnd_ptr  : Pointer to a serialized view of the handle. The user
 *           no longer needs to take care of this buffer.
 * hnd      : If the function succeeds this points to the deserialized
 *           handle.
 */
#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Hnd_deserialize_by_ref
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIU_SHMW_Hnd_deserialize_by_ref(
    MPIU_SHMW_Hnd_t hnd, char **ser_hnd_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_SHMW_Hnd_is_init(hnd));
    MPIU_Assert(ser_hnd_ptr);

    MPIU_ERR_CHKINTERNAL(!(*ser_hnd_ptr), mpi_errno, "ser hnd is null");

    MPIU_SHMW_Hnd_reset_val(hnd);
    MPIU_SHMW_Ghnd_set_by_ref(hnd, *ser_hnd_ptr);

    mpi_errno = MPIU_SHMW_Seg_open(hnd, 0);
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Initialize a shared memory handle
 * hnd_ptr : A pointer to the shared memory handle
 */

#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Hnd_init
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIU_SHMW_Hnd_init(
                    MPIU_SHMW_Hnd_t *hnd_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int rc = -1;

    MPIU_Assert(hnd_ptr);

    rc = MPIU_SHMW_Hnd_alloc(hnd_ptr);
    MPIU_ERR_CHKANDJUMP1((rc != 0), mpi_errno, MPI_ERR_OTHER,
        "**nomem", "**nomem %s", "shared mem handle");

    MPIU_SHMW_Hnd_reset_val(*hnd_ptr);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Finalize a shared memory handle.
 * hnd_ptr : A pointer to the shm handle to be finalized.
 *           Any handle that is init has to be finalized.
 */
#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Hnd_finalize
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIU_SHMW_Hnd_finalize(
                    MPIU_SHMW_Hnd_t *hnd_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(hnd_ptr);
    MPIU_Assert(*hnd_ptr);

    /* A finalize can/should be called on an invalid handle
     * Don't assert if we fail here ...
     */
    MPIU_SHMW_Hnd_close(*hnd_ptr);
    MPIU_SHMW_Hnd_free(*hnd_ptr);

    *hnd_ptr = MPIU_SHMW_HND_INVALID;

    return mpi_errno;
}

#ifdef  USE_SYSV_SHM
/************************** Sys V shm **************************/
/* A template function which creates/attaches shm seg handle
 * to the shared memory. Used by user-exposed functions below
 */
#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Seg_create_attach_templ
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIU_SHMW_Seg_create_attach_templ(
    MPIU_SHMW_Hnd_t hnd, int seg_sz, char **shm_addr_ptr,
    int offset, int flag)
{
    int mpi_errno = MPI_SUCCESS;
    int rc = -1;
    int lhnd = -1;

    if(flag & MPIU_SHMW_FLAG_SHM_CREATE){
        lhnd = shmget(IPC_PRIVATE, seg_sz, IPC_CREAT | S_IRWXU);
        /* Return error if SHM seg already exists or create fails */
        MPIU_ERR_CHKANDJUMP2((lhnd == -1) ||
            (MPIU_OSW_Get_errno() == MPIU_SHMW_SEG_ALREADY_EXISTS),
            mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem",
            "**alloc_shar_mem %s %s", "shmget",
            MPIU_OSW_Strerror(MPIU_OSW_Get_errno()));

        MPIU_SHMW_Lhnd_set(hnd, lhnd);

        rc = MPIU_SHMW_Ghnd_alloc(hnd);
        MPIU_ERR_CHKANDJUMP1((rc != 0), mpi_errno, MPI_ERR_OTHER,
            "**nomem", "**nomem %s", "shared mem global handle");

        rc = MPIU_SHMW_Ghnd_set_by_val(hnd, "%d", lhnd);
        MPIU_Assert(rc == 0);
    }
    else{
        /* Open an existing shared memory seg */
        MPIU_Assert(MPIU_SHMW_Ghnd_is_valid(hnd));

        if(!MPIU_SHMW_Lhnd_is_valid(hnd)){
            lhnd = atoi(MPIU_SHMW_Ghnd_get_by_ref(hnd));
            MPIU_ERR_CHKANDJUMP((lhnd == -1), mpi_errno,
                MPI_ERR_OTHER, "**shmw_badhnd");

            MPIU_SHMW_Lhnd_set(hnd, lhnd);
        }
    }

    if(flag & MPIU_SHMW_FLAG_SHM_ATTACH){
        /* Attach to shared mem seg */
        MPIU_Assert(shm_addr_ptr);

        *shm_addr_ptr = shmat(MPIU_SHMW_Lhnd_get(hnd), NULL, 0x0);

        MPIU_ERR_CHKANDJUMP2((*shm_addr_ptr == (void*)(-1)), mpi_errno, MPI_ERR_OTHER,
            "**attach_shar_mem", "**attach_shar_mem %s %s",
            "shmat", MPIU_OSW_Strerror(MPIU_OSW_Get_errno()));
    }
    
fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Detach from an attached SHM segment 
 * hnd : Handle to the shm segment
 * shm_addr_ptr : Pointer to the shm address to detach
 * seg_sz : Size of shm segment
 */
#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Seg_detach
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIU_SHMW_Seg_detach(
    MPIU_SHMW_Hnd_t hnd, char **shm_addr_ptr, int seg_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int rc = -1;

    MPIU_ERR_CHKANDJUMP(!MPIU_SHMW_Hnd_is_valid(hnd),
        mpi_errno, MPI_ERR_OTHER, "**shmw_badhnd");
    MPIU_Assert(shm_addr_ptr);
    MPIU_ERR_CHKINTERNAL(!(*shm_addr_ptr), mpi_errno, "shm address is null");

    rc = shmdt(*shm_addr_ptr);
    MPIU_ERR_CHKANDJUMP2((rc != 0), mpi_errno, MPI_ERR_OTHER,
        "**detach_shar_mem","**detach_shar_mem %s %s",
        "shmdt", MPIU_OSW_Strerror(MPIU_OSW_Get_errno()));
    *shm_addr_ptr = NULL;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Remove a shared memory segment
 * hnd : Handle to the shared memory segment to be removed
 */

#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Seg_remove
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int  MPIU_SHMW_Seg_remove(MPIU_SHMW_Hnd_t hnd)
{
    int mpi_errno = MPI_SUCCESS;
    struct shmid_ds ds;
    int rc = -1;

    MPIU_ERR_CHKANDJUMP(!MPIU_SHMW_Hnd_is_valid(hnd),
        mpi_errno, MPI_ERR_OTHER, "**shmw_badhnd");

    rc = shmctl(MPIU_SHMW_Lhnd_get(hnd), IPC_RMID, &ds);
    MPIU_ERR_CHKANDJUMP2((rc != 0), mpi_errno, MPI_ERR_OTHER,
        "**remove_shar_mem", "**remove_shar_mem %s %s","shmctl",
        MPIU_OSW_Strerror(MPIU_OSW_Get_errno()));

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#elif USE_MMAP_SHM
/************************** MMAP shm **************************/
/* A template function which creates/attaches shm seg handle
 * to the shared memory. Used by user-exposed functions below
 */
/* FIXME: Pass (void **)shm_addr_ptr instead of (char **) shm_addr_ptr 
 *  since a util func should be generic
 *  Currently not passing (void **) to reduce warning in nemesis
 *  code which passes (char **) ptrs to be attached to a seg
 */

#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Seg_create_attach_templ
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIU_SHMW_Seg_create_attach_templ(
    MPIU_SHMW_Hnd_t hnd, int seg_sz, char **shm_addr_ptr,
    int offset, int flag)
{
    int mpi_errno = MPI_SUCCESS;
    int lhnd = -1, rc = -1;

    if(flag & MPIU_SHMW_FLAG_SHM_CREATE){
        char dev_shm_fname[] = "/dev/shm/mpich_shar_tmpXXXXXX";
        char tmp_fname[] = "/tmp/mpich_shar_tmpXXXXXX";
        char *chosen_fname = NULL;

        chosen_fname = dev_shm_fname;
        lhnd = mkstemp(chosen_fname);
        if(lhnd == -1){
            chosen_fname = tmp_fname;
            lhnd = mkstemp(chosen_fname);
        }
        MPIU_ERR_CHKANDJUMP1((lhnd == -1), mpi_errno, 
            MPI_ERR_OTHER,"**mkstemp","**mkstemp %s",
            MPIU_OSW_Strerror(MPIU_OSW_Get_errno()));

        MPIU_SHMW_Lhnd_set(hnd, lhnd);
        rc = lseek(lhnd, seg_sz - 1, SEEK_SET);
        MPIU_ERR_CHKANDJUMP1((rc == -1), mpi_errno,
            MPI_ERR_OTHER, "**lseek", "**lseek %s",
            MPIU_OSW_Strerror(MPIU_OSW_Get_errno()));

        MPIU_OSW_RETRYON_INTR((rc == -1), (rc = write(lhnd, "", 1)));
        MPIU_ERR_CHKANDJUMP((rc == -1), mpi_errno, MPI_ERR_OTHER,
            "**write");

        rc = MPIU_SHMW_Ghnd_alloc(hnd);
        MPIU_ERR_CHKANDJUMP1((rc != 0),mpi_errno, MPI_ERR_OTHER,
            "**nomem", "**nomem %s", "shared memory global handle");

        rc = MPIU_SHMW_Ghnd_set_by_val(hnd, "%s", chosen_fname);
        MPIU_Assert(rc == 0);
    }
    else{
        /* Open an existing shared memory seg */
        MPIU_Assert(MPIU_SHMW_Ghnd_is_valid(hnd));

        if(!MPIU_SHMW_Lhnd_is_valid(hnd)){
            lhnd = open(MPIU_SHMW_Ghnd_get_by_ref(hnd), O_RDWR);
            MPIU_ERR_CHKANDJUMP1((lhnd == -1), mpi_errno,
                MPI_ERR_OTHER, "**open", "**open %s",
                MPIU_OSW_Strerror(MPIU_OSW_Get_errno()));

            MPIU_SHMW_Lhnd_set(hnd, lhnd);
        }
    }

    if(flag & MPIU_SHMW_FLAG_SHM_ATTACH){
        void *buf_ptr = NULL;

        MPIU_Assert(shm_addr_ptr);

        buf_ptr = mmap(NULL, seg_sz, PROT_READ | PROT_WRITE,
                        MAP_SHARED, MPIU_SHMW_Lhnd_get(hnd), 0);
        MPIU_ERR_CHKANDJUMP2((buf_ptr == MAP_FAILED),
            mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem",
            "**alloc_shar_mem %s %s", "mmap",
            MPIU_OSW_Strerror(MPIU_OSW_Get_errno()));

        *shm_addr_ptr = buf_ptr;
    }

fn_exit:
    /* FIXME: Close local handle only when closing the shm handle */
    if(MPIU_SHMW_Lhnd_is_valid(hnd)){
        rc = MPIU_SHMW_Lhnd_close(hnd);
        MPIU_Assert(rc == 0);
    } 
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Detach from an attached SHM segment */
#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Seg_detach
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIU_SHMW_Seg_detach(
    MPIU_SHMW_Hnd_t hnd, char **shm_addr_ptr, int seg_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int rc = -1;

    MPIU_Assert(shm_addr_ptr);
    MPIU_ERR_CHKINTERNAL(!(*shm_addr_ptr), mpi_errno, "shm address is null");

    rc = munmap(*shm_addr_ptr, seg_sz);
    MPIU_ERR_CHKANDJUMP2((rc != 0), mpi_errno, MPI_ERR_OTHER,
        "**detach_shar_mem", "**detach_shar_mem %s %s","munmap", 
        MPIU_OSW_Strerror(MPIU_OSW_Get_errno()));
    *shm_addr_ptr = NULL;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Remove an existing SHM segment */
static inline int  MPIU_SHMW_Seg_remove(MPIU_SHMW_Hnd_t hnd)
{
    int mpi_errno = MPI_SUCCESS;
    int rc = -1;

    MPIU_ERR_CHKANDJUMP(!MPIU_SHMW_Ghnd_is_valid(hnd),
        mpi_errno, MPI_ERR_OTHER, "**shmw_badhnd");

    rc = unlink(MPIU_SHMW_Ghnd_get_by_ref(hnd));
    MPIU_ERR_CHKANDJUMP2((rc != 0), mpi_errno, MPI_ERR_OTHER,
        "**remove_shar_mem", "**remove_shar_mem %s %s","unlink", 
        MPIU_OSW_Strerror(MPIU_OSW_Get_errno()));

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#elif defined (USE_NT_SHM)
/************************** NT shm **************************/
/* A template function which creates/attaches shm seg handle
 * to the shared memory. Used by user-exposed functions below
 */
#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Seg_create_attach_templ
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIU_SHMW_Seg_create_attach_templ(
    MPIU_SHMW_Hnd_t hnd, int seg_sz, char **shm_addr_ptr,
    int offset, int flag)
{
    int mpi_errno = MPI_SUCCESS;
    HANDLE lhnd = INVALID_HANDLE_VALUE;
    int rc = -1;

    if(!MPIU_SHMW_Ghnd_is_valid(hnd)){
        MPIU_Assert(flag & MPIU_SHMW_FLAG_SHM_CREATE);

        rc = MPIU_SHMW_Ghnd_set_uniq(hnd);
        MPIU_ERR_CHKANDJUMP((rc == 0), mpi_errno, MPI_ERR_OTHER,
            "**shmw_sethnd");
    }

    if(flag & MPIU_SHMW_FLAG_SHM_CREATE){
        lhnd = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, 
                PAGE_READWRITE, 0, seg_sz, 
                MPIU_SHMW_Ghnd_get_by_ref(hnd));
        /* Return error if SHM seg already exists or create fails */
        MPIU_ERR_CHKANDJUMP2((lhnd == INVALID_HANDLE_VALUE) ||
            (MPIU_OSW_Get_errno() == MPIU_SHMW_SEG_ALREADY_EXISTS),
            mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem",
            "**alloc_shar_mem %s %s", "CreateFileMapping",
            MPIU_OSW_Strerror(MPIU_OSW_Get_errno()));

        MPIU_SHMW_Lhnd_set(hnd, lhnd);
    }
    else{
        MPIU_Assert(MPIU_SHMW_Ghnd_is_valid(hnd));

        if(!MPIU_SHMW_Lhnd_is_valid(hnd)){
            /* Strangely OpenFileMapping() returns NULL on error! */
            lhnd = OpenFileMapping(FILE_MAP_WRITE, FALSE, 
                    MPIU_SHMW_Ghnd_get_by_ref(hnd));
            MPIU_ERR_CHKANDJUMP2((lhnd == NULL), mpi_errno,
                MPI_ERR_OTHER, "**alloc_shar_mem", 
                "**alloc_shar_mem %s %s", "OpenFileMapping",
                MPIU_OSW_Strerror(MPIU_OSW_Get_errno()));
        
            MPIU_SHMW_Lhnd_set(hnd, lhnd);
        }
    }

    if(flag & MPIU_SHMW_FLAG_SHM_ATTACH){
        MPIU_Assert(shm_addr_ptr);

        *shm_addr_ptr = (char *)MapViewOfFile(MPIU_SHMW_Lhnd_get(hnd),
                            FILE_MAP_WRITE, 0, offset, 0);
        MPIU_ERR_CHKANDJUMP2(!(*shm_addr_ptr), mpi_errno, MPI_ERR_OTHER,
            "**attach_shar_mem", "**attach_shar_mem %s %s",
            "MapViewOfFile", MPIU_OSW_Strerror(MPIU_OSW_Get_errno()));
    }
    
fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Detach from an attached SHM segment */
#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Seg_detach
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIU_SHMW_Seg_detach(
    MPIU_SHMW_Hnd_t hnd, char **shm_addr_ptr, int seg_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int rc = -1;

    MPIU_UNREFERENCED_ARG(seg_sz);
    MPIU_ERR_CHKANDJUMP(!MPIU_SHMW_Hnd_is_valid(hnd),
        mpi_errno, MPI_ERR_OTHER, "**shmw_badhnd");
    MPIU_Assert(shm_addr_ptr);
    MPIU_ERR_CHKINTERNAL(!(*shm_addr_ptr), mpi_errno, "shm address is null");

    rc = UnmapViewOfFile(*shm_addr_ptr);
    MPIU_ERR_CHKANDJUMP2((rc == 0), mpi_errno, MPI_ERR_OTHER,
        "**detach_shar_mem", "**detach_shar_mem %s %s","UnmapViewOfFile", 
        MPIU_OSW_Strerror(MPIU_OSW_Get_errno()));
    *shm_addr_ptr = NULL;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Nothing to be done when removing an SHM segment */
#define  MPIU_SHMW_Seg_remove(hnd) MPI_SUCCESS

#endif /* USE_NT_SHM */

/* Create new SHM segment 
 * hnd : A "init"ed shared memory handle
 * seg_sz : Size of shared memory segment to be created
 */
#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Seg_create
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIU_SHMW_Seg_create(
    MPIU_SHMW_Hnd_t hnd, int seg_sz)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_SHMW_Hnd_is_init(hnd));
    MPIU_Assert(seg_sz > 0);

    mpi_errno = MPIU_SHMW_Seg_create_attach_templ(hnd,
                    seg_sz, NULL, 0, MPIU_SHMW_FLAG_SHM_CREATE);
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Open an existing SHM segment 
 * hnd : A shm handle with a valid global handle
 * seg_sz : Size of shared memory segment to open
 * Currently only using internally within wrapper funcs
 */
#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Seg_open
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIU_SHMW_Seg_open(
    MPIU_SHMW_Hnd_t hnd, int seg_sz)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_SHMW_Hnd_is_init(hnd));

    mpi_errno = MPIU_SHMW_Seg_create_attach_templ(hnd, seg_sz,
            NULL, 0, MPIU_SHMW_FLAG_CLR);
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Create new SHM segment and attach to it 
 * hnd : A "init"ed shared mem handle
 * seg_sz: Size of shared mem segment
 * shm_addr_ptr : Pointer to shared memory address to attach
 *                  the shared mem segment
 * offset : Offset to attach the shared memory address to
 */
#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Seg_create_and_attach
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIU_SHMW_Seg_create_and_attach(
    MPIU_SHMW_Hnd_t hnd, int seg_sz, char **shm_addr_ptr, 
    int offset)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_SHMW_Hnd_is_init(hnd));
    MPIU_Assert(seg_sz > 0);
    MPIU_Assert(shm_addr_ptr);

    mpi_errno = MPIU_SHMW_Seg_create_attach_templ(hnd, seg_sz,
                    shm_addr_ptr, offset, MPIU_SHMW_FLAG_SHM_CREATE |
                    MPIU_SHMW_FLAG_SHM_ATTACH);
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Attach to an existing SHM segment
 * hnd : A "init"ed shared mem handle
 * seg_sz: Size of shared mem segment
 * shm_addr_ptr : Pointer to shared memory address to attach
 *                  the shared mem segment
 * offset : Offset to attach the shared memory address to
 */
#undef FUNCNAME
#define FUNCNAME MPIU_SHMW_Seg_attach
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
static inline int MPIU_SHMW_Seg_attach(
    MPIU_SHMW_Hnd_t hnd, int seg_sz, char **shm_addr_ptr,
    int offset)
{
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_SHMW_Hnd_is_init(hnd));
    MPIU_Assert(shm_addr_ptr);

    mpi_errno = MPIU_SHMW_Seg_create_attach_templ(hnd, seg_sz,
                shm_addr_ptr, offset, MPIU_SHMW_FLAG_SHM_ATTACH);
    if(mpi_errno != MPI_SUCCESS) { MPIU_ERR_POP(mpi_errno); }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#undef FCNAME
#endif /* MPIU_SHM_WRAPPERS_H_INCLUDED */
