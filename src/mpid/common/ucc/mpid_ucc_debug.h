/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef _MPID_UCC_DEBUG_H_
#define _MPID_UCC_DEBUG_H_

#include "mpid_ucc.h"

#define __MPIDI_COMMON_UCC_DEBUG(_level, _format, _file, _func, _line, ...) \
    do {                                                                    \
        if (MPIDI_common_ucc_priv.verbose_debug &&                          \
            ((MPIDI_common_ucc_priv.verbose_level == _level * (-1)) ||      \
             (MPIDI_common_ucc_priv.verbose_level > _level))) {             \
            fprintf(stderr, "==%u== UCC DEBUG %s in %s: (%s:%d) | "         \
                    _format "\n", getpid(),                                 \
                    MPIDI_COMMON_UCC_VERBOSE_LEVEL_TO_STRING[_level],       \
                    _file, _func, _line,                                    \
                    ## __VA_ARGS__);                                        \
        }                                                                   \
    } while (0)

#ifdef MPIDI_COMMON_UCC_DEBUG
#undef MPIDI_COMMON_UCC_DEBUG
#endif
#define MPIDI_COMMON_UCC_DEBUG(_level, _format, ...)            \
    __MPIDI_COMMON_UCC_DEBUG(_level, _format,                   \
                            __FILE__, __FUNCTION__, __LINE__,   \
                            ## __VA_ARGS__)

#define MPIDI_COMMON_UCC_DEBUG_WRAPPER(_level, _format, ...)    \
    __MPIDI_COMMON_UCC_DEBUG(_level, _format,                   \
                             _file, _func, _line,               \
                             ## __VA_ARGS__)

#define MPIDI_COMMON_UCC_DEBUG_FNCALL(_fn_name)	\
    MPIDI_COMMON_UCC_DEBUG_WRAPPER(MPIDI_COMMON_UCC_VERBOSE_LEVEL_FNCALL, "calling %s() ...", _fn_name);

#define MPIDI_COMMON_UCC_DEBUG_FNDONE(_fn_name) \
    MPIDI_COMMON_UCC_DEBUG_WRAPPER(MPIDI_COMMON_UCC_VERBOSE_LEVEL_FNCALL, "call of %s() DONE", _fn_name);

#define MPIDI_COMMON_UCC_DEBUG_WRAPPER_CALL(_fn_name, ...) MPIDI_COMMON_UCC_DEBUG_WRAPPER_ \
    ## _fn_name (__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define MPIDI_COMMON_UCC_DEBUG_WRAPPER_IMPL(_ret_type, _fn_name, _arglist, ...) \
    static _ret_type MPIDI_COMMON_UCC_DEBUG_WRAPPER_ ## _fn_name                \
        (const char * _file, const char * _func, int _line, __VA_ARGS__)        \
    {                                                                           \
        _ret_type ret_val;                                                      \
        MPIDI_COMMON_UCC_DEBUG_FNCALL(# _fn_name);                              \
        ret_val = _fn_name _arglist ;                                           \
        MPIDI_COMMON_UCC_DEBUG_FNDONE(# _fn_name);                              \
        return ret_val;                                                         \
    }


MPIDI_COMMON_UCC_DEBUG_WRAPPER_IMPL(ucc_status_t, ucc_init,
                                    (params, config, lib_p),
                                    const ucc_lib_params_t * params, const ucc_lib_config_h config,
                                    ucc_lib_h * lib_p);
#define ucc_init(...) MPIDI_COMMON_UCC_DEBUG_WRAPPER_CALL(ucc_init, __VA_ARGS__)

MPIDI_COMMON_UCC_DEBUG_WRAPPER_IMPL(ucc_status_t, ucc_lib_config_read,
                                    (env_prefix, filename, config),
                                    const char *env_prefix, const char *filename,
                                    ucc_lib_config_h * config);
#define ucc_lib_config_read(...) MPIDI_COMMON_UCC_DEBUG_WRAPPER_CALL(ucc_lib_config_read, __VA_ARGS__)

MPIDI_COMMON_UCC_DEBUG_WRAPPER_IMPL(ucc_status_t, ucc_context_config_read,
                                    (lib_handle, filename, config),
                                    ucc_lib_h lib_handle, const char *filename,
                                    ucc_context_config_h * config);
#define ucc_context_config_read(...) MPIDI_COMMON_UCC_DEBUG_WRAPPER_CALL(ucc_context_config_read, __VA_ARGS__)

MPIDI_COMMON_UCC_DEBUG_WRAPPER_IMPL(ucc_status_t, ucc_context_config_modify,
                                    (config, component, name, value),
                                    ucc_context_config_h config, const char *component,
                                    const char *name, const char *value);
#define ucc_context_config_modify(...) MPIDI_COMMON_UCC_DEBUG_WRAPPER_CALL(ucc_context_config_modify, __VA_ARGS__)

MPIDI_COMMON_UCC_DEBUG_WRAPPER_IMPL(ucc_status_t, ucc_context_create,
                                    (lib_handle, params, config, context),
                                    ucc_lib_h lib_handle, const ucc_context_params_t * params,
                                    const ucc_context_config_h config, ucc_context_h * context);
#define ucc_context_create(...) MPIDI_COMMON_UCC_DEBUG_WRAPPER_CALL(ucc_context_create, __VA_ARGS__)

MPIDI_COMMON_UCC_DEBUG_WRAPPER_IMPL(ucc_status_t, ucc_context_destroy,
                                    (context), ucc_context_h context);
#define ucc_context_destroy(...) MPIDI_COMMON_UCC_DEBUG_WRAPPER_CALL(ucc_context_destroy, __VA_ARGS__);

MPIDI_COMMON_UCC_DEBUG_WRAPPER_IMPL(ucc_status_t, ucc_finalize, (lib_p), ucc_lib_h lib_p);
#define ucc_finalize(...) MPIDI_COMMON_UCC_DEBUG_WRAPPER_CALL(ucc_finalize, __VA_ARGS__)


#define MPIDI_COMMON_UCC_OUTPUT_COND_CONCAT_STR(_str, _cond) do {      \
       if (_cond) {                                                    \
           if (strlen(_str)) strcat(_str, " + ");                      \
           strcat(_str , MPIDI_COMMON_UCC_OUTPUT_STRINGIFY(_cond));    \
       }                                                               \
   } while (0);
#define MPIDI_COMMON_UCC_OUTPUT_COND_CREATE_STR(_str) do {             \
       MPIDI_COMMON_UCC_OUTPUT_COND_CONCAT_STR(_str,                   \
                      MPIDI_COMMON_UCC_COMM_EXCL_COND_TYPE);           \
       MPIDI_COMMON_UCC_OUTPUT_COND_CONCAT_STR(_str,                   \
                      MPIDI_COMMON_UCC_COMM_EXCL_COND_SIZE);           \
       MPIDI_COMMON_UCC_OUTPUT_COND_CONCAT_STR(_str,                   \
                      MPIDI_COMMON_UCC_COMM_EXCL_COND_DEV);            \
    } while (0);
#define MPIDI_COMMON_UCC_OUTPUT_COMM_EXCL_COND do {                    \
       char reason_str[512] = {0};                                     \
       MPIDI_COMMON_UCC_OUTPUT_COND_CREATE_STR(reason_str);            \
       MPIDI_COMMON_UCC_DEBUG(MPIDI_COMMON_UCC_VERBOSE_LEVEL_COMM,     \
                              "excluded comm %p, comm_id %d from ucc " \
                              "use with reason \"%s\"",                \
                              (void *) comm_ptr, comm_ptr->context_id, \
                              reason_str);                             \
    } while (0);

#endif /*_MPID_UCC_DEBUG_H_*/
