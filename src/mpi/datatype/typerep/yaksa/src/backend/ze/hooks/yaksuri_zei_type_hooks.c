/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdlib.h>
#include <assert.h>
#include "yaksi.h"
#include "yaksu.h"
#include "yaksuri_zei.h"

extern const unsigned char *yaksuri_zei_pup_str[];
extern unsigned long yaksuri_zei_pup_size[];

extern char *yaksuri_zei_kernel_funcs[];
extern int yaksuri_zei_kernel_module_map[];

static ze_result_t yaksuri_ze_load_module(int module, ze_module_handle_t ** handle)
{
    ze_result_t zerr = ZE_RESULT_SUCCESS;
    int d;

    *handle = NULL;
    /* module string size 0 means an invalid module, should be ignored */
    if (yaksuri_ze_modules[module] == NULL && yaksuri_zei_pup_size[module] > 0) {
        yaksuri_ze_modules[module] =
            (ze_module_handle_t *) calloc(yaksuri_zei_global.ndevices, sizeof(ze_module_handle_t));

        for (d = 0; d < yaksuri_zei_global.ndevices; d++) {
            ze_module_desc_t desc = { ZE_STRUCTURE_TYPE_MODULE_DESC };
            desc.pNext = NULL;
#if ZE_NATIVE
            desc.format = ZE_MODULE_FORMAT_NATIVE;
            desc.pBuildFlags = NULL;
#else
            desc.format = ZE_MODULE_FORMAT_IL_SPIRV;
            desc.pBuildFlags = NULL;
#endif
            desc.inputSize = yaksuri_zei_pup_size[module];
            desc.pInputModule = yaksuri_zei_pup_str[module];
            desc.pConstants = NULL;
            zerr =
                zeModuleCreate(yaksuri_zei_global.context, yaksuri_zei_global.device[d], &desc,
                               &yaksuri_ze_modules[module][d], NULL);
            if (zerr != ZE_RESULT_SUCCESS) {
                if (zerr == ZE_RESULT_ERROR_MODULE_BUILD_FAILURE) {
#if ZE_DEBUG
                    ze_module_build_log_handle_t hModuleBuildLog = NULL;
                    zerr =
                        zeModuleCreate(yaksuri_zei_global.context, yaksuri_zei_global.device[d],
                                       &desc, &yaksuri_ze_modules[module][d], &hModuleBuildLog);
                    size_t pSize = 0;
                    char *pBuildLog = NULL;
                    zeModuleBuildLogGetString(hModuleBuildLog, &pSize, NULL);
                    pBuildLog = (char *) malloc(pSize + 1);
                    zeModuleBuildLogGetString(hModuleBuildLog, &pSize, pBuildLog);
                    fprintf(stderr, "Build log: %s\n", pBuildLog);
                    free(pBuildLog);
#endif
                    /* ignore build error, e.g. double type is not supported */
                    zerr = ZE_RESULT_SUCCESS;
                    yaksuri_ze_modules[module][d] = NULL;
                    continue;
                }
                fprintf(stderr, "ZE Error (%s:%s,%d,%d): 0x%x \n", __func__, __FILE__, __LINE__,
                        module, zerr);
                goto fn_fail;
            }
        }
    }
    *handle = yaksuri_ze_modules[module];

  fn_exit:
    return zerr;
  fn_fail:
    goto fn_exit;
}

static ze_result_t yaksuri_ze_load_kernel(int kernel, ze_kernel_handle_t ** handle)
{
    ze_module_handle_t *module_handle;
    ze_result_t zerr = ZE_RESULT_SUCCESS;

    ze_kernel_desc_t kdesc = {
        ZE_STRUCTURE_TYPE_KERNEL_DESC,
        NULL,
        0,      /* flag */
        NULL
    };

    if (yaksuri_ze_kernels[kernel] == NULL) {
        yaksuri_ze_kernels[kernel] =
            (ze_kernel_handle_t *) calloc(yaksuri_zei_global.ndevices, sizeof(ze_kernel_handle_t));
        zerr = yaksuri_ze_load_module(yaksuri_zei_kernel_module_map[kernel], &module_handle);
        /* ignore module error, it is ok that a module does not compile */
        if (zerr != ZE_RESULT_SUCCESS || module_handle == NULL) {
            goto fn_fail;
        }
        kdesc.pKernelName = yaksuri_zei_kernel_funcs[kernel];
        for (int d = 0; d < yaksuri_zei_global.ndevices; d++) {
            if (module_handle[d] == NULL)
                continue;
            yaksuri_zei_device_state_s *device_state = yaksuri_zei_global.device_states + d;
            pthread_mutex_lock(&device_state->mutex);

            zerr = zeKernelCreate(module_handle[d], &kdesc, &yaksuri_ze_kernels[kernel][d]);
            if (zerr != ZE_RESULT_SUCCESS) {
                fprintf(stderr, "ZE Error (%s:%s,%d,%d): 0x%x \n", __func__, __FILE__, __LINE__,
                        kernel, zerr);
                goto fn_fail;
            }
#if 0
            zerr =
                zeKernelSetIndirectAccess(yaksuri_ze_kernels[kernel][d],
                                          ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED);
            YAKSURI_ZEI_ZE_ERR_CHECK(zerr);
#endif
            pthread_mutex_unlock(&device_state->mutex);
        }
    }

    *handle = yaksuri_ze_kernels[kernel];
  fn_exit:
    return zerr;
  fn_fail:
    goto fn_exit;
}

static uintptr_t get_num_elements(yaksi_type_s * type)
{
    switch (type->kind) {
        case YAKSI_TYPE_KIND__BUILTIN:
            return type->num_contig;

        case YAKSI_TYPE_KIND__CONTIG:
            return type->u.contig.count * get_num_elements(type->u.contig.child);

        case YAKSI_TYPE_KIND__DUP:
            return get_num_elements(type->u.dup.child);

        case YAKSI_TYPE_KIND__RESIZED:
            return get_num_elements(type->u.resized.child);

        case YAKSI_TYPE_KIND__HVECTOR:
            return type->u.hvector.count * type->u.hvector.blocklength *
                get_num_elements(type->u.hvector.child);

        case YAKSI_TYPE_KIND__BLKHINDX:
            return type->u.blkhindx.count * type->u.blkhindx.blocklength *
                get_num_elements(type->u.blkhindx.child);

        case YAKSI_TYPE_KIND__HINDEXED:
            {
                uintptr_t nelems = 0;
                for (int i = 0; i < type->u.hindexed.count; i++)
                    nelems += type->u.hindexed.array_of_blocklengths[i];
                nelems *= get_num_elements(type->u.hindexed.child);
                return nelems;
            }

        default:
            return 0;
    }
}

int yaksuri_zei_type_create_hook(yaksi_type_s * type)
{
    ze_result_t zerr;
    int rc = YAKSA_SUCCESS;

    type->backend.ze.priv = malloc(sizeof(yaksuri_zei_type_s));
    yaksuri_zei_type_s *ze = (yaksuri_zei_type_s *) type->backend.ze.priv;

    ze->num_elements = get_num_elements(type);
    ze->md = NULL;
    pthread_mutex_init(&ze->mdmutex, NULL);

    rc = yaksuri_zei_populate_pupfns(type);
    YAKSU_ERR_CHECK(rc, fn_fail_no_lock);

    for (int i = 0; i < YAKSA_OP__LAST; i++) {
        ze->pack_kernels[i] = NULL;
        ze->unpack_kernels[i] = NULL;
        if (ze->pack[i] != YAKSURI_KERNEL_NULL) {
            pthread_mutex_lock(&yaksuri_zei_global.ze_mutex);
            zerr = yaksuri_ze_load_kernel(ze->pack[i], &ze->pack_kernels[i]);
            YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
            zerr = yaksuri_ze_load_kernel(ze->unpack[i], &ze->unpack_kernels[i]);
            YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
            pthread_mutex_unlock(&yaksuri_zei_global.ze_mutex);
        }
    }

  fn_exit:
    return rc;
  fn_fail:
    pthread_mutex_unlock(&yaksuri_zei_global.ze_mutex);
  fn_fail_no_lock:
    goto fn_exit;
}

int yaksuri_zei_type_free_hook(yaksi_type_s * type)
{
    int rc = YAKSA_SUCCESS;
    ze_result_t zerr;
    yaksuri_zei_type_s *ze = (yaksuri_zei_type_s *) type->backend.ze.priv;

    pthread_mutex_destroy(&ze->mdmutex);
    if (ze->md) {
        /* evict resident memory before freeing it */
        for (int i = 0; i < yaksuri_zei_global.ndevices; i++) {
            yaksuri_zei_md_s *md = ze->md[i];
            if (md == NULL)
                continue;
            yaksuri_zei_device_state_s *device_state = yaksuri_zei_global.device_states + i;
            pthread_mutex_lock(&device_state->mutex);
            yaksuri_zei_type_evict_resident(type, i);
            pthread_mutex_unlock(&device_state->mutex);
        }
        /* free memory */
        for (int i = 0; i < yaksuri_zei_global.ndevices; i++) {
            yaksuri_zei_md_s *md = ze->md[i];
            if (md == NULL)
                continue;
            yaksuri_zei_device_state_s *device_state = yaksuri_zei_global.device_states + i;
            pthread_mutex_lock(&device_state->mutex);
            if (type->kind == YAKSI_TYPE_KIND__BLKHINDX) {
                assert(md->u.blkhindx.array_of_displs);
                zerr =
                    zeMemFree(yaksuri_zei_global.context, (void *) md->u.blkhindx.array_of_displs);
                YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
            } else if (type->kind == YAKSI_TYPE_KIND__HINDEXED) {
                assert(md->u.hindexed.array_of_displs);
                zerr =
                    zeMemFree(yaksuri_zei_global.context, (void *) md->u.hindexed.array_of_displs);
                YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);

                assert(md->u.hindexed.array_of_blocklengths);
                zerr =
                    zeMemFree(yaksuri_zei_global.context,
                              (void *) md->u.hindexed.array_of_blocklengths);
                YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
            }

            zerr = zeMemFree(yaksuri_zei_global.context, md);
            YAKSURI_ZEI_ZE_ERR_CHKANDJUMP(zerr, rc, fn_fail);
            ze->md[i] = NULL;
            pthread_mutex_unlock(&device_state->mutex);
#if ZE_MD_HOST
            break;
#endif
        }
    }
    free(ze->md);
    free(ze);

  fn_exit:
    return rc;
  fn_fail:
    goto fn_exit;
}
