/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef BIND_H_INCLUDED
#define BIND_H_INCLUDED

/** @file bind.h */

#include "hydra_utils.h"

/*! \cond */

#define HYDT_BIND_OBJ_CHILD_ID(obj) \
    ((((char *) obj) - ((char *) obj->parent->children)) / sizeof(struct HYDT_bind_obj))

/*! \endcond */

/*! \addtogroup bind Process Binding Interface
 * @{
 */

/**
 * \brief HYDT_bind_support_level_t
 *
 * Level of binding support offered by the binding library.
 */
typedef enum {
    /** \brief No support is provided */
    HYDT_BIND_SUPPORT_NONE = 0,

    /** \brief Provides information on number of processing elements
     * in the machine, and allows for process binding */
    HYDT_BIND_SUPPORT_BASIC,

    /** \brief Provides information about the topology of the
     * processing elements (nodes, processors, cores, threads). */
    HYDT_BIND_SUPPORT_CPUTOPO,

    /** \brief Provides information about the topology of the
     * processing elements (nodes, processors, cores, threads) as well
     * as the memory hierarchy (caches, memory). */
    HYDT_BIND_SUPPORT_MEMTOPO
} HYDT_bind_support_level_t;


/**
 * \brief HYDT_bind_obj_type_t
 *
 * Type of object in the hardware topology map.
 */
typedef enum {
    /** \brief Cache coherent set of processors */
    HYDT_BIND_OBJ_MACHINE = 0,

    /** \brief Sockets sharing memory dimms */
    HYDT_BIND_OBJ_NODE,

    /** \brief A physical socket, possibly comprising of many cores */
    HYDT_BIND_OBJ_SOCKET,

    /** \brief A core, possibly comprising of many hardware threads */
    HYDT_BIND_OBJ_CORE,

    /** \brief A hardware thread */
    HYDT_BIND_OBJ_THREAD,

    /** \brief Marker for the last element in the enum */
    HYDT_BIND_OBJ_END
} HYDT_bind_obj_type_t;


#define HYDT_BIND_MAX_CPU_COUNT (16384)

#if (HYDT_BIND_MAX_CPU_COUNT < SIZEOF_UNSIGNED_LONG)
#error "Too small a CPU count"
#endif /* (HYDT_MAX_CPU_COUNT < SIZEOF_UNSIGNED_LONG) */

struct HYDT_bind_cpuset_t {
    unsigned long set[HYDT_BIND_MAX_CPU_COUNT / SIZEOF_UNSIGNED_LONG];
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
struct HYDT_bind_obj {
    /** \brief Object type */
    HYDT_bind_obj_type_t type;

    /** \brief OS index set of this object type */
    struct HYDT_bind_cpuset_t cpuset;

    /** \brief Parent object of which this is a part */
    struct HYDT_bind_obj *parent;

    /** \brief Number of children objects */
    int num_children;

    /** \brief Array of children objects */
    struct HYDT_bind_obj *children;

    /** \brief Memory object attached to this topology object */
    struct HYDT_bind_mem_obj {
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
 * \brief Binding information
 *
 * Contains private persistent information stored by the binding
 * library.
 */
struct HYDT_bind_info {
    /** \brief Support level provided by the binding library */
    HYDT_bind_support_level_t support_level;

    /** \brief Binding library to use */
    char *bindlib;

    /** \brief Ordered OS index map to bind the processes */
    struct HYDT_bind_cpuset_t *bindmap;

    /** \brief Total processing units available on the machine. This
     * is needed for all binding levels, except "NONE" */
    int total_proc_units;

    /** \brief Top-level topology object */
    struct HYDT_bind_obj machine;
};

/*! \cond */
extern struct HYDT_bind_info HYDT_bind_info;
/*! \endcond */

/**
 * \brief HYDT_bind_init - Initialize the binding library
 *
 * \param[in]  binding   Binding pattern to use
 * \param[in]  bindlib   Binding library to use
 *
 * This function initializes the binding library requested by the
 * user. It also queries for the support provided by the library and
 * stores it for future calls.
 */
HYD_status HYDT_bind_init(char *binding, char *bindlib);


/**
 * \brief HYDT_bind_finalize - Finalize the binding library
 *
 * This function cleans up any relevant state that the binding library
 * maintained.
 */
void HYDT_bind_finalize(void);


/**
 * \brief HYDT_bind_process - Bind process to a processing element
 *
 * \param[in] cpuset  The Operating System index set to bind the process to
 *
 * This function binds a process to an appropriate OS index set. If
 * the OS index does not contain any set OS index, no binding is done.
 */
HYD_status HYDT_bind_process(struct HYDT_bind_cpuset_t cpuset);


/**
 * \brief HYDT_bind_pid_to_cpuset - Get the OS index set for a process ID
 *
 * \param[in] process_id   The process index for which we need the OS index
 * \param[out] cpuset      The OS index set to which the process can bind
 *
 * This function looks up the appropriate OS indices (by wrapping
 * around in cases where the process_id is larger than the number of
 * available processing units).
 */
void HYDT_bind_pid_to_cpuset(int process_id, struct HYDT_bind_cpuset_t *cpuset);


static inline void HYDT_bind_cpuset_zero(struct HYDT_bind_cpuset_t *cpuset)
{
    int i;

    for (i = 0; i < HYDT_BIND_MAX_CPU_COUNT / SIZEOF_UNSIGNED_LONG; i++)
        cpuset->set[i] = 0;
}

static inline void HYDT_bind_cpuset_clr(int os_index, struct HYDT_bind_cpuset_t *cpuset)
{
    int idx;
    unsigned long mask;

    idx = (os_index / SIZEOF_UNSIGNED_LONG);
    mask = ~(1 << (os_index % SIZEOF_UNSIGNED_LONG));

    cpuset->set[idx] &= mask;
}

static inline void HYDT_bind_cpuset_set(int os_index, struct HYDT_bind_cpuset_t *cpuset)
{
    int idx;
    unsigned long mask;

    idx = (os_index / SIZEOF_UNSIGNED_LONG);
    mask = (1 << (os_index % SIZEOF_UNSIGNED_LONG));

    cpuset->set[idx] |= mask;
}

static inline int HYDT_bind_cpuset_isset(int os_index, struct HYDT_bind_cpuset_t cpuset)
{
    int idx;
    unsigned long mask;

    idx = (os_index / SIZEOF_UNSIGNED_LONG);
    mask = (1 << (os_index % SIZEOF_UNSIGNED_LONG));

    return (cpuset.set[idx] & mask);
}

static inline void HYDT_bind_cpuset_dup(struct HYDT_bind_cpuset_t src,
                                        struct HYDT_bind_cpuset_t *dest)
{
    int i;

    HYDT_bind_cpuset_zero(dest);
    for (i = 0; i < HYDT_BIND_MAX_CPU_COUNT; i++)
        if (HYDT_bind_cpuset_isset(i, src))
            HYDT_bind_cpuset_set(i, dest);
}

/*!
 * @}
 */

#endif /* BIND_H_INCLUDED */
