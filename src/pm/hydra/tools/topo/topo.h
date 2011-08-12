/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef TOPO_H_INCLUDED
#define TOPO_H_INCLUDED

/** @file topo.h */

#include "hydra.h"

/*! \cond */

#define HYDT_TOPO_OBJ_CHILD_ID(obj) \
    ((((char *) obj) - ((char *) obj->parent->children)) / sizeof(struct HYDT_topo_obj))

/* This should be higher than the number of cache levels we support */
#define HYDT_INVALID_CACHE_DEPTH (10)

/*! \endcond */

/*! \addtogroup topo Process Topology Interface
 * @{
 */

/**
 * \brief HYDT_topo_support_level_t
 *
 * Level of support offered by the topology library.
 */
typedef enum {
    /** \brief No support is provided */
    HYDT_TOPO_SUPPORT_NONE = 0,

    /** \brief Provides information on number of processing elements
     * in the machine */
    HYDT_TOPO_SUPPORT_BASIC,

    /** \brief Provides information about the topology of the
     * processing elements (nodes, processors, cores, threads). */
    HYDT_TOPO_SUPPORT_CPUTOPO,

    /** \brief Provides information about the topology of the
     * processing elements (nodes, processors, cores, threads) as well
     * as the memory hierarchy (caches, memory). */
    HYDT_TOPO_SUPPORT_MEMTOPO
} HYDT_topo_support_level_t;


/**
 * \brief HYDT_topo_obj_type_t
 *
 * Type of object in the hardware topology map.
 */
typedef enum {
    /** \brief Cache coherent set of processors */
    HYDT_TOPO_OBJ_MACHINE = 0,

    /** \brief Sockets sharing memory dimms */
    HYDT_TOPO_OBJ_NODE,

    /** \brief A physical socket, possibly comprising of many cores */
    HYDT_TOPO_OBJ_SOCKET,

    /** \brief A core, possibly comprising of many hardware threads */
    HYDT_TOPO_OBJ_CORE,

    /** \brief A hardware thread */
    HYDT_TOPO_OBJ_THREAD,

    /** \brief Marker for the last element in the enum */
    HYDT_TOPO_OBJ_END
} HYDT_topo_obj_type_t;

#define HYDT_TOPO_MAX_CPU_COUNT (16384)
#define HYDT_BITS_PER_LONG (8 * SIZEOF_UNSIGNED_LONG)

#if (HYDT_TOPO_MAX_CPU_COUNT < HYDT_BITS_PER_LONG)
#error "Too small a CPU count"
#endif /* (HYDT_MAX_CPU_COUNT < HYDT_BITS_PER_LONG) */

struct HYDT_topo_cpuset_t {
    unsigned long set[HYDT_TOPO_MAX_CPU_COUNT / HYDT_BITS_PER_LONG];
};


/**
 * \brief An object in the hardware topology map
 *
 * The overall topology is represented as a hierarchical structure of
 * objects. A machine object contains several node objects (which are
 * collections of sockets that share a memory region). Each node
 * object contains several sockets. Each socket contains several
 * cores. Each core contains several threads.
 */
struct HYDT_topo_obj {
    /** \brief Object type */
    HYDT_topo_obj_type_t type;

    /** \brief OS index set of this object type */
    struct HYDT_topo_cpuset_t cpuset;

    /** \brief Parent object of which this is a part */
    struct HYDT_topo_obj *parent;

    /** \brief Number of children objects */
    int num_children;

    /** \brief Array of children objects */
    struct HYDT_topo_obj *children;

    /** \brief Memory object attached to this topology object */
    struct HYDT_topo_mem_obj {
        /** \brief Local memory */
        size_t local_mem_size;

        /** \brief Number of caches */
        int num_caches;

        /** \brief Size of cache shared by this object's elements */
        size_t *cache_size;

        /** \brief Depth of the highest level cache in this object */
        int *cache_depth;
    } mem;
};


/**
 * \brief Topology information
 *
 * Contains private persistent information stored by the topology
 * library.
 */
struct HYDT_topo_info {
    /** \brief Support level provided by the topology library */
    HYDT_topo_support_level_t support_level;

    /** \brief Topology library to use */
    char *topolib;

    /** \brief Ordered OS index map */
    struct HYDT_topo_cpuset_t *bindmap;

    /** \brief Total processing units available on the machine. This
     * is needed for all supported topology levels, except "NONE" */
    int total_proc_units;

    /** \brief Top-level topology object */
    struct HYDT_topo_obj machine;
};

/*! \cond */
extern struct HYDT_topo_info HYDT_topo_info;
/*! \endcond */

/**
 * \brief HYDT_topo_init - Initialize the topology library
 *
 * \param[in]  binding   Binding pattern to use
 * \param[in]  topolib   Topology library to use
 *
 * This function initializes the topology library requested by the
 * user. It also queries for the support provided by the library and
 * stores it for future calls.
 */
HYD_status HYDT_topo_init(char *binding, char *topolib);


/**
 * \brief HYDT_topo_finalize - Finalize the topology library
 *
 * This function cleans up any relevant state that the topology library
 * maintained.
 */
HYD_status HYDT_topo_finalize(void);


/**
 * \brief HYDT_topo_get_topomap - Get the topology map
 *
 * This function returns the topology map as a string.
 */
HYD_status HYDT_topo_get_topomap(char **topomap);


/**
 * \brief HYDT_topo_get_processmap - Get the process binding map
 *
 * This function returns the process binding map as a string.
 */
HYD_status HYDT_topo_get_processmap(char **processmap);


/**
 * \brief HYDT_topo_bind - Bind process to a processing element
 *
 * \param[in] cpuset  The Operating System index set to bind the process to
 *
 * This function binds a process to an appropriate OS index set. If
 * the OS index does not contain any set OS index, no binding is done.
 */
HYD_status HYDT_topo_bind(struct HYDT_topo_cpuset_t cpuset);


/**
 * \brief HYDT_topo_pid_to_cpuset - Get the OS index set for a process ID
 *
 * \param[in] process_id   The process index for which we need the OS index
 * \param[out] cpuset      The OS index set
 *
 * This function looks up the appropriate OS indices (by wrapping
 * around in cases where the process_id is larger than the number of
 * available processing units).
 */
void HYDT_topo_pid_to_cpuset(int process_id, struct HYDT_topo_cpuset_t *cpuset);


static inline void HYDT_topo_cpuset_zero(struct HYDT_topo_cpuset_t *cpuset)
{
    int i;

    for (i = 0; i < HYDT_TOPO_MAX_CPU_COUNT / HYDT_BITS_PER_LONG; i++)
        cpuset->set[i] = 0;
}

static inline void HYDT_topo_cpuset_clr(int os_index, struct HYDT_topo_cpuset_t *cpuset)
{
    int idx;
    unsigned long mask;

    idx = (os_index / HYDT_BITS_PER_LONG);
    mask = ~(1 << (os_index % HYDT_BITS_PER_LONG));

    cpuset->set[idx] &= mask;
}

static inline void HYDT_topo_cpuset_set(int os_index, struct HYDT_topo_cpuset_t *cpuset)
{
    int idx;
    unsigned long mask;

    idx = (os_index / HYDT_BITS_PER_LONG);
    mask = (1 << (os_index % HYDT_BITS_PER_LONG));

    cpuset->set[idx] |= mask;
}

static inline int HYDT_topo_cpuset_isset(int os_index, struct HYDT_topo_cpuset_t cpuset)
{
    int idx;
    unsigned long mask;

    idx = (os_index / HYDT_BITS_PER_LONG);
    mask = (1 << (os_index % HYDT_BITS_PER_LONG));

    return (cpuset.set[idx] & mask);
}

static inline void HYDT_topo_cpuset_dup(struct HYDT_topo_cpuset_t src,
                                        struct HYDT_topo_cpuset_t *dest)
{
    int i;

    for (i = 0; i < HYDT_TOPO_MAX_CPU_COUNT / HYDT_BITS_PER_LONG; i++)
        dest->set[i] = src.set[i];
}

static inline void HYDT_topo_init_obj(struct HYDT_topo_obj *obj)
{
    obj->type = HYDT_TOPO_OBJ_END;
    HYDT_topo_cpuset_zero(&obj->cpuset);
    obj->num_children = 0;
    obj->children = NULL;
    obj->mem.local_mem_size = 0;
    obj->mem.num_caches = 0;
    obj->mem.cache_size = NULL;
    obj->mem.cache_depth = NULL;
}

static inline HYD_status HYDT_topo_alloc_objs(int nobjs, struct HYDT_topo_obj **obj_list)
{
    int i;
    HYD_status status = HYD_SUCCESS;

    HYDU_MALLOC((*obj_list), struct HYDT_topo_obj *, nobjs * sizeof(struct HYDT_topo_obj),
                status);

    for (i = 0; i < nobjs; i++)
        HYDT_topo_init_obj(&(*obj_list)[i]);

  fn_exit:
    HYDU_FUNC_EXIT();
    return status;

  fn_fail:
    goto fn_exit;
}

/*!
 * @}
 */

#endif /* TOPO_H_INCLUDED */
