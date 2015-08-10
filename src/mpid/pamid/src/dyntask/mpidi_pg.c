/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpidimpl.h>
#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif

#ifdef DYNAMIC_TASKING

extern int mpidi_dynamic_tasking;
int mpidi_sync_done=0;


#define MAX_JOBID_LEN 1024

/* FIXME: These routines need a description.  What is their purpose?  Who
   calls them and why?  What does each one do?
*/
static MPIDI_PG_t * MPIDI_PG_list = NULL;
static MPIDI_PG_t * MPIDI_PG_iterator_next = NULL;
static MPIDI_PG_Compare_ids_fn_t MPIDI_PG_Compare_ids_fn;
static MPIDI_PG_Destroy_fn_t MPIDI_PG_Destroy_fn;

/* Set verbose to 1 to record changes to the process group structure. */
static int verbose = 0;

/* Key track of the process group corresponding to the MPI_COMM_WORLD
   of this process */
MPIDI_PG_t *pg_world = NULL;

#define MPIDI_MAX_KVS_KEY_LEN      256

extern conn_info *_conn_info_list;

int MPIDI_PG_Init(int *argc_p, char ***argv_p,
		  MPIDI_PG_Compare_ids_fn_t compare_ids_fn,
		  MPIDI_PG_Destroy_fn_t destroy_fn)
{
    int mpi_errno = MPI_SUCCESS;
    char *p;

    MPIDI_PG_Compare_ids_fn = compare_ids_fn;
    MPIDI_PG_Destroy_fn     = destroy_fn;

    /* Check for debugging options.  We use MPICHD_DBG and -mpichd-dbg
       to avoid confusion with the code in src/util/dbg/dbg_printf.c */
    p = getenv( "MPICHD_DBG_PG" );
    if (p && ( strcmp( p, "YES" ) == 0 || strcmp( p, "yes" ) == 0) )
	verbose = 1;
    if (argc_p && argv_p) {
	int argc = *argc_p, i;
	char **argv = *argv_p;
	/* applied patch from Juha Jeronen, req #3920 */
	for (i=1; i<argc && argv[i]; i++) {
	    if (strcmp( "-mpichd-dbg-pg", argv[i] ) == 0) {
		int j;
		verbose = 1;
		for (j=i; j<argc-1; j++) {
		    argv[j] = argv[j+1];
		}
		argv[argc-1] = NULL;
		*argc_p = argc - 1;
		break;
	    }
	}
    }

    return mpi_errno;
}

void *mpidi_finalize_req(void *arg) {
    PMI2_Finalize();
    mpidi_sync_done=1;
}

/*@
   MPIDI_PG_Finalize - Finalize the process groups, including freeing all
   process group structures
  @*/
int MPIDI_PG_Finalize(void)
{
   int mpi_errno = MPI_SUCCESS;
   conn_info              *conn_node;
   int                    my_max_worldid, world_max_worldid;
   int                    wid_bit_array_size=0, wid;
   unsigned char          *wid_bit_array=NULL, *root_wid_barray=NULL;
   MPIDI_PG_t             *pg, *pgNext;
   char                   key[PMI2_MAX_KEYLEN];
   char                   value[PMI2_MAX_VALLEN];
   pthread_t              finalize_req_thread;

   /* Print the state of the process groups */
   if (verbose) {
     MPIU_PG_Printall( stdout );
   }

   /* FIXME - straighten out the use of PMI_Finalize - no use after
      PG_Finalize */
   conn_node     = _conn_info_list;
   my_max_worldid  = -1;

   while(NULL != conn_node) {
     if(conn_node->rem_world_id>my_max_worldid && conn_node->ref_count>0) {
       TRACE_ERR("conn_node->rem_world_id=%d conn_node->ref_count=%d\n", conn_node->rem_world_id, conn_node->ref_count);
       my_max_worldid = conn_node->rem_world_id;
     } else {
       TRACE_ERR("conn_node->rem_world_id=%d conn_node->ref_count=%d\n", conn_node->rem_world_id, conn_node->ref_count);
     }
     conn_node = conn_node->next;
   }
   MPIR_Allreduce_impl( &my_max_worldid, &world_max_worldid, 1, MPI_INT, MPI_MAX,   MPIR_Process.comm_world, &mpi_errno);

   /* create bit array of size = world_max_worldid + 1
    * We add 1 to world_max_worldid because suppose my world
    * is only connected to world_id 0 then world_max_worldid=0
    * and if we do not add 1, then size of bite array will be 0.
    * Also suppose in my world world_max_worldid is 8. Then if we
    * dont add 1, then the bit array will be size 1 byte, and when
    * we try to set bit in position 8, we will get segfault.
    */
   TRACE_ERR("my_max_worldid=%d world_max_worldid=%d\n", my_max_worldid, world_max_worldid);
   if(world_max_worldid != -1) {
     world_max_worldid++;
     wid_bit_array_size = (world_max_worldid + CHAR_BIT -1) / CHAR_BIT;
     wid_bit_array = MPIU_Malloc(wid_bit_array_size*sizeof(unsigned char));
     memset(wid_bit_array, 0, wid_bit_array_size*sizeof(unsigned char));
     root_wid_barray = MPIU_Malloc(wid_bit_array_size*sizeof(unsigned char));

     memset(root_wid_barray, 0, wid_bit_array_size*sizeof(unsigned char));
     conn_node     = _conn_info_list;
     while(NULL != conn_node) {
       if(conn_node->ref_count >0) {
	 wid = conn_node->rem_world_id;
	 wid_bit_array[wid/CHAR_BIT] |= 1 << (wid%CHAR_BIT);
	 TRACE_ERR("wid=%d wid_bit_array[%d]=%x\n", wid, wid/CHAR_BIT, 1 << (wid%CHAR_BIT));
       }
       conn_node = conn_node->next;

     }
     /* Let root of my world know about this bit array */
     MPIR_Reduce_impl(wid_bit_array,root_wid_barray,wid_bit_array_size,
		   MPI_UNSIGNED_CHAR,MPI_BOR,0,MPIR_Process.comm_world,&mpi_errno);

     MPIU_Free(wid_bit_array);
   }

   if(MPIR_Process.comm_world->rank == 0) {

     MPL_snprintf(key, PMI2_MAX_KEYLEN-1, "%s", "ROOTWIDARRAY");
     MPL_snprintf(value, PMI2_MAX_VALLEN-1, "%s", root_wid_barray);
     TRACE_ERR("root_wid_barray=%s\n", value);
     mpi_errno = PMI2_KVS_Put(key, value);
     TRACE_ERR("PMI2_KVS_Put returned with mpi_errno=%d\n", mpi_errno);

     MPL_snprintf(key, PMI2_MAX_KEYLEN-1, "%s", "WIDBITARRAYSZ");
     MPL_snprintf(value, PMI2_MAX_VALLEN-1, "%x", wid_bit_array_size);
     key[strlen(key)+1]='\0';
     value[strlen(value)+1]='\0';
     mpi_errno = PMI2_KVS_Put(key, value);
     TRACE_ERR("PMI2_KVS_Put returned with mpi_errno=%d\n", mpi_errno);

   }
   mpi_errno = PMI2_KVS_Fence();
   TRACE_ERR("PMI2_KVS_Fence returned with mpi_errno=%d\n", mpi_errno);

   MPIU_Free(root_wid_barray); /* root_wid_barray is now NULL for non-root */

#if 0
   pthread_create(&finalize_req_thread, NULL, mpidi_finalize_req, NULL);
   /*MPIU_THREAD_CS_EXIT(ALLFUNC,); */
   while (mpidi_sync_done !=1) {
     mpi_errno=PAMI_Context_advance(MPIDI_Context[0], 1000);
     if (mpi_errno == PAMI_EAGAIN) {
       usleep(1);
     }
   }

   if (mpi_errno = pthread_join(finalize_req_thread, NULL) ) {
         TRACE_ERR("error returned from pthread_join() mpi_errno=%d\n",mpi_errno);
   }
#endif
   MPIU_THREAD_CS_EXIT(ALLFUNC,);
   PMI2_Finalize();
   MPIU_THREAD_CS_ENTER(ALLFUNC,);

   if(_conn_info_list) {
     if(_conn_info_list->rem_taskids)
       MPIU_Free(_conn_info_list->rem_taskids);
     else
       MPIU_Free(_conn_info_list);
   }
   /* Free the storage associated with the process groups */
   pg = MPIDI_PG_list;
   while (pg) {
     pgNext = pg->next;

     /* In finalize, we free all process group information, even if
        the ref count is not zero.  This can happen if the user
        fails to use MPI_Comm_disconnect on communicators that
        were created with the dynamic process routines.*/
	/* XXX DJG FIXME-MT should we be checking this? */
     if (MPIU_Object_get_ref(pg) == 0 ) {
       if (pg == MPIDI_Process.my_pg)
         MPIDI_Process.my_pg = NULL;
       MPIU_Object_set_ref(pg, 0); /* satisfy assertions in PG_Destroy */
       MPIDI_PG_Destroy( pg );
     }
     pg     = pgNext;
   }

   /* If COMM_WORLD is still around (it normally should be),
      try to free it here.  The reason that we need to free it at this
      point is that comm_world (and comm_self) still exist, and
      hence the usual process to free the related VC structures will
      not be invoked. */

   /* The process group associated with MPI_COMM_WORLD will be
      freed when MPI_COMM_WORLD is freed */

   return mpi_errno;
}


/* This routine creates a new process group description and appends it to
   the list of the known process groups.  The pg_id is saved, not copied.
   The PG_Destroy routine that was set with MPIDI_PG_Init is responsible for
   freeing any storage associated with the pg_id.

   The new process group is returned in pg_ptr
*/
int MPIDI_PG_Create(int vct_sz, void * pg_id, MPIDI_PG_t ** pg_ptr)
{
    MPIDI_PG_t * pg = NULL, *pgnext;
    int p, i, j;
    int mpi_errno = MPI_SUCCESS;
    char *cp, *world_tasks, *cp1;

    pg = MPIU_Malloc(sizeof(MPIDI_PG_t));
    pg->vct = MPIU_Malloc(sizeof(struct MPID_VCR_t)*vct_sz);

    pg->handle = 0;
    /* The reference count indicates the number of vc's that are or
       have been in use and not disconnected. It starts at zero,
       except for MPI_COMM_WORLD. */
    MPIU_Object_set_ref(pg, 0);
    pg->size = vct_sz;
    pg->id   = MPIU_Strdup(pg_id);
    TRACE_ERR("PG_Create - pg=%x pg->id=%s pg->vct=%x\n", pg, pg->id, pg->vct);
    /* Initialize the connection information to null.  Use
       the appropriate MPIDI_PG_InitConnXXX routine to set up these
       fields */

    pg->connData           = 0;
    pg->getConnInfo        = 0;
    pg->connInfoToString   = 0;
    pg->connInfoFromString = 0;
    pg->freeConnInfo       = 0;

    for (p = 0; p < vct_sz; p++)
    {
	/* Initialize device fields in the VC object */
	MPIDI_VC_Init(&pg->vct[p], pg,p);
    }

    /* The first process group is always the world group */
    if (!pg_world) { pg_world = pg; }

    /* Add pg's at the tail so that comm world is always the first pg */
    pg->next = 0;
    if (!MPIDI_PG_list)
    {
	MPIDI_PG_list = pg;
    }
    else
    {
	pgnext = MPIDI_PG_list;
	while (pgnext->next)
	{
	    pgnext = pgnext->next;
	}
	pgnext->next = pg;
    }
    /* These are now done in MPIDI_VC_Init */
    *pg_ptr = pg;

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIDI_PG_Destroy(MPIDI_PG_t * pg)
{
    MPIDI_PG_t * pg_prev;
    MPIDI_PG_t * pg_cur;
    int i;
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(MPIU_Object_get_ref(pg) == 0);

    pg_prev = NULL;
    pg_cur = MPIDI_PG_list;

    while(pg_cur != NULL)
    {
	if (pg_cur == pg)
	{
	    if (MPIDI_PG_iterator_next == pg)
	    {
		MPIDI_PG_iterator_next = MPIDI_PG_iterator_next->next;
	    }

	    if (pg_prev == NULL)
		MPIDI_PG_list = pg->next;
	    else
		pg_prev->next = pg->next;

	    TRACE_ERR("destroying pg=%p pg->id=%s\n", pg, (char *)pg->id);

	    for (i = 0; i < pg->size; ++i) {
		/* FIXME it would be good if we could make this assertion.
		   Unfortunately, either:
		   1) We're not being disciplined and some caller of this
		      function doesn't bother to manage all the refcounts
		      because he thinks he knows better.  Annoying, but not
		      strictly a bug.
		      (wdg - actually, that is a bug - managing the ref
		      counts IS required and missing one is a bug.)
		   2) There is a real bug lurking out there somewhere and we
		      just haven't hit it in the tests yet.  */

		/* This used to be handled in MPID_VCRT_Release, but that was
		   not the right place to do this.  The VC should only be freed
		   when the PG that it belongs to is freed, not just when the
		   VC's refcount drops to zero. [goodell@ 2008-06-13] */
		/* In that case, the fact that the VC is in the PG should
		   increment the ref count - reflecting the fact that the
		   use in the PG constitutes a reference-count-incrementing
		   use.  Alternately, if the PG is able to recreate a VC,
		   and can thus free unused (or idle) VCs, it should be allowed
		   to do so.  [wdg 2008-08-31] */
	    }

	    MPIDI_PG_Destroy_fn(pg);
	    TRACE_ERR("destroying pg->vct=%x\n", pg->vct);
	    MPIU_Free(pg->vct);
	    TRACE_ERR("after destroying pg->vct=%x\n", pg->vct);

	    if (pg->connData) {
		if (pg->freeConnInfo) {
                    TRACE_ERR("calling freeConnInfo on pg\n");
		    (*pg->freeConnInfo)( pg );
		}
		else {
                    TRACE_ERR("free pg->connData\n");
		    MPIU_Free(pg->connData);
		}
	    }

	    TRACE_ERR("final destroying pg\n");
	    MPIU_Free(pg);

	    goto fn_exit;
	}

	pg_prev = pg_cur;
	pg_cur = pg_cur->next;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_PG_Find(void * id, MPIDI_PG_t ** pg_ptr)
{
    MPIDI_PG_t * pg;
    int mpi_errno = MPI_SUCCESS;

    pg = MPIDI_PG_list;

    while (pg != NULL)
    {
	if (MPIDI_PG_Compare_ids_fn(id, pg->id) != FALSE)
	{
	    *pg_ptr = pg;
	    goto fn_exit;
	}

	pg = pg->next;
    }

    *pg_ptr = NULL;

  fn_exit:
    return mpi_errno;
}


int MPIDI_PG_Id_compare(void * id1, void *id2)
{
    return MPIDI_PG_Compare_ids_fn(id1, id2);
}

/* iter always points at the next element */
int MPIDI_PG_Get_next(MPIDI_PG_iterator *iter, MPIDI_PG_t ** pg_ptr)
{
    *pg_ptr = (*iter);
    if ((*iter) != NULL) {
	(*iter) = (*iter)->next;
    }

    return MPI_SUCCESS;
}

int MPIDI_PG_Has_next(MPIDI_PG_iterator *iter)
{
    return (*iter != NULL);
}

int MPIDI_PG_Get_iterator(MPIDI_PG_iterator *iter)
{
    *iter = MPIDI_PG_list;
    return MPI_SUCCESS;
}

/* FIXME: What does DEV_IMPLEMENTS_KVS mean?  Why is it used?  Who uses
   PG_To_string and why?  */

/* PG_To_string is used in the implementation of connect/accept (and
   hence in spawn) */
/* Note: Allocated memory that is returned in str_ptr.  The user of
   this routine must free that data */
int MPIDI_PG_To_string(MPIDI_PG_t *pg_ptr, char **str_ptr, int *lenStr)
{
    int mpi_errno = MPI_SUCCESS;

    /* Replace this with the new string */
    MPIDI_connToStringKVS( str_ptr, lenStr, pg_ptr );
#if 0
    if (pg_ptr->connInfoToString) {
	(*pg_ptr->connInfoToString)( str_ptr, lenStr, pg_ptr );
    }
    else {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_INTERN,"**noConnInfoToString");
    }
#endif

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* This routine takes a string description of a process group (created with
   MPIDI_PG_To_string, usually on a different process) and returns a pointer to
   the matching process group.  If the group already exists, flag is set to
   false.  If the group does not exist, it is created with MPIDI_PG_Create (and
   hence is added to the list of active process groups) and flag is set to
   true.  In addition, the connection information is set up using the
   information in the input string.
*/
int MPIDI_PG_Create_from_string(const char * str, MPIDI_PG_t ** pg_pptr,
				int *flag)
{
    int mpi_errno = MPI_SUCCESS;
    const char *p;
    char *pg_id, *pg_id2, *cp2, *cp3,*str2, *str3;
    int vct_sz, i;
    MPIDI_PG_t *existing_pg, *pg_ptr=0;

    TRACE_ERR("MPIDI_PG_Create_from_string - str=%s\n", str);
    /* The pg_id is at the beginning of the string, so we can just pass
       it to the find routine */
    /* printf( "Looking for pg with id %s\n", str );fflush(stdout); */
    mpi_errno = MPIDI_PG_Find((void *)str, &existing_pg);
    if (mpi_errno) TRACE_ERR("MPIDI_PG_Find returned with mpi_errno=%d\n", mpi_errno);

    if (existing_pg != NULL) {
	/* return the existing PG */
	*pg_pptr = existing_pg;
	*flag = 0;
	/* Note that the memory for the pg_id is freed in the exit */
	goto fn_exit;
    }
    *flag = 1;

    /* Get the size from the string */
    p = str;
    while (*p) p++; p++;
    vct_sz = atoi(p);

    while (*p) p++;p++;
    char *p_tmp = MPIU_Strdup(p);
    TRACE_ERR("before MPIDI_PG_Create - p=%s p_tmp=%s vct_sz=%d\n", p, p_tmp, vct_sz);
    mpi_errno = MPIDI_PG_Create(vct_sz, (void *)str, pg_pptr);
    if (mpi_errno != MPI_SUCCESS) {
	TRACE_ERR("MPIDI_PG_Create returned with mpi_errno=%d\n", mpi_errno);
    }

    pg_ptr = *pg_pptr;
    pg_ptr->vct[0].taskid=atoi(strtok(p_tmp,":"));
    for(i=1; i<vct_sz; i++) {
	pg_ptr->vct[i].taskid=atoi(strtok(NULL,":"));
    }
    TRACE_ERR("pg_ptr->id = %s\n",(*pg_pptr)->id);
    MPIU_Free(p_tmp);

    if(verbose)
      MPIU_PG_Printall(stderr);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#ifdef HAVE_CTYPE_H
/* Needed for isdigit */
#include <ctype.h>
#endif


/* For all of these routines, the format of the process group description
   that is created and used by the connTo/FromString routines is this:
   (All items are strings, terminated by null)

   process group id string
   sizeof process group (as string)
   conninfo for rank 0
   conninfo for rank 1
   ...

   The "conninfo for rank 0" etc. for the original (MPI_COMM_WORLD)
   process group are stored in the PMI_KVS space with the keys
   p<rank>-businesscard .

   Fixme: Add a routine to publish the connection info to this file so that
   the key for the businesscard is defined in just this one file.
*/


/* The "KVS" versions are for the process group to which the calling
   process belongs.  These use the PMI_KVS routines to access the
   process information */
static int MPIDI_getConnInfoKVS( int rank, char *buf, int bufsize, MPIDI_PG_t *pg )
{
#ifdef USE_PMI2_API
    char key[MPIDI_MAX_KVS_KEY_LEN];
    int  mpi_errno = MPI_SUCCESS, rc;
    int vallen;

    rc = MPL_snprintf(key, MPIDI_MAX_KVS_KEY_LEN, "P%d-businesscard", rank );

    mpi_errno = PMI2_KVS_Get(pg->connData, PMI2_ID_NULL, key, buf, bufsize, &vallen);
    if (mpi_errno) {
	TRACE_ERR("PMI2_KVS_Get returned with mpi_errno=%d\n", mpi_errno);
    }
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
#else
    char key[MPIDI_MAX_KVS_KEY_LEN];
    int  mpi_errno = MPI_SUCCESS, rc, pmi_errno;

    rc = MPL_snprintf(key, MPIDI_MAX_KVS_KEY_LEN, "P%d-businesscard", rank );
    if (rc < 0 || rc > MPIDI_MAX_KVS_KEY_LEN) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
    }

/*    MPIU_THREAD_CS_ENTER(PMI,);*/
    pmi_errno = PMI_KVS_Get(pg->connData, key, buf, bufsize );
    if (pmi_errno) {
	MPIDI_PG_CheckForSingleton();
	pmi_errno = PMI_KVS_Get(pg->connData, key, buf, bufsize );
    }
/*    MPIU_THREAD_CS_EXIT(PMI,);*/
    if (pmi_errno) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**pmi_kvs_get");
    }

 fn_exit:
   return mpi_errno;
 fn_fail:
    goto fn_exit;
#endif
}

/* *slen is the length of the string, including the null terminator.  So if the
   resulting string is |foo\0bar\0|, then *slen == 8. */
int MPIDI_connToStringKVS( char **buf_p, int *slen, MPIDI_PG_t *pg )
{
    char *string = 0;
    char *pg_idStr = (char *)pg->id;      /* In the PMI/KVS space,
					     the pg id is a string */
    char buf[MPIDI_MAX_KVS_VALUE_LEN];
    int   i, j, vallen, rc, mpi_errno = MPI_SUCCESS, len;
    int   curSlen, nChars;

    /* Make an initial allocation of a string with an estimate of the
       needed space */
    len = 0;
    curSlen = 10 + pg->size * 128;
    string = (char *)MPIU_Malloc( curSlen );

    /* Start with the id of the pg */
    while (*pg_idStr && len < curSlen)
	string[len++] = *pg_idStr++;
    string[len++] = 0;

    /* Add the size of the pg */
    MPL_snprintf( &string[len], curSlen - len, "%d", pg->size );
    while (string[len]) len++;
    string[len++] = 0;

    /* add the taskids of the pg */
    for(i = 0; i < pg->size; i++) {
      MPL_snprintf(buf, MPIDI_MAX_KVS_VALUE_LEN, "%d:", pg->vct[i].taskid);
      vallen = strlen(buf);
      if (len+vallen+1 >= curSlen) {
        char *nstring = 0;
        curSlen += (pg->size - i) * (vallen + 1 );
        nstring = MPIU_Realloc( string, curSlen);
        MPID_assert(nstring != NULL);
        string = nstring;
      }
      /* Append to string */
      nChars = MPL_snprintf(&string[len], curSlen - len, "%d:", pg->vct[i].taskid);
      len+=nChars;
    }

#if 0
    for (i=0; i<pg->size; i++) {
	rc = getConnInfoKVS( i, buf, MPIDI_MAX_KVS_VALUE_LEN, pg );
	if (rc) {
	    MPL_internal_error_printf(
		    "Panic: getConnInfoKVS failed for %s (rc=%d)\n",
		    (char *)pg->id, rc );
	}
#ifndef USE_PERSISTENT_SHARED_MEMORY
	/* FIXME: This is a hack to avoid including shared-memory
	   queue names in the business card that may be used
	   by processes that were not part of the same COMM_WORLD.
	   To fix this, the shared memory channels should look at the
	   returned connection info and decide whether to use
	   sockets or shared memory by determining whether the
	   process is in the same MPI_COMM_WORLD. */
	/* FIXME: The more general problem is that the connection information
	   needs to include some information on the range of validity (e.g.,
	   all processes, same comm world, particular ranks), and that
	   representation needs to be scalable */
/*	printf( "Adding key %s value %s\n", key, val ); */
	{
	char *p = strstr( buf, "$shm_host" );
	if (p) p[1] = 0;
	/*	    printf( "(fixed) Adding key %s value %s\n", key, val ); */
	}
#endif
	/* Add the information to the output buffer */
	vallen = strlen(buf);
	/* Check that this will fix in the remaining space */
	if (len + vallen + 1 >= curSlen) {
	    char *nstring = 0;
            curSlen += (pg->size - i) * (vallen + 1 );
	    nstring = MPIU_Realloc( string, curSlen);
	    if (!nstring) {
		MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomem");
	    }
	    string = nstring;
	}
	/* Append to string */
	for (j=0; j<vallen+1; j++) {
	    string[len++] = buf[j];
	}
    }
#endif

    MPIU_Assert(len <= curSlen);

    *buf_p = string;
    *slen  = len;
 fn_exit:
    return mpi_errno;
 fn_fail:
    if (string) MPIU_Free(string);
    goto fn_exit;
}

static int MPIDI_connFromStringKVS( const char *buf ATTRIBUTE((unused)),
			      MPIDI_PG_t *pg ATTRIBUTE((unused)) )
{
    /* Fixme: this should be a failure to call this routine */
    return MPI_SUCCESS;
}
static int MPIDI_connFreeKVS( MPIDI_PG_t *pg )
{
    if (pg->connData) {
	MPIU_Free( pg->connData );
    }
    return MPI_SUCCESS;
}


int MPIDI_PG_InitConnKVS( MPIDI_PG_t *pg )
{
#ifdef USE_PMI2_API
    int mpi_errno = MPI_SUCCESS;

    pg->connData = (char *)MPIU_Malloc(MAX_JOBID_LEN);
    if (pg->connData == NULL) {
	TRACE_ERR("MPIDI_PG_InitConnKVS - MPIU_Malloc failure\n");
    }

    mpi_errno = PMI2_Job_GetId(pg->connData, MAX_JOBID_LEN);
    if (mpi_errno) TRACE_ERR("PMI2_Job_GetId returned with mpi_errno=%d\n", mpi_errno);
#else
    int pmi_errno, kvs_name_sz;
    int mpi_errno = MPI_SUCCESS;

    pmi_errno = PMI_KVS_Get_name_length_max( &kvs_name_sz );
    if (pmi_errno != PMI_SUCCESS) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
			     "**pmi_kvs_get_name_length_max",
			     "**pmi_kvs_get_name_length_max %d", pmi_errno);
    }

    pg->connData = (char *)MPIU_Malloc(kvs_name_sz + 1);
    if (pg->connData == NULL) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER, "**nomem");
    }

    pmi_errno = PMI_KVS_Get_my_name(pg->connData, kvs_name_sz);
    if (pmi_errno != PMI_SUCCESS) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,
			     "**pmi_kvs_get_my_name",
			     "**pmi_kvs_get_my_name %d", pmi_errno);
    }
#endif
    pg->getConnInfo        = MPIDI_getConnInfoKVS;
    pg->connInfoToString   = MPIDI_connToStringKVS;
    pg->connInfoFromString = MPIDI_connFromStringKVS;
    pg->freeConnInfo       = MPIDI_connFreeKVS;

 fn_exit:
    return mpi_errno;
 fn_fail:
    if (pg->connData) { MPIU_Free(pg->connData); }
    goto fn_exit;
}

/* Return the kvsname associated with the MPI_COMM_WORLD of this process. */
int MPIDI_PG_GetConnKVSname( char ** kvsname )
{
    *kvsname = pg_world->connData;
    return MPI_SUCCESS;
}

/* For process groups that are not our MPI_COMM_WORLD, store the connection
   information in an array of strings.  These routines and structure
   implement the access to this information. */
typedef struct {
    int     toStringLen;   /* Length needed to encode this connection info */
    char ** connStrings;   /* pointer to an array, indexed by rank, containing
			      connection information */
} MPIDI_ConnInfo;


static int MPIDI_getConnInfo( int rank, char *buf, int bufsize, MPIDI_PG_t *pg )
{
    MPIDI_ConnInfo *connInfo = (MPIDI_ConnInfo *)pg->connData;

    /* printf( "Entering getConnInfo\n" ); fflush(stdout); */
    if (!connInfo || !connInfo->connStrings || !connInfo->connStrings[rank]) {
	/* FIXME: Turn this into a valid error code create/return */
	/*printf( "Fatal error in getConnInfo (rank = %d)\n", rank );
	printf( "connInfo = %p\n", connInfo );fflush(stdout); */
	if (connInfo) {
/*	    printf( "connInfo->connStrings = %p\n", connInfo->connStrings ); */
	}
	/* Fatal error.  Connection information missing */
	fflush(stdout);
    }

    /* printf( "Copying %s to buf\n", connInfo->connStrings[rank] ); fflush(stdout); */

    MPIU_Strncpy( buf, connInfo->connStrings[rank], bufsize );
    return MPI_SUCCESS;
}


static int MPIDI_connToString( char **buf_p, int *slen, MPIDI_PG_t *pg )
{
    int mpi_errno = MPI_SUCCESS;
    char *str = NULL, *pg_id;
    int  i, len=0;
    MPIDI_ConnInfo *connInfo = (MPIDI_ConnInfo *)pg->connData;

    /* Create this from the string array */
    str = (char *)MPIU_Malloc(connInfo->toStringLen);

#if defined(MPICH_DEBUG_MEMINIT)
    memset(str, 0, connInfo->toStringLen);
#endif

    pg_id = pg->id;
    /* FIXME: This is a hack, and it doesn't even work */
    /*    MPIDI_PrintConnStrToFile( stdout, __FILE__, __LINE__,
	  "connToString: pg id is", (char *)pg_id );*/
    /* This is intended to cause a process to transition from a singleton
       to a non-singleton. */
    /* XXX DJG TODO figure out what this little bit is all about. */
    if (strstr( pg_id, "singinit_kvs" ) == pg_id) {
#ifdef USE_PMI2_API
        MPIU_Assertp(0); /* don't know what to do here for pmi2 yet.  DARIUS */
#else
	PMI_KVS_Get_my_name( pg->id, 256 );
#endif
    }

    while (*pg_id) str[len++] = *pg_id++;
    str[len++] = 0;

    MPL_snprintf( &str[len], 20, "%d", pg->size);
    /* Skip over the length */
    while (str[len++]);

    /* Copy each connection string */
    for (i=0; i<pg->size; i++) {
	char *p = connInfo->connStrings[i];
	while (*p) { str[len++] = *p++; }
	str[len++] = 0;
    }

    if (len > connInfo->toStringLen) {
	*buf_p = 0;
	*slen  = 0;
	TRACE_ERR("len > connInfo->toStringLen");
    }

    *buf_p = str;
    *slen = len;

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;

}


static int MPIDI_connFromString( const char *buf, MPIDI_PG_t *pg )
{
    MPIDI_ConnInfo *conninfo = 0;
    int i, mpi_errno = MPI_SUCCESS;
    const char *buf0 = buf;   /* save the start of buf */

    /* printf( "Starting with buf = %s\n", buf );fflush(stdout); */

    /* Skip the pg id */
    while (*buf) buf++; buf++;

    /* Determine the size of the pg */
    pg->size = atoi( buf );
    while (*buf) buf++; buf++;

    conninfo = (MPIDI_ConnInfo *)MPIU_Malloc( sizeof(MPIDI_ConnInfo) );
    conninfo->connStrings = (char **)MPIU_Malloc( pg->size * sizeof(char *));

    /* For now, make a copy of each item */
    for (i=0; i<pg->size; i++) {
	/* printf( "Adding conn[%d] = %s\n", i, buf );fflush(stdout); */
	conninfo->connStrings[i] = MPIU_Strdup( buf );
	while (*buf) buf++;
	buf++;
    }
    pg->connData = conninfo;

    /* Save the length of the string needed to encode the connection
       information */
    conninfo->toStringLen = (int)(buf - buf0) + 1;

    return mpi_errno;
}


static int MPIDI_connFree( MPIDI_PG_t *pg )
{
    MPIDI_ConnInfo *conninfo = (MPIDI_ConnInfo *)pg->connData;
    int i;

    for (i=0; i<pg->size; i++) {
	MPIU_Free( conninfo->connStrings[i] );
    }
    MPIU_Free( conninfo->connStrings );
    MPIU_Free( conninfo );

    return MPI_SUCCESS;
}


/*@
  MPIDI_PG_Dup_vcr - Duplicate a virtual connection from a process group

  Notes:
  This routine provides a dup of a virtual connection given a process group
  and a rank in that group.  This routine is used only in initializing
  the MPI-1 communicators 'MPI_COMM_WORLD' and 'MPI_COMM_SELF', and in creating
  the initial intercommunicator after an 'MPI_Comm_spawn',
  'MPI_Comm_spawn_multiple', or 'MPI_Comm_connect/MPI_Comm_accept'.

  In addition to returning a dup of the virtual connection, it manages the
  reference count of the process group, which is always the number of inuse
  virtual connections.
  @*/
int MPIDI_PG_Dup_vcr( MPIDI_PG_t *pg, int rank, pami_task_t taskid, MPID_VCR *vcr_p )
{
    int inuse;
    MPID_VCR vcr;

    TRACE_ERR("ENTER MPIDI_PG_Dup_vcr - pg->id=%s rank=%d taskid=%d\n", pg->id, rank, taskid);
    pg->vct[rank].taskid = taskid;

    vcr = MPIU_Malloc(sizeof(struct MPID_VCR_t));
    TRACE_ERR("MPIDI_PG_Dup_vcr- pg->vct[%d].pg=%x pg=%x vcr=%x vcr->pg=%x\n", rank, pg->vct[rank].pg, pg, vcr, vcr->pg);
    vcr->pg = pg;
    vcr->pg_rank = rank;
    vcr->taskid = taskid;
    /* Increase the reference count of the vc.  If the reference count
       increases from 0 to 1, increase the reference count of the
       process group *and* the reference count of the vc (this
       allows us to distinquish between Comm_free and Comm_disconnect) */
    /* FIXME-MT: This should be a fetch and increment for thread-safety */
    /*if (MPIU_Object_get_ref(vcr_p) == 0) { */
	TRACE_ERR("MPIDI_PG_add_ref on pg=%s pg=%x\n", pg->id, pg);
	MPIDI_PG_add_ref(pg);
        inuse=MPIU_Object_get_ref(pg);
	TRACE_ERR("after MPIDI_PG_add_ref on pg=%s inuse=%d\n", pg->id, inuse);
/*	MPIDI_VC_add_ref(vcr_p);
    }
    MPIDI_VC_add_ref(vcr_p);*/
    *vcr_p = vcr;

    return MPI_SUCCESS;
}


/*
 * This routine may be called to print the contents (including states and
 * reference counts) for process groups.
 */
int MPIU_PG_Printall( FILE *fp )
{
    MPIDI_PG_t *pg;
    int         i;

    pg = MPIDI_PG_list;

    fprintf( fp, "Process groups:\n" );
    while (pg) {
        /* XXX DJG FIXME-MT should we be checking this? */
	fprintf( fp, "size = %d, refcount = %d, id = %s\n",
		 pg->size, MPIU_Object_get_ref(pg), (char *)pg->id );
	for (i=0; i<pg->size; i++) {
	    fprintf( fp, "\tVCT rank = %d, refcount = %d, taskid = %d\n",
		     pg->vct[i].pg_rank, MPIU_Object_get_ref(pg),
		     pg->vct[i].taskid );
	}
	fflush(fp);
	pg = pg->next;
    }

    return 0;
}

#ifdef HAVE_CTYPE_H
/* Needed for isdigit */
#include <ctype.h>
#endif
/* Convert a process group id into a number.  This is a hash-based approach,
 * which has the potential for some collisions.  This is an alternative to the
 * previous approach that caused req#3930, which was to sum up the values of the
 * characters.  The summing approach worked OK when the id's were all similar
 * but with an incrementing prefix or suffix, but terrible for a 32 hex-character
 * UUID type of id.
 *
 * FIXME It would really be best if the PM could give us this value.
 */
/* FIXME: This is a temporary hack for devices that do not define
   MPIDI_DEV_IMPLEMENTS_KVS
   FIXME: MPIDI_DEV_IMPLEMENTS_KVS should be removed
 */
void MPIDI_PG_IdToNum( MPIDI_PG_t *pg, int *id )
{
    *id = atoi((char *)pg->id);
}


int MPID_PG_ForwardPGInfo( MPID_Comm *peer_ptr, MPID_Comm *comm_ptr,
			   int nPGids, const int gpids[],
			   int root )
{
    int mpi_errno = MPI_SUCCESS;
    int i, allfound = 1, pgid, pgidWorld;
    MPIDI_PG_t *pg = 0;
    MPIDI_PG_iterator iter;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    if(mpidi_dynamic_tasking) {
    /* Get the pgid for CommWorld (always attached to the first process
       group) */
    MPIDI_PG_Get_iterator(&iter);
    MPIDI_PG_Get_next( &iter, &pg );
    MPIDI_PG_IdToNum( pg, &pgidWorld );

    /* Extract the unique process groups */
    for (i=0; i<nPGids && allfound; i++) {
	if (gpids[0] != pgidWorld) {
	    /* Add this gpid to the list of values to check */
	    /* FIXME: For testing, we just test in place */
            MPIDI_PG_Get_iterator(&iter);
	    do {
                MPIDI_PG_Get_next( &iter, &pg );
		if (!pg) {
		    /* We don't know this pgid */
		    allfound = 0;
		    break;
		}
		MPIDI_PG_IdToNum( pg, &pgid );
	    } while (pgid != gpids[0]);
	}
	gpids += 2;
    }

    /* See if everyone is happy */
    mpi_errno = MPIR_Allreduce_impl( MPI_IN_PLACE, &allfound, 1, MPI_INT, MPI_LAND, comm_ptr, &errflag );

    if (allfound) return MPI_SUCCESS;

    /* FIXME: We need a cleaner way to handle this case than using an ifdef.
       We could have an empty version of MPID_PG_BCast in ch3u_port.c, but
       that's a rather crude way of addressing this problem.  Better is to
       make the handling of local and remote PIDS for the dynamic process
       case part of the dynamic process "module"; devices that don't support
       dynamic processes (and hence have only COMM_WORLD) could optimize for
       that case */
    /* We need to share the process groups.  We use routines
       from ch3u_port.c */
    MPID_PG_BCast( peer_ptr, comm_ptr, root );
    }
 fn_exit:
    return MPI_SUCCESS;
 fn_fail:
    goto fn_exit;
}
#endif
