/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* preallocated grequest classes */
#ifndef MPIR_GREQ_CLASS_PREALLOC
#define MPIR_GREQ_CLASS_PREALLOC 2
#endif

MPIR_Grequest_class MPIR_Grequest_class_direct[MPIR_GREQ_CLASS_PREALLOC];

MPIR_Object_alloc_t MPIR_Grequest_class_mem = { 0, 0, 0, 0, 0, 0, MPIR_GREQ_CLASS,
    sizeof(MPIR_Grequest_class),
    MPIR_Grequest_class_direct,
    MPIR_GREQ_CLASS_PREALLOC,
    NULL, {0}
};

/* We jump through some minor hoops to manage the list of classes ourselves and
 * only register a single finalizer to avoid hitting limitations in the current
 * finalizer code.  If the finalizer implementation is ever revisited this code
 * is a good candidate for registering one callback per greq class and trimming
 * some of this logic. */
static int MPIR_Grequest_registered_finalizer = 0;
static MPIR_Grequest_class *MPIR_Grequest_class_list = NULL;

/* Any internal routines can go here.  Make them static if possible.  If they
   are used by both the MPI and PMPI versions, use PMPI_LOCAL instead of
   static; this macro expands into "static" if weak symbols are supported and
   into nothing otherwise. */
static int MPIR_Grequest_free_classes_on_finalize(void *extra_data ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Grequest_class *last = NULL;
    MPIR_Grequest_class *cur = MPIR_Grequest_class_list;

    /* FIXME MT this function is not thread safe when using fine-grained threading */
    MPIR_Grequest_class_list = NULL;
    while (cur) {
        last = cur;
        cur = last->next;
        MPIR_Handle_obj_free(&MPIR_Grequest_class_mem, last);
    }

    return mpi_errno;
}

int MPIR_Grequest_complete_impl(MPIR_Request * request_ptr)
{
    MPIR_Request_complete(request_ptr);
    return MPI_SUCCESS;
}

int MPIR_Grequest_start_impl(MPI_Grequest_query_function * query_fn,
                             MPI_Grequest_free_function * free_fn,
                             MPI_Grequest_cancel_function * cancel_fn,
                             void *extra_state, MPIR_Request ** request_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_CHKPMEM_DECL(1);

    /* MT FIXME this routine is not thread-safe in the non-global case */

    *request_ptr = MPIR_Request_create(MPIR_REQUEST_KIND__GREQUEST);
    MPIR_ERR_CHKANDJUMP1(*request_ptr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
                         "generalized request");

    MPIR_Object_set_ref(*request_ptr, 1);
    (*request_ptr)->cc_ptr = &(*request_ptr)->cc;
    MPIR_cc_set((*request_ptr)->cc_ptr, 1);
    (*request_ptr)->comm = NULL;
    MPIR_CHKPMEM_MALLOC((*request_ptr)->u.ureq.greq_fns, struct MPIR_Grequest_fns *,
                        sizeof(struct MPIR_Grequest_fns), mpi_errno, "greq_fns", MPL_MEM_GREQ);
    (*request_ptr)->u.ureq.greq_fns->U.C.cancel_fn = cancel_fn;
    (*request_ptr)->u.ureq.greq_fns->U.C.free_fn = free_fn;
    (*request_ptr)->u.ureq.greq_fns->U.C.query_fn = query_fn;
    (*request_ptr)->u.ureq.greq_fns->poll_fn = NULL;
    (*request_ptr)->u.ureq.greq_fns->wait_fn = NULL;
    (*request_ptr)->u.ureq.greq_fns->grequest_extra_state = extra_state;
    (*request_ptr)->u.ureq.greq_fns->greq_lang = MPIR_LANG__C;

    /* Add an additional reference to the greq.  One of them will be
     * released when we complete the request, and the second one, when
     * we test or wait on it. */
    MPIR_Request_add_ref((*request_ptr));

    MPIR_CHKPMEM_COMMIT();
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

/* extensions for Generalized Request redesign paper */
int MPIR_Grequest_class_create_impl(MPI_Grequest_query_function * query_fn,
                                    MPI_Grequest_free_function * free_fn,
                                    MPI_Grequest_cancel_function * cancel_fn,
                                    MPIX_Grequest_poll_function * poll_fn,
                                    MPIX_Grequest_wait_function * wait_fn,
                                    MPIX_Grequest_class * greq_class)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Grequest_class *class_ptr;

    class_ptr = (MPIR_Grequest_class *)
        MPIR_Handle_obj_alloc(&MPIR_Grequest_class_mem);
    if (!class_ptr) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                         MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                         MPI_ERR_OTHER, "**nomem",
                                         "**nomem %s", "MPIX_Grequest_class");
        goto fn_fail;
    }

    class_ptr->query_fn = query_fn;
    class_ptr->free_fn = free_fn;
    class_ptr->cancel_fn = cancel_fn;
    class_ptr->poll_fn = poll_fn;
    class_ptr->wait_fn = wait_fn;

    MPIR_Object_set_ref(class_ptr, 1);

    if (MPIR_Grequest_class_list == NULL) {
        class_ptr->next = NULL;
    } else {
        class_ptr->next = MPIR_Grequest_class_list;
    }
    MPIR_Grequest_class_list = class_ptr;
    if (!MPIR_Grequest_registered_finalizer) {
        /* must run before (w/ higher priority than) the handle check
         * finalizer in order avoid being flagged as a leak */
        MPIR_Add_finalize(&MPIR_Grequest_free_classes_on_finalize,
                          NULL, MPIR_FINALIZE_CALLBACK_HANDLE_CHECK_PRIO + 1);
        MPIR_Grequest_registered_finalizer = 1;
    }

    MPIR_OBJ_PUBLISH_HANDLE(*greq_class, class_ptr->handle);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Grequest_class_allocate_impl(MPIX_Grequest_class greq_class,
                                      void *extra_state, MPIR_Request ** p_request_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *lrequest_ptr;
    MPIR_Grequest_class *class_ptr;

    MPIR_Grequest_class_get_ptr(greq_class, class_ptr);
    mpi_errno = MPIR_Grequest_start_impl(class_ptr->query_fn, class_ptr->free_fn,
                                         class_ptr->cancel_fn, extra_state, &lrequest_ptr);
    if (mpi_errno == MPI_SUCCESS) {
        lrequest_ptr->u.ureq.greq_fns->poll_fn = class_ptr->poll_fn;
        lrequest_ptr->u.ureq.greq_fns->wait_fn = class_ptr->wait_fn;
        lrequest_ptr->u.ureq.greq_fns->greq_class = greq_class;
        *p_request_ptr = lrequest_ptr;
    }

    return mpi_errno;
}
