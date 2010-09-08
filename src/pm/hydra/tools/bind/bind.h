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

#define HYDT_OBJ_CHILD_ID(obj) \
    ((((char *) obj) - ((char *) obj->parent->children)) / sizeof(struct HYDT_topo_obj))

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
    HYDT_BIND_NONE = 0,

    /** \brief Provides information on number of processing elements
     * in the machine, and allows for process binding */
    HYDT_BIND_BASIC,

    /** \brief Provides information about the topology of the
     * processing elements (nodes, processors, cores, threads). */
    HYDT_BIND_CPU,

    /** \brief Provides information about the topology of the
     * processing elements (nodes, processors, cores, threads) as well
     * as the memory hierarchy (caches, memory). */
    HYDT_BIND_MEM
} HYDT_bind_support_level_t;


/**
 * \brief HYDT_topo_obj_type_t
 *
 * Type of object in the hardware topology map.
 */
typedef enum {
    /** \brief Cache coherent set of processors */
    HYDT_OBJ_MACHINE = 0,

    /** \brief Sockets sharing memory dimms */
    HYDT_OBJ_NODE,

    /** \brief A physical socket, possibly comprising of many cores */
    HYDT_OBJ_SOCKET,

    /** \brief A core, possibly comprising of many hardware threads */
    HYDT_OBJ_CORE,

    /** \brief A hardware thread */
    HYDT_OBJ_THREAD,

    /** \brief Marker for the last element in the enum */
    HYDT_OBJ_END
} HYDT_topo_obj_type_t;


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

    /** \brief OS index of this object type (-1 if this type has
     * children) */
    int os_index;

    /** \brief Parent object of which this is a part */
    struct HYDT_topo_obj *parent;

    /** \brief Number of children objects */
    int num_children;

    /** \brief Array of children objects */
    struct HYDT_topo_obj *children;

    /** \brief Local memory */
    size_t local_mem_size;

    /** \brief Size of cache shared by this object's elements */
    size_t cache_size;

    /** \brief Depth of the highest level cache in this object */
    int cache_depth;
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
    int *bindmap;

    /** \brief Total processing units available on the machine. This
     * is needed for all binding levels, except "NONE" */
    int total_proc_units;

    /** \brief Top-level topology object */
    struct HYDT_topo_obj machine;
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
 * \param[in] os_index  The Operating System index to bind the process to
 *
 * This function binds a process to an appropriate OS index. If the OS
 * index is negative, no binding needs to be performed.
 */
HYD_status HYDT_bind_process(int os_index);


/**
 * \brief HYDT_bind_get_os_index - Get the OS index for a process ID
 *
 * \param[in] process_id   The process index for which we need the OS index
 *
 * This function looks up the appropriate OS index (by wrapping around
 * in cases where the process_id is larger than the number of
 * available processing units).
 */
int HYDT_bind_get_os_index(int process_id);

/*!
 * @}
 */

#endif /* BIND_H_INCLUDED */
