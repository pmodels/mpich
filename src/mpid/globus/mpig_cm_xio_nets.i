/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#define MPIG_CM_XIO_NET_MAX_KEY_NAME_SIZE 128

/**********************************************************************************************************************************
					     BEGIN CONSTRUCTOR / DESTRUCTOR SECTION
**********************************************************************************************************************************/
#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

#define mpig_cm_xio_net_cm_construct(cm_)	\
{						\
    (cm_)->cmu.xio.key_name = NULL;		\
    (cm_)->cmu.xio.topology_levels = 0;		\
    (cm_)->cmu.xio.driver_name = NULL;		\
    (cm_)->cmu.xio.contact_string = NULL;	\
    (cm_)->cmu.xio.available = FALSE;		\
}

#define mpig_cm_xio_net_cm_destruct(cm_)					\
{										\
    (cm_)->cmu.xio.key_name = MPIG_INVALID_PTR;					\
    (cm_)->cmu.xio.topology_levels = 0;						\
    MPIU_Free((cm_)->cmu.xio.driver_name);					\
    (cm_)->cmu.xio.driver_name = MPIG_INVALID_PTR;				\
    /* NOTE: the contact string is allocated internally by globus, so it must	\
       be freed using globus_libc_free() instead of MPIU_Free(). */		\
    globus_libc_free((cm_)->cmu.xio.contact_string);				\
    (cm_)->cmu.xio.contact_string = MPIG_INVALID_PTR;				\
    (cm_)->cmu.xio.available = FALSE;						\
}

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
/**********************************************************************************************************************************
					      END CONSTRUCTOR / DESTRUCTOR SECTION
**********************************************************************************************************************************/


#if !defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS)

/*
 * prototypes for XIO communication method functions
 */
static int mpig_cm_xio_net_init(mpig_cm_t * cm, int * argc_p, char *** argv_p);

static int mpig_cm_xio_net_wan_init(mpig_cm_t * cm, int * argc_p, char *** argv_p);

static int mpig_cm_xio_net_lan_init(mpig_cm_t * cm, int * argc_p, char *** argv_p);

static int mpig_cm_xio_net_san_init(mpig_cm_t * cm, int * argc_p, char *** argv_p);

static int mpig_cm_xio_net_default_init(mpig_cm_t * cm, int * argc_p, char *** argv_p);

static int mpig_cm_xio_net_finalize(mpig_cm_t * cm);

static int mpig_cm_xio_net_add_contact_info(mpig_cm_t * cm, mpig_bc_t * bc);

static int mpig_cm_xio_net_construct_vc_contact_info(mpig_cm_t * cm, struct mpig_vc * vc);

/* static void mpig_cm_xio_net_destruct_vc_contact_info(mpig_cm_t * cm, struct mpig_vc * vc); */

static int mpig_cm_xio_net_select_comm_method(mpig_cm_t * cm, struct mpig_vc * vc, bool_t * selected);

static int mpig_cm_xio_net_get_vc_compatability(mpig_cm_t * cm, const mpig_vc_t * vc1, const mpig_vc_t * vc2,
    unsigned levels_in, unsigned * levels_out);


/*
 * communication method and virtual table structure definitions
 */
MPIG_STATIC mpig_cm_vtable_t mpig_cm_xio_net_wan_vtable =
{
    mpig_cm_xio_net_wan_init,
    mpig_cm_xio_net_finalize,
    mpig_cm_xio_net_add_contact_info,
    mpig_cm_xio_net_construct_vc_contact_info,
    NULL, /* mpig_cm_xio_net_destruct_vc_contact_info, */
    mpig_cm_xio_net_select_comm_method,
    mpig_cm_xio_net_get_vc_compatability,
    mpig_cm_vtable_last_entry
};

mpig_cm_t mpig_cm_xio_net_wan =
{
    MPIG_CM_TYPE_XIO,
    "XIO WAN",
    &mpig_cm_xio_net_wan_vtable
};

MPIG_STATIC mpig_cm_vtable_t mpig_cm_xio_net_lan_vtable =
{
    mpig_cm_xio_net_lan_init,
    mpig_cm_xio_net_finalize,
    mpig_cm_xio_net_add_contact_info,
    mpig_cm_xio_net_construct_vc_contact_info,
    NULL, /* mpig_cm_xio_net_destruct_vc_contact_info, */
    mpig_cm_xio_net_select_comm_method,
    mpig_cm_xio_net_get_vc_compatability,
    mpig_cm_vtable_last_entry
};

mpig_cm_t mpig_cm_xio_net_lan =
{
    MPIG_CM_TYPE_XIO,
    "XIO LAN",
    &mpig_cm_xio_net_lan_vtable
};

MPIG_STATIC mpig_cm_vtable_t mpig_cm_xio_net_san_vtable =
{
    mpig_cm_xio_net_san_init,
    mpig_cm_xio_net_finalize,
    mpig_cm_xio_net_add_contact_info,
    mpig_cm_xio_net_construct_vc_contact_info,
    NULL, /* mpig_cm_xio_net_destruct_vc_contact_info, */
    mpig_cm_xio_net_select_comm_method,
    mpig_cm_xio_net_get_vc_compatability,
    mpig_cm_vtable_last_entry
};

mpig_cm_t mpig_cm_xio_net_san =
{
    MPIG_CM_TYPE_XIO,
    "XIO SAN",
    &mpig_cm_xio_net_san_vtable
};

MPIG_STATIC mpig_cm_vtable_t mpig_cm_xio_net_default_vtable =
{
    mpig_cm_xio_net_default_init,
    mpig_cm_xio_net_finalize,
    mpig_cm_xio_net_add_contact_info,
    mpig_cm_xio_net_construct_vc_contact_info,
    NULL, /* mpig_cm_xio_net_destruct_vc_contact_info, */
    mpig_cm_xio_net_select_comm_method,
    mpig_cm_xio_net_get_vc_compatability,
    mpig_cm_vtable_last_entry
};

mpig_cm_t mpig_cm_xio_net_default =
{
    MPIG_CM_TYPE_XIO,
    "XIO (Default)",
    &mpig_cm_xio_net_default_vtable
};


#else /* defined(MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS) */


/*
 * <mpi_errno> mpig_cm_xio_net_wan_init([IN/MOD] cm, [IN/OUT] argc_p, [IN/OUT] argv_p)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_net_init
static int mpig_cm_xio_net_init(mpig_cm_t * const cm, int * const argc_p, char *** const argv_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    char env_name[MPIG_CM_XIO_NET_MAX_KEY_NAME_SIZE];
    char * env_str;
    char * driver_args = NULL;
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_net_init)

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_net_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "entering: cm=" MPIG_PTR_FMT
	", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));

    /* intialize the XIO communication module if it hasn't already been initialized */
    mpi_errno = mpig_cm_xio_module_init();
    if (mpi_errno)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT,
	    "ERROR: initialization of the XIO commmunication module failed"));
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|module_init");
	goto error_module_init;
    }

    MPIU_Snprintf(env_name, MPIG_CM_XIO_NET_MAX_KEY_NAME_SIZE, "MPIG_%s_NET", cm->cmu.xio.key_name);
    env_str = globus_libc_getenv(env_name);
    if (env_str == NULL)
    {
	if (cm->cmu.xio.driver_name == NULL)
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_CM, "no driver specified; method disabled: cm=" MPIG_PTR_FMT ", cm_name=%s",
		MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));
	    goto fn_return;
	}
    }
    else
    {
	
        env_str = MPIU_Strdup(env_str);
        if(env_str == NULL)
        {
	    /* FIXME: add debug and error messages */
            goto error_driver_name_malloc;
        }
        driver_args = strchr(env_str, ':');
        if(driver_args != NULL)
        {
            *driver_args++ = '\0';
        }
	if (cm->cmu.xio.driver_name) MPIU_Free(cm->cmu.xio.driver_name);
	cm->cmu.xio.driver_name = env_str;
    }
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_CEMT, "initializing driver: driver_name=%s, driver_args=%s",
	cm->cmu.xio.driver_name, driver_args));

    /* build stack of communication drivers */
    grc = globus_xio_stack_init(&cm->cmu.xio.stack, NULL);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: call to %s() failed: "
	    "cm=" MPIG_PTR_FMT ", cm_name=%s, msg=%s", "globus_xio_stack_init", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm),
	    mpig_get_globus_error_msg(grc)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_stack_init", "**globus|cm_xio|xio_stack_init %s",
	    mpig_get_globus_error_msg(grc));
	goto error_stack_create;
    }   /* --END ERROR HANDLING-- */

    grc = globus_xio_driver_load(cm->cmu.xio.driver_name, &cm->cmu.xio.driver);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: call to %s() failed: cm="
	    MPIG_PTR_FMT ", cm_name=%s, driver=%s, msg=%s", "globus_xio_driver_load", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm),
	    cm->cmu.xio.driver_name, mpig_get_globus_error_msg(grc)));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_driver_load_transport",
	    "**globus|cm_xio|xio_driver_load_transport %s %s", cm->cmu.xio.driver_name, mpig_get_globus_error_msg(grc));
	goto error_driver_load;
    }   /* --END ERROR HANDLING-- */

    grc = globus_xio_stack_push_driver(cm->cmu.xio.stack, cm->cmu.xio.driver);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: call to %s() failed: cm="
	    MPIG_PTR_FMT ", cm_name=%s, driver=%s, msg=%s", "globus_xio_stack_push_driver", MPIG_PTR_CAST(cm),
	    mpig_cm_get_name(cm), cm->cmu.xio.driver_name, mpig_get_globus_error_msg(grc)));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_stack_push_driver",
	    "**globus|cm_xio|xio_stack_push_driver %s %s", cm->cmu.xio.driver_name, mpig_get_globus_error_msg(grc));
	goto error_driver_push;
    }   /* --END ERROR HANDLING-- */

#   if XXX_SECURE
    {
	globus_xio_driver_t gsi_driver;
	
        grc = globus_xio_driver_load("gsi", &gsi_driver);
	if (grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: call to %s() failed: "
		"cm=" MPIG_PTR_FMT ", cm_name=%s, driver=%s, msg=%s", "globus_xio_driver_load", MPIG_PTR_CAST(cm),
		mpig_cm_get_name(cm), "gsi", mpig_get_globus_error_msg(grc)));
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CEMT, "ERROR: call to %s() failed: driver=%s, msg=%s",
		"globus_xio_driver_load", "gsi", mpig_get_globus_error_msg(grc)));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_driver_load_transform",
	    "**globus|cm_xio|xio_driver_load_transform %s %s", "gsi", mpig_get_globus_error_msg(grc));
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

        grc = globus_xio_stack_push_driver(xio_l_fallback_info.stack, gsi_driver);
	if (grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: call to %s() failed: "
		"cm=" MPIG_PTR_FMT ", cm_name=%s, driver=%s, msg=%s", "globus_xio_stack_push_driver", MPIG_PTR_CAST(cm),
		mpig_cm_get_name(cm), "gsi", mpig_get_globus_error_msg(grc)));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_stack_push_driver",
		"**globus|cm_xio|xio_stack_push_driver %s %s", "gsi", mpig_get_globus_error_msg(grc));
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
    }
#   endif

    /* initialize the XIO attributes structure, and set any attributes appropriate for the protocol or specified by the user */
    grc = globus_xio_attr_init(&cm->cmu.xio.attrs);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: call to %s() failed: cm="
	    MPIG_PTR_FMT ", cm_name=%s, msg=%s", "globus_xio_attr_init", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm),
	    mpig_get_globus_error_msg(grc)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_attr_init", "**globus|cm_xio|xio_attr_init %s",
	    mpig_get_globus_error_msg(grc));
	goto error_attr_init;
    }   /* --END ERROR HANDLING-- */

    if (strcmp(cm->cmu.xio.driver_name, "tcp") == 0)
    {
	/* set TCP options and parameters */
	grc = globus_xio_attr_cntl(cm->cmu.xio.attrs, cm->cmu.xio.driver, GLOBUS_XIO_TCP_SET_NODELAY, GLOBUS_TRUE);
	if (grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: call to %s() failed: "
		"cm=" MPIG_PTR_FMT ", cm_name=%s, attr=%s, msg=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm),
		"globus_xio_attr_cntl", "TCP_SET_NODELAY", mpig_get_globus_error_msg(grc)));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_attr_cntl",
		"**globus|cm_xio|xio_attr_cntl %s %s", "TCP_SET_NODELAY", mpig_get_globus_error_msg(grc));
	    goto error_set_attrs;
	}   /* --END ERROR HANDLING-- */
    }
    
    if(driver_args != NULL)
    {
#       if defined(NEW_XIO)
	{
            /* NOTE: [JB] i dont think we care about an error here.  these are just supossed to be hints
	     *
	     * FIXME: [BRT] the user should, at a minimum, be warned if the parameters they specified could not be set
	     */
            globus_xio_attr_cntl(cm->cmu.xio.attrs, cm->cmu.xio.driver, GLOBUS_XIO_SET_STRING_OPTIONS, driver_args);
	}
#	else
	{
	    /***** XXX: FIXME: optimization parameters to the attr ******/
	}
#       endif
    }

    if (mpig_cm_xio_tcp_buf_size > 0)
    {
	grc = globus_xio_attr_cntl(cm->cmu.xio.attrs, cm->cmu.xio.driver, GLOBUS_XIO_TCP_SET_SNDBUF, mpig_cm_xio_tcp_buf_size);
	if (grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CEMT, "ERROR: call to %s() failed: attr=%s, msg=%s",
		"globus_xio_attr_cntl", "TCP_SET_SNDBUF", mpig_get_globus_error_msg(grc)));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_attr_cntl",
		"**globus|cm_xio|xio_attr_cntl %s %s", "TCP_SET_SNDBUF", mpig_get_globus_error_msg(grc));
	    goto error_set_attrs;
	}   /* --END ERROR HANDLING-- */

	grc = globus_xio_attr_cntl(cm->cmu.xio.attrs, cm->cmu.xio.driver, GLOBUS_XIO_TCP_SET_RCVBUF, mpig_cm_xio_tcp_buf_size);
	if (grc)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CEMT, "ERROR: call to %s() failed: attr=%s, msg=%s",
		"globus_xio_attr_cntl", "TCP_SET_RCVBUF", mpig_get_globus_error_msg(grc)));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_attr_cntl",
		"**globus|cm_xio|xio_attr_cntl %s %s", "TCP_SET_RCVBUF", mpig_get_globus_error_msg(grc));
	    goto error_set_attrs;
	}   /* --END ERROR HANDLING-- */
    }
    
    /* establish server to start listening for new connections */
    grc = globus_xio_server_create(&cm->cmu.xio.server, cm->cmu.xio.attrs, cm->cmu.xio.stack);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: call to %s() failed: cm="
	    MPIG_PTR_FMT ", cm_name=%s, msg=%s", "globus_xio_server_create", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm),
	    mpig_get_globus_error_msg(grc)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_create",
	    "**globus|cm_xio|server_create %s", mpig_get_globus_error_msg(grc));
	goto error_server_create;
    }   /* --END ERROR HANDLING-- */

    grc = globus_xio_server_get_contact_string(cm->cmu.xio.server, &cm->cmu.xio.contact_string);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: call to %s() failed: cm="
	    MPIG_PTR_FMT ", cm_name=%s, msg=%s", "globus_xio_server_get_contact_string", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm),
	    mpig_get_globus_error_msg(grc)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_get_cs",
	    "**globus|cm_xio|server_get_cs %s", mpig_get_globus_error_msg(grc));
	goto error_cs;
    }   /* --END ERROR HANDLING-- */

    mpi_errno = mpig_cm_xio_server_listen(cm);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: the server failed to "
	    "register a new accept operation: cm=" MPIG_PTR_FMT ", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|server_listen",
	    "**globus|cm_xio|server_listen %s %p", mpig_cm_get_name(cm), cm);
	goto error_listen;
    }	/* --END ERROR HANDLING-- */

    cm->cmu.xio.available = GLOBUS_TRUE;

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "communication method initialized and enabled; "
	"server listening:  cm=" MPIG_PTR_FMT ", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "exiting: cm=" MPIG_PTR_FMT
	", cm_name=%s, mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(cm), mpig_cm_get_name(cm), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_net_init);
    return mpi_errno;

    {   /* --BEGIN ERROR HANDLING-- */
	/* FIXME: check error codes and add any errors to the error reporting stack */
      error_listen:
	globus_libc_free(cm->cmu.xio.contact_string);
	
      error_cs:
	globus_xio_server_close(cm->cmu.xio.server);
	
      error_server_create:
      error_set_attrs:
	globus_xio_attr_destroy(cm->cmu.xio.attrs);

      error_attr_init:
      error_driver_push:
	globus_xio_driver_unload(cm->cmu.xio.driver);

      error_driver_load:
	globus_xio_stack_destroy(cm->cmu.xio.stack);

      error_stack_create:
	MPIU_Free(cm->cmu.xio.driver_name);

      error_driver_name_malloc:
      error_module_init:
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_net_init() */


/*
 * <mpi_errno> mpig_cm_xio_net_wan_init([IN/MOD] cm, [IN/OUT] argc_p, [IN/OUT] argv_p)
 *
 * see the documentation of the mpig_cm vtable routines in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_net_wan_init
static int mpig_cm_xio_net_wan_init(mpig_cm_t * cm, int * argc_p, char *** argv_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_net_wan_init)

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_net_wan_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "entering: cm=" MPIG_PTR_FMT
	", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));

    mpig_cm_xio_net_cm_construct(cm);
    cm->cmu.xio.key_name = "XIO_WAN";
    cm->cmu.xio.topology_levels = MPIG_TOPOLOGY_LEVEL_WAN_MASK;
    
    mpi_errno = mpig_cm_xio_net_init(cm, argc_p, argv_p);
    if (mpi_errno)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: initialization of the "
	    "communication method failed: cm=" MPIG_PTR_FMT ", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|nets_init", "**globus|cm_xio|nets_init %p %s",
	    cm, mpig_cm_get_name(cm));
	goto fn_fail;
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "exiting: cm=" MPIG_PTR_FMT
	"cm_name=%s, available=%s, mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(cm), mpig_cm_get_name(cm),
	MPIG_BOOL_STR(cm->cmu.xio.available), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_net_wan_init);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_net_wan_init() */


/*
 * <mpi_errno> mpig_cm_xio_net_lan_init([IN/MOD] cm, [IN/OUT] argc_p, [IN/OUT] argv_p)
 *
 * see the documentation of the mpig_cm vtable routines in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_net_lan_init
static int mpig_cm_xio_net_lan_init(mpig_cm_t * cm, int * argc_p, char *** argv_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_net_lan_init)

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_net_lan_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "entering: cm=" MPIG_PTR_FMT
	", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));

    mpig_cm_xio_net_cm_construct(cm);
    cm->cmu.xio.key_name = "XIO_LAN";
    cm->cmu.xio.topology_levels = MPIG_TOPOLOGY_LEVEL_LAN_MASK;
    
    mpi_errno = mpig_cm_xio_net_init(cm, argc_p, argv_p);
    if (mpi_errno)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: initialization of the "
	    "communication method failed: cm=" MPIG_PTR_FMT ", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|nets_init", "**globus|cm_xio|nets_init %p %s",
	    cm, mpig_cm_get_name(cm));
	goto fn_fail;
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "exiting: cm=" MPIG_PTR_FMT
	"cm_name=%s, available=%s, mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(cm), mpig_cm_get_name(cm),
	MPIG_BOOL_STR(cm->cmu.xio.available), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_net_lan_init);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_net_lan_init() */


/*
 * <mpi_errno> mpig_cm_xio_net_san_init([IN/MOD] cm, [IN/OUT] argc_p, [IN/OUT] argv_p)
 *
 * see the documentation of the mpig_cm vtable routines in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_net_san_init
static int mpig_cm_xio_net_san_init(mpig_cm_t * cm, int * argc_p, char *** argv_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_net_san_init)

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_net_san_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "entering: cm=" MPIG_PTR_FMT
	", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));

    mpig_cm_xio_net_cm_construct(cm);
    cm->cmu.xio.key_name = "XIO_SAN";
    cm->cmu.xio.topology_levels = MPIG_TOPOLOGY_LEVEL_SAN_MASK;
    
    mpi_errno = mpig_cm_xio_net_init(cm, argc_p, argv_p);
    if (mpi_errno)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: initialization of the "
	    "communication method failed: cm=" MPIG_PTR_FMT ", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|nets_init", "**globus|cm_xio|nets_init %p %s",
	    cm, mpig_cm_get_name(cm));
	goto fn_fail;
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "exiting: cm=" MPIG_PTR_FMT
	"cm_name=%s, available=%s, mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(cm), mpig_cm_get_name(cm),
	MPIG_BOOL_STR(cm->cmu.xio.available), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_net_san_init);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_net_san_init() */

/*
 * <mpi_errno> mpig_cm_xio_net_default_init([IN/MOD] cm, [IN/OUT] argc_p, [IN/OUT] argv_p)
 *
 * see the documentation of the mpig_cm vtable routines in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_net_default_init
static int mpig_cm_xio_net_default_init(mpig_cm_t * cm, int * argc_p, char *** argv_p)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_net_default_init)

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_net_default_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "entering: cm=" MPIG_PTR_FMT
	", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));

    mpig_cm_xio_net_cm_construct(cm);
    cm->cmu.xio.key_name = "XIO_DEFAULT";
    cm->cmu.xio.driver_name = MPIU_Strdup("tcp");
    cm->cmu.xio.topology_levels = MPIG_TOPOLOGY_LEVEL_WAN_MASK | MPIG_TOPOLOGY_LEVEL_LAN_MASK | MPIG_TOPOLOGY_LEVEL_SAN_MASK;
    
    mpi_errno = mpig_cm_xio_net_init(cm, argc_p, argv_p);
    if (mpi_errno)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: initialization of the "
	    "communication method failed: cm=" MPIG_PTR_FMT ", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|nets_init", "**globus|cm_xio|nets_init %p %s",
	    cm, mpig_cm_get_name(cm));
	goto fn_fail;
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "exiting: cm=" MPIG_PTR_FMT
	"cm_name=%s, available=%s, mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(cm), mpig_cm_get_name(cm),
	MPIG_BOOL_STR(cm->cmu.xio.available), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_net_default_init);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_net_default_init() */
    

/*
 * <mpi_errno> mpig_cm_xio_net_finalize([IN/MOD] cm)
 *
 * see the documentation of the mpig_cm vtable routines in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_net_finalize
static int mpig_cm_xio_net_finalize(mpig_cm_t * cm)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t grc;
    int mrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_net_finalize)

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_net_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM, "entering: cm=" MPIG_PTR_FMT ", cm_name=%s",
	MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));

    if (!cm->cmu.xio.available)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "XIO communication method not active; skipping finalize: "
	    "cm=" MPIG_PTR_FMT "cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));
	goto fn_return;
    }

    /* disable the connection server */
    grc = globus_xio_server_close(cm->cmu.xio.server);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: call to %s() failed: cm="
	    MPIG_PTR_FMT ", cm_name=%s, msg=%s", "globus_xio_server_close", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm),
	    mpig_get_globus_error_msg(grc)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_close_server",
	    "**globus|cm_xio|xio_close_server %s", mpig_get_globus_error_msg(grc));
    }   /* --END ERROR HANDLING-- */


    /* NOTE: prior to calling this routine, MPID_Finalize() dissolved all communicators, so there should not be any external VC
     * references outstanding.  that is not to say that the VCs are completely inactive.  some may be going through the close
     * protocol, while others may still be receiving messages from the remote process.  at this point message reception from a
     * remote process is only valid, meaning the application is a valid MPI application, if the remote process intends to cancel
     * those messages; otherwise, the messages will go unmatched, possibly causing the remote process to hang.  for example, a
     * rendezvous message arriving at the is point for which no matching receive exists would never be sent a clear-to-send.  as
     * a result, the remote process would hang waiting for the CTS, and this local process would hang wait for the ACK to its
     * close request.
     *
     * FIXME: the example above might be solved by having the RTS receive message handler detect the state of the connection and
     * send a NAK, allowing the remote process to return an error to the user.
     */
    
    /* wait for all of the connections associated with this communication method to close.  in reality, vc_wait_list_empty()
       waits for all connections to close regardless of which XIO communication method they are using; however, it is necessary
       to call this routine during the shutdown of each method since new connections could continue to arrive for any method
       whose server has not been closed. */
    mpig_cm_xio_vc_list_wait_empty();

    /* clean up XIO data structures used by this communication method */
    grc = globus_xio_stack_destroy(cm->cmu.xio.stack);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: call to %s() failed: cm="
	    MPIG_PTR_FMT ", cm_name=%s, msg=%s", "globus_xio_stack_destroy", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm),
	    mpig_get_globus_error_msg(grc)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_stack_destroy",
	    "**globus|cm_xio|xio_stack_destroy %s", mpig_get_globus_error_msg(grc));
    }   /* --END ERROR HANDLING-- */

    /* unload the XIO driver.  MT-NOTE: this routine is guaranteed not return until all callbacks associated with it have
       completed. */
    grc = globus_xio_driver_unload(cm->cmu.xio.driver);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: call to %s() failed: cm="
	    MPIG_PTR_FMT ", cm_name=%s, msg=%s", "globus_xio_driver_unload", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm),
	    mpig_get_globus_error_msg(grc)));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_driver_unload_transport",
	    "**globus|cm_xio|xio_driver_destroy_unload_transport %s %s", cm->cmu.xio.driver_name, mpig_get_globus_error_msg(grc));
    }   /* --END ERROR HANDLING-- */

    grc = globus_xio_attr_destroy(cm->cmu.xio.attrs);
    if (grc)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "ERROR: call to %s() failed: cm="
	    MPIG_PTR_FMT ", cm_name=%s, msg=%s", "globus_xio_attr_destroy", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm),
	    mpig_get_globus_error_msg(grc)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|xio_attr_destroy",
	    "**globus|cm_xio|xio_attr_destroy %s", mpig_get_globus_error_msg(grc));
    }   /* --END ERROR HANDLING-- */

    mpig_cm_xio_net_cm_destruct(cm);

    mrc = mpig_cm_xio_module_finalize();
    if (mpi_errno)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT,
	    "ERROR: finalization of the XIO commmunication module failed"));
	MPIU_ERR_SET(mrc, MPI_ERR_OTHER, "**globus|cm_xio|module_init");
	MPIU_ERR_ADD(mpi_errno, mrc);
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM, "exiting: cmp=" MPIG_PTR_FMT "cm=%s, mpi_errno="
	MPIG_ERRNO_FMT, MPIG_PTR_CAST(cm), mpig_cm_get_name(cm), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_net_finalize);
    return mpi_errno;
}
/* mpig_cm_xio_net_finalize() */


/*
 * <mpi_errno> mpig_cm_xio_net_add_contact_info([IN] cm, [IN/MOD] bc)
 *
 * see the documentation of the mpig_cm vtable routines in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_net_add_contact_info
static int mpig_cm_xio_net_add_contact_info(mpig_cm_t * const cm, mpig_bc_t * const bc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    static bool_t core_entries_added = FALSE;
    char key[MPIG_CM_XIO_NET_MAX_KEY_NAME_SIZE];
    char uint_str[10];
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_net_add_contact_info);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_net_add_contact_info);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "entering: cm=" MPIG_PTR_FMT
	", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));

    if(cm->cmu.xio.available == FALSE)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "communication method disabled; skipping the addition of "
	    "contact information: cm=" MPIG_PTR_FMT ", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));
	goto fn_return;
    }
    
    if (core_entries_added == FALSE)
    {
    
	MPIU_Snprintf(uint_str, (size_t) 10, "%u", (unsigned) MPIG_CM_XIO_PROTO_VERSION);
	mpi_errno = mpig_bc_add_contact(bc, "CM_XIO_PROTO_VERSION", uint_str);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
	    "**globus|bc_add_contact %s", "CM_XIO_PROTO_VERSION");

	mpi_errno = mpig_bc_add_contact(bc, "CM_XIO_CONTACT_STRING", cm->cmu.xio.contact_string);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER,
	    "**globus|bc_add_contact",
	    "**globus|bc_add_contact %s", "CM_XIO_CONTACT_STRING");

	MPIU_Snprintf(uint_str, (size_t) 10, "%u", (unsigned) GLOBUS_DC_FORMAT_LOCAL);
	mpi_errno = mpig_bc_add_contact(bc, "CM_XIO_DC_FORMAT", uint_str);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
	    "**globus|bc_add_contact %s", "CM_XIO_DC_FORMAT");

	mpi_errno = mpig_bc_add_contact(bc, "CM_XIO_DC_ENDIAN", MPIG_ENDIAN_STR(MPIG_MY_ENDIAN));
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
	    "**globus|bc_add_contact %s", "CM_XIO_DC_ENDIAN");

	core_entries_added = TRUE;
    }
    
    MPIU_Snprintf(key, MPIG_CM_XIO_NET_MAX_KEY_NAME_SIZE, "MPIG_XIO_%s_PROTO_NAME", cm->cmu.xio.key_name);
    mpi_errno = mpig_bc_add_contact(bc, key, cm->cmu.xio.driver_name);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact", "**globus|bc_add_contact %s", key);

    MPIU_Snprintf(key, MPIG_CM_XIO_NET_MAX_KEY_NAME_SIZE, "MPIG_XIO_%s_CONTACT_STRING", cm->cmu.xio.key_name);
    mpi_errno = mpig_bc_add_contact(bc, key, cm->cmu.xio.contact_string);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact", "**globus|bc_add_contact %s", key);
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: cm=" MPIG_PTR_FMT ", cm=%s, mpi_errno=" MPIG_ERRNO_FMT,
	MPIG_PTR_CAST(cm), mpig_cm_get_name(cm), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_net_add_contact_info);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_net_add_contact_info() */


/*
 * <mpi_errno> mpig_cm_xio_net_construct_vc_contact_info([IN] cm, [IN/MOD] vc)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_net_construct_vc_contact_info
static int mpig_cm_xio_net_construct_vc_contact_info(mpig_cm_t * cm, mpig_vc_t * const vc)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    unsigned level;
    int max_level;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_net_construct_vc_contact_info);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_net_construct_vc_contact_info);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "entering: cm=" MPIG_PTR_FMT
	", cm_name=%s, vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(cm), mpig_cm_get_name(cm), MPIG_PTR_CAST(vc)));

    if(cm->cmu.xio.available == FALSE)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "communication method disabled; skipping the construction "
	    "of contact information: cm=" MPIG_PTR_FMT ", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));
	goto fn_return;
    }
    
    /* NOTE: the XIO contact information is only used if the XIO CM is selected to handle the VC.  therefore, all such contact
       information is stored in the CM union.  to prevent collisions with data from other CMs, the CM must not store information
       into that union until it decides to claim responsibility for the VC.  thus, the extraction of this information from the
       business card is performed in mpig_cm_xio_select_comm_method(). */

    /* set the topology information.  NOTE: this may seem a bit wacky since the WAN, LAN and SAN levels may be set even if the
       XIO module is not responsible for the VC; however, the topology information is defined such that a level set if it is
       _possible_ for the module to perform the communication regardless of whether it does so or not. */
    vc->cms.topology_levels |= cm->cmu.xio.topology_levels;

    level = vc->cms.topology_levels;
    max_level = 0;
    while(level != 0)
    {
	max_level += 1;
	level >>= 1;
    }
    
    if (vc->cms.topology_num_levels < max_level)
    {
	vc->cms.topology_num_levels = max_level;
    }
		
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "exiting:  cm=" MPIG_PTR_FMT
	", cm_name=%s, vc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(cm), mpig_cm_get_name(cm),
	MPIG_PTR_CAST(vc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_net_construct_vc_contact_info);
    return mpi_errno;
}
/* mpig_cm_xio_net_construct_vc_contact_info() */


/*
 * <mpi_errno> mpig_cm_xio_net_select_comm_method([IN] cm, [IN/MOD] vc, [OUT] selected)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_net_select_comm_method
static int mpig_cm_xio_net_select_comm_method(mpig_cm_t * const cm, mpig_vc_t * const vc, bool_t * const selected)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    char key[MPIG_CM_XIO_NET_MAX_KEY_NAME_SIZE];
    mpig_vc_t * my_vc;
    unsigned levels_out;
    mpig_bc_t * bc;
    char * version_str = NULL;
    char * contact_str = NULL;
    char * format_str = NULL;
    char * endian_str = NULL;
    char * driver_name = NULL;
    int format;
    int version;
    mpig_endian_t endian;
    bool_t found;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_net_select_comm_method);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_net_select_comm_method);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "entering: cm=" MPIG_PTR_FMT
	", cm_name=%s, vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(cm), mpig_cm_get_name(cm), MPIG_PTR_CAST(vc)));

    *selected = FALSE;
    
    if(cm->cmu.xio.available == FALSE)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "communication method disabled; skipping the selection "
	    "process: cm=" MPIG_PTR_FMT ", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));
	goto fn_return;
    }
    
    mpig_pg_get_vc(mpig_process.my_pg, mpig_process.my_pg_rank, &my_vc);
    mpig_cm_xio_net_get_vc_compatability(cm, vc, my_vc, ~((unsigned) 0), &levels_out);
    if (levels_out == 0)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "communication method of remote VC is not compatible; "
	    "skipping the selection process: cm=" MPIG_PTR_FMT ", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));
	goto fn_return;
    }
    
    bc = mpig_vc_get_bc(vc);

    if (mpig_vc_get_cm(vc) == NULL)
    {
	/* Get protocol version number and check that it is compatible with this module */
	mpi_errno = mpig_bc_get_contact(bc, "CM_XIO_PROTO_VERSION", &version_str, &found);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact",
	    "**globus|bc_get_contact %s", "CM_XIO_PROTO_VERSION");
	if (!found) goto fn_return;

	rc = sscanf(version_str, "%d", &version);
	MPIU_ERR_CHKANDJUMP((rc != 1), mpi_errno, MPI_ERR_INTERN, "**keyval");
	if (version != MPIG_CM_XIO_PROTO_VERSION) goto fn_return;

	/* Get format of basic datatypes */
	mpi_errno = mpig_bc_get_contact(bc, "CM_XIO_DC_FORMAT", &format_str, &found);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact",
	    "**globus|bc_get_contact %s", "CM_XIO_DC_FORMAT");
	if (!found) goto fn_return;
	
	rc = sscanf(format_str, "%d", &format);
	MPIU_ERR_CHKANDJUMP((rc != 1), mpi_errno, MPI_ERR_INTERN, "**keyval");

	/* Get endianess of remote system */
	mpi_errno = mpig_bc_get_contact(bc, "CM_XIO_DC_ENDIAN", &endian_str, &found);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact",
	    "**globus|bc_get_contact %s", "CM_XIO_DC_ENDIAN");
	if (!found) goto fn_return;

	endian = (strcmp(endian_str, "little") == 0) ? MPIG_ENDIAN_LITTLE : MPIG_ENDIAN_BIG;
	
        /* verify that both sides want to use the same protocol.  this is done here rather than below, because if a connection is
	   already formed, the protocols must be compatable. */
	MPIU_Snprintf(key, MPIG_CM_XIO_NET_MAX_KEY_NAME_SIZE, "MPIG_XIO_%s_PROTO_NAME", cm->cmu.xio.key_name);
        mpi_errno = mpig_bc_get_contact(bc, key, &driver_name, &found);
        MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact", "**globus|bc_get_contact %s", key);
        if(!found) goto fn_return;
        if(strcmp(cm->cmu.xio.driver_name, driver_name) != 0) goto fn_return;

	/* initialize CM XIO fields in the VC */
	mpig_cm_xio_vc_construct(vc);
	mpig_vc_set_cm(vc, cm);
	mpig_cm_xio_vc_set_endian(vc, endian);
	mpig_cm_xio_vc_set_data_format(vc, format);
    }

    if (mpig_vc_get_cm_module_type(vc) == MPIG_CM_TYPE_XIO)
    {
	char * cs;
	
	/* Get the contact string */
	MPIU_Snprintf(key, MPIG_CM_XIO_NET_MAX_KEY_NAME_SIZE, "MPIG_XIO_%s_CONTACT_STRING", cm->cmu.xio.key_name);
	mpi_errno = mpig_bc_get_contact(bc, key, &contact_str, &found);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact", "**globus|bc_get_contact %s", key);
	if (!found) goto fn_return;

	/* add the contact string to the VC */
	cs = MPIU_Strdup(contact_str);
	MPIU_ERR_CHKANDJUMP1((cs == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "XIO contact string");
	mpig_cm_xio_vc_set_contact_string(vc, cs);

	/* set the selected flag to indicate that the XIO communication module has accepted responsibility for the VC */
	*selected = TRUE;
    }

  fn_return:
    if (version_str != NULL) mpig_bc_free_contact(version_str);
    if (contact_str != NULL) mpig_bc_free_contact(contact_str);
    if (format_str != NULL) mpig_bc_free_contact(format_str);
    if (endian_str != NULL) mpig_bc_free_contact(endian_str);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "exiting:  cm=" MPIG_PTR_FMT
	", cm_name=%s, vc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(cm), mpig_cm_get_name(cm),
	MPIG_PTR_CAST(vc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_net_select_comm_method);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	mpig_vc_set_cm(vc, NULL);
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_net_select_comm_method() */


/*
 * <mpi_errno> mpig_cm_xio_get_vc_net_compatability([IN] vc1, [IN] vc2, [IN] levels_in, [OUT] levels_out)
 *
 * see documentation in mpidpre.h.
 *
 * NOTE: if performance proves to be a problem, then separate routines can be created for each method, extracting just what is
 * important to that method.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_net_get_vc_compatability
static int mpig_cm_xio_net_get_vc_compatability(mpig_cm_t * const cm, const mpig_vc_t * const vc1, const mpig_vc_t * const vc2,
    const unsigned levels_in, unsigned * const levels_out)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_net_get_vc_compatability);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_net_get_vc_compatability);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: vc1=" MPIG_PTR_FMT ", vc2=" MPIG_PTR_FMT ", levels_in=0x%08x",
	MPIG_PTR_CAST(vc1), MPIG_PTR_CAST(vc2), levels_in));

    if(cm->cmu.xio.available == FALSE)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_CM | MPIG_DEBUG_LEVEL_CEMT, "communication method disabled; skipping the compatilbity "
	    "check: cm=" MPIG_PTR_FMT ", cm_name=%s", MPIG_PTR_CAST(cm), mpig_cm_get_name(cm)));
	*levels_out = 0;
	goto fn_return;
    }
    
    *levels_out = levels_in & MPIG_TOPOLOGY_LEVEL_WAN_MASK & cm->cmu.xio.topology_levels;
    
    if (levels_in & MPIG_TOPOLOGY_LEVEL_LAN_MASK & cm->cmu.xio.topology_levels)
    {
	if (mpig_vc_get_lan_id(vc1) != NULL && mpig_vc_get_lan_id(vc2) != NULL &&
	    strcmp(mpig_vc_get_lan_id(vc1), mpig_vc_get_lan_id(vc2)) == 0)
	{
	    *levels_out |= MPIG_TOPOLOGY_LEVEL_LAN_MASK;
	}
	else if (mpig_vc_get_lan_id(vc1) == NULL && mpig_vc_get_lan_id(vc2) == NULL &&
	    mpig_vc_get_pg(vc1) == mpig_vc_get_pg(vc2) && mpig_vc_get_app_num(vc1) == mpig_vc_get_app_num(vc2))
	{
	    *levels_out |= MPIG_TOPOLOGY_LEVEL_LAN_MASK;
	}
    }

    if (levels_in & MPIG_TOPOLOGY_LEVEL_SAN_MASK & cm->cmu.xio.topology_levels)
    {
	/* FIXME: for now, the XIO communication module assumes that it can always be used to communicate within the SAN.  this
	   might not be the case on all systems. */
	if (mpig_vc_get_pg(vc1) == mpig_vc_get_pg(vc2) && mpig_vc_get_app_num(vc1) == mpig_vc_get_app_num(vc2))
	{
	    *levels_out |= MPIG_TOPOLOGY_LEVEL_SAN_MASK;
	}
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: vc1=" MPIG_PTR_FMT ", vc2=" MPIG_PTR_FMT ", levels_out=0x%08x, "
	"mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vc1), MPIG_PTR_CAST(vc2), *levels_out, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_net_get_vc_compatability);
    return mpi_errno;
}
/* mpig_cm_xio_net_get_vc_compatability() */

#endif /* MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS */
