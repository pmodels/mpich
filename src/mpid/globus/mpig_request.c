/*
 * Globus device code:       Copyright 2005 Northern Illinois University
 * Borrowed CH3 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

#ifndef MPID_REQUEST_PREALLOC
#define MPID_REQUEST_PREALLOC 32
#endif

MPID_Request MPID_Request_direct[MPID_REQUEST_PREALLOC];
MPIU_Object_alloc_t MPID_Request_mem =
{
    0, 0, 0, 0, MPID_REQUEST, sizeof(MPID_Request), MPID_Request_direct, MPID_REQUEST_PREALLOC
};

mpig_mutex_t mpig_request_alloc_mutex;

#undef FUNCNAME
#define FUNCNAME mpig_request_alloc_init
void mpig_request_alloc_init()
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_request_alloc_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_request_alloc_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_REQ,
		       "entering"));

    mpig_mutex_construct(&mpig_request_alloc_mutex);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_REQ,
		       "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_request_alloc_init);
}
/* mpig_request_alloc_init() */


#undef FUNCNAME
#define FUNCNAME mpig_request_alloc_finalize
void mpig_request_alloc_finalize()
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_request_alloc_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_request_alloc_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_REQ,
		       "entering"));

    mpig_mutex_destruct(&mpig_request_alloc_mutex);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_REQ,
		       "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_request_alloc_finalize);
}
/* mpig_request_alloc_finalize() */


#undef FUNCNAME
#define FUNCNAME mpig_request_create
MPID_Request * mpig_request_create()
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * req;
    MPIG_STATE_DECL(MPID_STATE_mpig_request_create);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_request_create);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_REQ,
		       "entering"));

    mpig_request_alloc(&req);
    if (req != NULL)
    {
	MPIU_Assert(HANDLE_GET_MPI_KIND((req)->handle) == MPID_REQUEST);

	/* initialize the request fields to sane values */
	mpig_request_construct(req);
    }
    else
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR, "FATAL ERROR: unable to allocate a request"));
    }

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_REQ,
		       "exiting: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT, MPIG_HANDLE_VAL(req), MPIG_PTR_CAST(req)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_request_create);
    return req;
}
/* mpig_request_create() */


#undef FUNCNAME
#define FUNCNAME mpig_request_destroy
void mpig_request_destroy(MPID_Request * req)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_MPID_REQUEST_DESTROY);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_MPID_REQUEST_DESTROY);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_REQ,
		       "entering: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT, req->handle, MPIG_PTR_CAST(req)));

    MPIU_Assert(HANDLE_GET_MPI_KIND((req)->handle) == MPID_REQUEST);
    MPIU_Assert((req)->ref_count == 0);

    /* free any resources used by the request */
    mpig_request_destruct(req);

    mpig_request_free(req);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_REQ,
		       "exiting: reqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(req)));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_REQUEST_DESTROY);
}
/* mpig_request_destroy() */


/*
 * char * mpig_request_type_get_string([IN] req)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_request_state_get_string
const char * mpig_request_type_get_string(mpig_request_type_t req_type)
{
    const char * str;

    switch(req_type)
    {
	case MPIG_REQUEST_TYPE_UNDEFINED:
	    str ="MPIG_REQUEST_TYPE_UNDEFINED";
	    break;
	case MPIG_REQUEST_TYPE_INTERNAL:
	    str = "MPIG_REQUEST_TYPE_INTERNAL";
	    break;
	case MPIG_REQUEST_TYPE_RECV:
	    str = "MPIG_REQUEST_TYPE_RECV";
	    break;
	case MPIG_REQUEST_TYPE_SEND:
	    str = "MPIG_REQUEST_TYPE_SEND";
	    break;
	case MPIG_REQUEST_TYPE_RSEND:
	    str = "MPIG_REQUEST_TYPE_RSEND";
	    break;
	case MPIG_REQUEST_TYPE_SSEND:
	    str = "MPIG_REQUEST_TYPE_SSEND";
	    break;
	case MPIG_REQUEST_TYPE_BSEND:
	    str = "MPIG_REQUEST_TYPE_BSEND";
	    break;
	default:
	    str = "(unknown request type)";
    }
    
    return str;
}
/* mpig_request_type_get_string() */
