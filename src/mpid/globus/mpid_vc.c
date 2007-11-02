/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"


#if !defined(MPIG_VCRT_SERIALIZE_BC_ALLOC_SIZE)
#define MPIG_VCRT_SERIALIZE_BC_ALLOC_SIZE 128
#endif

#if !defined(MPIG_VCRT_SERIALIZE_PG_ALLOC_SIZE)
#define MPIG_VCRT_SERIALIZE_PG_ALLOC_SIZE 1024
#endif

#if !defined(MPIG_VCRT_DESERIALIZE_ALLOC_SIZE)
#define MPIG_VCRT_DESERIALIZE_ALLOC_SIZE 1024
#endif


/*
 * MPID_VCRT_Create()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Create
int MPID_VCRT_Create(int size, mpig_vcrt_t ** vcrt_ptr)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vcrt_t * vcrt;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_VCRT_CREATE);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCRT_CREATE);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_COMM, "entering: size=%d, vcrt_ptr="
	MPIG_PTR_FMT, size, MPIG_PTR_CAST(vcrt_ptr)));
    
    vcrt = MPIU_Malloc(sizeof(mpig_vcrt_t) + (size - 1) * sizeof(mpig_vc_t *));
    MPIU_ERR_CHKANDJUMP1((vcrt == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "virtual connection reference table");

    mpig_vcrt_mutex_construct(vcrt);
    mpig_vcrt_rc_acq(vcrt, FALSE);
    {
	vcrt->ref_count = 1;
	vcrt->size = size;
    }
    mpig_vcrt_rc_rel(vcrt, TRUE);
    
    *vcrt_ptr = vcrt;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_COMM, "exiting: vcrt_ptr=" MPIG_PTR_FMT
	", vcrt=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vcrt_ptr), MPIG_PTR_CAST(vcrt), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCRT_CREATE);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* MPID_VCRT_Create() */


/*
 * MPID_VCRT_Add_ref()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Add_ref
int MPID_VCRT_Add_ref(mpig_vcrt_t * vcrt)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_VCRT_ADD_REF);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCRT_ADD_REF);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_COMM, "entering: vcrt=" MPIG_PTR_FMT,
	MPIG_PTR_CAST(vcrt)));
    
    mpig_vcrt_mutex_lock(vcrt);
    {
	vcrt->ref_count += 1;
    }
    mpig_vcrt_mutex_unlock(vcrt);
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_COMM, "exiting: vcrt=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vcrt), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCRT_ADD_REF);
    return mpi_errno;
}
/* MPID_VCRT_Add_ref() */


/*
 * MPID_VCRT_Release()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Release
int MPID_VCRT_Release(mpig_vcrt_t * vcrt)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t inuse;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_VCRT_RELEASE);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCRT_RELEASE);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_COMM, "entering: vcrt=" MPIG_PTR_FMT,
	MPIG_PTR_CAST(vcrt)));

    mpig_vcrt_mutex_lock(vcrt);
    {
	inuse = (--vcrt->ref_count);
    }
    mpig_vcrt_mutex_unlock(vcrt);

    if (inuse == FALSE)
    {
	int p;
	
	for (p = 0; p < vcrt->size; p++)
	{
	    mpig_vc_release_ref(vcrt->vcr_table[p]);
	}
	
	vcrt->size = 0;
	mpig_vcrt_mutex_destruct(vcrt);
	MPIU_Free(vcrt);
    }

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_COMM, "exiting: vcrt=" MPIG_PTR_FMT
	"mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vcrt), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCRT_RELEASE);
    return mpi_errno;
}
/* MPID_VCRT_Release() */


/*
 * MPID_VCRT_Get_ptr()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Get_ptr
int MPID_VCRT_Get_ptr(mpig_vcrt_t * vcrt, mpig_vc_t *** vcr_array_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_VCRT_GET_PTR);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCRT_GET_PTR);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_COMM, "entering: vcrt=" MPIG_PTR_FMT,
	MPIG_PTR_CAST(vcrt)));
    
    mpig_vcrt_rc_acq(vcrt, TRUE);
    {
	*vcr_array_p = vcrt->vcr_table;
    }
    mpig_vcrt_rc_rel(vcrt, FALSE);
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_COMM, "exiting: vcrt=" MPIG_PTR_FMT
	", vcr_array=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vcrt), MPIG_PTR_CAST(*vcr_array_p), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCRT_GET_PTR);
    return mpi_errno;
}
/* MPID_VCRT_Get_ptr() */


/*
 * MPID_VCR_Dup()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCR_Dup
int MPID_VCR_Dup(mpig_vc_t * orig_vcr, mpig_vc_t ** new_vcr_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t vc_was_inuse;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_VCR_DUP);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCR_DUP);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_COMM, "entering: orig_vcr=" MPIG_PTR_FMT
	", new_vcr_p=" MPIG_PTR_FMT, MPIG_PTR_CAST(orig_vcr), MPIG_PTR_CAST(new_vcr_p)));

    mpig_vc_mutex_lock(orig_vcr);
    {
	mpig_vc_inc_ref_count(orig_vcr, &vc_was_inuse);
    }
    mpig_vc_mutex_unlock(orig_vcr);
    /* if (vc_was_inuse == FALSE) mpig_pg_add_ref(pg); -- not necessary since orig_vcr already has a reference to the VC and thus
       'vc_was_inuse' will always be true.  */

    *new_vcr_p = orig_vcr;
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_COMM, "exiting: orig_vcr=" MPIG_PTR_FMT
	", new_vcr_p=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(orig_vcr), MPIG_PTR_CAST(new_vcr_p),
	mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCR_DUP);
    return mpi_errno;
}
/* MPID_VCR_Dup() */


/*
 * MPID_VCR_Get_lpid()
 */
#undef FUNCNAME
#define FUNCNAME MPID_VCR_Get_lpid
int MPID_VCR_Get_lpid(mpig_vc_t * vcr, int * lpid_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_VCR_GET_LPID);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_VCR_GET_LPID);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_COMM, "entering: vcr=" MPIG_PTR_FMT
	", lpid_p=" MPIG_PTR_FMT, MPIG_PTR_CAST(vcr), MPIG_PTR_CAST(lpid_p)));
    
    mpig_vc_rc_acq(vcr, TRUE);
    {
	*lpid_p = vcr->lpid;
    }
    mpig_vc_rc_rel(vcr, FALSE);
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_COMM, "entering: vcr=" MPIG_PTR_FMT
	", lpid_p=" MPIG_PTR_FMT ", lpid=%d, mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vcr), MPIG_PTR_CAST(lpid_p),
	*lpid_p, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_VCR_GET_LPID);
    return mpi_errno;
}
/* MPID_VCR_Get_lpid() */


typedef struct mpig_pg_table_entry
{
    mpig_pg_t * pg;
    int index;
    mpig_genq_entry_t q_entry;
}
mpig_pg_table_entry_t;

typedef mpig_genq_t mpig_pg_table_t;

#undef FUNCNAME
#define FUNCNAME mpig_pg_table_construct
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
static int mpig_pg_table_construct(mpig_pg_table_t * const pg_table)
{
    mpig_genq_construct(pg_table);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME mpig_pg_table_destroy_q_entry
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
static void mpig_pg_table_destroy_q_entry(mpig_genq_entry_t * const entry)
{
    mpig_pg_table_entry_t * pg_table_entry;

    pg_table_entry = mpig_genq_entry_get_value(entry);
    
#   if defined(MPIG_DEBUG)
    {
        pg_table_entry->pg = MPIG_INVALID_PTR;
        pg_table_entry->index = -1;
    }
#   endif
    
    MPIU_Free(pg_table_entry);
}

#undef FUNCNAME
#define FUNCNAME mpig_pg_table_destruct
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
static int mpig_pg_table_destruct(mpig_pg_table_t * const pg_table)
{
    mpig_genq_destruct(pg_table, mpig_pg_table_destroy_q_entry);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME mpig_pg_table_insert_entry
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
static int mpig_pg_table_insert_entry(mpig_pg_table_t * const pg_table, mpig_pg_table_entry_t * const pg_table_entry)
{
    mpig_genq_enqueue_head_entry(pg_table, &pg_table_entry->q_entry);

    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME mpig_pg_table_compare_pgs
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
static bool_t mpig_pg_table_compare_pgs(const void * entry1, const void * entry2)
{
    const mpig_pg_table_entry_t * pg_table_entry1 = (const mpig_pg_table_entry_t *) entry1;
    const mpig_pg_table_entry_t * pg_table_entry2 = (const mpig_pg_table_entry_t *) entry2;

    if (pg_table_entry1->pg == pg_table_entry2->pg)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

#undef FUNCNAME
#define FUNCNAME mpig_pg_table_find_entry
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
static mpig_pg_table_entry_t * mpig_pg_table_find_entry(mpig_pg_table_t * pg_table, mpig_pg_t * pg)
{
    mpig_genq_entry_t * q_entry;
    mpig_pg_table_entry_t * pg_table_entry;
    
    q_entry = mpig_genq_find_entry(pg_table, pg, mpig_pg_table_compare_pgs);
    if (q_entry)
    {
        pg_table_entry = mpig_genq_entry_get_value(q_entry);
    }
    else
    {
        pg_table_entry = NULL;
    }

    return pg_table_entry;
}

#undef FUNCNAME
#define FUNCNAME mpig_pg_table_extract_entry
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
static mpig_pg_table_entry_t * mpig_pg_table_extract_entry(mpig_pg_table_t * pg_table)
{
    mpig_genq_entry_t * q_entry;
    mpig_pg_table_entry_t * pg_table_entry;

    q_entry = mpig_genq_dequeue_tail_entry(pg_table);
    if (q_entry)
    {
        pg_table_entry = mpig_genq_entry_get_value(q_entry);
    }
    else
    {
        pg_table_entry = NULL;
    }

    return pg_table_entry;
}

#undef FUNCNAME
#define FUNCNAME mpig_pg_table_entry_create
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
static int mpig_pg_table_entry_create(mpig_pg_t * const pg, const int index, mpig_pg_table_entry_t ** const pg_table_entry_p)
{
    mpig_pg_table_entry_t * pg_table_entry;
    int mpi_errno = MPI_SUCCESS;

    pg_table_entry = (mpig_pg_table_entry_t *) MPIU_Malloc(sizeof(mpig_pg_table_entry_t));
    if (pg_table_entry == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
        MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: allocation of a PG table entry failed: pg="
            MPIG_PTR_FMT ", index=%d", MPIG_PTR_CAST(pg), index));
        MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "PG table entry");
        goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    mpig_genq_entry_construct(&pg_table_entry->q_entry);
    mpig_genq_entry_set_value(&pg_table_entry->q_entry, pg_table_entry);
    pg_table_entry->pg = pg;
    pg_table_entry->index = index;
    
    *pg_table_entry_p = pg_table_entry;
    
  fn_return:
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
        goto fn_return;
    }   /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME mpig_pg_table_entry_destroy
#undef FCNAME
#define FCNAME MPIG_QUOTE(FUNCNAME)
static void mpig_pg_table_entry_destroy(mpig_pg_table_entry_t * const pg_table_entry)
{
    mpig_genq_entry_destruct(&pg_table_entry->q_entry);

#   if defined(MPIG_DEBUG)
    {
        pg_table_entry->pg = MPIG_INVALID_PTR;
        pg_table_entry->index = -1;
    }
#   endif
    
    MPIU_Free(pg_table_entry);
}

/*
 * <mpi_errno> mpig_vcrt_serialize_object([IN] vcrt, [OUT] str)
 *
 * this routine extracts enough information about the VCs referenced by the supplied VCRT and serializes that information into an
 * ASCII string.  using this string, an equivalent VCRT, and any necessary VC and PG objects, can be created using
 * mpig_vcrt_deserialize_object().
 *
 * Paramters:
 *
 * vcrt - [IN] virtual connection reference table to be serialized
 *
 * str - [OUT] a string containing the serialized VCRT and associated objects
 *
 */
#undef FUNCNAME
#define FUNCNAME mpig_vcrt_serialize_object
int mpig_vcrt_serialize_object(mpig_vcrt_t * const vcrt, char ** const str_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t vcrt_acquired = FALSE;
    mpig_strspace_t bcs_strspace;
    mpig_strspace_t pgs_strspace;
    mpig_pg_table_t pg_table;
    bool_t pg_table_constructed = FALSE;
    int pg_count;
    mpig_pg_table_entry_t * pg_table_entry;
    char * bc_str = NULL;
    char int_str[MPIG_INT_MAX_STRLEN];
    char * str = NULL;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_vcrt_serialize_object);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_vcrt_serialize_object);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "entering: vcrt=" MPIG_PTR_FMT, MPIG_PTR_CAST(vcrt)));

    mpig_strspace_construct(&bcs_strspace);
    mpig_strspace_construct(&pgs_strspace);

    mpi_errno = mpig_pg_table_construct(&pg_table);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: construction PG table failed vcrt="
	    MPIG_PTR_FMT, MPIG_PTR_CAST(vcrt)));
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    pg_table_constructed = TRUE;
    
    mpi_errno = mpig_strspace_grow(&pgs_strspace, (size_t) MPIG_VCRT_SERIALIZE_PG_ALLOC_SIZE);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: allocation of initial space for serialized PG "
	    "objects failed: vcrt=" MPIG_PTR_FMT, MPIG_PTR_CAST(vcrt)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "initial space for serialized PG objects");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    mpig_vcrt_rc_acq(vcrt, TRUE);
    vcrt_acquired = TRUE;
    {
	int p;
	
	mpi_errno = mpig_strspace_grow(&bcs_strspace, (size_t) vcrt->size * MPIG_VCRT_SERIALIZE_BC_ALLOC_SIZE);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: allocation of initial space for serialized "
		"VC objects: vcrt=" MPIG_PTR_FMT ", vcrt_size=%d, bytes=" MPIG_SIZE_FMT, MPIG_PTR_CAST(vcrt),
		vcrt->size, (size_t) vcrt->size * MPIG_VCRT_SERIALIZE_BC_ALLOC_SIZE));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "initial space for serialized VC objects");
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	rc = MPIU_Snprintf(int_str, MPIG_INT_MAX_STRLEN, "%d", vcrt->size);
	if (rc == MPIG_INT_MAX_STRLEN && int_str[MPIG_INT_MAX_STRLEN - 1] != '\0')
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: conversion of VCRT size to a string "
		"failed: vcrt=" MPIG_PTR_FMT ", vcrt->size=%d, rc=%d", MPIG_PTR_CAST(vcrt), vcrt->size, rc));
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|inttostrconv");
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
		
	mpi_errno = mpig_strspace_add_element(&bcs_strspace, int_str, MPIG_VCRT_SERIALIZE_BC_ALLOC_SIZE);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: addition of the VCRT size failed: "
		"vcrt=" MPIG_PTR_FMT ", vcrt->size=%d", MPIG_PTR_CAST(vcrt), vcrt->size));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
		"string spce for the number of serialized VC objects");
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	    pg_count = 0;
	
	for (p = 0; p < vcrt->size; p++)
	{
	    mpig_vc_t * const vc = vcrt->vcr_table[p];
	    mpig_pg_t * pg = mpig_vc_get_pg(vc);
	    const int pg_rank = mpig_vc_get_pg_rank(vc);
	    int pg_index;

            pg_table_entry = mpig_pg_table_find_entry(&pg_table, pg);
	    if (pg_table_entry != NULL)
	    {
		pg_index = pg_table_entry->index;
	    }
	    else
	    {
		pg_index = pg_count;
                
                mpi_errno = mpig_pg_table_entry_create(pg, pg_index, &pg_table_entry);
                if (mpi_errno)
		{   /* --BEGIN ERROR HANDLING-- */
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: PG table entry creation failed: "
                        "vcrt=" MPIG_PTR_FMT ", pg=" MPIG_PTR_FMT ", index=%d", MPIG_PTR_CAST(vcrt), MPIG_PTR_CAST(pg), pg_index));
		    goto fn_fail;
		}   /* --END ERROR HANDLING-- */

                mpi_errno = mpig_pg_table_insert_entry(&pg_table, pg_table_entry);
		if (mpi_errno)
		{   /* --BEGIN ERROR HANDLING-- */
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: insert into PG table failed: "
                        "vcrt=" MPIG_PTR_FMT ", pg=" MPIG_PTR_FMT ", index=%d", MPIG_PTR_CAST(vcrt), MPIG_PTR_CAST(pg), pg_index));
                    
                    mpig_pg_table_entry_destroy(pg_table_entry);
		    goto fn_fail;
		}   /* --END ERROR HANDLING-- */
		
		pg_count += 1;
	    }
	    
	    mpig_vc_rc_acq(vc, TRUE);
	    {
		mpi_errno = mpig_bc_serialize_object(mpig_vc_get_bc(vc), &bc_str);
	    }
	    mpig_vc_rc_rel(vc, FALSE);

	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: serialization of BC object failed: "
		    "vcrt=" MPIG_PTR_FMT ", p=%d, vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vcrt), p, MPIG_PTR_CAST(vc)));
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "space for serialized BC object");
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */
	    
	    rc = MPIU_Snprintf(int_str, MPIG_INT_MAX_STRLEN, "%d", pg_index);
	    if (rc == MPIG_INT_MAX_STRLEN && int_str[MPIG_INT_MAX_STRLEN - 1] != '\0')
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: conversion of pg_index to a string "
		"failed: vcrt=" MPIG_PTR_FMT ", pg_index=%d, rc=%d", MPIG_PTR_CAST(vcrt), pg_index, rc));
		    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|inttostrconv");
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */
		
	    mpi_errno = mpig_strspace_add_element(&bcs_strspace, int_str, mpig_strspace_get_size(&bcs_strspace) / (p + 1) *
		(vcrt->size - p) + MPIG_VCRT_SERIALIZE_BC_ALLOC_SIZE);
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: addition of serialized VC object "
		    "(pg_index) failed: vcrt=" MPIG_PTR_FMT ", p=%d, vc=" MPIG_PTR_FMT ", pg_index=%d", MPIG_PTR_CAST(vcrt), p,
		    MPIG_PTR_CAST(vc), pg_index));
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "space for serialized VC object (pg_index)");
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */

	    rc = MPIU_Snprintf(int_str, MPIG_INT_MAX_STRLEN, "%d", pg_rank);
	    if (rc == MPIG_INT_MAX_STRLEN && int_str[MPIG_INT_MAX_STRLEN - 1] != '\0')
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: conversion of pg_rank to a string "
		    "failed: vcrt=" MPIG_PTR_FMT ", pg_rank=%d, rc=%d", MPIG_PTR_CAST(vcrt), pg_rank, rc));
		MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|inttostrconv");
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */
		
	    mpi_errno = mpig_strspace_add_element(&bcs_strspace, int_str, mpig_strspace_get_size(&bcs_strspace) / (p + 1) *
		(vcrt->size - p) + MPIG_VCRT_SERIALIZE_BC_ALLOC_SIZE);
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: addition of serialized VC object "
		    "(pg_rank) failed: vcrt=" MPIG_PTR_FMT ", p=%d, vc=" MPIG_PTR_FMT ", pg_rank=%d", MPIG_PTR_CAST(vcrt), p,
		    MPIG_PTR_CAST(vc), pg_rank));
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "space for serialized VC object (pg_rank)");
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */

	    mpi_errno = mpig_strspace_add_element(&bcs_strspace, bc_str, mpig_strspace_get_size(&bcs_strspace) / (p + 1) *
		(vcrt->size - p) + MPIG_VCRT_SERIALIZE_BC_ALLOC_SIZE);
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: addition of serialized VC object (BC) "
		    "failed: vcrt=" MPIG_PTR_FMT ", p=%d, vc=" MPIG_PTR_FMT ", bc=%s", MPIG_PTR_CAST(vcrt), p,
		    MPIG_PTR_CAST(vc), bc_str));
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "space for serialized VC object (BC)");
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */

	    mpig_bc_free_serialized_object(bc_str);
	    bc_str = NULL;
	}
	/* end for (p = 0; p < vcrt->size; p++) */
    }
    mpig_vcrt_rc_rel(vcrt, FALSE);
    vcrt_acquired = FALSE;

    rc = MPIU_Snprintf(int_str, MPIG_INT_MAX_STRLEN, "%d", pg_count);
    if (rc == MPIG_INT_MAX_STRLEN && int_str[MPIG_INT_MAX_STRLEN - 1] != '\0')
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: conversion of pg_count to a string failed: "
	    "vcrt=" MPIG_PTR_FMT ", pg_count=%d, rc=%d", MPIG_PTR_CAST(vcrt), pg_count, rc));
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|inttostrconv");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    mpi_errno = mpig_strspace_add_element(&pgs_strspace, int_str, MPIG_VCRT_SERIALIZE_PG_ALLOC_SIZE);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: addition of the number of serialized PG "
	    "objects failed: vcrt=" MPIG_PTR_FMT ", pg_count=%d", MPIG_PTR_CAST(vcrt), pg_count));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "space for number of serialized PG objects");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    pg_table_entry = mpig_pg_table_extract_entry(&pg_table);
    while (pg_table_entry != NULL)
    {
	const mpig_pg_t * pg = pg_table_entry->pg;
	const char * const pg_id = mpig_pg_get_id(pg);
	const int pg_size = mpig_pg_get_size(pg);
	const int pg_index = pg_table_entry->index;
	
	mpig_pg_table_entry_destroy(pg_table_entry);
    
	rc = MPIU_Snprintf(int_str, MPIG_INT_MAX_STRLEN, "%d", pg_index);
	if (rc == MPIG_INT_MAX_STRLEN && int_str[MPIG_INT_MAX_STRLEN - 1] != '\0')
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: conversion of pg_index to a string "
		"failed: vcrt=" MPIG_PTR_FMT ", pg_index=%d, rc=%d", MPIG_PTR_CAST(vcrt), pg_index, rc));
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|inttostrconv");
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
		
	mpi_errno = mpig_strspace_add_element(&pgs_strspace, int_str, MPIG_VCRT_SERIALIZE_PG_ALLOC_SIZE);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: addition of serialized PG object "
		"(pg_index) failed: vcrt=" MPIG_PTR_FMT ", pg=" MPIG_PTR_FMT ", pg_id=%s, pg_index=%d", MPIG_PTR_CAST(vcrt),
		MPIG_PTR_CAST(pg), pg_id, pg_index));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "space for serialized PG object (pg_index)");
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
		
	rc = MPIU_Snprintf(int_str, MPIG_INT_MAX_STRLEN, "%d", pg_size);
	if (rc == MPIG_INT_MAX_STRLEN && int_str[MPIG_INT_MAX_STRLEN - 1] != '\0')
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: conversion of pg_size to a string "
		"failed: vcrt=" MPIG_PTR_FMT ", pg_size=%d, rc=%d", MPIG_PTR_CAST(vcrt), pg_size, rc));
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|inttostrconv");
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
		
	mpi_errno = mpig_strspace_add_element(&pgs_strspace, int_str, MPIG_VCRT_SERIALIZE_PG_ALLOC_SIZE);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: addition of serialized PG object "
		"(pg_size) failed: vcrt=" MPIG_PTR_FMT ", pg=" MPIG_PTR_FMT ", pg_id=%s, pg_size=%d",
		MPIG_PTR_CAST(vcrt), MPIG_PTR_CAST(pg), pg_id, pg_size));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "space for serialized PG object (pg_size)");
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
		
	mpi_errno = mpig_strspace_add_element(&pgs_strspace, pg_id, MPIG_VCRT_SERIALIZE_PG_ALLOC_SIZE);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: addition of serialized PG object "
		"(pg_id) failed: vcrt=" MPIG_PTR_FMT ", pg=" MPIG_PTR_FMT ", pg_id=%s,", MPIG_PTR_CAST(vcrt),
		MPIG_PTR_CAST(pg), pg_id));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "space for serialized PG object (pg_id)");
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

        pg_table_entry = mpig_pg_table_extract_entry(&pg_table);
    }
    
    {
	const size_t pgs_eod = mpig_strspace_get_eod(&pgs_strspace);
	const size_t bcs_eod = mpig_strspace_get_eod(&bcs_strspace);

	MPIU_Assertp(pgs_eod > 0 && bcs_eod > 0);
	MPIU_Assertp(pgs_eod == strlen(mpig_strspace_get_base_ptr(&pgs_strspace)));
	MPIU_Assertp(bcs_eod == strlen(mpig_strspace_get_base_ptr(&bcs_strspace)));
	
	str = (char *) MPIU_Malloc(pgs_eod + bcs_eod + 1);
	if (str == NULL)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: allocation of string for serialized VCRT "
		"failed: vcrt=" MPIG_PTR_FMT ", size=" MPIG_SIZE_FMT, MPIG_PTR_CAST(vcrt), pgs_eod + bcs_eod - 1));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "serialized VCRT");
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
	
	strcpy(str, mpig_strspace_get_base_ptr(&pgs_strspace));
	strcpy(str + pgs_eod, mpig_strspace_get_base_ptr(&bcs_strspace));

	*str_p = str;
    }
    
  fn_return:
    mpig_strspace_destruct(&bcs_strspace);
    mpig_strspace_destruct(&pgs_strspace);
    if (pg_table_constructed) mpig_pg_table_destruct(&pg_table);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: vcrt=" MPIG_PTR_FMT ", str=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vcrt), MPIG_PTR_CAST(str), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_vcrt_serialize_object);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	if (bc_str) mpig_bc_free_serialized_object(bc_str);
	if (vcrt_acquired) mpig_vcrt_rc_rel(vcrt, FALSE);
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_vcrt_serialize_object() */


/*
 * <void> mpig_vcrt_free_serialized_object([IN] str)
 *
 * this routine frees a string previously created mpig_vcrt_serialize_object().
 *
 * Paramters:
 *
 * str - [IN] the string containing a serialized VCRT and associated objects that needs to be freed
 */
#undef FUNCNAME
#define FUNCNAME mpig_vcrt_free_serialized_object
void mpig_vcrt_free_serialized_object(char * const str)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_vcrt_free_serialized_object);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_vcrt_free_serialized_object);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "entering: str=" MPIG_PTR_FMT, MPIG_PTR_CAST(str)));
    
    MPIU_Free(str);
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: str=" MPIG_PTR_FMT, MPIG_PTR_CAST(str)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_vcrt_free_serialized_object);
    return;
}
/* mpig_vcrt_free_serialized_object() */


typedef struct mpig_vcrt_deserialize_pg_table_entry
{
    mpig_pg_t * pg;
    bool_t committed;
    bool_t locked;
}
mpig_vcrt_deserialize_pg_table_entry_t;

/*
 * <mpi_errno> mpig_vcrt_deserialize_object([IN] vcrt_str, [OUT] vcrt)
 *
 * this routine takes a string containing a serialized VCRT object and creates a new VCRT object that is equivalent.  any VC
 * and PG objects referred to by the VCRT will also be allocated and constructed if they do not already exist or have not been
 * intialized.
 *
 * Paramters:
 *
 * vcrt_str - [IN] the string containing the serialized VCRT and associated objects
 *
 * vcrt - [OUT] a new virtual connection reference table equivalent to the one described in the supplied string
 */
#undef FUNCNAME
#define FUNCNAME mpig_vcrt_deserialize_object
int mpig_vcrt_deserialize_object(char * vcrt_str, mpig_vcrt_t ** const vcrt_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_strspace_t space;
    char * elem_str = NULL;
    int pg_count = 0;
    mpig_vcrt_deserialize_pg_table_entry_t * pg_table = NULL;
    int vcrt_size = 0;
    mpig_vcrt_t * vcrt = NULL;
    bool_t vcrt_acquired = FALSE;
    mpig_vc_t * vc = NULL;
    bool_t vc_locked = FALSE;
    bool_t vc_was_inuse;
    int p;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_vcrt_deserialize_object);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_vcrt_deserialize_object);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "entering: vcrt_str=" MPIG_PTR_FMT,
	MPIG_PTR_CAST(vcrt_str)));
    
    /* place the serialized VCRT into a string space to ease extraction of information */
    mpig_strspace_construct(&space);
    mpi_errno = mpig_strspace_import_string(&space, vcrt_str);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    /* get the number of process groups involved */
    mpi_errno = mpig_strspace_extract_next_element(&space, MPIG_VCRT_DESERIALIZE_ALLOC_SIZE, &elem_str);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: extraction of the process group count failed: "
	    "vcrt_str=" MPIG_PTR_FMT ", space=" MPIG_PTR_FMT, MPIG_PTR_CAST(vcrt_str), MPIG_PTR_CAST(&space)));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|strspace_extract_element",
	    "**globus|strspace_extract_element %s %p", "pg_count", &space);
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    rc = sscanf(elem_str, "%d", &pg_count);
    if (rc != 1)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: conversion of the process group size string to "
	    "an integer failed: vcrt_str=" MPIG_PTR_FMT ", elem_str=%s", MPIG_PTR_CAST(vcrt_str), elem_str));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|strtointconv", "**globus|strtointconv %s %s", "pg_count", elem_str);
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    mpig_strspace_free_extracted_element(elem_str);
    elem_str = NULL;

    /* create and initialized the table to hold all of the process groups */
    pg_table = MPIU_Malloc(pg_count * sizeof(mpig_vcrt_deserialize_pg_table_entry_t));
    if (pg_table == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: allocation of the process group table failed: "
	"failed: vcrt_str=" MPIG_PTR_FMT ", pg_count=%d", MPIG_PTR_CAST(vcrt_str), pg_count));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "process group table");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    for (p = 0; p < pg_count; p++)
    {
	pg_table[p].pg = NULL;
	pg_table[p].committed = FALSE;
	pg_table[p].locked = FALSE;
    }

    /* extract infromation about each of the process groups and acquire a locked reference to them */ 
    for (p = 0; p < pg_count; p++)
    {
	int pg_index;
	int pg_size;

	/* get the index of the next serialized process group */
	mpi_errno = mpig_strspace_extract_next_element(&space, MPIG_VCRT_DESERIALIZE_ALLOC_SIZE, &elem_str);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: extraction of the process group index "
		"failed: vcrt_str=" MPIG_PTR_FMT ", space=" MPIG_PTR_FMT, MPIG_PTR_CAST(vcrt_str), MPIG_PTR_CAST(&space)));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|strspace_extract_element",
		"**globus|strspace_extract_element %s %p", "pg_index", &space);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	rc = sscanf(elem_str, "%d", &pg_index);
	if (rc != 1)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: converstion of the process group index "
		"string to an integer failed: vcrt_str=" MPIG_PTR_FMT ", elem_str=%s", MPIG_PTR_CAST(vcrt_str), elem_str));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|strtointconv", "**globus|strtointconv %s %s", "pg_index", elem_str);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
	mpig_strspace_free_extracted_element(elem_str);
	elem_str = NULL;
	
	/* get the size of the next serialized process group */
	mpi_errno = mpig_strspace_extract_next_element(&space, MPIG_VCRT_DESERIALIZE_ALLOC_SIZE, &elem_str);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: extraction of the process group size "
		"failed: vcrt_str=" MPIG_PTR_FMT ", space=" MPIG_PTR_FMT, MPIG_PTR_CAST(vcrt_str), MPIG_PTR_CAST(&space)));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|strspace_extract_element",
		"**globus|strspace_extract_element %s %p", "pg_size", &space);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	rc = sscanf(elem_str, "%d", &pg_size);
	if (rc != 1)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: converstion of the process group size "
		"string to an integer failed: vcrt_str=" MPIG_PTR_FMT ", elem_str=%s", MPIG_PTR_CAST(vcrt_str), elem_str));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|strtointconv", "**globus|strtointconv %s %s", "pg_size", elem_str);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	mpig_strspace_free_extracted_element(elem_str);
	elem_str = NULL;

	/* get the id string of the next serialized process group */
    	mpi_errno = mpig_strspace_extract_next_element(&space, MPIG_VCRT_DESERIALIZE_ALLOC_SIZE, &elem_str);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: extraction of the process group id string "
		"failed: vcrt_str=" MPIG_PTR_FMT ", space=" MPIG_PTR_FMT, MPIG_PTR_CAST(vcrt_str), MPIG_PTR_CAST(&space)));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|strspace_extract_element",
		"**globus|strspace_extract_element %s %p", "pg_id", &space);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	/*
	 * acquire/create the process group
	 * NOTE: if the process group does not already exit, it will be created.  in such a case, this routine is responsible for
         * committing the process group once any needed VCs within the process group have been initialized and the mutex on the
         * process group has been released.
	 */
	mpi_errno = mpig_pg_acquire_ref(elem_str, pg_size, TRUE, &pg_table[pg_index].pg, &pg_table[pg_index].committed);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: acquisition/creation of the process group "
		"failed: vcrt_str=" MPIG_PTR_FMT ", pg_id=%s, pg_size=%d", MPIG_PTR_CAST(vcrt_str), elem_str, pg_size));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|pg_acquire_ref",
		"**globus|pg_acquire_ref %s %d", elem_str, pg_size);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	pg_table[pg_index].locked = TRUE;

	/* free the process group id string extracted from the string space */
	mpig_strspace_free_extracted_element(elem_str);
	elem_str = NULL;
    }
    /* end for (p = 0; p < pg_count; p++) -- extract process groups */

    /* get the number of processes (virtual connections) involved */
    mpi_errno = mpig_strspace_extract_next_element(&space, MPIG_VCRT_DESERIALIZE_ALLOC_SIZE, &elem_str);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: extraction of the VCRT size failed: vcrt_str="
	    MPIG_PTR_FMT ", space=" MPIG_PTR_FMT, MPIG_PTR_CAST(vcrt_str), MPIG_PTR_CAST(&space)));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|strspace_extract_element",
	    "**globus|strspace_extract_element %s %p", "vcrt_size", &space);
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    rc = sscanf(elem_str, "%d", &vcrt_size);
    if (rc != 1)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: conversion of the VCRT size string to an "
	    "integer failed: vcrt_str=" MPIG_PTR_FMT "elem_str=%s", MPIG_PTR_CAST(vcrt_str), elem_str));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|strtointconv", "**globus|strtointconv %s %s", "pg_count", elem_str);
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    mpig_strspace_free_extracted_element(elem_str);
    elem_str = NULL;
    
    /* create a new VCRT object */
    mpi_errno = MPID_VCRT_Create(vcrt_size, &vcrt);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: creation of the VCRT failed: vcrt_str="
	    MPIG_PTR_FMT ", vcrt_size=%d", MPIG_PTR_CAST(vcrt_str), vcrt_size));
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**dev|vcrt_create");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    mpig_vcrt_rc_acq(vcrt, FALSE);
    vcrt_acquired = TRUE;
    {
	for (p = 0; p < vcrt_size; p++)
	{
	    vcrt->vcr_table[p] = NULL;
	}

	/* extract information about the processes from the string and populate the VCRT */
	for (p = 0; p < vcrt_size; p++)
	{
	    int pg_index;
	    int pg_rank;
	    const char * pg_id;
	    
	    /* get the index of the process group to which the process belongs */
	    mpi_errno = mpig_strspace_extract_next_element(&space, MPIG_VCRT_DESERIALIZE_ALLOC_SIZE, &elem_str);
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: extraction of the process group index "
		    "failed: vcrt_str=" MPIG_PTR_FMT ", space=" MPIG_PTR_FMT "p=%d", MPIG_PTR_CAST(vcrt_str),
		    MPIG_PTR_CAST(&space), p));
		MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|strspace_extract_element",
		    "**globus|strspace_extract_element %s %p", "pg_index", &space);
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */

	    rc = sscanf(elem_str, "%d", &pg_index);
	    if (rc != 1)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: conversion of the process group index "
		    "string to an integer failed: vcrt_str=" MPIG_PTR_FMT "p=%d, elem_str=%s", MPIG_PTR_CAST(vcrt_str), p,
		    elem_str));
		MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|strtointconv", "**globus|strtointconv %s %s",
		    "pg_index", elem_str);
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */

	    mpig_strspace_free_extracted_element(elem_str);
	    elem_str = NULL;
	    
	    pg_id = mpig_pg_get_id(pg_table[pg_index].pg);
	    
	    /* get the rank of the process within the process group */
	    mpi_errno = mpig_strspace_extract_next_element(&space, MPIG_VCRT_DESERIALIZE_ALLOC_SIZE, &elem_str);
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: extraction of the process group rank "
		    "failed: vcrt_str=" MPIG_PTR_FMT ", space=" MPIG_PTR_FMT "p=%d", MPIG_PTR_CAST(vcrt_str),
		    MPIG_PTR_CAST(&space), p));
		MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|strspace_extract_element",
		    "**globus|strspace_extract_element %s %p", "pg_rank", &space);
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */

	    rc = sscanf(elem_str, "%d", &pg_rank);
	    if (rc != 1)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: conversion of the process group rank "
		    "string to an integer failed: vcrt_str=" MPIG_PTR_FMT "p=%d, elem_str=%s", MPIG_PTR_CAST(vcrt_str), p,
		    elem_str));
		MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|strtointconv", "**globus|strtointconv %s %s",
		    "pg_index", elem_str);
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */

	    mpig_strspace_free_extracted_element(elem_str);
	    elem_str = NULL;
	    
	    /* get the business card describing the process */
	    mpi_errno = mpig_strspace_extract_next_element(&space, MPIG_VCRT_DESERIALIZE_ALLOC_SIZE, &elem_str);
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: extraction of the business card "
		    "failed: vcrt_str=" MPIG_PTR_FMT ", space=" MPIG_PTR_FMT "proc=%d", MPIG_PTR_CAST(vcrt_str),
		    MPIG_PTR_CAST(&space), p));
		MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|strspace_extract_element",
		    "**globus|strspace_extract_element %s %p", "bc", &space);
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */

	    /* using the PG index and rank, get a pointer to the VC object.  acquire the VC mutex. */
	    mpig_pg_get_vc(pg_table[pg_index].pg, pg_rank, &vc);
	    mpig_vc_mutex_lock(vc);
	    vc_locked = TRUE;
	    {
		if (mpig_vc_is_initialized(vc) == FALSE)
		{
		    /* deserialize the business card, placing its contents in the BC object attached to the VC */
		    mpi_errno = mpig_bc_deserialize_object(elem_str, mpig_vc_get_bc(vc));
		    if (mpi_errno)
		    {   /* --BEGIN ERROR HANDLING-- */
			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: business card deserialization "
			    "failed: vcrt_str=" MPIG_PTR_FMT ", vc=" MPIG_PTR_FMT "pg_id=%s, pg_rank=%d", MPIG_PTR_CAST(vcrt_str),
			    MPIG_PTR_CAST(vc), pg_id, pg_rank));
			MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|bc_deserialize");
			goto fn_fail;
		    }   /* --END ERROR HANDLING-- */

		    /* free the string containing the serialized business card */
		    mpig_strspace_free_extracted_element(elem_str);
		    elem_str = NULL;

		    /* construct VC contact information using infromation in the business card */
		    mpi_errno = mpig_vc_construct_contact_info(vc);
		    if (mpi_errno)
		    {   /* --BEGIN ERROR HANDLING-- */
			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: construction of VC contact "
			    "information failed: vcrt_str=" MPIG_PTR_FMT ", vc=" MPIG_PTR_FMT "pg_id=%s, pg_rank=%d",
			    MPIG_PTR_CAST(vcrt_str), MPIG_PTR_CAST(vc), pg_id, pg_rank));
			MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|bc_deserialize");
			goto fn_fail;
		    }   /* --END ERROR HANDLING-- */

		    /* select the communication method that will be used for each VC */
		    mpi_errno = mpig_vc_select_comm_method(vc);
		    if (mpi_errno)
		    {   /* --BEGIN ERROR HANDLING-- */
			MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: selection of a "
			    "communication method failed: vcrt_str=" MPIG_PTR_FMT ", vc=" MPIG_PTR_FMT "pg_id=%s, pg_rank=%d",
			    MPIG_PTR_CAST(vcrt_str), MPIG_PTR_CAST(vc), pg_id, pg_rank));
			MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|vc_select_comm_method",
			    "**globus|vc_select_comm_method %s %d", pg_id, pg_rank);
			goto fn_fail;
		    }   /* --END ERROR HANDLING-- */

		    mpig_vc_set_initialized(vc, TRUE);
		}
		/* if (mpig_vc_get_is_initialized(vc) == FALSE) */
		
		/* increment the reference count to reflect the addition of the VC to the VCRT (done below outside of the lock) */
		mpig_vc_inc_ref_count(vc, &vc_was_inuse);
		if (vc_was_inuse == FALSE)
		{
		    mpig_pg_inc_ref_count(pg_table[pg_index].pg);
		}
	    }
	    mpig_vc_mutex_unlock(vc);
	    vc_locked = FALSE;
	    
	    vcrt->vcr_table[p] = vc;
	    vc = NULL;
	}
	/* end for (p = 0; p < vcrt_size; p++) */

	/* vcrt->ref_count += 1; -- the VCRT reference count is set to one (1) when it is created */
    }
    mpig_vcrt_rc_rel(vcrt, TRUE);
    vcrt_acquired = FALSE;

    *vcrt_p = vcrt;
    
  fn_return:
    for (p = 0; p < pg_count; p++)
    {
	if (pg_table[p].pg != NULL)
	{
	    mpig_pg_mutex_unlock_conditional(pg_table[p].pg, pg_table[p].locked);
	    mpig_pg_release_ref(pg_table[p].pg);
	    
	    if (pg_table[p].committed == FALSE)
	    {
		mpig_pg_commit(pg_table[p].pg);
	    }
	}
    }
    
    mpig_strspace_destruct(&space);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: vcrt_str=" MPIG_PTR_FMT ", vcrt=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vcrt_str), MPIG_PTR_CAST(vcrt), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_vcrt_deserialize_object);
    return mpi_errno;

    {   /* --BEGIN ERROR HANDLING-- */
      fn_fail:
	if (elem_str != NULL) mpig_strspace_free_extracted_element(elem_str);
	mpig_vc_mutex_unlock_conditional(vc, (vc_locked));
	if (vcrt_acquired) mpig_vcrt_rc_rel(vcrt, FALSE);

	/* mpig_vc_release_ref() may attempt to lock the mutex of the PG to which it belongs.  therefore, the mutex of each PG in
	   use must be released. */
	for (p = 0; p < pg_count; p++)
	{
	    if (pg_table[p].pg != NULL)
	    {
		mpig_pg_mutex_unlock_conditional(pg_table[p].pg, pg_table[p].locked);
		pg_table[p].locked = FALSE;
	    }
	}
	
	for (p = 0; p < vcrt->size; p++)
	{
	    if (vcrt->vcr_table[p] != NULL) mpig_vc_release_ref(vcrt->vcr_table[p]);
	}
	
	vcrt->ref_count = 0;
	vcrt->size = 0;
	mpig_vcrt_mutex_destruct(vcrt);
	MPIU_Free(vcrt);
	
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_vcrt_deserialize_object() */


/*
 * <mpi_errno> mpig_vc_construct_contact_info([IN/MOD] vc)
 *
 * this routine initializes the contact information (ci) structure contained with the VC
 *
 * Paramters:
 *
 *   vc - [IN/MOD] virtual connection object containing the contact information structure to be initialized
 *
 * NOTE: this routines assumes that the business card object within the VC already cotains any necessary contact information that
 * might need to be extracted by this routine or one of the communicaiton method extraction routines.
 */
#undef FUNCNAME
#define FUNCNAME mpig_vc_construct_contact_info
int mpig_vc_construct_contact_info(mpig_vc_t * vc)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_bc_t * const bc = mpig_vc_get_bc(vc);
    char * lan_id = NULL;
    bool_t found;
    int cm_n = -1;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_vc_construct_contact_info);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_vc_construct_contact_info);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VC, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));

    /* get the app num from the business card and store it in the VC */
    mpi_errno = mpig_pm_get_app_num(bc, &vc->cms.app_num);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VC, "ERROR: extraction of the APP_NUM from "
	    "the business card failed: vc=" MPIG_PTR_FMT "pg_id=%s, pg_rank=%d", MPIG_PTR_CAST(vc), mpig_vc_get_pg_id(vc),
	    mpig_vc_get_pg_rank(vc)));
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|pm_get_app_num");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

    /* get the LAN identification string from the business card and store it in the VC */
    mpi_errno = mpig_bc_get_contact(bc, "MPIG_LAN_ID", &lan_id, &found);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VC, "ERROR: extraction of the LAN ID from "
	    "the business card failed: vc=" MPIG_PTR_FMT "pg_id=%s, pg_rank=%d", MPIG_PTR_CAST(vc), mpig_vc_get_pg_id(vc),
	    mpig_vc_get_pg_rank(vc)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact", "**globus|bc_get_contact %s",
	    "MPIG_LAN_ID");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    if (found)
    {
	vc->cms.lan_id = MPIU_Strdup(lan_id);
	mpig_bc_free_contact(lan_id);
	if (vc->cms.lan_id == NULL)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VC, "ERROR: duplication of the LAN ID "
		"failed: vc=" MPIG_PTR_FMT "pg_id=%s, pg_rank=%d", MPIG_PTR_CAST(vc), mpig_vc_get_pg_id(vc),
		mpig_vc_get_pg_rank(vc)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "LAN ID");
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
    }
    
    /* extract the contact information from the business card attached to the VC object */
    for (cm_n = 0; cm_n < mpig_cm_table_num_entries; cm_n++)
    {
	mpig_cm_t * cm = mpig_cm_table[cm_n];
	
	if (mpig_cm_get_vtable(cm)->construct_vc_contact_info != NULL)
	{
	    mpi_errno = mpig_cm_get_vtable(cm)->construct_vc_contact_info(cm, vc);
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VC, "ERROR: construction of contact "
		    "information failed: vc=" MPIG_PTR_FMT "pg_id=%s, pg_rank=%d, cm=%s", MPIG_PTR_CAST(vc),
		    mpig_vc_get_pg_id(vc), mpig_vc_get_pg_rank(vc), mpig_cm_get_name(cm)));
		MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm|construct_vc_contact_info",
		    "**globus|cm|construct_vc_contact_info %s %s %d", mpig_cm_get_name(cm), mpig_vc_get_pg_id(vc),
		    mpig_vc_get_pg_rank(vc));
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */
	}
    }

    vc->cms.ci_initialized = TRUE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VC, "exiting: vc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	MPIG_PTR_CAST(vc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_vc_construct_contact_info);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	while(cm_n >= 0)
	{
	    mpig_cm_t * cm = mpig_cm_table[cm_n];
	    
	    if (mpig_cm_get_vtable(cm)->destruct_vc_contact_info != NULL)
	    {
		mpig_cm_get_vtable(cm)->destruct_vc_contact_info(cm, vc);
	    }
	    cm_n -= 1;
	}
	
	vc->cms.app_num = -1;
	
    	if (vc->cms.lan_id) MPIU_Free(vc->cms.lan_id);
	vc->cms.lan_id = NULL;
	
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_vc_construct_contact_info() */


/*
 * <void> mpig_vc_destruct_contact_info([IN/MOD] vc)
 *
 * this releases resources used by the contact information (ci) structure contained with the VC
 *
 * Paramters:
 *
 *   vc - [IN/MOD] virtual connection object containing the contact information structure to be initialized
 *
 * NOTE: this routines assumes that the business card object within the VC already cotains any necessary contact information that
 * might need to be extracted by this routine or one of the communicaiton method extraction routines.
 */
#undef FUNCNAME
#define FUNCNAME mpig_vc_destruct_contact_info
void mpig_vc_destruct_contact_info(mpig_vc_t * vc)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int cm_n;
    MPIG_STATE_DECL(MPID_STATE_mpig_vc_destruct_contact_info);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_vc_destruct_contact_info);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VC, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));

    if (vc->cms.ci_initialized == FALSE) goto fn_return;
    
    /* give the commmunication methods an opportunity to destroy any contact information they might have stored in the VC */
    for (cm_n = 0; cm_n < mpig_cm_table_num_entries; cm_n++)
    {
	mpig_cm_t * cm = mpig_cm_table[cm_n];
	
	if (mpig_cm_get_vtable(cm)->destruct_vc_contact_info != NULL)
	{
	    mpig_cm_get_vtable(cm)->destruct_vc_contact_info(cm, vc);
	}
    }

    vc->cms.app_num = -1;

    if (vc->cms.lan_id) MPIU_Free(vc->cms.lan_id);
    vc->cms.lan_id = NULL;
	
    vc->cms.ci_initialized = FALSE;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VC, "exiting: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_vc_destruct_contact_info);
    return;
}
/* mpig_vc_destruct_contact_info() */


/*
 * <mpi_errno> mpig_vc_select_comm_method([IN/MOD] vc)
 *
 * this routine selects the communication method to use when communicating with the process represented by the VC
 *
 * Paramters:
 *
 *   vc - [IN/MOD] virtual connection object representing the process on which method selection is to occur
 *
 * NOTE: this routines assumes that the business card object within the VC already cotains any necessary contact information that
 * might need to be extracted by one of the communicaiton method selection routines.
 */
#undef FUNCNAME
#define FUNCNAME mpig_vc_select_comm_method
int mpig_vc_select_comm_method(mpig_vc_t * vc)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int cm_n = 0;
    bool_t selected = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_vc_select_comm_method);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_vc_select_comm_method);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VC, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));

    while (selected == FALSE && cm_n < mpig_cm_table_num_entries)
    {
	mpig_cm_t * cm = mpig_cm_table[cm_n];
	
	if (mpig_cm_get_vtable(cm)->select_comm_method != NULL)
	{
	    mpi_errno = mpig_cm_get_vtable(cm)->select_comm_method(cm, vc, &selected);
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VC, "ERROR: selection of communication method "
		    "failed: vc=" MPIG_PTR_FMT "pg_id=%s, pg_rank=%d, cm=%s", MPIG_PTR_CAST(vc), mpig_vc_get_pg_id(vc),
		    mpig_vc_get_pg_rank(vc), mpig_cm_get_name(cm)));
		MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|cm|select_comm_method",
		    "**globus|cm|select_comm_method %s %s %d", mpig_cm_get_name(cm), mpig_vc_get_pg_id(vc),
		    mpig_vc_get_pg_rank(vc));
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */
	}
	
	cm_n += 1;
    }

    if (!selected)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_VC, "ERROR: no communication method accepted the VC: vc="
	    MPIG_PTR_FMT "pg_id=%s, pg_rank=%d", MPIG_PTR_CAST(vc), mpig_vc_get_pg_id(vc), mpig_vc_get_pg_rank(vc)));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm|select_comm_method_none",
	    "**globus|cm|select_comm_method_none %s %d", mpig_vc_get_pg_id(vc), mpig_vc_get_pg_rank(vc));
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_VC, "exiting: vc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	MPIG_PTR_CAST(vc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_vc_select_comm_method);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_vc_select_comm_method() */


/*
 * <void> mpig_vc_release_ref([IN/MOD] vc)
 *
 * Paramters:
 *
 * vc - [IN/MOD] virtual connection object
 *
 * MT-NOTE: this routine locks the VC mutex, and may lock the mutex of the associated PG object as well.  therefore, no other
 * routines on call stack of the current context may be not be holding those mutexes when this routine is called.
 */
#undef FUNCNAME
#define FUNCNAME mpig_vc_release_ref
void mpig_vc_release_ref(mpig_vc_t * const vc)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t vc_inuse;
    mpig_pg_t * pg = NULL;
    MPIG_STATE_DECL(MPID_STATE_mpig_vc_release_ref);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_vc_release_ref);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_VC, "entering: vc=" MPIG_PTR_FMT,
	MPIG_PTR_CAST(vc)));
    
    mpig_vc_mutex_lock(vc);
    {
	pg = mpig_vc_get_pg(vc);
	mpig_vc_dec_ref_count(vc, &vc_inuse);
    }
    mpig_vc_mutex_unlock(vc);

    /*
     * note: a communication module may decide that a VC is still in use even if the VC's reference count has reached zero.  for
     * example: this can occur when a shutdown protocol is required to cleanly terminate the underlying connection, and the
     * presence of the VC object is required until that protocol completes.  in cases such as this, it is the responsibility of
     * the CM to release the PG once the reference count once the VC is no longer needed.
     */
    if (vc_inuse == FALSE) mpig_pg_release_ref(pg);
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_VC, "exiting: vc=" MPIG_PTR_FMT
	", vc_inuse=%s", MPIG_PTR_CAST(vc), MPIG_BOOL_STR(vc_inuse)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_vc_release_ref);
    return;
}
/* mpig_vc_release_ref() */


/*
 * <double> mpig_vc_vtable_last_entry(...)
 *
 * this routine serves as the last function in the VC function table.  its purpose is to help detect when a communication
 * module's VC table has not be updated when a function be added or removed from the table.  it is not fool proof as it requires
 * the type signature not to match, but there should be few (if any) routines in the VC table that have no arguments and return
 * no values.
 */
#undef FUNCNAME
#define FUNCNAME mpig_vc_vtable_last_entry
double mpig_vc_vtable_last_entry(float foo, int bar, const short * baz, char bif)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_vc_vtable_last_entry);

    MPIG_UNUSED_ARG(foo);
    MPIG_UNUSED_ARG(bar);
    MPIG_UNUSED_ARG(baz);
    MPIG_UNUSED_ARG(bif);
    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_vc_vtable_last_entry);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR, "FATAL ERROR: mpig_vc_vtable_last_entry called.  aborting program"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_vc_vtable_last_entry);
    MPID_Abort(NULL, MPI_SUCCESS, 13, "FATAL ERROR: mpig_vc_vtable_last_entry called.  Aborting Program.");
    return 0.0;
}
