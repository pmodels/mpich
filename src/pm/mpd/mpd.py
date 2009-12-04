#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage: mpd [--host=<host> --port=<portnum>] [--noconsole]
           [--trace] [--echo] [--daemon] [--bulletproof] --ncpus=<ncpus>
           [--ifhn=<interface-hostname>] [--listenport=<listenport>]
           [--pid=<pidfilename>] --tmpdir=<tmpdir>] [-zc] [--debug]

Some long parameter names may be abbreviated to their first letters by using
  only one hyphen and no equal sign:
     mpd -h donner -p 4268 -n
  is equivalent to
     mpd --host=magpie --port=4268 --noconsole

--host and --port must be specified together; they tell the new mpd where
  to enter an existing ring;  if they are omitted, the new mpd forms a
  stand-alone ring that other mpds may enter later
--noconsole is useful for running 2 mpds on the same machine; only one of
  them will accept mpd commands
--trace yields lots of traces thru mpd routines; currently too verbose
--debug turns on some debugging prints; currently not verbose enough
--echo causes the mpd echo its listener port by which other mpds may connect
--daemon causes mpd to run backgrounded, with no controlling tty
--bulletproof says to turn bulletproofing on (experimental)
--ncpus indicates how many cpus are on the local host; used for starting processes
--ifhn specifies an alternate interface hostname for the host this mpd is running on;
  e.g. may be used to specify the alias for an interface other than default
--listenport specifies a port for this mpd to listen on; by default it will
  acquire one from the system
--conlistenport specifies a port for this mpd to listen on for console
  connections (only used when employing inet socket for console); by default it
  will acquire one from the system
--pid=filename writes the mpd pid into the specified file, or --pid alone
  writes it into /var/run/mpd.pid
--tmpdir=tmpdirname where mpd places temporary sockets, etc.
-zc is a purely EXPERIMENTAL option right now used to investigate zeroconf
  networking; it can be used to allow mpds to discover each other locally
  using multicast DNS; its usage may change over time
  Currently, -zc is specified like this:  -zc N
  where N specifies a 'level' in a startup set of mpds.  The first mpd in a ring
  must have 1 and it will establish a ring of one mpd.  Subsequent mpds can specify
  -zc 2 and will hook into the ring via the one at level 1.  Except for level 1, new
  mpds enter the ring via an mpd at level-1.

A file named .mpd.conf file must be present in the user's home directory
  with read and write access only for the user, and must contain at least
  a line with MPD_SECRETWORD=<secretword>

To run mpd as root, install it while root and instead of a .mpd.conf file
use mpd.conf (no leading dot) in the /etc directory.' 
"""
from  time    import  ctime
from  mpdlib  import  mpd_version
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.160 $"
__version__ += "  " + str(mpd_version())
__credits__ = ""


import sys, os, signal, socket, stat

from  re          import  sub
from  atexit      import  register
from  cPickle     import  dumps
from  types       import  ClassType
from  random      import  seed, randrange, random
from  time        import  sleep
from  md5         import  new as md5new
from  mpdlib      import  mpd_set_my_id, mpd_check_python_version, mpd_sockpair, \
                          mpd_print, mpd_get_my_username, mpd_close_zc, \
                          mpd_get_groups_for_username, mpd_uncaught_except_tb, \
                          mpd_set_procedures_to_trace, mpd_trace_calls, \
                          mpd_dbg_level, mpd_set_dbg_level, mpd_set_tmpdir, \
                          MPDSock, MPDListenSock, MPDConListenSock, \
                          MPDStreamHandler, MPDRing, MPDParmDB
from  mpdman      import  MPDMan

# fix for ticket #753 where the set() builtin isn't available in python2.3
try:
    set
except NameError:
    from sets import Set as set


try:
    import pwd
    pwd_module_available = 1
except:
    pwd_module_available = 0
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


def sigchld_handler(signum,frame):
    done = 0
    while not done:
        try:
            (pid,status) = os.waitpid(-1,os.WNOHANG)
            if pid == 0:    # no existing child process is finished
                done = 1
        except:    # no more child processes to be waited for
            done = 1
            
class MPD(object):
    def __init__(self):
        self.myHost = socket.gethostname()
        try:
            hostinfo = socket.gethostbyname_ex(self.myHost)
            self.myIfhn = hostinfo[2][0]    # chgd below when I get the real value
        except:
            print 'mpd failed: gethostbyname_ex failed for %s' % (self.myHost)
            sys.exit(-1)
    def run(self):
        if syslog_module_available:
            syslog.openlog("mpd",0,syslog.LOG_DAEMON)
            syslog.syslog(syslog.LOG_INFO,"mpd starting; no mpdid yet")
        sys.excepthook = mpd_uncaught_except_tb
        self.spawnQ = []
        self.spawnInProgress = 0
        self.parmdb = MPDParmDB(orderedSources=['cmdline','xml','env','rcfile','thispgm'])
        self.parmsToOverride = {
                                 'MPD_SECRETWORD'       :  '',
                                 'MPD_MY_IFHN'          :  self.myIfhn,
                                 'MPD_ENTRY_IFHN'       :  '',
                                 'MPD_ENTRY_PORT'       :  0,
                                 'MPD_NCPUS'            :  1,
                                 'MPD_LISTEN_PORT'      :  0,
                                 'MPD_TRACE_FLAG'       :  0,
                                 'MPD_CONSOLE_FLAG'     :  1,
                                 'MPD_ECHO_PORT_FLAG'   :  0,
                                 'MPD_DAEMON_FLAG'      :  0,
                                 'MPD_BULLETPROOF_FLAG' :  0,
                                 'MPD_PID_FILENAME'     :  '',
                                 'MPD_ZC'               :  0,
                                 'MPD_LOGFILE_TRUNC_SZ' :  4000000,  # -1 -> don't trunc
                                 'MPD_PORT_RANGE'       :  0,
                                 'MPD_TMPDIR'           :  '/tmp',
                               }
        for (k,v) in self.parmsToOverride.items():
            self.parmdb[('thispgm',k)] = v
        self.get_parms_from_cmdline()
        self.parmdb.get_parms_from_rcfile(self.parmsToOverride,errIfMissingFile=1)
        self.parmdb.get_parms_from_env(self.parmsToOverride)
        self.myIfhn = self.parmdb['MPD_MY_IFHN']    # variable for convenience
        self.myPid = os.getpid()
        if self.parmdb['MPD_PORT_RANGE']:
            os.environ['MPICH_PORT_RANGE'] = self.parmdb['MPD_PORT_RANGE']
        self.tmpdir = self.parmdb['MPD_TMPDIR']
        mpd_set_tmpdir(self.tmpdir)
        self.listenSock = MPDListenSock(name='ring_listen_sock',
                                        port=self.parmdb['MPD_LISTEN_PORT'])
        self.parmdb[('thispgm','MPD_LISTEN_PORT')] = self.listenSock.sock.getsockname()[1]
        self.myId = '%s_%d' % (self.myHost,self.parmdb['MPD_LISTEN_PORT'])
        mpd_set_my_id(myid=self.myId)
        self.streamHandler = MPDStreamHandler()
        self.ring = MPDRing(streamHandler=self.streamHandler,
                            secretword=self.parmdb['MPD_SECRETWORD'],
                            listenSock=self.listenSock,
                            myIfhn=self.myIfhn,
                            entryIfhn=self.parmdb['MPD_ENTRY_IFHN'],
                            entryPort=self.parmdb['MPD_ENTRY_PORT'],
                            zcMyLevel=self.parmdb['MPD_ZC'])
        # setup tracing if requested via args
        if self.parmdb['MPD_TRACE_FLAG']:
            proceduresToTrace = []
            import inspect
            symbolsAndTypes = globals().items() + \
                              inspect.getmembers(self) + \
                              inspect.getmembers(self.ring) + \
                              inspect.getmembers(self.streamHandler)
            for (symbol,symtype) in symbolsAndTypes:
                if symbol == '__init__':  # a problem to trace
                    continue
                if inspect.isfunction(symtype)  or  inspect.ismethod(symtype):
                    # print symbol
                    proceduresToTrace.append(symbol)
            mpd_set_procedures_to_trace(proceduresToTrace)
            sys.settrace(mpd_trace_calls)
        if syslog_module_available:
            syslog.syslog(syslog.LOG_INFO,"mpd has mpdid=%s (port=%d)" % \
                          (self.myId,self.parmdb['MPD_LISTEN_PORT']) )
        vinfo = mpd_check_python_version()
        if vinfo:
            print "mpd: your python version must be >= 2.2 ; current version is:", vinfo
            sys.exit(-1)

        # need to close both object and underlying fd (ticket #963)
        sys.stdin.close()
        os.close(0)

        if self.parmdb['MPD_ECHO_PORT_FLAG']:    # do this before becoming a daemon
            # print self.parmdb['MPD_LISTEN_PORT']
            print "mpd_port=%d" % self.parmdb['MPD_LISTEN_PORT']
            sys.stdout.flush()
            ##### NEXT 2 for debugging
            ## print >>sys.stderr, self.parmdb['MPD_LISTEN_PORT']
            ## sys.stderr.flush()
        self.myRealUsername = mpd_get_my_username()
        self.currRingSize = 1    # default
        self.currRingNCPUs = 1   # default
        if os.environ.has_key('MPD_CON_EXT'):
            self.conExt = '_'  + os.environ['MPD_CON_EXT']
        else:
            self.conExt = ''
        self.logFilename = self.tmpdir + '/mpd2.logfile_' + mpd_get_my_username() + self.conExt
        if self.parmdb['MPD_PID_FILENAME']:  # may overwrite it below
            pidFile = open(self.parmdb['MPD_PID_FILENAME'],'w')
            print >>pidFile, "%d" % (os.getpid())
            pidFile.close()

        self.conListenSock = 0    # don't want one when I do cleanup for forked daemon procs
        if self.parmdb['MPD_DAEMON_FLAG']:      # see if I should become a daemon with no controlling tty
            rc = os.fork()
            if rc != 0:   # parent exits; child in background
                sys.exit(0)
            os.setsid()  # become session leader; no controlling tty
            signal.signal(signal.SIGHUP,signal.SIG_IGN)  # make sure no sighup when leader ends
            ## leader exits; svr4: make sure do not get another controlling tty
            rc = os.fork()
            if rc != 0:
                sys.exit(0)
            if self.parmdb['MPD_PID_FILENAME']:  # overwrite one above before chg usmask
                pidFile = open(self.parmdb['MPD_PID_FILENAME'],'w')
                print >>pidFile, "%d" % (os.getpid())
                pidFile.close()
            os.chdir("/")  # free up filesys for umount
            os.umask(0)
            try:    os.unlink(self.logFilename)
            except: pass
            logFileFD = os.open(self.logFilename,os.O_CREAT|os.O_WRONLY|os.O_EXCL,0600)
            self.logFile = os.fdopen(logFileFD,'w',0)
            sys.stdout = self.logFile
            sys.stderr = self.logFile
            print >>sys.stdout, 'logfile for mpd with pid %d' % os.getpid()
            sys.stdout.flush()
            os.dup2(self.logFile.fileno(),sys.__stdout__.fileno())
            os.dup2(self.logFile.fileno(),sys.__stderr__.fileno())
        if self.parmdb['MPD_CONSOLE_FLAG']:
            self.conListenSock = MPDConListenSock(secretword=self.parmdb['MPD_SECRETWORD'])
            self.streamHandler.set_handler(self.conListenSock,
                                           self.handle_console_connection)
        register(self.cleanup)
        seed()
        self.nextJobInt    = 1
        self.activeJobs    = {}
        self.conSock       = 0
        self.allExiting    = 0    # for mpdallexit (for first loop for graceful exit)
        self.exiting       = 0    # for mpdexit or mpdallexit
        self.kvs_cntr      = 0    # for mpdman
        self.pulse_cntr    = 0
        rc = self.ring.enter_ring(lhsHandler=self.handle_lhs_input,
                                  rhsHandler=self.handle_rhs_input)
        if rc < 0:
            mpd_print(1,"failed to enter ring")
            sys.exit(-1)
        self.pmi_published_names = {}
        if hasattr(signal,'SIGCHLD'):
            signal.signal(signal.SIGCHLD,sigchld_handler)
        if not self.parmdb['MPD_BULLETPROOF_FLAG']:
            #    import profile ; profile.run('self.runmainloop()')
            self.runmainloop()
        else:
            try:
                from threading import Thread
            except:
                print '*** mpd terminating'
                print '    bulletproof option must be able to import threading-Thread'
                sys.exit(-1)
            # may use SIG_IGN on all but SIGCHLD and SIGHUP (handled above)
            while 1:
                mpdtid = Thread(target=self.runmainloop)
                mpdtid.start()
                # signals must be handled in main thread; thus we permit timeout of join
                while mpdtid.isAlive():
                    mpdtid.join(2)   # come out sometimes and handle signals
                if self.exiting:
                    break
                if self.conSock:
                    msgToSend = { 'cmd' : 'restarting_mpd' }
                    self.conSock.msgToSend.send_dict_msg(msgToSend)
                    self.streamHandler.del_handler(self.conSock)
                    self.conSock.close()
                    self.conSock = 0
    def runmainloop(self):
        # Main Loop
        while 1:
            if self.spawnQ  and  not self.spawnInProgress:
                self.ring.rhsSock.send_dict_msg(self.spawnQ[0])
                self.spawnQ = self.spawnQ[1:]
                self.spawnInProgress = 1
                continue
            rv = self.streamHandler.handle_active_streams(timeout=8.0)
            if rv[0] < 0:
                if type(rv[1]) == ClassType  and  rv[1] == KeyboardInterrupt: # ^C
                    sys.exit(-1)
            if self.exiting:
                break
            if rv[0] == 0:
                if self.pulse_cntr == 0  and  self.ring.rhsSock:
                    self.ring.rhsSock.send_dict_msg({'cmd':'pulse'})
                self.pulse_cntr += 1
            if self.pulse_cntr >= 3:
                if self.ring.rhsSock:  # rhs must have disappeared
                    self.streamHandler.del_handler(self.ring.rhsSock)
                    self.ring.rhsSock.close()
                    self.ring.rhsSock = 0
                if self.ring.lhsSock:
                    self.streamHandler.del_handler(self.ring.lhsSock)
                    self.ring.lhsSock.close()
                    self.ring.lhsSock = 0
                mpd_print(1,'no pulse_ack from rhs; re-entering ring')
                rc = self.ring.reenter_ring(lhsHandler=self.handle_lhs_input,
                                            rhsHandler=self.handle_rhs_input,
                                            ntries=16)
                if rc == 0:
                    mpd_print(1,'back in ring')
		else:
                    mpd_print(1,'failed to reenter ring')
                    sys.exit(-1)
                self.pulse_cntr = 0
        mpd_close_zc()  # only does something if we have zc
    def usage(self):
        print __doc__
        print "This version of mpd is", mpd_version()
        sys.exit(-1)
    def cleanup(self):
        try:
            mpd_print(0, "CLEANING UP" )
            if syslog_module_available:
                syslog.syslog(syslog.LOG_INFO,"mpd ending mpdid=%s (inside cleanup)" % \
                              (self.myId) )
                syslog.closelog()
            if self.conListenSock:    # only del if I created
                os.unlink(self.conListenSock.conFilename)
        except:
            pass
    def get_parms_from_cmdline(self):
        global mpd_dbg_level
        argidx = 1
        while argidx < len(sys.argv):
            if sys.argv[argidx] == '--help':
                self.usage()
                argidx += 1
            elif sys.argv[argidx] == '-h':
                if len(sys.argv) < 3:
                    self.usage()
                self.parmdb[('cmdline','MPD_ENTRY_IFHN')] = sys.argv[argidx+1]
                argidx += 2
            elif sys.argv[argidx].startswith('--host'):
                try:
                    entryHost = sys.argv[argidx].split('=',1)[1]
                except:
                    print 'failed to parse --host option'
                    self.usage()
                self.parmdb[('cmdline','MPD_ENTRY_IFHN')] = entryHost
                argidx += 1
            elif sys.argv[argidx] == '-p':
                if argidx >= (len(sys.argv)-1):
                    print 'missing arg for -p'
                    sys.exit(-1)
                if not sys.argv[argidx+1].isdigit():
                    print 'invalid port %s ; must be numeric' % (sys.argv[argidx+1])
                    sys.exit(-1)
                self.parmdb[('cmdline','MPD_ENTRY_PORT')] = int(sys.argv[argidx+1])
                argidx += 2
            elif sys.argv[argidx].startswith('--port'):
                try:
                    entryPort = sys.argv[argidx].split('=',1)[1]
                except:
                    print 'failed to parse --port option'
                    self.usage()
                if not entryPort.isdigit():
                    print 'invalid port %s ; must be numeric' % (entryPort)
                    sys.exit(-1)
                self.parmdb[('cmdline','MPD_ENTRY_PORT')] = int(entryPort)
                argidx += 1
            elif sys.argv[argidx].startswith('--ncpus'):
                try:
                    NCPUs = sys.argv[argidx].split('=',1)[1]
                except:
                    print 'failed to parse --ncpus option'
                    self.usage()
                if not NCPUs.isdigit():
                    print 'invalid ncpus %s ; must be numeric' % (NCPUs)
                    sys.exit(-1)
                self.parmdb[('cmdline','MPD_NCPUS')] = int(NCPUs)
                argidx += 1
            elif sys.argv[argidx].startswith('--pid'):
                try:
                    splitPid = sys.argv[argidx].split('=')
                except:
                    print 'failed to parse --pid option'
                    self.usage()
                if len(splitPid) == 1  or  not splitPid[1]:
                    pidFilename = '/var/run/mpd.pid'
                else:
                    pidFilename = splitPid[1]
                self.parmdb[('cmdline','MPD_PID_FILENAME')] = pidFilename
                argidx += 1
            elif sys.argv[argidx].startswith('--tmpdir'):
                try:
                    splitTmpdir = sys.argv[argidx].split('=')
                except:
                    print 'failed to parse --tmpdir option'
                    self.usage()
                if len(splitTmpdir) == 1  or  not splitTmpdir[1]:
                    tmpdirName = '/tmp'
                else:
                    tmpdirName = splitTmpdir[1]
                self.parmdb[('cmdline','MPD_TMPDIR')] = tmpdirName
                argidx += 1
            elif sys.argv[argidx].startswith('--ifhn'):
                try:
                    ifhn = sys.argv[argidx].split('=',1)[1]
                except:
                    print 'failed to parse --ifhn option'
                    self.usage()
                try:
                    hostinfo = socket.gethostbyname_ex(ifhn)
                    ifhn = hostinfo[2][0]
                except:
                    print 'mpd failed: gethostbyname_ex failed for %s' % (ifhn)
                    sys.exit(-1)
                self.parmdb[('cmdline','MPD_MY_IFHN')] = ifhn
                argidx += 1
            elif sys.argv[argidx] == '-l':
                if argidx >= (len(sys.argv)-1):
                    print 'missing arg for -l'
                    sys.exit(-1)
                if not sys.argv[argidx+1].isdigit():
                    print 'invalid listenport %s ; must be numeric' % (sys.argv[argidx+1])
                    sys.exit(-1)
                self.parmdb[('cmdline','MPD_LISTEN_PORT')] = int(sys.argv[argidx+1])
                argidx += 2
            elif sys.argv[argidx].startswith('--listenport'):
                try:
                    myListenPort = sys.argv[argidx].split('=',1)[1]
                except:
                    print 'failed to parse --listenport option'
                    self.usage()
                if not myListenPort.isdigit():
                    print 'invalid listenport %s ; must be numeric' % (myListenPort)
                    sys.exit(-1)
                self.parmdb[('cmdline','MPD_LISTEN_PORT')] = int(myListenPort)
                argidx += 1
            elif sys.argv[argidx] == '-hp':
                if argidx >= (len(sys.argv)-1):
                    print 'missing arg for -hp'
                    sys.exit(-1)
                try:
                    (entryIfhn,entryPort) = sys.argv[argidx+1].split('_')
                except:
                    print 'invalid entry host: %s' % (sys.argv[argidx+1])
                    sys.exit(-1)
                if not entryPort.isdigit():
                    print 'invalid port %s ; must be numeric' % (sys.argv[argidx+1])
                    sys.exit(-1)
                self.parmdb[('cmdline','MPD_ENTRY_IFHN')] = entryIfhn
                self.parmdb[('cmdline','MPD_ENTRY_PORT')] = int(entryPort)
                argidx += 2
            elif sys.argv[argidx] == '-t'  or  sys.argv[argidx] == '--trace':
                self.parmdb[('cmdline','MPD_TRACE_FLAG')] = 1
                argidx += 1
            elif sys.argv[argidx] == '--debug':
                mpd_set_dbg_level(1)
                argidx += 1
            elif sys.argv[argidx] == '-n'  or  sys.argv[argidx] == '--noconsole':
                self.parmdb[('cmdline','MPD_CONSOLE_FLAG')] = 0
                argidx += 1
            elif sys.argv[argidx] == '-e'  or  sys.argv[argidx] == '--echo':
                self.parmdb[('cmdline','MPD_ECHO_PORT_FLAG')] = 1 
                argidx += 1
            elif sys.argv[argidx] == '-d'  or  sys.argv[argidx] == '--daemon':
                self.parmdb[('cmdline','MPD_DAEMON_FLAG')] = 1 
                argidx += 1
            elif sys.argv[argidx] == '-b'  or  sys.argv[argidx] == '--bulletproof':
                self.parmdb[('cmdline','MPD_BULLETPROOF_FLAG')] = 1 
                argidx += 1
            elif sys.argv[argidx] == '-zc':
                if argidx >= (len(sys.argv)-1):
                    print 'missing arg for -zc'
                    sys.exit(-1)
                if not sys.argv[argidx+1].isdigit():
                    print 'invalid arg for -zc %s ; must be numeric' % (sys.argv[argidx+1])
                    sys.exit(-1)
                intarg = int(sys.argv[argidx+1])
                if intarg < 1:
                    print 'invalid arg for -zc %s ; must be >= 1' % (sys.argv[argidx+1])
                    sys.exit(-1)
                self.parmdb[('cmdline','MPD_ZC')] = intarg
                argidx += 2
            else:
                print 'unrecognized arg: %s' % (sys.argv[argidx])
                sys.exit(-1)
        if (self.parmdb['MPD_ENTRY_IFHN']  and  not self.parmdb['MPD_ENTRY_PORT']) \
        or (self.parmdb['MPD_ENTRY_PORT']  and  not self.parmdb['MPD_ENTRY_IFHN']):
            print 'host and port must be specified together'
            sys.exit(-1)
    def handle_console_connection(self,sock):
        if not self.conSock:
            (self.conSock,newConnAddr) = sock.accept()
            if hasattr(socket,'AF_UNIX')  and  sock.family == socket.AF_UNIX:
                line = self.conSock.recv_char_msg().rstrip()
                if not line:  # caller went away (perhaps another mpd seeing if I am here)
                    self.streamHandler.del_handler(self.conSock)
                    self.conSock.close()
                    self.conSock = 0
                    return
                errorMsg = ''
                try:
                    (kv1,kv2) = line.split(' ',1)  # 'realusername=xxx secretword=yyy'
                except:
                    errorMsg = 'failed to split this msg on " ": %s' % line
                if not errorMsg:
                    try:
                        (k1,self.conSock.realUsername) = kv1.split('=',1)
                    except:
                        errorMsg = 'failed to split first kv pair on "=": %s' % line
                if not errorMsg:
                    try:
                        (k2,secretword) = kv2.split('=',1)
                    except:
                        errorMsg = 'failed to split second kv pair on "=": %s' % line
                if not errorMsg  and  k1 != 'realusername':
                    errorMsg = 'first key is not realusername'
                if not errorMsg  and  k2 != 'secretword':
                    errorMsg = 'second key is not secretword'
                if not errorMsg  and  os.getuid() == 0  and  secretword != self.parmdb['MPD_SECRETWORD']:
                    errorMsg = 'invalid secretword to root mpd'
                if errorMsg:
                    try:
                        self.conSock.send_dict_msg({'error_msg': errorMsg})
                    except:
                        pass
                    self.streamHandler.del_handler(self.conSock)
                    self.conSock.close()
                    self.conSock = 0
                    return
                self.conSock.beingChallenged = 0
            else:
                msg = self.conSock.recv_dict_msg()
                if not msg:    # caller went away (perhaps another mpd seeing if I am here)
                    self.streamHandler.del_handler(self.conSock)
                    self.conSock.close()
                    self.conSock = 0
                    return
                if not msg.has_key('cmd')  or  msg['cmd'] != 'con_init':
                    mpd_print(1, 'console sent bad msg :%s:' % (msg) )
                    try:  # try to let console know
                        self.conSock.send_dict_msg({'cmd':'invalid_msg_received_from_you'})
                    except:
                        pass
                    self.streamHandler.del_handler(self.conSock)
                    self.conSock.close()
                    self.conSock = 0
                    return
                self.streamHandler.set_handler(self.conSock,self.handle_console_input)
                self.conSock.beingChallenged = 1
                self.conSock.name = 'console'
                randNum = randrange(1,10000)
                randVal = sock.secretword + str(randNum)
                self.conSock.expectedResponse = md5new(randVal).digest()
                self.conSock.send_dict_msg({'cmd' : 'con_challenge', 'randnum' : randNum })
                self.conSock.realUsername = mpd_get_my_username()
            self.streamHandler.set_handler(self.conSock,self.handle_console_input)
            self.conSock.name = 'console'
        else:
            return  ## postpone it; hope the other one frees up soon
    def handle_console_input(self,sock):
        msg = self.conSock.recv_dict_msg()
        if not msg:
            mpd_print(0000, 'console has disappeared; closing it')
            self.streamHandler.del_handler(self.conSock)
            self.conSock.close()
            self.conSock = 0
            return
        if not msg.has_key('cmd'):
            mpd_print(1, 'console sent bad msg :%s:' % msg)
            try:  # try to let console know
                self.conSock.send_dict_msg({ 'cmd':'invalid_msg_received_from_you' })
            except:
                pass
            self.streamHandler.del_handler(self.conSock)
            self.conSock.close()
            self.conSock = 0
            return
        if self.conSock.beingChallenged  and  msg['cmd'] != 'con_challenge_response':
            mpd_print(1, 'console did not respond to con_challenge; msg=:%s:' % msg)
            try:  # try to let console know
                self.conSock.send_dict_msg({ 'cmd':'expected_con_challenge_response' })
            except:
                pass
            self.streamHandler.del_handler(self.conSock)
            self.conSock.close()
            self.conSock = 0
            return
        if msg['cmd'] == 'con_challenge_response':
            self.conSock.beingChallenged = 0
            self.conSock.realUsername = msg['realusername']
            if not msg.has_key('response'):
                try:  # try to let console know
                    self.conSock.send_dict_msg({ 'cmd':'missing_response_in_msg' })
                except:
                    pass
                self.streamHandler.del_handler(self.conSock)
                self.conSock.close()
                self.conSock = 0
                return
            elif msg['response'] != self.conSock.expectedResponse:
                try:  # try to let console know
                    self.conSock.send_dict_msg({ 'cmd':'invalid_response' })
                except:
                    pass
                self.streamHandler.del_handler(self.conSock)
                self.conSock.close()
                self.conSock = 0
                return
            self.conSock.send_dict_msg({ 'cmd':'valid_response' })
        elif msg['cmd'] == 'mpdrun':
            # permit anyone to run but use THEIR own username
            #   thus, override any username specified by the user
            if self.conSock.realUsername != 'root':
                msg['username'] = self.conSock.realUsername
                msg['users'] = { (0,msg['nprocs']-1) : self.conSock.realUsername }
            #
            msg['mpdid_mpdrun_start'] = self.myId
            msg['nstarted_on_this_loop'] = 0
            msg['first_loop'] = 1
            msg['ringsize'] = 0
            msg['ring_ncpus'] = 0
            # maps rank => hostname
            msg['process_mapping'] = {}
            if msg.has_key('try_1st_locally'):
                self.do_mpdrun(msg)
            else:
                self.ring.rhsSock.send_dict_msg(msg)
            # send ack after job is going
        elif msg['cmd'] == 'get_mpdrun_values':
            msgToSend = { 'cmd' : 'response_get_mpdrun_values',
	                  'mpd_version' : mpd_version(),
	                  'mpd_ifhn' : self.myIfhn }
            self.conSock.send_dict_msg(msgToSend)
        elif msg['cmd'] == 'mpdtrace':
            msgToSend = { 'cmd'     : 'mpdtrace_info',
                          'dest'    : self.myId,
                          'id'      : self.myId,
                          'ifhn'    : self.myIfhn,
                          'lhsport' : '%s' % (self.ring.lhsPort),
                          'lhsifhn' : '%s' % (self.ring.lhsIfhn),
                          'rhsport' : '%s' % (self.ring.rhsPort),
                          'rhsifhn' : '%s' % (self.ring.rhsIfhn) }
            self.ring.rhsSock.send_dict_msg(msgToSend)
            msgToSend = { 'cmd'  : 'mpdtrace_trailer', 'dest' : self.myId }
            self.ring.rhsSock.send_dict_msg(msgToSend)
            # do not send an ack to console now; will send trace info later
        elif msg['cmd'] == 'mpdallexit':
            if self.conSock.realUsername != self.myRealUsername:
                msgToSend = { 'cmd':'invalid_username_to_make_this_request' }
                self.conSock.send_dict_msg(msgToSend)
                self.streamHandler.del_handler(self.conSock)
                self.conSock.close()
                self.conSock = 0
                return
            # self.allExiting = 1  # doesn't really help here
            self.ring.rhsSock.send_dict_msg( {'cmd' : 'mpdallexit', 'src' : self.myId} )
            self.conSock.send_dict_msg( {'cmd' : 'mpdallexit_ack'} )
        elif msg['cmd'] == 'mpdexit':
            if self.conSock.realUsername != self.myRealUsername:
                msgToSend = { 'cmd':'invalid_username_to_make_this_request' }
                self.conSock.send_dict_msg(msgToSend)
                self.streamHandler.del_handler(self.conSock)
                self.conSock.close()
                self.conSock = 0
                return
            if msg['mpdid'] == 'localmpd':
                msg['mpdid'] = self.myId
            self.ring.rhsSock.send_dict_msg( {'cmd' : 'mpdexit', 'src' : self.myId,
                                              'done' : 0, 'dest' : msg['mpdid']} )
        elif msg['cmd'] == 'mpdringtest':
            msg['src'] = self.myId
            self.ring.rhsSock.send_dict_msg(msg)
            # do not send an ack to console now; will send ringtest info later
        elif msg['cmd'] == 'mpdlistjobs':
            msgToSend = { 'cmd'  : 'local_mpdid', 'id' : self.myId }
            self.conSock.send_dict_msg(msgToSend)
            for jobid in self.activeJobs.keys():
                for manPid in self.activeJobs[jobid]:
                    msgToSend = { 'cmd' : 'mpdlistjobs_info',
                                  'dest' : self.myId,
                                  'jobid' : jobid,
                                  'username' : self.activeJobs[jobid][manPid]['username'],
                                  'host' : self.myHost,
                                  'ifhn' : self.myIfhn,
                                  'clipid' : str(self.activeJobs[jobid][manPid]['clipid']),
                                  'sid' : str(manPid),  # may chg to actual sid later
                                  'pgm'  : self.activeJobs[jobid][manPid]['pgm'],
                                  'rank' : self.activeJobs[jobid][manPid]['rank'] }
                    self.conSock.send_dict_msg(msgToSend)
            msgToSend = { 'cmd'  : 'mpdlistjobs_trailer', 'dest' : self.myId }
            self.ring.rhsSock.send_dict_msg(msgToSend)
            # do not send an ack to console now; will send listjobs info later
        elif msg['cmd'] == 'mpdkilljob':
            # permit anyone to kill but use THEIR own username
            #   thus, override any username specified by the user
            if self.conSock.realUsername != 'root':
                msg['username'] = self.conSock.realUsername
            msg['src'] = self.myId
            msg['handled'] = 0
            if msg['mpdid'] == '':
                msg['mpdid'] = self.myId
            self.ring.rhsSock.send_dict_msg(msg)
            # send ack to console after I get this msg back and do the kill myself
        elif msg['cmd'] == 'mpdsigjob':
            # permit anyone to sig but use THEIR own username
            #   thus, override any username specified by the user
            if self.conSock.realUsername != 'root':
                msg['username'] = self.conSock.realUsername
            msg['src'] = self.myId
            msg['handled'] = 0
            if msg['mpdid'] == '':
                msg['mpdid'] = self.myId
            self.ring.rhsSock.send_dict_msg(msg)
            # send ack to console after I get this msg back
        elif msg['cmd'] == 'verify_hosts_in_ring':
            msgToSend = { 'cmd'  : 'verify_hosts_in_ring', 'dest' : self.myId,
                          'host_list' : msg['host_list'] }
            self.ring.rhsSock.send_dict_msg(msgToSend)
            # do not send an ack to console now; will send trace info later
        else:
            msgToSend = { 'cmd' : 'invalid_msg_received_from_you' }
            self.conSock.send_dict_msg(msgToSend)
            badMsg = 'invalid msg received from console: %s' % (str(msg))
            mpd_print(1, badMsg)
            if syslog_module_available:
                syslog.syslog(syslog.LOG_ERR,badMsg)
    def handle_man_input(self,sock):
        msg = sock.recv_dict_msg()
        if not msg:
            for jobid in self.activeJobs.keys():
                deleted = 0
                for manPid in self.activeJobs[jobid]:
                    if sock == self.activeJobs[jobid][manPid]['socktoman']:
			mpd_print(mpd_dbg_level,\
                                  "Deleting %s %d" % (str(jobid),manPid))
                        del self.activeJobs[jobid][manPid]
                        if len(self.activeJobs[jobid]) == 0:
                            del self.activeJobs[jobid]
                        deleted = 1
                        break
                if deleted:
                    break
            self.streamHandler.del_handler(sock)
            sock.close()
            return
        if not msg.has_key('cmd'):
            mpd_print(1, 'INVALID msg for man request msg=:%s:' % (msg) )
            msgToSend = { 'cmd' : 'invalid_msg' }
            sock.send_dict_msg(msgToSend)
            self.streamHandler.del_handler(sock)
            sock.close()
            return
	# Who asks, and why?  
        # We have a failure that deletes the spawnerManPid from the
	# activeJobs[jobid] variable.   The temporary work-around is
        # to ignore this request if the target process is no longer 
	# in the activeJobs table.
        if msg['cmd'] == 'client_info':
            jobid = msg['jobid']
            manPid = msg['manpid']
            self.activeJobs[jobid][manPid]['clipid'] = msg['clipid']
            if msg['spawner_manpid']  and  msg['rank'] == 0:
                if msg['spawner_mpd'] == self.myId:
                    spawnerManPid = msg['spawner_manpid']
		    mpd_print(mpd_dbg_level,\
                       "About to check %s:%s" % (str(jobid),str(spawnerManPid)))

                    if not self.activeJobs[jobid].has_key(spawnerManPid):
                        mpd_print(0,"Missing %d in %s" % (spawnerManPid,str(jobid)))
                    elif not self.activeJobs[jobid][spawnerManPid].has_key('socktoman'):
                        mpd_print(0,"Missing socktoman!")
                    else:
                        spawnerManSock = self.activeJobs[jobid][spawnerManPid]['socktoman']
                        msgToSend = { 'cmd' : 'spawn_done_by_mpd', 'rc' : 0, 'reason' : '' }
                        spawnerManSock.send_dict_msg(msgToSend)
                else:
                    self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'spawn':
            msg['mpdid_mpdrun_start'] = self.myId
            msg['spawner_mpd'] = self.myId
            msg['nstarted_on_this_loop'] = 0
            msg['first_loop'] = 1
            msg['jobalias'] = ''
            msg['stdin_dest'] = '0'
            msg['ringsize'] = 0
            msg['ring_ncpus'] = 0
            msg['gdb'] = 0
            msg['gdba'] = ''
            msg['totalview'] = 0
            msg['ifhns'] = {}
            # maps rank => hostname
            msg['process_mapping'] = {}
            self.spawnQ.append(msg)
        elif msg['cmd'] == 'publish_name':
            self.pmi_published_names[msg['service']] = msg['port']
            msgToSend = { 'cmd' : 'publish_result', 'info' : 'ok' }
            sock.send_dict_msg(msgToSend)
        elif msg['cmd'] == 'lookup_name':
            if self.pmi_published_names.has_key(msg['service']):
                msgToSend = { 'cmd' : 'lookup_result', 'info' : 'ok',
                              'port' : self.pmi_published_names[msg['service']] }
                sock.send_dict_msg(msgToSend)
            else:
                msg['cmd'] = 'pmi_lookup_name'    # add pmi_
                msg['src'] = self.myId
                msg['port'] = 0    # invalid
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'unpublish_name':
            if self.pmi_published_names.has_key(msg['service']):
                del self.pmi_published_names[msg['service']]
                msgToSend = { 'cmd' : 'unpublish_result', 'info' : 'ok' }
                sock.send_dict_msg(msgToSend)
            else:
                msg['cmd'] = 'pmi_unpublish_name'    # add pmi_
                msg['src'] = self.myId
                self.ring.rhsSock.send_dict_msg(msg)
        else:
            mpd_print(1, 'INVALID request from man msg=:%s:' % (msg) )
            msgToSend = { 'cmd' : 'invalid_request' }
            sock.send_dict_msg(msgToSend)

    def calculate_process_mapping(self,mapping_dict):
        # mapping_dict maps ranks => hostnames
        ranks = list(mapping_dict.keys())
        ranks.sort()

        # assign node ids based in first-come-first-serve order when iterating
        # over the ranks in increasing order
        next_id = 0
        node_ids = {}
        for rank in ranks:
            host = mapping_dict[rank]
            if not node_ids.has_key(host):
                node_ids[host] = next_id
                next_id += 1


        # maps {node_id_A: set([rankX,rankY,...]), node_id_B:...}
        node_to_ranks = {}
        for rank in ranks:
            node_id = node_ids[mapping_dict[rank]]
            if not node_to_ranks.has_key(node_id):
                node_to_ranks[node_id] = set([])
            node_to_ranks[node_id].add(rank)

        # we only handle two cases for now:
        # 1. block regular
        # 2. round-robin regular
        # we do handle "remainder nodes" that might not be full
        delta = -1
        max_ranks_per_node = 0
        for node_id in node_to_ranks.keys():
            last_rank = -1
            if len(node_to_ranks[node_id]) > max_ranks_per_node:
                max_ranks_per_node = len(node_to_ranks[node_id])
            ranks = list(node_to_ranks[node_id])
            ranks.sort()
            for rank in ranks:
                if last_rank != -1:
                    if delta == -1:
                        if node_id == 0:
                            delta = rank - last_rank
                        else:
                            # irregular case detected such as {0:A,1:B,2:B}
                            mpd_print(1, "irregular case A detected")
                            return ''
                    elif (rank - last_rank) != delta:
                        # irregular such as {0:A,1:B,2:A,3:A,4:B}
                        mpd_print(1, "irregular case B detected")
                        return ''
                last_rank = rank

        # another check (case caught in ticket #905) for layouts like {0:A,1:A,2:B,3:B,4:B}
        if len(node_to_ranks.keys()) > 1:
            first_size = len(node_to_ranks[0])
            last_size  = len(node_to_ranks[len(node_to_ranks.keys())-1])
            if (last_size > first_size):
                mpd_print(1, "irregular case C1 detected")
                return ''
            in_remainder = False
            node_ids = node_to_ranks.keys()
            node_ids.sort()
            for node_id in node_ids:
                node_size = len(node_to_ranks[node_id])
                if not in_remainder:
                    if node_size == first_size:
                        pass # OK
                    elif node_size == last_size:
                        in_remainder = True
                    else:
                        mpd_print(1, "irregular case C2 detected")
                        return ''
                else: # in_remainder
                    if node_size != last_size:
                        mpd_print(1, "irregular case C3 detected")
                        return ''

        num_nodes = len(node_to_ranks.keys())
        if delta == 1:
            return '(vector,(%d,%d,%d))' % (0,num_nodes,max_ranks_per_node)
        else:
            # either we are round-robin-regular (delta > 1) or there is only one
            # process per node (delta == -1), either way results in the same
            # mapping spec
            return '(vector,(%d,%d,%d))' % (0,num_nodes,1)

    def handle_lhs_input(self,sock):
        msg = self.ring.lhsSock.recv_dict_msg()
        if not msg:    # lost lhs; don't worry
            mpd_print(0, "CLOSING self.ring.lhsSock ", self.ring.lhsSock )
            self.streamHandler.del_handler(self.ring.lhsSock)
            self.ring.lhsSock.close()
            self.ring.lhsSock = 0
            return
        if msg['cmd'] == 'mpdrun'  or  msg['cmd'] == 'spawn':
            if  msg.has_key('mpdid_mpdrun_start')  \
            and msg['mpdid_mpdrun_start'] == self.myId:
                if msg['first_loop']:
                    self.currRingSize = msg['ringsize']
                    self.currRingNCPUs = msg['ring_ncpus']
                if msg['nstarted'] == msg['nprocs']:
                    # we have started all processes in the job, tell the
                    # requester this and stop forwarding the mpdrun/spawn
                    # message around the loop
                    if msg['cmd'] == 'spawn':
                        self.spawnInProgress = 0
                    if self.conSock:
                        msgToSend = { 'cmd' : 'mpdrun_ack',
                                      'ringsize' : self.currRingSize,
                                      'ring_ncpus' : self.currRingNCPUs}
                        self.conSock.send_dict_msg(msgToSend)
                    # Tell all MPDs in the ring the final process mapping.  In
                    # turn, they will inform all of their child mpdmans.
                    # Only do this in the case of a regular mpdrun.  The spawn
                    # case it too complicated to handle this way right now.
                    if msg['cmd'] == 'mpdrun':
                        process_mapping_str = self.calculate_process_mapping(msg['process_mapping'])
                        msgToSend = { 'cmd' : 'process_mapping',
                                      'jobid' : msg['jobid'],
                                      'mpdid_mpdrun_start' : self.myId,
                                      'process_mapping' : process_mapping_str }
                        self.ring.rhsSock.send_dict_msg(msgToSend)
                    return
                if not msg['first_loop']  and  msg['nstarted_on_this_loop'] == 0:
                    if msg.has_key('jobid'):
                        if msg['cmd'] == 'mpdrun':
                            msgToSend = { 'cmd' : 'abortjob', 'src' : self.myId,
                                          'jobid' : msg['jobid'],
                                          'reason' : 'some_procs_not_started' }
                            self.ring.rhsSock.send_dict_msg(msgToSend)
                        else:  # spawn
                            msgToSend = { 'cmd' : 'startup_status', 'rc' : -1,
                                          'reason' : 'some_procs_not_started' }
                            jobid = msg['jobid']
                            manPid = msg['spawner_manpid']
                            manSock = self.activeJobs[jobid][manPid]['socktoman']
                            manSock.send_dict_msg(msgToSend)
                    if self.conSock:
                        msgToSend = { 'cmd' : 'job_failed',
                                      'reason' : 'some_procs_not_started',
                                      'remaining_hosts' : msg['hosts'] }
                        self.conSock.send_dict_msg(msgToSend)
                    return
                msg['first_loop'] = 0
                msg['nstarted_on_this_loop'] = 0
            self.do_mpdrun(msg)
        elif msg['cmd'] == 'process_mapping':
            # message transmission terminates once the message has made it all
            # the way around the loop once
            if msg['mpdid_mpdrun_start'] != self.myId:
                self.ring.rhsSock.send_dict_msg(msg) # forward it on around

            # send to all mpdman's for the jobid embedded in the msg
            jobid = msg['jobid']

            # there may be no entry for jobid in the activeJobs table if there
            # weren't any processes from that job actually launched on our host
            if self.activeJobs.has_key(jobid):
                for manPid in self.activeJobs[jobid].keys():
                    manSock = self.activeJobs[jobid][manPid]['socktoman']
                    manSock.send_dict_msg(msg)
        elif msg['cmd'] == 'mpdtrace_info':
            if msg['dest'] == self.myId:
                if self.conSock:
                    self.conSock.send_dict_msg(msg)
            else:
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'mpdtrace_trailer':
            if msg['dest'] == self.myId:
                if self.conSock:
                    self.conSock.send_dict_msg(msg)
            else:
                msgToSend = { 'cmd'     : 'mpdtrace_info',
                              'dest'    : msg['dest'],
                              'id'      : self.myId,
                              'ifhn'    : self.myIfhn,
                              'lhsport' : '%s' % (self.ring.lhsPort),
                              'lhsifhn' : '%s' % (self.ring.lhsIfhn),
                              'rhsport' : '%s' % (self.ring.rhsPort),
                              'rhsifhn' : '%s' % (self.ring.rhsIfhn) }
                self.ring.rhsSock.send_dict_msg(msgToSend)
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'mpdlistjobs_info':
            if msg['dest'] == self.myId:
                if self.conSock:
                    self.conSock.send_dict_msg(msg)
            else:
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'mpdlistjobs_trailer':
            if msg['dest'] == self.myId:
                if self.conSock:
                    self.conSock.send_dict_msg(msg)
            else:
                for jobid in self.activeJobs.keys():
                    for manPid in self.activeJobs[jobid]:
                        msgToSend = { 'cmd' : 'mpdlistjobs_info',
                                      'dest' : msg['dest'],
                                      'jobid' : jobid,
                                      'username' : self.activeJobs[jobid][manPid]['username'],
                                      'host' : self.myHost,
                                      'ifhn' : self.myIfhn,
                                      'clipid' : str(self.activeJobs[jobid][manPid]['clipid']),
                                      'sid' : str(manPid),  # may chg to actual sid later
                                      'pgm' : self.activeJobs[jobid][manPid]['pgm'],
                                      'rank' : self.activeJobs[jobid][manPid]['rank'] }
                        self.ring.rhsSock.send_dict_msg(msgToSend)
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'mpdallexit':
            if self.allExiting:   # already seen this once
                self.exiting = 1  # set flag to exit main loop
            self.allExiting = 1
            self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'mpdexit':
            if msg['dest'] == self.myId:
                msg['done'] = 1    # do this first
            if msg['src'] == self.myId:    # may be src and dest
                if self.conSock:
                    if msg['done']:
                        self.conSock.send_dict_msg({'cmd' : 'mpdexit_ack'})
                    else:
                        self.conSock.send_dict_msg({'cmd' : 'mpdexit_failed'})
            else:
                self.ring.rhsSock.send_dict_msg(msg)
            if msg['dest'] == self.myId:
                self.exiting = 1
                self.ring.lhsSock.send_dict_msg( { 'cmd'     : 'mpdexiting',
                                                   'rhsifhn' : self.ring.rhsIfhn,
                                                   'rhsport' : self.ring.rhsPort })
        elif msg['cmd'] == 'mpdringtest':
            if msg['src'] != self.myId:
                self.ring.rhsSock.send_dict_msg(msg)
            else:
                numLoops = msg['numloops'] - 1
                if numLoops > 0:
                    msg['numloops'] = numLoops
                    self.ring.rhsSock.send_dict_msg(msg)
                else:
                    if self.conSock:    # may have closed it if user did ^C at console
                        self.conSock.send_dict_msg({'cmd' : 'mpdringtest_done' })
        elif msg['cmd'] == 'mpdsigjob':
            forwarded = 0
            if msg['handled']  and  msg['src'] != self.myId:
                self.ring.rhsSock.send_dict_msg(msg)
                forwarded = 1
            handledHere = 0
            for jobid in self.activeJobs.keys():
                sjobid = jobid.split('  ')  # jobnum and mpdid
                if (sjobid[0] == msg['jobnum']  and  sjobid[1] == msg['mpdid'])  \
                or (msg['jobalias']  and  sjobid[2] == msg['jobalias']):
                    for manPid in self.activeJobs[jobid].keys():
                        if self.activeJobs[jobid][manPid]['username'] == msg['username']  \
                        or msg['username'] == 'root':
                            manSock = self.activeJobs[jobid][manPid]['socktoman']
                            manSock.send_dict_msg( { 'cmd' : 'signal_to_handle',
                                                     's_or_g' : msg['s_or_g'],
                                                     'sigtype' : msg['sigtype'] } )
                            handledHere = 1
            if handledHere:
                msg['handled'] = 1
            if not forwarded  and  msg['src'] != self.myId:
                self.ring.rhsSock.send_dict_msg(msg)
            if msg['src'] == self.myId:
                if self.conSock:
                    self.conSock.send_dict_msg( {'cmd' : 'mpdsigjob_ack',
                                                 'handled' : msg['handled'] } )
        elif msg['cmd'] == 'mpdkilljob':
            forwarded = 0
            if msg['handled'] and msg['src'] != self.myId:
                self.ring.rhsSock.send_dict_msg(msg)
                forwarded = 1
            handledHere = 0
            for jobid in self.activeJobs.keys():
                sjobid = jobid.split('  ')  # jobnum and mpdid
                if (sjobid[0] == msg['jobnum']  and  sjobid[1] == msg['mpdid'])  \
                or (msg['jobalias']  and  sjobid[2] == msg['jobalias']):
                    for manPid in self.activeJobs[jobid].keys():
                        if self.activeJobs[jobid][manPid]['username'] == msg['username']  \
                        or msg['username'] == 'root':
                            try:
                                pgrp = manPid * (-1)  # neg manPid -> group
                                os.kill(pgrp,signal.SIGKILL)
                                cliPid = self.activeJobs[jobid][manPid]['clipid']
                                pgrp = cliPid * (-1)  # neg Pid -> group
                                os.kill(pgrp,signal.SIGKILL)  # neg Pid -> group
                                handledHere = 1
                            except:
                                pass
                    # del self.activeJobs[jobid]  ## handled when child goes away
            if handledHere:
                msg['handled'] = 1
            if not forwarded  and  msg['src'] != self.myId:
                self.ring.rhsSock.send_dict_msg(msg)
            if msg['src'] == self.myId:
                if self.conSock:
                    self.conSock.send_dict_msg( {'cmd' : 'mpdkilljob_ack',
                                                 'handled' : msg['handled'] } )
        elif msg['cmd'] == 'abortjob':
            if msg['src'] != self.myId:
                self.ring.rhsSock.send_dict_msg(msg)
            for jobid in self.activeJobs.keys():
                if jobid == msg['jobid']:
                    for manPid in self.activeJobs[jobid].keys():
                        manSocket = self.activeJobs[jobid][manPid]['socktoman']
                        if manSocket:
                            manSocket.send_dict_msg(msg)
                            sleep(0.5)  # give man a brief chance to deal with this
                        try:
                            pgrp = manPid * (-1)  # neg manPid -> group
                            os.kill(pgrp,signal.SIGKILL)
                            cliPid = self.activeJobs[jobid][manPid]['clipid']
                            pgrp = cliPid * (-1)  # neg Pid -> group
                            os.kill(pgrp,signal.SIGKILL)  # neg Pid -> group
                        except:
                            pass
                    # del self.activeJobs[jobid]  ## handled when child goes away
        elif msg['cmd'] == 'pulse':
            self.ring.lhsSock.send_dict_msg({'cmd':'pulse_ack'})
        elif msg['cmd'] == 'verify_hosts_in_ring':
            while self.myIfhn in msg['host_list']  or  self.myHost in msg['host_list']:
                if self.myIfhn in msg['host_list']:
                    msg['host_list'].remove(self.myIfhn)
                elif self.myHost in msg['host_list']:
                    msg['host_list'].remove(self.myHost)
            if msg['dest'] == self.myId:
                msgToSend = { 'cmd' : 'verify_hosts_in_ring_response',
                              'host_list' : msg['host_list'] }
                self.conSock.send_dict_msg(msgToSend)
            else:
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'pmi_lookup_name':
            if msg['src'] == self.myId:
                if msg.has_key('port') and msg['port'] != 0:
                    msgToSend = msg
                    msgToSend['cmd'] = 'lookup_result'
                    msgToSend['info'] = 'ok'
                else:
                    msgToSend = { 'cmd' : 'lookup_result', 'info' : 'unknown_service',
                                  'port' : 0}
                jobid = msg['jobid']
                manPid = msg['manpid']
                manSock = self.activeJobs[jobid][manPid]['socktoman']
                manSock.send_dict_msg(msgToSend)
            else:
                if self.pmi_published_names.has_key(msg['service']):
                    msg['port'] = self.pmi_published_names[msg['service']]
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'pmi_unpublish_name':
            if msg['src'] == self.myId:
                if msg.has_key('done'):
                    msgToSend = msg
                    msgToSend['cmd'] = 'unpublish_result'
                    msgToSend['info'] = 'ok'
                else:
                    msgToSend = { 'cmd' : 'unpublish_result', 'info' : 'unknown_service' }
                jobid = msg['jobid']
                manPid = msg['manpid']
                manSock = self.activeJobs[jobid][manPid]['socktoman']
                manSock.send_dict_msg(msgToSend)
            else:
                if self.pmi_published_names.has_key(msg['service']):
                    del self.pmi_published_names[msg['service']]
                    msg['done'] = 1
                self.ring.rhsSock.send_dict_msg(msg)
        elif msg['cmd'] == 'client_info':
            if msg['spawner_manpid']  and  msg['rank'] == 0:
                if msg['spawner_mpd'] == self.myId:
                    jobid = msg['jobid']
                    spawnerManPid = msg['spawner_manpid']
                    if self.activeJobs[jobid].has_key(spawnerManPid):
                        spawnerManSock = self.activeJobs[jobid][spawnerManPid]['socktoman']
                        msgToSend = { 'cmd' : 'spawn_done_by_mpd', 'rc' : 0, 'reason' : '' }
                        spawnerManSock.send_dict_msg(msgToSend)
                else:
                    self.ring.rhsSock.send_dict_msg(msg)
        else:
            mpd_print(1, 'unrecognized cmd from lhs: %s' % (msg) )

    def handle_rhs_input(self,sock):
        if self.allExiting:
            return
        msg = sock.recv_dict_msg()
        if not msg:    # lost rhs; re-knit the ring
            if sock == self.ring.rhsSock:
                needToReenter = 1
            else:
                needToReenter = 0
            if sock == self.ring.rhsSock  and self.ring.lhsSock:
                self.streamHandler.del_handler(self.ring.lhsSock)
                self.ring.lhsSock.close()
                self.ring.lhsSock = 0
            if sock == self.ring.rhsSock  and self.ring.rhsSock:
                self.streamHandler.del_handler(self.ring.rhsSock)
                self.ring.rhsSock.close()
                self.ring.rhsSock = 0
            if needToReenter:
                mpd_print(1,'lost rhs; re-entering ring')
                rc = self.ring.reenter_ring(lhsHandler=self.handle_lhs_input,
                                            rhsHandler=self.handle_rhs_input,
                                            ntries=16)
                if rc == 0:
                    mpd_print(1,'back in ring')
		else:
                    mpd_print(1,'failed to reenter ring')
                    sys.exit(-1)
            return
        if msg['cmd'] == 'pulse_ack':
            self.pulse_cntr = 0
        elif msg['cmd'] == 'mpdexiting':    # for mpdexit
            if self.ring.rhsSock:
                self.streamHandler.del_handler(self.ring.rhsSock)
                self.ring.rhsSock.close()
                self.ring.rhsSock = 0
            # connect to new rhs
            self.ring.rhsIfhn = msg['rhsifhn']
            self.ring.rhsPort = int(msg['rhsport'])
            if self.ring.rhsIfhn == self.myIfhn  and  self.ring.rhsPort == self.parmdb['MPD_LISTEN_PORT']:
                rv = self.ring.connect_rhs(rhsHost=self.ring.rhsIfhn,
                                           rhsPort=self.ring.rhsPort,
                                           rhsHandler=self.handle_rhs_input,
                                           numTries=3)
                if rv[0] <=  0:  # connect did not succeed; may try again
                    mpd_print(1,"rhs connect failed")
                    sys.exit(-1)
                return
            self.ring.rhsSock = MPDSock(name='rhs')
            self.ring.rhsSock.connect((self.ring.rhsIfhn,self.ring.rhsPort))
            self.pulse_cntr = 0
            if not self.ring.rhsSock:
                mpd_print(1,'handle_rhs_input failed to obtain rhs socket')
                return
            msgToSend = { 'cmd' : 'request_to_enter_as_lhs', 'host' : self.myHost,
                          'ifhn' : self.myIfhn, 'port' : self.parmdb['MPD_LISTEN_PORT'] }
            self.ring.rhsSock.send_dict_msg(msgToSend)
            msg = self.ring.rhsSock.recv_dict_msg()
            if (not msg) or  \
               (not msg.has_key('cmd')) or  \
               (msg['cmd'] != 'challenge') or (not msg.has_key('randnum')):
                mpd_print(1, 'failed to recv challenge from rhs; msg=:%s:' % (msg) )
            response = md5new(''.join([self.parmdb['MPD_SECRETWORD'],
                                       msg['randnum']])).digest()
            msgToSend = { 'cmd' : 'challenge_response',
                          'response' : response,
                          'host' : self.myHost, 'ifhn' : self.myIfhn,
                          'port' : self.parmdb['MPD_LISTEN_PORT'] }
            self.ring.rhsSock.send_dict_msg(msgToSend)
            msg = self.ring.rhsSock.recv_dict_msg()
            if (not msg) or  \
               (not msg.has_key('cmd')) or  \
               (msg['cmd'] != 'OK_to_enter_as_lhs'):
                mpd_print(1, 'NOT OK to enter ring; msg=:%s:' % (msg) )
            self.streamHandler.set_handler(self.ring.rhsSock,self.handle_rhs_input)
        else:
            mpd_print(1, 'unexpected from rhs; msg=:%s:' % (msg) )
        return

    def do_mpdrun(self,msg):
        if self.parmdb['MPD_LOGFILE_TRUNC_SZ'] >= 0:
            try:
                logSize = os.stat(self.logFilename)[stat.ST_SIZE]
                if logSize > self.parmdb['MPD_LOGFILE_TRUNC_SZ']:
                    self.logFile.truncate(self.parmdb['MPD_LOGFILE_TRUNC_SZ'])
            except:
                pass

        if msg.has_key('jobid'):
            jobid = msg['jobid']
        else:
            jobid = str(self.nextJobInt) + '  ' + self.myId + '  ' + msg['jobalias']
            self.nextJobInt += 1
            msg['jobid'] = jobid
        if msg['nstarted'] >= msg['nprocs']:
            self.ring.rhsSock.send_dict_msg(msg)  # forward it on around
            return
        hosts = msg['hosts']
        if self.myIfhn in hosts.values():
            hostsKeys = hosts.keys()
            hostsKeys.sort()
            for ranks in hostsKeys:
                if hosts[ranks] == self.myIfhn:
                    (lorank,hirank) = ranks
                    for rank in range(lorank,hirank+1):
                        self.run_one_cli(rank,msg)
                        # we use myHost under the assumption that there is only
                        # one mpd per user on a given host.  The ifhn only
                        # affects how the MPDs communicate with each other, not
                        # which host they are on
                        msg['process_mapping'][rank] = self.myHost
                        msg['nstarted'] += 1
                        msg['nstarted_on_this_loop'] += 1
                    del msg['hosts'][ranks]
        elif '_any_from_pool_' in hosts.values():
            hostsKeys = hosts.keys()
            hostsKeys.sort()
            for ranks in hostsKeys:
                if hosts[ranks] == '_any_from_pool_':
                    (lorank,hirank) = ranks
                    hostSpecPool = msg['host_spec_pool']
                    if self.myIfhn in hostSpecPool  or  self.myHost in hostSpecPool:
                        self.run_one_cli(lorank,msg)
                        msg['process_mapping'][lorank] = self.myHost
                        msg['nstarted'] += 1
                        msg['nstarted_on_this_loop'] += 1
                        del msg['hosts'][ranks]
                        if lorank < hirank:
                            msg['hosts'][(lorank+1,hirank)] = '_any_from_pool_'
                    break
        elif '_any_' in hosts.values():
            done = 0
            while not done:
                hostsKeys = hosts.keys()
                hostsKeys.sort()
                for ranks in hostsKeys:
                    if hosts[ranks] == '_any_':
                        (lorank,hirank) = ranks
                        self.run_one_cli(lorank,msg)
                        msg['process_mapping'][lorank] = self.myHost
                        msg['nstarted'] += 1
                        msg['nstarted_on_this_loop'] += 1
                        del msg['hosts'][ranks]
                        if lorank < hirank:
                            msg['hosts'][(lorank+1,hirank)] = '_any_'
                        # self.activeJobs maps:
                        # { jobid => { mpdman_pid => {...} } }
                        procsHereForJob = len(self.activeJobs[jobid].keys())
                        if procsHereForJob >= self.parmdb['MPD_NCPUS']:
                            break  # out of for loop
                # if no more to start via any or enough started here
                if '_any_' not in hosts.values() \
                or procsHereForJob >= self.parmdb['MPD_NCPUS']:
                    done = 1
        if msg['first_loop']:
            msg['ringsize'] += 1
            msg['ring_ncpus'] += self.parmdb['MPD_NCPUS']
        self.ring.rhsSock.send_dict_msg(msg)  # forward it on around
    def run_one_cli(self,currRank,msg):
        users = msg['users']
        for ranks in users.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                username = users[ranks]
                break
        execs = msg['execs']
        for ranks in execs.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgm = execs[ranks]
                break
        paths = msg['paths']
        for ranks in paths.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pathForExec = paths[ranks]
                break
        args = msg['args']
        for ranks in args.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgmArgs = dumps(args[ranks])
                break
        envvars = msg['envvars']
        for ranks in envvars.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgmEnvVars = dumps(envvars[ranks])
                break
        limits = msg['limits']
        for ranks in limits.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgmLimits = dumps(limits[ranks])
                break
        cwds = msg['cwds']
        for ranks in cwds.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                cwd = cwds[ranks]
                break
        umasks = msg['umasks']
        for ranks in umasks.keys():
            (lo,hi) = ranks
            if currRank >= lo  and  currRank <= hi:
                pgmUmask = umasks[ranks]
                break
        man_env = {}
        if msg['ifhns'].has_key(currRank):
            man_env['MPICH_INTERFACE_HOSTNAME'] = msg['ifhns'][currRank]
        else:
            man_env['MPICH_INTERFACE_HOSTNAME'] = self.myIfhn
        man_env.update(os.environ)    # may only want to mov non-MPD_ stuff
        man_env['MPDMAN_MYHOST'] = self.myHost
        man_env['MPDMAN_MYIFHN'] = self.myIfhn
        man_env['MPDMAN_JOBID'] = msg['jobid']
        man_env['MPDMAN_CLI_PGM'] = pgm
        man_env['MPDMAN_CLI_PATH'] = pathForExec
        man_env['MPDMAN_PGM_ARGS'] = pgmArgs
        man_env['MPDMAN_PGM_ENVVARS'] = pgmEnvVars
        man_env['MPDMAN_PGM_LIMITS'] = pgmLimits
        man_env['MPDMAN_CWD'] = cwd
        man_env['MPDMAN_UMASK'] = pgmUmask
        man_env['MPDMAN_SPAWNED'] = str(msg['spawned'])
        if msg.has_key('spawner_manpid'):
            man_env['MPDMAN_SPAWNER_MANPID'] = str(msg['spawner_manpid'])
        else:
            man_env['MPDMAN_SPAWNER_MANPID'] = '0'
        if msg.has_key('spawner_mpd'):
            man_env['MPDMAN_SPAWNER_MPD'] = msg['spawner_mpd']
        else:
            man_env['MPDMAN_SPAWNER_MPD'] = ''
        man_env['MPDMAN_NPROCS'] = str(msg['nprocs'])
        man_env['MPDMAN_MPD_LISTEN_PORT'] = str(self.parmdb['MPD_LISTEN_PORT'])
        man_env['MPDMAN_MPD_CONF_SECRETWORD'] = self.parmdb['MPD_SECRETWORD']
        man_env['MPDMAN_CONHOST'] = msg['conhost']
        man_env['MPDMAN_CONIFHN'] = msg['conifhn']
        man_env['MPDMAN_CONPORT'] = str(msg['conport'])
        man_env['MPDMAN_RANK'] = str(currRank)
        man_env['MPDMAN_POS_IN_RING'] = str(msg['nstarted'])
        man_env['MPDMAN_STDIN_DEST'] = msg['stdin_dest']
        man_env['MPDMAN_TOTALVIEW'] = str(msg['totalview'])
        man_env['MPDMAN_GDB'] = str(msg['gdb'])
        man_env['MPDMAN_GDBA'] = str(msg['gdba'])  # for attach to running pgm
        fullDirName = os.path.abspath(os.path.split(sys.argv[0])[0])  # normalize
        man_env['MPDMAN_FULLPATHDIR'] = fullDirName    # used to find gdbdrv
        man_env['MPDMAN_SINGINIT_PID']  = str(msg['singinitpid'])
        man_env['MPDMAN_SINGINIT_PORT'] = str(msg['singinitport'])
        man_env['MPDMAN_LINE_LABELS_FMT'] = msg['line_labels']
        if msg.has_key('rship'):
            man_env['MPDMAN_RSHIP'] = msg['rship']
            man_env['MPDMAN_MSHIP_HOST'] = msg['mship_host']
            man_env['MPDMAN_MSHIP_PORT'] = str(msg['mship_port'])
        if msg.has_key('doing_bnr'):
            man_env['MPDMAN_DOING_BNR'] = '1'
        else:
            man_env['MPDMAN_DOING_BNR'] = '0'
        if msg['nstarted'] == 0:
            manKVSTemplate = '%s_%s_%d' % \
                             (self.myHost,self.parmdb['MPD_LISTEN_PORT'],self.kvs_cntr)
            manKVSTemplate = sub('\.','_',manKVSTemplate)  # chg magpie.cs to magpie_cs
            manKVSTemplate = sub('\-','_',manKVSTemplate)  # chg node-0 to node_0
            self.kvs_cntr += 1
            msg['kvs_template'] = manKVSTemplate
        man_env['MPDMAN_KVS_TEMPLATE'] = msg['kvs_template']
	msg['username'] = username
        if hasattr(os,'fork'):
            (manPid,toManSock) = self.launch_mpdman_via_fork(msg,man_env)
            if not manPid:
                print '**** mpd: launch_client_via_fork_exec failed; exiting'
        elif subprocess_module_available:
            (manPid,toManSock) = self.launch_mpdman_via_subprocess(msg,man_env)
        else:
            mpd_print(1,'neither fork nor subprocess is available')
            sys.exit(-1)
        jobid = msg['jobid']
        if not self.activeJobs.has_key(jobid):
            self.activeJobs[jobid] = {}
        self.activeJobs[jobid][manPid] = { 'pgm' : pgm, 'rank' : currRank,
                                           'username' : username,
                                           'clipid' : -1,    # until report by man
                                           'socktoman' : toManSock }
        mpd_print(mpd_dbg_level,"Created entry for %s %d" % (str(jobid),manPid) )
    def launch_mpdman_via_fork(self,msg,man_env):
        man_env['MPDMAN_HOW_LAUNCHED'] = 'FORK'
        currRank = int(man_env['MPDMAN_RANK'])
        manListenSock = MPDListenSock('',0,name='tempsock')
        manListenPort = manListenSock.getsockname()[1]
        if msg['nstarted'] == 0:
            manEntryIfhn = ''
            manEntryPort = 0
            msg['pos0_host'] = self.myHost
            msg['pos0_ifhn'] = self.myIfhn
            msg['pos0_port'] = str(manListenPort)
            man_env['MPDMAN_POS0_IFHN'] = self.myIfhn
            man_env['MPDMAN_POS0_PORT'] = str(manListenPort)
        else:
            manEntryIfhn = msg['entry_ifhn']
            manEntryPort = msg['entry_port']
            man_env['MPDMAN_POS0_IFHN'] = msg['pos0_ifhn']
            man_env['MPDMAN_POS0_PORT'] = msg['pos0_port']
        man_env['MPDMAN_LHS_IFHN']  = manEntryIfhn
        man_env['MPDMAN_LHS_PORT'] = str(manEntryPort)
        man_env['MPDMAN_MY_LISTEN_FD'] = str(manListenSock.fileno())
        man_env['MPDMAN_MY_LISTEN_PORT'] = str(manListenPort)
        mpd_print(mpd_dbg_level,"About to get sockpair for mpdman")
        (toManSock,toMpdSock) = mpd_sockpair()
        mpd_print(mpd_dbg_level,"Found sockpair (%d,%d) for mpdman" % \
                                (toManSock.fileno(), toMpdSock.fileno()) )
        toManSock.name = 'to_man'
        toMpdSock.name = 'to_mpd'  ## to be used by mpdman below
        man_env['MPDMAN_TO_MPD_FD'] = str(toMpdSock.fileno())
        self.streamHandler.set_handler(toManSock,self.handle_man_input)
        msg['entry_host'] = self.myHost
        msg['entry_ifhn'] = self.myIfhn
        msg['entry_port'] = manListenPort
        maxTries = 6
        numTries = 0
        while numTries < maxTries:
            try:
                manPid = os.fork()
                errinfo = 0
            except OSError, errinfo:
                pass  ## could check for errinfo.errno == 35 (resource unavailable)
            if errinfo:
                sleep(1)
                numTries += 1
            else:
                break
        if numTries >= maxTries:
            return (0,0)
        if manPid == 0:
            self.conListenSock = 0    # don't want to clean up console if I am manager
            self.myId = '%s_man_%d' % (self.myHost,self.myPid)
            mpd_set_my_id(self.myId)
            self.streamHandler.close_all_active_streams()
            os.setpgrp()
            os.environ = man_env
            if hasattr(os,'getuid')  and  os.getuid() == 0  and  pwd_module_available:
		username = msg['username']
                try:
                    pwent = pwd.getpwnam(username)
                except:
                    mpd_print(1,'invalid username :%s: on %s' % (username,self.myHost))
                    msgToSend = {'cmd' : 'job_failed', 'reason' : 'invalid_username',
                                 'username' : username, 'host' : self.myHost }
                    self.conSock.send_dict_msg(msgToSend)
                    return
                uid = pwent[2]
                gid = pwent[3]
                os.setgroups(mpd_get_groups_for_username(username))
                os.setregid(gid,gid)
                try:
                    os.setreuid(uid,uid)
                except OSError, errmsg1:
                    try:
                        os.setuid(uid)
                    except OSError, errmsg2:
                        mpd_print(1,"unable to perform setreuid or setuid")
                        sys.exit(-1)
            import atexit    # need to use full name of _exithandlers
            atexit._exithandlers = []    # un-register handlers in atexit module
            # import profile
            # print 'profiling the manager'
            # profile.run('mpdman()')
            mpdman = MPDMan()
            mpdman.run()
            sys.exit(0)  # do NOT do cleanup (eliminated atexit handlers above)
        # After the fork, if we're the parent, close the other side of the
        # mpdpair sockets, as well as the listener socket
        manListenSock.close()
        toMpdSock.close()
        return (manPid,toManSock)
    def launch_mpdman_via_subprocess(self,msg,man_env):
        man_env['MPDMAN_HOW_LAUNCHED'] = 'SUBPROCESS'
        currRank = int(man_env['MPDMAN_RANK'])
        if msg['nstarted'] == 0:
            manEntryIfhn = ''
            manEntryPort = 0
        else:
            manEntryIfhn = msg['entry_ifhn']
            manEntryPort = msg['entry_port']
            man_env['MPDMAN_POS0_IFHN'] = msg['pos0_ifhn']
            man_env['MPDMAN_POS0_PORT'] = msg['pos0_port']
        man_env['MPDMAN_LHS_IFHN']  = manEntryIfhn
        man_env['MPDMAN_LHS_PORT'] = str(manEntryPort)
        tempListenSock = MPDListenSock()
        man_env['MPDMAN_MPD_PORT'] = str(tempListenSock.getsockname()[1])
        # python_executable = '\Python24\python.exe'
        python_executable = 'python2.4'
        fullDirName = man_env['MPDMAN_FULLPATHDIR']
        manCmd = os.path.join(fullDirName,'mpdman.py')
        runner = subprocess.Popen([python_executable,'-u',manCmd],  # only one 'python' arg
                                  bufsize=0,
                                  env=man_env,
                                  close_fds=False)
                                  ### stdin=subprocess.PIPE,stdout=subprocess.PIPE,
                                  ### stderr=subprocess.PIPE)
        manPid = runner.pid
        oldTimeout = socket.getdefaulttimeout()
        socket.setdefaulttimeout(8)
        try:
            (toManSock,toManAddr) = tempListenSock.accept()
        except Exception, errmsg:
            toManSock = 0
        socket.setdefaulttimeout(oldTimeout)
        tempListenSock.close()
        if not toManSock:
            mpd_print(1,'failed to recv msg from launched man')
            return (0,0)
        msgFromMan = toManSock.recv_dict_msg()
        if not msgFromMan  or  not msgFromMan.has_key('man_listen_port'):
            toManSock.close()
            mpd_print(1,'invalid msg from launched man')
            return (0,0)
        manListenPort = msgFromMan['man_listen_port']
        if currRank == 0:
            msg['pos0_host'] = self.myHost
            msg['pos0_ifhn'] = self.myIfhn
            msg['pos0_port'] = str(manListenPort)
        msg['entry_host'] = self.myHost
        msg['entry_ifhn'] = self.myIfhn
        msg['entry_port'] = manListenPort
        return (manPid,toManSock)

# code for testing
if __name__ == '__main__':
    mpd = MPD()
    mpd.run()
