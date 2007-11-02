/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

int mpig_topology_depths_keyval = MPI_KEYVAL_INVALID;
int mpig_topology_colors_keyval = MPI_KEYVAL_INVALID;


MPIG_STATIC int mpig_topology_destroy_depths_attr(MPI_Comm comm, int keyval, void * attr, void * state);

MPIG_STATIC int mpig_topology_destroy_colors_attr(MPI_Comm comm, int keyval, void * attr, void * state);

MPIG_STATIC int mpig_topology_get_vc_match(const mpig_vc_t * vc1, const mpig_vc_t * vc2, int level, bool_t * match);

/*
 * <mpi_errno> mpig_topology_init(void)
 *
 * Parameters: (none)
 *
 * Returns: a MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_topology_init
int mpig_topology_init(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_topology_init);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_topology_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "entering"));

    MPIR_Nest_incr();
    
    /* create the toplogy attribute keys */
    mpi_errno = NMPI_Comm_create_keyval(MPI_NULL_COPY_FN, mpig_topology_destroy_depths_attr, &mpig_topology_depths_keyval, NULL);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|comm_create_key", "**globus|comm_create_key %s",
	"topology depths keyval");
    mpi_errno = NMPI_Comm_create_keyval(MPI_NULL_COPY_FN, mpig_topology_destroy_colors_attr, &mpig_topology_colors_keyval, NULL);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|comm_create_key", "**globus|comm_create_key %s",
	"topology colors keyval");
    
  fn_return:
    MPIR_Nest_decr();
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_topology_init);
    return mpi_errno;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	if (mpig_topology_depths_keyval != MPI_KEYVAL_INVALID) NMPI_Comm_free_keyval(&mpig_topology_depths_keyval);
	if (mpig_topology_colors_keyval != MPI_KEYVAL_INVALID) NMPI_Comm_free_keyval(&mpig_topology_colors_keyval);
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_topology_init() */


/*
 * <mpi_errno> mpig_topology_finalize([IN/MOD] comm)
 *
 * Parameters: (none)
 *
 * Returns: a MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_topology_finalize
int mpig_topology_finalize(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_topology_finalize);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_topology_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "entering"));

    MPIR_Nest_incr();
    
    /* destroy the toplogy attribute keys */
    mpi_errno = NMPI_Comm_free_keyval(&mpig_topology_depths_keyval);
    MPIU_ERR_CHKANDSTMT1((mpi_errno), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|comm_destroy_key", "**globus|comm_destroy_key %s",
	"topology depths keyval");
    NMPI_Comm_free_keyval(&mpig_topology_colors_keyval);
    MPIU_ERR_CHKANDSTMT1((mpi_errno), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|comm_destroy_key", "**globus|comm_destroy_key %s",
	"topology colors keyval");

    /* fn_return: */
    MPIR_Nest_decr();
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_topology_finalize);
    return mpi_errno;
}
/* mpig_topology_finalize() */


/*
 * <mpi_errno> mpig_topology_comm_construct([IN/MOD] comm)
 *
 * Parameters:
 *
 *   comm [IN/MOD] - communicator being created
 *
 * Returns: a MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_topology_comm_construct
int mpig_topology_comm_construct(MPID_Comm * const comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int size = comm->remote_size;
    const int my_rank = comm->rank;
    int * depths = NULL;
    int ** colors = NULL;
    int ** ranks = NULL;
    int ** cluster_ids = NULL;
    int ** cluster_sizes = NULL;
    mpig_comm_set_t * comm_sets = NULL;
    int * depths_attr_copy = NULL;
    int ** colors_attr_copy = NULL;
    int p0;
    int level;
    int max_depth;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_topology_comm_construct);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_topology_comm_construct);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
	"entering: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, MPIG_PTR_CAST(comm)));

    MPIR_Nest_incr();
    
    /* topology information is only created for intracommunicators, at least for now */
    if (comm->comm_kind == MPID_INTERCOMM) goto fn_return;

    /* allocate memory for copies of the depths and colors arrays */
    depths = (int *) MPIU_Malloc(size * sizeof(int));
    MPIU_ERR_CHKANDJUMP1((depths == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "topology depths array");

    colors = (int **) MPIU_Malloc(size * sizeof(int *));
    MPIU_ERR_CHKANDJUMP1((colors == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "topology colors array");
    memset(colors, 0, size * sizeof(int *));
    
    ranks = (int **) MPIU_Malloc(size * sizeof(int *));
    MPIU_ERR_CHKANDJUMP1((ranks == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "topology ranks array");
    memset(ranks, 0, size * sizeof(int *));
    
    cluster_ids = (int **) MPIU_Malloc(size * sizeof(int *));
    MPIU_ERR_CHKANDJUMP1((cluster_ids == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "topology cluster ids array");
    memset(cluster_ids, 0, size * sizeof(int *));
    
    /*
     * phase 1 of 3 - finding depths
     */
    max_depth = 0;
    for (p0 = 0; p0 < size; p0++)
    {
	mpig_vc_t * const vc = mpig_comm_get_remote_vc(comm, p0);
	
	depths[p0] = mpig_vc_get_num_topology_levels(vc);

	if (depths[p0] > max_depth)
	{
	    max_depth = depths[p0];
	}

	/* allocate memory for the colors, ranks, and cluster ids */
	colors[p0] = (int *) MPIU_Malloc(depths[p0] * sizeof(int));
	MPIU_ERR_CHKANDJUMP1((colors[p0] == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "topology colors subarray");

	ranks[p0] = (int *) MPIU_Malloc(depths[p0] * sizeof(int));
	MPIU_ERR_CHKANDJUMP1((ranks[p0] == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "topology ranks subarray");

	cluster_ids[p0] = (int *) MPIU_Malloc(depths[p0] * sizeof(int));
	MPIU_ERR_CHKANDJUMP1((cluster_ids[p0] == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
	    "topology cluster ids subarray");

	/* initialize the colors and cluster IDs to invalid value of MPI_UNDEFINED */
	for (level = 0; level < depths[p0]; level ++)
	{
	    colors[p0][level] = MPI_UNDEFINED;
	    cluster_ids[p0][level] = MPI_UNDEFINED;
	}
    }
    /* end for (p0 = 0; p0 < size; p0++) */

    /* allocate memory for the sizes of the clusters that the local process belongs to at each communication level */
    cluster_sizes = (int **) MPIU_Malloc(max_depth * sizeof(int *));
    MPIU_ERR_CHKANDJUMP1((cluster_sizes == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
	"topology cluster sizes array");
    memset(cluster_sizes, 0, max_depth * sizeof(int *));

    /* allocate memory for the sets of communicating processes that the local process will be involved in */
    comm_sets = (mpig_comm_set_t *) MPIU_Malloc(depths[my_rank] * sizeof(mpig_comm_set_t));
    MPIU_ERR_CHKANDJUMP1((comm_sets == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
	"topology array containing sets of communicating processes");
    for (level = 0; level < depths[my_rank]; level++)
    {
	comm_sets[level].set = NULL;
    }
    
    /*
     * phase 2 of 3 - coloring
     */
    for (level = 0; level < max_depth; level++)
    {
	int next_color = 0;

	for (p0 = 0; p0 < size; p0++)
	{
	    int rank;
	    int p1;
	  
	    if (level < depths[p0])
	    {
		if (colors[p0][level] == MPI_UNDEFINED)
		{
		    const mpig_vc_t * const vc0 = mpig_comm_get_remote_vc(comm, p0);
	       
		    /* this proc has not been colored at this level yet, i.e., it hasn't matched any of the procs to the left at
		       this level yet ... ok, start new color at this level. */
	       
		    colors[p0][level] = next_color++;
		    for (p1 = p0 + 1; p1 < size; p1++)
		    {
			if (level < depths[p1] && colors[p1][level] == MPI_UNDEFINED)
			{
			    const mpig_vc_t * const vc1 = mpig_comm_get_remote_vc(comm, p1);
			    bool_t match;
			
			    mpi_errno = mpig_topology_get_vc_match(vc0, vc1, level, &match);
			    if (mpi_errno) goto fn_fail;
			    if (match)
			    {
				colors[p1][level] = colors[p0][level];
			    }
			}
		    }
		}
		/* end if (colors[p0][level] == MPI_UNDEFINED) */

		/* determine the rank of this process inside its cluster */
		rank = 0;
		for (p1 = 0; p1 < p0; p1++)
		{
		    if (level < depths[p1] && colors[p1][level] == colors[p0][level])
		    {
			rank += 1;
		    }
		}
	   
		ranks[p0][level] = rank;
	    }
	    /* end if (level < depths[p0]) */
	}
	/* end for (p0 = 0; p0 < size; p0++) */

	/* allocate space for the array to hold the cluster sizes for the current level */
	cluster_sizes[level] = (int *) MPIU_Malloc(next_color * sizeof(int));
	MPIU_ERR_CHKANDJUMP1((cluster_sizes[level] == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
	    "topology cluster sizes subarray");

	/* determine the sizes of the clusters at this level */
	for (p0 = 0; p0 < next_color; p0++)
	{
	    cluster_sizes[level][p0] = 0;
	}
       
	for (p0 = 0; p0 < size; p0++)
	{
	    if (depths[p0] > level)
	    {
		cluster_sizes[level][colors[p0][level]] += 1;
	    }
	}
    }
    /* end for (level = 0; level < max_depth; level++) */

    /*
     * phase 3 of 3 - setting CID's based on colors
     */
    for (level = max_depth - 1; level >= 0; level--)
    {
	for (p0 = 0; p0 < size; p0++)
	{
	    if (level < depths[p0] && cluster_ids[p0][level] == -1)
	    {
		/*
		 * p0 has not been assigned a cid at this level yet, which means all the procs at this level that have the same
		 * color as p0 at this level have also not been assigned cid's yet.
		 *
		 * find the rest of the procs at this level that have the same color as p0 and assign them cids.  same color are
		 * enumerated.
		 */
		int next_cid;
		int p1;

		cluster_ids[p0][level] = 0;
		next_cid = 1;

		for (p1 = p0 + 1; p1 < size; p1++)
		{
		    if (level < depths[p1] && colors[p0][level] == colors[p1][level])
		    {
			/*
			 * p0 and p1 match colors at this level, which means p1 will now get its cid set at this level.  but to
			 * what value?  if p1 also matches color with any proc to its left at level level+1, then p1 copies that
			 * proc's cid at this level, otherwise p1 gets the next cid value at this level.
			 */
			if (level + 1 < depths[p1])
			{
			    int p2;

			    for (p2 = 0; p2 < p1; p2 ++)
			    {
				if (level + 1 < depths[p2] && colors[p1][level] == colors[p2][level] &&
				    colors[p1][level + 1] == colors[p2][level + 1])
				{
				    cluster_ids[p1][level] = cluster_ids[p2][level];
				    break; /* for p2 */
				}
			    }
			    
			    if (p2 == p1)
			    {
				/* did not find one */
				cluster_ids[p1][level] = next_cid++;
			    }
			}
			else /* if (level + 1 >= depths[p1]) */
			{
			    /* p1 does not have a level level+1 */
			    cluster_ids[p1][level] = next_cid ++;
			}
			/* end else if (level + 1 >= depths[p1]) */
		    }
		    /* end if (level < depths[p1] && colors[p0][level] == colors[p1][level]) */
		}
		/* end for (p1 = p0 + 1; p1 < size; p1++) */
	    }
	    /* end if (level < depths[p0] && cluster_ids[p0][level] == -1) */
	}
	/* end for (p0 = 0; p0 < size; p0++) */
    }
    /* end for (level = max_depth - 1; level >= 0; level--) */

    /* allocate enough memory for the sets of communicating processes at each level */
    for (level = 0; level < depths[my_rank]; level++)
    {
	/* number of procs I may have to talk to at this level */
	int n_cid = 0;

	for (p0 = 0; p0 < size; p0++)
	{
	    if (depths[p0] > level && colors[my_rank][level] == colors[p0][level] && cluster_ids[p0][level] > n_cid)
	    {
		n_cid = cluster_ids[p0][level];
	    }
	}

	comm_sets[level].set = (int *) MPIU_Malloc((n_cid + 1) * sizeof(int));
	MPIU_ERR_CHKANDJUMP1((comm_sets[level].set == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
	    "topology subarray containing a set of communication processes");
    }
    /* end for (level = 0; level < depths[my_rank]; level++) */
   
    /* allocate memory for copies of the depths and colors arrays */
    depths_attr_copy = (int *) MPIU_Malloc(size * sizeof(int));
    MPIU_ERR_CHKANDJUMP1((depths_attr_copy == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
	"duplicate of topology depths array for communicator attributes");

    colors_attr_copy = (int **) MPIU_Malloc(size * sizeof(int *));
    MPIU_ERR_CHKANDJUMP1((depths_attr_copy == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
	"duplicate of topology colors array for communicator attributes");
    memset(colors_attr_copy, 0, size * sizeof(int *));
    
    for (p0 = 0; p0 < size; p0++)
    {
	depths_attr_copy[p0] = depths[p0];
	
	colors_attr_copy[p0] = (int *) MPIU_Malloc(depths[p0] * sizeof(int));
	MPIU_ERR_CHKANDJUMP1((colors_attr_copy[p0] == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
	    "duplicate of topology copy subarray for communicator attributes");

	for (level = 0; level < depths[p0]; level++)
	{
	    colors_attr_copy[p0][level] = colors[p0][level];
	}
    }

    /* attach a the copies of the topology depths and collors information to the communicator using the attribute keys */
    mpi_errno = NMPI_Comm_set_attr(comm->handle, mpig_topology_colors_keyval, colors_attr_copy);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|comm_set_attr", "**globus|comm_set_attr %s",
	"topology colors attribute");
    mpi_errno = NMPI_Comm_set_attr(comm->handle, mpig_topology_depths_keyval, depths_attr_copy);
    if (mpi_errno)
    {
	NMPI_Comm_delete_attr(comm->handle, mpig_topology_colors_keyval);
	colors_attr_copy = NULL;
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|comm_set_attr", "**globus|comm_set_attr %s",
	    "topology depths attribute");
    }

    /* set pointers in the communcator object */
    comm->dev.topology_colors = colors;
    comm->dev.topology_depths = depths;
    comm->dev.topology_ranks = ranks;
    comm->dev.topology_cluster_ids = cluster_ids;
    comm->dev.topology_cluster_sizes = cluster_sizes;
    comm->dev.topology_comm_sets = comm_sets;
    comm->dev.topology_max_depth = max_depth;

  fn_return:
    MPIR_Nest_decr();
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT
	",mpi_errno=" MPIG_ERRNO_FMT, comm->handle, MPIG_PTR_CAST(comm), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_topology_comm_construct);
    return mpi_errno;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	if (colors_attr_copy)
	{
	    for (p0 = 0; p0 < size; p0++)
	    {
		if (colors_attr_copy[p0]) MPIU_Free(colors_attr_copy[p0]);
	    }
	    MPIU_Free(colors_attr_copy);
	}
	if (depths_attr_copy) MPIU_Free(depths_attr_copy);
		    
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_topology_comm_construct() */


/*
 * <mpi_errno> mpig_topology_comm_destruct([IN/MOD] comm)
 *
 * Parameters:
 *
 *   comm [IN/MOD] - communicator being destroyed
 *
 * Returns: a MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_topology_comm_destruct
int mpig_topology_comm_destruct(MPID_Comm * const comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int size = comm->remote_size;
    const int my_rank = comm->rank;
    int p;
    int level;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_topology_comm_destruct);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_topology_comm_destruct);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
	"entering: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, MPIG_PTR_CAST(comm)));

    /* topology information is only created for intracommunicators, at least for now */
    if (comm->comm_kind == MPID_INTERCOMM) goto fn_return;

    /* free all of the toploogy information arrays attach to the communicator */
    for (level = 0; level < comm->dev.topology_depths[my_rank]; level++)
    {
	MPIU_Free(comm->dev.topology_comm_sets[level].set);
    }
    MPIU_Free(comm->dev.topology_comm_sets);
    comm->dev.topology_comm_sets = MPIG_INVALID_PTR;
    
    for (level = 0; level < comm->dev.topology_max_depth; level++)
    {
	MPIU_Free(comm->dev.topology_cluster_sizes[level]);
    }
    MPIU_Free(comm->dev.topology_cluster_sizes);
    comm->dev.topology_cluster_sizes = MPIG_INVALID_PTR;

    for (p = 0; p < size; p++)
    {
	MPIU_Free(comm->dev.topology_cluster_ids[p]);
    }
    MPIU_Free(comm->dev.topology_cluster_ids);
    comm->dev.topology_cluster_ids = MPIG_INVALID_PTR;

    for (p = 0; p < size; p++)
    {
	MPIU_Free(comm->dev.topology_ranks[p]);
    }
    MPIU_Free(comm->dev.topology_ranks);
    comm->dev.topology_ranks = MPIG_INVALID_PTR;

    for (p = 0; p < size; p++)
    {
	MPIU_Free(comm->dev.topology_colors[p]);
    }
    MPIU_Free(comm->dev.topology_colors);
    comm->dev.topology_colors = MPIG_INVALID_PTR;

    MPIU_Free(comm->dev.topology_depths);
    comm->dev.topology_depths = MPIG_INVALID_PTR;

    comm->dev.topology_max_depth = -1;

    /* NOTE: the copy of the topology depths and colors arrays, which are attached as attributes to the communicator, will be
       destroyed by the attribute destructor callbacks */

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT
	",mpi_errno=" MPIG_ERRNO_FMT, comm->handle, MPIG_PTR_CAST(comm), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_topology_comm_destruct);
    return mpi_errno;
}
/* mpig_topology_comm_destruct() */


/*
 * mpig_topology_get_vc_match([IN] vc1, [IN] vc2, [IN] topology_level, [OUT] matched)
 *
 * Returns: an MPI error code
 */
#undef FUNCNAME
#define FUNCNAME mpig_topology_get_vc_match
MPIG_STATIC int mpig_topology_get_vc_match(const mpig_vc_t * const vc1, const mpig_vc_t * const vc2, const int level,
    bool_t * const match_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int n;
    unsigned levels_out;
    bool_t match = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_topology_get_vc_match);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_topology_get_vc_match);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_TOPO, "entering: vc1=" MPIG_PTR_FMT ", vc2=" MPIG_PTR_FMT
	", level=%d", MPIG_PTR_CAST(vc1), MPIG_PTR_CAST(vc2), level));

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_TOPO, "level_mask=0x%02x, vc1_levels=0x%02x, vc2_levels=0x%02x, result=0x%02x",
	((unsigned) 1 << level), mpig_vc_get_topology_levels(vc1), mpig_vc_get_topology_levels(vc2),
	(((unsigned) 1 << level) & mpig_vc_get_topology_levels(vc1) & mpig_vc_get_topology_levels(vc2))));
    if (((unsigned) 1 << level) & mpig_vc_get_topology_levels(vc1) & mpig_vc_get_topology_levels(vc2))
    {
	for (n = 0; n < mpig_cm_table_num_entries; n++)
	{
	    if (mpig_cm_get_vtable(mpig_cm_table[n])->get_vc_compatability != NULL)
	    {
		mpi_errno = mpig_cm_get_vtable(mpig_cm_table[n])->get_vc_compatability(mpig_cm_table[n], vc1, vc2,
		    ((unsigned) 1 << level), &levels_out);
		MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|get_vc_compatability",
		    "**globus|get_vc_compatability %s", mpig_cm_get_name(mpig_cm_table[n]));
		
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_TOPO, "cm=%s, level_mask=0x%02x, levels_out=0x%02x",
		    mpig_cm_get_name(mpig_cm_table[n]), ((unsigned) 1 << level), levels_out));
		    
		if (levels_out)
		{
		    match = TRUE;
		    break; /* for n */
		}
	    }
	}
    }

    *match_p = match;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_TOPO, "exiting: vc1=" MPIG_PTR_FMT ", vc2=" MPIG_PTR_FMT
	", match=%s", MPIG_PTR_CAST(vc1), MPIG_PTR_CAST(vc2), MPIG_BOOL_STR(*match_p)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_topology_get_vc_match);
    return mpi_errno;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_topology_get_vc_match() */


/*
 * <mpi_errno> mpig_topology_destroy_depths_attr(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_topology_destroy_depths_attr
MPIG_STATIC int mpig_topology_destroy_depths_attr(MPI_Comm comm_handle, int keyval, void * attr, void * state)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int * const depths = (int *) attr;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_topology_destroy_depths_attr);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_topology_destroy_depths_attr);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM | MPIG_DEBUG_LEVEL_TOPO, "entering: comm=" MPIG_HANDLE_FMT
	", keyval=" MPIG_HANDLE_FMT ", depths=" MPIG_PTR_FMT, comm_handle, keyval, MPIG_PTR_CAST(keyval)));

    MPIU_Free(depths);
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM | MPIG_DEBUG_LEVEL_TOPO, "exiting: comm=" MPIG_HANDLE_FMT
	", keyval=" MPIG_HANDLE_FMT ", depths=" MPIG_PTR_FMT, comm_handle, keyval, MPIG_PTR_CAST(keyval)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_topology_destroy_depths_attr);
    return mpi_errno;
}
/* mpig_topology_destroy_depths_attr() */


/*
 * <mpi_errno> mpig_topology_destroy_colors_attr(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_topology_destroy_colors_attr
MPIG_STATIC int mpig_topology_destroy_colors_attr(MPI_Comm comm_handle, int keyval, void * attr, void * state)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int ** const colors = (int **) attr;
    MPID_Comm * comm;
    int p;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_topology_destroy_colors_attr);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_topology_destroy_colors_attr);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM | MPIG_DEBUG_LEVEL_TOPO, "entering: comm=" MPIG_HANDLE_FMT
	", keyval=" MPIG_HANDLE_FMT ", colors=" MPIG_PTR_FMT, comm_handle, keyval, MPIG_PTR_CAST(keyval)));

    MPID_Comm_get_ptr(comm_handle, comm);
    
    for (p = 0; p < comm->remote_size; p++)
    {
	MPIU_Free(colors[p]);
	colors[p] = MPIG_INVALID_PTR;
    }
    MPIU_Free(colors);
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM | MPIG_DEBUG_LEVEL_TOPO, "exiting: comm=" MPIG_HANDLE_FMT
	", keyval=" MPIG_HANDLE_FMT ", colors=" MPIG_PTR_FMT, comm_handle, keyval, MPIG_PTR_CAST(keyval)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_topology_destroy_colors_attr);
    return mpi_errno;
}
/* mpig_topology_destroy_colors_attr() */
