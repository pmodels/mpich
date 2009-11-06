#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
mpdman does NOT run as a standalone console program;
    it is only exec'd (or imported) by mpd
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.160 $"
__credits__ = ""


import sys, os, signal, socket

from types    import ClassType
from re       import findall, sub
from cPickle  import loads
from time     import sleep
from urllib   import quote
from mpdlib   import mpd_set_my_id, mpd_print, mpd_read_nbytes,  \
                     mpd_sockpair, mpd_get_ranks_in_binary_tree, \
                     mpd_get_my_username, mpd_set_cli_app,       \
                     mpd_dbg_level, mpd_handle_signal,           \
                     MPDSock, MPDListenSock, MPDStreamHandler, MPDRing

try:
    import  syslog  
    syslog_module_available = 1
except:
    syslog_module_available = 0
try:
    import  subprocess
    subprocess_module_available = 1
except:
    subprocess_module_available = 0


global clientPid, clientExited, clientExitStatus, clientExitStatusSent

class MPDMan(object):
    def __init__(self):
        pass
    def run(self):
        global clientPid, clientExited, clientExitStatus, clientExitStatusSent
        clientExited = 0
        clientExitStatusSent = 0
        if hasattr(signal,'SIGCHLD'):
            signal.signal(signal.SIGCHLD,sigchld_handler)
        self.myHost    = os.environ['MPDMAN_MYHOST']
        self.myIfhn    = os.environ['MPDMAN_MYIFHN']
        self.myRank    = int(os.environ['MPDMAN_RANK'])
        self.posInRing = int(os.environ['MPDMAN_POS_IN_RING'])
        self.myId      = self.myHost + '_mpdman_' + str(self.myRank)
        self.spawned   = int(os.environ['MPDMAN_SPAWNED'])
        self.spawnInProgress = 0
        if self.spawned:
            self.myId = self.myId + '_s'
        # Note that in the spawned process case, this id for the mpdman
        # will not be unique (it needs something like the world number
        # or the pid of the mpdman process itself)
        mpd_set_my_id(myid=self.myId)
        self.clientPgm = os.environ['MPDMAN_CLI_PGM']
        mpd_set_cli_app(self.clientPgm)
        try:
            os.chdir(os.environ['MPDMAN_CWD'])
        except Exception, errmsg:
            errmsg =  '%s: invalid dir: %s' % (self.myId,os.environ['MPDMAN_CWD'])
            # print errmsg    ## may syslog it in some cases ?
        if os.environ['MPDMAN_HOW_LAUNCHED'] == 'FORK':
            self.listenRingPort = int(os.environ['MPDMAN_MY_LISTEN_PORT'])
            listenRingFD = int(os.environ['MPDMAN_MY_LISTEN_FD'])  # closed in loop below
            self.listenRingSock = socket.fromfd(listenRingFD,socket.AF_INET,socket.SOCK_STREAM)
            self.listenRingSock = MPDSock(sock=self.listenRingSock)
            mpdFD = int(os.environ['MPDMAN_TO_MPD_FD'])  # closed in loop below
            self.mpdSock = socket.fromfd(mpdFD,socket.AF_INET,socket.SOCK_STREAM)
            self.mpdSock = MPDSock(sock=self.mpdSock)
        elif os.environ['MPDMAN_HOW_LAUNCHED'] == 'SUBPROCESS':
            self.listenRingSock = MPDListenSock()
            self.listenRingPort = self.listenRingSock.getsockname()[1]
            mpdPort = int(os.environ['MPDMAN_MPD_PORT'])
            self.mpdSock = MPDSock()
            self.mpdSock.connect((self.myIfhn,mpdPort))
            self.mpdSock.send_dict_msg( {'man_listen_port' : self.listenRingPort} )
        else:
            mpd_print(1,'I cannot figure out how I was launched')
            sys.exit(-1)
        self.pos0Ifhn = os.environ['MPDMAN_POS0_IFHN']
        self.pos0Port = int(os.environ['MPDMAN_POS0_PORT'])
        # close unused fds before I grab any more
        # NOTE: this will also close syslog's fd inherited from mpd; re-opened below
        try:     max_fds = os.sysconf('SC_OPEN_MAX')
        except:  max_fds = 1024
        # FIXME This snippet causes problems on Fedora Core 12.  FC12's python
        # opens a file object to /etc/abrt/pyhook.conf.  Closing the fd out from
        # under the higher level object causes problems at exit time when the
        # higher level object is garbage collected.  See MPICH2 ticket #902 for
        # more information.
        #for fd in range(3,max_fds):
        #    if fd == self.mpdSock.fileno()  or  fd == self.listenRingSock.fileno():
        #        continue
        #    try:    os.close(fd)
        #    except: pass
        if syslog_module_available:
            syslog.openlog("mpdman",0,syslog.LOG_DAEMON)
            syslog.syslog(syslog.LOG_INFO,"mpdman starting new log; %s" % (self.myId) )
        self.umask = os.environ['MPDMAN_UMASK']
        if self.umask.startswith('0x'):
            self.umask = int(self.umask,16)
        elif self.umask.startswith('0'):
            self.umask = int(self.umask,8)
        else:
            self.umask = int(self.umask)
        self.oldumask = os.umask(self.umask)
        self.clientPgmArgs = loads(os.environ['MPDMAN_PGM_ARGS'])
        self.clientPgmEnv = loads(os.environ['MPDMAN_PGM_ENVVARS'])
        self.clientPgmLimits = loads(os.environ['MPDMAN_PGM_LIMITS'])
        self.jobid = os.environ['MPDMAN_JOBID']
        self.nprocs = int(os.environ['MPDMAN_NPROCS'])
        self.mpdPort = int(os.environ['MPDMAN_MPD_LISTEN_PORT'])
        self.mpdConfPasswd = os.environ['MPDMAN_MPD_CONF_SECRETWORD']
        os.environ['MPDMAN_MPD_CONF_SECRETWORD'] = ''  ## do NOT pass it on to clients
        self.kvs_template_from_env = os.environ['MPDMAN_KVS_TEMPLATE']
        self.conIfhn  = os.environ['MPDMAN_CONIFHN']
        self.conPort  = int(os.environ['MPDMAN_CONPORT'])
        self.lhsIfhn  = os.environ['MPDMAN_LHS_IFHN']
        self.lhsPort  = int(os.environ['MPDMAN_LHS_PORT'])
        self.stdinDest = os.environ['MPDMAN_STDIN_DEST']
        self.totalview = int(os.environ['MPDMAN_TOTALVIEW'])
        self.gdb = int(os.environ['MPDMAN_GDB'])
        self.gdba = os.environ['MPDMAN_GDBA']
        self.lineLabelFmt = os.environ['MPDMAN_LINE_LABELS_FMT']
        self.startStdoutLineLabel = 1
        self.startStderrLineLabel = 1
        self.singinitPID  = int(os.environ['MPDMAN_SINGINIT_PID'])
        self.singinitPORT = int(os.environ['MPDMAN_SINGINIT_PORT'])
        self.doingBNR = int(os.environ['MPDMAN_DOING_BNR'])
        self.listenNonRingSock = MPDListenSock('',0,name='nonring_listen_sock')
        self.listenNonRingPort = self.listenNonRingSock.getsockname()[1]
        self.streamHandler = MPDStreamHandler()
        self.streamHandler.set_handler(self.mpdSock,self.handle_mpd_input)
        self.streamHandler.set_handler(self.listenNonRingSock,
                                       self.handle_nonring_connection)

        # set up pmi stuff early in case I was spawned
        self.universeSize = -1
        self.appnum = -1
        self.pmiVersion = 1
        self.pmiSubversion = 1
        self.KVSs = {}
        if self.singinitPID:
            # self.kvsname_template = 'singinit_kvs_'
            self.kvsname_template = 'singinit_kvs_' + str(os.getpid())
        else:
            self.kvsname_template = 'kvs_' + self.kvs_template_from_env + '_'
        self.default_kvsname = self.kvsname_template + '0'
        self.default_kvsname = sub('\.','_',self.default_kvsname)  # magpie.cs to magpie_cs
        self.default_kvsname = sub('\-','_',self.default_kvsname)  # chg node-0 to node_0
        self.KVSs[self.default_kvsname] = {}
        cli_env = {}
        cli_env['MPICH_INTERFACE_HOSTNAME'] = os.environ['MPICH_INTERFACE_HOSTNAME']
        cli_env['MPICH_INTERFACE_HOSTNAME_R%d' % self.myRank] = os.environ['MPICH_INTERFACE_HOSTNAME']
        for k in self.clientPgmEnv.keys():
            if k.startswith('MPI_APPNUM'):
                self.appnum = self.clientPgmEnv[k]    # don't put in application env
            elif k.startswith('MPICH_INTERFACE_HOSTNAME'):
                continue    ## already put it in above
            else:
                cli_env[k] = self.clientPgmEnv[k]
        self.kvs_next_id = 1
        self.jobEndingEarly = 0
        self.pmiCollectiveJob = 0
        self.spawnedCnt = 0
        self.pmiSock = 0   # obtained later
        self.ring = MPDRing(listenSock=self.listenRingSock,
                            streamHandler=self.streamHandler,
                            myIfhn=self.myIfhn)
        if self.nprocs == 1:
            self.ring.create_single_mem_ring(ifhn=self.myIfhn,
                                             port=self.listenRingPort,
                                             lhsHandler=self.handle_lhs_input,
                                             rhsHandler=self.handle_rhs_input)
        else:
            if self.posInRing == 0:    # one 'end'
                self.ring.accept_rhs(rhsHandler=self.handle_rhs_input)
                self.ring.accept_lhs(lhsHandler=self.handle_lhs_input)
            elif self.posInRing == (self.nprocs-1):  # the other 'end'
                rv = self.ring.connect_lhs(lhsIfhn=self.lhsIfhn,
                                           lhsPort=self.lhsPort,
                                           lhsHandler=self.handle_lhs_input,
                                           numTries=8)
                if rv[0] <= 0:
                    mpd_print(1,"lhs connect failed")
                    sys.exit(-1)
                self.rhsIfhn = self.pos0Ifhn
                self.rhsPort = self.pos0Port
                rv = self.ring.connect_rhs(rhsIfhn=self.rhsIfhn,
                                           rhsPort=self.rhsPort,
                                           rhsHandler=self.handle_rhs_input,
                                           numTries=8)
                if rv[0] <=  0:  # connect did not succeed; may try again
                    mpd_print(1,"rhs connect failed")
                    sys.exit(-1)
            else:  # ring members 'in the middle'
                rv = self.ring.connect_lhs(lhsIfhn=self.lhsIfhn,
                                           lhsPort=self.lhsPort,
                                           lhsHandler=self.handle_lhs_input,
                                           numTries=8)
                if rv[0] <= 0:
                    mpd_print(1,"lhs connect failed")
                    sys.exit(-1)
                self.ring.accept_rhs(rhsHandler=self.handle_rhs_input)

        if self.myRank == 0:
            self.conSock = MPDSock(name='to_console')
            self.conSock.connect((self.conIfhn,self.conPort))
            self.streamHandler.set_handler(self.conSock,self.handle_console_input)
            if self.spawned:
                msgToSend = { 'cmd' : 'spawned_man0_is_up',
                              'spawned_id' : os.environ['MPDMAN_SPAWNED'] }
                self.conSock.send_dict_msg(msgToSend)
                msg = self.conSock.recv_dict_msg()
                # If there is a failure in the connection, this
                # receive will fail and if not handled, cause mpdman 
                # to fail.  For now, we just check on a empty or unexpected
                # message
                if not msg or msg['cmd'] != 'preput_info_for_child':
                    mpd_print(1,'invalid msg from parent :%s:' % msg)
                    sys.exit(-1)
                try:
                    for k in msg['kvs'].keys():
                        self.KVSs[self.default_kvsname][k] = msg['kvs'][k]
                except:
                    mpd_print(1,'failed to insert preput_info')
                    sys.exit(-1)
                msg = self.conSock.recv_dict_msg()
                if not msg  or  not msg.has_key('cmd')  or  msg['cmd'] != 'ringsize':
                    mpd_print(1,'spawned: bad msg from con; got: %s' % (msg) )
                    sys.exit(-1)
                self.universeSize = msg['ring_ncpus']
                # if the rshSock is closed, we'll get an AttributeError 
                # exception about 'int' has no attribute 'send_dict_msg'
                # FIXME: Does every use of a sock on which send_dict_msg
                # is used need an "if xxxx.rhsSock:" test first?
                # Is there an else for those cases?
                self.ring.rhsSock.send_dict_msg(msg)  # forward it on
            else:
                msgToSend = { 'cmd' : 'man_checking_in' }
                self.conSock.send_dict_msg(msgToSend)
                msg = self.conSock.recv_dict_msg()
                if not msg  or  not msg.has_key('cmd')  or  msg['cmd'] != 'ringsize':
                    mpd_print(1,'invalid msg from con; expected ringsize got: %s' % (msg) )
                    sys.exit(-1)
                if self.clientPgmEnv.has_key('MPI_UNIVERSE_SIZE'):
                    self.universeSize = int(self.clientPgmEnv['MPI_UNIVERSE_SIZE'])
                else:
                    self.universeSize = msg['ring_ncpus']
                self.ring.rhsSock.send_dict_msg(msg)
            ## NOTE: if you spawn a non-MPI job, it may not send this msg
            ## in which case the pgm will hang; the reason for this is that
            ## mpich2 does an Accept after the PMI_Spawn_multiple and a non-mpi
            ## pgm will never do the expected Connect.
            self.stdoutToConSock = MPDSock(name='stdout_to_console')
            self.stdoutToConSock.connect((self.conIfhn,self.conPort))
            if self.spawned:
                msgToSend = { 'cmd' : 'child_in_stdout_tree', 'from_rank' : self.myRank }
                self.stdoutToConSock.send_dict_msg(msgToSend)
            self.stderrToConSock = MPDSock(name='stderr_to_console')
            self.stderrToConSock.connect((self.conIfhn,self.conPort))
            if self.spawned:
                msgToSend = { 'cmd' : 'child_in_stderr_tree', 'from_rank' : self.myRank }
                self.stderrToConSock.send_dict_msg(msgToSend)
        else:
            self.conSock = 0
        if self.myRank == 0:
            self.parentStdoutSock = self.stdoutToConSock
            self.parentStderrSock = self.stderrToConSock
        else:
            self.parentStdoutSock = 0
            self.parentStderrSock = 0
        msg = self.ring.lhsSock.recv_dict_msg()    # recv msg containing ringsize
        if not msg  or  not msg.has_key('cmd')  or  msg['cmd'] != 'ringsize':
            mpd_print(1,'invalid msg from lhs; expecting ringsize got: %s' % (msg) )
            sys.exit(-1)
        if self.myRank != 0:
            self.ring.rhsSock.send_dict_msg(msg)
            if self.clientPgmEnv.has_key('MPI_UNIVERSE_SIZE'):
                self.universeSize = int(self.clientPgmEnv['MPI_UNIVERSE_SIZE'])
            else:
                self.universeSize = msg['ring_ncpus']
        if self.doingBNR:
            (self.pmiSock,self.cliBNRSock) = mpd_sockpair()
            self.streamHandler.set_handler(self.pmiSock,self.handle_pmi_input)
            cli_env['MAN_MSGS_FD'] = str(self.cliBNRSock.fileno())       ## BNR
        self.numDone = 0
        self.numWithIO = 2    # stdout and stderr so far
        self.numConndWithIO = 2
        # FIXME: This is the old singleton approach, which didn't allow 
        # for more than one process to be a singleton
        if self.singinitPORT:
            self.pmiListenSock = 0
            self.pmiSock = MPDSock(name='pmi')
            self.pmiSock.connect((self.myIfhn,self.singinitPORT))
            self.streamHandler.set_handler(self.pmiSock,self.handle_pmi_input)
            self.pmiSock.send_char_msg('cmd=singinit authtype=none\n')
            line = self.pmiSock.recv_char_msg()
            charMsg = 'cmd=singinit_info rc=0 versionok=yes stdio=yes kvsname=%s\n' % (self.default_kvsname)
            self.pmiSock.send_char_msg(charMsg)

            sock_write_cli_stdin = MPDSock(name='write_cli_stdin')
            sock_write_cli_stdin.connect((self.myIfhn,self.singinitPORT))
            self.fd_write_cli_stdin = sock_write_cli_stdin.fileno()

            sock_read_cli_stdout = MPDSock(name='read_cli_stdout')
            sock_read_cli_stdout.connect((self.myIfhn,self.singinitPORT))
            self.fd_read_cli_stdout = sock_read_cli_stdout.fileno()
            
            sock_read_cli_stderr = MPDSock(name='read_cli_stderr')
            sock_read_cli_stderr.connect((self.myIfhn,self.singinitPORT))
            self.fd_read_cli_stderr = sock_read_cli_stderr.fileno()
        else:
            self.cliListenSock = MPDListenSock('',0,name='cli_listen_sock')  ## BNR
            self.cliListenPort = self.cliListenSock.getsockname()[1]         ## BNR
            self.pmiListenSock = MPDListenSock('',0,name='pmi_listen_sock')
            self.pmiListenPort = self.pmiListenSock.getsockname()[1]
        self.subproc = 0    # default; use fork instead of subprocess
        if self.singinitPID:
            clientPid = self.singinitPID
        else:
            cli_env['PATH']      = os.environ['MPDMAN_CLI_PATH']
            cli_env['PMI_PORT']  = '%s:%s' % (self.myIfhn,self.pmiListenPort)
            cli_env['PMI_SIZE']  = str(self.nprocs)
            cli_env['PMI_RANK']  = str(self.myRank)
            cli_env['PMI_DEBUG'] = str(0)
            cli_env['PMI_TOTALVIEW'] = str(self.totalview)
            if self.spawned:
                cli_env['PMI_SPAWNED'] = '1'
            else:
                cli_env['PMI_SPAWNED'] = '0'
            if self.doingBNR:
                cli_env['MPD_TVDEBUG'] = str(0)                                   ## BNR
                cli_env['MPD_JID'] = os.environ['MPDMAN_JOBID']                   ## BNR
                cli_env['MPD_JSIZE'] = str(self.nprocs)                           ## BNR
                cli_env['MPD_JRANK'] = str(self.myRank)                           ## BNR
                cli_env['CLIENT_LISTENER_FD'] = str(self.cliListenSock.fileno())  ## BNR
            if hasattr(os,'fork'):
                (self.fd_read_cli_stdin, self.fd_write_cli_stdin ) = os.pipe()
                (self.fd_read_cli_stdout,self.fd_write_cli_stdout) = os.pipe()
                (self.fd_read_cli_stderr,self.fd_write_cli_stderr) = os.pipe()
                (self.handshake_sock_man_end,self.handshake_sock_cli_end) = mpd_sockpair()
                clientPid = self.launch_client_via_fork_exec(cli_env)
                if clientPid < 0:
                    print '**** mpdman: launch_client_via_fork_exec failed; exiting'
                    sys.exit(-1)
                elif clientPid > 0:
                    self.handshake_sock_cli_end.close()
                else:  # 0
                    self.handshake_sock_man_end.close()
            elif subprocess_module_available:
                clientPid = self.launch_client_via_subprocess(cli_env)  # may chg self.subproc
            else:
                mpd_print(1,'neither fork nor subprocess is available')
                sys.exit(-1)
        # if not initially a recvr of stdin (e.g. gdb) then give immediate eof to client
        if not in_stdinRcvrs(self.myRank,self.stdinDest):
            if self.subproc:    # must close subproc's file (not just the fd)
                self.subproc.stdin.close()
            else:
                os.close(self.fd_write_cli_stdin)
        if self.doingBNR:
            self.cliBNRSock.close()
        msgToSend = { 'cmd' : 'client_info', 'jobid' : self.jobid, 'clipid' : clientPid,
                      'manpid' : os.getpid(), 'rank' : self.myRank,
                      'spawner_manpid' : int(os.environ['MPDMAN_SPAWNER_MANPID']),
                      'spawner_mpd' : os.environ['MPDMAN_SPAWNER_MPD'] }
        self.mpdSock.send_dict_msg(msgToSend)

        if not self.subproc:
            self.streamHandler.set_handler(self.fd_read_cli_stdout,
                                           self.handle_cli_stdout_input)
            self.streamHandler.set_handler(self.fd_read_cli_stderr,
                                           self.handle_cli_stderr_input)
        self.waitPids = [clientPid]

        if self.pmiListenSock:
            self.streamHandler.set_handler(self.pmiListenSock,self.handle_pmi_connection)

        # begin setup of stdio tree
        (parent,lchild,rchild) = mpd_get_ranks_in_binary_tree(self.myRank,self.nprocs)
        self.spawnedChildSocks = []
        self.childrenStdoutTreeSocks = []
        self.childrenStderrTreeSocks = []
        if lchild >= 0:
            self.numWithIO += 2    # stdout and stderr from child
            msgToSend = { 'cmd' : 'info_from_parent_in_tree',
                          'to_rank' : str(lchild),
                          'parent_ifhn'   : self.myIfhn,
                          'parent_port' : self.listenNonRingPort }
            self.ring.rhsSock.send_dict_msg(msgToSend)
        if rchild >= 0:
            self.numWithIO += 2    # stdout and stderr from child
            msgToSend = { 'cmd' : 'info_from_parent_in_tree',
                          'to_rank' : str(rchild),
                          'parent_ifhn'   : self.myIfhn,
                          'parent_port' : self.listenNonRingPort }
            self.ring.rhsSock.send_dict_msg(msgToSend)

        if os.environ.has_key('MPDMAN_RSHIP'):
            rship = os.environ['MPDMAN_RSHIP']
            # (rshipSock,rshipPort) = mpd_get_inet_listen_sock('',0)
            rshipPid = os.fork()
            if rshipPid == 0:
                os.environ['MPDCP_MSHIP_HOST'] = os.environ['MPDMAN_MSHIP_HOST']
                os.environ['MPDCP_MSHIP_PORT'] = os.environ['MPDMAN_MSHIP_PORT']
                os.environ['MPDCP_MSHIP_NPROCS'] = str(self.nprocs)
                os.environ['MPDCP_CLI_PID'] = str(clientPid)
                try:
                    os.execvpe(rship,[rship],os.environ)
                except Exception, errmsg:
                    # make sure my error msgs get to console
                    os.dup2(self.parentStdoutSock.fileno(),1)  # closes fd 1 (stdout) if open
                    os.dup2(self.parentStderrSock.fileno(),2)  # closes fd 2 (stderr) if open
                    mpd_print(1,'execvpe failed for copgm %s; errmsg=:%s:' % (rship,errmsg) )
                    sys.exit(-1)
                sys.exit(0)
            # rshipSock.close()
            self.waitPids.append(rshipPid)

        if not self.spawned:
            # receive the final process mapping from our MPD overlords
            msg = self.mpdSock.recv_dict_msg(timeout=-1)

            # a few defensive checks now to make sure that the various parts of the
            # code are all on the same page
            if not msg.has_key('cmd') or msg['cmd'] != 'process_mapping':
                mpd_print(1,'expected cmd="process_mapping", got cmd="%s" instead' % (msg.get('cmd','**not_present**')))
                sys.exit(-1)
            if msg['jobid'] != self.jobid:
                mpd_print(1,'expected jobid="%s", got jobid="%s" instead' % (self.jobid,msg['jobid']))
                sys.exit(-1)
            if not msg.has_key('process_mapping'):
                mpd_print(1,'expected msg to contain a process_mapping key')
                sys.exit(-1)
            self.KVSs[self.default_kvsname]['PMI_process_mapping'] = msg['process_mapping']


        self.tvReady = 0
        self.pmiBarrierInRecvd = 0
        self.holdingPMIBarrierLoop1 = 0
        if self.myRank == 0:
            self.holdingEndBarrierLoop1 = 1
            self.holdingJobgoLoop1 = { 'cmd' : 'jobgo_loop_1', 'procinfo' : [] }
        else:
            self.holdingEndBarrierLoop1 = 0
            self.holdingJobgoLoop1 = 0
        self.jobStarted = 0
        self.endBarrierDone = 0
        # Main Loop
        while not self.endBarrierDone:
            if self.numDone >= self.numWithIO  and  (self.singinitPID or self.subproc):
                clientExited = 1
                clientExitStatus = 0
            if self.holdingJobgoLoop1 and self.numConndWithIO >= self.numWithIO:
                msgToSend = self.holdingJobgoLoop1
                self.ring.rhsSock.send_dict_msg(msgToSend)
                self.holdingJobgoLoop1 = 0
            rv = self.streamHandler.handle_active_streams(timeout=5.0)
            if rv[0] < 0:
                if type(rv[1]) == ClassType  and  rv[1] == KeyboardInterrupt: # ^C
                    sys.exit(-1)
            if clientExited:
                if self.jobStarted  and  not clientExitStatusSent:
                    msgToSend = { 'cmd' : 'client_exit_status', 'man_id' : self.myId,
                                  'cli_status' : clientExitStatus, 'cli_host' : self.myHost,
                                  'cli_ifhn' : self.myIfhn, 'cli_pid' : clientPid,
                                  'cli_rank' : self.myRank }
                    if self.myRank == 0:
                        if self.conSock:
                            try:
                                self.conSock.send_dict_msg(msgToSend)
                            except:
                                pass
                    else:
                        if self.ring.rhsSock:
                            self.ring.rhsSock.send_dict_msg(msgToSend)
                    clientExitStatusSent = 1
                if self.holdingEndBarrierLoop1 and self.numDone >= self.numWithIO:
                    self.holdingEndBarrierLoop1 = 0
                    msgToSend = {'cmd' : 'end_barrier_loop_1'}
                    self.ring.rhsSock.send_dict_msg(msgToSend)
        mpd_print(0000, "out of loop")
        # may want to wait for waitPids here
    def handle_nonring_connection(self,sock):
        (tempSock,tempConnAddr) = self.listenNonRingSock.accept()
        msg = tempSock.recv_dict_msg()
        if msg  and  msg.has_key('cmd'):
            if msg['cmd'] == 'child_in_stdout_tree':
                self.streamHandler.set_handler(tempSock,self.handle_child_stdout_tree_input)
                self.childrenStdoutTreeSocks.append(tempSock)
                self.numConndWithIO += 1
            elif msg['cmd'] == 'child_in_stderr_tree':
                self.streamHandler.set_handler(tempSock,self.handle_child_stderr_tree_input)
                self.childrenStderrTreeSocks.append(tempSock)
                self.numConndWithIO += 1
            elif msg['cmd'] == 'spawned_man0_is_up':
                self.streamHandler.set_handler(tempSock,self.handle_spawned_child_input)
                self.spawnedChildSocks.append(tempSock)
                tempID = msg['spawned_id']
                spawnedKVSname = 'mpdman_kvs_for_spawned_' + tempID
                msgToSend = { 'cmd' : 'preput_info_for_child',
                              'kvs' : self.KVSs[spawnedKVSname] }
                tempSock.send_dict_msg(msgToSend)
                msgToSend = { 'cmd' : 'ringsize', 'ring_ncpus' : self.universeSize }
                tempSock.send_dict_msg(msgToSend)
            else:
                mpd_print(1, 'unknown msg recvd on listenNonRingSock :%s:' % (msg) )
    def handle_lhs_input(self,sock):
        msg = self.ring.lhsSock.recv_dict_msg()
        if not msg:
            mpd_print(0000, 'lhs died' )
            self.streamHandler.del_handler(self.ring.lhsSock)
            self.ring.lhsSock.close()
        elif msg['cmd'] == 'jobgo_loop_1':
            if self.myRank == 0:
                if self.totalview:
                    msg['procinfo'].insert(0,(socket.gethostname(),self.clientPgm,clientPid))
                # let console pgm proceed
                msgToSend = { 'cmd' : 'job_started', 'jobid' : self.jobid,
                              'procinfo' : msg['procinfo'] }
                self.conSock.send_dict_msg(msgToSend,errprint=0)
                msgToSend = { 'cmd' : 'jobgo_loop_2' }
                self.ring.rhsSock.send_dict_msg(msgToSend)
            else:
                if self.totalview:
                    msg['procinfo'].append((socket.gethostname(),self.clientPgm,clientPid))
                if self.numConndWithIO >= self.numWithIO:
                    self.ring.rhsSock.send_dict_msg(msg)  # forward it on
                else:
                    self.holdingJobgoLoop1 = msg
        elif msg['cmd'] == 'jobgo_loop_2':
            if self.myRank != 0:
                self.ring.rhsSock.send_dict_msg(msg)  # forward it on
            if not self.singinitPID:
                self.handshake_sock_man_end.send_char_msg('go\n')
                self.handshake_sock_man_end.close()
            self.jobStarted = 1
        elif msg['cmd'] == 'info_from_parent_in_tree':
            if int(msg['to_rank']) == self.myRank:
                self.parentIfhn = msg['parent_ifhn']
                self.parentPort = msg['parent_port']
                self.parentStdoutSock = MPDSock(name='stdout_ro_parent')
                self.parentStdoutSock.connect((self.parentIfhn,self.parentPort))
                msgToSend = { 'cmd' : 'child_in_stdout_tree', 'from_rank' : self.myRank }
                self.parentStdoutSock.send_dict_msg(msgToSend)
                self.parentStderrSock = MPDSock(name='stderr_ro_parent')
                self.parentStderrSock.connect((self.parentIfhn,self.parentPort))
                msgToSend = { 'cmd' : 'child_in_stderr_tree', 'from_rank' : self.myRank }
                self.parentStderrSock.send_dict_msg(msgToSend)
            else:
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'end_barrier_loop_1':
            if self.myRank == 0:
                msgToSend = { 'cmd' : 'end_barrier_loop_2' }
                self.ring.rhsSock.send_dict_msg(msgToSend)
            else:
                if self.numDone >= self.numWithIO:
                    if self.ring.rhsSock:
                        self.ring.rhsSock.send_dict_msg(msg)
                else:
                    self.holdingEndBarrierLoop1 = 1
        elif msg['cmd'] == 'end_barrier_loop_2':
            self.endBarrierDone = 1
            if self.myRank != 0:
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'pmi_barrier_loop_1':
            if self.myRank == 0:
                msgToSend = { 'cmd' : 'pmi_barrier_loop_2' }
                self.ring.rhsSock.send_dict_msg(msgToSend)
                if self.doingBNR:    ## BNR
                    pmiMsgToSend = 'cmd=client_bnr_fence_out\n'
                    self.pmiSock.send_char_msg(pmiMsgToSend)
                    sleep(0.1)  # minor pause before intr
                    os.kill(clientPid,signal.SIGUSR1)
                else:
                    if self.pmiSock:
                        pmiMsgToSend = 'cmd=barrier_out\n'
                        self.pmiSock.send_char_msg(pmiMsgToSend)
            else:
                self.holdingPMIBarrierLoop1 = 1
                if self.pmiBarrierInRecvd:
                    self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'pmi_barrier_loop_2':
            self.pmiBarrierInRecvd = 0
            self.holdingPMIBarrierLoop1 = 0
            if self.myRank != 0:
                self.ring.rhsSock.send_dict_msg(msg)
                if self.doingBNR:    ## BNR
                    pmiMsgToSend = 'cmd=client_bnr_fence_out\n'
                    self.pmiSock.send_char_msg(pmiMsgToSend)
                    sleep(0.1)  # minor pause before intr
                    os.kill(clientPid,signal.SIGUSR1)
                else:
                    if self.pmiSock:
                        pmiMsgToSend = 'cmd=barrier_out\n'
                        self.pmiSock.send_char_msg(pmiMsgToSend)
        elif msg['cmd'] == 'pmi_get':
            if msg['from_rank'] == self.myRank:
                if self.pmiSock:  # may have disappeared in early shutdown
                    pmiMsgToSend = 'cmd=get_result rc=-1 key="%s"\n' % msg['key']
                    self.pmiSock.send_char_msg(pmiMsgToSend)
            else:
                key = msg['key']
                kvsname = msg['kvsname']
                if self.KVSs.has_key(kvsname)  and  self.KVSs[kvsname].has_key(key):
                    value = self.KVSs[kvsname][key]
                    msgToSend = { 'cmd' : 'response_to_pmi_get', 'key' : key,
                                  'kvsname' : kvsname, 'value' : value,
                                  'to_rank' : msg['from_rank'] }
                    self.ring.rhsSock.send_dict_msg(msgToSend)
                else:
                    self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'pmi_getbyidx':
            if msg['from_rank'] == self.myRank:
                if self.pmiSock:  # may have disappeared in early shutdown
                    self.KVSs[self.default_kvsname].update(msg['kvs'])
                    if self.KVSs[self.default_kvsname].keys():
                        key = self.KVSs[self.default_kvsname].keys()[0]
                        val = self.KVSs[self.default_kvsname][key]
                        pmiMsgToSend = 'cmd=getbyidx_results rc=0 nextidx=1 key=%s val=%s\n' % \
                                       (key,val)
                    else:
                        pmiMsgToSend = 'cmd=getbyidx_results rc=-2 reason=no_more_keyvals\n'
                    self.pmiSock.send_char_msg(pmiMsgToSend)
            else:
                msg['kvs'].update(self.KVSs[self.default_kvsname])
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'response_to_pmi_get':
            # [goodell@ 2009-05-05] The next few lines add caching in to the kvs
            # gets to improve lookup performance and reduce MPI_Init times.
            # Note that this doesn't handle consistency correctly if PMI is ever
            # changed to permit overwriting keyvals.
            if msg['kvsname'] not in self.KVSs.keys():
                self.KVSs[msg['kvsname']] = dict()
            self.KVSs[msg['kvsname']][msg['key']] = msg['value']

            if msg['to_rank'] == self.myRank:
                if self.pmiSock:  # may have disappeared in early shutdown
                    pmiMsgToSend = 'cmd=get_result rc=0 value=%s\n' % (msg['value'])
                    self.pmiSock.send_char_msg(pmiMsgToSend)
            else:
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'signal':
            if msg['signo'] == 'SIGINT':
                if not self.gdb:
                    self.jobEndingEarly = 1
                for s in self.spawnedChildSocks:
                    s.send_dict_msg(msg)
                if self.myRank != 0:
                    if self.ring.rhsSock:  # still alive ?
                        self.ring.rhsSock.send_dict_msg(msg)
                    if self.gdb:
                        os.kill(clientPid,signal.SIGINT)
                    else:
                        try:
                            pgrp = clientPid * (-1)   # neg Pid -> group
                            os.kill(pgrp,signal.SIGKILL)   # may be reaped by sighandler
                        except:
                            pass
            elif msg['signo'] == 'SIGKILL':
                self.jobEndingEarly = 1
                for s in self.spawnedChildSocks:
                    s.send_dict_msg(msg)
                if self.myRank != 0:
                    if self.ring.rhsSock:  # still alive ?
                        self.ring.rhsSock.send_dict_msg(msg)
                    if self.gdb:
                        os.kill(clientPid,signal.SIGUSR1)   # tell gdb driver to kill all
                    else:
                        try:
                            pgrp = clientPid * (-1)   # neg Pid -> group
                            os.kill(pgrp,signal.SIGKILL)   # may be reaped by sighandler
                        except:
                            pass
            elif msg['signo'] == 'SIGTSTP':
                if msg['dest'] != self.myId:
                    self.ring.rhsSock.send_dict_msg(msg)
                    try:
                        pgrp = clientPid * (-1)   # neg Pid -> group
                        os.kill(pgrp,signal.SIGTSTP)   # may be reaped by sighandler
                    except:
                        pass
            elif msg['signo'] == 'SIGCONT':
                if msg['dest'] != self.myId:
                    self.ring.rhsSock.send_dict_msg(msg)
                    try:
                        pgrp = clientPid * (-1)   # neg Pid -> group
                        os.kill(pgrp,signal.SIGCONT)   # may be reaped by sighandler
                    except:
                        pass
        elif msg['cmd'] == 'client_exit_status':
            if self.myRank == 0:
                if self.conSock:
                    self.conSock.send_dict_msg(msg,errprint=0)
            else:
                if self.ring.rhsSock:
                    self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'collective_abort':
            self.jobEndingEarly = 1
            if msg['src'] != self.myId:
                if self.ring.rhsSock:  # still alive ?
                    self.ring.rhsSock.send_dict_msg(msg)
            if self.conSock:
                msgToSend = { 'cmd' : 'job_aborted_early', 'jobid' : self.jobid,
                              'rank' : msg['rank'], 
                              'exit_status' : msg['exit_status'] }
                self.conSock.send_dict_msg(msgToSend,errprint=0)
            try:
                pgrp = clientPid * (-1)   # neg Pid -> group
                os.kill(pgrp,signal.SIGKILL)   # may be reaped by sighandler
            except:
                pass
        elif msg['cmd'] == 'startup_status':
            if msg['rc'] < 0:
                self.jobEndingEarly = 1
                try:
                    pgrp = clientPid * (-1)   # neg Pid -> group
                    os.kill(pgrp,signal.SIGKILL)   # may be reaped by sighandler
                except:
                    pass
            ##### RMB if msg['src'] == self.myId:
            if self.myRank == 0:
                if self.conSock:
                    self.conSock.send_dict_msg(msg,errprint=0)
            else:
                if msg['src'] != self.myId  and  self.ring.rhsSock:  # rhs still alive ?
                    self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'stdin_from_user':
            if msg['src'] != self.myId:
                self.ring.rhsSock.send_dict_msg(msg)
                if in_stdinRcvrs(self.myRank,self.stdinDest):
                    if msg.has_key('eof'):
                        if self.subproc:    # must close subproc's file (not just the fd)
                            self.subproc.stdin.close()
                        else:
                            os.close(self.fd_write_cli_stdin)
                    else:
                        os.write(self.fd_write_cli_stdin,msg['line'])
        elif msg['cmd'] == 'stdin_dest':
            if msg['src'] != self.myId:
                self.stdinDest = msg['stdin_procs']
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'interrupt_peer_with_msg':    ## BNR
            if int(msg['torank']) == self.myRank:
                if self.pmiSock:  # may have disappeared in early shutdown
                    pmiMsgToSend = '%s\n' % (msg['msg'])
                    self.pmiSock.send_char_msg(pmiMsgToSend)
                    sleep(0.1)  # minor pause before intr
                    os.kill(clientPid,signal.SIGUSR1)
            else:
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'tv_ready':
            self.tvReady = 1
            if self.myRank != 0:
                msg['src'] = self.myId
                self.ring.rhsSock.send_dict_msg(msg)
                if self.pmiSock:    # should be valid sock if running tv
                    pmiMsgToSend = 'cmd=tv_ready\n'
                    self.pmiSock.send_char_msg(pmiMsgToSend)
        else:
            mpd_print(1, 'unexpected msg recvd on lhsSock :%s:' % msg )

    def handle_rhs_input(self,sock):
        msg = self.ring.rhsSock.recv_dict_msg()  #### NOT USING msg; should I ?
        mpd_print(0000, 'rhs died' )
        self.streamHandler.del_handler(self.ring.rhsSock)
        self.ring.rhsSock.close()
        self.ring.rhsSock = 0
    def handle_cli_stdout_input(self,sock):
        line = mpd_read_nbytes(sock,1024)  # sock is self.fd_read_cli_stdout
        if not line:
            if self.subproc:    # must close subproc's file (not just the fd)
                self.subproc.stdout.close()
            else:
                self.streamHandler.del_handler(self.fd_read_cli_stdout)
                os.close(self.fd_read_cli_stdout)
            self.numDone += 1
            if self.numDone >= self.numWithIO:
                if self.parentStdoutSock:
                    self.parentStdoutSock.close()
                    self.parentStdoutSock = 0
                if self.parentStderrSock:
                    self.parentStderrSock.close()
                    self.parentStderrSock = 0
        else:
            if self.parentStdoutSock:
                if self.lineLabelFmt:
                    lineLabel = self.create_line_label(self.lineLabelFmt,self.spawned)
                    splitLine = line.split('\n',1024)
                    if self.startStdoutLineLabel:
                        line = lineLabel
                    else:
                        line = ''
                    if splitLine[-1] == '':
                        self.startStdoutLineLabel = 1
                        del splitLine[-1]
                    else:
                        self.startStdoutLineLabel = 0
                    for s in splitLine[0:-1]:
                        line = line + s + '\n' + lineLabel
                    line = line + splitLine[-1]
                    if self.startStdoutLineLabel:
                        line = line + '\n'
                self.parentStdoutSock.send_char_msg(line,errprint=0)
        return line
    def handle_cli_stderr_input(self,sock):
        line = mpd_read_nbytes(sock,1024)  # sock is self.fd_read_cli_stderr
        if not line:
            if self.subproc:    # must close subproc's file (not just the fd)
                self.subproc.stderr.close()
            else:
                self.streamHandler.del_handler(self.fd_read_cli_stderr)
                os.close(self.fd_read_cli_stderr)
            self.numDone += 1
            if self.numDone >= self.numWithIO:
                if self.parentStdoutSock:
                    self.parentStdoutSock.close()
                    self.parentStdoutSock = 0
                if self.parentStderrSock:
                    self.parentStderrSock.close()
                    self.parentStderrSock = 0
        else:
            if self.parentStderrSock:
                if self.lineLabelFmt:
                    lineLabel = self.create_line_label(self.lineLabelFmt,self.spawned)
                    splitLine = line.split('\n',1024)
                    if self.startStderrLineLabel:
                        line = lineLabel
                    else:
                        line = ''
                    if splitLine[-1] == '':
                        self.startStderrLineLabel = 1
                        del splitLine[-1]
                    else:
                        self.startStderrLineLabel = 0
                    for s in splitLine[0:-1]:
                        line = line + s + '\n' + lineLabel
                    line = line + splitLine[-1]
                    if self.startStderrLineLabel:
                        line = line + '\n'
                self.parentStderrSock.send_char_msg(line,errprint=0)
        return line
    def handle_child_stdout_tree_input(self,sock):
        if self.lineLabelFmt:
            line = sock.recv_one_line()
        else:
            line = sock.recv(1024)
        if not line:
            self.streamHandler.del_handler(sock)
            sock.close()
            self.numDone += 1
            if self.numDone >= self.numWithIO:
                if self.parentStdoutSock:
                    self.parentStdoutSock.close()
                    self.parentStdoutSock = 0
                if self.parentStderrSock:
                    self.parentStderrSock.close()
                    self.parentStderrSock = 0
        else:
            if self.parentStdoutSock:
                self.parentStdoutSock.send_char_msg(line,errprint=0)
                # parentStdoutSock.sendall('FWD by %d: |%s|' % (self.myRank,line) )
    def handle_child_stderr_tree_input(self,sock):
        if self.lineLabelFmt:
            line = sock.recv_one_line()
        else:
            line = sock.recv(1024)
        if not line:
            self.streamHandler.del_handler(sock)
            sock.close()
            self.numDone += 1
            if self.numDone >= self.numWithIO:
                if self.parentStdoutSock:
                    self.parentStdoutSock.close()
                    self.parentStdoutSock = 0
                if self.parentStderrSock:
                    self.parentStderrSock.close()
                    self.parentStderrSock = 0
        else:
            if self.parentStderrSock:
                self.parentStderrSock.send_char_msg(line,errprint=0)
                # parentStdoutSock.sendall('FWD by %d: |%s|' % (self.myRank,line) )
    def handle_spawned_child_input(self,sock):
        msg = sock.recv_dict_msg()
        if not msg:
            self.streamHandler.del_handler(sock)
            self.spawnedChildSocks.remove(sock)
            sock.close()
        elif msg['cmd'] == 'job_started':
            pass
        elif msg['cmd'] == 'client_exit_status':
            if self.myRank == 0:
                if self.conSock:
                    self.conSock.send_dict_msg(msg,errprint=0)
            else:
                if self.ring.rhsSock:
                    self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'job_aborted_early':
            if self.conSock:
                msgToSend = { 'cmd' : 'job_aborted_early', 'jobid' : msg['jobid'],
                              'rank' : msg['rank'], 
                              'exit_status' : msg['exit_status'] }
                self.conSock.send_dict_msg(msgToSend,errprint=0)
        elif msg['cmd'] == 'startup_status':
            # remember this rc to put in spawn_result
            self.spawnInProgress['errcodes'][msg['rank']] = msg['rc']
            if None not in self.spawnInProgress['errcodes']:  # if all errcodes are now filled in
                # send pmi msg to spawner
                strerrcodes = ''  # put errcodes in str format for pmi msg
                for ec in self.spawnInProgress['errcodes']:
                    strerrcodes = strerrcodes + str(ec) + ','
                strerrcodes = strerrcodes[:-1]
                if self.pmiSock:  # may have disappeared in early shutdown
                    # may want to make rc < 0 if any errcode is < 0
                    pmiMsgToSend = 'cmd=spawn_result rc=0 errcodes=%s\n' % (strerrcodes)
                    self.pmiSock.send_char_msg(pmiMsgToSend)
                self.spawnInProgress = 0
        else:
            mpd_print(1, "unrecognized msg from spawned child :%s:" % msg )
    def handle_pmi_connection(self,sock):
        if self.pmiSock:  # already have one
            pmiMsgToSend = 'cmd=you_already_have_an_open_pmi_conn_to_me\n'
            self.pmiSock.send_char_msg(pmiMsgToSend)
            self.streamHandler.del_handler(self.pmiSock)
            self.pmiSock.close()
            self.pmiSock = 0
            errmsg = "mpdman: invalid attempt to open 2 simultaneous pmi connections\n" + \
                     "  client=%s  cwd=%s" % (self.clientPgm,os.environ['MPDMAN_CWD'])
            print errmsg ; sys.stdout.flush()
            clientExitStatus = 137  # assume kill -9 below
            msgToSend = { 'cmd' : 'collective_abort',
                          'src' : self.myId, 'rank' : self.myRank,
                          'exit_status' : clientExitStatus }
            self.ring.rhsSock.send_dict_msg(msgToSend)
            return
        (self.pmiSock,tempConnAddr) = self.pmiListenSock.accept()
        # the following lines are commented out so that we can support a process
        # that runs 2 MPI pgms in tandem  (e.g. mpish at ANL)
        ##### del socksToSelect[pmiListenSock]
        ##### pmiListenSock.close()
        if not self.pmiSock:
            mpd_print(1,"failed accept for pmi connection from client")
            sys.exit(-1)
        self.pmiSock.name = 'pmi'
        self.streamHandler.set_handler(self.pmiSock,self.handle_pmi_input)
        if self.tvReady:
            pmiMsgToSend = 'cmd=tv_ready\n'
            self.pmiSock.send_char_msg(pmiMsgToSend)
    def handle_pmi_input(self,sock):
        global clientPid, clientExited, clientExitStatus, clientExitStatusSent
        if self.spawnInProgress:
            return
        line = self.pmiSock.recv_char_msg()
        if not line:
            self.streamHandler.del_handler(self.pmiSock)
            self.pmiSock.close()
            self.pmiSock = 0
            if self.pmiCollectiveJob:
                if self.ring.rhsSock:  # still alive ?
                    if not self.jobEndingEarly:  # if I did not already know this
                        if not clientExited:
                            clientExitStatus = 137  # assume kill -9 below
                        msgToSend = { 'cmd' : 'collective_abort',
                                      'src' : self.myId, 'rank' : self.myRank,
                                      'exit_status' : clientExitStatus }
                        self.ring.rhsSock.send_dict_msg(msgToSend)
                try:
                    pgrp = clientPid * (-1)   # neg Pid -> group
                    os.kill(pgrp,signal.SIGKILL)   # may be reaped by sighandler
                except:
                    pass
            return
        if line.startswith('mcmd='):
            parsedMsg = {}
            line = line.rstrip()
            splitLine = line.split('=',1)
            parsedMsg['cmd'] = splitLine[1]
            line = ''
            while not line.startswith('endcmd'):
                line = self.pmiSock.recv_char_msg()
                if not line.startswith('endcmd'):
                    line = line.rstrip()
                    splitLine = line.split('=',1)
                    parsedMsg[splitLine[0]] = splitLine[1]
        else:
            parsedMsg = parse_pmi_msg(line)
        if not parsedMsg.has_key('cmd'):
            pmiMsgToSend = 'cmd=unparseable_msg rc=-1\n'
            self.pmiSock.send_char_msg(pmiMsgToSend)
            return
        # startup_status may be sent here from new process BEFORE starting client
        if parsedMsg['cmd'] == 'startup_status':
            msgToSend = { 'cmd' : 'startup_status', 'src' : self.myId, 
                          'rc' : parsedMsg['rc'],
                          'jobid' : self.jobid, 'rank' : self.myRank,
                          'exec' : parsedMsg['exec'], 'reason' : parsedMsg['reason']  }
            if self.ring.rhsSock:
                self.ring.rhsSock.send_dict_msg(msgToSend)
        elif parsedMsg['cmd'] == 'init':
            self.pmiCollectiveJob = 1
            version = int(parsedMsg['pmi_version'])
            subversion = int(parsedMsg['pmi_subversion'])
            if self.pmiVersion == version  and  self.pmiSubversion >= subversion:
                rc = 0
            else:
                rc = -1
            pmiMsgToSend = 'cmd=response_to_init pmi_version=%d pmi_subversion=%d rc=%d\n' % \
                           (self.pmiVersion,self.pmiSubversion,rc)
            self.pmiSock.send_char_msg(pmiMsgToSend)
            msgToSend = { 'cmd' : 'startup_status', 'src' : self.myId, 'rc' : 0,
                          'jobid' : self.jobid, 'rank' : self.myRank,
                          'exec' : '', 'reason' : ''  }
            self.ring.rhsSock.send_dict_msg(msgToSend)
        elif parsedMsg['cmd'] == 'get_my_kvsname':
            pmiMsgToSend = 'cmd=my_kvsname kvsname=%s\n' % (self.default_kvsname)
            self.pmiSock.send_char_msg(pmiMsgToSend)
        elif parsedMsg['cmd'] == 'get_maxes':
            pmiMsgToSend = 'cmd=maxes kvsname_max=4096 ' + \
                           'keylen_max=4096 vallen_max=4096\n'
            self.pmiSock.send_char_msg(pmiMsgToSend)
        elif parsedMsg['cmd'] == 'get_universe_size':
            pmiMsgToSend = 'cmd=universe_size size=%s\n' % (self.universeSize)
            self.pmiSock.send_char_msg(pmiMsgToSend)
        elif parsedMsg['cmd'] == 'get_appnum':
            pmiMsgToSend = 'cmd=appnum appnum=%s\n' % (self.appnum)
            self.pmiSock.send_char_msg(pmiMsgToSend)
        elif parsedMsg['cmd'] == 'publish_name':
            msgToSend = { 'cmd' : 'publish_name',
                          'service' : parsedMsg['service'],
                          'port' : parsedMsg['port'],
                          'jobid' : self.jobid,
                          'manpid' : os.getpid() }
            self.mpdSock.send_dict_msg(msgToSend)
        elif parsedMsg['cmd'] == 'unpublish_name':
            msgToSend = { 'cmd' : 'unpublish_name',
                          'service' : parsedMsg['service'],
                          'jobid' : self.jobid,
                          'manpid' : os.getpid() }
            self.mpdSock.send_dict_msg(msgToSend)
        elif parsedMsg['cmd'] == 'lookup_name':
            msgToSend = { 'cmd' : 'lookup_name',
                          'service' : parsedMsg['service'],
                          'jobid' : self.jobid,
                          'manpid' : os.getpid() }
            self.mpdSock.send_dict_msg(msgToSend)
        elif parsedMsg['cmd'] == 'create_kvs':
            new_kvsname = self.kvsname_template + str(self.kvs_next_id)
            self.KVSs[new_kvsname] = {}
            self.kvs_next_id += 1
            pmiMsgToSend = 'cmd=newkvs kvsname=%s\n' % (new_kvsname)
            self.pmiSock.send_char_msg(pmiMsgToSend)
        elif parsedMsg['cmd'] == 'destroy_kvs':
            kvsname = parsedMsg['kvsname']
            try:
                del self.KVSs[kvsname]
                pmiMsgToSend = 'cmd=kvs_destroyed rc=0\n'
            except:
                pmiMsgToSend = 'cmd=kvs_destroyed rc=-1\n'
            self.pmiSock.send_char_msg(pmiMsgToSend)
        elif parsedMsg['cmd'] == 'put':
            kvsname = parsedMsg['kvsname']
            key = parsedMsg['key']
            value = parsedMsg['value']
            try:
                self.KVSs[kvsname][key] = value
                pmiMsgToSend = 'cmd=put_result rc=0\n'
                self.pmiSock.send_char_msg(pmiMsgToSend)
            except Exception, errmsg:
                pmiMsgToSend = 'cmd=put_result rc=-1 msg="%s"\n' % errmsg
                self.pmiSock.send_char_msg(pmiMsgToSend)
        elif parsedMsg['cmd'] == 'barrier_in':
            self.pmiBarrierInRecvd = 1
            if self.myRank == 0  or  self.holdingPMIBarrierLoop1:
                msgToSend = { 'cmd' : 'pmi_barrier_loop_1' }
                self.ring.rhsSock.send_dict_msg(msgToSend)
        elif parsedMsg['cmd'] == 'get':
            key = parsedMsg['key']
            kvsname = parsedMsg['kvsname']
            if self.KVSs.has_key(kvsname)  and  self.KVSs[kvsname].has_key(key):
                value = self.KVSs[kvsname][key]
                pmiMsgToSend = 'cmd=get_result rc=0 value=%s\n' % (value)
                self.pmiSock.send_char_msg(pmiMsgToSend)
            else:
                msgToSend = { 'cmd' : 'pmi_get', 'key' : key,
                              'kvsname' : kvsname, 'from_rank' : self.myRank }
                self.ring.rhsSock.send_dict_msg(msgToSend)
        elif parsedMsg['cmd'] == 'getbyidx':
            kvsname = parsedMsg['kvsname']
            idx = int(parsedMsg['idx'])
            if idx == 0:
                msgToSend = { 'cmd' : 'pmi_getbyidx', 'kvsname' : kvsname,
                              'from_rank' : self.myRank,
                              'kvs' : self.KVSs[self.default_kvsname] }
                self.ring.rhsSock.send_dict_msg(msgToSend)
            else:
                if len(self.KVSs[self.default_kvsname].keys()) > idx:
                    key = self.KVSs[self.default_kvsname].keys()[idx]
                    val = self.KVSs[self.default_kvsname][key]
                    nextidx = idx + 1
                    pmiMsgToSend = 'cmd=getbyidx_results rc=0 nextidx=%d key=%s val=%s\n' % \
                                   (nextidx,key,val)
                else:
                    pmiMsgToSend = 'cmd=getbyidx_results rc=-2 reason=no_more_keyvals\n'
                self.pmiSock.send_char_msg(pmiMsgToSend)
        elif parsedMsg['cmd'] == 'spawn':
            ## This code really is handling PMI_Spawn_multiple.  It translates a
            ## sequence of separate spawn messages into a single message to send
            ## to the mpd.  It keeps track by the "totspawns" and "spawnssofar"
            ## parameters in the incoming message.  The first message has
            ## "spawnssofar" set to 1. 
            ##
            ## This proc may produce stdout and stderr; do this early so I
            ## won't exit before child sets up its conns with me.
            ## NOTE: if you spawn a non-MPI job, it may not send these msgs
            ## in which case adding 2 to numWithIO will cause the pgm to hang.
            totspawns = int(parsedMsg['totspawns'])
            spawnssofar = int(parsedMsg['spawnssofar'])
            if spawnssofar == 1: # this is the first of possibly several spawn msgs
                self.numWithIO += 2
                self.tpsf = 0             # total processes spawned so far
                self.spawnExecs = {}      # part of MPI_Spawn_multiple args
                self.spawnHosts = {}      # comes from info
                self.spawnUsers = {}      # always the current user
                self.spawnCwds  = {}      # could come from info, but doesn't yet
                self.spawnUmasks = {}     # could come from info, but doesn't yet
                self.spawnPaths = {}      # could come from info, but doesn't yet
                self.spawnEnvvars = {}    # whole environment from mpiexec, plus appnum
                self.spawnLimits = {}
                self.spawnArgs = {}
            self.spawnNprocs  = int(parsedMsg['nprocs']) # num procs in this spawn
            pmiInfo = {}
            for i in range(0,int(parsedMsg['info_num'])):
                info_key = parsedMsg['info_key_%d' % i]
                info_val = parsedMsg['info_val_%d' % i]
                pmiInfo[info_key] = info_val

            if pmiInfo.has_key('host'):
                try:
                    toIfhn = socket.gethostbyname_ex(pmiInfo['host'])[2][0]
                    self.spawnHosts[(self.tpsf,self.tpsf+self.spawnNprocs-1)] = toIfhn
                except:
                    mpd_print(1, "unable to obtain host info for :%s:" % (pmiInfo['host']))
                    pmiMsgToSend = 'cmd=spawn_result rc=-2 status=unknown_host\n'
                    self.pmiSock.send_char_msg(pmiMsgToSend)
                    return
            else:
                self.spawnHosts[(self.tpsf,self.tpsf+self.spawnNprocs-1)] = '_any_'
            if pmiInfo.has_key('path'):
                self.spawnPaths[(self.tpsf,self.tpsf+self.spawnNprocs-1)] = pmiInfo['path']
            else:
                self.spawnPaths[(self.tpsf,self.tpsf+self.spawnNprocs-1)] = os.environ['MPDMAN_CLI_PATH']
            if pmiInfo.has_key('wdir'):
                self.spawnCwds[(self.tpsf,self.tpsf+self.spawnNprocs-1)] = pmiInfo['wdir']
            else:
                self.spawnCwds[(self.tpsf,self.tpsf+self.spawnNprocs-1)] = os.environ['MPDMAN_CWD']
            if pmiInfo.has_key('umask'):
                self.spawnUmasks[(self.tpsf,self.tpsf+self.spawnNprocs-1)] = pmiInfo['umask'] 
            else:
                self.spawnUmasks[(self.tpsf,self.tpsf+self.spawnNprocs-1)]  = os.environ['MPDMAN_UMASK']
            self.spawnExecs[(self.tpsf,self.tpsf+self.spawnNprocs-1)] = parsedMsg['execname']
            self.spawnUsers[(self.tpsf,self.tpsf+self.spawnNprocs-1)] = mpd_get_my_username()
            self.spawnEnv = {}
            self.spawnEnv.update(os.environ)
            self.spawnEnv['MPI_APPNUM'] = str(spawnssofar-1)
            self.spawnEnvvars[(self.tpsf,self.tpsf+self.spawnNprocs-1)] = self.spawnEnv
            self.spawnLimits[(self.tpsf,self.tpsf+self.spawnNprocs-1)] = {} # not implemented yet
            ##### args[(tpsf,tpsf+spawnNprocs-1) = [ parsedMsg['args'] ]
            ##### args[(tpsf,tpsf+spawnNprocs-1) = [ 'AA', 'BB', 'CC' ]
            cliArgs = []
            cliArgcnt = int(parsedMsg['argcnt'])
            for i in range(1,cliArgcnt+1):    # start at 1
                cliArgs.append(parsedMsg['arg%d' % i])
            self.spawnArgs[(self.tpsf,self.tpsf+self.spawnNprocs-1)] = cliArgs
            self.tpsf += self.spawnNprocs

            if totspawns == spawnssofar:    # This is the last in the spawn sequence
                self.spawnedCnt += 1    # non-zero to use for creating kvsname in msg below
                msgToSend = { 'cmd'          : 'spawn',
                              'conhost'      : self.myHost,
                              'conifhn'      : self.myIfhn,
                              'conport'      : self.listenNonRingPort,
                              'spawned'      : self.spawnedCnt,
                              'jobid'        : self.jobid,
                              'nstarted'     : 0,
                              'nprocs'       : self.tpsf,
                              'hosts'        : self.spawnHosts,
                              'execs'        : self.spawnExecs,
                              'users'        : self.spawnUsers,
                              'cwds'         : self.spawnCwds,
                              'umasks'       : self.spawnUmasks,
                              'paths'        : self.spawnPaths,
                              'args'         : self.spawnArgs,
                              'envvars'      : self.spawnEnvvars,
                              'limits'       : self.spawnLimits,
                              'singinitpid'  : 0,
                              'singinitport' : 0,
                            }
                msgToSend['line_labels'] = self.lineLabelFmt
                msgToSend['spawner_manpid'] = os.getpid()
                self.mpdSock.send_dict_msg(msgToSend)
                self.spawnInProgress = parsedMsg
                self.spawnInProgress['errcodes'] = [None] * self.tpsf  # one for each spawn
                # I could send the preput_info along but will keep it here
                # and let the spawnee call me up and ask for it; he will
                # call me anyway since I am his parent in the tree.  So, I
                # will create a KVS to hold the info until he calls
                self.spawnedKVSname = 'mpdman_kvs_for_spawned_' + str(self.spawnedCnt)
                self.KVSs[self.spawnedKVSname] = {}
                preput_num = int(parsedMsg['preput_num'])
                for i in range(0,preput_num):
                    preput_key = parsedMsg['preput_key_%d' % i]
                    preput_val = parsedMsg['preput_val_%d' % i]
                    self.KVSs[self.spawnedKVSname][preput_key] = preput_val
        elif parsedMsg['cmd'] == 'finalize':
            # the following lines are present to support a process that runs
            # 2 MPI pgms in tandem (e.g. mpish at ANL)
            self.KVSs = {}
            self.KVSs[self.default_kvsname] = {}
            self.kvs_next_id = 1
            self.jobEndingEarly = 0
            self.pmiCollectiveJob = 0
            self.spawnedCnt = 0
            pmiMsgToSend = 'cmd=finalize_ack\n' 
            self.pmiSock.send_char_msg(pmiMsgToSend)
        elif parsedMsg['cmd'] == 'client_bnr_fence_in':    ## BNR
            self.pmiBarrierInRecvd = 1
            if self.myRank == 0  or  self.holdingPMIBarrierLoop1:
                msgToSend = { 'cmd' : 'pmi_barrier_loop_1' }
                self.ring.rhsSock.send_dict_msg(msgToSend)
        elif parsedMsg['cmd'] == 'client_bnr_put':         ## BNR
            key = parsedMsg['attr']
            value = parsedMsg['val']
            try:
                self.KVSs[self.default_kvsname][key] = value
                pmiMsgToSend = 'cmd=put_result rc=0\n'
                self.pmiSock.send_char_msg(pmiMsgToSend)
            except Exception, errmsg:
                pmiMsgToSend = 'cmd=put_result rc=-1 msg="%s"\n' % errmsg
                self.pmiSock.send_char_msg(pmiMsgToSend)
        elif parsedMsg['cmd'] == 'client_bnr_get':          ## BNR
            key = parsedMsg['attr']
            if self.KVSs[self.default_kvsname].has_key(key):
                value = self.KVSs[self.default_kvsname][key]
                pmiMsgToSend = 'cmd=client_bnr_get_output rc=0 val=%s\n' % (value)
                self.pmiSock.send_char_msg(pmiMsgToSend)
            else:
                msgToSend = { 'cmd' : 'bnr_get', 'key' : key,
                              'kvsname' : kvsname, 'from_rank' : self.myRank }
                self.ring.rhsSock.send_dict_msg(msgToSend)
        elif parsedMsg['cmd'] == 'client_ready':               ## BNR
            ## continue to wait for accepting_signals
            pass
        elif parsedMsg['cmd'] == 'accepting_signals':          ## BNR
            ## handle it like a barrier_in ??
            self.pmiBarrierInRecvd = 1
            self.doingBNR = 1    ## BNR  # set again is OK
        elif parsedMsg['cmd'] == 'interrupt_peer_with_msg':    ## BNR
            self.ring.rhsSock.send_dict_msg(parsedMsg)
        else:
            mpd_print(1, "unrecognized pmi msg :%s:" % line )
    def handle_console_input(self,sock):
        msg = self.conSock.recv_dict_msg()
        if not msg:
            if self.conSock:
                self.streamHandler.del_handler(self.conSock)
                self.conSock.close()
                self.conSock = 0
            if self.parentStdoutSock:
                self.streamHandler.del_handler(self.parentStdoutSock)
                self.parentStdoutSock.close()
                self.parentStdoutSock = 0
            if self.parentStderrSock:
                self.streamHandler.del_handler(self.parentStderrSock)
                self.parentStderrSock.close()
                self.parentStderrSock = 0
            if self.ring.rhsSock:
                msgToSend = { 'cmd' : 'signal', 'signo' : 'SIGKILL' }
                self.ring.rhsSock.send_dict_msg(msgToSend)
            try:
                pgrp = clientPid * (-1)   # neg Pid -> group
                os.kill(pgrp,signal.SIGKILL)   # may be reaped by sighandler
            except:
                pass
        elif msg['cmd'] == 'signal':
            if msg['signo'] == 'SIGINT':
                self.ring.rhsSock.send_dict_msg(msg)
                for s in self.spawnedChildSocks:
                    s.send_dict_msg(msg)
                if self.gdb:
                    os.kill(clientPid,signal.SIGINT)
                else:
                    try:
                        pgrp = clientPid * (-1)   # neg Pid -> group
                        os.kill(pgrp,signal.SIGKILL)   # may be reaped by sighandler
                    except:
                        pass
            elif msg['signo'] == 'SIGKILL':
                try:
                    self.ring.rhsSock.send_dict_msg(msg)
                except:
                    pass
                for s in self.spawnedChildSocks:
                    try:
                        s.send_dict_msg(msg)
                    except:
                        pass
                if self.gdb:
                    os.kill(clientPid,signal.SIGUSR1)    # tell gdb driver to kill all
                else:
                    try:
                        pgrp = clientPid * (-1)   # neg Pid -> group
                        os.kill(pgrp,signal.SIGKILL)   # may be reaped by sighandler
                    except:
                        pass
            elif msg['signo'] == 'SIGTSTP':
                msg['dest'] = self.myId
                self.ring.rhsSock.send_dict_msg(msg)
                try:
                    pgrp = clientPid * (-1)   # neg Pid -> group
                    os.kill(pgrp,signal.SIGTSTP)   # may be reaped by sighandler
                except:
                    pass
            elif msg['signo'] == 'SIGCONT':
                msg['dest'] = self.myId
                self.ring.rhsSock.send_dict_msg(msg)
                try:
                    pgrp = clientPid * (-1)   # neg Pid -> group
                    os.kill(pgrp,signal.SIGCONT)   # may be reaped by sighandler
                except:
                    pass
        elif msg['cmd'] == 'stdin_from_user':
            msg['src'] = self.myId
            if self.ring.rhsSock:
                # Only send to rhs if that sock is open
                self.ring.rhsSock.send_dict_msg(msg)
            if in_stdinRcvrs(self.myRank,self.stdinDest):
                try:
                    if msg.has_key('eof'):
                        if self.subproc:    # must close subproc's file (not just the fd)
                            self.subproc.stdin.close()
                        else:
                            os.close(self.fd_write_cli_stdin)
                    else:
                        os.write(self.fd_write_cli_stdin,msg['line'])
                except:
                    mpd_print(1, 'cannot send stdin to client')
        elif msg['cmd'] == 'stdin_dest':
            self.stdinDest = msg['stdin_procs']
            msg['src'] = self.myId
            if self.ring.rhsSock:
                # Only send to rhs if that sock is open
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'tv_ready':
            self.tvReady = 1
            msg['src'] = self.myId
            self.ring.rhsSock.send_dict_msg(msg)
            if self.pmiSock:    # should be valid sock if running tv
                pmiMsgToSend = 'cmd=tv_ready\n'
                self.pmiSock.send_char_msg(pmiMsgToSend)
        else:
            mpd_print(1, 'unexpected msg recvd on conSock :%s:' % msg )
    def handle_mpd_input(self,sock):
        msg = self.mpdSock.recv_dict_msg()
        mpd_print(0000, 'msg recvd on mpdSock :%s:' % msg )
        if not msg:
            if self.conSock:
                msgToSend = { 'cmd' : 'job_aborted', 'reason' : 'mpd disappeared',
                              'jobid' : self.jobid }
                self.conSock.send_dict_msg(msgToSend,errprint=0)
                self.streamHandler.del_handler(self.conSock)
                self.conSock.close()
                self.conSock = 0
            try:
                os.kill(0,signal.SIGKILL)  # pid 0 -> all in my process group
            except:
                pass
            sys.exit(0)
        if msg['cmd'] == 'abortjob':
            mpd_print(1, "job aborted by mpd; reason=%s" % (msg['reason']))
        elif msg['cmd'] == 'startup_status':  # probably some hosts not found
            if self.pmiSock:  # may have disappeared in early shutdown
                pmiMsgToSend = 'cmd=spawn_result rc=-1 errcodes='' reason=%s\n' % (msg['reason'])
                self.pmiSock.send_char_msg(pmiMsgToSend)
        elif msg['cmd'] == 'signal_to_handle'  and  msg.has_key('sigtype'):
            if msg['sigtype'].isdigit():
                signum = int(msg['sigtype'])
            else:
                exec('signum = %s' % 'signal.SIG' + msg['sigtype'])
            try:    
                if msg['s_or_g'] == 's':    # single (not entire group)
                    pgrp = clientPid          # just client process
                else:
                    pgrp = clientPid * (-1)   # neg Pid -> process group
                os.kill(pgrp,signum)
            except Exception, errmsg:
                mpd_print(1, 'invalid signal (%d) from mpd' % (signum) )
        elif msg['cmd'] == 'publish_result':
            if self.pmiSock:
                pmiMsgToSend = 'cmd=publish_result info=%s\n' % (msg['info'])
                self.pmiSock.send_char_msg(pmiMsgToSend)
        elif msg['cmd'] == 'unpublish_result':
            if self.pmiSock:
                pmiMsgToSend = 'cmd=unpublish_result info=%s\n' % (msg['info'])
                self.pmiSock.send_char_msg(pmiMsgToSend)
        elif msg['cmd'] == 'lookup_result':
            if self.pmiSock:
                pmiMsgToSend = 'cmd=lookup_result info=%s port=%s\n' % \
                               (msg['info'],msg['port'])
                self.pmiSock.send_char_msg(pmiMsgToSend)
        elif msg['cmd'] == 'spawn_done_by_mpd':
            pass
        else:
            mpd_print(1, 'invalid msg recvd on mpdSock :%s:' % msg )
    def launch_client_via_fork_exec(self,cli_env):
        maxTries = 6
        numTries = 0
        while numTries < maxTries:
            try:
                cliPid = os.fork()
                errinfo = 0
            except OSError, errinfo:
                pass  ## could check for errinfo.errno == 35 (resource unavailable)
            if errinfo:
                sleep(1)
                numTries += 1
            else:
                break
        if numTries >= maxTries:
            ## print '**** mpdman: fork failed for launching client'
            return -1
        if cliPid == 0:
            mpd_set_my_id(socket.gethostname() + '_man_before_exec_client_' + `os.getpid()`)
            self.ring.lhsSock.close()
            self.ring.rhsSock.close()
            self.listenRingSock.close()
            if self.conSock:
                self.streamHandler.del_handler(self.conSock)
                self.conSock.close()
                self.conSock = 0
            self.pmiListenSock.close()
            os.setpgrp()

            os.close(self.fd_write_cli_stdin)
            os.dup2(self.fd_read_cli_stdin,0)  # closes fd 0 (stdin) if open

            # to simply print on the mpd's tty:
            #     comment out the next lines
            os.close(self.fd_read_cli_stdout)
            os.dup2(self.fd_write_cli_stdout,1)  # closes fd 1 (stdout) if open
            os.close(self.fd_write_cli_stdout)
            os.close(self.fd_read_cli_stderr)
            os.dup2(self.fd_write_cli_stderr,2)  # closes fd 2 (stderr) if open
            os.close(self.fd_write_cli_stderr)

            msg = self.handshake_sock_cli_end.recv_char_msg()
            if not msg.startswith('go'):
                mpd_print(1,'%s: invalid go msg from man :%s:' % (self.myId,msg) )
                sys.exit(-1)
            self.handshake_sock_cli_end.close()

            self.clientPgmArgs = [self.clientPgm] + self.clientPgmArgs
            errmsg = set_limits(self.clientPgmLimits)
            if errmsg:
                self.pmiSock = MPDSock(name='pmi')
                self.pmiSock.connect((self.myIfhn,self.pmiListenPort))
                reason = quote(str(errmsg))
                pmiMsgToSend = 'cmd=startup_status rc=-1 reason=%s exec=%s\n' % \
                               (reason,self.clientPgm)
                self.pmiSock.send_char_msg(pmiMsgToSend)
                sys.exit(0)
            try:
                mpd_print(0000, 'execing clientPgm=:%s:' % (self.clientPgm) )
                if self.gdb:
                    fullDirName = os.environ['MPDMAN_FULLPATHDIR']
                    gdbdrv = os.path.join(fullDirName,'mpdgdbdrv.py')
                    if not os.access(gdbdrv,os.X_OK):
                        print 'mpdman: cannot execute mpdgdbdrv %s' % gdbdrv
                        sys.exit(0);
                    if self.gdba:
                        self.clientPgmArgs.insert(0,'-attach')
                    self.clientPgmArgs.insert(0,self.clientPgm)
                    os.execvpe(gdbdrv,self.clientPgmArgs,cli_env)    # client
                else:
                    os.environ['PATH'] = cli_env['PATH']
                    os.execvpe(self.clientPgm,self.clientPgmArgs,cli_env)    # client
            except Exception, errmsg:
                # print '%s: could not run %s; probably executable file not found' % \
                #        (self.myId,clientPgm)
                self.pmiSock = MPDSock(name='pmi')
                self.pmiSock.connect((self.myIfhn,self.pmiListenPort))
                reason = quote(str(errmsg))
                pmiMsgToSend = 'cmd=startup_status rc=-1 reason=%s exec=%s\n' % \
                               (reason,self.clientPgm)
                self.pmiSock.send_char_msg(pmiMsgToSend)
                sys.exit(0)
            sys.exit(0)
        if not self.singinitPORT:
            os.close(self.fd_read_cli_stdin)
            os.close(self.fd_write_cli_stdout)
            os.close(self.fd_write_cli_stderr)
            self.cliListenSock.close()
        return cliPid
    def launch_client_via_subprocess(self,cli_env):
        import threading
        def read_fd_with_func(fd,func):
            line = 'x'
            while line:
                line = func(fd)
        tempListenSock = MPDListenSock()
        tempListenPort = tempListenSock.getsockname()[1]
        # python_executable = '\Python24\python.exe'
        python_executable = 'python2.4'
        fullDirName = os.environ['MPDMAN_FULLPATHDIR']
        mpdwrapcli = os.path.join(fullDirName,'mpdwrapcli.py')
        wrapCmdAndArgs = [ mpdwrapcli, str(tempListenPort),
                           self.clientPgm, self.clientPgm ] + self.clientPgmArgs
        cli_env.update(os.environ) ######  RMB: MAY NEED VARS OTHER THAN PATH ?????
        self.subproc = subprocess.Popen([python_executable,'-u'] + wrapCmdAndArgs,
                                        bufsize=0,env=cli_env,close_fds=False,
                                        stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE)
        self.fd_write_cli_stdin = self.subproc.stdin.fileno()
        stdout_thread = threading.Thread(target=read_fd_with_func,
                                         args=(self.subproc.stdout.fileno(),
                                               self.handle_cli_stdout_input))
        stdout_thread.start()
        stderr_thread = threading.Thread(target=read_fd_with_func,
                                         args=(self.subproc.stderr.fileno(),
                                               self.handle_cli_stderr_input))
        stderr_thread.start()
        (self.handshake_sock_man_end,tempAddr) = tempListenSock.accept()
        cliPid = self.subproc.pid
        ## an mpd_print wreaks havoc here; simple prints are OK (probably a stack issue)
        # mpd_print(0000,"CLIPID=%d" % cliPid)  
        # print "CLIPID=%d" % cliPid  ;  sys.stdout.flush()
        return cliPid
    def create_line_label(self,line_label_fmt,spawned):
        lineLabel = ''  # default is no label
        if line_label_fmt:
            i = 0
            while i < len(line_label_fmt):
                if line_label_fmt[i] == '%':
                    fmtchar = line_label_fmt[i+1]
                    i += 2
                    if fmtchar == 'r':
                        lineLabel += str(self.myRank)
                    elif fmtchar == 'h':
                        lineLabel += self.myHost
                else:
                    lineLabel += line_label_fmt[i]
                    i += 1
            if spawned:
                lineLabel += ',' + str(spawned) + ': '    # spawned is actually a count
            else:
                lineLabel += ': '
        return lineLabel

def in_stdinRcvrs(myRank,stdinDest):
    s1 = stdinDest.split(',')
    for s in s1:
        s2 = s.split('-')
        if len(s2) == 1:
            if myRank == int(s2[0]):
                return 1
        else:
            if myRank >= int(s2[0])  and  myRank <= int(s2[1]):
                return 1
    return 0
        

def parse_pmi_msg(msg):
    parsed_msg = {}
    try:
        sm = findall(r'\S+',msg)
        for e in sm:
            se = e.split('=')
            parsed_msg[se[0]] = se[1]
    except:
        print 'unable to parse pmi msg :%s:' % msg
        parsed_msg = {}
    return parsed_msg

def set_limits(limits):
    try:
        import resource
    except:
        return 'unable to import resource module to set limits'
    for limtype in limits.keys():
        limit = int(limits[limtype])
        try:
            if   limtype == 'core':
                resource.setrlimit(resource.RLIMIT_CORE,(limit,limit))
            elif limtype == 'cpu':
                resource.setrlimit(resource.RLIMIT_CPU,(limit,limit))
            elif limtype == 'fsize':
                resource.setrlimit(resource.RLIMIT_FSIZE,(limit,limit))
            elif limtype == 'data':
                resource.setrlimit(resource.RLIMIT_DATA,(limit,limit))
            elif limtype == 'stack':
                resource.setrlimit(resource.RLIMIT_STACK,(limit,limit))
            elif limtype == 'rss':
                resource.setrlimit(resource.RLIMIT_RSS,(limit,limit))
            elif limtype == 'nproc':
                resource.setrlimit(resource.RLIMIT_NPROC,(limit,limit))
            elif limtype == 'nofile':
                resource.setrlimit(resource.RLIMIT_NOFILE,(limit,limit))
            elif limtype == 'ofile':
                resource.setrlimit(resource.RLIMIT_OFILE,(limit,limit))
            elif limtype == 'memloc':
                resource.setrlimit(resource.RLIMIT_MEMLOCK,(limit,limit))
            elif  limtype == 'as':
                resource.setrlimit(resource.RLIMIT_AS,(limit,limit))
            elif  limtype == 'vmem':
                resource.setrlimit(resource.RLIMIT_VMEM,(limit,limit))
            else:
                raise NameError, 'invalid resource name: %s' % limtype  # validated at mpdrun
        except (NameError,ImportError), errmsg:
            return errmsg
    return 0

def sigchld_handler(signum,frame):
    global clientPid, clientExited, clientExitStatus, clientExitStatusSent
    done = 0
    while not done:
        try:
            (pid,status) = os.waitpid(-1,os.WNOHANG)
            if pid == 0:    # no existing child process is finished
                done = 1
            if pid == clientPid:
                clientExited = 1
                clientExitStatus = status
                mpd_handle_signal(signum,0)
        except:
            done = 1


if __name__ == '__main__':
    if not os.environ.has_key('MPDMAN_CLI_PGM'):    # assume invoked from keyboard
        print __doc__
        sys.exit(-1)
    mpdman = MPDMan()
    mpdman.run()
