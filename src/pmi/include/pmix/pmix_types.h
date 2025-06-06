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
 * PMIx Standard types, constants, and callback functions
 */
#ifndef PMIX_TYPES_H
#define PMIX_TYPES_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h> /* for struct timeval */
#include <unistd.h> /* for uid_t and gid_t */
#include <sys/types.h> /* for uid_t and gid_t */

extern char **environ;

/* define maximum value and key sizes */
#define PMIX_MAX_NSLEN     255
#define PMIX_MAX_KEYLEN    511

/* define abstract types for namespaces and keys */
typedef char pmix_nspace_t[PMIX_MAX_NSLEN+1];
typedef char pmix_key_t[PMIX_MAX_KEYLEN+1];

/* define a type for rank values */
typedef uint32_t pmix_rank_t;

#define PMIX_RANK_UNDEF         UINT32_MAX
#define PMIX_RANK_WILDCARD      UINT32_MAX-1
#define PMIX_RANK_LOCAL_NODE    UINT32_MAX-2
#define PMIX_RANK_LOCAL_PEERS   UINT32_MAX-4
#define PMIX_RANK_INVALID       UINT32_MAX-3
#define PMIX_RANK_VALID         UINT32_MAX-50

#define PMIX_APP_WILDCARD  UINT32_MAX


/****  PMIX ENVIRONMENTAL PARAMETERS  ****/
#define PMIX_LAUNCHER_RNDZ_URI  "PMIX_LAUNCHER_RNDZ_URI"
#define PMIX_LAUNCHER_RNDZ_FILE "PMIX_LAUNCHER_RNDZ_FILE"
#define PMIX_KEEPALIVE_PIPE     "PMIX_KEEPALIVE_PIPE"


/**** PMIx Attributes ****/
#define PMIX_ATTR_UNDEF                     "pmix.undef"
#define PMIX_EXTERNAL_PROGRESS              "pmix.evext"
#define PMIX_SERVER_TOOL_SUPPORT            "pmix.srvr.tool"
#define PMIX_SERVER_REMOTE_CONNECTIONS      "pmix.srvr.remote"
#define PMIX_SERVER_SYSTEM_SUPPORT          "pmix.srvr.sys"
#define PMIX_SERVER_SESSION_SUPPORT         "pmix.srvr.sess"
#define PMIX_SERVER_TMPDIR                  "pmix.srvr.tmpdir"
#define PMIX_SYSTEM_TMPDIR                  "pmix.sys.tmpdir"
#define PMIX_SERVER_SHARE_TOPOLOGY          "pmix.srvr.share"
#define PMIX_SERVER_ENABLE_MONITORING       "pmix.srv.monitor"
#define PMIX_SERVER_NSPACE                  "pmix.srv.nspace"
#define PMIX_SERVER_RANK                    "pmix.srv.rank"
#define PMIX_SERVER_GATEWAY                 "pmix.srv.gway"
#define PMIX_SERVER_SCHEDULER               "pmix.srv.sched"
#define PMIX_SERVER_START_TIME              "pmix.srv.strtime"
#define PMIX_HOMOGENEOUS_SYSTEM             "pmix.homo"
#define PMIX_SINGLETON                      "pmix.singleton"
#define PMIX_TOOL_NSPACE                    "pmix.tool.nspace"
#define PMIX_TOOL_RANK                      "pmix.tool.rank"
#define PMIX_SERVER_PIDINFO                 "pmix.srvr.pidinfo"
#define PMIX_CONNECT_TO_SYSTEM              "pmix.cnct.sys"
#define PMIX_CONNECT_SYSTEM_FIRST           "pmix.cnct.sys.first"
#define PMIX_SERVER_URI                     "pmix.srvr.uri"
#define PMIX_SERVER_HOSTNAME                "pmix.srvr.host"
#define PMIX_CONNECT_MAX_RETRIES            "pmix.tool.mretries"
#define PMIX_CONNECT_RETRY_DELAY            "pmix.tool.retry"
#define PMIX_TOOL_DO_NOT_CONNECT            "pmix.tool.nocon"
#define PMIX_TOOL_CONNECT_OPTIONAL          "pmix.tool.conopt"
#define PMIX_LAUNCHER                       "pmix.tool.launcher"
#define PMIX_LAUNCHER_RENDEZVOUS_FILE       "pmix.tool.lncrnd"
#define PMIX_TOOL_ATTACHMENT_FILE           "pmix.tool.attach"
#define PMIX_PRIMARY_SERVER                 "pmix.pri.srvr"
#define PMIX_NOHUP                          "pmix.nohup"
#define PMIX_LAUNCHER_DAEMON                "pmix.lnch.dmn"
#define PMIX_EXEC_AGENT                     "pmix.exec.agnt"
#define PMIX_LAUNCH_DIRECTIVES              "pmix.lnch.dirs"
#define PMIX_USERID                         "pmix.euid"
#define PMIX_GRPID                          "pmix.egid"
#define PMIX_VERSION_INFO                   "pmix.version"
#define PMIX_REQUESTOR_IS_TOOL              "pmix.req.tool"
#define PMIX_REQUESTOR_IS_CLIENT            "pmix.req.client"
#define PMIX_PSET_NAME                      "pmix.pset.nm"
#define PMIX_PSET_NAMES                     "pmix.pset.nms"
#define PMIX_PSET_MEMBERS                   "pmix.pset.mems"
#define PMIX_REINCARNATION                  "pmix.reinc"
#define PMIX_PROGRAMMING_MODEL              "pmix.pgm.model"
#define PMIX_MODEL_LIBRARY_NAME             "pmix.mdl.name"
#define PMIX_MODEL_LIBRARY_VERSION          "pmix.mld.vrs"
#define PMIX_THREADING_MODEL                "pmix.threads"
#define PMIX_MODEL_NUM_THREADS              "pmix.mdl.nthrds"
#define PMIX_MODEL_NUM_CPUS                 "pmix.mdl.ncpu"
#define PMIX_MODEL_CPU_TYPE                 "pmix.mdl.cputype"
#define PMIX_MODEL_PHASE_NAME               "pmix.mdl.phase"
#define PMIX_MODEL_PHASE_TYPE               "pmix.mdl.ptype"
#define PMIX_MODEL_AFFINITY_POLICY          "pmix.mdl.tap"
#define PMIX_USOCK_DISABLE                  "pmix.usock.disable"
#define PMIX_SOCKET_MODE                    "pmix.sockmode"
#define PMIX_SINGLE_LISTENER                "pmix.sing.listnr"
#define PMIX_TCP_REPORT_URI                 "pmix.tcp.repuri"
#define PMIX_TCP_URI                        "pmix.tcp.uri"
#define PMIX_TCP_IF_INCLUDE                 "pmix.tcp.ifinclude"
#define PMIX_TCP_IF_EXCLUDE                 "pmix.tcp.ifexclude"
#define PMIX_TCP_IPV4_PORT                  "pmix.tcp.ipv4"
#define PMIX_TCP_IPV6_PORT                  "pmix.tcp.ipv6"
#define PMIX_TCP_DISABLE_IPV4               "pmix.tcp.disipv4"
#define PMIX_TCP_DISABLE_IPV6               "pmix.tcp.disipv6"
#define PMIX_CPUSET                         "pmix.cpuset"
#define PMIX_CPUSET_BITMAP                  "pmix.bitmap"
#define PMIX_CREDENTIAL                     "pmix.cred"
#define PMIX_SPAWNED                        "pmix.spawned"
#define PMIX_NODE_OVERSUBSCRIBED            "pmix.ndosub"
#define PMIX_TMPDIR                         "pmix.tmpdir"
#define PMIX_NSDIR                          "pmix.nsdir"
#define PMIX_PROCDIR                        "pmix.pdir"
#define PMIX_TDIR_RMCLEAN                   "pmix.tdir.rmclean"
#define PMIX_CLUSTER_ID                     "pmix.clid"
#define PMIX_PROCID                         "pmix.procid"
#define PMIX_NSPACE                         "pmix.nspace"
#define PMIX_JOBID                          "pmix.jobid"
#define PMIX_APPNUM                         "pmix.appnum"
#define PMIX_RANK                           "pmix.rank"
#define PMIX_GLOBAL_RANK                    "pmix.grank"
#define PMIX_APP_RANK                       "pmix.apprank"
#define PMIX_NPROC_OFFSET                   "pmix.offset"
#define PMIX_LOCAL_RANK                     "pmix.lrank"
#define PMIX_NODE_RANK                      "pmix.nrank"
#define PMIX_PACKAGE_RANK                   "pmix.pkgrank"
#define PMIX_LOCALLDR                       "pmix.lldr"
#define PMIX_APPLDR                         "pmix.aldr"
#define PMIX_PROC_PID                       "pmix.ppid"
#define PMIX_SESSION_ID                     "pmix.session.id"
#define PMIX_NODE_LIST                      "pmix.nlist"
#define PMIX_ALLOCATED_NODELIST             "pmix.alist"
#define PMIX_HOSTNAME                       "pmix.hname"
#define PMIX_HOSTNAME_ALIASES               "pmix.alias"
#define PMIX_HOSTNAME_KEEP_FQDN             "pmix.fqdn"
#define PMIX_NODEID                         "pmix.nodeid"
#define PMIX_LOCAL_PEERS                    "pmix.lpeers"
#define PMIX_LOCAL_PROCS                    "pmix.lprocs"
#define PMIX_LOCAL_CPUSETS                  "pmix.lcpus"
#define PMIX_PARENT_ID                      "pmix.parent"
#define PMIX_EXIT_CODE                      "pmix.exit.code"
#define PMIX_UNIV_SIZE                      "pmix.univ.size"
#define PMIX_JOB_SIZE                       "pmix.job.size"
#define PMIX_JOB_NUM_APPS                   "pmix.job.napps"
#define PMIX_APP_SIZE                       "pmix.app.size"
#define PMIX_LOCAL_SIZE                     "pmix.local.size"
#define PMIX_NODE_SIZE                      "pmix.node.size"
#define PMIX_MAX_PROCS                      "pmix.max.size"
#define PMIX_NUM_SLOTS                      "pmix.num.slots"
#define PMIX_NUM_NODES                      "pmix.num.nodes"
#define PMIX_NUM_ALLOCATED_NODES            "pmix.num.anodes"
#define PMIX_AVAIL_PHYS_MEMORY              "pmix.pmem"
#define PMIX_DAEMON_MEMORY                  "pmix.dmn.mem"
#define PMIX_CLIENT_AVG_MEMORY              "pmix.cl.mem.avg"
#define PMIX_TOPOLOGY2                      "pmix.topo2"
#define PMIX_LOCALITY_STRING                "pmix.locstr"
#define PMIX_COLLECT_DATA                   "pmix.collect"
#define PMIX_ALL_CLONES_PARTICIPATE         "pmix.clone.part"
#define PMIX_COLLECT_GENERATED_JOB_INFO     "pmix.collect.gen"
#define PMIX_TIMEOUT                        "pmix.timeout"
#define PMIX_IMMEDIATE                      "pmix.immediate"
#define PMIX_WAIT                           "pmix.wait"
#define PMIX_NOTIFY_COMPLETION              "pmix.notecomp"
#define PMIX_RANGE                          "pmix.range"
#define PMIX_PERSISTENCE                    "pmix.persist"
#define PMIX_DATA_SCOPE                     "pmix.scope"
#define PMIX_OPTIONAL                       "pmix.optional"
#define PMIX_GET_STATIC_VALUES              "pmix.get.static"
#define PMIX_GET_POINTER_VALUES             "pmix.get.pntrs"
#define PMIX_EMBED_BARRIER                  "pmix.embed.barrier"
#define PMIX_JOB_TERM_STATUS                "pmix.job.term.status"
#define PMIX_PROC_TERM_STATUS               "pmix.proc.term.status"
#define PMIX_PROC_STATE_STATUS              "pmix.proc.state"
#define PMIX_GET_REFRESH_CACHE              "pmix.get.refresh"
#define PMIX_ACCESS_PERMISSIONS             "pmix.aperms"
#define PMIX_ACCESS_USERIDS                 "pmix.auids"
#define PMIX_ACCESS_GRPIDS                  "pmix.agids"
#define PMIX_WAIT_FOR_CONNECTION            "pmix.wait.conn"
#define PMIX_REGISTER_NODATA                "pmix.reg.nodata"
#define PMIX_NODE_MAP                       "pmix.nmap"
#define PMIX_NODE_MAP_RAW                   "pmix.nmap.raw"
#define PMIX_PROC_MAP                       "pmix.pmap"
#define PMIX_PROC_MAP_RAW                   "pmix.pmap.raw"
#define PMIX_ANL_MAP                        "pmix.anlmap"
#define PMIX_APP_MAP_TYPE                   "pmix.apmap.type"
#define PMIX_APP_MAP_REGEX                  "pmix.apmap.regex"
#define PMIX_REQUIRED_KEY                   "pmix.req.key"
#define PMIX_LOCAL_COLLECTIVE_STATUS        "pmix.loc.col.st"
#define PMIX_EVENT_HDLR_NAME                "pmix.evname"
#define PMIX_EVENT_HDLR_FIRST               "pmix.evfirst"
#define PMIX_EVENT_HDLR_LAST                "pmix.evlast"
#define PMIX_EVENT_HDLR_FIRST_IN_CATEGORY   "pmix.evfirstcat"
#define PMIX_EVENT_HDLR_LAST_IN_CATEGORY    "pmix.evlastcat"
#define PMIX_EVENT_HDLR_BEFORE              "pmix.evbefore"
#define PMIX_EVENT_HDLR_AFTER               "pmix.evafter"
#define PMIX_EVENT_HDLR_PREPEND             "pmix.evprepend"
#define PMIX_EVENT_HDLR_APPEND              "pmix.evappend"
#define PMIX_EVENT_CUSTOM_RANGE             "pmix.evrange"
#define PMIX_EVENT_AFFECTED_PROC            "pmix.evproc"
#define PMIX_EVENT_AFFECTED_PROCS           "pmix.evaffected"
#define PMIX_EVENT_NON_DEFAULT              "pmix.evnondef"
#define PMIX_EVENT_RETURN_OBJECT            "pmix.evobject"
#define PMIX_EVENT_DO_NOT_CACHE             "pmix.evnocache"
#define PMIX_EVENT_SILENT_TERMINATION       "pmix.evsilentterm"
#define PMIX_EVENT_PROXY                    "pmix.evproxy"
#define PMIX_EVENT_TEXT_MESSAGE             "pmix.evtext"
#define PMIX_EVENT_TIMESTAMP                "pmix.evtstamp"
#define PMIX_EVENT_TERMINATE_SESSION        "pmix.evterm.sess"
#define PMIX_EVENT_TERMINATE_JOB            "pmix.evterm.job"
#define PMIX_EVENT_TERMINATE_NODE           "pmix.evterm.node"
#define PMIX_EVENT_TERMINATE_PROC           "pmix.evterm.proc"
#define PMIX_EVENT_ACTION_TIMEOUT           "pmix.evtimeout"
#define PMIX_PERSONALITY                    "pmix.pers"
#define PMIX_HOST                           "pmix.host"
#define PMIX_HOSTFILE                       "pmix.hostfile"
#define PMIX_ADD_HOST                       "pmix.addhost"
#define PMIX_ADD_HOSTFILE                   "pmix.addhostfile"
#define PMIX_PREFIX                         "pmix.prefix"
#define PMIX_WDIR                           "pmix.wdir"
#define PMIX_DISPLAY_MAP                    "pmix.dispmap"
#define PMIX_PPR                            "pmix.ppr"
#define PMIX_MAPBY                          "pmix.mapby"
#define PMIX_RANKBY                         "pmix.rankby"
#define PMIX_BINDTO                         "pmix.bindto"
#define PMIX_PRELOAD_BIN                    "pmix.preloadbin"
#define PMIX_PRELOAD_FILES                  "pmix.preloadfiles"
#define PMIX_STDIN_TGT                      "pmix.stdin"
#define PMIX_DEBUGGER_DAEMONS               "pmix.debugger"
#define PMIX_COSPAWN_APP                    "pmix.cospawn"
#define PMIX_SET_SESSION_CWD                "pmix.ssncwd"
#define PMIX_INDEX_ARGV                     "pmix.indxargv"
#define PMIX_CPUS_PER_PROC                  "pmix.cpuperproc"
#define PMIX_NO_PROCS_ON_HEAD               "pmix.nolocal"
#define PMIX_NO_OVERSUBSCRIBE               "pmix.noover"
#define PMIX_REPORT_BINDINGS                "pmix.repbind"
#define PMIX_CPU_LIST                       "pmix.cpulist"
#define PMIX_JOB_RECOVERABLE                "pmix.recover"
#define PMIX_JOB_CONTINUOUS                 "pmix.continuous"
#define PMIX_MAX_RESTARTS                   "pmix.maxrestarts"
#define PMIX_FWD_STDIN                      "pmix.fwd.stdin"
#define PMIX_FWD_STDOUT                     "pmix.fwd.stdout"
#define PMIX_FWD_STDERR                     "pmix.fwd.stderr"
#define PMIX_FWD_STDDIAG                    "pmix.fwd.stddiag"
#define PMIX_SPAWN_TOOL                     "pmix.spwn.tool"
#define PMIX_CMD_LINE                       "pmix.cmd.line"
#define PMIX_FORKEXEC_AGENT                 "pmix.fe.agnt"
#define PMIX_JOB_TIMEOUT                    "pmix.job.time"
#define PMIX_SPAWN_TIMEOUT                  "pmix.sp.time"
#define PMIX_TIMEOUT_STACKTRACES            "pmix.tim.stack"
#define PMIX_TIMEOUT_REPORT_STATE           "pmix.tim.state"
#define PMIX_APP_ARGV                       "pmix.app.argv"
#define PMIX_NOTIFY_JOB_EVENTS              "pmix.note.jev"
#define PMIX_NOTIFY_PROC_TERMINATION        "pmix.noteproc"
#define PMIX_NOTIFY_PROC_ABNORMAL_TERMINATION   "pmix.noteabproc"
#define PMIX_ENVARS_HARVESTED               "pmix.evar.hvstd"
#define PMIX_QUERY_SUPPORTED_KEYS           "pmix.qry.keys"
#define PMIX_QUERY_NAMESPACES               "pmix.qry.ns"
#define PMIX_QUERY_NAMESPACE_INFO           "pmix.qry.nsinfo"
#define PMIX_QUERY_JOB_STATUS               "pmix.qry.jst"
#define PMIX_QUERY_QUEUE_LIST               "pmix.qry.qlst"
#define PMIX_QUERY_QUEUE_STATUS             "pmix.qry.qst"
#define PMIX_QUERY_PROC_TABLE               "pmix.qry.ptable"
#define PMIX_QUERY_LOCAL_PROC_TABLE         "pmix.qry.lptable"
#define PMIX_QUERY_AUTHORIZATIONS           "pmix.qry.auths"
#define PMIX_QUERY_SPAWN_SUPPORT            "pmix.qry.spawn"
#define PMIX_QUERY_DEBUG_SUPPORT            "pmix.qry.debug"
#define PMIX_QUERY_MEMORY_USAGE             "pmix.qry.mem"
#define PMIX_QUERY_ALLOC_STATUS             "pmix.query.alloc"
#define PMIX_TIME_REMAINING                 "pmix.time.remaining"
#define PMIX_QUERY_NUM_PSETS                "pmix.qry.psetnum"
#define PMIX_QUERY_PSET_NAMES               "pmix.qry.psets"
#define PMIX_QUERY_PSET_MEMBERSHIP          "pmix.qry.pmems"
#define PMIX_QUERY_NUM_GROUPS               "pmix.qry.pgrpnum"
#define PMIX_QUERY_GROUP_NAMES              "pmix.qry.pgrp"
#define PMIX_QUERY_GROUP_MEMBERSHIP         "pmix.qry.pgrpmems"
#define PMIX_QUERY_ATTRIBUTE_SUPPORT        "pmix.qry.attrs"
#define PMIX_CLIENT_FUNCTIONS               "pmix.client.fns"
#define PMIX_SERVER_FUNCTIONS               "pmix.srvr.fns"
#define PMIX_TOOL_FUNCTIONS                 "pmix.tool.fns"
#define PMIX_HOST_FUNCTIONS                 "pmix.host.fns"
#define PMIX_QUERY_AVAIL_SERVERS            "pmix.qry.asrvrs"
#define PMIX_QUERY_QUALIFIERS               "pmix.qry.quals"
#define PMIX_QUERY_RESULTS                  "pmix.qry.res"
#define PMIX_QUERY_REFRESH_CACHE            "pmix.qry.rfsh"
#define PMIX_QUERY_LOCAL_ONLY               "pmix.qry.local"
#define PMIX_QUERY_REPORT_AVG               "pmix.qry.avg"
#define PMIX_QUERY_REPORT_MINMAX            "pmix.qry.minmax"
#define PMIX_CLIENT_ATTRIBUTES              "pmix.client.attrs"
#define PMIX_SERVER_ATTRIBUTES              "pmix.srvr.attrs"
#define PMIX_HOST_ATTRIBUTES                "pmix.host.attrs"
#define PMIX_TOOL_ATTRIBUTES                "pmix.tool.attrs"
#define PMIX_QUERY_SUPPORTED_QUALIFIERS     "pmix.qry.quals"
#define PMIX_SESSION_INFO                   "pmix.ssn.info"
#define PMIX_JOB_INFO                       "pmix.job.info"
#define PMIX_APP_INFO                       "pmix.app.info"
#define PMIX_NODE_INFO                      "pmix.node.info"
#define PMIX_SESSION_INFO_ARRAY             "pmix.ssn.arr"
#define PMIX_JOB_INFO_ARRAY                 "pmix.job.arr"
#define PMIX_APP_INFO_ARRAY                 "pmix.app.arr"
#define PMIX_PROC_INFO_ARRAY                "pmix.pdata"
#define PMIX_NODE_INFO_ARRAY                "pmix.node.arr"
#define PMIX_SERVER_INFO_ARRAY              "pmix.srv.arr"
#define PMIX_LOG_SOURCE                     "pmix.log.source"
#define PMIX_LOG_STDERR                     "pmix.log.stderr"
#define PMIX_LOG_STDOUT                     "pmix.log.stdout"
#define PMIX_LOG_SYSLOG                     "pmix.log.syslog"
#define PMIX_LOG_LOCAL_SYSLOG               "pmix.log.lsys"
#define PMIX_LOG_GLOBAL_SYSLOG              "pmix.log.gsys"
#define PMIX_LOG_SYSLOG_PRI                 "pmix.log.syspri"
#define PMIX_LOG_TIMESTAMP                  "pmix.log.tstmp"
#define PMIX_LOG_GENERATE_TIMESTAMP         "pmix.log.gtstmp"
#define PMIX_LOG_TAG_OUTPUT                 "pmix.log.tag"
#define PMIX_LOG_TIMESTAMP_OUTPUT           "pmix.log.tsout"
#define PMIX_LOG_XML_OUTPUT                 "pmix.log.xml"
#define PMIX_LOG_ONCE                       "pmix.log.once"
#define PMIX_LOG_MSG                        "pmix.log.msg"
#define PMIX_LOG_EMAIL                      "pmix.log.email"
#define PMIX_LOG_EMAIL_ADDR                 "pmix.log.emaddr"
#define PMIX_LOG_EMAIL_SENDER_ADDR          "pmix.log.emfaddr"
#define PMIX_LOG_EMAIL_SUBJECT              "pmix.log.emsub"
#define PMIX_LOG_EMAIL_MSG                  "pmix.log.emmsg"
#define PMIX_LOG_EMAIL_SERVER               "pmix.log.esrvr"
#define PMIX_LOG_EMAIL_SRVR_PORT            "pmix.log.esrvrprt"
#define PMIX_LOG_GLOBAL_DATASTORE           "pmix.log.gstore"
#define PMIX_LOG_JOB_RECORD                 "pmix.log.jrec"
#define PMIX_LOG_PROC_TERMINATION           "pmix.logproc"
#define PMIX_LOG_PROC_ABNORMAL_TERMINATION  "pmix.logabproc"
#define PMIX_LOG_JOB_EVENTS                 "pmix.log.jev"
#define PMIX_LOG_COMPLETION                 "pmix.logcomp"
#define PMIX_DEBUG_STOP_ON_EXEC             "pmix.dbg.exec"
#define PMIX_DEBUG_STOP_IN_INIT             "pmix.dbg.init"
#define PMIX_DEBUG_STOP_IN_APP              "pmix.dbg.notify"
#define PMIX_BREAKPOINT                     "pmix.brkpnt"
#define PMIX_DEBUG_TARGET                   "pmix.dbg.tgt"
#define PMIX_DEBUG_DAEMONS_PER_PROC         "pmix.dbg.dpproc"
#define PMIX_DEBUG_DAEMONS_PER_NODE         "pmix.dbg.dpnd"
#define PMIX_RM_NAME                        "pmix.rm.name"
#define PMIX_RM_VERSION                     "pmix.rm.version"
#define PMIX_SET_ENVAR                      "pmix.envar.set"
#define PMIX_ADD_ENVAR                      "pmix.envar.add"
#define PMIX_UNSET_ENVAR                    "pmix.envar.unset"
#define PMIX_PREPEND_ENVAR                  "pmix.envar.prepnd"
#define PMIX_APPEND_ENVAR                   "pmix.envar.appnd"
#define PMIX_FIRST_ENVAR                    "pmix.envar.first"
#define PMIX_ALLOC_REQ_ID                   "pmix.alloc.reqid"
#define PMIX_ALLOC_ID                       "pmix.alloc.id"
#define PMIX_ALLOC_NUM_NODES                "pmix.alloc.nnodes"
#define PMIX_ALLOC_NODE_LIST                "pmix.alloc.nlist"
#define PMIX_ALLOC_NUM_CPUS                 "pmix.alloc.ncpus"
#define PMIX_ALLOC_NUM_CPU_LIST             "pmix.alloc.ncpulist"
#define PMIX_ALLOC_CPU_LIST                 "pmix.alloc.cpulist"
#define PMIX_ALLOC_MEM_SIZE                 "pmix.alloc.msize"
#define PMIX_ALLOC_FABRIC                   "pmix.alloc.net"
#define PMIX_ALLOC_FABRIC_ID                "pmix.alloc.netid"
#define PMIX_ALLOC_BANDWIDTH                "pmix.alloc.bw"
#define PMIX_ALLOC_FABRIC_QOS               "pmix.alloc.netqos"
#define PMIX_ALLOC_TIME                     "pmix.alloc.time"
#define PMIX_ALLOC_FABRIC_TYPE              "pmix.alloc.nettype"
#define PMIX_ALLOC_FABRIC_PLANE             "pmix.alloc.netplane"
#define PMIX_ALLOC_FABRIC_ENDPTS            "pmix.alloc.endpts"
#define PMIX_ALLOC_FABRIC_ENDPTS_NODE       "pmix.alloc.endpts.nd"
#define PMIX_ALLOC_FABRIC_SEC_KEY           "pmix.alloc.nsec"
#define PMIX_ALLOC_QUEUE                    "pmix.alloc.queue"
#define PMIX_JOB_CTRL_ID                    "pmix.jctrl.id"
#define PMIX_JOB_CTRL_PAUSE                 "pmix.jctrl.pause"
#define PMIX_JOB_CTRL_RESUME                "pmix.jctrl.resume"
#define PMIX_JOB_CTRL_CANCEL                "pmix.jctrl.cancel"
#define PMIX_JOB_CTRL_KILL                  "pmix.jctrl.kill"
#define PMIX_JOB_CTRL_RESTART               "pmix.jctrl.restart"
#define PMIX_JOB_CTRL_CHECKPOINT            "pmix.jctrl.ckpt"
#define PMIX_JOB_CTRL_CHECKPOINT_EVENT      "pmix.jctrl.ckptev"
#define PMIX_JOB_CTRL_CHECKPOINT_SIGNAL     "pmix.jctrl.ckptsig"
#define PMIX_JOB_CTRL_CHECKPOINT_TIMEOUT    "pmix.jctrl.ckptsig"
#define PMIX_JOB_CTRL_CHECKPOINT_METHOD     "pmix.jctrl.ckmethod"
#define PMIX_JOB_CTRL_SIGNAL                "pmix.jctrl.sig"
#define PMIX_JOB_CTRL_PROVISION             "pmix.jctrl.pvn"
#define PMIX_JOB_CTRL_PROVISION_IMAGE       "pmix.jctrl.pvnimg"
#define PMIX_JOB_CTRL_PREEMPTIBLE           "pmix.jctrl.preempt"
#define PMIX_JOB_CTRL_TERMINATE             "pmix.jctrl.term"
#define PMIX_REGISTER_CLEANUP               "pmix.reg.cleanup"
#define PMIX_REGISTER_CLEANUP_DIR           "pmix.reg.cleanupdir"
#define PMIX_CLEANUP_RECURSIVE              "pmix.clnup.recurse"
#define PMIX_CLEANUP_EMPTY                  "pmix.clnup.empty"
#define PMIX_CLEANUP_IGNORE                 "pmix.clnup.ignore"
#define PMIX_CLEANUP_LEAVE_TOPDIR           "pmix.clnup.lvtop"
#define PMIX_MONITOR_ID                     "pmix.monitor.id"
#define PMIX_MONITOR_CANCEL                 "pmix.monitor.cancel"
#define PMIX_MONITOR_APP_CONTROL            "pmix.monitor.appctrl"
#define PMIX_MONITOR_HEARTBEAT              "pmix.monitor.mbeat"
#define PMIX_SEND_HEARTBEAT                 "pmix.monitor.beat"
#define PMIX_MONITOR_HEARTBEAT_TIME         "pmix.monitor.btime"
#define PMIX_MONITOR_HEARTBEAT_DROPS        "pmix.monitor.bdrop"
#define PMIX_MONITOR_FILE                   "pmix.monitor.fmon"
#define PMIX_MONITOR_FILE_SIZE              "pmix.monitor.fsize"
#define PMIX_MONITOR_FILE_ACCESS            "pmix.monitor.faccess"
#define PMIX_MONITOR_FILE_MODIFY            "pmix.monitor.fmod"
#define PMIX_MONITOR_FILE_CHECK_TIME        "pmix.monitor.ftime"
#define PMIX_MONITOR_FILE_DROPS             "pmix.monitor.fdrop"
#define PMIX_CRED_TYPE                      "pmix.sec.ctype"
#define PMIX_CRYPTO_KEY                     "pmix.sec.key"
#define PMIX_IOF_CACHE_SIZE                 "pmix.iof.csize"
#define PMIX_IOF_DROP_OLDEST                "pmix.iof.old"
#define PMIX_IOF_DROP_NEWEST                "pmix.iof.new"
#define PMIX_IOF_BUFFERING_SIZE             "pmix.iof.bsize"
#define PMIX_IOF_BUFFERING_TIME             "pmix.iof.btime"
#define PMIX_IOF_COMPLETE                   "pmix.iof.cmp"
#define PMIX_IOF_PUSH_STDIN                 "pmix.iof.stdin"
#define PMIX_IOF_TAG_OUTPUT                 "pmix.iof.tag"
#define PMIX_IOF_RANK_OUTPUT                "pmix.iof.rank"
#define PMIX_IOF_TIMESTAMP_OUTPUT           "pmix.iof.ts"
#define PMIX_IOF_MERGE_STDERR_STDOUT        "pmix.iof.mrg"
#define PMIX_IOF_XML_OUTPUT                 "pmix.iof.xml"
#define PMIX_IOF_OUTPUT_TO_FILE             "pmix.iof.file"
#define PMIX_IOF_FILE_PATTERN               "pmix.iof.fpt"
#define PMIX_IOF_OUTPUT_TO_DIRECTORY        "pmix.iof.dir"
#define PMIX_IOF_FILE_ONLY                  "pmix.iof.fonly"
#define PMIX_IOF_COPY                       "pmix.iof.cpy"
#define PMIX_IOF_REDIRECT                   "pmix.iof.redir"
#define PMIX_IOF_LOCAL_OUTPUT               "pmix.iof.local"
#define PMIX_SETUP_APP_ENVARS               "pmix.setup.env"
#define PMIX_SETUP_APP_NONENVARS            "pmix.setup.nenv"
#define PMIX_SETUP_APP_ALL                  "pmix.setup.all"
#define PMIX_GROUP_ID                       "pmix.grp.id"
#define PMIX_GROUP_LEADER                   "pmix.grp.ldr"
#define PMIX_GROUP_OPTIONAL                 "pmix.grp.opt"
#define PMIX_GROUP_NOTIFY_TERMINATION       "pmix.grp.notterm"
#define PMIX_GROUP_FT_COLLECTIVE            "pmix.grp.ftcoll"
#define PMIX_GROUP_MEMBERSHIP               "pmix.grp.mbrs"
#define PMIX_GROUP_ASSIGN_CONTEXT_ID        "pmix.grp.actxid"
#define PMIX_GROUP_CONTEXT_ID               "pmix.grp.ctxid"
#define PMIX_GROUP_LOCAL_ONLY               "pmix.grp.lcl"
#define PMIX_GROUP_ENDPT_DATA               "pmix.grp.endpt"
#define PMIX_GROUP_NAMES                    "pmix.pgrp.nm"
#define PMIX_QUERY_STORAGE_LIST             "pmix.strg.list"
#define PMIX_STORAGE_CAPACITY_LIMIT         "pmix.strg.cap"
#define PMIX_STORAGE_OBJECT_LIMIT           "pmix.strg.obj"
#define PMIX_STORAGE_ID                     "pmix.strg.id"
#define PMIX_STORAGE_PATH                   "pmix.strg.path"
#define PMIX_STORAGE_TYPE                   "pmix.strg.type"
#define PMIX_STORAGE_ACCESSIBILITY          "pmix.strg.access"
#define PMIX_STORAGE_ACCESS_TYPE            "pmix.strg.atype"
#define PMIX_STORAGE_BW_CUR                 "pmix.strg.bwcur"
#define PMIX_STORAGE_BW_MAX                 "pmix.strg.bwmax"
#define PMIX_STORAGE_CAPACITY_USED          "pmix.strg.capuse"
#define PMIX_STORAGE_IOPS_CUR               "pmix.strg.iopscur"
#define PMIX_STORAGE_IOPS_MAX               "pmix.strg.iopsmax"
#define PMIX_STORAGE_MEDIUM                 "pmix.strg.medium"
#define PMIX_STORAGE_MINIMAL_XFER_SIZE      "pmix.strg.minxfer"
#define PMIX_STORAGE_OBJECTS_USED           "pmix.strg.objuse"
#define PMIX_STORAGE_PERSISTENCE            "pmix.strg.persist"
#define PMIX_STORAGE_SUGGESTED_XFER_SIZE    "pmix.strg.sxfer"
#define PMIX_STORAGE_VERSION                "pmix.strg.ver"
#define PMIX_FABRIC_COST_MATRIX             "pmix.fab.cm"
#define PMIX_FABRIC_GROUPS                  "pmix.fab.grps"
#define PMIX_FABRIC_VENDOR                  "pmix.fab.vndr"
#define PMIX_FABRIC_IDENTIFIER              "pmix.fab.id"
#define PMIX_FABRIC_INDEX                   "pmix.fab.idx"
#define PMIX_FABRIC_COORDINATES             "pmix.fab.coord"
#define PMIX_FABRIC_DEVICE_VENDORID         "pmix.fabdev.vendid"
#define PMIX_FABRIC_NUM_DEVICES             "pmix.fab.nverts"
#define PMIX_FABRIC_DIMS                    "pmix.fab.dims"
#define PMIX_FABRIC_PLANE                   "pmix.fab.plane"
#define PMIX_FABRIC_SWITCH                  "pmix.fab.switch"
#define PMIX_FABRIC_ENDPT                   "pmix.fab.endpt"
#define PMIX_FABRIC_SHAPE                   "pmix.fab.shape"
#define PMIX_FABRIC_SHAPE_STRING            "pmix.fab.shapestr"
#define PMIX_SWITCH_PEERS                   "pmix.speers"
#define PMIX_FABRIC_DEVICE                  "pmix.fabdev"
#define PMIX_FABRIC_DEVICES                 "pmix.fab.devs"
#define PMIX_FABRIC_DEVICE_NAME             "pmix.fabdev.nm"
#define PMIX_FABRIC_DEVICE_INDEX            "pmix.fabdev.idx"
#define PMIX_FABRIC_DEVICE_VENDOR           "pmix.fabdev.vndr"
#define PMIX_FABRIC_DEVICE_DRIVER           "pmix.fabdev.driver"
#define PMIX_FABRIC_DEVICE_FIRMWARE         "pmix.fabdev.fmwr"
#define PMIX_FABRIC_DEVICE_ADDRESS          "pmix.fabdev.addr"
#define PMIX_FABRIC_DEVICE_COORDINATES      "pmix.fab.coord"
#define PMIX_FABRIC_DEVICE_MTU              "pmix.fabdev.mtu"
#define PMIX_FABRIC_DEVICE_SPEED            "pmix.fabdev.speed"
#define PMIX_FABRIC_DEVICE_STATE            "pmix.fabdev.state"
#define PMIX_FABRIC_DEVICE_TYPE             "pmix.fabdev.type"
#define PMIX_FABRIC_DEVICE_PCI_DEVID        "pmix.fabdev.pcidevid"
#define PMIX_DEVICE_DISTANCES               "pmix.dev.dist"
#define PMIX_DEVICE_TYPE                    "pmix.dev.type"
#define PMIX_DEVICE_ID                      "pmix.dev.id"
#define PMIX_MAX_VALUE                      "pmix.descr.maxval"
#define PMIX_MIN_VALUE                      "pmix.descr.minval"
#define PMIX_ENUM_VALUE                     "pmix.descr.enum"


typedef uint8_t pmix_proc_state_t;
#define PMIX_PROC_STATE_UNDEF                    0
#define PMIX_PROC_STATE_PREPPED                  1
#define PMIX_PROC_STATE_LAUNCH_UNDERWAY          2
#define PMIX_PROC_STATE_RESTART                  3
#define PMIX_PROC_STATE_TERMINATE                4
#define PMIX_PROC_STATE_RUNNING                  5
#define PMIX_PROC_STATE_CONNECTED                6
#define PMIX_PROC_STATE_UNTERMINATED            15
#define PMIX_PROC_STATE_TERMINATED              20
#define PMIX_PROC_STATE_ERROR                   50
#define PMIX_PROC_STATE_KILLED_BY_CMD           (PMIX_PROC_STATE_ERROR +  1)
#define PMIX_PROC_STATE_ABORTED                 (PMIX_PROC_STATE_ERROR +  2)
#define PMIX_PROC_STATE_FAILED_TO_START         (PMIX_PROC_STATE_ERROR +  3)
#define PMIX_PROC_STATE_ABORTED_BY_SIG          (PMIX_PROC_STATE_ERROR +  4)
#define PMIX_PROC_STATE_TERM_WO_SYNC            (PMIX_PROC_STATE_ERROR +  5)
#define PMIX_PROC_STATE_COMM_FAILED             (PMIX_PROC_STATE_ERROR +  6)
#define PMIX_PROC_STATE_SENSOR_BOUND_EXCEEDED   (PMIX_PROC_STATE_ERROR +  7)
#define PMIX_PROC_STATE_CALLED_ABORT            (PMIX_PROC_STATE_ERROR +  8)
#define PMIX_PROC_STATE_HEARTBEAT_FAILED        (PMIX_PROC_STATE_ERROR +  9)
#define PMIX_PROC_STATE_MIGRATING               (PMIX_PROC_STATE_ERROR + 10)
#define PMIX_PROC_STATE_CANNOT_RESTART          (PMIX_PROC_STATE_ERROR + 11)
#define PMIX_PROC_STATE_TERM_NON_ZERO           (PMIX_PROC_STATE_ERROR + 12)
#define PMIX_PROC_STATE_FAILED_TO_LAUNCH        (PMIX_PROC_STATE_ERROR + 13)


typedef uint8_t pmix_job_state_t;
#define PMIX_JOB_STATE_UNDEF                     0
#define PMIX_JOB_STATE_AWAITING_ALLOC            1
#define PMIX_JOB_STATE_LAUNCH_UNDERWAY           2
#define PMIX_JOB_STATE_RUNNING                   3
#define PMIX_JOB_STATE_SUSPENDED                 4
#define PMIX_JOB_STATE_CONNECTED                 5
#define PMIX_JOB_STATE_UNTERMINATED             15
#define PMIX_JOB_STATE_TERMINATED               20
#define PMIX_JOB_STATE_TERMINATED_WITH_ERROR    50


typedef int pmix_status_t;
#define PMIX_SUCCESS                                 0
#define PMIX_ERROR                                  -1
#define PMIX_ERR_PROC_RESTART                       -4
#define PMIX_ERR_PROC_CHECKPOINT                    -5
#define PMIX_ERR_PROC_MIGRATE                       -6
#define PMIX_ERR_EXISTS                             -11
#define PMIX_ERR_INVALID_CRED                       -12
#define PMIX_ERR_WOULD_BLOCK                        -15
#define PMIX_ERR_UNKNOWN_DATA_TYPE                  -16
#define PMIX_ERR_TYPE_MISMATCH                      -18
#define PMIX_ERR_UNPACK_INADEQUATE_SPACE            -19
#define PMIX_ERR_UNPACK_FAILURE                     -20
#define PMIX_ERR_PACK_FAILURE                       -21
#define PMIX_ERR_NO_PERMISSIONS                     -23
#define PMIX_ERR_TIMEOUT                            -24
#define PMIX_ERR_UNREACH                            -25
#define PMIX_ERR_BAD_PARAM                          -27
#define PMIX_ERR_RESOURCE_BUSY                      -28
#define PMIX_ERR_OUT_OF_RESOURCE                    -29
#define PMIX_ERR_INIT                               -31
#define PMIX_ERR_NOMEM                              -32
#define PMIX_ERR_NOT_FOUND                          -46
#define PMIX_ERR_NOT_SUPPORTED                      -47
#define PMIX_ERR_PARAM_VALUE_NOT_SUPPORTED          -59
#define PMIX_ERR_COMM_FAILURE                       -49
#define PMIX_ERR_UNPACK_READ_PAST_END_OF_BUFFER     -50
#define PMIX_ERR_CONFLICTING_CLEANUP_DIRECTIVES     -51
#define PMIX_ERR_PARTIAL_SUCCESS                    -52
#define PMIX_ERR_DUPLICATE_KEY                      -53
#define PMIX_ERR_EMPTY                              -60
#define PMIX_ERR_LOST_CONNECTION                    -61
#define PMIX_ERR_EXISTS_OUTSIDE_SCOPE               -62
#define PMIX_PROCESS_SET_DEFINE                     -55
#define PMIX_PROCESS_SET_DELETE                     -56
#define PMIX_DEBUGGER_RELEASE                       -3
#define PMIX_READY_FOR_DEBUG                        -58
#define PMIX_QUERY_PARTIAL_SUCCESS                  -104
#define PMIX_JCTRL_CHECKPOINT                       -106
#define PMIX_JCTRL_CHECKPOINT_COMPLETE              -107
#define PMIX_JCTRL_PREEMPT_ALERT                    -108
#define PMIX_MONITOR_HEARTBEAT_ALERT                -109
#define PMIX_MONITOR_FILE_ALERT                     -110
#define PMIX_PROC_TERMINATED                        -111
#define PMIX_ERR_EVENT_REGISTRATION                 -144
#define PMIX_MODEL_DECLARED                         -147
#define PMIX_MODEL_RESOURCES                        -151
#define PMIX_OPENMP_PARALLEL_ENTERED                -152
#define PMIX_OPENMP_PARALLEL_EXITED                 -153
#define PMIX_LAUNCHER_READY                         -155
#define PMIX_OPERATION_IN_PROGRESS                  -156
#define PMIX_OPERATION_SUCCEEDED                    -157
#define PMIX_ERR_INVALID_OPERATION                  -158
#define PMIX_GROUP_INVITED                          -159
#define PMIX_GROUP_LEFT                             -160
#define PMIX_GROUP_INVITE_ACCEPTED                  -161
#define PMIX_GROUP_INVITE_DECLINED                  -162
#define PMIX_GROUP_INVITE_FAILED                    -163
#define PMIX_GROUP_MEMBERSHIP_UPDATE                -164
#define PMIX_GROUP_CONSTRUCT_ABORT                  -165
#define PMIX_GROUP_CONSTRUCT_COMPLETE               -166
#define PMIX_GROUP_LEADER_SELECTED                  -167
#define PMIX_GROUP_LEADER_FAILED                    -168
#define PMIX_GROUP_CONTEXT_ID_ASSIGNED              -169
#define PMIX_GROUP_MEMBER_FAILED                    -170
#define PMIX_ERR_REPEAT_ATTR_REGISTRATION           -171
#define PMIX_ERR_IOF_FAILURE                        -172
#define PMIX_ERR_IOF_COMPLETE                       -173
#define PMIX_LAUNCH_COMPLETE                        -174
#define PMIX_FABRIC_UPDATED                         -175
#define PMIX_FABRIC_UPDATE_PENDING                  -176
#define PMIX_FABRIC_UPDATE_ENDPOINTS                -113
#define PMIX_ERR_JOB_APP_NOT_EXECUTABLE             -177
#define PMIX_ERR_JOB_NO_EXE_SPECIFIED               -178
#define PMIX_ERR_JOB_FAILED_TO_MAP                  -179
#define PMIX_ERR_JOB_CANCELED                       -180
#define PMIX_ERR_JOB_FAILED_TO_LAUNCH               -181
#define PMIX_ERR_JOB_ABORTED                        -182
#define PMIX_ERR_JOB_KILLED_BY_CMD                  -183
#define PMIX_ERR_JOB_ABORTED_BY_SIG                 -184
#define PMIX_ERR_JOB_TERM_WO_SYNC                   -185
#define PMIX_ERR_JOB_SENSOR_BOUND_EXCEEDED          -186
#define PMIX_ERR_JOB_NON_ZERO_TERM                  -187
#define PMIX_ERR_JOB_ALLOC_FAILED                   -188
#define PMIX_ERR_JOB_ABORTED_BY_SYS_EVENT           -189
#define PMIX_ERR_JOB_EXE_NOT_FOUND                  -190
#define PMIX_ERR_JOB_WDIR_NOT_FOUND                 -233
#define PMIX_ERR_JOB_INSUFFICIENT_RESOURCES         -234
#define PMIX_ERR_JOB_SYS_OP_FAILED                  -235
#define PMIX_EVENT_JOB_START                        -191
#define PMIX_EVENT_JOB_END                          -145
#define PMIX_EVENT_SESSION_START                    -192
#define PMIX_EVENT_SESSION_END                      -193
#define PMIX_ERR_PROC_TERM_WO_SYNC                  -200
#define PMIX_EVENT_PROC_TERMINATED                  -201
#define PMIX_EVENT_SYS_BASE                         -230
#define PMIX_EVENT_NODE_DOWN                        -231
#define PMIX_EVENT_NODE_OFFLINE                     -232
#define PMIX_EVENT_SYS_OTHER                        -330
#define PMIX_EVENT_NO_ACTION_TAKEN                  -331
#define PMIX_EVENT_PARTIAL_ACTION_TAKEN             -332
#define PMIX_EVENT_ACTION_DEFERRED                  -333
#define PMIX_EVENT_ACTION_COMPLETE                  -334
#define PMIX_EXTERNAL_ERR_BASE                      -3000


typedef uint16_t pmix_data_type_t;
#define PMIX_UNDEF                       0
#define PMIX_BOOL                        1
#define PMIX_BYTE                        2
#define PMIX_STRING                      3
#define PMIX_SIZE                        4
#define PMIX_PID                         5
#define PMIX_INT                         6
#define PMIX_INT8                        7
#define PMIX_INT16                       8
#define PMIX_INT32                       9
#define PMIX_INT64                      10
#define PMIX_UINT                       11
#define PMIX_UINT8                      12
#define PMIX_UINT16                     13
#define PMIX_UINT32                     14
#define PMIX_UINT64                     15
#define PMIX_FLOAT                      16
#define PMIX_DOUBLE                     17
#define PMIX_TIMEVAL                    18
#define PMIX_TIME                       19
#define PMIX_STATUS                     20
#define PMIX_VALUE                      21
#define PMIX_PROC                       22
#define PMIX_APP                        23
#define PMIX_INFO                       24
#define PMIX_PDATA                      25
#define PMIX_BYTE_OBJECT                27
#define PMIX_KVAL                       28
#define PMIX_PERSIST                    30
#define PMIX_POINTER                    31
#define PMIX_SCOPE                      32
#define PMIX_DATA_RANGE                 33
#define PMIX_COMMAND                    34
#define PMIX_INFO_DIRECTIVES            35
#define PMIX_DATA_TYPE                  36
#define PMIX_PROC_STATE                 37
#define PMIX_PROC_INFO                  38
#define PMIX_DATA_ARRAY                 39
#define PMIX_PROC_RANK                  40
#define PMIX_QUERY                      41
#define PMIX_COMPRESSED_STRING          42
#define PMIX_ALLOC_DIRECTIVE            43
#define PMIX_IOF_CHANNEL                45
#define PMIX_ENVAR                      46
#define PMIX_COORD                      47
#define PMIX_REGATTR                    48
#define PMIX_REGEX                      49
#define PMIX_JOB_STATE                  50
#define PMIX_LINK_STATE                 51
#define PMIX_PROC_CPUSET                52
#define PMIX_GEOMETRY                   53
#define PMIX_DEVICE_DIST                54
#define PMIX_ENDPOINT                   55
#define PMIX_TOPO                       56
#define PMIX_DEVTYPE                    57
#define PMIX_LOCTYPE                    58
#define PMIX_COMPRESSED_BYTE_OBJECT     59
#define PMIX_PROC_NSPACE                60
#define PMIX_PROC_STATS                 61
#define PMIX_DISK_STATS                 62
#define PMIX_NET_STATS                  63
#define PMIX_NODE_STATS                 64
#define PMIX_DATA_BUFFER                65
#define PMIX_STOR_MEDIUM                66
#define PMIX_STOR_ACCESS                67
#define PMIX_STOR_PERSIST               68
#define PMIX_STOR_ACCESS_TYPE           69
#define PMIX_DATA_TYPE_MAX             500


typedef uint8_t pmix_scope_t;
#define PMIX_SCOPE_UNDEF    0
#define PMIX_LOCAL          1
#define PMIX_REMOTE         2
#define PMIX_GLOBAL         3
#define PMIX_INTERNAL       4


typedef uint8_t pmix_data_range_t;
#define PMIX_RANGE_UNDEF        0
#define PMIX_RANGE_RM           1
#define PMIX_RANGE_LOCAL        2
#define PMIX_RANGE_NAMESPACE    3
#define PMIX_RANGE_SESSION      4
#define PMIX_RANGE_GLOBAL       5
#define PMIX_RANGE_CUSTOM       6
#define PMIX_RANGE_PROC_LOCAL   7
#define PMIX_RANGE_INVALID   UINT8_MAX


typedef uint8_t pmix_persistence_t;
#define PMIX_PERSIST_INDEF          0
#define PMIX_PERSIST_FIRST_READ     1
#define PMIX_PERSIST_PROC           2
#define PMIX_PERSIST_APP            3
#define PMIX_PERSIST_SESSION        4
#define PMIX_PERSIST_INVALID   UINT8_MAX


typedef uint32_t pmix_info_directives_t;
#define PMIX_INFO_REQD              0x00000001
#define PMIX_INFO_ARRAY_END         0x00000002
#define PMIX_INFO_REQD_PROCESSED    0x00000004
#define PMIX_INFO_DIR_RESERVED      0xffff0000


typedef uint8_t pmix_alloc_directive_t;
#define PMIX_ALLOC_NEW          1
#define PMIX_ALLOC_EXTEND       2
#define PMIX_ALLOC_RELEASE      3
#define PMIX_ALLOC_REAQUIRE     4
#define PMIX_ALLOC_EXTERNAL     128


typedef uint16_t pmix_iof_channel_t;
#define PMIX_FWD_NO_CHANNELS        0x0000
#define PMIX_FWD_STDIN_CHANNEL      0x0001
#define PMIX_FWD_STDOUT_CHANNEL     0x0002
#define PMIX_FWD_STDERR_CHANNEL     0x0004
#define PMIX_FWD_STDDIAG_CHANNEL    0x0008
#define PMIX_FWD_ALL_CHANNELS       0x00ff


typedef enum {
    PMIX_GROUP_DECLINE,
    PMIX_GROUP_ACCEPT
} pmix_group_opt_t;


typedef enum {
    PMIX_GROUP_CONSTRUCT,
    PMIX_GROUP_DESTRUCT
} pmix_group_operation_t;


typedef uint64_t pmix_storage_medium_t;
#define PMIX_STORAGE_MEDIUM_UNKNOWN     0x0000000000000001
#define PMIX_STORAGE_MEDIUM_TAPE        0x0000000000000002
#define PMIX_STORAGE_MEDIUM_HDD         0x0000000000000004
#define PMIX_STORAGE_MEDIUM_SSD         0x0000000000000008
#define PMIX_STORAGE_MEDIUM_NVME        0x0000000000000010
#define PMIX_STORAGE_MEDIUM_PMEM        0x0000000000000020
#define PMIX_STORAGE_MEDIUM_RAM         0x0000000000000040


typedef uint64_t pmix_storage_accessibility_t;
#define PMIX_STORAGE_ACCESSIBILITY_NODE     0x0000000000000001
#define PMIX_STORAGE_ACCESSIBILITY_SESSION  0x0000000000000002
#define PMIX_STORAGE_ACCESSIBILITY_JOB      0x0000000000000004
#define PMIX_STORAGE_ACCESSIBILITY_RACK     0x0000000000000008
#define PMIX_STORAGE_ACCESSIBILITY_CLUSTER  0x0000000000000010
#define PMIX_STORAGE_ACCESSIBILITY_REMOTE   0x0000000000000020


typedef uint64_t pmix_storage_persistence_t;
#define PMIX_STORAGE_PERSISTENCE_TEMPORARY  0x0000000000000001
#define PMIX_STORAGE_PERSISTENCE_NODE       0x0000000000000002
#define PMIX_STORAGE_PERSISTENCE_SESSION    0x0000000000000004
#define PMIX_STORAGE_PERSISTENCE_JOB        0x0000000000000008
#define PMIX_STORAGE_PERSISTENCE_SCRATCH    0x0000000000000010
#define PMIX_STORAGE_PERSISTENCE_PROJECT    0x0000000000000020
#define PMIX_STORAGE_PERSISTENCE_ARCHIVE    0x0000000000000040


typedef uint16_t pmix_storage_access_type_t;
#define PMIX_STORAGE_ACCESS_RD      0x0001
#define PMIX_STORAGE_ACCESS_WR      0x0002
#define PMIX_STORAGE_ACCESS_RDWR    0x0003


typedef uint8_t pmix_coord_view_t;
#define PMIX_COORD_VIEW_UNDEF       0x00
#define PMIX_COORD_LOGICAL_VIEW     0x01
#define PMIX_COORD_PHYSICAL_VIEW    0x02


typedef struct pmix_coord {
    pmix_coord_view_t view;
    uint32_t *coord;
    size_t dims;
} pmix_coord_t;

#define PMIX_COORD_STATIC_INIT      \
{                                   \
    .view = PMIX_COORD_VIEW_UNDEF,  \
    .coord = NULL,                  \
    .dims = 0                       \
}


typedef uint8_t pmix_link_state_t;
#define PMIX_LINK_STATE_UNKNOWN     0
#define PMIX_LINK_DOWN              1
#define PMIX_LINK_UP                2


typedef struct{
    char *source;
    void *bitmap;
} pmix_cpuset_t;

#define PMIX_CPUSET_STATIC_INIT \
{                               \
    .source = NULL,             \
    .bitmap = NULL              \
}


typedef uint8_t pmix_bind_envelope_t;
#define PMIX_CPUBIND_PROCESS    0
#define PMIX_CPUBIND_THREAD     1


typedef struct {
    char *source;
    void *topology;
} pmix_topology_t;

#define PMIX_TOPOLOGY_STATIC_INIT   \
{                                   \
    .source = NULL,                 \
    .topology = NULL                \
}


typedef uint16_t pmix_locality_t;
#define PMIX_LOCALITY_UNKNOWN           0x0000
#define PMIX_LOCALITY_NONLOCAL          0x8000
#define PMIX_LOCALITY_SHARE_HWTHREAD    0x0001
#define PMIX_LOCALITY_SHARE_CORE        0x0002
#define PMIX_LOCALITY_SHARE_L1CACHE     0x0004
#define PMIX_LOCALITY_SHARE_L2CACHE     0x0008
#define PMIX_LOCALITY_SHARE_L3CACHE     0x0010
#define PMIX_LOCALITY_SHARE_PACKAGE     0x0020
#define PMIX_LOCALITY_SHARE_NUMA        0x0040
#define PMIX_LOCALITY_SHARE_NODE        0x4000


typedef struct pmix_geometry {
    size_t fabric;
    char *uuid;
    char *osname;
    pmix_coord_t *coordinates;
    size_t ncoords;
} pmix_geometry_t;

#define PMIX_GEOMETRY_STATIC_INIT   \
{                                   \
    .fabric = 0,                    \
    .uuid = NULL,                   \
    .osname = NULL,                 \
    .coordinates = NULL,            \
    .ncoords = 0                    \
}


typedef uint64_t pmix_device_type_t;
#define PMIX_DEVTYPE_UNKNOWN        0x00
#define PMIX_DEVTYPE_BLOCK          0x01
#define PMIX_DEVTYPE_GPU            0x02
#define PMIX_DEVTYPE_NETWORK        0x04
#define PMIX_DEVTYPE_OPENFABRICS    0x08
#define PMIX_DEVTYPE_DMA            0x10
#define PMIX_DEVTYPE_COPROC         0x20


typedef struct pmix_device_distance {
    char *uuid;
    char *osname;
    pmix_device_type_t type;
    uint16_t mindist;
    uint16_t maxdist;
} pmix_device_distance_t;

#define PMIX_DEVICE_DIST_STATIC_INIT    \
{                                       \
    .uuid = NULL,                       \
    .osname = NULL,                     \
    .type = PMIX_DEVTYPE_UNKNOWN,       \
    .mindist = 0,                       \
    .maxdist = 0                        \
}


typedef struct pmix_byte_object {
    char *bytes;
    size_t size;
} pmix_byte_object_t;

#define PMIX_BYTE_OBJECT_STATIC_INIT    \
{                                       \
    .bytes = NULL,                      \
    .size = 0                           \
}


typedef struct pmix_endpoint {
    char *uuid;
    char *osname;
    pmix_byte_object_t endpt;
} pmix_endpoint_t;

#define PMIX_ENDPOINT_STATIC_INIT           \
{                                           \
    .uuid = NULL,                           \
    .osname = NULL,                         \
    .endpt = PMIX_BYTE_OBJECT_STATIC_INIT   \
}


typedef struct {
    char *envar;
    char *value;
    char separator;
} pmix_envar_t;

#define PMIX_ENVAR_STATIC_INIT  \
{                               \
    .envar = NULL,              \
    .value = NULL,              \
    .separator = '\0'           \
}


typedef struct pmix_proc {
    pmix_nspace_t nspace;
    pmix_rank_t rank;
} pmix_proc_t;

#define PMIX_PROC_STATIC_INIT   \
{                               \
.   nspace = {0},              \
.   rank = PMIX_RANK_UNDEF     \
}


typedef struct pmix_proc_info {
    pmix_proc_t proc;
    char *hostname;
    char *executable_name;
    pid_t pid;
    int exit_code;
    pmix_proc_state_t state;
} pmix_proc_info_t;

#define PMIX_PROC_INFO_STATIC_INIT  \
{                                   \
    .proc = PMIX_PROC_STATIC_INIT,  \
    .hostname = NULL,               \
    .executable_name = NULL,        \
    .pid = 0,                       \
    .exit_code = 0,                 \
    .state = PMIX_PROC_STATE_UNDEF  \
}


typedef struct pmix_data_array {
    pmix_data_type_t type;
    size_t size;
    void *array;
} pmix_data_array_t;

#define PMIX_DATA_ARRAY_STATIC_INIT     \
{                                       \
    .type = PMIX_UNDEF,                 \
    .size = 0,                          \
    .array = NULL                       \
}


typedef struct pmix_data_buffer {
    char *base_ptr;
    char *pack_ptr;
    char *unpack_ptr;
    size_t bytes_allocated;
    size_t bytes_used;
} pmix_data_buffer_t;

#define PMIX_DATA_BUFFER_STATIC_INIT    \
{                                       \
    .base_ptr = NULL,                   \
    .pack_ptr = NULL,                   \
    .unpack_ptr = NULL,                 \
    .bytes_allocated = 0,               \
    .bytes_used = 0                     \
}


typedef struct pmix_value {
    pmix_data_type_t type;
    union {
        bool flag;
        uint8_t byte;
        char *string;
        size_t size;
        pid_t pid;
        int integer;
        int8_t int8;
        int16_t int16;
        int32_t int32;
        int64_t int64;
        unsigned int uint;
        uint8_t uint8;
        uint16_t uint16;
        uint32_t uint32;
        uint64_t uint64;
        float fval;
        double dval;
        struct timeval tv;
        time_t time;
        pmix_status_t status;
        pmix_rank_t rank;
        pmix_nspace_t *nspace;
        pmix_proc_t *proc;
        pmix_byte_object_t bo;
        pmix_persistence_t persist;
        pmix_scope_t scope;
        pmix_data_range_t range;
        pmix_proc_state_t state;
        pmix_proc_info_t *pinfo;
        pmix_data_array_t *darray;
        void *ptr;
        pmix_alloc_directive_t adir;
        pmix_envar_t envar;
        pmix_coord_t *coord;
        pmix_link_state_t linkstate;
        pmix_job_state_t jstate;
        pmix_topology_t *topo;
        pmix_cpuset_t *cpuset;
        pmix_locality_t locality;
        pmix_geometry_t *geometry;
        pmix_device_type_t devtype;
        pmix_device_distance_t *devdist;
        pmix_endpoint_t *endpoint;
        pmix_data_buffer_t *dbuf;
    } data;
} pmix_value_t;

#define PMIX_VALUE_STATIC_INIT  \
{                               \
    .type = PMIX_UNDEF,         \
    .data.ptr = NULL            \
}


typedef struct pmix_info {
    pmix_key_t key;
    pmix_info_directives_t flags;
    pmix_value_t value;
} pmix_info_t;

#define PMIX_INFO_STATIC_INIT       \
{                                   \
    .key = {0},                     \
    .flags = 0,                     \
    .value = PMIX_VALUE_STATIC_INIT \
}


typedef struct pmix_pdata {
    pmix_proc_t proc;
    pmix_key_t key;
    pmix_value_t value;
} pmix_pdata_t;

#define PMIX_LOOKUP_STATIC_INIT     \
{                                   \
    .proc = PMIX_PROC_STATIC_INIT,  \
    .key = {0},                     \
    .value = PMIX_VALUE_STATIC_INIT \
}


typedef struct pmix_app {
    char *cmd;
    char **argv;
    char **env;
    char *cwd;
    int maxprocs;
    pmix_info_t *info;
    size_t ninfo;
} pmix_app_t;

#define PMIX_APP_STATIC_INIT    \
{                               \
    .cmd = NULL,                \
    .argv = NULL,               \
    .env = NULL,                \
    .cwd = NULL,                \
    .maxprocs = 0,              \
    .info = NULL,               \
    .ninfo = 0                  \
}


typedef struct pmix_query {
    char **keys;
    pmix_info_t *qualifiers;
    size_t nqual;
} pmix_query_t;

#define PMIX_QUERY_STATIC_INIT  \
{                               \
    .keys = NULL,               \
    .qualifiers = NULL,         \
    .nqual = 0                  \
}


typedef struct pmix_regattr_t {
    char *name;
    pmix_key_t string;
    pmix_data_type_t type;
    char **description;
} pmix_regattr_t;

#define PMIX_REGATTR_STATIC_INIT    \
{                                   \
    .name = NULL,                   \
    .string = {0},                  \
    .type = PMIX_UNDEF,             \
    .description = NULL             \
}


typedef struct pmix_fabric_s {
    char *name;
    size_t index;
    pmix_info_t *info;
    size_t ninfo;
    void *module;
} pmix_fabric_t;

#define PMIX_FABRIC_STATIC_INIT \
{                               \
    .name = NULL,               \
    .index = 0,                 \
    .info = NULL,               \
    .ninfo = 0,                 \
    .module = NULL              \
}


typedef enum {
    PMIX_FABRIC_REQUEST_INFO,
    PMIX_FABRIC_UPDATE_INFO
} pmix_fabric_operation_t;


/****    CALLBACK FUNCTIONS FOR NON-BLOCKING OPERATIONS    ****/

typedef void (*pmix_release_cbfunc_t)(void *cbdata);

typedef void (*pmix_modex_cbfunc_t)(pmix_status_t status,
                                    const char *data, size_t ndata,
                                    void *cbdata,
                                    pmix_release_cbfunc_t release_fn,
                                    void *release_cbdata);

typedef void (*pmix_spawn_cbfunc_t)(pmix_status_t status,
                                    pmix_nspace_t nspace, void *cbdata);

typedef void (*pmix_op_cbfunc_t)(pmix_status_t status, void *cbdata);

typedef void (*pmix_lookup_cbfunc_t)(pmix_status_t status,
                                     pmix_pdata_t data[], size_t ndata,
                                     void *cbdata);

typedef void (*pmix_event_notification_cbfunc_fn_t)(pmix_status_t status,
                                                    pmix_info_t *results, size_t nresults,
                                                    pmix_op_cbfunc_t cbfunc, void *thiscbdata,
                                                    void *notification_cbdata);

typedef void (*pmix_notification_fn_t)(size_t evhdlr_registration_id,
                                       pmix_status_t status,
                                       const pmix_proc_t *source,
                                       pmix_info_t info[], size_t ninfo,
                                       pmix_info_t *results, size_t nresults,
                                       pmix_event_notification_cbfunc_fn_t cbfunc,
                                       void *cbdata);

typedef void (*pmix_hdlr_reg_cbfunc_t)(pmix_status_t status,
                                       size_t refid,
                                       void *cbdata);
typedef void (*pmix_evhdlr_reg_cbfunc_t)(pmix_status_t status,
                                         size_t refid,
                                         void *cbdata);

typedef void (*pmix_value_cbfunc_t)(pmix_status_t status,
                                    pmix_value_t *kv, void *cbdata);

typedef void (*pmix_info_cbfunc_t)(pmix_status_t status,
                                   pmix_info_t *info, size_t ninfo,
                                   void *cbdata,
                                   pmix_release_cbfunc_t release_fn,
                                   void *release_cbdata);

typedef void (*pmix_credential_cbfunc_t)(pmix_status_t status,
                                         pmix_byte_object_t *credential,
                                         pmix_info_t info[], size_t ninfo,
                                         void *cbdata);

typedef void (*pmix_validation_cbfunc_t)(pmix_status_t status,
                                         pmix_info_t info[], size_t ninfo,
                                         void *cbdata);

typedef void (*pmix_device_dist_cbfunc_t)(pmix_status_t status,
                                          pmix_device_distance_t *dist,
                                          size_t ndist,
                                          void *cbdata,
                                          pmix_release_cbfunc_t release_fn,
                                          void *release_cbdata);

typedef void (*pmix_iof_cbfunc_t)(size_t iofhdlr, pmix_iof_channel_t channel,
                                  pmix_proc_t *source, pmix_byte_object_t *payload,
                                  pmix_info_t info[], size_t ninfo);

typedef void (*pmix_connection_cbfunc_t)(int incoming_sd, void *cbdata);

typedef void (*pmix_tool_connection_cbfunc_t)(pmix_status_t status,
                                              pmix_proc_t *proc, void *cbdata);

typedef void (*pmix_dmodex_response_fn_t)(pmix_status_t status,
                                          char *data, size_t sz,
                                          void *cbdata);

typedef void (*pmix_setup_application_cbfunc_t)(pmix_status_t status,
                                                pmix_info_t info[], size_t ninfo,
                                                void *provided_cbdata,
                                                pmix_op_cbfunc_t cbfunc, void *cbdata);

/* PMIX_TYPES_H */
#endif
