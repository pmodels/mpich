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
 * These PMIx ABI Support function are _not_ defined in the PMIx standard.
 * They exist here to provide a complete implementation for the macros
 * defined in pmix_macros.h
 * They are all prefixed with 'pmixabi_' to avoid the standard namespace
 * prefix of 'pmix_'.
 *
 * The few PMIx ABI Support functions defined in this file rely on
 * the macros defined in pmix_macros.h. So are included at the bottom
 * of that file.
 */
#ifndef PMIX_ABI_SUPPORT_BOTTOM_H
#define PMIX_ABI_SUPPORT_BOTTOM_H

static inline
void pmixabi_value_destruct(pmix_value_t * m)
{
    if (PMIX_STRING == (m)->type) {
        if (NULL != (m)->data.string) {
            pmixabi_free((m)->data.string);
            (m)->data.string = NULL;
        }
    } else if ((PMIX_BYTE_OBJECT == (m)->type) ||
               (PMIX_COMPRESSED_STRING == (m)->type)) {
        if (NULL != (m)->data.bo.bytes) {
            pmixabi_free((m)->data.bo.bytes);
            (m)->data.bo.bytes = NULL;
            (m)->data.bo.size = 0;
        }
    } else if (PMIX_DATA_ARRAY == (m)->type) {
        if (NULL != (m)->data.darray) {
            pmixabi_darray_destruct((m)->data.darray);
            pmixabi_free((m)->data.darray);
            (m)->data.darray = NULL;
        }
    } else if (PMIX_ENVAR == (m)->type) {
        PMIX_ENVAR_DESTRUCT(&(m)->data.envar);
    } else if (PMIX_PROC == (m)->type) {
        PMIX_PROC_RELEASE((m)->data.proc);
    }
}

static inline
void pmixabi_darray_destruct(pmix_data_array_t *m)
{
    if (NULL != m) {
        if (PMIX_INFO == m->type) {
            pmix_info_t *_info = (pmix_info_t*)m->array;
            PMIX_INFO_FREE(_info, m->size);
        } else if (PMIX_PROC == m->type) {
            pmix_proc_t *_p = (pmix_proc_t*)m->array;
            PMIX_PROC_FREE(_p, m->size);
        } else if (PMIX_PROC_INFO == m->type) {
            pmix_proc_info_t *_pi = (pmix_proc_info_t*)m->array;
            PMIX_PROC_INFO_FREE(_pi, m->size);
        } else if (PMIX_ENVAR == m->type) {
            pmix_envar_t *_e = (pmix_envar_t*)m->array;
            PMIX_ENVAR_FREE(_e, m->size); 
        } else if (PMIX_VALUE == m->type) {
            pmix_value_t *_v = (pmix_value_t*)m->array;
            PMIX_VALUE_FREE(_v, m->size);
        } else if (PMIX_PDATA == m->type) {
            pmix_pdata_t *_pd = (pmix_pdata_t*)m->array;
            PMIX_PDATA_FREE(_pd, m->size);
        } else if (PMIX_QUERY == m->type) {
            pmix_query_t *_q = (pmix_query_t*)m->array;
            PMIX_QUERY_FREE(_q, m->size);
        } else if (PMIX_APP == m->type) {
            pmix_app_t *_a = (pmix_app_t*)m->array;
            PMIX_APP_FREE(_a, m->size);
        } else if (PMIX_BYTE_OBJECT == m->type ||
                   PMIX_COMPRESSED_STRING == m->type) {
            pmix_byte_object_t *_b = (pmix_byte_object_t*)m->array;
            PMIX_BYTE_OBJECT_FREE(_b, m->size);
        } else if (PMIX_STRING == m->type) {
            char **_s = (char**)m->array;
            size_t _si;
            for (_si=0; _si < m->size; _si++) {
                pmixabi_free(_s[_si]);
            }
            pmixabi_free(m->array);
            m->array = NULL;
        } else {
            pmixabi_free(m->array);
        }
    }
}

/* PMIX_ABI_SUPPORT_BOTTOM_H */
#endif
