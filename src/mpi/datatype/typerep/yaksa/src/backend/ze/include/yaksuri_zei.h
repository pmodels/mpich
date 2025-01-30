/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef YAKSURI_ZEI_H_INCLUDED
#define YAKSURI_ZEI_H_INCLUDED

#include "yaksi.h"
#include <stdint.h>
#include <pthread.h>
#include <level_zero/ze_api.h>
#include "yaksuri_zei_md.h"

#define ZE_DEBUG 0

#define ZE_P2P_ENABLED  (1)
#define ZE_P2P_DISABLED (2)
#define ZE_P2P_CLIQUES  (3)

#define YAKSURI_KERNEL_NULL   -1

/* create yaksuri_zei_md_s in USM host memory instead of USM shared memory */
#define ZE_MD_HOST             1

#define ZE_THROTTLE_THRESHOLD  (4 * 1024)
#define ZE_CMD_LIST_INIT_POOL_SIZE      8

#define ZE_EVENT_POOL_BITS      13
#define ZE_EVENT_POOL_CAP       (1 << ZE_EVENT_POOL_BITS)
#define ZE_EVENT_POOL_MASK      ((1 << ZE_EVENT_POOL_BITS) - 1)
#define ZE_EVENT_POOL_INDEX(x)  ((x) & ZE_EVENT_POOL_MASK)

#define YAKSURI_ZEI_INFO__DEFAULT_IOV_PUP_THRESHOLD     (16384)

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

#define YAKSURI_ZEI_ZE_ERR_CHECK(zerr)                                  \
    do {                                                                \
        if (zerr != ZE_RESULT_SUCCESS) {                                \
            fprintf(stderr, "ZE Error (%s:%s,%d): %d\n", __func__, __FILE__, __LINE__, zerr); \
        }                                                               \
    } while (0)

#define YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail)                \
    do {                                                                \
        if (zerr != ZE_RESULT_SUCCESS) {                                \
            fprintf(stderr, "ZE Error (%s:%s,%d): %d\n", __func__, __FILE__, __LINE__, zerr); \
            rc = YAKSA_ERR__INTERNAL;                                   \
            goto fn_fail;                                               \
        }                                                               \
    } while (0)

typedef struct {
    ze_device_handle_t *subdevices;
    uint32_t nsubdevices;
    uint32_t deviceId;          /* level zero device Id */
    ze_event_pool_handle_t ep;  /* event pool, one per device */
    int ev_pool_idx;
    int ev_lb, ev_ub;           /* [lb,ub] defines the events being used */
    ze_event_handle_t *events;
    int last_event_idx;         /* immed. cmd lists are serialized */
    ze_command_list_handle_t cmdlist;   /* immed. cmd lists being used */
    ze_command_queue_group_properties_t *queueProperties;
    uint32_t numQueueGroups;
    pthread_mutex_t mutex;
    int dev_id;
} yaksuri_zei_device_state_s;

typedef struct {
    ze_driver_handle_t driver;
    uint32_t ndevices;
    ze_device_handle_t *device;
    ze_context_handle_t context;
    int throttle_threshold;
    bool **p2p;
    pthread_mutex_t ze_mutex;
    yaksuri_zei_device_state_s *device_states;
} yaksuri_zei_global_s;
extern yaksuri_zei_global_s yaksuri_zei_global;

/* Define abstraction for ze event and buffer command list with this */
typedef struct {
    ze_event_handle_t ze_event;
    int dev_id;
    int idx;
} yaksuri_zei_event_s;

typedef struct yaksuri_zei_type_s {
    int pack[YAKSA_OP__LAST];
    int unpack[YAKSA_OP__LAST];
    ze_kernel_handle_t *pack_kernels[YAKSA_OP__LAST];
    ze_kernel_handle_t *unpack_kernels[YAKSA_OP__LAST];
    yaksuri_zei_md_s **md;
    pthread_mutex_t mdmutex;
    uintptr_t num_elements;
    const char *name;
} yaksuri_zei_type_s;

typedef struct {
    int yaksa_ze_use_copy_engine;
    uintptr_t iov_pack_threshold;
    uintptr_t iov_unpack_threshold;
    struct {
        bool is_valid;
        struct {
            ze_memory_allocation_properties_t prop;
            ze_device_handle_t device;
        } attr;
    } inbuf, outbuf;
} yaksuri_zei_info_s;

extern ze_module_handle_t *yaksuri_ze_modules[];
extern ze_kernel_handle_t *yaksuri_ze_kernels[];

int yaksuri_zei_finalize_hook(void);
int yaksuri_zei_type_create_hook(yaksi_type_s * type);
int yaksuri_zei_type_free_hook(yaksi_type_s * type);
int yaksuri_zei_info_create_hook(yaksi_info_s * info);
int yaksuri_zei_info_free_hook(yaksi_info_s * info);
int yaksuri_zei_info_keyval_append(yaksi_info_s * info, const char *key, const void *val,
                                   unsigned int vallen);

int yaksuri_zei_event_record(int device, void **event);
int yaksuri_zei_event_query(void *event, int *completed);
int yaksuri_zei_add_dependency(int device1, int device2);

int yaksuri_zei_get_ptr_attr(const void *inbuf, void *outbuf, yaksi_info_s * info,
                             yaksur_ptr_attr_s * inattr, yaksur_ptr_attr_s * outattr);

int yaksuri_zei_md_alloc(yaksi_type_s * type, int dev_id);
int yaksuri_zei_populate_pupfns(yaksi_type_s * type);

int yaksuri_zei_ipack(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                      yaksi_info_s * info, yaksa_op_t op, int target);
int yaksuri_zei_iunpack(const void *inbuf, void *outbuf, uintptr_t count, yaksi_type_s * type,
                        yaksi_info_s * info, yaksa_op_t op, int target);
int yaksuri_zei_synchronize(int target);
int yaksuri_zei_flush_all(void);
int yaksuri_zei_pup_is_supported(yaksi_type_s * type, yaksa_op_t op, bool * is_supported);
uintptr_t yaksuri_zei_get_iov_pack_threshold(yaksi_info_s * info);
uintptr_t yaksuri_zei_get_iov_unpack_threshold(yaksi_info_s * info);

ze_result_t yaksuri_ze_init_module_kernel(void);
ze_result_t yaksuri_ze_finalize_module_kernel(void);

int yaksuri_zei_type_make_resident(yaksi_type_s * type, int dev_id);
int yaksuri_zei_type_evict_resident(yaksi_type_s * type, int dev_id);

int create_ze_event(int dev_id, ze_event_handle_t * ze_event, int *idx);
void recycle_command_list(ze_command_list_handle_t cl, int dev_id);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif /* YAKSURI_ZEI_H_INCLUDED */
