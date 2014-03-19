/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/mpid_vc.c
 * \brief Maintain the virtual connection reference table
 */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpidimpl.h>

extern int mpidi_dynamic_tasking;
/**
 * \brief Virtual connection reference table
 */
struct MPIDI_VCRT
{
  MPIU_OBJECT_HEADER;
  unsigned size;          /**< Number of entries in the table */
  MPID_VCR *vcr_table;  /**< Array of virtual connection references */
};


int MPID_VCR_Dup(MPID_VCR orig_vcr, MPID_VCR * new_vcr)
{
#ifdef DYNAMIC_TASKING
    if(mpidi_dynamic_tasking) {
      if (orig_vcr->pg) {
        MPIDI_PG_add_ref( orig_vcr->pg );
      }
    }
#endif

    (*new_vcr)->taskid = orig_vcr->taskid;
#ifdef DYNAMIC_TASKING
    (*new_vcr)->pg_rank = orig_vcr->pg_rank;
    (*new_vcr)->pg = orig_vcr->pg;
#endif
    return MPI_SUCCESS;
}

int MPID_VCR_Get_lpid(MPID_VCR vcr, int * lpid_ptr)
{
    *lpid_ptr = (int)(vcr->taskid);
    return MPI_SUCCESS;
}

int MPID_VCRT_Create(int size, MPID_VCRT *vcrt_ptr)
{
    struct MPIDI_VCRT * vcrt;
    int i,result;

    vcrt = MPIU_Malloc(sizeof(struct MPIDI_VCRT));
    vcrt->vcr_table = MPIU_Malloc(size*sizeof(MPID_VCR));

    for(i = 0; i < size; i++) {
	vcrt->vcr_table[i] = MPIU_Malloc(sizeof(struct MPID_VCR_t));
    }
    if (vcrt != NULL)
    {
        MPIU_Object_set_ref(vcrt, 1);
        vcrt->size = size;
        *vcrt_ptr = vcrt;
        result = MPI_SUCCESS;
    }
    else
    {
        result = MPIR_ERR_MEMALLOCFAILED;
    }
    return result;
}

int MPID_VCRT_Add_ref(MPID_VCRT vcrt)
{
    MPIU_Object_add_ref(vcrt);
    return MPI_SUCCESS;
}

int MPID_VCRT_Release(MPID_VCRT vcrt, int isDisconnect)
{
    int count, i;

    MPIU_Object_release_ref(vcrt, &count);

    if (count == 0) {
#ifdef DYNAMIC_TASKING
     int inuse;
     if(mpidi_dynamic_tasking) {
      for (i = 0; i < vcrt->size; i++)
        {
          MPID_VCR const vcr = vcrt->vcr_table[i];

            if (vcr->pg == MPIDI_Process.my_pg &&
                vcr->pg_rank == MPIDI_Process.my_pg_rank)
              {
	        TRACE_ERR("before MPIDI_PG_release_ref on vcr=%x pg=%x pg=%s inuse=%d\n", vcr, vcr->pg, (vcr->pg)->id, inuse);
                inuse=MPIU_Object_get_ref(vcr->pg);
                MPIDI_PG_release_ref(vcr->pg, &inuse);
                if (inuse == 0)
                 {
                   MPIDI_PG_Destroy(vcr->pg);
                   MPIU_Free(vcr);
                 }
                 continue;
              }
            inuse=MPIU_Object_get_ref(vcr->pg);

            MPIDI_PG_release_ref(vcr->pg, &inuse);
            if (inuse == 0)
              MPIDI_PG_Destroy(vcr->pg);
        if(vcr) MPIU_Free(vcr);
       }
       MPIU_Free(vcrt->vcr_table);
     } /** CHECK */
     else {
      for (i = 0; i < vcrt->size; i++)
	MPIU_Free(vcrt->vcr_table[i]);
      MPIU_Free(vcrt->vcr_table);vcrt->vcr_table=NULL;
     }
#else
      for (i = 0; i < vcrt->size; i++)
	MPIU_Free(vcrt->vcr_table[i]);
      MPIU_Free(vcrt->vcr_table);vcrt->vcr_table=NULL;
#endif
     MPIU_Free(vcrt);vcrt=NULL;
    }
    return MPI_SUCCESS;
}

int MPID_VCRT_Get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr)
{
    *vc_pptr = vcrt->vcr_table;
    return MPI_SUCCESS;
}

#ifdef DYNAMIC_TASKING
int MPID_VCR_CommFromLpids( MPID_Comm *newcomm_ptr,
			    int size, const int lpids[] )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *commworld_ptr;
    int i;
    MPIDI_PG_iterator iter;

    commworld_ptr = MPIR_Process.comm_world;
    /* Setup the communicator's vc table: remote group */
    MPID_VCRT_Create( size, &newcomm_ptr->vcrt );
    MPID_VCRT_Get_ptr( newcomm_ptr->vcrt, &newcomm_ptr->vcr );
    if(mpidi_dynamic_tasking) {
      for (i=0; i<size; i++) {
        MPID_VCR vc = 0;

	/* For rank i in the new communicator, find the corresponding
	   virtual connection.  For lpids less than the size of comm_world,
	   we can just take the corresponding entry from comm_world.
	   Otherwise, we need to search through the process groups.
	*/
	/* printf( "[%d] Remote rank %d has lpid %d\n",
	   MPIR_Process.comm_world->rank, i, lpids[i] ); */
#if 0
	if (lpids[i] < commworld_ptr->remote_size) {
           vc = commworld_ptr->vcr[lpids[i]];
	}
	else {
#endif
	    /* We must find the corresponding vcr for a given lpid */
	    /* For now, this means iterating through the process groups */
	    MPIDI_PG_t *pg = 0;
	    int j;

	    MPIDI_PG_Get_iterator(&iter);
	    /* Skip comm_world */
            /*MPIDI_PG_Get_next( &iter, &pg ); */
	    do {
		MPIDI_PG_Get_next( &iter, &pg );
                /*MPIU_ERR_CHKINTERNAL(!pg, mpi_errno, "no pg"); */
		/* FIXME: a quick check on the min/max values of the lpid
		   for this process group could help speed this search */
		for (j=0; j<pg->size; j++) {
		    /*printf( "Checking lpid %d against %d in pg %s\n",
                            lpids[i], pg->vct[j].taskid, (char *)pg->id );
                           fflush(stdout); */
		    if (pg->vct[j].taskid == lpids[i]) {
			vc = &pg->vct[j];
			/*printf( "found vc %x for lpid = %d in another pg\n",
			  (int)vc, lpids[i] );*/
			break;
		    }
		}
	    } while (!vc);
#if 0
	}
#endif

	/* printf( "about to dup vc %x for lpid = %d in another pg\n",
	   (int)vc, lpids[i] ); */
	/* Note that his will increment the ref count for the associate
	   PG if necessary.  */
	MPID_VCR_Dup( vc, &newcomm_ptr->vcr[i] );
      }
    } else  {
      for (i=0; i<size; i++) {
        /* For rank i in the new communicator, find the corresponding
           rank in the comm world (FIXME FOR MPI2) */
        /* printf( "[%d] Remote rank %d has lpid %d\n",
           MPIR_Process.comm_world->rank, i, lpids[i] ); */
        if (lpids[i] < commworld_ptr->remote_size) {
            MPID_VCR_Dup( commworld_ptr->vcr[lpids[i]],
                          &newcomm_ptr->vcr[i] );
        }
        else {
            /* We must find the corresponding vcr for a given lpid */
            /* FIXME: Error */
            return 1;
            /* MPID_VCR_Dup( ???, &newcomm_ptr->vcr[i] ); */
        }
      }

    }
fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/*
 * The following is a very simple code for looping through
 * the GPIDs.  Note that this code requires that all processes
 * have information on the process groups.
 */
int MPID_GPID_ToLpidArray( int size, int gpid[], int lpid[] )
{
    int i, mpi_errno = MPI_SUCCESS;
    int pgid;
    MPIDI_PG_t *pg = 0;
    MPIDI_PG_iterator iter;

    if(mpidi_dynamic_tasking) {
      for (i=0; i<size; i++) {
        MPIDI_PG_Get_iterator(&iter);
	do {
	    MPIDI_PG_Get_next( &iter, &pg );
	    if (!pg) {
		/* Internal error.  This gpid is unknown on this process */
		TRACE_ERR("No matching pg foung for id = %d\n", pgid );
		lpid[i] = -1;
		/*MPIU_ERR_SET2(mpi_errno,MPI_ERR_INTERN, "**unknowngpid",
			      "**unknowngpid %d %d", gpid[0], gpid[1] ); */
		return mpi_errno;
	    }
	    MPIDI_PG_IdToNum( pg, &pgid );

	    if (pgid == gpid[0]) {
		/* found the process group.  gpid[1] is the rank in
		   this process group */
		/* Sanity check on size */
		TRACE_ERR("found the progress group for id = %d\n", pgid );
		TRACE_ERR("pg->size = %d gpid[1]=%d\n", pg->size, gpid[1] );
		if (pg->size > gpid[1]) {
		    TRACE_ERR("pg->vct[gpid[1]].taskid = %d\n", pg->vct[gpid[1]].taskid );
		    lpid[i] = pg->vct[gpid[1]].taskid;
		}
		else {
		    lpid[i] = -1;
		    /*MPIU_ERR_SET2(mpi_errno,MPI_ERR_INTERN, "**unknowngpid",
				  "**unknowngpid %d %d", gpid[0], gpid[1] ); */
		    return mpi_errno;
		}
		/* printf( "lpid[%d] = %d for gpid = (%d)%d\n", i, lpid[i],
		   gpid[0], gpid[1] ); */
		break;
	    }
	} while (1);
	gpid += 2;
      }
    } else {
      for (i=0; i<size; i++) {
        lpid[i] = *++gpid;  gpid++;
      }
      return 0;

    }

    return mpi_errno;
}
/*
 * The following routines convert to/from the global pids, which are
 * represented as pairs of ints (process group id, rank in that process group)
 */

/* FIXME: These routines belong in a different place */
int MPID_GPID_GetAllInComm( MPID_Comm *comm_ptr, int local_size,
			    int local_gpids[], int *singlePG )
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int *gpid = local_gpids;
    int lastPGID = -1, pgid;
    MPID_VCR vc;

    MPIU_Assert(comm_ptr->local_size == local_size);

    if(mpidi_dynamic_tasking) {
      *singlePG = 1;
      for (i=0; i<comm_ptr->local_size; i++) {
	  vc = comm_ptr->vcr[i];

	  /* Get the process group id as an int */
	  MPIDI_PG_IdToNum( vc->pg, &pgid );

	  *gpid++ = pgid;
	  if (lastPGID != pgid) {
	      if (lastPGID != -1)
                 *singlePG = 0;
	      lastPGID = pgid;
	  }
	  *gpid++ = vc->pg_rank;

          MPIU_DBG_MSG_FMT(COMM,VERBOSE, (MPIU_DBG_FDEST,
                           "pgid=%d vc->pg_rank=%d",
                           pgid, vc->pg_rank));
      }
    } else {
      for (i=0; i<comm_ptr->local_size; i++) {
        *gpid++ = 0;
        (void)MPID_VCR_Get_lpid( comm_ptr->vcr[i], gpid );
        gpid++;
      }
      *singlePG = 1;
    }

    return mpi_errno;
}


int MPIDI_VC_Init( MPID_VCR vcr, MPIDI_PG_t *pg, int rank )
{
    vcr->pg      = pg;
    vcr->pg_rank = rank;
}


#endif
