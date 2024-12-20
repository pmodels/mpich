/*
 * Copyright (c) 2013-2020 Intel, Inc.  All rights reserved.
 * Copyright (c) 2016      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
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
 * Copyright (c) 2021-2022 Nanook Consulting  All rights reserved.
 * Copyright (c) 2022      IBM Corporation.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef PMIX_H
#define PMIX_H

/* *******************************************************************
 * PMIx Standard types, constants, and callback functions
 *  - pmix_types.h is included by pmix_macros.h
 * PMIx Standard macros
 * *******************************************************************/
#include "pmix_macros.h"


#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif


/**** PMIx Client functions ****/
pmix_status_t PMIx_Init(pmix_proc_t *proc,
                                    pmix_info_t info[], size_t ninfo);

pmix_status_t PMIx_Finalize(const pmix_info_t info[], size_t ninfo);

int PMIx_Initialized(void);

pmix_status_t PMIx_Abort(int status, const char msg[],
                         pmix_proc_t procs[], size_t nprocs);

pmix_status_t PMIx_Put(pmix_scope_t scope, const char key[], pmix_value_t *val);

pmix_status_t PMIx_Commit(void);

pmix_status_t PMIx_Fence(const pmix_proc_t procs[], size_t nprocs,
                         const pmix_info_t info[], size_t ninfo);

pmix_status_t PMIx_Fence_nb(const pmix_proc_t procs[], size_t nprocs,
                            const pmix_info_t info[], size_t ninfo,
                            pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Get(const pmix_proc_t *proc, const char key[],
                       const pmix_info_t info[], size_t ninfo,
                       pmix_value_t **val);

pmix_status_t PMIx_Get_nb(const pmix_proc_t *proc, const char key[],
                          const pmix_info_t info[], size_t ninfo,
                          pmix_value_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Publish(const pmix_info_t info[], size_t ninfo);

pmix_status_t PMIx_Publish_nb(const pmix_info_t info[], size_t ninfo,
                              pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Lookup(pmix_pdata_t data[], size_t ndata,
                          const pmix_info_t info[], size_t ninfo);

pmix_status_t PMIx_Lookup_nb(char **keys, const pmix_info_t info[], size_t ninfo,
                             pmix_lookup_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Unpublish(char **keys,
                             const pmix_info_t info[], size_t ninfo);

pmix_status_t PMIx_Unpublish_nb(char **keys,
                                const pmix_info_t info[], size_t ninfo,
                                pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Spawn(const pmix_info_t job_info[], size_t ninfo,
                         const pmix_app_t apps[], size_t napps,
                         pmix_nspace_t nspace);

pmix_status_t PMIx_Spawn_nb(const pmix_info_t job_info[], size_t ninfo,
                            const pmix_app_t apps[], size_t napps,
                            pmix_spawn_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Connect(const pmix_proc_t procs[], size_t nprocs,
                           const pmix_info_t info[], size_t ninfo);

pmix_status_t PMIx_Connect_nb(const pmix_proc_t procs[], size_t nprocs,
                              const pmix_info_t info[], size_t ninfo,
                              pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Disconnect(const pmix_proc_t procs[], size_t nprocs,
                              const pmix_info_t info[], size_t ninfo);

pmix_status_t PMIx_Disconnect_nb(const pmix_proc_t ranges[], size_t nprocs,
                                 const pmix_info_t info[], size_t ninfo,
                                 pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Resolve_peers(const char *nodename,
                                 const pmix_nspace_t nspace,
                                 pmix_proc_t **procs, size_t *nprocs);

pmix_status_t PMIx_Resolve_nodes(const pmix_nspace_t nspace, char **nodelist);

pmix_status_t PMIx_Query_info(pmix_query_t queries[], size_t nqueries,
                              pmix_info_t **results, size_t *nresults);

pmix_status_t PMIx_Query_info_nb(pmix_query_t queries[], size_t nqueries,
                                 pmix_info_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Log(const pmix_info_t data[], size_t ndata,
                       const pmix_info_t directives[], size_t ndirs);

pmix_status_t PMIx_Log_nb(const pmix_info_t data[], size_t ndata,
                          const pmix_info_t directives[], size_t ndirs,
                          pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Allocation_request(pmix_alloc_directive_t directive,
                                      pmix_info_t *info, size_t ninfo,
                                      pmix_info_t **results, size_t *nresults);

pmix_status_t PMIx_Allocation_request_nb(pmix_alloc_directive_t directive,
                                         pmix_info_t *info, size_t ninfo,
                                         pmix_info_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Job_control(const pmix_proc_t targets[], size_t ntargets,
                               const pmix_info_t directives[], size_t ndirs,
                               pmix_info_t **results, size_t *nresults);

pmix_status_t PMIx_Job_control_nb(const pmix_proc_t targets[], size_t ntargets,
                                  const pmix_info_t directives[], size_t ndirs,
                                  pmix_info_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Process_monitor(const pmix_info_t *monitor, pmix_status_t error,
                                   const pmix_info_t directives[], size_t ndirs,
                                   pmix_info_t **results, size_t *nresults);

pmix_status_t PMIx_Process_monitor_nb(const pmix_info_t *monitor, pmix_status_t error,
                                      const pmix_info_t directives[], size_t ndirs,
                                      pmix_info_cbfunc_t cbfunc, void *cbdata);

/* define a special macro to simplify sending of a heartbeat */
#define PMIx_Heartbeat()                                                    \
    do {                                                                    \
        pmix_info_t _in;                                                    \
        PMIX_INFO_CONSTRUCT(&_in);                                          \
        PMIX_INFO_LOAD(&_in, PMIX_SEND_HEARTBEAT, NULL, PMIX_POINTER);      \
        PMIx_Process_monitor_nb(&_in, PMIX_SUCCESS, NULL, 0, NULL, NULL);   \
        PMIX_INFO_DESTRUCT(&_in);                                           \
    } while(0)

pmix_status_t PMIx_Get_credential(const pmix_info_t info[], size_t ninfo,
                                  pmix_byte_object_t *credential);

pmix_status_t PMIx_Get_credential_nb(const pmix_info_t info[], size_t ninfo,
                                     pmix_credential_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Validate_credential(const pmix_byte_object_t *cred,
                                       const pmix_info_t info[], size_t ninfo,
                                       pmix_info_t **results, size_t *nresults);

pmix_status_t PMIx_Validate_credential_nb(const pmix_byte_object_t *cred,
                                          const pmix_info_t info[], size_t ninfo,
                                          pmix_validation_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Group_construct(const char grp[],
                                   const pmix_proc_t procs[], size_t nprocs,
                                   const pmix_info_t directives[], size_t ndirs,
                                   pmix_info_t **results, size_t *nresults);

pmix_status_t PMIx_Group_construct_nb(const char grp[],
                                      const pmix_proc_t procs[], size_t nprocs,
                                      const pmix_info_t info[], size_t ninfo,
                                      pmix_info_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Group_invite(const char grp[],
                                const pmix_proc_t procs[], size_t nprocs,
                                const pmix_info_t info[], size_t ninfo,
                                pmix_info_t **results, size_t *nresult);

pmix_status_t PMIx_Group_invite_nb(const char grp[],
                                   const pmix_proc_t procs[], size_t nprocs,
                                   const pmix_info_t info[], size_t ninfo,
                                   pmix_info_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Group_join(const char grp[],
                              const pmix_proc_t *leader,
                              pmix_group_opt_t opt,
                              const pmix_info_t info[], size_t ninfo,
                              pmix_info_t **results, size_t *nresult);

pmix_status_t PMIx_Group_join_nb(const char grp[],
                                 const pmix_proc_t *leader,
                                 pmix_group_opt_t opt,
                                 const pmix_info_t info[], size_t ninfo,
                                 pmix_info_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Group_leave(const char grp[],
                               const pmix_info_t info[], size_t ninfo);

pmix_status_t PMIx_Group_leave_nb(const char grp[],
                                  const pmix_info_t info[], size_t ninfo,
                                  pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Group_destruct(const char grp[],
                                  const pmix_info_t info[], size_t ninfo);

pmix_status_t PMIx_Group_destruct_nb(const char grp[],
                                     const pmix_info_t info[], size_t ninfo,
                                     pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Register_event_handler(pmix_status_t codes[], size_t ncodes,
                                          pmix_info_t info[], size_t ninfo,
                                          pmix_notification_fn_t evhdlr,
                                          pmix_hdlr_reg_cbfunc_t cbfunc,
                                          void *cbdata);

pmix_status_t PMIx_Deregister_event_handler(size_t evhdlr_ref,
                                            pmix_op_cbfunc_t cbfunc,
                                            void *cbdata);

pmix_status_t PMIx_Notify_event(pmix_status_t status,
                                const pmix_proc_t *source,
                                pmix_data_range_t range,
                                const pmix_info_t info[], size_t ninfo,
                                pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Fabric_register(pmix_fabric_t *fabric,
                                   const pmix_info_t directives[],
                                   size_t ndirs);

pmix_status_t PMIx_Fabric_register_nb(pmix_fabric_t *fabric,
                                      const pmix_info_t directives[],
                                      size_t ndirs,
                                      pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Fabric_update(pmix_fabric_t *fabric);

pmix_status_t PMIx_Fabric_update_nb(pmix_fabric_t *fabric,
                                    pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Fabric_deregister(pmix_fabric_t *fabric);

pmix_status_t PMIx_Fabric_deregister_nb(pmix_fabric_t *fabric,
                                        pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Compute_distances(pmix_topology_t *topo,
                                     pmix_cpuset_t *cpuset,
                                     pmix_info_t info[], size_t ninfo,
                                     pmix_device_distance_t *distances[],
                                     size_t *ndist);

pmix_status_t PMIx_Compute_distances_nb(pmix_topology_t *topo,
                                        pmix_cpuset_t *cpuset,
                                        pmix_info_t info[], size_t ninfo,
                                        pmix_device_dist_cbfunc_t cbfunc,
                                        void *cbdata);

pmix_status_t PMIx_Load_topology(pmix_topology_t *topo);

void PMIx_Topology_destruct(pmix_topology_t *topo);

pmix_status_t PMIx_Parse_cpuset_string(const char *cpuset_string,
                                       pmix_cpuset_t *cpuset);

pmix_status_t PMIx_Get_cpuset(pmix_cpuset_t *cpuset,
                              pmix_bind_envelope_t ref);

pmix_status_t PMIx_Get_relative_locality(const char *locality1,
                                         const char *locality2,
                                         pmix_locality_t *locality);

void PMIx_Progress(void);

const char* PMIx_Error_string(pmix_status_t status);
const char* PMIx_Proc_state_string(pmix_proc_state_t state);
const char* PMIx_Scope_string(pmix_scope_t scope);
const char* PMIx_Persistence_string(pmix_persistence_t persist);
const char* PMIx_Data_range_string(pmix_data_range_t range);
const char* PMIx_Info_directives_string(pmix_info_directives_t directives);
const char* PMIx_Data_type_string(pmix_data_type_t type);
const char* PMIx_Alloc_directive_string(pmix_alloc_directive_t directive);
const char* PMIx_IOF_channel_string(pmix_iof_channel_t channel);
const char* PMIx_Job_state_string(pmix_job_state_t state);
const char* PMIx_Get_attribute_string(const char *attribute);
const char* PMIx_Get_attribute_name(const char *attrstring);
const char* PMIx_Link_state_string(pmix_link_state_t state);
const char* PMIx_Device_type_string(pmix_device_type_t type);

const char* PMIx_Get_version(void);

pmix_status_t PMIx_Store_internal(const pmix_proc_t *proc,
                                  const char key[], pmix_value_t *val);

pmix_status_t PMIx_Data_pack(const pmix_proc_t *target,
                             pmix_data_buffer_t *buffer,
                             void *src, int32_t num_vals,
                             pmix_data_type_t type);

pmix_status_t PMIx_Data_unpack(const pmix_proc_t *source,
                               pmix_data_buffer_t *buffer, void *dest,
                               int32_t *max_num_values,
                               pmix_data_type_t type);

pmix_status_t PMIx_Data_copy(void **dest, void *src,
                             pmix_data_type_t type);

pmix_status_t PMIx_Data_print(char **output, const char *prefix,
                              void *src, pmix_data_type_t type);

pmix_status_t PMIx_Data_copy_payload(pmix_data_buffer_t *dest,
                                     pmix_data_buffer_t *src);

pmix_status_t PMIx_Data_unload(pmix_data_buffer_t *buffer,
                               pmix_byte_object_t *payload);

pmix_status_t PMIx_Data_load(pmix_data_buffer_t *buffer,
                             pmix_byte_object_t *payload);

pmix_status_t PMIx_Data_embed(pmix_data_buffer_t *buffer,
                              const pmix_byte_object_t *payload);

bool PMIx_Data_compress(const uint8_t *inbytes,
                        size_t size,
                        uint8_t **outbytes,
                        size_t *nbytes);

bool PMIx_Data_decompress(const uint8_t *inbytes,
                          size_t size,
                          uint8_t **outbytes,
                          size_t *nbytes);


/**** PMIx Tool functions ****/
pmix_status_t PMIx_tool_init(pmix_proc_t *proc,
                             pmix_info_t info[], size_t ninfo);

pmix_status_t PMIx_tool_finalize(void);

pmix_status_t PMIx_tool_attach_to_server(pmix_proc_t *myproc, pmix_proc_t *server,
                                         pmix_info_t info[], size_t ninfo);

pmix_status_t PMIx_tool_disconnect(const pmix_proc_t *server);

pmix_status_t PMIx_tool_get_servers(pmix_proc_t *servers[], size_t *nservers);

pmix_status_t PMIx_tool_set_server(const pmix_proc_t *server,
                                   pmix_info_t info[], size_t ninfo);

pmix_status_t PMIx_IOF_pull(const pmix_proc_t procs[], size_t nprocs,
                            const pmix_info_t directives[], size_t ndirs,
                            pmix_iof_channel_t channel, pmix_iof_cbfunc_t cbfunc,
                            pmix_hdlr_reg_cbfunc_t regcbfunc, void *regcbdata);

pmix_status_t PMIx_IOF_deregister(size_t iofhdlr,
                                  const pmix_info_t directives[], size_t ndirs,
                                  pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_IOF_push(const pmix_proc_t targets[], size_t ntargets,
                            pmix_byte_object_t *bo,
                            const pmix_info_t directives[], size_t ndirs,
                            pmix_op_cbfunc_t cbfunc, void *cbdata);


/**** PMIx Utility functions ****/

pmix_status_t PMIx_Value_load(pmix_value_t *val,
                              const void *data,
                              pmix_data_type_t type);

pmix_status_t PMIx_Value_unload(pmix_value_t *val,
                                void **data,
                                size_t *sz);

pmix_status_t PMIx_Value_xfer(pmix_value_t *dest,
                              const pmix_value_t *src);

pmix_status_t PMIx_Info_load(pmix_info_t *info,
                             const char *key,
                             const void *data,
                             pmix_data_type_t type);

pmix_status_t PMIx_Info_xfer(pmix_info_t *dest,
                             const pmix_info_t *src);

void* PMIx_Info_list_start(void);

pmix_status_t PMIx_Info_list_add(void *ptr,
                                 const char *key,
                                 const void *value,
                                 pmix_data_type_t type);

pmix_status_t PMIx_Info_list_xfer(void *ptr,
                                  const pmix_info_t *info);

pmix_status_t PMIx_Info_list_convert(void *ptr,
                                     pmix_data_array_t *par);

void PMIx_Info_list_release(void *ptr);


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
pmix_status_t PMIx_server_init(pmix_server_module_t *module,
                               pmix_info_t info[], size_t ninfo);

pmix_status_t PMIx_server_finalize(void);

pmix_status_t PMIx_generate_regex(const char *input, char **regex);

pmix_status_t PMIx_generate_ppn(const char *input, char **ppn);

pmix_status_t PMIx_server_register_nspace(const pmix_nspace_t nspace, int nlocalprocs,
                                          pmix_info_t info[], size_t ninfo,
                                          pmix_op_cbfunc_t cbfunc, void *cbdata);

void PMIx_server_deregister_nspace(const pmix_nspace_t nspace,
                                   pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_server_register_client(const pmix_proc_t *proc,
                                          uid_t uid, gid_t gid,
                                          void *server_object,
                                          pmix_op_cbfunc_t cbfunc, void *cbdata);

void PMIx_server_deregister_client(const pmix_proc_t *proc,
                                   pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_server_setup_fork(const pmix_proc_t *proc, char ***env);

pmix_status_t PMIx_server_dmodex_request(const pmix_proc_t *proc,
                                         pmix_dmodex_response_fn_t cbfunc,
                                         void *cbdata);

pmix_status_t PMIx_server_setup_application(const pmix_nspace_t nspace,
                                            pmix_info_t info[], size_t ninfo,
                                            pmix_setup_application_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_server_setup_local_support(const pmix_nspace_t nspace,
                                              pmix_info_t info[], size_t ninfo,
                                              pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_server_IOF_deliver(const pmix_proc_t *source,
                                      pmix_iof_channel_t channel,
                                      const pmix_byte_object_t *bo,
                                      const pmix_info_t info[], size_t ninfo,
                                      pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_server_collect_inventory(pmix_info_t directives[], size_t ndirs,
                                            pmix_info_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_server_deliver_inventory(pmix_info_t info[], size_t ninfo,
                                            pmix_info_t directives[], size_t ndirs,
                                            pmix_op_cbfunc_t cbfunc, void *cbdata);

pmix_status_t PMIx_Register_attributes(const char *function, char *attrs[]);

pmix_status_t PMIx_server_generate_locality_string(const pmix_cpuset_t *cpuset,
                                                   char **locality);

pmix_status_t PMIx_server_generate_cpuset_string(const pmix_cpuset_t *cpuset,
                                                 char **cpuset_string);

pmix_status_t PMIx_server_define_process_set(const pmix_proc_t *members,
                                             size_t nmembers, const char *pset_name);

pmix_status_t PMIx_server_delete_process_set(const char *pset_name);

pmix_status_t PMIx_server_register_resources(pmix_info_t info[], size_t ninfo,
                                             pmix_op_cbfunc_t cbfunc,
                                             void *cbdata);

pmix_status_t PMIx_server_deregister_resources(pmix_info_t info[], size_t ninfo,
                                               pmix_op_cbfunc_t cbfunc,
                                               void *cbdata);
    
#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

/* PMIX_H */
#endif
