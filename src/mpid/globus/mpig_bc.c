/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

#define MPIG_BC_STR_GROWTH_RATE 1024
#define MPIG_BC_VAL_GROWTH_RATE 128

/*
 * <mpi_errno> mpig_bc_copy([IN] orig_bc, [IN/MOD] copy_bc)
 *
 * Paramters:
 *
 * orig_bc [IN] - original business card object
 *
 * copy_bc [IN/MOD] - (constructed) business card object in which to place the duplicate
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_copy
int mpig_bc_copy(const mpig_bc_t * const orig_bc, mpig_bc_t * const copy_bc)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    char * str;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_bc_copy);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_bc_copy);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC, "entering: orig_bc=" MPIG_PTR_FMT ", copy_bc=" MPIG_PTR_FMT,
	MPIG_PTR_CAST(orig_bc), MPIG_PTR_CAST(copy_bc)));
    
    str = MPIU_Strdup(orig_bc->str_begin);
    MPIU_ERR_CHKANDJUMP1((str == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "duplicate business card");

    if (copy_bc->str_begin) MPIU_Free(copy_bc->str_begin);
    
    copy_bc->str_begin = str;
    copy_bc->str_size = strlen(copy_bc->str_begin);
    copy_bc->str_end = copy_bc->str_begin + copy_bc->str_size;
    copy_bc->str_left = 0;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC, "exiting: orig_bc=" MPIG_PTR_FMT ", copy_bc=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(orig_bc), MPIG_PTR_CAST(copy_bc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_bc_copy);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* int mpig_bc_copy() */


/*
 * <mpi_errno> mpig_bc_add_contact([IN/MOD] bc, [IN] key)
 * 
 * Paramters:
 *
 * bc [IN/MOD] - pointer to the business card in which to add the contact item
 *
 * key [IN] - key under which to store contact item (e.g., "FAX")
 *
 * value [IN] - value of the contact item
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_add_contact
int mpig_bc_add_contact(mpig_bc_t * const bc, const char * const key, const char * const value)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_bc_add_contact);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_bc_add_contact);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC, "entering: bc=" MPIG_PTR_FMT ", key=\"%s\", value=\"%s\"",
	MPIG_PTR_CAST(bc), key, value));

    do
    {
	char * str_end = bc->str_end;
	int str_left = bc->str_left;

	rc = (str_left > 0) ? MPIU_Str_add_string_arg(&str_end, &str_left, key, value) : MPIU_STR_NOMEM;
	if (rc == MPIU_STR_SUCCESS)
	{
	    bc->str_end = str_end;
	    bc->str_left = str_left;
	}
	else if (rc == MPIU_STR_NOMEM)
	{
	    char * str_begin;

	    str_begin = MPIU_Malloc(bc->str_size + MPIG_BC_STR_GROWTH_RATE);
	    MPIU_ERR_CHKANDJUMP1((str_begin == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "business card contact");

	    if (bc->str_begin != NULL)
	    { 
		MPIU_Strncpy(str_begin, bc->str_begin, (size_t) bc->str_size);
		MPIU_Free(bc->str_begin);
	    }
	    else
	    {
		str_begin[0] = '\0';
	    }
	    bc->str_begin = str_begin;
	    bc->str_end = bc->str_begin + (bc->str_size - bc->str_left);
	    bc->str_size += MPIG_BC_STR_GROWTH_RATE;
	    bc->str_left += MPIG_BC_STR_GROWTH_RATE;
	}
	else
	{
	    MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_INTERN, "**internrc", "internrc %d", rc);
	}
    }
    while (rc != MPIU_STR_SUCCESS);
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC, "exiting: bc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	MPIG_PTR_CAST(bc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_bc_add_contact);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* int mpig_bc_add_contact() */


/*
 * <mpi_errno> mpig_bc_get_contact([IN] bc, [IN] key, [OUT] value, [OUT] flag)
 * 
 * Paramters:
 *
 * bc - [IN] pointer to the business card in which to find the contact item
 *
 * key - [IN] key under which the contact item is stored (e.g., "FAX")
 *
 * value - [OUT] pointer to the memory containing the value of the contact item
 *
 * flag - [OUT] flag indicating if the contact item was found in the business card
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_get_contact
int mpig_bc_get_contact(const mpig_bc_t * const bc, const char * const key, char ** const value, int * const flag)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    char * val_str;
    int val_size;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_bc_get_contact);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_bc_get_contact);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC, "entering: bc=" MPIG_PTR_FMT ", key=\"%s\"",
	MPIG_PTR_CAST(bc), key));
    
    val_size = MPIG_BC_VAL_GROWTH_RATE;
    val_str = (char *) MPIU_Malloc(val_size);
    MPIU_ERR_CHKANDJUMP1((val_str == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "business card contact value");
    
    do
    {
	rc = MPIU_Str_get_string_arg(bc->str_begin, key, val_str, val_size);
	if (rc == MPIU_STR_NOMEM)
	{
	    MPIU_Free(val_str);
	    
	    val_size += MPIG_BC_VAL_GROWTH_RATE;
	    val_str = MPIU_Malloc(val_size);
	    MPIU_ERR_CHKANDJUMP1((val_str == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
		"business card contact value");
	}
	else if (rc == MPIU_STR_FAIL)
	{
	    MPIU_Free(val_str);
	    *flag = FALSE;
	    *value = NULL;
	    goto fn_return;
	}
	else if (rc != MPIU_STR_SUCCESS)
	{
	    MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_INTERN, "**internrc", "internrc %d", rc);
	}
    }
    while (rc != MPIU_STR_SUCCESS);

    *flag = TRUE;
    *value = val_str;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC, "exiting: bc=" MPIG_PTR_FMT ", key=\"%s\", value=\"%s\""
	", valuep=" MPIG_PTR_FMT "flag=%s, mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(bc), key, (*flag) ? *value : "",
	MPIG_PTR_CAST(*value), MPIG_BOOL_STR(*flag), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_bc_get_contact);
    return mpi_errno;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	MPIU_Free(val_str);
	*flag = FALSE;
	*value = NULL;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/*mpig_bc_get_contact() */


/*
 * mpig_bc_free_contact([IN] value)
 *
 * Parameters:
 *
 * value [IN] - pointer to value previous returned by mpig_bc_get_contact()
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_free_contact
void mpig_bc_free_contact(char * const value)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_bc_free_contact);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_bc_free_contact);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC, "entering: valuep=" MPIG_PTR_FMT ", value=\"%s\"",
	MPIG_PTR_CAST(value), value));
    
    MPIU_Free(value);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC, "exiting: valuep=" MPIG_PTR_FMT, MPIG_PTR_CAST(value)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_bc_free_contact);
    return;
}
/* int mpig_bc_free_contact() */


/*
 * <mpi_errno> mpig_bc_serialize_object([IN] bc, [OUT] str)
 *
 * Parameters:
 *
 * bc - [IN] pointer to the business card object to be serialized
 *
 * value [OUT] - pointer to the string containing the serialized business card object
 *
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_serialize_object
int mpig_bc_serialize_object(mpig_bc_t * const bc, char ** const str)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_bc_serialize_object);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_bc_serialize_object);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC, "exiting: bc=" MPIG_PTR_FMT, MPIG_PTR_CAST(bc)));
    
    *str = MPIU_Strdup(bc->str_begin);
    MPIU_ERR_CHKANDJUMP1((*str == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "serialized business card");

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC, "exiting: bc=" MPIG_PTR_FMT ", str=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(bc), MPIG_PTR_CAST(*str), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_bc_serialize_object);
    return mpi_errno;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_bc_serialize_object() */


/*
 * void mpig_bc_free_serialized_object([IN] str)
 *
 * Paramters: str [IN] - pointer to the string previously returned by mpig_bc_serialized_object()
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_free_serialized_object
void mpig_bc_free_serialized_object(char * const str)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_bc_free_serialized_object);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_bc_free_serialized_object);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC, "entering: str=" MPIG_PTR_FMT, MPIG_PTR_CAST(str)));
    
    MPIU_Free(str);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC, "exiting: str=" MPIG_PTR_FMT, MPIG_PTR_CAST(str)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_bc_free_serialized_object);
    return;
}
/* void mpig_bc_free_serialized_object() */


/*
 * <mpi_errno> mpig_bc_deserialize_object([IN] str, [IN/OUT] bc)
 *
 * Parameters:
 *
 * str - [IN] string containing a serialzed business card object
 *
 * bc [IN/OUT] - pointer to an existing (constructed) business card object; any informaton stored in the object will be replaced
 */
#undef FUNCNAME
#define FUNCNAME mpig_bc_deserialize_object
int mpig_bc_deserialize_object(const char * const str, mpig_bc_t * const bc)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int str_len;
    char * new_str;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_bc_deserialize_object);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_bc_deserialize_object);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC, "entering: bc=" MPIG_PTR_FMT, MPIG_PTR_CAST(bc)));

    str_len = strlen(str);
    
    new_str = MPIU_Strdup(str);
    MPIU_ERR_CHKANDJUMP1((new_str == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "business card string");

    if (bc->str_begin) MPIU_Free(bc->str_begin);
    
    /*
     * str_end should point at the terminating NULL, not the last character, so one (1) is not subtracted.  likewise, one (1) is
     * added to str_size.
     */
    bc->str_begin = new_str;
    bc->str_end = bc->str_begin + str_len;
    bc->str_size = str_len + 1;
    bc->str_left = 0;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_BC, "exiting: bc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT,
	MPIG_PTR_CAST(bc), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_bc_deserialize_object);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	bc->str_begin = NULL;
	bc->str_end = NULL;
	bc->str_size = 0;
	bc->str_left = 0;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* int mpig_bc_deserialize_object([IN] str, [OUT] bc) */
