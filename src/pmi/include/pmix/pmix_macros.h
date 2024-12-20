/*
 * Copyright (c) 2013-2020 Intel, Inc.  All rights reserved.
 * Copyright (c) 2016-2019 Research Organization for Information Science
 *                         and Technology (RIST).  All rights reserved.
 * Copyright (c) 2016-2019 Mellanox Technologies, Inc.
 *                         All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer listed
 *   in this license in the documentation and/or other materials
 *   provided with the distribution.
 *
 * - Neither the name of the copyright holders nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * The copyright holders provide no reassurances that the source code
 * provided does not infringe any patent, copyright, or any other
 * intellectual property rights of third parties.  The copyright holders
 * disclaim any liability to any recipient for claims brought against
 * recipient by any third party for infringement of that parties
 * intellectual property rights.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 2020      Cisco Systems, Inc.  All rights reserved
 * Copyright (c) 2021-2022 Nanook Consulting  All rights reserved.
 * Copyright (c) 2016-2022 IBM Corporation.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/*
 * PMIx Standard macros
 */
#ifndef PMIX_MACROS_H
#define PMIX_MACROS_H

/* *******************************************************************
 * PMIx Standard types, constants, and callback functions
 * *******************************************************************/
#include "pmix_types.h"

/* *******************************************************************
 * The PMIx ABI Support functions that _do not_ rely on the macros defined
 * in this header. They provide 'static inline' functions required to
 * support these macro definitions.
 * *******************************************************************/
#include "pmix_abi_support.h"



/* define a macro for testing for valid ranks */
#define PMIX_RANK_IS_VALID(r)   \
    ((r) < PMIX_RANK_VALID)

/* define a macro for identifying system event values */
#define PMIX_SYSTEM_EVENT(a)   \
    ((a) <= PMIX_EVENT_SYS_BASE && PMIX_EVENT_SYS_OTHER <= (a))

/* declare a convenience macro for checking keys */
#define PMIX_CHECK_KEY(a, b) \
    (0 == strncmp((a)->key, (b), PMIX_MAX_KEYLEN))

/* check if the key is reserved */
#define PMIX_CHECK_RESERVED_KEY(a) \
    (0 == strncmp((a), "pmix", 4))

/* load the key into the data structure */
#define PMIX_LOAD_KEY(a, b)                                                 \
    do {                                                                    \
        memset((a), 0, PMIX_MAX_KEYLEN+1);                                  \
        if (NULL != (b)) {                                                  \
            pmixabi_strncpy((char*)(a), (const char*)(b), PMIX_MAX_KEYLEN); \
        }                                                                   \
    }while(0)

/* define a convenience macro for loading nspaces */
#define PMIX_LOAD_NSPACE(a, b)                                  \
    do {                                                        \
        memset((a), 0, PMIX_MAX_NSLEN+1);                       \
        if (NULL != (b)) {                                      \
            pmixabi_strncpy((char*)(a), (b), PMIX_MAX_NSLEN);   \
        }                                                       \
    }while(0)

/* define a convenience macro for checking nspaces */
#define PMIX_CHECK_NSPACE(a, b) \
    (PMIX_NSPACE_INVALID((a)) || PMIX_NSPACE_INVALID((b)) || 0 == strncmp((a), (b), PMIX_MAX_NSLEN))

/* define a convenience macro for loading names */
#define PMIX_LOAD_PROCID(a, b, c)               \
    do {                                        \
        PMIX_LOAD_NSPACE((a)->nspace, (b));     \
        (a)->rank = (c);                        \
    }while(0)

#define PMIX_XFER_PROCID(a, b)      \
    memcpy((a), (b), sizeof(pmix_proc_t))

#define PMIX_PROCID_XFER(a, b) PMIX_XFER_PROCID(a, b)

/* define a convenience macro for checking names */
#define PMIX_CHECK_PROCID(a, b) \
    (PMIX_CHECK_NSPACE((a)->nspace, (b)->nspace) && ((a)->rank == (b)->rank || (PMIX_RANK_WILDCARD == (a)->rank || PMIX_RANK_WILDCARD == (b)->rank)))

#define PMIX_CHECK_RANK(a, b) \
    ((a) == (b) || (PMIX_RANK_WILDCARD == (a) || PMIX_RANK_WILDCARD == (b)))

#define PMIX_NSPACE_INVALID(a) \
    (NULL == (a) || 0 == pmixabi_nslen((a)))

#define PMIX_PROCID_INVALID(a)  \
    (PMIX_NSPACE_INVALID((a)->nspace) || PMIX_RANK_INVALID == (a)->rank)

/*
 * ARGV support
 */
#define PMIX_ARGV_COUNT(r, a) \
    (r) = pmixabi_argv_count(a)

#define PMIX_ARGV_APPEND(r, a, b) \
    (r) = pmixabi_argv_append_nosize(&(a), (b))

#define PMIX_ARGV_PREPEND(r, a, b) \
    (r) = pmixabi_argv_prepend_nosize(&(a), b)

#define PMIX_ARGV_APPEND_UNIQUE(r, a, b) \
    (r) = pmixabi_argv_append_unique_nosize(a, b)

#define PMIX_ARGV_FREE(a)                       \
do {                                            \
    if (NULL != (a)) {                          \
        for (char **p = (a); NULL != *p; ++p) { \
            pmixabi_free(*p);                   \
        }                                       \
        pmixabi_free(a);                        \
    }                                           \
} while(0)

#define PMIX_ARGV_SPLIT(a, b, c) \
    (a) = pmixabi_argv_split(b, c)

#define PMIX_ARGV_JOIN(a, b, c) \
    (a) = pmixabi_argv_join(b, c)

#define PMIX_ARGV_COPY(a, b) \
    (a) = pmixabi_argv_copy(b)

/*
 * Environment support
 */
#define PMIX_SETENV(r, a, b, c) \
    (r) = pmixabi_setenv((a), (b), true, (c))

/*
 * pmix_coord_t
 */
#define PMIX_COORD_CREATE(m, d, n)                                      \
    do {                                                                \
        pmix_coord_t *_m;                                               \
        _m = (pmix_coord_t*)pmixabi_calloc((d), sizeof(pmix_coord_t));  \
        if (NULL != _m) {                                               \
            _m->view = PMIX_COORD_VIEW_UNDEF;                           \
            _m->dims = (n);                                             \
            _m->coord = (uint32_t*)pmixabi_calloc((n), sizeof(uint32_t)); \
            (m) = _m;                                                   \
        }                                                               \
    } while(0)

#define PMIX_COORD_CONSTRUCT(m)             \
    do {                                    \
        (m)->view = PMIX_COORD_VIEW_UNDEF;  \
        (m)->coord = NULL;                  \
        (m)->dims = 0;                      \
    } while(0)

#define PMIX_COORD_DESTRUCT(m)              \
    do {                                    \
        (m)->view = PMIX_COORD_VIEW_UNDEF;  \
        if (NULL != (m)->coord) {           \
            pmixabi_free((m)->coord);       \
            (m)->coord = NULL;              \
            (m)->dims = 0;                  \
        }                                   \
    } while(0)

#define PMIX_COORD_FREE(m, n)                       \
    do {                                            \
        size_t _nc_;                                \
        if (NULL != (m)) {                          \
            for (_nc_ = 0; _nc_ < (n); _nc_++) {    \
                PMIX_COORD_DESTRUCT(&(m)[_nc_]);    \
            }                                       \
            pmixabi_free((m));                      \
            (m) = NULL;                             \
        }                                           \
    } while(0)

/*
 * pmix_cpuset_t
 */
#define PMIX_CPUSET_CONSTRUCT(m) \
    memset((m), 0, sizeof(pmix_cpuset_t))

#define PMIX_CPUSET_CREATE(m, n)    \
    (m) = (pmix_cpuset_t*)pmixabi_calloc((n), sizeof(pmix_cpuset_t));

/*
 * pmix_topology_t
 */
#define PMIX_TOPOLOGY_CONSTRUCT(m) \
    memset((m), 0, sizeof(pmix_topology_t))

#define PMIX_TOPOLOGY_CREATE(m, n) \
    (m) = (pmix_topology_t*)pmixabi_calloc(n, sizeof(pmix_topology_t))

/*
 * pmix_geometry_t
 */
#define PMIX_GEOMETRY_CONSTRUCT(m) \
    memset((m), 0, sizeof(pmix_geometry_t));

#define PMIX_GEOMETRY_DESTRUCT(m)                               \
    do {                                                        \
        if (NULL != (m)->uuid) {                                \
            pmixabi_free((m)->uuid);                            \
            (m)->uuid = NULL;                                   \
        }                                                       \
        if (NULL != (m)->osname) {                              \
            pmixabi_free((m)->osname);                          \
            (m)->osname = NULL;                                 \
        }                                                       \
        if (NULL != (m)->coordinates) {                         \
            PMIX_COORD_FREE((m)->coordinates, (m)->ncoords);    \
        }                                                       \
    } while(0)

#define PMIX_GEOMETRY_CREATE(m, n)                              \
    (m) = (pmix_geometry_t*)pmixabi_calloc((n), sizeof(pmix_geometry_t))

#define PMIX_GEOMETRY_FREE(m, n)                    \
    do {                                            \
        size_t _i;                                  \
        if (NULL != (m)) {                          \
            for (_i=0; _i < (n); _i++) {            \
                PMIX_GEOMETRY_DESTRUCT(&(m)[_i]);   \
            }                                       \
            pmixabi_free((m));                      \
            (m) = NULL;                             \
        }                                           \
    } while(0)

/*
 * pmix_device_distance_t
 */
#define PMIX_DEVICE_DIST_CONSTRUCT(m)                       \
    do {                                                    \
        memset((m), 0, sizeof(pmix_device_distance_t));     \
        (m)->mindist = UINT16_MAX;                          \
        (m)->maxdist = UINT16_MAX;                          \
    } while(0);

#define PMIX_DEVICE_DIST_DESTRUCT(m)    \
    do {                                \
        if (NULL != ((m)->uuid)) {      \
            free((m)->uuid);            \
        }                               \
        if (NULL != ((m)->osname)) {    \
            free((m)->osname);          \
        }                               \
    } while(0)

#define PMIX_DEVICE_DIST_CREATE(m, n)                                                       \
    do {                                                                                    \
        size_t _i;                                                                          \
        pmix_device_distance_t *_m;                                                         \
        _m = (pmix_device_distance_t*)pmixabi_calloc((n), sizeof(pmix_device_distance_t));  \
        if (NULL != _m) {                                                                   \
            for (_i=0; _i < (n); _i++) {                                                    \
                _m[_i].mindist = UINT16_MAX;                                                \
                _m[_i].maxdist = UINT16_MAX;                                                \
            }                                                                               \
        }                                                                                   \
        (m) = _m;                                                                           \
    } while(0)

#define PMIX_DEVICE_DIST_FREE(m, n)                     \
    do {                                                \
        size_t _i;                                      \
        if (NULL != (m)) {                              \
            for (_i=0; _i < (n); _i++) {                \
                PMIX_DEVICE_DIST_DESTRUCT(&(m)[_i]);    \
            }                                           \
            free((m));                                  \
            (m) = NULL;                                 \
        }                                               \
    } while(0)

/*
 * pmix_byte_object_t
 */
#define PMIX_BYTE_OBJECT_CREATE(m, n)   \
    do {                                \
        (m) = (pmix_byte_object_t*)pmixabi_malloc((n) * sizeof(pmix_byte_object_t)); \
        if (NULL != (m)) {                                                     \
            memset((m), 0, (n)*sizeof(pmix_byte_object_t));                    \
        }                                                                      \
    } while(0)

#define PMIX_BYTE_OBJECT_CONSTRUCT(m)   \
    do {                                \
        (m)->bytes = NULL;              \
        (m)->size = 0;                  \
    } while(0)

#define PMIX_BYTE_OBJECT_DESTRUCT(m)    \
    do {                                \
        if (NULL != (m)->bytes) {       \
            pmixabi_free((m)->bytes);   \
        }                               \
        (m)->bytes = NULL;              \
        (m)->size = 0;                  \
    } while(0)

#define PMIX_BYTE_OBJECT_FREE(m, n)                     \
    do {                                                \
        size_t _bon;                                    \
        if (NULL != (m)) {                              \
            for (_bon=0; _bon < n; _bon++) {            \
                PMIX_BYTE_OBJECT_DESTRUCT(&(m)[_bon]);  \
            }                                           \
            pmixabi_free((m));                          \
            (m) = NULL;                                 \
        }                                               \
    } while(0)

#define PMIX_BYTE_OBJECT_LOAD(b, d, s)      \
    do {                                    \
        (b)->bytes = (char*)(d);            \
        (d) = NULL;                         \
        (b)->size = (s);                    \
        (s) = 0;                            \
    } while(0)

/*
 * pmix_endpoint_t
 */
#define PMIX_ENDPOINT_CONSTRUCT(m)      \
    memset((m), 0, sizeof(pmix_endpoint_t))

#define PMIX_ENDPOINT_DESTRUCT(m)       \
    do {                                \
        if (NULL != (m)->uuid) {        \
            free((m)->uuid);            \
        }                               \
        if (NULL != (m)->osname) {      \
            free((m)->osname);          \
        }                               \
        if (NULL != (m)->endpt.bytes) { \
            free((m)->endpt.bytes);     \
        }                               \
    } while(0)

#define PMIX_ENDPOINT_CREATE(m, n)      \
    (m) = (pmix_endpoint_t*)pmixabi_calloc((n), sizeof(pmix_endpoint_t))

#define PMIX_ENDPOINT_FREE(m, n)                    \
    do {                                            \
        size_t _n;                                  \
        if (NULL != (m)) {                          \
            for (_n=0; _n < (n); _n++) {            \
                PMIX_ENDPOINT_DESTRUCT(&((m)[_n])); \
            }                                       \
            free((m));                              \
            (m) = NULL;                             \
        }                                           \
    } while(0)

/*
 * pmix_envar_t
 */
#define PMIX_ENVAR_CONSTRUCT(m)        \
    do {                               \
        (m)->envar = NULL;             \
        (m)->value = NULL;             \
        (m)->separator = '\0';         \
    } while(0)

#define PMIX_ENVAR_DESTRUCT(m)         \
    do {                               \
        if (NULL != (m)->envar) {      \
            pmixabi_free((m)->envar);  \
            (m)->envar = NULL;         \
        }                              \
        if (NULL != (m)->value) {      \
            pmixabi_free((m)->value);  \
            (m)->value = NULL;         \
        }                              \
    } while(0)

#define PMIX_ENVAR_CREATE(m, n)                                          \
    do {                                                                 \
        (m) = (pmix_envar_t*)pmixabi_calloc((n) , sizeof(pmix_envar_t)); \
    } while (0)
#define PMIX_ENVAR_FREE(m, n)                       \
    do {                                            \
        size_t _ek;                                 \
        if (NULL != (m)) {                          \
            for (_ek=0; _ek < (n); _ek++) {         \
                PMIX_ENVAR_DESTRUCT(&(m)[_ek]);     \
            }                                       \
            pmixabi_free((m));                      \
        }                                           \
    } while (0)

#define PMIX_ENVAR_LOAD(m, e, v, s)    \
    do {                               \
        if (NULL != (e)) {             \
            (m)->envar = strdup(e);    \
        }                              \
        if (NULL != (v)) {             \
            (m)->value = strdup(v);    \
        }                              \
        (m)->separator = (s);          \
    } while(0)

/*
 * pmix_proc_t
 */
#define PMIX_PROC_CREATE(m, n)                                  \
    do {                                                        \
        (m) = (pmix_proc_t*)pmixabi_calloc((n) , sizeof(pmix_proc_t));  \
    } while (0)

#define PMIX_PROC_RELEASE(m)    \
    do {                        \
        pmixabi_free((m));      \
        (m) = NULL;             \
    } while (0)

#define PMIX_PROC_CONSTRUCT(m)                  \
    do {                                        \
        memset((m), 0, sizeof(pmix_proc_t));    \
    } while (0)

#define PMIX_PROC_DESTRUCT(m)

#define PMIX_PROC_FREE(m, n)                    \
    do {                                        \
        if (NULL != (m)) {                      \
            pmixabi_free((m));                  \
            (m) = NULL;                         \
        }                                       \
    } while (0)

#define PMIX_PROC_LOAD(m, n, r)                                         \
    do {                                                                \
        PMIX_PROC_CONSTRUCT((m));                                       \
        pmixabi_strncpy((char*)(m)->nspace, (n), PMIX_MAX_NSLEN);       \
        (m)->rank = (r);                                                \
    } while(0)

#define PMIX_MULTICLUSTER_NSPACE_CONSTRUCT(t, c, n)                     \
    do {                                                                \
        size_t _len;                                                    \
        memset((t), 0, PMIX_MAX_NSLEN+1);                               \
        _len = pmixabi_nslen((c));                                      \
        if ((_len + pmixabi_nslen((n))) < PMIX_MAX_NSLEN) {             \
            pmixabi_strncpy((char*)(t), (c), PMIX_MAX_NSLEN);           \
            (t)[_len] = ':';                                            \
            pmixabi_strncpy((char*)&(t)[_len+1], (n), PMIX_MAX_NSLEN - _len); \
        }                                                               \
    } while(0)

#define PMIX_MULTICLUSTER_NSPACE_PARSE(t, c, n)             \
    do {                                                    \
        size_t _n, _j;                                      \
        for (_n=0; '\0' != (t)[_n] && ':' != (t)[_n] &&     \
             _n <= PMIX_MAX_NSLEN; _n++) {                  \
            (c)[_n] = (t)[_n];                              \
        }                                                   \
        _n++;                                               \
        for (_j=0; _n <= PMIX_MAX_NSLEN &&                  \
             '\0' != (t)[_n]; _n++, _j++) {                 \
            (n)[_j] = (t)[_n];                              \
        }                                                   \
    } while(0)

/*
 * pmix_proc_info_t
 */
#define PMIX_PROC_INFO_CREATE(m, n)                                         \
    do {                                                                    \
        (m) = (pmix_proc_info_t*)pmixabi_calloc((n) , sizeof(pmix_proc_info_t)); \
    } while (0)

#define PMIX_PROC_INFO_RELEASE(m)      \
    do {                               \
        PMIX_PROC_INFO_FREE((m), 1);   \
    } while (0)

#define PMIX_PROC_INFO_CONSTRUCT(m)                 \
    do {                                            \
        memset((m), 0, sizeof(pmix_proc_info_t));   \
    } while (0)

#define PMIX_PROC_INFO_DESTRUCT(m)              \
    do {                                        \
        if (NULL != (m)->hostname) {            \
            pmixabi_free((m)->hostname);        \
            (m)->hostname = NULL;               \
        }                                       \
        if (NULL != (m)->executable_name) {     \
            pmixabi_free((m)->executable_name); \
            (m)->executable_name = NULL;        \
        }                                       \
    } while(0)

#define PMIX_PROC_INFO_FREE(m, n)                   \
    do {                                            \
        size_t _k;                                  \
        if (NULL != (m)) {                          \
            for (_k=0; _k < (n); _k++) {            \
                PMIX_PROC_INFO_DESTRUCT(&(m)[_k]);  \
            }                                       \
            pmixabi_free((m));                      \
        }                                           \
    } while (0)

/*
 * pmix_value_t
 */
/* allocate and initialize a specified number of value structs */
#define PMIX_VALUE_CREATE(m, n)                                 \
    do {                                                        \
        int _ii;                                                \
        pmix_value_t *_v;                                       \
        (m) = (pmix_value_t*)pmixabi_calloc((n), sizeof(pmix_value_t)); \
        _v = (pmix_value_t*)(m);                                \
        if (NULL != (m)) {                                      \
            for (_ii=0; _ii < (int)(n); _ii++) {                \
                _v[_ii].type = PMIX_UNDEF;                      \
            }                                                   \
        }                                                       \
    } while (0)

/* release a single pmix_value_t struct, including its data */
#define PMIX_VALUE_RELEASE(m)       \
    do {                            \
        PMIX_VALUE_DESTRUCT((m));   \
        pmixabi_free((m));          \
        (m) = NULL;                 \
    } while (0)

/* initialize a single value struct */
#define PMIX_VALUE_CONSTRUCT(m)                 \
    do {                                        \
        memset((m), 0, sizeof(pmix_value_t));   \
        (m)->type = PMIX_UNDEF;                 \
    } while (0)

/* release the memory in the value struct data field */
#define PMIX_VALUE_DESTRUCT(m) \
    pmixabi_value_destruct(m)

#define PMIX_VALUE_FREE(m, n)                           \
    do {                                                \
        size_t _vv;                                     \
        if (NULL != (m)) {                              \
            for (_vv=0; _vv < (n); _vv++) {             \
                PMIX_VALUE_DESTRUCT(&((m)[_vv]));       \
            }                                           \
            pmixabi_free((m));                          \
            (m) = NULL;                                 \
        }                                               \
    } while (0)

#define PMIX_VALUE_GET_NUMBER(s, m, n, t)               \
    do {                                                \
        (s) = PMIX_SUCCESS;                             \
        if (PMIX_SIZE == (m)->type) {                   \
            (n) = (t)((m)->data.size);                  \
        } else if (PMIX_INT == (m)->type) {             \
            (n) = (t)((m)->data.integer);               \
        } else if (PMIX_INT8 == (m)->type) {            \
            (n) = (t)((m)->data.int8);                  \
        } else if (PMIX_INT16 == (m)->type) {           \
            (n) = (t)((m)->data.int16);                 \
        } else if (PMIX_INT32 == (m)->type) {           \
            (n) = (t)((m)->data.int32);                 \
        } else if (PMIX_INT64 == (m)->type) {           \
            (n) = (t)((m)->data.int64);                 \
        } else if (PMIX_UINT == (m)->type) {            \
            (n) = (t)((m)->data.uint);                  \
        } else if (PMIX_UINT8 == (m)->type) {           \
            (n) = (t)((m)->data.uint8);                 \
        } else if (PMIX_UINT16 == (m)->type) {          \
            (n) = (t)((m)->data.uint16);                \
        } else if (PMIX_UINT32 == (m)->type) {          \
            (n) = (t)((m)->data.uint32);                \
        } else if (PMIX_UINT64 == (m)->type) {          \
            (n) = (t)((m)->data.uint64);                \
        } else if (PMIX_FLOAT == (m)->type) {           \
            (n) = (t)((m)->data.fval);                  \
        } else if (PMIX_DOUBLE == (m)->type) {          \
            (n) = (t)((m)->data.dval);                  \
        } else if (PMIX_PID == (m)->type) {             \
            (n) = (t)((m)->data.pid);                   \
        } else if (PMIX_PROC_RANK == (m)->type) {       \
            (n) = (t)((m)->data.rank);                  \
        } else {                                        \
            (s) = PMIX_ERR_BAD_PARAM;                   \
        }                                               \
    } while(0)

/*
 * pmix_info_t
 */
#define PMIX_INFO_CREATE(m, n)                                          \
    do {                                                                \
        pmix_info_t *_i;                                                \
        (m) = (pmix_info_t*)pmixabi_calloc((n), sizeof(pmix_info_t));   \
        if (NULL != (m)) {                                              \
            _i = (pmix_info_t*)(m);                                     \
            _i[(n)-1].flags = PMIX_INFO_ARRAY_END;                      \
        }                                                               \
    } while (0)

#define PMIX_INFO_CONSTRUCT(m)                  \
    do {                                        \
        memset((m), 0, sizeof(pmix_info_t));    \
        (m)->value.type = PMIX_UNDEF;           \
    } while (0)

#define PMIX_INFO_DESTRUCT(m) \
    do {                                        \
        PMIX_VALUE_DESTRUCT(&(m)->value);       \
    } while (0)

#define PMIX_INFO_FREE(m, n)                        \
    do {                                            \
        size_t _is;                                 \
        if (NULL != (m)) {                          \
            for (_is=0; _is < (n); _is++) {         \
                PMIX_INFO_DESTRUCT(&((m)[_is]));    \
            }                                       \
            pmixabi_free((m));                      \
            (m) = NULL;                             \
        }                                           \
    } while (0)

#define PMIX_INFO_REQUIRED(m)       \
    ((m)->flags |= PMIX_INFO_REQD)
#define PMIX_INFO_OPTIONAL(m)       \
    ((m)->flags &= ~PMIX_INFO_REQD)

#define PMIX_INFO_IS_REQUIRED(m)    \
    ((m)->flags & PMIX_INFO_REQD)
#define PMIX_INFO_IS_OPTIONAL(m)    \
    !((m)->flags & PMIX_INFO_REQD)

#define PMIX_INFO_WAS_PROCESSED(m)  \
    ((m)->flags |= PMIX_INFO_REQD_PROCESSED)
#define PMIX_INFO_PROCESSED(m)  \
    ((m)->flags & PMIX_INFO_REQD_PROCESSED)

#define PMIX_INFO_IS_END(m)         \
    ((m)->flags & PMIX_INFO_ARRAY_END)

#define PMIX_INFO_TRUE(m)   \
    (PMIX_UNDEF == (m)->value.type || (PMIX_BOOL == (m)->value.type && (m)->value.data.flag)) ? true : false

/*
 * pmix_pdata_t
 */
#define PMIX_PDATA_CREATE(m, n)                                 \
    do {                                                        \
        (m) = (pmix_pdata_t*)pmixabi_calloc((n), sizeof(pmix_pdata_t)); \
    } while (0)

#define PMIX_PDATA_RELEASE(m)                   \
    do {                                        \
        PMIX_VALUE_DESTRUCT(&(m)->value);       \
        pmixabi_free((m));                      \
        (m) = NULL;                             \
    } while (0)

#define PMIX_PDATA_CONSTRUCT(m)                 \
    do {                                        \
        memset((m), 0, sizeof(pmix_pdata_t));   \
        (m)->value.type = PMIX_UNDEF;           \
    } while (0)

#define PMIX_PDATA_DESTRUCT(m)                  \
    do {                                        \
        PMIX_VALUE_DESTRUCT(&(m)->value);       \
    } while (0)

#define PMIX_PDATA_FREE(m, n)                           \
    do {                                                \
        size_t _ps;                                     \
        pmix_pdata_t *_pdf = (pmix_pdata_t*)(m);        \
        if (NULL != _pdf) {                             \
            for (_ps=0; _ps < (n); _ps++) {             \
                PMIX_PDATA_DESTRUCT(&(_pdf[_ps]));      \
            }                                           \
            pmixabi_free((m));                          \
            (m) = NULL;                                 \
        }                                               \
    } while (0)

/*
 * pmix_app_t
 */
#define PMIX_APP_CREATE(m, n)                                   \
    do {                                                        \
        (m) = (pmix_app_t*)pmixabi_calloc((n), sizeof(pmix_app_t));     \
    } while (0)

#define PMIX_APP_INFO_CREATE(m, n)                  \
    do {                                            \
        (m)->ninfo = (n);                           \
        PMIX_INFO_CREATE((m)->info, (m)->ninfo);    \
    } while(0)

#define PMIX_APP_RELEASE(m)                     \
    do {                                        \
        PMIX_APP_DESTRUCT((m));                 \
        pmixabi_free((m));                      \
        (m) = NULL;                             \
    } while (0)

#define PMIX_APP_CONSTRUCT(m)                   \
    do {                                        \
        memset((m), 0, sizeof(pmix_app_t));     \
    } while (0)

#define PMIX_APP_DESTRUCT(m)                                    \
    do {                                                        \
        size_t _aii;                                            \
        if (NULL != (m)->cmd) {                                 \
            pmixabi_free((m)->cmd);                             \
            (m)->cmd = NULL;                                    \
        }                                                       \
        if (NULL != (m)->argv) {                                \
            for (_aii=0; NULL != (m)->argv[_aii]; _aii++) {     \
                pmixabi_free((m)->argv[_aii]);                  \
            }                                                   \
            pmixabi_free((m)->argv);                            \
            (m)->argv = NULL;                                   \
        }                                                       \
        if (NULL != (m)->env) {                                 \
            for (_aii=0; NULL != (m)->env[_aii]; _aii++) {      \
                pmixabi_free((m)->env[_aii]);                   \
            }                                                   \
            pmixabi_free((m)->env);                             \
            (m)->env = NULL;                                    \
        }                                                       \
        if (NULL != (m)->cwd) {                                 \
            pmixabi_free((m)->cwd);                             \
            (m)->cwd = NULL;                                    \
        }                                                       \
        if (NULL != (m)->info) {                                \
            PMIX_INFO_FREE((m)->info, (m)->ninfo);              \
            (m)->info = NULL;                                   \
            (m)->ninfo = 0;                                     \
        }                                                       \
    } while (0)

#define PMIX_APP_FREE(m, n)                     \
    do {                                        \
        size_t _as;                             \
        if (NULL != (m)) {                      \
            for (_as=0; _as < (n); _as++) {     \
                PMIX_APP_DESTRUCT(&((m)[_as])); \
            }                                   \
            pmixabi_free((m));                  \
            (m) = NULL;                         \
        }                                       \
    } while (0)

/*
 * pmix_query_t
 */
#define PMIX_QUERY_CREATE(m, n)                                     \
    do {                                                            \
        (m) = (pmix_query_t*)pmixabi_calloc((n) , sizeof(pmix_query_t)); \
    } while (0)

#define PMIX_QUERY_QUALIFIERS_CREATE(m, n)                  \
    do {                                                    \
        (m)->nqual = (n);                                   \
        PMIX_INFO_CREATE((m)->qualifiers, (m)->nqual);      \
    } while(0)

#define PMIX_QUERY_RELEASE(m)       \
    do {                            \
        PMIX_QUERY_DESTRUCT((m));   \
        pmixabi_free((m));          \
        (m) = NULL;                 \
    } while (0)

#define PMIX_QUERY_CONSTRUCT(m)                 \
    do {                                        \
        memset((m), 0, sizeof(pmix_query_t));   \
    } while (0)

#define PMIX_QUERY_DESTRUCT(m)                                  \
    do {                                                        \
        size_t _qi;                                             \
        if (NULL != (m)->keys) {                                \
            for (_qi=0; NULL != (m)->keys[_qi]; _qi++) {        \
                pmixabi_free((m)->keys[_qi]);                   \
            }                                                   \
            pmixabi_free((m)->keys);                            \
            (m)->keys = NULL;                                   \
        }                                                       \
        if (NULL != (m)->qualifiers) {                          \
            PMIX_INFO_FREE((m)->qualifiers, (m)->nqual);        \
            (m)->qualifiers = NULL;                             \
            (m)->nqual = 0;                                     \
        }                                                       \
    } while (0)

#define PMIX_QUERY_FREE(m, n)                       \
    do {                                            \
        size_t _qs;                                 \
        if (NULL != (m)) {                          \
            for (_qs=0; _qs < (n); _qs++) {         \
                PMIX_QUERY_DESTRUCT(&((m)[_qs]));   \
            }                                       \
            pmixabi_free((m));                      \
            (m) = NULL;                             \
        }                                           \
    } while (0)

/*
 * pmix_regattr_t
 */
#define PMIX_REGATTR_CONSTRUCT(a)                       \
    do {                                                \
        if (NULL != (a)) {                              \
            (a)->name = NULL;                           \
            memset((a)->string, 0, PMIX_MAX_KEYLEN+1);  \
            (a)->type = PMIX_UNDEF;                     \
            (a)->description = NULL;                    \
        }                                               \
    } while(0)

#define PMIX_REGATTR_LOAD(a, n, k, t, v)                        \
    do {                                                        \
        pmix_status_t _rgl;                                     \
        if (NULL != (n)) {                                      \
            (a)->name = strdup((n));                            \
        }                                                       \
        if (NULL != (k)) {                                      \
            PMIX_LOAD_KEY((a)->string, (k));                    \
        }                                                       \
        (a)->type = (t);                                        \
        if (NULL != (v)) {                                      \
            PMIX_ARGV_APPEND(_rgl, &(a)->description, (v));     \
        }                                                       \
    } while(0)

#define PMIX_REGATTR_DESTRUCT(a)                    \
    do {                                            \
        if (NULL != (a)) {                          \
            if (NULL != (a)->name) {                \
                pmixabi_free((a)->name);            \
            }                                       \
            if (NULL != (a)->description) {         \
                PMIX_ARGV_FREE((a)->description);   \
            }                                       \
        }                                           \
    } while(0)

#define PMIX_REGATTR_CREATE(m, n)                                       \
    do {                                                                \
        (m) = (pmix_regattr_t*)pmixabi_calloc((n) , sizeof(pmix_regattr_t)); \
    } while (0)

#define PMIX_REGATTR_FREE(m, n)                         \
    do {                                                \
        size_t _ra;                                     \
        if (NULL != (m)) {                              \
            for (_ra=0; _ra < (n); _ra++) {             \
                PMIX_REGATTR_DESTRUCT(&((m)[_ra]));     \
            }                                           \
            pmixabi_free((m));                          \
            (m) = NULL;                                 \
        }                                               \
    } while (0)

#define PMIX_REGATTR_XFER(a, b)                                         \
    do {                                                                \
        size_t _n;                                                      \
        PMIX_REGATTR_CONSTRUCT((a));                                    \
        if (NULL != ((b)->name)) {                                      \
            (a)->name = strdup((b)->name);                              \
        }                                                               \
        PMIX_LOAD_KEY((a)->string, (b)->string);                        \
        (a)->type = (b)->type;                                          \
        if (NULL != (b)->description) {                                 \
            PMIX_ARGV_COPY((a)->description, (b)->description);         \
        }                                                               \
    } while(0)

/*
 * pmix_fabric_t
 */
#define PMIX_FABRIC_CONSTRUCT(x) \
    memset(x, 0, sizeof(pmix_fabric_t))

/*
 * pmix_data_array_t
 */
#define PMIX_DATA_ARRAY_CONSTRUCT(m, n, t)                          \
    do {                                                            \
        (m)->type = (t);                                            \
        (m)->size = (n);                                            \
        if (0 < (n)) {                                              \
            if (PMIX_INFO == (t)) {                                 \
                PMIX_INFO_CREATE((m)->array, (n));                  \
                                                                    \
            } else if (PMIX_PROC == (t)) {                          \
                PMIX_PROC_CREATE((m)->array, (n));                  \
                                                                    \
            } else if (PMIX_PROC_INFO == (t)) {                     \
                PMIX_PROC_INFO_CREATE((m)->array, (n));             \
                                                                    \
            } else if (PMIX_ENVAR == (t)) {                         \
                PMIX_ENVAR_CREATE((m)->array, (n));                 \
                                                                    \
            } else if (PMIX_VALUE == (t)) {                         \
                PMIX_VALUE_CREATE((m)->array, (n));                 \
                                                                    \
            } else if (PMIX_PDATA == (t)) {                         \
                PMIX_PDATA_CREATE((m)->array, (n));                 \
                                                                    \
            } else if (PMIX_QUERY == (t)) {                         \
                PMIX_QUERY_CREATE((m)->array, (n));                 \
                                                                    \
            } else if (PMIX_APP == (t)) {                           \
                PMIX_APP_CREATE((m)->array, (n));                   \
                                                                    \
            } else if (PMIX_BYTE_OBJECT == (t) ||                   \
                       PMIX_COMPRESSED_STRING == (t)) {             \
                PMIX_BYTE_OBJECT_CREATE((m)->array, (n));           \
                                                                    \
            } else if (PMIX_ALLOC_DIRECTIVE == (t) ||               \
                       PMIX_PROC_STATE == (t) ||                    \
                       PMIX_PERSIST == (t) ||                       \
                       PMIX_SCOPE == (t) ||                         \
                       PMIX_DATA_RANGE == (t) ||                    \
                       PMIX_BYTE == (t) ||                          \
                       PMIX_INT8 == (t) ||                          \
                       PMIX_UINT8 == (t) ||                         \
                       PMIX_POINTER == (t)) {                       \
                (m)->array = pmixabi_calloc((n), sizeof(int8_t));   \
                                                                    \
            } else if (PMIX_STRING == (t)) {                        \
                (m)->array = pmixabi_calloc((n), sizeof(char*));    \
                                                                    \
            } else if (PMIX_SIZE == (t)) {                          \
                (m)->array = pmixabi_calloc((n), sizeof(size_t));   \
                                                                    \
            } else if (PMIX_PID == (t)) {                           \
                (m)->array = pmixabi_calloc((n), sizeof(pid_t));    \
                                                                    \
            } else if (PMIX_INT == (t) ||                           \
                       PMIX_UINT == (t) ||                          \
                       PMIX_STATUS == (t)) {                        \
                (m)->array = pmixabi_calloc((n), sizeof(int));      \
                                                                    \
            } else if (PMIX_IOF_CHANNEL == (t) ||                   \
                       PMIX_DATA_TYPE == (t) ||                     \
                       PMIX_INT16 == (t) ||                         \
                       PMIX_UINT16 == (t)) {                        \
                (m)->array = pmixabi_calloc((n), sizeof(int16_t));  \
                                                                    \
            } else if (PMIX_PROC_RANK == (t) ||                     \
                       PMIX_INFO_DIRECTIVES == (t) ||               \
                       PMIX_INT32 == (t) ||                         \
                       PMIX_UINT32 == (t)) {                        \
                (m)->array = pmixabi_calloc((n), sizeof(int32_t));  \
                                                                    \
            } else if (PMIX_INT64 == (t) ||                         \
                       PMIX_UINT64 == (t)) {                        \
                (m)->array = pmixabi_calloc((n), sizeof(int64_t));  \
                                                                    \
            } else if (PMIX_FLOAT == (t)) {                         \
                (m)->array = pmixabi_calloc((n), sizeof(float));    \
                                                                    \
            } else if (PMIX_DOUBLE == (t)) {                        \
                (m)->array = pmixabi_calloc((n), sizeof(double));   \
                                                                    \
            } else if (PMIX_TIMEVAL == (t)) {                       \
                (m)->array = pmixabi_calloc((n), sizeof(struct timeval)); \
                                                                    \
            } else if (PMIX_TIME == (t)) {                          \
                (m)->array = pmixabi_calloc((n), sizeof(time_t));   \
                                                                    \
            } else if (PMIX_REGATTR == (t)) {                       \
                PMIX_REGATTR_CREATE((m)->array, (n));               \
                                                                    \
            } else if (PMIX_BOOL == (t)) {                          \
                (m)->array = pmixabi_calloc((n), sizeof(bool));     \
                                                                    \
            } else if (PMIX_COORD == (t)) {                         \
                (m)->array = pmixabi_calloc((n), sizeof(pmix_coord_t)); \
                                                                    \
            } else if (PMIX_LINK_STATE == (t)) {                    \
                (m)->array = pmixabi_calloc((n), sizeof(pmix_link_state_t)); \
                                                                    \
            } else if (PMIX_ENDPOINT == (t)) {                      \
                PMIX_ENDPOINT_CREATE((m)->array, n);                \
                                                                    \
            } else if (PMIX_PROC_NSPACE == (t)) {                   \
                (m)->array = pmixabi_calloc((n), sizeof(pmix_nspace_t)); \
                                                                    \
            } else if (PMIX_PROC_STATS == (t)) {                    \
                PMIX_PROC_STATS_CREATE((m)->array, n);              \
                                                                    \
            } else if (PMIX_DISK_STATS == (t)) {                    \
                PMIX_DISK_STATS_CREATE((m)->array, n);              \
                                                                    \
            } else if (PMIX_NET_STATS == (t)) {                     \
                PMIX_NET_STATS_CREATE((m)->array, n);               \
                                                                    \
            } else if (PMIX_NODE_STATS == (t)) {                    \
                PMIX_NODE_STATS_CREATE((m)->array, n);              \
                                                                    \
            } else if (PMIX_DEVICE_DIST == (t)) {                   \
                PMIX_DEVICE_DIST_CREATE((m)->array, n);             \
                                                                    \
            } else if (PMIX_GEOMETRY == (t)) {                      \
                PMIX_GEOMETRY_CREATE((m)->array, n);                \
                                                                    \
            } else if (PMIX_REGATTR == (t)) {                       \
                PMIX_REGATTR_CREATE((m)->array, n);                 \
                                                                    \
            } else if (PMIX_PROC_CPUSET == (t)) {                   \
                PMIX_CPUSET_CREATE((m)->array, n);                  \
            } else {                                                \
                (m)->array = NULL;                                  \
                (m)->size = 0;                                      \
            }                                                       \
        } else {                                                    \
            (m)->array = NULL;                                      \
        }                                                           \
    } while(0)
#define PMIX_DATA_ARRAY_CREATE(m, n, t)                                         \
    do {                                                                        \
        (m) = (pmix_data_array_t*)pmixabi_calloc(1, sizeof(pmix_data_array_t)); \
        if (NULL != (m)) {                                                      \
            PMIX_DATA_ARRAY_CONSTRUCT((m), (n), (t));                           \
        }                                                                       \
    } while(0)

#define PMIX_DATA_ARRAY_DESTRUCT(m) pmixabi_darray_destruct(m)

#define PMIX_DATA_ARRAY_FREE(m)             \
    do {                                    \
        if (NULL != (m)) {                  \
            PMIX_DATA_ARRAY_DESTRUCT(m);    \
            pmixabi_free((m));              \
            (m) = NULL;                     \
        }                                   \
    } while(0)


/* *******************************************************************
 * Some of the PMIx ABI Support functions rely on the macros above,
 * but are not used by the macros above. So they need to be defined
 * after the macros are defined.
 * *******************************************************************/
#include "pmix_abi_support_bottom.h"

/* PMIX_MACROS_H */
#endif
