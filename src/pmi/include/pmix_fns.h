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
 * PMIx Standard API function pointer declarations for PMIx Standard APIs
 *
 * The goal of this header to ease the incorporation of PMIx routines
 * for applications and tools that wish to dlopen() a PMIx Standard
 * compliant library and then dlsym() the various functions.
 */

#ifndef PMIX_FNS_H
#define PMIX_FNS_H

/* *******************************************************************
 * PMIx Standard types, constants, and callback functions
 *  - pmix_types.h is included by pmix_macros.h
 * PMIx Standard macros
 * *******************************************************************/
#include "pmix_macros.h"


/**** PMIx Client functions ****/
typedef pmix_status_t (*pmix_init_fn_t)(pmix_proc_t *proc,
                                        pmix_info_t info[], size_t ninfo);

typedef pmix_status_t (*pmix_finalize_fn_t)(const pmix_info_t info[], size_t ninfo);

typedef int (*pmix_initialized_fn_t)(void);

typedef pmix_status_t (*pmix_abort_fn_t)(int status, const char msg[],
                                         pmix_proc_t procs[], size_t nprocs);

typedef pmix_status_t (*pmix_put_fn_t)(pmix_scope_t scope,
                                       const char key[],
                                       pmix_value_t *val);

typedef pmix_status_t (*pmix_commit_fn_t)(void);

typedef pmix_status_t (*pmix_fence_fn_t)(const pmix_proc_t procs[], size_t nprocs,
                                         const pmix_info_t info[], size_t ninfo);

typedef pmix_status_t (*pmix_fence_nb_fn_t)(const pmix_proc_t procs[], size_t nprocs,
                                            const pmix_info_t info[], size_t ninfo,
                                            pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_get_fn_t)(const pmix_proc_t *proc, const char key[],
                                       const pmix_info_t info[], size_t ninfo,
                                       pmix_value_t **val);

typedef pmix_status_t (*pmix_get_nb_fn_t)(const pmix_proc_t *proc, const char key[],
                                          const pmix_info_t info[], size_t ninfo,
                                          pmix_value_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_publish_fn_t)(const pmix_info_t info[], size_t ninfo);

typedef pmix_status_t (*pmix_publish_nb_fn_t)(const pmix_info_t info[], size_t ninfo,
                                              pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_lookup_fn_t)(pmix_pdata_t data[], size_t ndata,
                                          const pmix_info_t info[], size_t ninfo);

typedef pmix_status_t (*pmix_lookup_nb_fn_t)(char **keys, const pmix_info_t info[], size_t ninfo,
                                             pmix_lookup_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_unpublish_fn_t)(char **keys,
                                             const pmix_info_t info[], size_t ninfo);

typedef pmix_status_t (*pmix_unpublish_nb_fn_t)(char **keys,
                                                const pmix_info_t info[], size_t ninfo,
                                                pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_spawn_fn_t)(const pmix_info_t job_info[], size_t ninfo,
                                         const pmix_app_t apps[], size_t napps,
                                         pmix_nspace_t nspace);

typedef pmix_status_t (*pmix_spawn_nb_fn_t)(const pmix_info_t job_info[], size_t ninfo,
                                            const pmix_app_t apps[], size_t napps,
                                            pmix_spawn_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_connect_fn_t)(const pmix_proc_t procs[], size_t nprocs,
                                           const pmix_info_t info[], size_t ninfo);

typedef pmix_status_t (*pmix_connect_nb_fn_t)(const pmix_proc_t procs[], size_t nprocs,
                                              const pmix_info_t info[], size_t ninfo,
                                              pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_disconnect_fn_t)(const pmix_proc_t procs[], size_t nprocs,
                                              const pmix_info_t info[], size_t ninfo);

typedef pmix_status_t (*pmix_disconnect_nb_fn_t)(const pmix_proc_t procs[], size_t nprocs,
                                                 const pmix_info_t info[], size_t ninfo,
                                                 pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_resolve_peers_fn_t)(const char *nodename,
                                                 const pmix_nspace_t nspace,
                                                 pmix_proc_t **procs, size_t *nprocs);

typedef pmix_status_t (*pmix_resolve_nodes_fn_t)(const pmix_nspace_t nspace, char **nodelist);

typedef pmix_status_t (*pmix_query_info_fn_t)(pmix_query_t queries[], size_t nqueries,
                                              pmix_info_t **results, size_t *nresults);

typedef pmix_status_t (*pmix_query_info_nb_fn_t)(pmix_query_t queries[], size_t nqueries,
                                                 pmix_info_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_log_fn_t)(const pmix_info_t data[], size_t ndata,
                                       const pmix_info_t directives[], size_t ndirs);

typedef pmix_status_t (*pmix_log_nb_fn_t)(const pmix_info_t data[], size_t ndata,
                                          const pmix_info_t directives[], size_t ndirs,
                                          pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_allocation_request_fn_t)(pmix_alloc_directive_t directive,
                                                      pmix_info_t *info, size_t ninfo,
                                                      pmix_info_t **results, size_t *nresults);

typedef pmix_status_t (*pmix_allocation_request_nb_fn_t)(pmix_alloc_directive_t directive,
                                                         pmix_info_t *info, size_t ninfo,
                                                         pmix_info_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_job_control_fn_t)(const pmix_proc_t targets[], size_t ntargets,
                                               const pmix_info_t directives[], size_t ndirs,
                                               pmix_info_t **results, size_t *nresults);

typedef pmix_status_t (*pmix_job_control_nb_fn_t)(const pmix_proc_t targets[], size_t ntargets,
                                                  const pmix_info_t directives[], size_t ndirs,
                                                  pmix_info_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_process_monitor_fn_t)(const pmix_info_t *monitor, pmix_status_t error,
                                                   const pmix_info_t directives[], size_t ndirs,
                                                   pmix_info_t **results, size_t *nresults);

typedef pmix_status_t (*pmix_process_monitor_nb_fn_t)(const pmix_info_t *monitor, pmix_status_t error,
                                                      const pmix_info_t directives[], size_t ndirs,
                                                      pmix_info_cbfunc_t cbfunc, void *cbdata);

/* No function pointer for PMIx_Heartbeat() */

typedef pmix_status_t (*pmix_get_credential_fn_t)(const pmix_info_t info[], size_t ninfo,
                                                  pmix_byte_object_t *credential);

typedef pmix_status_t (*pmix_get_credential_nb_fn_t)(const pmix_info_t info[], size_t ninfo,
                                                     pmix_credential_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_validate_credential_fn_t)(const pmix_byte_object_t *cred,
                                                       const pmix_info_t info[], size_t ninfo,
                                                       pmix_info_t **results, size_t *nresults);

typedef pmix_status_t (*pmix_validate_credential_nb_fn_t)(const pmix_byte_object_t *cred,
                                                          const pmix_info_t info[], size_t ninfo,
                                                          pmix_validation_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_group_construct_fn_t)(const char grp[],
                                                   const pmix_proc_t procs[], size_t nprocs,
                                                   const pmix_info_t directives[], size_t ndirs,
                                                   pmix_info_t **results, size_t *nresults);

typedef pmix_status_t (*pmix_group_construct_nb_fn_t)(const char grp[],
                                                      const pmix_proc_t procs[], size_t nprocs,
                                                      const pmix_info_t info[], size_t ninfo,
                                                      pmix_info_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_group_invite_fn_t)(const char grp[],
                                                const pmix_proc_t procs[], size_t nprocs,
                                                const pmix_info_t info[], size_t ninfo,
                                                pmix_info_t **results, size_t *nresult);

typedef pmix_status_t (*pmix_group_invite_nb_fn_t)(const char grp[],
                                                   const pmix_proc_t procs[], size_t nprocs,
                                                   const pmix_info_t info[], size_t ninfo,
                                                   pmix_info_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_group_join_fn_t)(const char grp[],
                                              const pmix_proc_t *leader,
                                              pmix_group_opt_t opt,
                                              const pmix_info_t info[], size_t ninfo,
                                              pmix_info_t **results, size_t *nresult);

typedef pmix_status_t (*pmix_group_join_nb_fn_t)(const char grp[],
                                                 const pmix_proc_t *leader,
                                                 pmix_group_opt_t opt,
                                                 const pmix_info_t info[], size_t ninfo,
                                                 pmix_info_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_group_leave_fn_t)(const char grp[],
                                               const pmix_info_t info[], size_t ninfo);

typedef pmix_status_t (*pmix_group_leave_nb_fn_t)(const char grp[],
                                                  const pmix_info_t info[], size_t ninfo,
                                                  pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_group_destruct_fn_t)(const char grp[],
                                                  const pmix_info_t info[], size_t ninfo);

typedef pmix_status_t (*pmix_group_destruct_nb_fn_t)(const char grp[],
                                                     const pmix_info_t info[], size_t ninfo,
                                                     pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_register_event_handler_fn_t)(pmix_status_t codes[], size_t ncodes,
                                                          pmix_info_t info[], size_t ninfo,
                                                          pmix_notification_fn_t evhdlr,
                                                          pmix_hdlr_reg_cbfunc_t cbfunc,
                                                          void *cbdata);

typedef pmix_status_t (*pmix_deregister_event_handler_fn_t)(size_t evhdlr_ref,
                                                            pmix_op_cbfunc_t cbfunc,
                                                            void *cbdata);

typedef pmix_status_t (*pmix_notify_event_fn_t)(pmix_status_t status,
                                                const pmix_proc_t *source,
                                                pmix_data_range_t range,
                                                const pmix_info_t info[], size_t ninfo,
                                                pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_fabric_register_fn_t)(pmix_fabric_t *fabric,
                                                   const pmix_info_t directives[],
                                                   size_t ndirs);

typedef pmix_status_t (*pmix_fabric_register_nb_fn_t)(pmix_fabric_t *fabric,
                                                      const pmix_info_t directives[],
                                                      size_t ndirs,
                                                      pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_fabric_update_fn_t)(pmix_fabric_t *fabric);

typedef pmix_status_t (*pmix_fabric_update_nb_fn_t)(pmix_fabric_t *fabric,
                                                    pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_fabric_deregister_fn_t)(pmix_fabric_t *fabric);

typedef pmix_status_t (*pmix_fabric_deregister_nb_fn_t)(pmix_fabric_t *fabric,
                                                        pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_compute_distances_fn_t)(pmix_topology_t *topo,
                                                     pmix_cpuset_t *cpuset,
                                                     pmix_info_t info[], size_t ninfo,
                                                     pmix_device_distance_t *distances[],
                                                     size_t *ndist);

typedef pmix_status_t (*pmix_compute_distances_nb_fn_t)(pmix_topology_t *topo,
                                                        pmix_cpuset_t *cpuset,
                                                        pmix_info_t info[], size_t ninfo,
                                                        pmix_device_dist_cbfunc_t cbfunc,
                                                        void *cbdata);

typedef pmix_status_t (*pmix_load_topology_fn_t)(pmix_topology_t *topo);

typedef void (*pmix_topology_destruct_fn_t)(pmix_topology_t *topo);

typedef pmix_status_t (*pmix_parse_cpuset_string_fn_t)(const char *cpuset_string,
                                                       pmix_cpuset_t *cpuset);

typedef pmix_status_t (*pmix_get_cpuset_fn_t)(pmix_cpuset_t *cpuset,
                                              pmix_bind_envelope_t ref);

typedef pmix_status_t (*pmix_get_relative_locality_fn_t)(const char *locality1,
                                                         const char *locality2,
                                                         pmix_locality_t *locality);

typedef void (*pmix_progress_fn_t)(void);

typedef const char* (*pmix_error_string_fn_t)(pmix_status_t status);
typedef const char* (*pmix_proc_state_string_fn_t)(pmix_proc_state_t state);
typedef const char* (*pmix_scope_string_fn_t)(pmix_scope_t scope);
typedef const char* (*pmix_persistence_string_fn_t)(pmix_persistence_t persist);
typedef const char* (*pmix_data_range_string_fn_t)(pmix_data_range_t range);
typedef const char* (*pmix_info_directives_string_fn_t)(pmix_info_directives_t directives);
typedef const char* (*pmix_data_type_string_fn_t)(pmix_data_type_t type);
typedef const char* (*pmix_alloc_directive_string_fn_t)(pmix_alloc_directive_t directive);
typedef const char* (*pmix_iof_channel_string_fn_t)(pmix_iof_channel_t channel);
typedef const char* (*pmix_job_state_string_fn_t)(pmix_job_state_t state);
typedef const char* (*pmix_get_attribute_string_fn_t)(char *attribute);
typedef const char* (*pmix_get_attribute_name_fn_t)(char *attrstring);
typedef const char* (*pmix_link_state_string_fn_t)(pmix_link_state_t state);
typedef const char* (*pmix_device_type_string_fn_t)(pmix_device_type_t type);

typedef const char* (*pmix_get_version_fn_t)(void);

typedef pmix_status_t (*pmix_store_internal_fn_t)(const pmix_proc_t *proc,
                                                  const char key[], pmix_value_t *val);

typedef pmix_status_t (*pmix_data_pack_fn_t)(const pmix_proc_t *target,
                                             pmix_data_buffer_t *buffer,
                                             void *src, int32_t num_vals,
                                             pmix_data_type_t type);

typedef pmix_status_t (*pmix_data_unpack_fn_t)(const pmix_proc_t *source,
                                               pmix_data_buffer_t *buffer, void *dest,
                                               int32_t *max_num_values,
                                               pmix_data_type_t type);

typedef pmix_status_t (*pmix_data_copy_fn_t)(void **dest, void *src,
                                             pmix_data_type_t type);

typedef pmix_status_t (*pmix_data_print_fn_t)(char **output, char *prefix,
                                              void *src, pmix_data_type_t type);

typedef pmix_status_t (*pmix_data_copy_payload_fn_t)(pmix_data_buffer_t *dest,
                                                     pmix_data_buffer_t *src);

typedef pmix_status_t (*pmix_data_unload_fn_t)(pmix_data_buffer_t *buffer,
                                               pmix_byte_object_t *payload);

typedef pmix_status_t (*pmix_data_load_fn_t)(pmix_data_buffer_t *buffer,
                                             pmix_byte_object_t *payload);

typedef pmix_status_t (*pmix_data_embed_fn_t)(pmix_data_buffer_t *buffer,
                                              const pmix_byte_object_t *payload);

typedef bool (*pmix_data_compress_fn_t)(const uint8_t *inbytes,
                                        size_t size,
                                        uint8_t **outbytes,
                                        size_t *nbytes);

typedef bool (*pmix_data_decompress_fn_t)(const uint8_t *inbytes,
                                          size_t size,
                                          uint8_t **outbytes,
                                          size_t *nbytes);


/**** PMIx Tool functions ****/
typedef pmix_status_t (*pmix_tool_init_fn_t)(pmix_proc_t *proc,
                                             pmix_info_t info[], size_t ninfo);

typedef pmix_status_t (*pmix_tool_finalize_fn_t)(void);

typedef pmix_status_t (*pmix_tool_attach_to_server_fn_t)(pmix_proc_t *myproc, pmix_proc_t *server,
                                                         pmix_info_t info[], size_t ninfo);

typedef pmix_status_t (*pmix_tool_disconnect_fn_t)(const pmix_proc_t *server);

typedef pmix_status_t (*pmix_tool_get_servers_fn_t)(pmix_proc_t *servers[], size_t *nservers);

typedef pmix_status_t (*pmix_tool_set_server_fn_t)(const pmix_proc_t *server,
                                                   pmix_info_t info[], size_t ninfo);

typedef pmix_status_t (*pmix_iof_pull_fn_t)(const pmix_proc_t procs[], size_t nprocs,
                                            const pmix_info_t directives[], size_t ndirs,
                                            pmix_iof_channel_t channel, pmix_iof_cbfunc_t cbfunc,
                                            pmix_hdlr_reg_cbfunc_t regcbfunc, void *regcbdata);

typedef pmix_status_t (*pmix_iof_deregister_fn_t)(size_t iofhdlr,
                                                  const pmix_info_t directives[], size_t ndirs,
                                                  pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_iof_push_fn_t)(const pmix_proc_t targets[], size_t ntargets,
                                            pmix_byte_object_t *bo,
                                            const pmix_info_t directives[], size_t ndirs,
                                            pmix_op_cbfunc_t cbfunc, void *cbdata);


/**** PMIx Utility functions ****/
typedef pmix_status_t (*pmix_value_load_fn_t)(pmix_value_t *val,
                                              const void *data,
                                              pmix_data_type_t type);

typedef pmix_status_t (*pmix_value_unload_fn_t)(pmix_value_t *val,
                                                void **data,
                                                size_t *sz);

typedef pmix_status_t (*pmix_value_xfer_fn_t)(pmix_value_t *dest,
                                              const pmix_value_t *src);

typedef pmix_status_t (*pmix_info_load)(pmix_info_t *info,
                                        const char *key,
                                        const void *data,
                                        pmix_data_type_t type);

typedef pmix_status_t (*pmix_info_xfer)(pmix_info_t *dest,
                                        const pmix_info_t *src);

typedef void* (*pmix_info_list_start_fn_t)(void);

typedef pmix_status_t (*pmix_info_list_add_fn_t)(void *ptr,
                                                 const char *key,
                                                 const void *value,
                                                 pmix_data_type_t type);

typedef pmix_status_t (*pmix_info_list_xfer_fn_t)(void *ptr,
                                                  const pmix_info_t *info);

typedef pmix_status_t (*pmix_info_list_convert_fn_t)(void *ptr,
                                                     pmix_data_array_t *par);

typedef void (*pmix_info_list_release_fn_t)(void *ptr);


/**** PMIx Server Module functions ****/
typedef pmix_status_t (*pmix_server_client_connected_fn_t)(const pmix_proc_t *proc, void* server_object,
                                                           pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_client_connected2_fn_t)(const pmix_proc_t *proc, void* server_object,
                                                            pmix_info_t info[], size_t ninfo,
                                                            pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_client_finalized_fn_t)(const pmix_proc_t *proc, void* server_object,
                                                           pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_abort_fn_t)(const pmix_proc_t *proc, void *server_object,
                                                int status, const char msg[],
                                                pmix_proc_t procs[], size_t nprocs,
                                                pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_fencenb_fn_t)(const pmix_proc_t procs[], size_t nprocs,
                                                  const pmix_info_t info[], size_t ninfo,
                                                  char *data, size_t ndata,
                                                  pmix_modex_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_dmodex_req_fn_t)(const pmix_proc_t *proc,
                                                     const pmix_info_t info[], size_t ninfo,
                                                     pmix_modex_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_publish_fn_t)(const pmix_proc_t *proc,
                                                  const pmix_info_t info[], size_t ninfo,
                                                  pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_lookup_fn_t)(const pmix_proc_t *proc, char **keys,
                                                 const pmix_info_t info[], size_t ninfo,
                                                 pmix_lookup_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_unpublish_fn_t)(const pmix_proc_t *proc, char **keys,
                                                    const pmix_info_t info[], size_t ninfo,
                                                    pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_spawn_fn_t)(const pmix_proc_t *proc,
                                                const pmix_info_t job_info[], size_t ninfo,
                                                const pmix_app_t apps[], size_t napps,
                                                pmix_spawn_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_connect_fn_t)(const pmix_proc_t procs[], size_t nprocs,
                                                  const pmix_info_t info[], size_t ninfo,
                                                  pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_disconnect_fn_t)(const pmix_proc_t procs[], size_t nprocs,
                                                     const pmix_info_t info[], size_t ninfo,
                                                     pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_register_events_fn_t)(pmix_status_t *codes, size_t ncodes,
                                                          const pmix_info_t info[], size_t ninfo,
                                                          pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_deregister_events_fn_t)(pmix_status_t *codes, size_t ncodes,
                                                            pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_notify_event_fn_t)(pmix_status_t code,
                                                       const pmix_proc_t *source,
                                                       pmix_data_range_t range,
                                                       pmix_info_t info[], size_t ninfo,
                                                       pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_listener_fn_t)(int listening_sd,
                                                   pmix_connection_cbfunc_t cbfunc,
                                                   void *cbdata);

typedef pmix_status_t (*pmix_server_query_fn_t)(pmix_proc_t *proct,
                                                pmix_query_t *queries, size_t nqueries,
                                                pmix_info_cbfunc_t cbfunc,
                                                void *cbdata);

typedef void (*pmix_server_tool_connection_fn_t)(pmix_info_t *info, size_t ninfo,
                                                 pmix_tool_connection_cbfunc_t cbfunc,
                                                 void *cbdata);

typedef void (*pmix_server_log_fn_t)(const pmix_proc_t *client,
                                     const pmix_info_t data[], size_t ndata,
                                     const pmix_info_t directives[], size_t ndirs,
                                     pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_alloc_fn_t)(const pmix_proc_t *client,
                                                pmix_alloc_directive_t directive,
                                                const pmix_info_t data[], size_t ndata,
                                                pmix_info_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_job_control_fn_t)(const pmix_proc_t *requestor,
                                                      const pmix_proc_t targets[], size_t ntargets,
                                                      const pmix_info_t directives[], size_t ndirs,
                                                      pmix_info_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_monitor_fn_t)(const pmix_proc_t *requestor,
                                                  const pmix_info_t *monitor, pmix_status_t error,
                                                  const pmix_info_t directives[], size_t ndirs,
                                                  pmix_info_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_get_cred_fn_t)(const pmix_proc_t *proc,
                                                   const pmix_info_t directives[], size_t ndirs,
                                                   pmix_credential_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_validate_cred_fn_t)(const pmix_proc_t *proc,
                                                        const pmix_byte_object_t *cred,
                                                        const pmix_info_t directives[], size_t ndirs,
                                                        pmix_validation_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_iof_fn_t)(const pmix_proc_t procs[], size_t nprocs,
                                              const pmix_info_t directives[], size_t ndirs,
                                              pmix_iof_channel_t channels,
                                              pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_stdin_fn_t)(const pmix_proc_t *source,
                                                const pmix_proc_t targets[], size_t ntargets,
                                                const pmix_info_t directives[], size_t ndirs,
                                                const pmix_byte_object_t *bo,
                                                pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_grp_fn_t)(pmix_group_operation_t op, char grp[],
                                              const pmix_proc_t procs[], size_t nprocs,
                                              const pmix_info_t directives[], size_t ndirs,
                                              pmix_info_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_fabric_fn_t)(const pmix_proc_t *requestor,
                                                 pmix_fabric_operation_t op,
                                                 const pmix_info_t directives[], size_t ndirs,
                                                 pmix_info_cbfunc_t cbfunc, void *cbdata);


/**** PMIx Server Module ****/
typedef struct pmix_server_module_4_0_0_t {
    /* v1x interfaces */
    pmix_server_client_connected_fn_t   client_connected;
    pmix_server_client_finalized_fn_t   client_finalized;
    pmix_server_abort_fn_t              abort;
    pmix_server_fencenb_fn_t            fence_nb;
    pmix_server_dmodex_req_fn_t         direct_modex;
    pmix_server_publish_fn_t            publish;
    pmix_server_lookup_fn_t             lookup;
    pmix_server_unpublish_fn_t          unpublish;
    pmix_server_spawn_fn_t              spawn;
    pmix_server_connect_fn_t            connect;
    pmix_server_disconnect_fn_t         disconnect;
    pmix_server_register_events_fn_t    register_events;
    pmix_server_deregister_events_fn_t  deregister_events;
    pmix_server_listener_fn_t           listener;
    /* v2x interfaces */
    pmix_server_notify_event_fn_t       notify_event;
    pmix_server_query_fn_t              query;
    pmix_server_tool_connection_fn_t    tool_connected;
    pmix_server_log_fn_t                log;
    pmix_server_alloc_fn_t              allocate;
    pmix_server_job_control_fn_t        job_control;
    pmix_server_monitor_fn_t            monitor;
    /* v3x interfaces */
    pmix_server_get_cred_fn_t           get_credential;
    pmix_server_validate_cred_fn_t      validate_credential;
    pmix_server_iof_fn_t                iof_pull;
    pmix_server_stdin_fn_t              push_stdin;
    /* v4x interfaces */
    pmix_server_grp_fn_t                group;
    pmix_server_fabric_fn_t             fabric;
    pmix_server_client_connected2_fn_t  client_connected2;
} pmix_server_module_t;


/**** PMIx Server functions ****/
typedef pmix_status_t (*pmix_server_init_fn_t)(pmix_server_module_t *module,
                                               pmix_info_t info[], size_t ninfo);

typedef pmix_status_t (*pmix_server_finalize_fn_t)(void);

typedef pmix_status_t (*pmix_generate_regex_fn_t)(const char *input, char **regex);

typedef pmix_status_t (*pmix_generate_ppn_fn_t)(const char *input, char **ppn);

typedef pmix_status_t (*pmix_server_register_nspace_fn_t)(const pmix_nspace_t nspace, int nlocalprocs,
                                                          pmix_info_t info[], size_t ninfo,
                                                          pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_deregister_nspace_fn_t)(const pmix_nspace_t nspace,
                                                            pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_register_client_fn_t)(const pmix_proc_t *proc,
                                                          uid_t uid, gid_t gid,
                                                          void *server_object,
                                                          pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef void (*pmix_server_deregister_client_fn_t)(const pmix_proc_t *proc,
                                                   pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_setup_fork_fn_t)(const pmix_proc_t *proc, char ***env);

typedef pmix_status_t (*pmix_server_dmodex_request_fn_t)(const pmix_proc_t *proc,
                                                         pmix_dmodex_response_fn_t cbfunc,
                                                         void *cbdata);

typedef pmix_status_t (*pmix_server_setup_application_fn_t)(const pmix_nspace_t nspace,
                                                            pmix_info_t info[], size_t ninfo,
                                                            pmix_setup_application_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_setup_local_support_fn_t)(const pmix_nspace_t nspace,
                                                              pmix_info_t info[], size_t ninfo,
                                                              pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_iof_deliver_fn_t)(const pmix_proc_t *source,
                                                      pmix_iof_channel_t channel,
                                                      const pmix_byte_object_t *bo,
                                                      const pmix_info_t info[], size_t ninfo,
                                                      pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_collect_inventory_fn_t)(pmix_info_t directives[], size_t ndirs,
                                                            pmix_info_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_server_deliver_inventory_fn_t)(pmix_info_t info[], size_t ninfo,
                                                            pmix_info_t directives[], size_t ndirs,
                                                            pmix_op_cbfunc_t cbfunc, void *cbdata);

typedef pmix_status_t (*pmix_register_attributes_fn_t)(char *function, char *attrs[]);

typedef pmix_status_t (*pmix_server_generate_locality_string_fn_t)(const pmix_cpuset_t *cpuset,
                                                                   char **locality);

typedef pmix_status_t (*pmix_server_generate_cpuset_string_fn_t)(const pmix_cpuset_t *cpuset,
                                                                 char **cpuset_string);

typedef pmix_status_t (*pmix_server_define_process_set_fn_t)(const pmix_proc_t *members,
                                                             size_t nmembers, char *pset_name);

typedef pmix_status_t (*pmix_server_delete_process_set_fn_t)(char *pset_name);

typedef pmix_status_t (*pmix_server_register_resources_fn_t)(pmix_info_t info[], size_t ninfo,
                                                             pmix_op_cbfunc_t cbfunc,
                                                             void *cbdata);

typedef pmix_status_t (*pmix_server_register_resources_fn_t)(pmix_info_t info[], size_t ninfo,
                                                             pmix_op_cbfunc_t cbfunc,
                                                             void *cbdata);

/* PMIX_FNS_H */
#endif
