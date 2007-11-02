/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

/*
 * This file contains an implementation of process management routines that interface with the Globus 2.x DUROC/GRAM service.
 */
#include "mpidimpl.h"

#if defined(MPIG_VMPI)

/* define MPIG_PM_VMPI_ENABLE_DEBUG to force early initialization of the debug logging subsystem.  see comments in
   mpig_pm_vmpi_init(). */
#undef MPIG_PM_VMPI_ENABLE_DEBUG

/*
 * prototypes and data structures exposing the vendor MPI process management class
 */
static int mpig_pm_vmpi_init(int * argc, char *** argv, mpig_pm_t * pm, bool_t * my_pm_p);

static int mpig_pm_vmpi_finalize(mpig_pm_t * pm);

static int mpig_pm_vmpi_abort(mpig_pm_t * pm, int exit_code);

static int mpig_pm_vmpi_exchange_business_cards(mpig_pm_t * pm, mpig_bc_t * bc, mpig_bc_t ** bcs_p);

static int mpig_pm_vmpi_free_business_cards(mpig_pm_t * pm, mpig_bc_t * bcs);

static int mpig_pm_vmpi_get_pg_size(mpig_pm_t * pm, int * pg_size);

static int mpig_pm_vmpi_get_pg_rank(mpig_pm_t * pm, int * pg_rank);

static int mpig_pm_vmpi_get_pg_id(mpig_pm_t * pm, const char ** pg_id_p);

static int mpig_pm_vmpi_get_app_num(mpig_pm_t * pm, const mpig_bc_t * bc, int * app_num);

MPIG_STATIC mpig_pm_vtable_t mpig_pm_vmpi_vtable =
{
    mpig_pm_vmpi_init,
    mpig_pm_vmpi_finalize,
    mpig_pm_vmpi_abort,
    mpig_pm_vmpi_exchange_business_cards,
    mpig_pm_vmpi_free_business_cards,
    mpig_pm_vmpi_get_pg_size,
    mpig_pm_vmpi_get_pg_rank,
    mpig_pm_vmpi_get_pg_id,
    mpig_pm_vmpi_get_app_num,
    mpig_pm_vtable_last_entry
};

mpig_pm_t mpig_pm_vmpi =
{
    "vendor MPI",
    &mpig_pm_vmpi_vtable
};


/*
 * internal data and state
 */
MPIG_STATIC enum
{
    MPIG_PM_VMPI_STATE_UNINITIALIZED = 0,
    MPIG_PM_VMPI_STATE_INITIALIZED,
    MPIG_PM_VMPI_STATE_GOT_PG_INFO,
    MPIG_PM_VMPI_STATE_FINALIZED
}
mpig_pm_vmpi_state = MPIG_PM_VMPI_STATE_UNINITIALIZED;

/* control this seting with the environment variable MPIG_USE_SYSTEM_ABORT */
MPIG_STATIC bool_t mpig_pm_vmpi_use_system_abort = FALSE;

/* the size of the initial process group (comm world), and the rank of the local process within it */
MPIG_STATIC int mpig_pm_vmpi_pg_size = -1;
MPIG_STATIC int mpig_pm_vmpi_pg_rank = -1;

/* the unique identification string of the initial process group */
MPIG_STATIC const char * mpig_pm_vmpi_pg_id = NULL;

/* a duplicate of MPIG_VMPI_COMM_WORLD to be used by this module */
MPIG_STATIC mpig_vmpi_comm_t mpig_pm_vmpi_comm;


/*
 * mpig_pm_vmpi_init()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_vmpi_init
int mpig_pm_vmpi_init(int * argc, char *** argv, mpig_pm_t * const pm, bool_t * const my_pm_p)
{
    const char fcname[] = "mpig_pm_vmpi_init";
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_vmpi_init);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_ARG(pm);
    MPIG_UNUSED_ARG(pm);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_vmpi_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));

    /* 
     * {
     *     int env_found;
     *     int use_vmpi_pm;
     *  
     *     env_found = MPIU_GetEnvBool("MPIG_USE_VMPI_PM", &use_vmpi_pm);
     *     if (env_found == FALSE || use_vmpi_pm == FALSE)
     *     {
     *         *my_pm_p = FALSE;
     *         goto fn_return;
     *     }
     * }
     */

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PM, "initializing vendor MPI"));

    /* NOTE: this routine assumes that the vendor MPI has already been initialized */
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PM, "getting size and rank from vendor MPI"));
    vrc = mpig_vmpi_comm_size(MPIG_VMPI_COMM_WORLD, &mpig_pm_vmpi_pg_size);
    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_size", &mpi_errno);
        
    vrc = mpig_vmpi_comm_rank(MPIG_VMPI_COMM_WORLD, &mpig_pm_vmpi_pg_rank);
    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_rank", &mpi_errno);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PM, "creating a duplicate of MPI_COMM_WORLD"));
    vrc = mpig_vmpi_comm_dup(MPIG_VMPI_COMM_WORLD, &mpig_pm_vmpi_comm);
    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_dup", &mpi_errno);

    /* determine if the job should be cancelled or abort() should be called if an error occurs.  this feature is used to get a
       core dump while debugging and is not some users will normally set. */
    MPIU_GetEnvBool("MPIG_USE_SYSTEM_ABORT",&mpig_pm_vmpi_use_system_abort);
     
    mpig_pm_vmpi_state = MPIG_PM_VMPI_STATE_INITIALIZED;

    /* NOTE: THIS IS A HACK!  now that we have the process topology information, initialize the debugging system.  normally, the
       debug logging subsystem is intialized in MPID_Init(); however, initializng it here allows us to catch some activities that
       may occur before MPID_Init() has all of the information.  NOTE: the process group id is not know until the business card
       exchange occurs, and this remains set to "(unknown)" until after the exchange has taken place and MPID_Init() sets the
       value.  the upshot is that the log filename and the output within it will contain "(unknown)" as the process group id, but
       this is a small tradeoff for catching early rece conditions. */
#   if defined(MPIG_PM_VMPI_ENABLE_DEBUG) && defined(MPIG_DEBUG)
    {
        mpig_process.my_pg_rank = mpig_pm_vmpi_pg_rank;
        mpig_debug_init();
    }
#   endif

    *my_pm_p = TRUE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_vmpi_init);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
        goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_vmpi_init() */


/*
 * mpig_pm_vmpi_finalize()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_vmpi_finalize
int mpig_pm_vmpi_finalize(mpig_pm_t * const pm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_vmpi_finalize);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_ARG(pm);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_vmpi_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state == MPIG_PM_VMPI_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_OTHER,
        "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state == MPIG_PM_VMPI_STATE_FINALIZED), mpi_errno, MPI_ERR_OTHER, "**globus|pm_finalized");
    
    mpig_pm_vmpi_state = MPIG_PM_VMPI_STATE_FINALIZED;

    vrc = mpig_vmpi_comm_free(&mpig_pm_vmpi_comm);
    MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Comm_dup", &mpi_errno, {;});
        
    /* NOTE: this routine assumes that the vendor MPI will be shutdown sometime after this routine completes */
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_vmpi_finalize);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
        goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_vmpi_finalize() */


/*
 * mpig_pm_vmpi_abort()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_vmpi_abort
int mpig_pm_vmpi_abort(mpig_pm_t * const pm, int exit_code)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_vmpi_abort);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_ARG(pm);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_vmpi_abort);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));

    if (mpig_pm_vmpi_use_system_abort)
    {
        MPIU_Internal_error_printf("mpig_pm_vmpi_abort() called with MPIG_USE_SYSTEM_ABORT set.  calling abort().\n");
        abort();
    }

    vrc = mpig_vmpi_abort(mpig_pm_vmpi_comm, exit_code);
    MPIU_Internal_error_printf("WARNING: %s: failed to cancel the local (my) subjob using the vendor MPI_Abort() function.  "
        "Attempting to cancel the local subjob using the GRAM cancel function.\n", fcname);

    fclose(stdout);
    fclose(stderr);

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_vmpi_abort);
    return mpi_errno;
}
/* mpig_pm_vmpi_abort() */


/*
 * mpig_pm_vmpi_exchange_business_cards()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_vmpi_exchange_business_cards
int mpig_pm_vmpi_exchange_business_cards(mpig_pm_t * const pm, mpig_bc_t * const bc, mpig_bc_t ** const bcs_ptr)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIU_CHKLMEM_DECL(3);
    MPIU_CHKPMEM_DECL(1);
    char * bc_str = NULL;
    int bc_len;
    int * bc_lens;
    int * bc_displs;
    int bcs_len;
    char * bcs_str;
    mpig_bc_t * bcs;
    int p;
    int errors = 0;
    int mrc;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_vmpi_exchange_business_cards);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_ARG(pm);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_vmpi_exchange_business_cards);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering: bc=" MPIG_PTR_FMT, MPIG_PTR_CAST(bc)));

    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state == MPIG_PM_VMPI_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_OTHER,
        "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state == MPIG_PM_VMPI_STATE_FINALIZED), mpi_errno, MPI_ERR_OTHER,
        "**globus|pm_finalized");

    /* if this process is the process group master, then create a process group id and add it to the business card */
    if (mpig_pm_vmpi_pg_rank == 0)
    {
        mpig_uuid_t uuid;
        char uuid_str[MPIG_UUID_MAX_STR_LEN + 1];
        
        mpi_errno = mpig_uuid_generate(&uuid);
        MPIU_ERR_CHKANDSTMT((mpi_errno), mpi_errno, MPI_ERR_OTHER, {errors++; goto end_create_pg_id_block;},
            "**globus|pm_pg_id_create");

        mpig_uuid_unparse(&uuid, uuid_str);
        mpi_errno = mpig_bc_add_contact(bc, "PM_VMPI_PG_ID", uuid_str);
        MPIU_ERR_CHKANDSTMT1((mpi_errno), mpi_errno, MPI_ERR_OTHER,  {errors++; goto end_create_pg_id_block;},
            "**globus|bc_add_contact", "**globus|bc_add_contact %s", "PM_VMPI_PG_ID");

      end_create_pg_id_block:
        if (mpi_errno) goto fn_fail;
    }
    
    /* convert the business card object for this process into a string that can be distributed to other processes */
    mpi_errno = mpig_bc_serialize_object(bc, &bc_str);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_serialize");

    /* allocate memory for the array of business cards objects that will result from the exchange, as well as the intermediate
       arrays needed to hold the stringified representations of those objects */
    /* gather the sizes of each of the business cards */
    MPIU_CHKLMEM_MALLOC(bc_lens, int *, mpig_pm_vmpi_pg_size * sizeof(int), mpi_errno, "array of business cards lengths");
    MPIU_CHKLMEM_MALLOC(bc_displs, int *, mpig_pm_vmpi_pg_size * sizeof(int), mpi_errno, "array of business cards displacements");

    bc_len = strlen(bc_str) + 1;

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "gathering business card sizes: my_bc_len=%d", bc_len));

    vrc = mpig_vmpi_allgather(&bc_len, 1, MPIG_VMPI_INT, bc_lens, 1, MPIG_VMPI_INT, mpig_pm_vmpi_comm);
    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Allgather", &mpi_errno);

    bcs_len = 0;
    for (p = 0; p < mpig_pm_vmpi_pg_size; p++)
    {
        bc_displs[p] = bcs_len;
        bcs_len += bc_lens[p];
    }
    
    /* perform the all-to-all distribution of the business cards */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "gathering business card data: bcs_len=%d", bcs_len));

    MPIU_CHKLMEM_MALLOC(bcs_str, char *, bcs_len * sizeof(char), mpi_errno, "serialized business cards");
    vrc = mpig_vmpi_allgatherv(bc_str, bc_len, MPIG_VMPI_CHAR, bcs_str, bc_lens, bc_displs, MPIG_VMPI_CHAR, mpig_pm_vmpi_comm);
    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Allgatherv", &mpi_errno);

    /* convert each serialized business card back to business card object */
    MPIU_CHKPMEM_MALLOC(bcs, mpig_bc_t *, mpig_pm_vmpi_pg_size * sizeof(mpig_bc_t), mpi_errno, "array of business cards");
    
    for (p = 0; p < mpig_pm_vmpi_pg_size; p++)
    {
        mpig_bc_construct(&bcs[p]);
        mrc = mpig_bc_deserialize_object(&bcs_str[bc_displs[p]], &bcs[p]);
        MPIU_ERR_CHKANDSTMT((mrc), mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc);}, "**globus|bc_deserialize");
    }

    /* get the process group id from the process group master */
    {
        char * pg_id_str;
        bool_t found;
        
        mrc = mpig_bc_get_contact(&bcs[0], "PM_VMPI_PG_ID", &pg_id_str, &found);
        MPIU_ERR_CHKANDSTMT1((mrc), mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc); goto end_get_pg_id_block;},
            "**globus|bc_get_contact", "**globus|bc_get_contact %s", "PM_VMPI_PG_ID");
        if (found)
        {
            mpig_pm_vmpi_pg_id = MPIU_Strdup(pg_id_str);
            MPIU_ERR_CHKANDSTMT1((mpig_pm_vmpi_pg_id == NULL), mpi_errno, MPI_ERR_OTHER, {errors++; goto end_get_pg_id_block;},
                "**nomem", "nomem %s", "process group id");
            mpig_bc_free_contact(pg_id_str);
        }
        else
        {
            MPIU_ERR_SETANDSTMT1(mpi_errno, MPI_ERR_OTHER, {errors++; goto end_get_pg_id_block;}, "**globus|bc_contact_not_found",
                "**globus|bc_contact_not_found %s", "PM_VMPI_PG_ID");
        }

      end_get_pg_id_block: ;
    }
    
    if (mpi_errno) goto fn_fail;
    
    MPIU_CHKPMEM_COMMIT();
    *bcs_ptr = bcs;
    
    mpig_pm_vmpi_state = MPIG_PM_VMPI_STATE_GOT_PG_INFO;
    
  fn_return:
    /* free the memory used by the stringified respresentation of business card for this process */
    if (bc_str) mpig_bc_free_serialized_object(bc_str);

    MPIU_CHKLMEM_FREEALL();
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
        "exiting: bcs=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(*bcs_ptr), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_vmpi_exchange_business_cards);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
        MPIU_CHKPMEM_REAP();
        *bcs_ptr = NULL;
        goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_vmpi_exchange_business_cards() */


/*
 * mpig_pm_vmpi_free_business_cards()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_vmpi_free_business_cards
int mpig_pm_vmpi_free_business_cards(mpig_pm_t * const pm, mpig_bc_t * const bcs)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int p;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_vmpi_free_business_cards);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_ARG(pm);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_vmpi_free_business_cards);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering: bcs=" MPIG_PTR_FMT, MPIG_PTR_CAST(bcs)));

    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state == MPIG_PM_VMPI_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_OTHER,
        "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state == MPIG_PM_VMPI_STATE_FINALIZED), mpi_errno, MPI_ERR_OTHER, "**globus|pm_finalized");
    
    for (p = 0; p < mpig_pm_vmpi_pg_size; p++)
    {
        mpig_bc_destruct(&bcs[p]);
    }
    MPIU_Free(bcs);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_vmpi_free_business_cards);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
        goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_vmpi_free_business_cards() */


/*
 * mpig_pm_vmpi_get_pg_size()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_vmpi_get_pg_size
int mpig_pm_vmpi_get_pg_size(mpig_pm_t * const pm, int * const pg_size)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_vmpi_get_pg_size);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_ARG(pm);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_vmpi_get_pg_size);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state == MPIG_PM_VMPI_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_OTHER,
        "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state == MPIG_PM_VMPI_STATE_FINALIZED), mpi_errno, MPI_ERR_OTHER,
        "**globus|pm_finalized");
    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state != MPIG_PM_VMPI_STATE_GOT_PG_INFO), mpi_errno, MPI_ERR_OTHER,
        "**globus|pm_no_exchange");

    *pg_size = mpig_pm_vmpi_pg_size;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
        "exiting: pg_size=%d, mpi_errno=" MPIG_ERRNO_FMT, (mpi_errno) ? -1 : *pg_size, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_vmpi_get_pg_size);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
        goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_vmpi_get_pg_size() */


/*
 * mpig_pm_vmpi_get_pg_rank()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_vmpi_get_pg_rank
int mpig_pm_vmpi_get_pg_rank(mpig_pm_t * const pm, int * const pg_rank)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_vmpi_get_pg_rank);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_ARG(pm);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_vmpi_get_pg_rank);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state == MPIG_PM_VMPI_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_OTHER,
        "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state == MPIG_PM_VMPI_STATE_FINALIZED), mpi_errno, MPI_ERR_OTHER,
        "**globus|pm_finalized");
    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state != MPIG_PM_VMPI_STATE_GOT_PG_INFO), mpi_errno, MPI_ERR_OTHER,
        "**globus|pm_no_exchange");

    *pg_rank = mpig_pm_vmpi_pg_rank;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
                       "exiting: pg_rank=%d, mpi_errno=" MPIG_ERRNO_FMT, (mpi_errno) ? -1 : *pg_rank, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_vmpi_get_pg_rank);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
        goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_vmpi_get_pg_rank() */


/*
 * mpig_pm_vmpi_get_pg_id()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_vmpi_get_pg_id
int mpig_pm_vmpi_get_pg_id(mpig_pm_t * const pm, const char ** const pg_id)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_vmpi_get_pg_id);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_ARG(pm);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_vmpi_get_pg_id);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state == MPIG_PM_VMPI_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_OTHER,
        "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state == MPIG_PM_VMPI_STATE_FINALIZED), mpi_errno, MPI_ERR_OTHER,
        "**globus|pm_finalized");
    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state != MPIG_PM_VMPI_STATE_GOT_PG_INFO), mpi_errno, MPI_ERR_OTHER,
        "**globus|pm_no_exchange");

    *pg_id = mpig_pm_vmpi_pg_id;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM,
                       "exiting: pg_id=%s, mpi_errno=" MPIG_ERRNO_FMT, (mpi_errno) ? "(null)" : *pg_id, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_vmpi_get_pg_id);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
        goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_vmpi_get_pg_id() */


/*
 * mpig_pm_vmpi_get_app_num()
 */
#undef FUNCNAME
#define FUNCNAME mpig_pm_vmpi_get_app_num
int mpig_pm_vmpi_get_app_num(mpig_pm_t * const pm, const mpig_bc_t * const bc, int * const app_num_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_pm_vmpi_get_app_num);

    MPIG_UNUSED_VAR(fcname);
    MPIG_UNUSED_ARG(pm);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_pm_vmpi_get_app_num);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "entering"));

    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state == MPIG_PM_VMPI_STATE_UNINITIALIZED), mpi_errno, MPI_ERR_OTHER,
        "**globus|pm_not_init");
    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state == MPIG_PM_VMPI_STATE_FINALIZED), mpi_errno, MPI_ERR_OTHER,
        "**globus|pm_finalized");
    MPIU_ERR_CHKANDJUMP((mpig_pm_vmpi_state != MPIG_PM_VMPI_STATE_GOT_PG_INFO), mpi_errno, MPI_ERR_OTHER,
        "**globus|pm_no_exchange");

    *app_num_p = 0;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PM, "exiting: app_num=%d, mpi_errno=" MPIG_ERRNO_FMT,
        0, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_pm_vmpi_get_app_num);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
        goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_pm_vmpi_get_app_num() */

#else /* !defined(MPIG_VMPI) */

mpig_pm_t mpig_pm_vmpi =
{
    "vendor MPI",
    NULL
};

#endif /* #if/else defined(MPIG_VMPI) */
