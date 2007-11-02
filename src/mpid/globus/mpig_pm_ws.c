/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

/*
 * This file contains an implementation of process management routines that interface with Web Services implementation of Globus.
 * In particular, this module makes use of the GRAM and Redezvous services.
 */

#include "mpidimpl.h"

#if defined(HAVE_GLOBUS_RENDEZVOUS_MODULE)

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include "globus_rendezvous.h"

#include "globus_error_gssapi.h"
#include "gssapi.h"
#include "globus_common.h"
#include "globus_xio.h"
/* #include "globus_delegation_client_util.h" */
#include "globus_ws_addressing.h"
#include "globus_service_engine.h"
#include "globus_notification_consumer.h"
#include "globus_xml_buffer.h"
/* #include "globus_wsrf_core_tools.h" */
#include "globus_wsrf_core_tools.h"
#include "wsa_EndpointReferenceType.h"
#include "ManagedMultiJobService_client.h"

/* XXX: this is a hack to work around the Globus xsd_long.h header file also defining int64_t */
#include "mpichconf.h"
#if !defined(HAVE_INT64_T)
#define HAVE_INT64_T 1
#endif
#include "mpidimpl.h"

/*
 * prototypes and data structures exposing the "web services" process management class
 */
static int mpig_pm_ws_init(int * argc, char *** argv, mpig_pm_t * pm, bool_t * my_pm_p);

static int mpig_pm_ws_finalize(mpig_pm_t * pm);

static int mpig_pm_ws_abort(mpig_pm_t * pm, int exit_code);

static int mpig_pm_ws_exchange_business_cards(mpig_pm_t * pm, mpig_bc_t * bc, mpig_bc_t ** bcs_p);

static int mpig_pm_ws_free_business_cards(mpig_pm_t * pm, mpig_bc_t * bcs);

static int mpig_pm_ws_get_pg_size(mpig_pm_t * pm, int * pg_size);

static int mpig_pm_ws_get_pg_rank(mpig_pm_t * pm, int * pg_rank);

static int mpig_pm_ws_get_pg_id(mpig_pm_t * pm, const char ** pg_id_p);

static int mpig_pm_ws_get_app_num(mpig_pm_t * pm, const mpig_bc_t * bc, int * app_num);

MPIG_STATIC mpig_pm_vtable_t mpig_pm_ws_vtable =
{
    mpig_pm_ws_init,
    mpig_pm_ws_finalize,
    mpig_pm_ws_abort,
    mpig_pm_ws_exchange_business_cards,
    mpig_pm_ws_free_business_cards,
    mpig_pm_ws_get_pg_size,
    mpig_pm_ws_get_pg_rank,
    mpig_pm_ws_get_pg_id,
    mpig_pm_ws_get_app_num,
    mpig_pm_vtable_last_entry
};

mpig_pm_t mpig_pm_ws =
{
    "web services",
    &mpig_pm_ws_vtable
};


#define GLOBUS_NAMESPACE "http://www.globus.org/namespaces/2004/10/gram/job"

#define GLOBUS_GRAM_MULTIJOB_HANDLE "GLOBUS_GRAM_MULTIJOB_HANDLE"
#define GLOBUS_GRAM_JOB_HANDLE      "GLOBUS_GRAM_JOB_HANDLE"
#define GLOBUS_GRAM_MULTIJOB_HANDLE "GLOBUS_GRAM_MULTIJOB_HANDLE"
#define GLOBUS_GRAM_SUBJOB_RANK     "GLOBUS_GRAM_SUBJOB_RANK"

#define BUFFSIZE 10

#define TEST_RES(res) \
{ \
    if ((res) != GLOBUS_SUCCESS) \
    { \
        mpig_pm_ws_GlobusErrorStr = globus_error_print_friendly(globus_error_peek((res))); \
        sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: %s\n", \
	    __FILE__, __LINE__, mpig_pm_ws_GlobusErrorStr); \
        globus_free(mpig_pm_ws_GlobusErrorStr); \
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg); \
    } \
}

#define TRY_GETENV(ptr, envvar) \
{ \
    if (!((ptr) = globus_libc_getenv(envvar))) \
    { \
	sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: undefined env var %s", __FILE__, __LINE__, envvar); \
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg); \
    } \
}

#define TRY_MALLOC(ptr, cast, nbytes) \
{ \
    if (!((ptr) = (cast) globus_malloc((size_t)(nbytes))))	\
    { \
        sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: failed malloc %d bytes\n", \
	    __FILE__, __LINE__, (int) (nbytes)); \
	MPID_Abort(NULL, MPI_ERR_INTERN, 1, mpig_pm_ws_ErrorMsg); \
    } \
}

#define TRY_REALLOC(nptr, optr, cast, nbytes) \
{ \
    if (!((nptr) = (cast) globus_realloc((optr), (size_t)(nbytes))))	\
    { \
        sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: failed realloc %d bytes\n", \
	    __FILE__, __LINE__, (int) (nbytes)); \
	MPID_Abort(NULL, MPI_ERR_INTERN, 1, mpig_pm_ws_ErrorMsg); \
    } \
}

struct rz_data
{
    char *data;
    size_t length;
    int mpi_errno;
}; /* end struct rz_data */

/* GLOBAL VARIABLES */
/* NICK: debugging */
/* MPIG_STATIC FILE *Log_fp;                              */
/* MPIG_STATIC int Pid;                                   */
/* static void print_byte_array(char *v, int vlen);       */
/* NICK: debugging */

MPIG_STATIC mpig_mutex_t mpig_pm_ws_Mutex;
MPIG_STATIC mpig_cond_t  mpig_pm_ws_Cond;
MPIG_STATIC globus_rz_handle_t mpig_pm_ws_SubjobHandle    = NULL;
MPIG_STATIC globus_rz_handle_t mpig_pm_ws_WholejobHandle  = NULL;
MPIG_STATIC globus_rz_handle_attr_t mpig_pm_ws_HandleAttr = NULL;
MPIG_STATIC int mpig_pm_ws_Initialized    =  0;
MPIG_STATIC int mpig_pm_ws_SizeAndRankSet =  0;
MPIG_STATIC int mpig_pm_ws_MySubjobRank   = -1;
MPIG_STATIC int mpig_pm_ws_PG_Size        = -1;
MPIG_STATIC int mpig_pm_ws_PG_Rank        = -1;
MPIG_STATIC char *mpig_pm_ws_PG_Id = NULL;
MPIG_STATIC int mpig_pm_ws_Done;
MPIG_STATIC int mpig_pm_ws_SubjobIdx;
#ifndef MPIG_VMPI
MPIG_STATIC globus_xio_stack_t  mpig_pm_ws_SubjobBootStack;
MPIG_STATIC globus_xio_driver_t mpig_pm_ws_SubjobBootDriver; /* must be persistent till end */
MPIG_STATIC globus_xio_server_t mpig_pm_ws_SubjobBootServer;
MPIG_STATIC char *mpig_pm_ws_SubjobBootCS;
#endif
MPIG_STATIC char *mpig_pm_ws_GlobusErrorStr; /* for TEST_RES() */
MPIG_STATIC char mpig_pm_ws_ErrorMsg[500]; 

/* LOCAL CALLBACK FUNCTIONS */
static void mpig_pm_ws_rz_data_callback(globus_result_t res, const char *data, size_t length, void *args);
/* LOCAL UTILITY FUNCTIONS */
static globus_result_t set_attr_security(
    globus_soap_message_attr_t          message_attr,
    globus_soap_message_authz_method_t  value,
    globus_bool_t                       encrypt,
    const char *                        id,
    globus_soap_message_auth_method_t   msg_sec);

static void mpig_pm_ws_gather_subjob_data(struct rz_data *subjob_data, char *my_checkinbuff, int hdr_len, int my_checkinbufflen);
static void mpig_pm_ws_process_wholejob_byte_array(char *v,
					    int vlen,
					    int my_subjobidx,
					    int rank_in_my_subjob,
					    int *mysubjobsize,
					    char ***byte_arrays,
					    int **byte_array_lens,
					    char ***boot_cs_array,
					    int *nprocs,
					    int *my_grank);
static void mpig_pm_ws_process_subjob_byte_array(char *v,
					    int vlen,
					    char **cp,
					    int SubjobIdx,
					    int *nprocs,
					    char ***subjob_byte_arrays,
					    int **subjob_byte_array_lens,
					    char ***subjob_boot_cs_array);
static void mpig_pm_ws_extract_int_and_single_space(char *v, int vlen, char **cp, int *val);
#ifndef MPIG_VMPI
static int mpig_pm_ws_distribute_byte_array_to_my_children(char *v,
						    int vlen,
						    int my_subjob_rank,
						    int my_subjob_size,
						    int my_grank,
						    char **contact_strings);
#endif


/*********************
 * mpig_pm_ws_init()
 ********************/ 
static int mpig_pm_ws_init(int * argc, char *** argv, mpig_pm_t * const pm, bool_t * my_pm_p)
{       
    globus_result_t res;
    const char *multijob_epr;
    const char *subjob_epr;
    const char *subjobidx_c;

    MPIG_UNUSED_ARG(argc);
    MPIG_UNUSED_ARG(argv);
    MPIG_UNUSED_ARG(pm);
    
    if (mpig_pm_ws_Initialized)
	return MPI_SUCCESS;

#if 0
    /* START NICK: debugging */
    {
	char fname[100];
	Pid = getpid();
	sprintf(fname, "mpig_pm_ws.%ud.log", Pid);
	if ((Log_fp = fopen(fname, "w")) == NULL)
	{
	    sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: unable to fopen log file%s", __FILE__, __LINE__, fname); \
	    MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg); \
	} /* endif */
    }
    /* END NICK: debugging */
#endif

    /****************/
    /* get env vars */
    /****************/

    /* first verify that that the MultiJob service was used to start the job.  if it is not, let the calling routine know that
       this PM module is not compatable with the mechanism used to start the job. */
    multijob_epr = globus_libc_getenv("GLOBUS_GRAM_MULTIJOB_HANDLE");
    if (multijob_epr == NULL)
    {
	*my_pm_p = FALSE;
	goto fn_return;
    }

    TRY_GETENV(subjob_epr,  GLOBUS_GRAM_JOB_HANDLE);
    TRY_GETENV(subjobidx_c, GLOBUS_GRAM_SUBJOB_RANK);

    if ((mpig_pm_ws_SubjobIdx = atoi(subjobidx_c)) < 0)
    {
        sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: %s has integer value %d (must be >= 0)", 
	    __FILE__, __LINE__, GLOBUS_GRAM_SUBJOB_RANK, mpig_pm_ws_SubjobIdx);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    } /* endif */

    /********************************************************************/
    /* activate rendezvous module and initialize global data structures */
    /********************************************************************/

    mpig_pm_ws_Initialized = 1;

    res = globus_module_activate(GLOBUS_RENDEZVOUS_MODULE); TEST_RES(res);
    res = mpig_mutex_construct(&mpig_pm_ws_Mutex);          TEST_RES(res);
    res = mpig_cond_construct(&mpig_pm_ws_Cond);            TEST_RES(res);

    /***********************/
    /* build subjob handle */
    /***********************/

    res = globus_rz_handle_attr_init(&mpig_pm_ws_HandleAttr,
                                        1,
                                        GLOBUS_SOAP_MESSAGE_AUTHZ_METHOD_KEY,
                                        NULL,
                                        NULL,
                                        GLOBUS_SOAP_MESSAGE_AUTHZ_HOST);              TEST_RES(res);
    res = globus_rz_handle_init(&mpig_pm_ws_SubjobHandle, mpig_pm_ws_HandleAttr, subjob_epr); TEST_RES(res);

#ifndef MPIG_VMPI
    res = globus_module_activate(GLOBUS_XIO_MODULE);                                                   TEST_RES(res);
    res = globus_xio_stack_init(&mpig_pm_ws_SubjobBootStack, NULL);                                    TEST_RES(res);
    res = globus_xio_driver_load("tcp", &mpig_pm_ws_SubjobBootDriver);                                 TEST_RES(res);
    res = globus_xio_stack_push_driver(mpig_pm_ws_SubjobBootStack, mpig_pm_ws_SubjobBootDriver);       TEST_RES(res);
    res = globus_xio_server_create(&mpig_pm_ws_SubjobBootServer, NULL, mpig_pm_ws_SubjobBootStack);    TEST_RES(res);
    res = globus_xio_server_get_contact_string(mpig_pm_ws_SubjobBootServer, &mpig_pm_ws_SubjobBootCS); TEST_RES(res);
#endif

    *my_pm_p = TRUE;

  fn_return:
    return MPI_SUCCESS;

} /* end mpig_pm_ws_init() */

/*********************
 * mpig_pm_ws_finalize()
 ********************/ 
static int mpig_pm_ws_finalize(mpig_pm_t * const pm)
{
    globus_result_t res;

    MPIG_UNUSED_ARG(pm);
    
    if (!mpig_pm_ws_Initialized)
    {
        sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_finalize() called without calling mpig_pm_ws_init()", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    } /* endif */

    res = mpig_cond_destruct(&mpig_pm_ws_Cond);   TEST_RES(res);
    res = mpig_mutex_destruct(&mpig_pm_ws_Mutex); TEST_RES(res);
    globus_rz_handle_destroy(mpig_pm_ws_SubjobHandle);       /* returns void */

    if (mpig_pm_ws_WholejobHandle) /* only subjob masters get this populated */
    {
        globus_rz_handle_destroy(mpig_pm_ws_WholejobHandle); /* returns void */
    } /* endif */
    globus_rz_handle_attr_destroy(mpig_pm_ws_HandleAttr);    /* returns void */

    res = globus_module_deactivate(GLOBUS_RENDEZVOUS_MODULE); TEST_RES(res);
    
#ifndef MPIG_VMPI
    globus_free(mpig_pm_ws_SubjobBootCS);

    res = globus_xio_stack_destroy(mpig_pm_ws_SubjobBootStack); TEST_RES(res);
    res = globus_xio_server_close(mpig_pm_ws_SubjobBootServer); TEST_RES(res);
    res = globus_xio_driver_unload(mpig_pm_ws_SubjobBootDriver); TEST_RES(res);
#endif

    /* fclose(Log_fp);  */

    return MPI_SUCCESS;

} /* end mpig_pm_ws_finalize() */

/*
 * mpig_pm_ws_abort()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_ws_abort
static int mpig_pm_ws_abort(mpig_pm_t * const pm, int exit_code)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const char *wholejob_epr_str;
    wsa_EndpointReferenceType *wholejob_epr;
    xsd_QName wsgram_resourceid_qname;
    int msg_idx;
    int value_idx;
    globus_bool_t encrypt_flag;
    int npossible_msg_secs = 0;
    globus_soap_message_auth_method_t *possible_msg_secs = NULL;
    static globus_soap_message_authz_method_t possible_values[] = {
	GLOBUS_SOAP_MESSAGE_AUTHZ_HOST, 
	GLOBUS_SOAP_MESSAGE_AUTHZ_SELF
    };
    globus_result_t result;
    int mpi_errno = MPI_SUCCESS;

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_ARG(pm);
    
    if (globus_module_activate(MANAGEDMULTIJOBSERVICE_MODULE))
    {
	globus_fatal("managed multi job service module activate failed");
    }
    else if (globus_module_activate(GLOBUS_WSRF_CORE_TOOLS_MODULE))
    {
	globus_fatal("globus wsrf core tools module activate failed");
    } /* endif */

    /********************************/
    /* getting whole job epr handle */
    /********************************/

    if (!(wholejob_epr_str = globus_libc_getenv(GLOBUS_GRAM_MULTIJOB_HANDLE)))
    {
	globus_fatal("NO GLOBUS_GRAM_MULTIJOB_HANDLE env!");
    } /* endif */
    /* fprintf(Log_fp, "ABORT: wholejob epr string=%s\n", wholejob_epr_str);  */
    /* fflush(Log_fp); */

    /* env var GLOBUS_GRAM_MULTIJOB_HANDLE has something like an EPR,    */
    /* but it's not exactly an EPR.                                      */
    /* extracting whole job EPR from GLOBUS_GRAM_MULTIJOB_HANDLE env var */
    result = wsa_EndpointReferenceType_init(&wholejob_epr);
    if (result != GLOBUS_SUCCESS)
    {
	globus_panic(NULL, result, "Failed wsa_EndpointReferenceType_init");
    } /* endif */
    
    wsgram_resourceid_qname.Namespace  = GLOBUS_NAMESPACE;
    wsgram_resourceid_qname.local      = "ResourceID";
    result = globus_wsrf_core_unwrap_endpoint_reference(wholejob_epr_str,
						&wsgram_resourceid_qname,
						wholejob_epr);
    if (result != GLOBUS_SUCCESS)
    {
	globus_panic(NULL, result, 
	    "Failed globus_wsrf_core_unwrap_endpoint_reference");
    } /* endif */

    /* 
       THIS WHOLE PROCESS IS A KLUDGE ... THE CORRECT SOLUTION IS
       TO HAVE THE GLOBUS FOLKS EMBED IN THE WHOLEJOB CONTAINER EPR 
       (AND PROVIDE ACCESSOR FUNCTIONS) THE SECURITY INFORMATION WE'RE 
       FORCE TO GUESS AT WITH THE METHOD DESCRIBED HERE.

       Guessing at what kind of security will be needed for this
       process (the client) to contact the wholejob container
       process (the server) based on whether or not "https"
       or "http" is in GLOBUS_GRAM_MULTIJOB_HANDLE (wholejob epr string)

       There are THREE security parameters that need to be set
       (as explained to me by John Bresnahan):

       (1) globus_soap_message_authz_method_t

           This is what *I*, this process (the client), expects
	   the server to use to identify itself, in other words, this
	   is the DN that I expect to receive from the wholejob container
	   process (the server).  Legal values are:

		- GLOBUS_SOAP_MESSAGE_AUTHZ_HOST 
		    Expect a host DN that "matches" the host found
		    somewhere in the URL that *I* (this process, client)
		    used to connect to the wholejob container process (server).

		- GLOBUS_SOAP_MESSAGE_AUTHZ_SELF
		    Expect a DN that matches the DN found in the proxy
		    that the this process (the client) is running under
		    This is used when connecting to a wholejob container
		    that was stood up by the same user running this 
		    process, (i.e., a "personal container").

		- GLOBUS_SOAP_MESSAGE_AUTHZ_IDENTITY
		    Here *I* (client) provides an ID string (a DN)
		    and it expects from the wholejob container process
		    a DN that matches it.

		- GLOBUS_SOAP_MESSAGE_AUTHZ_NONE
		    This process, the client, is willing to accept anything
		    or nothing at all ... no security, just connect.

	   We try only these TWO from the FOUR above:
	    GLOBUS_SOAP_MESSAGE_AUTHZ_HOST and 
	    GLOBUS_SOAP_MESSAGE_AUTHZ_SELF

       (2) globus_soap_message_auth_method_t (note "auth" here, "authz" in (1))

           This is the type of security the wholejob container process (server)
	   insists that *I* (client) use to connect back to him.
	   Legal values are:

		- GLOBUS_SOAP_MESSAGE_AUTH_SECURE
		    Transport security, meaning (as I understand)
		    using something like secure sockets.

		- GLOBUS_SOAP_MESSAGE_AUTH_SECURE_MESSAGE
		    Message security, meaning (as I understand)
		    adding a checksum (not encrypting) to make
		    sure the message has not been tampered with.
		    Messages are still sent in the clear.

		- GLOBUS_SOAP_MESSAGE_AUTH_SECURE_CONVERSATION
		    I don't know what "conversation" means.
		    They have *not* implemented this in C, and so, 
		    I can't try it even if I wanted to.

		- GLOBUS_SOAP_MESSAGE_AUTH_NONE
		    Sever requires no security.

	   Here we look at the env var GLOBUS_GRAM_MULTIJOB_HANDLE, 
	   which is a string that "represents" the wholejob container
	   EPR (it's *not* the actual EPR) and decide which of the above
	   FOUR to try based on if the env var contains if "https" or "http":

	   if (https)
		try GLOBUS_SOAP_MESSAGE_AUTH_SECURE
		and GLOBUS_SOAP_MESSAGE_AUTH_SECURE_MESSAGE
	    else if (http)
		try GLOBUS_SOAP_MESSAGE_AUTH_NONE
		and GLOBUS_SOAP_MESSAGE_AUTH_SECURE_MESSAGE
	    else
		give up ... don't try anything
	    endif

       (3) globus_bool_t encrypt
	    The wholejob container process (server) dictates whether
	    or not the messages must be encrypted.
	    We try both.
    */
    if (strstr(wholejob_epr_str, "https"))
    {
	/* wholejob epr has https */
	npossible_msg_secs = 2;

	if (!(possible_msg_secs = (globus_soap_message_auth_method_t *)
	malloc(npossible_msg_secs*sizeof(globus_soap_message_auth_method_t))))
	{
	    globus_fatal("failed malloc (https), npossible_msg_secs");
	} /* endif */

	possible_msg_secs[0] = GLOBUS_SOAP_MESSAGE_AUTH_SECURE;
	possible_msg_secs[1] = GLOBUS_SOAP_MESSAGE_AUTH_SECURE_MESSAGE;
    }
    else if (strstr(wholejob_epr_str, "http"))
    {
	/* wholejob epr has http */
	npossible_msg_secs = 2;

	if (!(possible_msg_secs = (globus_soap_message_auth_method_t *)
	malloc(npossible_msg_secs*sizeof(globus_soap_message_auth_method_t))))
	{
	    globus_fatal("failed malloc (http), npossible_msg_secs");
	} /* endif */

	possible_msg_secs[0] = GLOBUS_SOAP_MESSAGE_AUTH_NONE;
	possible_msg_secs[1] = GLOBUS_SOAP_MESSAGE_AUTH_SECURE_MESSAGE;
    } 
    else
    {
	/* cannot figure out how to even guess at security ... 
	   just give up trying to kill all procs now */
		/* fprintf(Log_fp,  */
		    /* "ABORT: ERROR: could not find 'https' or 'http' in >%s<\n " */
		    /* "... returning immediately without trying to kill",  */
		    /* wholejob_epr_str);  */
		/* fflush(Log_fp); */
	goto fn_return;
    } /* endif */

    /**************************************************************/
    /* loops to run through all combinations of security settings */
    /* that make sense based on what we were able to tease out    */
    /* of the EPR                                                 */
    /**************************************************************/

    for (msg_idx = 0; msg_idx < npossible_msg_secs; msg_idx ++)
    {
	for (value_idx = 0; 
	     value_idx < (int)
		 (sizeof(possible_values)/sizeof(globus_soap_message_authz_method_t));
	     value_idx ++)
	{
	    for (encrypt_flag = 0; encrypt_flag < 2; encrypt_flag ++)
	    {
		globus_soap_message_attr_t             	attr;
		ManagedMultiJobService_client_handle_t 	destroy_handle;
		wsrl_DestroyType                       	destroy;
		wsrl_DestroyResponseType               	*destroy_response;
		ManagedMultiJobPortType_Destroy_fault_t fault_type;
		xsd_any 				*fault;

		/* fprintf(Log_fp,  */
	    /* "*** ABORT: top of loop: msg_idx %d value_idx %d encrypt_flag %d\n", */
		    /* msg_idx, value_idx, encrypt_flag);  */
		/* fflush(Log_fp); */

		if ((result = globus_soap_message_attr_init(&attr)) 
		    != GLOBUS_SUCCESS)
		{
		    globus_panic(NULL, result, 
			"Failed globus_soap_message_attr_init");
		} /* endif */

		if ((result = set_attr_security(attr,
						possible_values[value_idx],
						encrypt_flag,
						NULL,
						possible_msg_secs[msg_idx])) 
		    != GLOBUS_SUCCESS)
		{
		    globus_panic(NULL, result, "Failed set_attr_security");
		} /* endif */

		result = ManagedMultiJobService_client_init(&destroy_handle,
							    attr,
							    NULL);
		if (result != GLOBUS_SUCCESS)
		{
		    globus_panic(NULL, result, 
			"Failed ManagedMultiJobService_client_init");
		} /* endif */

		destroy_response = NULL;
		/* 
		   OK ... we pullthe trigger here ... if we return from 
		   this then we know that this attempt to bring down
		   the ENTIRE job did not work
		 */
		/* fprintf(Log_fp,  */
		    /* "    ABORT: msg_idx %d value_idx %d encrypt_flag %d: " */
		    /* "BEFORE pulling the trigger\n", */
		    /* msg_idx, value_idx, encrypt_flag);  */
		/* fflush(Log_fp); */
		result = ManagedMultiJobPortType_Destroy_epr(destroy_handle,
							    wholejob_epr,
							    &destroy,
							    &destroy_response,
							    &fault_type,
							    &fault);
		/* fprintf(Log_fp,  */
		    /* "    ABORT: msg_idx %d value_idx %d encrypt_flag %d: " */
		    /* "AFTER pulling the trigger ... empty chamber :-(\n", */
		    /* msg_idx, value_idx, encrypt_flag);  */
		/* fflush(Log_fp); */
		/* if I get to this point the last attempt to kill
		   the entire job failed.  I might be able to  do something
		   clever with result, fault, and fault_type to help
		   guide my next guess, but at this point I don;t
		   know how to use any of that information to help 
		   guide my next move and so I ignore them for now.
		*/
#if 0
if(result != GLOBUS_SUCCESS)
{
    if(fault_type != GLOBUS_SUCCESS)
    {
	/* check fault */
	...

	xsd_any_destroy(fault);
    } /* endif */

    /* deal with error */
} /* endif */
#endif
		/* dunno if this all of this "destroy" at the end of
		this loop is necessary if every time, or worst, 
		if it breaks things, but what the heck */
		globus_soap_message_attr_destroy(attr);
		/* fprintf(Log_fp,  */
		    /* "    ABORT: msg_idx %d value_idx %d encrypt_flag %d: " */
		    /* "after globus_soap_message_attr_destroy\n", */
		    /* msg_idx, value_idx, encrypt_flag);  */
		/* fflush(Log_fp); */

		wsrl_DestroyType_destroy_contents(&destroy);
		/* fprintf(Log_fp,  */
		    /* "    ABORT: msg_idx %d value_idx %d encrypt_flag %d: " */
		    /* "after wsrl_DestroyType_destroy_contents\n", */
		    /* msg_idx, value_idx, encrypt_flag);  */
		/* fflush(Log_fp); */

		wsrl_DestroyResponseType_destroy(destroy_response);
		/* fprintf(Log_fp,  */
		    /* "    ABORT: msg_idx %d value_idx %d encrypt_flag %d: " */
		    /* "after wsrl_DestroyResponseType_destroy\n", */
		    /* msg_idx, value_idx, encrypt_flag);  */
		/* fflush(Log_fp); */

		ManagedMultiJobService_client_destroy(destroy_handle);
		/* fprintf(Log_fp,  */
		    /* "    ABORT: msg_idx %d value_idx %d encrypt_flag %d: " */
		    /* "after ManagedMultiJobService_client_destroy ... " */
		    /* "that was the last one\n", */
		    /* msg_idx, value_idx, encrypt_flag);  */
		/* fflush(Log_fp); */
	    } /* endfor */
	} /* endfor */
    } /* endfor */

  fn_return:
    return mpi_errno;
}
/* end mpig_pm_ws_abort() */


/*********************
 * mpig_pm_ws_exchange_business_cards()
 * assumptions made in writing MPIDI_PM_distribute_byte_array(): 
 * - vMPI MPI_Init has been called before entering this function 
 * - globus_module_activate(GLOBUS_XIO_MODULE) has been called 
 *   before entering this function 
 * - globus_xio_server_{register_}accept() was NOT called on the
 *   'server' arg before calling this function
 * - the byte arrays we are distributing here MUST be the biz cards
 * - all procs use the same stack as passed by the arg 'stack'
 ********************/ 
static int mpig_pm_ws_exchange_business_cards(mpig_pm_t * const pm, mpig_bc_t * const bc, mpig_bc_t ** const bcs_ptr)
{       
    int rc;
    char *inbuf = NULL;
    int inbuf_len;
    char **outbufs = NULL;
    int *outbufs_lens = NULL;
    char **boot_cs_array = NULL;
    char *my_checkinbuff = NULL;
    char *cp;
    int hdr_len;
    int my_checkinbuffmalloc;
    int my_checkinbufflen;
    globus_result_t res;
    int mysubjobsize;
    int i;
    struct rz_data subjob_data;
#ifdef MPIG_VMPI
    int subjobcslen = 0;
#else 
    int subjobcslen = strlen(mpig_pm_ws_SubjobBootCS)+1;
#endif
    int mpi_errno = MPI_SUCCESS;
    /* fprintf(Log_fp, "%ud: subjobcslen %d: ", Pid, subjobcslen); fflush(Log_fp); print_byte_array(mpig_pm_ws_SubjobBootCS, subjobcslen); */

    MPIG_UNUSED_ARG(pm);
    
    if (!mpig_pm_ws_Initialized)
    {
        sprintf(mpig_pm_ws_ErrorMsg, 
	    "ERROR: file %s: line %d: mpig_pm_ws_exchange_business_cards() called without calling mpig_pm_ws_init()", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    }
    else if (!bc)
    {
        sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_exchange_business_cards() passed NULL bc", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    }
    else if (!bcs_ptr)
    {
        sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_exchange_business_cards() passed NULL bcs_ptr", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    } /* endif */

    /* create a process group id and add it to the business card.  only the id added to the job master will be used below. */
    {
	mpig_uuid_t uuid;
        char uuid_str[MPIG_UUID_MAX_STR_LEN + 1];
        
        mpi_errno = mpig_uuid_generate(&uuid);
	if (mpi_errno)
	{
	    sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_exchange_business_cards() could not create UUID", 
		__FILE__, __LINE__);
	    MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	}

        mpig_uuid_unpares(&uuid, uuid_str);
	mpi_errno = mpig_bc_add_contact(bc, "PM_WS_PG_ID", uuid_str);
	if (mpi_errno)
	{
	    sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_exchange_business_cards() was unable to add "
		"the process group ID to the business card", __FILE__, __LINE__);
	    MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	}
    }

    /* add the subjob index to the business card.  the subjob index is be needed by the topology module to perform topology
       discovery and to fill in the MPI_APP_NUM attribute attached to MPI_COMM_WORLD (see MPID_Init). */
    {
	char sj_index_str[10];

	if (MPIU_Snprintf(sj_index_str, 10, "%d", mpig_pm_ws_SubjobIdx) < 10)
        {
	    mpi_errno = mpig_bc_add_contact(bc, "PM_WS_APP_NUM", sj_index_str);

	    if (mpi_errno)
	    {
		sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_exchange_business_cards() was unable to add "
		    "the subjob index to the business card", __FILE__, __LINE__);
		MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	    }
	}
	else /* if (MPIU_Snprintf(sj_index_str, 10, "%d", mpig_pm_ws_SubjobIdx) >= 10) */
        {
		sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_exchange_business_cards() failed to convert "
		    "the subjob index to a string", __FILE__, __LINE__);
		MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	}
	/* end if/else (MPIU_Snprintf(sj_index_str, 10, "%d", mpig_pm_ws_SubjobIdx) < 10) */
    }
    /* end of block that adds the subjob index to the business card */

    mpig_bc_serialize_object(bc, &inbuf);
    if (!inbuf)
    {
        sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_bc_serialize_object() returned NULL inbuf", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    } /* endif */

    inbuf_len = strlen(inbuf)+1;
    /* fprintf(Log_fp, "%ud: after mpig_bc_serialize_object: inbuf_len %d: ", Pid, inbuf_len); fflush(Log_fp); print_byte_array(inbuf, inbuf_len); */

    /* building checkin buff from linearized biz card  */
    /* need to pre-prend boostrap server CS            */

    my_checkinbuffmalloc = (3*BUFFSIZE) /* sprintf of 3 lengths */
			+ subjobcslen   /* subjob boot cs */
			+ inbuf_len;    /* inbuf payload */
    TRY_MALLOC(my_checkinbuff, char *, my_checkinbuffmalloc);

    /* 
     * building my byte array with following format:
     *
     *  +-----------------------------------------+
     *  |Nbytes SJ_CS_Len SJ_CS BizCardLen BizCard|
     *  +-----------------------------------------+
     *          |------- Nbytes ------------------|
     *
     * where (SJ_CS_Len=0 and SJ_CS=NULL) iff vMPI build
     *
     * NOTE: need to place (SJ_CS_Len=0 and SJ_CS=NULL) for
     *       mpig_pm_ws_process_subjob_byte_array().  procs in other
     *       subjobs have no way to know if I have vMPI
     *       so I cannot simply omit <SJ_CS_Len, SJ_CS>.
     */
    {
	char b[BUFFSIZE];

	sprintf(b, "%d ", subjobcslen);
	my_checkinbufflen = strlen(b);

	my_checkinbufflen += subjobcslen;

	sprintf(b, "%d ", inbuf_len);
	my_checkinbufflen += strlen(b);

	my_checkinbufflen += inbuf_len;
    }

    cp = my_checkinbuff;
    sprintf(cp, "%d ", my_checkinbufflen);
    hdr_len = strlen(my_checkinbuff);
    cp += strlen(cp);

    /* subjob bootstrap cs */
    sprintf(cp, "%d ", subjobcslen);
    cp += strlen(cp);
#ifndef MPIG_VMPI
    memcpy(cp, mpig_pm_ws_SubjobBootCS, (size_t) subjobcslen);
    cp += subjobcslen;
#endif

    /* business card */
    sprintf(cp, "%d ", inbuf_len);
    cp += strlen(cp);
    memcpy(cp, inbuf, (size_t) inbuf_len);
    cp += inbuf_len;
    mpig_bc_free_serialized_object(inbuf);

    /* subjob registration */
#ifdef MPIG_VMPI
    if (mpig_pm_ws_MySubjobRank == -1)
    {
	mpig_vmpi_comm_rank(MPIG_VMPI_COMM_WORLD, &mpig_pm_ws_MySubjobRank);
    } /* endif */
#endif

    subjob_data.data      = NULL;
    subjob_data.length    = 0;
    subjob_data.mpi_errno = MPI_SUCCESS;

    /* assigns mpig_pm_ws_MySubjobRank (if not already known) */
    /* and gathers SJ data in mpig_pm_ws_MySubjobRank = 0     */
    /* fprintf(Log_fp, "%ud: before call mpig_pm_ws_gather_subjob_data: hdr_len %d my_checkinbufflen %d\n", Pid, hdr_len, my_checkinbufflen); fflush(Log_fp);  */
    mpig_pm_ws_gather_subjob_data(&subjob_data, 
			my_checkinbuff, 
			hdr_len, 
			my_checkinbufflen);

    if (mpig_pm_ws_MySubjobRank == 0)
    {
	/*****************************************/
	/* subjob master  ... the only proc at   */
	/* this point that has the subjob's data */
	/*****************************************/

	const char *wholejob_epr = NULL;
	globus_reltime_t poll_freq;

	if (mpig_pm_ws_WholejobHandle == NULL)
	{
	    /* first time MPIDI_PM_distribute_byte_array() called */
	    TRY_GETENV(wholejob_epr, GLOBUS_GRAM_MULTIJOB_HANDLE);
	    res = globus_rz_handle_init(&mpig_pm_ws_WholejobHandle, mpig_pm_ws_HandleAttr, wholejob_epr); TEST_RES(res);
	} /* endif */

	/* subjob masters register their subjob's byte array to the wholejob rend serv */
	/* fprintf(Log_fp, "%ud: before globus_rz_multi_register() passing mpig_pm_ws_SubjobIdx %d\n", Pid, mpig_pm_ws_SubjobIdx); fflush(Log_fp); */
	res = globus_rz_multi_register(mpig_pm_ws_WholejobHandle, subjob_data.data, subjob_data.length, &mpig_pm_ws_SubjobIdx);
	TEST_RES(res);


	/* fprintf(Log_fp, "%ud: after globus_rz_multi_register() mpig_pm_ws_SubjobIdx %d\n", Pid, mpig_pm_ws_SubjobIdx); fflush(Log_fp); */
	globus_free(subjob_data.data);
	subjob_data.data      = NULL;
	subjob_data.length    = 0;

	mpig_pm_ws_Done = 0;
	GlobusTimeReltimeSet(poll_freq, 1, 0); /* NICK */
	res = globus_rz_multi_data_request_begin(mpig_pm_ws_WholejobHandle, &poll_freq, mpig_pm_ws_rz_data_callback, &subjob_data);
	TEST_RES(res);

	/* fprintf(Log_fp, "%ud: after globus_rz_multi_data_request_begin()\n", Pid); fflush(Log_fp); */

	/* Wait for result */
	/* fprintf(Log_fp, "%ud: subjob master BEFORE wholejob wait loop\n", Pid); fflush(Log_fp); */
	mpig_mutex_lock(&mpig_pm_ws_Mutex);
	while (!mpig_pm_ws_Done)
	{
	    mpig_cond_wait(&mpig_pm_ws_Cond, &mpig_pm_ws_Mutex);
	} /* endwhile */
	mpig_mutex_unlock(&mpig_pm_ws_Mutex);
	/* fprintf(Log_fp, "%ud: subjob master AFTER wholejob wait loop\n", Pid); fflush(Log_fp); */

	/* free up some resources in globus_rz_XXX lib */
	/* NICK: need to check if this is the right thing to 
	         do over and over again.
	*/
	res = globus_rz_data_request_finished(mpig_pm_ws_WholejobHandle); TEST_RES(res);
	/* fprintf(Log_fp, "%ud: after globus_rz_data_request_finished()\n", Pid); fflush(Log_fp); */

	mpig_pm_ws_process_wholejob_byte_array(subjob_data.data,
				    subjob_data.length,
				    mpig_pm_ws_SubjobIdx,
				    mpig_pm_ws_MySubjobRank,
				    &mysubjobsize,
				    &outbufs,
				    &outbufs_lens,
				    &boot_cs_array,
				    &mpig_pm_ws_PG_Size,
				    &mpig_pm_ws_PG_Rank);
	mpig_pm_ws_SizeAndRankSet = 1;

	/* fprintf(Log_fp, "%ud: after mpig_pm_ws_process_wholejob_byte_array(): mysubjobsize %d my_grank %d nprocs %d\n", Pid, mysubjobsize, mpig_pm_ws_PG_Rank, mpig_pm_ws_PG_Size); fflush(Log_fp); */

	/*****************************************************/
	/* distributing wholejob byte array to subjob slaves */
	/*****************************************************/

#ifdef MPIG_VMPI
	mpig_vmpi_bcast(&(subjob_data.length), 1, MPIG_VMPI_INT, 0, MPIG_VMPI_COMM_WORLD);
	mpig_vmpi_bcast(subjob_data.data, 
		    subjob_data.length, 
		    MPIG_VMPI_BYTE, 
		    0, 
		    MPIG_VMPI_COMM_WORLD);
#else
	rc = mpig_pm_ws_distribute_byte_array_to_my_children(subjob_data.data,
						subjob_data.length,
						mpig_pm_ws_MySubjobRank,
						mysubjobsize,
						mpig_pm_ws_PG_Rank,
						boot_cs_array);
	if (rc)
	{
	    for (i = 0; i < mpig_pm_ws_PG_Size; i ++)
		globus_free(boot_cs_array[i]);
	    globus_free(boot_cs_array);
	    globus_free(subjob_data.data);
	    sprintf(mpig_pm_ws_ErrorMsg, 
		"ERROR: file %s: line %d: recvd erroneous rc %d from mpig_pm_ws_distribute_byte_array_to_my_children", 
		__FILE__, __LINE__, rc);
	    MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	} /* endif */
#endif
	globus_free(subjob_data.data);
    }
    else
    {
	/****************/
	/* subjob slave */
	/****************/

	int ba_len;
	char *ba = NULL;
#ifndef MPIG_VMPI
	char buff[BUFFSIZE];
	globus_size_t nbytes;
	globus_xio_handle_t parent_handle;
#endif

#ifdef MPIG_VMPI
	mpig_vmpi_bcast(&ba_len, 1, MPIG_VMPI_INT, 0, MPIG_VMPI_COMM_WORLD);

	TRY_MALLOC(ba, char *, ba_len);

	mpig_vmpi_bcast(ba, ba_len, MPIG_VMPI_BYTE, 0, MPIG_VMPI_COMM_WORLD);
	mpig_pm_ws_process_wholejob_byte_array(ba,
				    ba_len,
				    mpig_pm_ws_SubjobIdx,
				    mpig_pm_ws_MySubjobRank,
				    &mysubjobsize,
				    &outbufs,
				    &outbufs_lens,
				    &boot_cs_array,
				    &mpig_pm_ws_PG_Size,
				    &mpig_pm_ws_PG_Rank);
	mpig_pm_ws_SizeAndRankSet = 1;
#else
	/* receiving wholejob byte arrary from parent */
	res = globus_xio_server_accept(&parent_handle, mpig_pm_ws_SubjobBootServer);  TEST_RES(res);
        res = globus_xio_open(parent_handle, NULL, NULL);                             TEST_RES(res);

	/* reading wholejob byte array from parent ... first ba len */
        res = globus_xio_read(parent_handle, buff, BUFFSIZE, BUFFSIZE, &nbytes, NULL); TEST_RES(res);
        if (nbytes != BUFFSIZE)
        {
	    sprintf(mpig_pm_ws_ErrorMsg, 
		"ERROR: file %s: line %d: MPIDI_PM_distribute_byte_array() ba_len read only %d bytes, requested %d bytes", 
		__FILE__, __LINE__, (int) nbytes, BUFFSIZE);
	    MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
        } 
        else if ((ba_len = atoi(buff)) <= 0)
        {
	    sprintf(mpig_pm_ws_ErrorMsg, 
		"ERROR: file %s: line %d: MPIDI_PM_distribute_byte_array() recvd ba_len %d from buff %s (must be > 0)", 
		__FILE__, __LINE__, ba_len, buff);
	    MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
        } /* endif */

	TRY_MALLOC(ba, char *, ba_len);

	/* now read the actual ba */
        res = globus_xio_read(parent_handle, ba, (globus_size_t )ba_len, (globus_size_t) ba_len, &nbytes, NULL); TEST_RES(res);
        if (nbytes != (globus_size_t) ba_len)
        {
	    sprintf(mpig_pm_ws_ErrorMsg, 
		"ERROR: file %s: line %d: MPIDI_PM_distribute_byte_array() ba_len read only %d bytes, requested %d bytes", 
		__FILE__, __LINE__, (int) nbytes, ba_len);
	    MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
        } /* endif */

        res = globus_xio_close(parent_handle, NULL); TEST_RES(res);

	mpig_pm_ws_process_wholejob_byte_array(ba,
				    ba_len,
				    mpig_pm_ws_SubjobIdx,
				    mpig_pm_ws_MySubjobRank,
				    &mysubjobsize,
				    &outbufs,
				    &outbufs_lens,
				    &boot_cs_array,
				    &mpig_pm_ws_PG_Size,
				    &mpig_pm_ws_PG_Rank);
	mpig_pm_ws_SizeAndRankSet = 1;

	/* fprintf(Log_fp, "%ud: after mpig_pm_ws_process_wholejob_byte_array(): mysubjobsize %d my_grank %d nprocs %d\n", Pid, mysubjobsize, mpig_pm_ws_PG_Rank, mpig_pm_ws_PG_Size); fflush(Log_fp); */
	/* distributing wholejob byte array to my children */
	if (mpig_pm_ws_distribute_byte_array_to_my_children(ba,
					    ba_len,
					    mpig_pm_ws_MySubjobRank,
					    mysubjobsize,
					    mpig_pm_ws_PG_Rank,
					    boot_cs_array))
	{
	    for (i = 0; i < mpig_pm_ws_PG_Size; i ++)
		globus_free(boot_cs_array[i]);
	    globus_free(boot_cs_array);
	    globus_free(ba);
	    sprintf(mpig_pm_ws_ErrorMsg, 
		"ERROR: file %s: line %d: recvd erroneous rc %d from mpig_pm_ws_distribute_byte_array_to_my_children", 
		__FILE__, __LINE__, rc);
	    MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	} /* endif */
	/* fprintf(Log_fp, "%ud: after mpig_pm_ws_distribute_byte_array_to_my_children(): mysubjobsize %d my_grank %d nprocs %d\n", Pid, mysubjobsize, mpig_pm_ws_PG_Rank, mpig_pm_ws_PG_Size); fflush(Log_fp); */
#endif
	globus_free(ba);
    } /* endif */

    /* fprintf(Log_fp, "%ud: my_grank %d nprocs %d: before converting %d byte arrays to biz cards\n", Pid, mpig_pm_ws_PG_Rank, mpig_pm_ws_PG_Size, mpig_pm_ws_PG_Size); fflush(Log_fp); */
    /* converting vector of byte arrays to a vector of biz cards */
    TRY_MALLOC(*bcs_ptr, mpig_bc_t *, mpig_pm_ws_PG_Size*sizeof(mpig_bc_t));
    /* fprintf(Log_fp, "%ud: my_grank %d nprocs %d: after successful malloc %d mpig_bc_t\n", Pid, mpig_pm_ws_PG_Rank, mpig_pm_ws_PG_Size, mpig_pm_ws_PG_Size); fflush(Log_fp); */
    for (i = 0; i < mpig_pm_ws_PG_Size; i ++)
    {
	/* fprintf(Log_fp, "%ud: my_grank %d nprocs %d: before mpig_bc_deserialize_object outbufs[%d]: ", Pid, mpig_pm_ws_PG_Rank, mpig_pm_ws_PG_Size, i); fflush(Log_fp); print_byte_array(outbufs[i], outbufs_lens[i]); */
	mpig_bc_construct(&((*bcs_ptr)[i]));
	mpig_bc_deserialize_object((char *) outbufs[i], &((*bcs_ptr)[i]));
	/* fprintf(Log_fp, "%ud: my_grank %d nprocs %d: after mpig_bc_deserialize_object outbufs[%d]\n", Pid, mpig_pm_ws_PG_Rank, mpig_pm_ws_PG_Size, i); fflush(Log_fp); */
    } /* endif */
    /* fprintf(Log_fp, "%ud: my_grank %d nprocs %d: after converting %d byte arrays to biz cards\n", Pid, mpig_pm_ws_PG_Rank, mpig_pm_ws_PG_Size, mpig_pm_ws_PG_Size); fflush(Log_fp); */

    /* get the process group id from the job master */
    {
	char * pg_id_str;
	bool_t found;

	mpi_errno = mpig_bc_get_contact(bcs_ptr[0], "PM_WS_PG_ID", &pg_id_str, &found);
	if (mpi_errno)
	{
	    sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_exchange_business_cards() could not extract the "
		"process group ID from the job master business card", __FILE__, __LINE__);
	    MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	}

	if (found)
        {
	    mpig_pm_ws_PG_Id = globus_libc_strdup(pg_id_str);
	    if (mpig_pm_ws_PG_Id == NULL)
	    {
		sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_exchange_business_cards() failed to allocate "
		    "memory for the process group ID", __FILE__, __LINE__);
		MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	    }

	    mpig_bc_free_contact(pg_id_str);
	}
	else
        {
	    sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_exchange_business_cards() the process group ID "
		"was not found in the job master's business card", __FILE__, __LINE__);
	    MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	}
    }

    /* cleanup */
    globus_free(my_checkinbuff);
    for (i = 0; i < mpig_pm_ws_PG_Size; i ++)
    {
	globus_free(boot_cs_array[i]);
	globus_free(outbufs[i]);
    } /* endif */
    globus_free(boot_cs_array);
    globus_free(outbufs);
    globus_free(outbufs_lens);

    /* fprintf(Log_fp, "%ud: exit mpig_pm_ws_exchange_business_cards: mysubjobsize %d my_grank %d nprocs %d\n", Pid, mysubjobsize, mpig_pm_ws_PG_Rank, mpig_pm_ws_PG_Size); fflush(Log_fp); */

    return MPI_SUCCESS;

} /* end mpig_pm_ws_exchange_business_cards() */

/*********************
 * mpig_pm_ws_free_business_cards()
 * NICK: need to write this later ... dunno what it's for. Brian added this.
 ********************/ 
static int mpig_pm_ws_free_business_cards(mpig_pm_t * const pm, mpig_bc_t * const bcs)
{       
    int mpi_errno = MPI_SUCCESS;
    int i;

    MPIG_UNUSED_ARG(pm);
    
    if (!mpig_pm_ws_Initialized)
    {
        sprintf(mpig_pm_ws_ErrorMsg, 
	    "ERROR: file %s: line %d: mpig_pm_ws_free_business_cards() called without calling mpig_pm_ws_init()", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    }
    else if (!mpig_pm_ws_SizeAndRankSet)
    {
        sprintf(mpig_pm_ws_ErrorMsg, 
	    "ERROR: file %s: line %d: mpig_pm_ws_free_business_cards() called without calling mpig_pm_ws_exchange_business_cards()",
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    }
    else if (!bcs)
    {
        sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_free_business_cards() passed NULL bcs", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    } /* endif */

    for (i = 0; i < mpig_pm_ws_PG_Size; i ++)
    {
	mpig_bc_destruct(&bcs[i]);
    } /* endfor */
    globus_free(bcs);

    return mpi_errno;

} /* end mpig_pm_ws_free_business_cards() */

/*********************
 * mpig_pm_ws_get_pg_size()
 * Assumed that mpig_pm_ws_exchange_business_cards() called before this func
 ********************/ 
static int mpig_pm_ws_get_pg_size(mpig_pm_t * const pm, int * const pg_size)
{       
    MPIG_UNUSED_ARG(pm);
    
    if (!mpig_pm_ws_SizeAndRankSet)
    {
        sprintf(mpig_pm_ws_ErrorMsg, 
	    "ERROR: file %s: line %d: mpig_pm_ws_get_pg_size() called without calling mpig_pm_ws_exchange_business_cards()", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    }
    else if (!pg_size)
    {
        sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_get_pg_size() passed NULL pg_size", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    } /* endif */

    *pg_size = mpig_pm_ws_PG_Size;

    return MPI_SUCCESS;

} /* end mpig_pm_ws_get_pg_size() */

/*********************
 * mpig_pm_ws_get_pg_rank()
 * Assumed that mpig_pm_ws_exchange_business_cards() called before this func
 ********************/ 
static int mpig_pm_ws_get_pg_rank(mpig_pm_t * const pm, int * const pg_rank)
{       
    MPIG_UNUSED_ARG(pm);

    if (!mpig_pm_ws_SizeAndRankSet)
    {
        sprintf(mpig_pm_ws_ErrorMsg, 
	    "ERROR: file %s: line %d: mpig_pm_ws_get_pg_rank() called without calling mpig_pm_ws_exchange_business_cards()", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    }
    else if (!pg_rank)
    {
        sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_get_pg_rank() passed NULL pg_rank", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    } /* endif */

    *pg_rank = mpig_pm_ws_PG_Rank;

    return MPI_SUCCESS;

} /* end mpig_pm_ws_get_pg_rank() */

/*********************
 * mpig_pm_ws_get_pg_id()
 * Assumed that mpig_pm_ws_exchange_business_cards() called before this func
 * NICK: need to write this later ... dunno what it's for. Brian added this.
 ********************/ 
static int mpig_pm_ws_get_pg_id(mpig_pm_t * const pm, const char ** const pg_id)
{       
    MPIG_UNUSED_ARG(pm);
    
    if (!mpig_pm_ws_SizeAndRankSet)
    {
        sprintf(mpig_pm_ws_ErrorMsg, 
	    "ERROR: file %s: line %d: mpig_pm_ws_get_pg_id() called without calling mpig_pm_ws_exchange_business_cards()", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    }
    else if (!pg_id)
    {
        sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_get_pg_id() passed NULL pg_id", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    } /* endif */

    *pg_id = mpig_pm_ws_PG_Id;

    return MPI_SUCCESS;

} /* end mpig_pm_ws_get_pg_id() */


/*
 * mpig_pm_ws_get_app_num()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_ws_get_app_num
static int mpig_pm_ws_get_app_num(mpig_pm_t * const pm, const mpig_bc_t * const bc, int * const app_num_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    char * app_num_str = NULL;
    int app_num;
    bool_t found;
    int rc;
    int mpi_errno = MPI_SUCCESS;

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_ARG(pm);
    
    mpi_errno = mpig_bc_get_contact(bc, "PM_WS_APP_NUM", &app_num_str, &found);
    if (mpi_errno)
    {
        sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_get_app_num() failed to extract the application number "
	    "from the specified business card", __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    }

    if (!found)
    {
        sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_get_app_num() could not find the application number "
	    "in the specified business card", __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    }

    rc = sscanf(app_num_str, "%d", &app_num);
    if (rc != 1)
    {
        sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_get_app_num() found a malformed application number "
	    "in the specified business card", __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    }
    
    *app_num_p = app_num;

    return mpi_errno;
}
/* mpig_pm_ws_get_app_num() */

    

/****************************/
/* LOCAL CALLBACK FUNCTIONS */
/****************************/

#undef FUNCNAME
#define FUNCNAME mpig_pm_ws_rz_data_callback
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
static void mpig_pm_ws_rz_data_callback(globus_result_t res, const char *data, size_t length, void *args)
{
    struct rz_data *v = (struct rz_data *) args;

    TEST_RES(res);

    if (!v)
    {
        sprintf(mpig_pm_ws_ErrorMsg, "ERROR: file %s: line %d: mpig_pm_ws_rz_data_callback() passed NULL args", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    } /* endif */

    mpig_mutex_lock(&mpig_pm_ws_Mutex);
    {
	TRY_MALLOC(v->data, char *, length);
	memcpy(v->data, data, length);
	v->length = length;

	mpig_pm_ws_Done = 1;
	mpig_cond_signal(&mpig_pm_ws_Cond);
    }
    mpig_mutex_unlock(&mpig_pm_ws_Mutex);

    return;

} /* end mpig_pm_ws_rz_data_callback() */

/***************************/
/* LOCAL UTILITY FUNCTIONS */
/***************************/

#if 0
static void print_byte_array(char *v, int vlen)
{
    int i;

    fprintf(Log_fp, "%ud: byte array %d bytes >", Pid, vlen); fflush(Log_fp);
    for (i = 0; i < vlen; i ++)
    {
        fprintf(Log_fp, "%c", (isprint(v[i]) ? v[i] : '.')); fflush(Log_fp);
    } /* endfor */
    fprintf(Log_fp, "<\n"); fflush(Log_fp);

} /* end print_byte_array() */
#endif

static globus_result_t set_attr_security(
    globus_soap_message_attr_t          message_attr,
    globus_soap_message_authz_method_t  value,
    globus_bool_t                       encrypt,
    const char *                        id,
    globus_soap_message_auth_method_t   msg_sec)
{
    globus_result_t                     res;
    gss_buffer_desc                     send_tok;
    gss_name_t                          target_name;
    OM_uint32                           min_stat;
    OM_uint32                           maj_stat;

    switch(value)
    {
        case GLOBUS_SOAP_MESSAGE_AUTHZ_IDENTITY:
            send_tok.value = (void *)id;
            send_tok.length = strlen(id) + 1;

            maj_stat = gss_import_name(
                &min_stat,
                &send_tok,
                GSS_C_NT_USER_NAME,
                &target_name);
            if(maj_stat != GSS_S_COMPLETE)
            {
                res = globus_error_put(GLOBUS_ERROR_NO_INFO);
                goto error;
            }

            res = globus_soap_message_attr_set(
                message_attr,
                GLOBUS_SOAP_MESSAGE_AUTHZ_TARGET_NAME_KEY,
                NULL,
                NULL,
                (void *)target_name);
            if(res != GLOBUS_SUCCESS)
            {
                goto error;
            }
            break;

        case GLOBUS_SOAP_MESSAGE_AUTHZ_SELF:
        case GLOBUS_SOAP_MESSAGE_AUTHZ_HOST:
        case GLOBUS_SOAP_MESSAGE_AUTHZ_NONE:
        default:
            break;
    }

    globus_soap_message_attr_set(
        message_attr,
        GLOBUS_SOAP_MESSAGE_AUTHZ_METHOD_KEY,
        NULL,
        NULL,
        (void *) value);

    globus_soap_message_attr_set(
        message_attr,
        GLOBUS_SOAP_MESSAGE_AUTHENTICATION_METHOD_KEY,
        NULL,
        NULL,
        (void *) msg_sec);

    if(encrypt)
    {
        globus_soap_message_attr_set(
            message_attr,
            GLOBUS_SOAP_MESSAGE_AUTH_PROTECTION_KEY,
            NULL,
            NULL,
            (void *) GLOBUS_SOAP_MESSAGE_AUTH_PROTECTION_PRIVACY);
    }
    return GLOBUS_SUCCESS;

error:
    return res;
    /* return 0; */
} /* end set_attr_security() */


#ifdef MPIG_VMPI
/* mpig_pm_ws_gather_subjob_data() using vMPI */
static void mpig_pm_ws_gather_subjob_data(struct rz_data *subjob_data,
			    char *my_checkinbuff, 
			    int hdr_len, 
			    int my_checkinbufflen)
{
    int mysubjobsize;
    int i;
    int *checkinlens = NULL;
    int *displs      = NULL;
    int mycheckinlen = hdr_len+my_checkinbufflen;

    mpig_vmpi_comm_size(MPIG_VMPI_COMM_WORLD, &mysubjobsize);

    if (mpig_pm_ws_MySubjobRank == 0)
    {
	TRY_MALLOC(checkinlens, int *, mysubjobsize*sizeof(int));
	TRY_MALLOC(displs,      int *, mysubjobsize*sizeof(int));
    } /* endif */

    mpig_vmpi_gather(&mycheckinlen,    /* sendbuff  */
		1,               /* sendcount */
		MPIG_VMPI_INT,         /* sendtype  */
		checkinlens,     /* recvbuff  */
		1,               /* recvcount */
		MPIG_VMPI_INT,         /* recvtype  */
		0,               /* root      */
		MPIG_VMPI_COMM_WORLD); /* comm      */
/* fprintf(Log_fp, "mpig_pm_ws_gather_subjob_data(): " "after MPI_Gather mysubjobsize %d ... i submitted mycheckinlen %d\n", mysubjobsize, mycheckinlen); fflush(Log_fp); */

    if (mpig_pm_ws_MySubjobRank == 0)
    {
	char b[BUFFSIZE];

	sprintf(b, "%d ", mysubjobsize);
	displs[0] = strlen(b);
	if (checkinlens[0] < 0)
	{
	    sprintf(mpig_pm_ws_ErrorMsg, "ERROR: %s: line %d: mpig_pm_ws_gather_subjob_data(): received checkinlens[0]=%d (must be >= 0)",
		__FILE__, __LINE__, checkinlens[0]);
	    MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	} /* endif */
	subjob_data->length = strlen(b) + checkinlens[0];
/* fprintf(Log_fp, "mpig_pm_ws_gather_subjob_data(): " "after adding checkinlens[0]=%d: subjob_data->length = %d\n", checkinlens[0], subjob_data->length); fflush(Log_fp); */

	for (i = 1; i < mysubjobsize; i ++)
	{
	    if (checkinlens[i] < 0)
	    {
		sprintf(mpig_pm_ws_ErrorMsg, "ERROR: %s: line %d: mpig_pm_ws_gather_subjob_data(): received checkinlens[%d]=%d (must be >= 0)",
		    __FILE__, __LINE__, i, checkinlens[i]);
		MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	    } /* endif */
	    (subjob_data->length) += checkinlens[i];
/* fprintf(Log_fp, "mpig_pm_ws_gather_subjob_data(): " "after adding checkinlens[%d]=%d: subjob_data->length = %d\n", i, checkinlens[i], subjob_data->length); fflush(Log_fp); */
	    displs[i] = displs[i-1] + checkinlens[i-1];
	} /* endfor */

	TRY_MALLOC(subjob_data->data, char *, subjob_data->length);
	sprintf(subjob_data->data, "%d ", mysubjobsize);

    } /* endif */

    mpig_vmpi_gatherv(my_checkinbuff,    /* sendbuff  */
		mycheckinlen,      /* sendcount */
		MPIG_VMPI_BYTE,          /* sendtype  */
		subjob_data->data, /* recvbuff  */
		checkinlens,       /* recvcounts */
		displs,            /* displs */
		MPIG_VMPI_BYTE,          /* recvtype  */
		0,                 /* root      */
		MPIG_VMPI_COMM_WORLD);   /* comm      */
    /* fprintf(Log_fp, "mpig_pm_ws_gather_subjob_data(): after vMPI checkin my byte array to subjob master %d bytes: ", mycheckinlen); print_byte_array(my_checkinbuff, mycheckinlen); */

    globus_free(checkinlens);
    globus_free(displs);

    return;

} /* end mpig_pm_ws_gather_subjob_data() - vMPI */
#else
/* mpig_pm_ws_gather_subjob_data() using rendezvous service (i.e., without vMPI) */
static void mpig_pm_ws_gather_subjob_data(struct rz_data *subjob_data,
			    char *my_checkinbuff, 
			    int hdr_len, 
			    int my_checkinbufflen)
{

    globus_result_t res;

    /* every process registers their byte array to their subjob rend serv */
    res = globus_rz_sub_register(mpig_pm_ws_SubjobHandle, my_checkinbuff, hdr_len+my_checkinbufflen, &mpig_pm_ws_MySubjobRank); 
    TEST_RES(res);

    /* fprintf(Log_fp, "%ud: after subjob register: assigned mpig_pm_ws_MySubjobRank %d: " "registered buff (hdr_len %d my_checkinbufflen %d) ", Pid, mpig_pm_ws_MySubjobRank, hdr_len, my_checkinbufflen); fflush(Log_fp); print_byte_array(my_checkinbuff, hdr_len+my_checkinbufflen); */

    if (mpig_pm_ws_MySubjobRank == 0)
    {
	globus_reltime_t poll_freq;

	subjob_data->data = NULL;
	subjob_data->length = 0;

	/* fprintf(Log_fp, "%ud: subjob master before globus_rz_sub_data_request_begin()\n", Pid); fflush(Log_fp); */

	mpig_pm_ws_Done = 0;
	GlobusTimeReltimeSet(poll_freq, 1, 0);
	res = globus_rz_sub_data_request_begin(mpig_pm_ws_SubjobHandle, &poll_freq, mpig_pm_ws_rz_data_callback, subjob_data);
	TEST_RES(res);

	/* Wait for result */
	/* fprintf(Log_fp, "%ud: subjob master BEFORE wait loop\n", Pid); fflush(Log_fp); */
	mpig_mutex_lock(&mpig_pm_ws_Mutex);
	while (!mpig_pm_ws_Done)
	{
	    mpig_cond_wait(&mpig_pm_ws_Cond, &mpig_pm_ws_Mutex);
	} /* endwhile */
	mpig_mutex_unlock(&mpig_pm_ws_Mutex);
	/* fprintf(Log_fp, "%ud: subjob master AFTER wait loop\n", Pid); fflush(Log_fp); */

	/* free up some resources in globus_rz_XXX lib */
	/* NICK: need to check if this is the right thing to 
		 do over and over again.
	*/
	res = globus_rz_data_request_finished(mpig_pm_ws_SubjobHandle); TEST_RES(res);
	/* fprintf(Log_fp, "%ud: after globus_rz_data_request_finished\n", Pid); fflush(Log_fp); */
    } /* endif */

    return;

} /* end mpig_pm_ws_gather_subjob_data() - rendezvous service */
#endif

/* format of v = <nsubjobs><singlespace><subjob_0>...<subjob_n-1>
       where <subjob_i> = <nprocs><singlespace><proc_0>...<proc_n-1>
	    where <proc_i> = <nbytes><singlespace><nbytes of data>
 */

static void mpig_pm_ws_process_wholejob_byte_array(char *v, 
					    int vlen,
					    int my_subjobidx, 
					    int rank_in_my_subjob,
					    int *mysubjobsize,
					    char ***byte_arrays, 
					    int **byte_array_lens,
					    char ***boot_cs_array,
					    int *nprocs,
					    int *my_grank) 
{
    int nsubjobs;
    int subjobidx;
    char *cp;

    *byte_arrays     = NULL;
    *byte_array_lens = NULL;
    *nprocs = 0;
    *my_grank = rank_in_my_subjob;

    if (!(cp = v))
    {
	sprintf(mpig_pm_ws_ErrorMsg, "ERROR: %s: line %d: mpig_pm_ws_process_wholejob_byte_array(): passed NULL bytearray", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    } /* endif */

    /* extracting <nsubjobs> */
    mpig_pm_ws_extract_int_and_single_space(v, vlen, &cp, &nsubjobs);
    if (my_subjobidx >= nsubjobs)
    {
	sprintf(mpig_pm_ws_ErrorMsg, "ERROR: %s: line %d: mpig_pm_ws_process_wholejob_byte_array(): my_subjobidx %d >= nsubjobs %d", 
	    __FILE__, __LINE__, my_subjobidx, nsubjobs);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    } /* endif */
    /* fprintf(Log_fp, "%ud: mpig_pm_ws_process_wholejob_byte_array: nsubjobs %d\n", Pid, nsubjobs); fflush(Log_fp); */

    /* processing each subjob one at a time */
    for (subjobidx = 0; subjobidx < nsubjobs; subjobidx ++)
    {
	int i;
	int nsubjobprocs;
	char **subjob_byte_arrays;
	int *subjob_byte_array_lens;
	char **subjob_boot_cs_array;

	mpig_pm_ws_process_subjob_byte_array(v, 
				vlen, 
				&cp, 
				subjobidx, 
				&nsubjobprocs, 
				&subjob_byte_arrays,
				&subjob_byte_array_lens,
				&subjob_boot_cs_array);

	if (my_subjobidx == subjobidx && rank_in_my_subjob >= nsubjobprocs)
	{
	    sprintf(mpig_pm_ws_ErrorMsg, 
		"ERROR: %s: line %d: mpig_pm_ws_process_wholejob_byte_array(): encountered subjobidx %d with %d nprocs "
		"but I'm rank %d in that same subjob", 
		__FILE__, __LINE__, subjobidx, nsubjobprocs, rank_in_my_subjob);
	    MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	}
	else if (my_subjobidx == subjobidx)
	    *mysubjobsize = nsubjobprocs;
	else if (my_subjobidx > subjobidx)
	    *my_grank += nsubjobprocs;

	TRY_REALLOC(*byte_arrays,     *byte_arrays,     char **, sizeof(char *)*(*nprocs+nsubjobprocs));
	TRY_REALLOC(*byte_array_lens, *byte_array_lens, int *,   sizeof(int)   *(*nprocs+nsubjobprocs));
	TRY_REALLOC(*boot_cs_array,   *boot_cs_array,   char **, sizeof(char *)*(*nprocs+nsubjobprocs));

	for (i = 0; i < nsubjobprocs; i ++)
	{
	    (*byte_arrays)[*nprocs+i]     = subjob_byte_arrays[i];
	    (*byte_array_lens)[*nprocs+i] = subjob_byte_array_lens[i];
	    (*boot_cs_array)[*nprocs+i]   = subjob_boot_cs_array[i];
	} /* endfor */
	globus_free(subjob_byte_arrays);
	globus_free(subjob_byte_array_lens);
	globus_free(subjob_boot_cs_array);

	*nprocs += nsubjobprocs;

    } /* endfor */

} /* end mpig_pm_ws_process_wholejob_byte_array() */

/* format of v = <nprocs><singlespace><proc_0>...<proc_n-1>
    where 
    <proc_i> = <nbytes><sp><nbytes of boot CS><kbytes><sp><kbytes of biz card>
*/

static void mpig_pm_ws_process_subjob_byte_array(char *v,
					    int vlen,
					    char **cp, 
					    int SubjobIdx,
					    int *nprocs,
					    char ***subjob_byte_arrays,
					    int **subjob_byte_array_lens,
					    char ***subjob_boot_cs_array)
{
    int proc;

    /* extracting <nprocs> */
    mpig_pm_ws_extract_int_and_single_space(v, vlen, cp, nprocs);
    /* fprintf(Log_fp, "%ud: mpig_pm_ws_process_subjob_byte_array: nprocs %d\n", Pid, *nprocs); fflush(Log_fp); */

    TRY_MALLOC(*subjob_byte_arrays,     char **, (*nprocs)*sizeof(char *));
    TRY_MALLOC(*subjob_byte_array_lens, int *,   (*nprocs)*sizeof(int));
    TRY_MALLOC(*subjob_boot_cs_array,   char **, (*nprocs)*sizeof(char *));

    for (proc = 0; proc < *nprocs; proc ++)
    {
	int nbytes;

	/* extract this proc's checkin length which includes boot CS len,
	   boot CS, biz card len, and biz card ... just tossing this checkin len
	*/
	mpig_pm_ws_extract_int_and_single_space(v, vlen, cp, &nbytes);
	/* fprintf(Log_fp, "%ud: mpig_pm_ws_process_subjob_byte_array: proccheckinlen %d ... tossing it\n", Pid, nbytes); fflush(Log_fp); */

	/* extract boot CS */
	mpig_pm_ws_extract_int_and_single_space(v, vlen, cp, &nbytes);
	/* fprintf(Log_fp, "%ud: mpig_pm_ws_process_subjob_byte_array: bootCSlen %d\n", Pid, nbytes); fflush(Log_fp); */

	if (nbytes < 0)
	{
	    sprintf(mpig_pm_ws_ErrorMsg, 
		"ERROR: %s: line %d: mpig_pm_ws_process_subjob_byte_array(): extracted invalid nbytes %d for proc %d of subjob %d", 
		__FILE__, __LINE__, nbytes, proc, SubjobIdx);
	    MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	}
	else if (*cp+nbytes-v > vlen)
	{
	    sprintf(mpig_pm_ws_ErrorMsg, 
		"ERROR: %s: line %d: mpig_pm_ws_process_subjob_byte_array(): "
		"ran off end of bytearray reading data %d bytes for proc %d of subjob %d", 
		__FILE__, __LINE__, nbytes, proc, SubjobIdx);
	    MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	} /* endif */

	if (nbytes > 0)
	{
	    TRY_MALLOC((*subjob_boot_cs_array)[proc], char *, nbytes);
	    memcpy((*subjob_boot_cs_array)[proc], *cp, (size_t) nbytes);
	}
	else
	    (*subjob_boot_cs_array)[proc] = NULL;

	*cp = *cp + nbytes;

	/* extract biz card */

	mpig_pm_ws_extract_int_and_single_space(v, vlen, cp, &nbytes);
	/* fprintf(Log_fp, "%ud: mpig_pm_ws_process_subjob_byte_array: bizcardlen %d\n", Pid, nbytes); fflush(Log_fp); */

	if (nbytes < 0)
	{
	    sprintf(mpig_pm_ws_ErrorMsg, 
		"ERROR: %s: line %d: mpig_pm_ws_process_subjob_byte_array(): extracted invalid nbytes %d for proc %d of subjob %d", 
		__FILE__, __LINE__, nbytes, proc, SubjobIdx);
	    MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	}
	else if (*cp+nbytes-v > vlen)
	{
	    sprintf(mpig_pm_ws_ErrorMsg, 
		"ERROR: %s: line %d: mpig_pm_ws_process_subjob_byte_array(): "
		"ran off end of bytearray reading data %d bytes for proc %d of subjob %d", 
		__FILE__, __LINE__, nbytes, proc, SubjobIdx);
	    MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	} /* endif */

	if (nbytes > 0)
	{
	    TRY_MALLOC((*subjob_byte_arrays)[proc], char *, nbytes);
	    memcpy((*subjob_byte_arrays)[proc], *cp, (size_t) nbytes);
	}
	else
	    (*subjob_byte_arrays)[proc] = NULL;

	(*subjob_byte_array_lens)[proc] = nbytes;
	*cp = *cp + nbytes;

    } /* endfor */

} /* end mpig_pm_ws_process_subjob_byte_array() */

static void mpig_pm_ws_extract_int_and_single_space(char *v, int vlen, char **cp, int *val)
{
    int rc;

    /* sscanf returns the number of items assigned */
    rc = sscanf(*cp, "%d ", val);
    if (rc == 0 || rc == EOF)
    {
	sprintf(mpig_pm_ws_ErrorMsg, "ERROR: %s: line %d: mpig_pm_ws_extract_int_and_single_space(): sscanf for val returned %d",
	    __FILE__, __LINE__, rc);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    } /* endif */

    if ((*cp = index(*cp, ' ')) != NULL)
	*cp = *cp + 1;
    else
    {
	sprintf(mpig_pm_ws_ErrorMsg, "ERROR: %s: line %d: mpig_pm_ws_extract_int_and_single_space(): no space found after val", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    } /* endif */

    if (*cp-v > vlen)
    {
	sprintf(mpig_pm_ws_ErrorMsg, "ERROR: %s: line %d: mpig_pm_ws_extract_int_and_single_space: "
	"ran off end of bytearray after reading val + single space", 
	    __FILE__, __LINE__);
	MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
    } /* endif */

} /* end mpig_pm_ws_extract_int_and_single_space() */

#ifndef MPIG_VMPI
/* 
 * assumes that ALL children are listening on handles created
 * with the same stack as passed with 'stack' arg
 *
 *  also assumes that data has been received from parent
 *  and is in 'v' and 'vlen'
 *
 * for non-vMPI environments this function uses globus functions
 * to distribute information within a subjob using a binomial tree.
 * 
 * def of binomial tree:
 *   A binomial tree, B_k (k>=0), is an ordered tree of order k.  The root
 *   has k children (ordered from left to right) where each child is
 *   binomial tree B_(k-1), B_(k-2), ..., B_0.
 * note a B_k tree 
 *     - has 2**k nodes
 *     - height = k
 *     - there are k choose i nodes at dept i
 ****************************************************************************
 * ex
 *     B_0    root            =     0
 *
 *     B_1    root            =     0
 *             |                    | 
 *            B_0                   0
 *
 *     B_2    root            =          0
 *             |                         |
 *        +----+----+               +----+----+
 *        |         |               |         |
 *       B_1       B_0              0         0
 *                                  |      
 *                                  0
 *     B_3    root            =             0
 *             |                            |
 *        +----+----+               +-------+----+
 *        |    |    |               |       |    |
 *       B_2  B_1  B_0              0       0    0
 *                                  |       |      
 *                              +---+---+   |
 *                              |       |   |
 *                              0       0   0
 *                              |
 *                              0
 ****************************************************************************
 *   This uses a fairly basic recursive subdivision algorithm.
 *   The root sends to the process size/2 away; the receiver becomes
 *   a root for a subtree and applies the same process. 
 *
 *   So that the new root can easily identify the size of its
 *   subtree, the (subtree) roots are all powers of two (relative to the root)
 *   If m = the first power of 2 such that 2^m >= the size of the
 *   communicator, then the subtree at root at 2^(m-k) has size 2^k
 *   (with special handling for subtrees that aren't a power of two in size).
 *
 *
 * basically, consider every rank as a binary number.  find the least
 * significant (rightmost) '1' bit (LSB) in each such rank.  so, for each
 * rank you have the following bitmask:
 *
 *                 xxxx100...0
 *                 ^^^^^^^^^^^
 *                 |   |   |
 *         0 or more  LSB  +---- followed by zero or more 0's
 *          leading
 *          bits
 *  or
 *                 0.........0 (root only)
 *
 * each non-root node receives from the src who has the same rank
 * BUT has the LSB=0, so, 4 (100), 2 (01), and 1 (1) all rcv from 0.
 * 5 (101) and 6 (110) both rcv from 4 (100).
 * the sends are ordered from left->right by sending first to the
 * bit to the immediate right of the LSB.  so, 4 (100) sends first
 * to 6 (110) and then to 5 (101).  
 *
 * note that under this scheme the root (0) receives from nobody
 * and all the leaf nodes (sending to nobody) are the odd-numbered nodes.
 *   
 */

static int mpig_pm_ws_distribute_byte_array_to_my_children(char *v, 
						int vlen,
						int my_subjob_rank,
						int my_subjob_size,
						int my_grank,
						char **contact_strings)
{
    int mask;
    globus_xio_handle_t child_handle;
    globus_size_t nbytes;
    globus_result_t res;
    char buff[BUFFSIZE];

    /*
     *   Do subdivision.  There are two phases:
     *   1. Wait for arrival of data.  Because of the power of two nature
     *      of the subtree roots, the source of this message is alwyas the
     *      process whose relative rank has the least significant bit CLEARED.
     *      That is, process 4 (100) receives from process 0, process 7 (111) 
     *      from process 6 (110), etc.   
     *   2. Forward to my subtree
     *
     *   Note that the process that is the tree root is handled automatically
     *   by this code, since it has no bits set.
     */

    /* loop to find LSB */
    for (mask = 0x1;
        mask < my_subjob_size && !(my_subjob_rank & mask);
            mask <<= 1)
    ;

    /*
     *   This process is responsible for all processes that have bits set from
     *   the LSB upto (but not including) mask.  Because of the "not including",
     *   we start by shifting mask back down one.
     */
    sprintf(buff, "%d", vlen);
    mask >>= 1;
    while (mask > 0)
    {
        if (my_subjob_rank + mask < my_subjob_size)
        {
	    /* making sure that there's a boot contact string */
	    if (!(contact_strings[my_grank+mask]))
	    {
		sprintf(mpig_pm_ws_ErrorMsg, 
		"ERROR: %s: line %d: mpig_pm_ws_distribute_byte_array_to_my_children() found NULL contact_strings[%d]: my_grank %d mask %d", 
		    __FILE__, __LINE__, my_grank+mask, my_grank, mask);
		MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	    } /* endif */

	    /* need to create before each open */
	    res = globus_xio_handle_create(&child_handle, mpig_pm_ws_SubjobBootStack);     TEST_RES(res);
	    /* 
	     * dest_grank = (grank_mysubjob_p0) + my_subjob_rank + mask
	     *            = (my_grank - my_subjob_rank) + my_subjob_rank + mask
	     *            = my_grank + mask
	     */
	    /* fprintf(Log_fp, "%ud: my_grank %d before globus_xio_open() to child grank %d\n", Pid, my_grank, my_grank+mask); fflush(Log_fp); */
	    res = globus_xio_open(child_handle, contact_strings[my_grank+mask], NULL);     TEST_RES(res);
	    /* fprintf(Log_fp, "%ud: my_grank %d after globus_xio_open() to child grank %d\n", Pid, my_grank, my_grank+mask); fflush(Log_fp); */

	    res = globus_xio_write(child_handle, buff, BUFFSIZE, BUFFSIZE, &nbytes, NULL); TEST_RES(res);
	    if (nbytes != BUFFSIZE)
	    {
		sprintf(mpig_pm_ws_ErrorMsg, 
		    "ERROR: %s: line %d: mpig_pm_ws_distribute_byte_array_to_my_children(): ba_len wrote only %d bytes, requested %d bytes", 
		    __FILE__, __LINE__, (int) nbytes, BUFFSIZE);
		MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
	    } /* endif */

	    res = globus_xio_write(child_handle, v, (globus_size_t) vlen, (globus_size_t) vlen, &nbytes, NULL); TEST_RES(res);

	    if (nbytes != vlen)
	    {
		sprintf(mpig_pm_ws_ErrorMsg, 
		    "ERROR: %s: line %d: mpig_pm_ws_distribute_byte_array_to_my_children(): ba wrote only %d bytes, requested %d bytes", 
		    __FILE__, __LINE__, (int) nbytes, vlen);
		MPID_Abort(NULL, MPI_ERR_OTHER, 1, mpig_pm_ws_ErrorMsg);
		return -1;
	    } /* endif */

	    res = globus_xio_close(child_handle, NULL);    TEST_RES(res);
        } /* endif */

        mask >>= 1;
    } /* endwhile */

    return 0;

} /* end mpig_pm_ws_distribute_byte_array_to_my_children() */

#endif /* !defined(MPIG_VMPI) */

#else /* !defined(HAVE_GLOBUS_RENDEZVOUS_MODULE) */

mpig_pm_t mpig_pm_ws =
{
    "web services",
    NULL
};

#endif /* if/else defined(HAVE_GLOBUS_RENDEZVOUS_MODULE) */
