/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif

#ifdef DYNAMIC_TASKING

extern mpidi_dynamic_tasking;

/* Define the name of the kvs key used to provide the port name to the
   children */
#define MPIDI_PARENT_PORT_KVSKEY "PARENT_ROOT_PORT_NAME"

/* FIXME: We can avoid these two routines if we define PMI as using
   MPI info values */
/* Turn a SINGLE MPI_Info into an array of PMI_keyvals (return the pointer
   to the array of PMI keyvals) */
static int  MPIDI_mpi_to_pmi_keyvals( MPID_Info *info_ptr, PMI_keyval_t **kv_ptr, int *nkeys_ptr )
{
    char key[MPI_MAX_INFO_KEY];
    PMI_keyval_t *kv = 0;
    int          i, nkeys = 0, vallen, flag, mpi_errno=MPI_SUCCESS;

    if (!info_ptr || info_ptr->handle == MPI_INFO_NULL) {
	goto fn_exit;
    }

    MPIR_Info_get_nkeys_impl( info_ptr, &nkeys );
    if (nkeys == 0) {
	goto fn_exit;
    }
    kv = (PMI_keyval_t *)MPIU_Malloc( nkeys * sizeof(PMI_keyval_t) );

    for (i=0; i<nkeys; i++) {
	mpi_errno = MPIR_Info_get_nthkey_impl( info_ptr, i, key );
	if (mpi_errno)
          TRACE_ERR("MPIR_Info_get_nthkey_impl returned with mpi_errno=%d\n", mpi_errno);
	MPIR_Info_get_valuelen_impl( info_ptr, key, &vallen, &flag );

	kv[i].key = MPIU_Strdup(key);
	kv[i].val = MPIU_Malloc( vallen + 1 );
	MPIR_Info_get_impl( info_ptr, key, vallen+1, kv[i].val, &flag );
	TRACE_OUT("key: <%s>, value: <%s>\n", kv[i].key, kv[i].val);
    }

 fn_fail:
 fn_exit:
    *kv_ptr    = kv;
    *nkeys_ptr = nkeys;
    return mpi_errno;
}


/* Free the entire array of PMI keyvals */
static void MPIDI_free_pmi_keyvals(PMI_keyval_t **kv, int size, int *counts)
{
    int i,j;

    for (i=0; i<size; i++)
    {
	for (j=0; j<counts[i]; j++)
	{
	    if (kv[i][j].key != NULL)
		MPIU_Free((char *)kv[i][j].key);
	    if (kv[i][j].val != NULL)
		MPIU_Free(kv[i][j].val);
	}
	if (kv[i] != NULL)
	{
	    MPIU_Free(kv[i]);
	}
    }
}

/*@
   MPID_Comm_spawn_multiple -

   Input Arguments:
+  int count - count
.  char *array_of_commands[] - commands
.  char* *array_of_argv[] - arguments
.  int array_of_maxprocs[] - maxprocs
.  MPI_Info array_of_info[] - infos
.  int root - root
-  MPI_Comm comm - communicator

   Output Arguments:
+  MPI_Comm *intercomm - intercommunicator
-  int array_of_errcodes[] - error codes

   Notes:

.N Errors
.N MPI_SUCCESS
@*/
#undef FUNCNAME
#define FUNCNAME MPID_Comm_spawn_multiple
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_Comm_spawn_multiple(int count, char *array_of_commands[],
			     char ** array_of_argv[], const int array_of_maxprocs[],
			     MPID_Info * array_of_info_ptrs[], int root,
			     MPID_Comm * comm_ptr, MPID_Comm ** intercomm,
			     int array_of_errcodes[])
{
    int mpi_errno = MPI_SUCCESS;

    if(mpidi_dynamic_tasking == 0) {
	fprintf(stderr, "Received spawn request for non-dynamic jobs\n");
        MPIU_ERR_SETANDSTMT(mpi_errno, MPI_ERR_SPAWN,
                            return mpi_errno, "**spawn");
    }

    /* We allow an empty implementation of this function to
       simplify building MPICH on systems that have difficulty
       supporing process creation */
    mpi_errno = MPIDI_Comm_spawn_multiple(count, array_of_commands,
					  array_of_argv, array_of_maxprocs,
					  array_of_info_ptrs,
					  root, comm_ptr, intercomm,
					  array_of_errcodes);
    return mpi_errno;
}


/*
 * MPIDI_Comm_spawn_multiple()
 */
int MPIDI_Comm_spawn_multiple(int count, char **commands,
                              char ***argvs, int *maxprocs,
                              MPID_Info **info_ptrs, int root,
                              MPID_Comm *comm_ptr, MPID_Comm
                              **intercomm, int *errcodes)
{
    char port_name[MPI_MAX_PORT_NAME];
    char jobId[64];
    char ctxid_str[16];
    int jobIdSize = 64;
    int len=0;
    int *info_keyval_sizes=0, i, mpi_errno=MPI_SUCCESS;
    PMI_keyval_t **info_keyval_vectors=0, preput_keyval_vector;
    int *pmi_errcodes = 0, pmi_errno=0;
    int total_num_processes, should_accept = 1;
    MPID_Info tmp_info_ptr;
    char *tmp;
    int tmp_ret = 0;

    if (comm_ptr->rank == root) {
	/* create an array for the pmi error codes */
	total_num_processes = 0;
	for (i=0; i<count; i++) {
	    total_num_processes += maxprocs[i];
	}
	pmi_errcodes = (int*)MPIU_Malloc(sizeof(int) * total_num_processes);

	/* initialize them to 0 */
	for (i=0; i<total_num_processes; i++)
	    pmi_errcodes[i] = 0;

	/* Open a port for the spawned processes to connect to */
	/* FIXME: info may be needed for port name */
        mpi_errno = MPID_Open_port(NULL, port_name);
	TRACE_ERR("mpi_errno from MPID_Open_port=%d\n", mpi_errno);

	/* Spawn the processes */
#ifdef USE_PMI2_API
        MPIU_Assert(count > 0);
        {
            int *argcs = MPIU_Malloc(count*sizeof(int));
            struct MPID_Info preput;
            struct MPID_Info *preput_p[2] = { &preput, &tmp_info_ptr };

            MPIU_Assert(argcs);

            info_keyval_sizes = MPIU_Malloc(count * sizeof(int));

            /* FIXME cheating on constness */
            preput.key = (char *)MPIDI_PARENT_PORT_KVSKEY;
            preput.value = port_name;
            preput.next = &tmp_info_ptr;

	    tmp_info_ptr.key = "COMMCTX";
	    len=sprintf(ctxid_str, "%d", comm_ptr->context_id);
	    TRACE_ERR("COMMCTX=%d\n", comm_ptr->context_id);
	    ctxid_str[len]='\0';
	    tmp_info_ptr.value = ctxid_str;
	    tmp_info_ptr.next = NULL;

            /* compute argcs array */
            for (i = 0; i < count; ++i) {
                argcs[i] = 0;
                if (argvs != NULL && argvs[i] != NULL) {
                    while (argvs[i][argcs[i]]) {
                        ++argcs[i];
                    }
                }
            }

            /*MPIU_THREAD_CS_ENTER(PMI,);*/
            /* release the global CS for spawn PMI calls */
            MPIU_THREAD_CS_EXIT(ALLFUNC,);
            pmi_errno = PMI2_Job_Spawn(count, (const char **)commands,
                                       argcs, (const char ***)argvs,
                                       maxprocs,
                                       info_keyval_sizes, (const MPID_Info **)info_ptrs,
                                       2, (const struct MPID_Info **)preput_p,
                                       jobId, jobIdSize,
                                       pmi_errcodes);
	    TRACE_ERR("after PMI2_Job_Spawn - pmi_errno=%d jobId=%s\n", pmi_errno, jobId);
            MPIU_THREAD_CS_ENTER(ALLFUNC,);

	    tmp=MPIU_Strdup(jobId);
	    tmp_ret = atoi(strtok(tmp, ";"));

	    if( (pmi_errno == PMI2_SUCCESS) && (tmp_ret != -1) ) {
	      pami_task_t leader_taskid = atoi(strtok(NULL, ";"));
	      pami_endpoint_t ldest;

              PAMI_Endpoint_create(MPIDI_Client,  leader_taskid, 0, &ldest);
	      TRACE_ERR("PAMI_Resume to taskid=%d\n", leader_taskid);
              PAMI_Resume(MPIDI_Context[0], &ldest, 1);
            }

            MPIU_Free(tmp);

            MPIU_Free(argcs);
            if (pmi_errno != PMI2_SUCCESS) {
               TRACE_ERR("PMI2_Job_Spawn returned with pmi_errno=%d\n", pmi_errno);
            }
        }
#else
        /* FIXME: This is *really* awkward.  We should either
           Fix on MPI-style info data structures for PMI (avoid unnecessary
           duplication) or add an MPIU_Info_getall(...) that creates
           the necessary arrays of key/value pairs */

        /* convert the infos into PMI keyvals */
        info_keyval_sizes   = (int *) MPIU_Malloc(count * sizeof(int));
        info_keyval_vectors =
            (PMI_keyval_t**) MPIU_Malloc(count * sizeof(PMI_keyval_t*));

        if (!info_ptrs) {
            for (i=0; i<count; i++) {
                info_keyval_vectors[i] = 0;
                info_keyval_sizes[i]   = 0;
            }
        }
        else {
            for (i=0; i<count; i++) {
                mpi_errno = MPIDI_mpi_to_pmi_keyvals( info_ptrs[i],
                                                &info_keyval_vectors[i],
                                                &info_keyval_sizes[i] );
                if (mpi_errno) { TRACE_ERR("MPIDI_mpi_to_pmi_keyvals returned with mpi_errno=%d\n", mpi_errno); }
            }
        }

        preput_keyval_vector.key = MPIDI_PARENT_PORT_KVSKEY;
        preput_keyval_vector.val = port_name;

        pmi_errno = PMI_Spawn_multiple(count, (const char **)
                                       commands,
                                       (const char ***) argvs,
                                       maxprocs, info_keyval_sizes,
                                       (const PMI_keyval_t **)
                                       info_keyval_vectors, 1,
                                       &preput_keyval_vector,
                                       pmi_errcodes);
	TRACE_ERR("pmi_errno from PMI_Spawn_multiple=%d\n", pmi_errno);
#endif

        if (errcodes != MPI_ERRCODES_IGNORE) {
	    for (i=0; i<total_num_processes; i++) {
		/* FIXME: translate the pmi error codes here */
		errcodes[i] = pmi_errcodes[0];
                /* We want to accept if any of the spawns succeeded.
                   Alternatively, this is the same as we want to NOT accept if
                   all of them failed.  should_accept = NAND(e_0, ..., e_n)
                   Remember, success equals false (0). */
                should_accept = should_accept && errcodes[i];
	    }
            should_accept = !should_accept; /* the `N' in NAND */
	}

#ifdef USE_PMI2_API
        if( (pmi_errno == PMI2_SUCCESS) && (tmp_ret == -1) )
#else
        if( (pmi_errno == PMI_SUCCESS) && (tmp_ret == -1) )
#endif
	  should_accept = 0;
    }

    if (errcodes != MPI_ERRCODES_IGNORE) {
        MPIR_Errflag_t errflag = MPIR_ERR_NONE;
        mpi_errno = MPIR_Bcast_impl(&should_accept, 1, MPI_INT, root, comm_ptr, &errflag);
        if (mpi_errno) TRACE_ERR("MPIR_Bcast_impl returned with mpi_errno=%d\n", mpi_errno);

        mpi_errno = MPIR_Bcast_impl(&pmi_errno, 1, MPI_INT, root, comm_ptr, &errflag);
        if (mpi_errno) TRACE_ERR("MPIR_Bcast_impl returned with mpi_errno=%d\n", mpi_errno);

        mpi_errno = MPIR_Bcast_impl(&total_num_processes, 1, MPI_INT, root, comm_ptr, &errflag);
        if (mpi_errno) TRACE_ERR("MPIR_Bcast_impl returned with mpi_errno=%d\n", mpi_errno);

        mpi_errno = MPIR_Bcast_impl(errcodes, total_num_processes, MPI_INT, root, comm_ptr, &errflag);
        if (mpi_errno) TRACE_ERR("MPIR_Bcast_impl returned with mpi_errno=%d\n", mpi_errno);
    }

    if (should_accept) {
        mpi_errno = MPID_Comm_accept(port_name, NULL, root, comm_ptr, intercomm);
	TRACE_ERR("mpi_errno from MPID_Comm_accept=%d\n", mpi_errno);
    } else {
	if( (pmi_errno == PMI2_SUCCESS) && (errcodes[0] != 0) ) {
	  MPIR_Comm_create(intercomm);
	}
    }

    if (comm_ptr->rank == root) {
	/* Close the port opened for the spawned processes to connect to */
	mpi_errno = MPID_Close_port(port_name);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	    TRACE_ERR("MPID_Close_port returned with mpi_errno=%d\n", mpi_errno);
	/* --END ERROR HANDLING-- */
    }

    if(pmi_errno) {
           mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, __FILE__, __LINE__, MPI_ERR_SPAWN,
            "**mpi_comm_spawn", 0);
    }

 fn_exit:
    if (info_keyval_vectors) {
	MPIDI_free_pmi_keyvals(info_keyval_vectors, count, info_keyval_sizes);
	MPIU_Free(info_keyval_vectors);
    }
    if (info_keyval_sizes) {
	MPIU_Free(info_keyval_sizes);
    }
    if (pmi_errcodes) {
	MPIU_Free(pmi_errcodes);
    }
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* This function is used only with mpid_init to set up the parent communicator
   if there is one.  The routine should be in this file because the parent
   port name is setup with the "preput" arguments to PMI_Spawn_multiple */
static char *parent_port_name = 0;    /* Name of parent port if this
					 process was spawned (and is root
					 of comm world) or null */
#undef FUNCNAME
#define FUNCNAME MPIDI_GetParentPort
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIDI_GetParentPort(char ** parent_port)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    char val[MPIDI_MAX_KVS_VALUE_LEN];

    if (parent_port_name == NULL)
    {
	char *kvsname = NULL;
	/* We can always use PMI_KVS_Get on our own process group */
	MPIDI_PG_GetConnKVSname( &kvsname );
#ifdef USE_PMI2_API
        {
            int vallen = 0;
            pmi_errno = PMI2_KVS_Get(kvsname, PMI2_ID_NULL, MPIDI_PARENT_PORT_KVSKEY, val, sizeof(val), &vallen);
	    TRACE_ERR("PMI2_KVS_Get - val=%s\n", val);
            if (pmi_errno)
                TRACE_ERR("PMI2_KVS_Get returned with pmi_errno=%d\n", pmi_errno);
        }
#else
	/*MPIU_THREAD_CS_ENTER(PMI,);*/
	pmi_errno = PMI_KVS_Get( kvsname, MPIDI_PARENT_PORT_KVSKEY, val, sizeof(val));
/*	MPIU_THREAD_CS_EXIT(PMI,);*/
	if (pmi_errno) {
            mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**pmi_kvsget", "**pmi_kvsget %d", pmi_errno);
            goto fn_exit;
	}
#endif
	parent_port_name = MPIU_Strdup(val);
    }

    *parent_port = parent_port_name;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


void MPIDI_FreeParentPort(void)
{
    if (parent_port_name) {
	MPIU_Free( parent_port_name );
	parent_port_name = 0;
    }
}


#endif
