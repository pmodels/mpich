#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

# workaround to suppress deprecated module warnings in python2.6
# see https://trac.mcs.anl.gov/projects/mpich2/ticket/362 for tracking
import warnings
warnings.filterwarnings('ignore', '.*the md5 module is deprecated.*', DeprecationWarning)
warnings.filterwarnings('ignore', '.*the popen2 module is deprecated.*', DeprecationWarning)

import sys, os, signal, popen2, socket, select, inspect

from  cPickle   import  dumps, loads
from  types     import  TupleType
from  traceback import  extract_tb, extract_stack, format_list
from  re        import  sub, split
from  errno     import  EINTR, ECONNRESET, EISCONN, ECONNREFUSED, EPIPE
from  md5       import  new as md5new
from  time      import  sleep
from  random    import  randrange, random

try:
    import pwd
    pwd_module_available = 1
except:
    pwd_module_available = 0
try:
    import grp
    grp_module_available = 1
except:
    grp_module_available = 0
try:
    import  syslog
    syslog_module_available = 1
except:
    syslog_module_available = 0
try:
    import subprocess
    subprocess_module_available = 1
except:
    subprocess_module_available = 0


# some global vars for some utilities
global mpd_my_id, mpd_signum, mpd_my_hostname, mpd_procedures_to_trace
global mpd_cli_app  # for debug during mpich nightly tests
global mpd_tmpdir
mpd_cli_app = ''
mpd_my_id = ''
mpd_procedures_to_trace = []
mpd_my_hostname = ''
# mpd_signum can be set by mpd_handle_signal to indicate which signal was recently caught;
# this can be useful below to pop out of loops that ordinarily continue after sigs
# NOTE: mpd_handle_signal must be called by the user, e.g. in his own signal handler
mpd_signum = 0
mpd_zc = 0
if os.environ.has_key('MPD_TMPDIR'):
    mpd_tmpdir = os.environ['MPD_TMPDIR']
else:
    mpd_tmpdir = '/tmp'

# For easier debugging, we provide this variable that is used in the
# mpd_print calls.  This makes it a little easier to debug problems involving
# communication with other processes, such as handling EINTR from signals.
global mpd_dbg_level
mpd_dbg_level = 0

def mpd_set_dbg_level(flag):
    global mpd_dbg_level
    mpd_dbg_level = flag

def mpd_set_my_id(myid=''):
    global mpd_my_id
    mpd_my_id = myid

def mpd_set_tmpdir(tmpdir):
    global mpd_tmpdir
    mpd_tmpdir = tmpdir

def mpd_get_my_id():
    global mpd_my_id
    return(mpd_my_id)

def mpd_set_cli_app(app):    # for debug during mpich nightly tests
    global mpd_cli_app
    mpd_cli_app = app

def mpd_handle_signal(signum,frame):
    global mpd_signum
    mpd_signum = signum

def mpd_print(*args):
    global mpd_my_id
    if not args[0]:
        return
    stack = extract_stack()
    callingProc = stack[-2][2]
    callingLine = stack[-2][1]
    printLine = '%s (%s %d): ' % (mpd_my_id,callingProc,callingLine)
    for arg in args[1:]:
        printLine = printLine + str(arg)
    # We've seen an EINTR on the flush here
    while 1:
        try:
            print printLine
            break
        except os.error, errinfo:
            if errinfo[0] != EINTR:
                raise os.error, errinfo
    # end of while
    while 1:
        try:
            sys.stdout.flush()
            break
        except os.error, errinfo:
	    if errinfo[0] != EINTR:
                raise os.error, errinfo
    # end of while
    if syslog_module_available:
        syslog.syslog(syslog.LOG_INFO,printLine)

def mpd_print_tb(*args):
    global mpd_my_id
    if not args[0]:
        return
    stack = extract_stack()
    callingProc = stack[-2][2]
    callingLine = stack[-2][1]
    stack = extract_stack()
    stack.reverse()
    stack = stack[1:]
    printLine = '%s (%s %d):' % (mpd_my_id,callingProc,callingLine)
    for arg in args[1:]:
        printLine = printLine + str(arg)
    printLine += '\n  mpdtb:\n'
    for line in format_list(stack):
        line = sub(r'\n.*','',line)
        splitLine = split(',',line)
        splitLine[0] = sub('  File "(.*)"',lambda mo: mo.group(1),splitLine[0])
        splitLine[1] = sub(' line ','',splitLine[1])
        splitLine[2] = sub(' in ','',splitLine[2])
        printLine = printLine + '    %s,  %s,  %s\n' % tuple(splitLine)
    if mpd_cli_app:    # debug mpich apps in nightly tests
        printLine += '    mpd_cli_app=%s\n' % (mpd_cli_app)
        printLine += '    cwd=%s' % (os.getcwd())
    print printLine
    sys.stdout.flush()
    if syslog_module_available:
        syslog.syslog(syslog.LOG_INFO,printLine)

def mpd_uncaught_except_tb(arg1,arg2,arg3):
    global mpd_my_id
    global mpd_cli_id
    if mpd_my_id:
        errstr = '%s: ' % (mpd_my_id)
    else:
        errstr = ''
    errstr += 'mpd_uncaught_except_tb handling:\n'
    errstr += '  %s: %s\n' % (arg1,arg2)
    tb = extract_tb(arg3)
    tb.reverse()
    for tup in tb:
        # errstr += '    file %s  line# %i  procedure %s\n        %s\n' % (tup)
        errstr += '    %s  %i  %s\n        %s\n' % (tup)
    if mpd_cli_app:    # debug mpich apps in nightly tests
        errstr += '    mpd_cli_app=%s\n' % (mpd_cli_app)
        errstr += '    cwd=%s' % (os.getcwd())
    print errstr,
    if syslog_module_available:
        syslog.syslog(syslog.LOG_ERR, errstr)

def mpd_set_procedures_to_trace(procs):
    global mpd_procedures_to_trace
    mpd_procedures_to_trace = procs

def mpd_trace_calls(frame,event,args):
    global mpd_my_id, mpd_procedures_to_trace
    if frame.f_code.co_name not in mpd_procedures_to_trace:
        return None
    args_info = apply(inspect.formatargvalues,inspect.getargvalues(frame))
    # Be VERY careful here; under AIX, it looked like EINTR is 
    # possible within print (!).  
    while (1):
        try:
            print '%s: ENTER %s in %s at line %d; ARGS=%s' % \
          (mpd_my_id,frame.f_code.co_name,frame.f_code.co_filename,frame.f_lineno,args_info)
            break
        except os.error, errinfo:
            if errinfo[0] != EINTR:
                raise os.error, errinfo
    # end of while
    return mpd_trace_returns

def mpd_trace_returns(frame,event,args):
    global mpd_my_id
    if event == 'return':
        # Be VERY careful here; under AIX, it looked like EINTR is 
        # possible within print (!).  
        while (1):
            try:
                print '%s: EXIT %s at line %d ' % (mpd_my_id,frame.f_code.co_name,frame.f_lineno)
                break
            except os.error, errinfo:
                if errinfo[0] != EINTR:
                    raise os.error, errinfo
        # end of while
        return None
    else:
        return mpd_trace_returns

def mpd_sockpair():
    sock1 = MPDSock()
    rc = sock1.sock.bind(('',0))
    rc = sock1.sock.listen(5)
    port1 = sock1.sock.getsockname()[1]
    sock2 = MPDSock()
    #
    # We have encountered situations where the connection fails; as this is
    # a connection to this process, we retry a few times in that case 
    # (seen on AIX)
    #
    try:
        connAttempts = 0
        while (1):
            try:
                rc = sock2.sock.connect(('localhost',port1))
                break
            except socket.error, errinfo:
                # In some cases, connect will return EINTR and then on the
                # next iteration, returns EISCONN.
                if errinfo[0] == EISCONN:
                    break
                if errinfo[0] == ECONNREFUSED and connAttempts < 10:
                    mpd_print(mpd_dbg_level,"Retrying on connection refused")
                    connAttempts += 1
                    sleep(random())
                elif errinfo[0] != EINTR:
                    mpd_print(1,"connect %d %s" % (errinfo[0],errinfo[1]))
                    raise socket.error, errinfo
	# End of the while
    except socket.error, errinfo:
        # we have seen at least one machine that needs it this way
        # We've seen a failure here; it could be EINPROGRESS, EALREADY, 
        # or EADDRINUSE.  In that case, we may need to do something else
	mpd_print(1,"connect error with %d %s" % (errinfo[0],errinfo[1]))
        # Should this only attempt on ECONNREFUSED, ENETUNREACH, EADDRNOTAVAIL
        # FIXME: Does this need a try/except?
        while 1:
            try:  
                rc = sock2.sock.connect(('',port1))
                break
            except socket.error, errinfo:
                if errinfo[0] == EISCONN:
                    break
                elif errinfo[0] != EINTR:
                    mpd_print(1,"connect %d %s" % (errinfo[0],errinfo[1]))
                    raise socket.error, errinfo
        # end of while
    # Accept can fail on EINTR, so we handle that here
    while (1):
        try:
            (sock3,addr) = sock1.sock.accept()
            break
        except socket.error, errinfo:
            if errinfo[0] != EINTR:
                mpd_print(1,"connect %d %s" % (errinfo[0],errinfo[1]))
                raise socket.error, errinfo
    # end of while
    sock3 = MPDSock(sock=sock3)
    sock1.close()
    return (sock2,sock3)

def mpd_which(execName,user_path=None):
    if not user_path:
        if os.environ.has_key('PATH'):
            user_path = os.environ['PATH']
        else:
            return ''
    for d in user_path.split(os.pathsep):
        fpn = os.path.join(d,execName)
        if os.path.isdir(fpn):  # follows symlinks; dirs can have execute permission
            continue
        if os.access(fpn,os.X_OK):    # NOTE access works based on real uid (not euid)
            return fpn
    return ''

def mpd_check_python_version():
    # version_info: (major,minor,micro,releaselevel,serial)
    if (sys.version_info[0] < 2)  or  \
       (sys.version_info[0] == 2 and sys.version_info[1] < 2):
        return sys.version_info
    return 0

def mpd_version():
    return (1,0,1,'July, 2006 release')  # major, minor, micro, special

def mpd_get_my_username():
    if pwd_module_available:
        username = pwd.getpwuid(os.getuid())[0]    # favor this over env
    elif os.environ.has_key('USER'):
        username = os.environ['USER']
    elif os.environ.has_key('USERNAME'):
        username = os.environ['USERNAME']
    else:
        username = 'unknown_username'
    return username

def mpd_get_ranks_in_binary_tree(myRank,nprocs):
    if myRank == 0:
        parent = -1;
    else:   
        parent = (myRank - 1) / 2; 
    lchild = (myRank * 2) + 1
    if lchild > (nprocs - 1):
        lchild = -1;
    rchild = (myRank * 2) + 2
    if rchild > (nprocs - 1):
        rchild = -1;
    return (parent,lchild,rchild)

def mpd_same_ips(host1,host2):    # hosts may be names or IPs
    try:
        ips1 = socket.gethostbyname_ex(host1)[2]    # may fail if invalid host
        ips2 = socket.gethostbyname_ex(host2)[2]    # may fail if invalid host
    except:
        return 0
    for ip1 in ips1:
        for ip2 in ips2:
            if ip1 == ip2:
                return 1
    return 0

def mpd_read_nbytes(fd,nbytes):
    global mpd_signum
    rv = 0
    while 1:
        try:
            mpd_signum = 0
            rv = os.read(fd,nbytes)
            break
        except os.error, errinfo:
            if errinfo[0] == EINTR:
                if mpd_signum == signal.SIGINT  or  mpd_signum == signal.SIGALRM:
                    break
                else:
                    continue
            elif errinfo[0] == ECONNRESET:   # connection reset (treat as eof)
                break
            else:
                mpd_print(1, 'read error: %s' % os.strerror(errinfo[0]))
                break
        except KeyboardInterrupt, errinfo:
            break
        except Exception, errinfo:
            mpd_print(1, 'other error after read %s :%s:' % ( errinfo.__class__, errinfo) )
            break
    return rv

def mpd_get_groups_for_username(username):
    if pwd_module_available  and  grp_module_available:
        userGroups = [pwd.getpwnam(username)[3]]  # default group for the user
        allGroups = grp.getgrall();
        for group in allGroups:
            if username in group[3]  and  group[2] not in userGroups:
                userGroups.append(group[2])
    else:
        userGroups = []
    return userGroups


class MPDSock(object):
    def __init__(self,family=socket.AF_INET,socktype=socket.SOCK_STREAM,proto=0,
                 sock=None,name=''):
        if sock:
            self.sock = sock
        else:
            self.sock = socket.socket(family=family,type=socktype,proto=proto)
        self.name = name
        self.type = socktype
        self.family = family
        ## used this when inherited from socket.socket (only works with py 2.3+)
        ## socket.socket.__init__(self,family=family,type=socktype,proto=proto,_sock=sock)
    def close(self):
        self.sock.close()
    def sendall(self,data):
        self.sock.sendall(data)
    def getsockname(self):
        return self.sock.getsockname()
    def fileno(self):
        return self.sock.fileno()

    def connect(self,*args):
        # We handle EINTR in this method, unless it appears that a
        # SIGINT or SIGALRM are delivered.  In that case, we do not
        # complete the connection (FIXME: make sure that all uses of this
        # do the right thing in that case).
        while 1:
            try:
                mpd_signum = 0
                self.sock.connect(*args)
                break
            except socket.error, errinfo:
                if errinfo[0] == EINTR:   # sigchld, sigint, etc.
                    if mpd_signum == signal.SIGINT  or  mpd_signum == signal.SIGALRM:
                        break
                    else:
                        continue
                else:
                    raise socket.error, errinfo
        # end of while

    def accept(self,name='accepter'):
        global mpd_signum
        newsock = 0
        newaddr = 0
        while 1:
            try:
                mpd_signum = 0
                (newsock,newaddr) = self.sock.accept()
                break
            except socket.error, errinfo:
                if errinfo[0] == EINTR:   # sigchld, sigint, etc.
                    if mpd_signum == signal.SIGINT  or  mpd_signum == signal.SIGALRM:
                        break
                    else:
                        continue
                elif errinfo[0] == ECONNRESET:   # connection reset (treat as eof)
                    break
                else:
                    print '%s: accept error: %s' % (mpd_my_id,os.strerror(errinfo[0]))
                    break
            except Exception, errinfo:
                print '%s: failure doing accept : %s : %s' % \
                      (mpd_my_id,errinfo.__class__,errinfo)
                break
        if newsock:
            newsock = MPDSock(sock=newsock,name=name)    # turn new socket into an MPDSock
        return (newsock,newaddr)
    def recv(self,nbytes):
        global mpd_signum
        data = 0
        while 1:
            try:
                mpd_signum = 0
                data = self.sock.recv(nbytes)
                break
            except socket.error, errinfo:
                if errinfo[0] == EINTR:   # sigchld, sigint, etc.
                    if mpd_signum == signal.SIGINT  or  mpd_signum == signal.SIGALRM:
                        break
                    else:
                        continue
                elif errinfo[0] == ECONNRESET:   # connection reset (treat as eof)
                    break
                else:
                    print '%s: recv error: %s' % (mpd_my_id,os.strerror(errinfo[0]))
                    break
            except Exception, errinfo:
                print '%s: failure doing recv %s :%s:' % \
                      (mpd_my_id,errinfo.__class__,errinfo)
                break
        return data
    def recv_dict_msg(self,timeout=None):
        global mpd_signum
        global mpd_dbg_level

        mpd_print(mpd_dbg_level, \
                  "Entering recv_dict_msg with timeout=%s" % (str(timeout)))
        msg = {}
        readyToRecv = 0
        if timeout:
            try:
		# Loop while we get EINTR.
                # FIXME: In some cases, we may want to exit if 
	        # the signal was SIGINT.  We need to restart if 
                # we see SIGCLD
                while 1:
                    try:
		        mpd_signum = 0
                        if timeout == -1:
                            # use -1 to indicate indefinite timeout
                            (readyToRecv,unused1,unused2) = select.select([self.sock],[],[])
                        else:
                            (readyToRecv,unused1,unused2) = select.select([self.sock],[],[],timeout)
                        break;
                    except os.error, errinfo:
                        if errinfo[0] == EINTR:
                            # Retry interrupted system calls
                            pass
                        else:
                            raise os.error, errinfo
                # End of the while(1)
            except select.error, errinfo:
                if errinfo[0] == EINTR:
                    if mpd_signum == signal.SIGINT  or  mpd_signum == signal.SIGALRM:
                        mpd_print(0,"sigint/alrm check");
                        pass   # assume timedout; returns {} below
                    elif mpd_signum == signal.SIGCLD:
                        mpd_print_tb(1,"mishandling sigchild in recv_dict_msg, errinfo=:%s" % (errinfo) )
                    else:
                        mpd_print_tb(1,"Unhandled EINTR: errinfo=%s" % (errinfo) )
                else:
                    mpd_print(1, '%s: select error: %s' % (mpd_my_id,os.strerror(errinfo[0])))
            except KeyboardInterrupt, errinfo:
                # print 'recv_dict_msg: keyboard interrupt during select'
                mpd_print(0,"KeyboardInterrupt");
                return msg
            except Exception, errinfo:
                mpd_print(1, 'recv_dict_msg: exception during select %s :%s:' % \
                      ( errinfo.__class__, errinfo))
                return msg
        else:
            readyToRecv = 1
        if readyToRecv:
            mpd_print(mpd_dbg_level,"readyToRecv");
            try:
                pickledLen = ''
                tempRecvd = ''
                lenLeft = 8
                while lenLeft:
                    while (1):
                        try:
                            tempRecvd = self.sock.recv(lenLeft)
                            # FIXME: Shouldn't this block until there is a
                            # message unless it raises an exception.
                            # Is no message an EOF, and in that case, 
                            # do we really want to immediately delete
                            # the corresponding entry?
                            #if not pickledLen:
                            #    mpd_print(1,"continuing because recv failed")
                            #    continue
                            break
                        except socket.error,errinfo:
                            if errinfo[0] == EINTR:
                                mpd_print(mpd_dbg_level,"Saw EINTR")
                                pass
                            elif errinfo[0] == ECONNRESET:
                                mpd_print(mpd_dbg_level,"Saw ECONNRESET, ignore (return null msg)")
                                return msg;
                            else:
                                mpd_print_tb(1,"recv_dict_msg: sock.recv(8): errinfo=:%s:" % (errinfo))
                                raise socket.error,errinfo
                    # end of while(1)
                    if not tempRecvd:
                         break
                    pickledLen += tempRecvd
                    lenLeft -= len(tempRecvd)
                if not pickledLen:
                    mpd_print(mpd_dbg_level,"no pickeled len")
                if pickledLen:
                    pickledLen = int(pickledLen)
                    pickledMsg = ''
                    lenLeft = pickledLen
                    while lenLeft:
                        while (1):
                            try:
                                recvdMsg = self.sock.recv(lenLeft)
                                break
                            except socket.error,errinfo:
                                if errinfo[0] == EINTR:
                                    pass
                                else:
                                    mpd_print_tb(1,"recv_dict_msg: sock.recv(8): errinfo=:%s:" % (errinfo))
                                    raise socket.error,errinfo
                        # end of while(1)            

                        pickledMsg += recvdMsg
                        lenLeft -= len(recvdMsg)
                    msg = loads(pickledMsg)
            except socket.error, errinfo:
                if errinfo[0] == EINTR:
                    mpd_print(1, "Unhandled EINTR on sock.recv")
                    return msg
                elif errinfo[0] == ECONNRESET:   # connection reset (treat as eof)
                    mpd_print(mpd_dbg_level,"Connection reset")
                    pass   # socket.error: (104, 'Connection reset by peer')
                else:
                    mpd_print_tb(1,'recv_dict_msg: socket error: errinfo=:%s:' % (errinfo))
            except StandardError, errmsg:    # any built-in exceptions
                mpd_print_tb(1, 'recv_dict_msg: errmsg=:%s:' % (errmsg) )
            except Exception, errmsg:
                mpd_print_tb(1, 'recv_dict_msg failed on sock %s errmsg=:%s:' % \
                             (self.name,errmsg) )
        if mpd_dbg_level:
            if msg:
                mpd_print(1,"Returning with non-null msg, length = %d, head = %s" % (pickledLen,pickledMsg[0:32].replace('\n','<NL>') ) )
	    else:
                mpd_print(1,"Returning with null msg" )
        return msg
    def recv_char_msg(self):
        return self.recv_one_line()  # use leading len later
    def recv_one_line(self):
        msg = ''
	# A failure with EINTR was observed here, so a loop to retry on 
        # EINTR has been added
        try:
            while 1:
                try:
                    c = self.sock.recv(1)
                    break
                except socket.error, errinfo:
                    if errinfo[0] != EINTR:
                        raise socket.error, errinfo
            # end of while
        except socket.error, errinfo:
            if errinfo[0] == EINTR:   # sigchld, sigint, etc.
                # This should no longer happen (handled above)
                mpd_print_tb( 1,  "Unhandled EINTR in sock.recv" );
                return msg
            elif errinfo[0] == ECONNRESET:   # connection reset (treat as eof)
                return msg
            else:
                print '%s: recv error: %s' % (mpd_my_id,os.strerror(errinfo[0]))
                sys.exit(-1)
        except Exception, errmsg:
            c = ''
            msg = ''
            mpd_print_tb(1, 'recv_char_msg: errmsg=:%s:' % (errmsg) )
        if c:
            while c != '\n':
                msg += c
                try:
                    c = self.sock.recv(1)
                except socket.error, errinfo:
                    if errinfo[0] == EINTR:   # sigchld, sigint, etc.
                        return msg
                    elif errinfo[0] == ECONNRESET:   # connection reset (treat as eof)
                        return msg
                    else:
                        print '%s: recv error: %s' % (mpd_my_id,os.strerror(errinfo[0]))
                        sys.exit(-1)
                except Exception, errmsg:
                    c = ''
                    msg = ''
                    mpd_print_tb(1, 'recv_char_msg: errmsg=:%s:' % (errmsg) )
                    break
            msg += c
        return msg
 
    # The default behavior on an error needs to be to handle and/or report
    # it.  Otherwise, we all waste time trying to figure out why 
    # the code is silently failing.  I've set the default for errprint 
    # to YES rather than NO.
    def send_dict_msg(self,msg,errprint=1):
        pickledMsg = dumps(msg) 
        # FIXME: Does this automatically handle EINTR, or does it need an
        # except os.error, errinfo: and check on errinfo[0] == EINTR
        try:
            while 1:
                try:
                    self.sendall( "%08d%s" % (len(pickledMsg),pickledMsg) )
                    break
                except socket.error, errmsg:
		    if errmsg[0] == EPIPE  \
                    or errmsg[0] == ECONNRESET \
                    or errmsg[0] == EINTR:
			# silent failure on pipe failure, as we usually
                        # just want to discard messages in this case 
                        # (We need to plan error handling more thoroughly)
                        break  ## RMB: chgd from pass
                    else:
                        raise socket.error, errmsg
            # end of While
        except Exception, errmsg:
            mpd_print_tb(errprint,'send_dict_msg: sock=%s errmsg=:%s:' % (self.name,errmsg))
    def send_char_msg(self,msg,errprint=1):
        try:
            while 1:
                try:
                    self.sock.sendall(msg)
                    break
                except socket.error, errmsg:
		    if errmsg[0] == EPIPE:
			# silent failure on pipe failure, as we usually
                        # just want to discard messages in this case 
                        # (We need to plan error handling more thoroughly)
                        pass
                    if errmsg[0] != EINTR:
                        raise socket.error, errmsg
            # end of While
        except Exception, errmsg:
            mpd_print_tb(errprint,'send_char_msg: sock=%s errmsg=:%s:' % (self.name,errmsg))

class MPDListenSock(MPDSock):
    def __init__(self,host='',port=0,filename='',listen=5,name='listener',**kargs):
        MPDSock.__init__(self,name=name,**kargs)
        self.sock.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
        if filename:
            self.sock.bind(filename)
            self.sock.listen(listen)
            return
        # see if we have a PORT_RANGE environment variable
        try:
            port_range = os.environ['MPIEXEC_PORT_RANGE']
            (low_port, high_port) = map(int, port_range.split(':'))
        except:
            try:
                port_range = os.environ['MPICH_PORT_RANGE']
                (low_port, high_port) = map(int, port_range.split(':'))
            except:
                (low_port,high_port) = (0,0)
        if low_port < 0  or  high_port < low_port:
            (low_port,high_port) = (0,0)
        if low_port != 0  and  high_port != 0:
            if port == 0:
                port = low_port
                while 1:
                    try:
                        self.sock.bind((host,port))
                        self.sock.listen(listen)
                        break
                    except socket.error, e:
                        port += 1
                        if port <= high_port:
                            self.sock.close()
                            MPDSock.__init__(self,name=name,**kargs)
                            self.sock.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
                            continue
                        else:
                            mpd_print_tb(1,'** no free ports in MPICH_PORT_RANGE')
                            sys.exit(-1)
            else:  # else use the explicitly specified port
                if port < low_port  or  port > high_port:
                    mpd_print_tb(1,'** port %d is outside MPICH_PORT_RANGE' % port)
                    sys.exit(-1)
                self.sock.bind((host,port))  # go ahead and bind
                self.sock.listen(listen)
        else:
            self.sock.bind((host,port))  # no port range set, so just bind as usual
            self.sock.listen(listen)

class MPDStreamHandler(object):
    def __init__(self):
        self.activeStreams = {}
    def set_handler(self,stream,handler,args=()):
        self.activeStreams[stream] = (handler,args)
    def del_handler(self,stream):
        if self.activeStreams.has_key(stream):
            del self.activeStreams[stream]
    def close_all_active_streams(self):
        for stream in self.activeStreams.keys():
            del self.activeStreams[stream]
            stream.close()
    def handle_active_streams(self,streams=None,timeout=0.1):
        global mpd_signum
        while 1:
            if streams:
                streamsToSelect = streams
            else:
                streamsToSelect = self.activeStreams.keys()
            readyStreams = []
            try:
                mpd_signum = 0
                (readyStreams,u1,u2) = select.select(streamsToSelect,[],[],timeout)
                break
            except select.error, errinfo:
                if errinfo[0] == EINTR:
                    if mpd_signum == signal.SIGCHLD:
                        break
                    if mpd_signum == signal.SIGINT  or  mpd_signum == signal.SIGALRM:
                        break
                    else:
                        continue
                else:
                    print '%s: handle_active_streams: select error: %s' % \
                          (mpd_my_id,os.strerror(errinfo[0]))
                    return (-1,os.strerror(errinfo[0]))
            except KeyboardInterrupt, errinfo:
                # print 'handle_active_streams: keyboard interrupt during select'
                return (-1,errinfo.__class__,errinfo)
            except Exception, errinfo:
                print 'handle_active_streams: exception during select %s :%s:' % \
                      ( errinfo.__class__, errinfo)
                return (-1,errinfo.__class__,errinfo)
        for stream in readyStreams:
            if self.activeStreams.has_key(stream):
                (handler,args) = self.activeStreams[stream]
                handler(stream,*args)
            else:
                # this is not nec bad; an active stream (handler) may
                # have been deleted by earlier handler in this loop
                print '*** OOPS, unknown stream in handle_active_streams'
        return (len(readyStreams),0)  #  len >= 0

class MPDRing(object):
    def __init__(self,listenSock=None,streamHandler=None,secretword='',
                 myIfhn='',entryIfhn='',entryPort=0,zcMyLevel=0):
        if not streamHandler:
            mpd_print(1, "must supply handler for new conns in ring")
            sys.exit(-1)
        if not listenSock:
            mpd_print(1, "must supply listenSock for new ring")
            sys.exit(-1)
        if not myIfhn:
            mpd_print(1, "must supply myIfhn for new ring")
            sys.exit(-1)
        self.secretword = secretword
        self.myIfhn     = myIfhn
        self.generation = 0
        self.listenSock = listenSock
        self.listenPort = self.listenSock.sock.getsockname()[1]
        self.streamHandler = streamHandler
        self.streamHandler.set_handler(self.listenSock,self.handle_ring_listener_connection)
        self.entryIfhn = entryIfhn
        self.entryPort = entryPort
        self.lhsIfhn = ''
        self.lhsPort = 0
        self.rhsIfhn = ''
        self.rhsPort = 0
        self.lhsSock = 0
        self.rhsSock = 0
        self.lhsHandler = None
        self.rhsHandler = None
        self.zcMyLevel = zcMyLevel
        if self.zcMyLevel:
            mpd_init_zc(self.myIfhn,self.zcMyLevel)
    def create_single_mem_ring(self,ifhn='',port=0,lhsHandler=None,rhsHandler=None):
        self.lhsSock,self.rhsSock = mpd_sockpair()
        self.lhsIfhn = ifhn
        self.lhsPort = port
        self.rhsIfhn = ifhn
        self.rhsPort = port
        self.lhsHandler = lhsHandler
        self.streamHandler.set_handler(self.lhsSock,lhsHandler)
        self.rhsHandler = rhsHandler
        self.streamHandler.set_handler(self.rhsSock,rhsHandler)
    def reenter_ring(self,entryIfhn='',entryPort=0,lhsHandler='',rhsHandler='',ntries=5):
        if mpd_zc:
            mpd_close_zc()
            mpd_init_zc(self.myIfhn,self.zcMyLevel)
        rc = -1
        numTries = 0
	self.generation += 1
        while rc < 0  and  numTries < ntries:
            numTries += 1
            rc = self.enter_ring(entryIfhn=entryIfhn,entryPort=entryPort,
                                 lhsHandler=lhsHandler,rhsHandler=rhsHandler,
				 ntries=1)
	    sleepTime = random() * 1.5  # a single random is between 0 and 1
            sleep(sleepTime)
        mpd_print(1,'reenter_ring rc=%d after numTries=%d' % (rc,numTries) )
        return rc
    def enter_ring(self,entryIfhn='',entryPort=0,lhsHandler='',rhsHandler='',ntries=1):
        if not lhsHandler  or  not rhsHandler:
            print 'missing handler for enter_ring'
            sys.exit(-1)
        if not entryIfhn:
            entryIfhn = self.entryIfhn
        if not entryPort:
            entryPort = self.entryPort
        if not entryIfhn  and  mpd_zc:
            if self.zcMyLevel == 1:
                (entryHost,entryPort) = ('',0)
            else:
                (entryIfhn,entryPort) = mpd_find_zc_peer(self.zcMyLevel-1)
                if not entryPort:
                    print "FAILED TO FIND A PEER AT LEVEL", self.zcMyLevel-1
                    sys.exit(-1)
            print "ENTRY INFO", (entryIfhn,entryPort)
        if not entryIfhn:
            self.create_single_mem_ring(ifhn=self.myIfhn,
                                        port=self.listenPort,
                                        lhsHandler=lhsHandler,
                                        rhsHandler=rhsHandler)
        else:
            rv = self.connect_lhs(lhsIfhn=entryIfhn,
                                  lhsPort=entryPort,
                                  lhsHandler=lhsHandler,
                                  numTries=ntries)
            if rv[0] <= 0:  # connect failed with problem
                mpd_print(1,"lhs connect failed")
                return -1
            if rv[1]:  # rhsifhn and rhsport
                rhsIfhn = rv[1][0]
                rhsPort = rv[1][1]
            else:
                mpd_print(1,"did not recv rhs host&port from lhs")
                return -1
            rv = self.connect_rhs(rhsIfhn=rhsIfhn,
                                  rhsPort=rhsPort,
                                  rhsHandler=rhsHandler,
                                  numTries=ntries)
            if rv[0] <=  0:  # connect did not succeed; may try again
                mpd_print(1,"rhs connect failed")
                return -1
        if mpd_zc:
            mpd_register_zc(self.myIfhn,self.zcMyLevel)
        return 0
    def connect_lhs(self,lhsIfhn='',lhsPort=0,lhsHandler=None,numTries=1):
        if not lhsHandler:
            mpd_print(1, "must supply handler for lhs in ring")
            return (-1,None)
        if not lhsIfhn:
            mpd_print(1, "must supply host for lhs in ring")
            return (-1,None)
        self.lhsIfhn = lhsIfhn
        if not lhsPort:
            mpd_print(1, "must supply port for lhs in ring")
            return (-1,None)
        self.lhsPort = lhsPort
        numConnTries = 0
        while numConnTries < numTries:
            numConnTries += 1
            self.lhsSock = MPDSock(name='lhs')
            try:
                self.lhsSock.connect((self.lhsIfhn,self.lhsPort))
            except socket.error, errinfo:
                print '%s: conn error in connect_lhs: %s' % \
                      (mpd_my_id,os.strerror(errinfo[0]))
                self.lhsSock.close()
                self.lhsSock = 0
                sleep(random())
                continue
            break
        if not self.lhsSock  or  numConnTries > numTries:
            mpd_print(1,'failed to connect to lhs at %s %d' % (self.lhsIfhn,self.lhsPort))
            return (0,None)
        msgToSend = { 'cmd' : 'request_to_enter_as_rhs', 'ifhn' : self.myIfhn,
                      'port' : self.listenPort,
                      'mpd_version' : mpd_version() }
        self.lhsSock.send_dict_msg(msgToSend)
        msg = self.lhsSock.recv_dict_msg()
        if (not msg) \
        or (not msg.has_key('cmd')) \
        or (not msg['cmd'] == 'challenge') \
        or (not msg.has_key('randnum')) \
        or (not msg.has_key('generation')):
            mpd_print(1,'invalid challenge from %s %d: %s' % \
                      (self.lhsIfhn,self.lhsPort,msg) )
            return (-1,None)
        if msg['generation'] < self.generation:
            mpd_print(1,'bad generation from lhs; lhsgen=%d mygen=%d' % (msg['generation'],self.generation))
            return(-1,'bad_generation')  # RMB: try again here later
        response = md5new(''.join([self.secretword,msg['randnum']])).digest()
        msgToSend = { 'cmd' : 'challenge_response', 'response' : response,
                      'ifhn' : self.myIfhn, 'port' : self.listenPort }
        self.lhsSock.send_dict_msg(msgToSend)
        msg = self.lhsSock.recv_dict_msg()
        if (not msg) \
        or (not msg.has_key('cmd')) \
        or (not msg['cmd'] == 'OK_to_enter_as_rhs'):
            mpd_print(1,'NOT OK to enter ring; one likely cause: mismatched secretwords')
            return (-1,None)
        self.lhsHandler = lhsHandler
        self.streamHandler.set_handler(self.lhsSock,lhsHandler)
        if msg.has_key('rhsifhn') and msg.has_key('rhsport'):
            return (1,(msg['rhsifhn'],msg['rhsport']))
        else:
            return (1,None)
    def connect_rhs(self,rhsIfhn='',rhsPort=0,rhsHandler=None,numTries=1):
        if not rhsHandler:
            mpd_print(1, "must supply handler for rhs in ring")
            return (-1,None)
        if not rhsIfhn:
            mpd_print(1, "must supply host for rhs in ring")
            return (-1,None)
        self.rhsIfhn = rhsIfhn
        if not rhsPort:
            mpd_print(1, "must supply port for rhs in ring")
            return (-1,None)
        self.rhsPort = rhsPort
        numConnTries = 0
        while numConnTries < numTries:
            numConnTries += 1
            self.rhsSock = MPDSock(name='rhs')
            try:
                self.rhsSock.connect((self.rhsIfhn,self.rhsPort))
            except socket.error, errinfo:
                print '%s: conn error in connect_rhs: %s' % \
                      (mpd_my_id,os.strerror(errinfo[0]))
                self.rhsSock.close()
                self.rhsSock = 0
                sleep(random())
                continue
            break
        if not self.rhsSock or numConnTries > numTries:
            mpd_print(1,'failed to connect to rhs at %s %d' % (self.rhsIfhn,self.rhsPort))
            return (0,None)
        msgToSend = { 'cmd' : 'request_to_enter_as_lhs', 'ifhn' : self.myIfhn,
                      'port' : self.listenPort,
                      'mpd_version' : mpd_version() }
        self.rhsSock.send_dict_msg(msgToSend)
        msg = self.rhsSock.recv_dict_msg()
        if (not msg) \
        or (not msg.has_key('cmd')) \
        or (not msg['cmd'] == 'challenge') \
        or (not msg.has_key('randnum')) \
        or (not msg.has_key('generation')):
            mpd_print(1,'invalid challenge from %s %d: %s' % (self.rhsIfhn,rhsPort,msg) )
            return (-1,None)
        if msg['generation'] < self.generation:
            mpd_print(1,'bad generation from rhs; lhsgen=%d mygen=%d' % (msg['generation'],self.generation))
            return(-1,'bad_generation')  # RMB: try again here later
        response = md5new(''.join([self.secretword,msg['randnum']])).digest()
        msgToSend = { 'cmd' : 'challenge_response', 'response' : response,
                      'ifhn' : self.myIfhn, 'port' : self.listenPort }
        self.rhsSock.send_dict_msg(msgToSend)
        msg = self.rhsSock.recv_dict_msg()
        if (not msg) \
        or (not msg.has_key('cmd')) \
        or (not msg['cmd'] == 'OK_to_enter_as_lhs'):
            mpd_print(1,'NOT OK to enter ring; one likely cause: mismatched secretwords')
            return (-1,None)
        self.rhsHandler = rhsHandler
        self.streamHandler.set_handler(self.rhsSock,rhsHandler)
        if msg.has_key('lhsifhn') and msg.has_key('lhsport'):
            return (1,(msg['lhsifhn'],msg['lhsport']))
        else:
            return (1,None)
    def accept_lhs(self,lhsHandler=None):
        self.lhsHandler = lhsHandler
        newsock = self.handle_ring_listener_connection(self.listenSock)
        self.handle_lhs_challenge_response(newsock)
        self.streamHandler.set_handler(self.lhsSock,lhsHandler)
    def accept_rhs(self,rhsHandler=None):
        self.rhsHandler = rhsHandler
        newsock = self.handle_ring_listener_connection(self.listenSock)
        self.handle_rhs_challenge_response(newsock)
        self.streamHandler.set_handler(self.rhsSock,rhsHandler)
    def handle_ring_listener_connection(self,sock):
        randHiRange = 10000
        (newsock,newaddr) = sock.accept()
        newsock.name = 'candidate_to_enter_ring'
        msg = newsock.recv_dict_msg()
        if (not msg) or \
           (not msg.has_key('cmd')) or (not msg.has_key('ifhn')) or  \
           (not msg.has_key('port')):
            mpd_print(1, 'INVALID msg from new connection :%s: msg=:%s:' % (newaddr,msg) )
            newsock.close()
            return None
        if msg.has_key('mpd_version'):  # ping, etc may not have one
            if msg['mpd_version'] != mpd_version():
                msgToSend = { 'cmd' : 'entry_rejected_bad_mpd_version',
                              'your_version' : msg['mpd_version'],
                              'my_version' : mpd_version() }
                newsock.send_dict_msg(msgToSend)
                newsock.close()
                return None
        randNumStr = '%04d' % (randrange(1,randHiRange))  # 0001-(hi-1), inclusive
        newsock.correctChallengeResponse = \
                         md5new(''.join([self.secretword,randNumStr])).digest()
        msgToSend = { 'cmd' : 'challenge', 'randnum' : randNumStr,
                      'generation' : self.generation }
        newsock.send_dict_msg(msgToSend)
        if msg['cmd'] == 'request_to_enter_as_lhs':
            self.streamHandler.set_handler(newsock,self.handle_lhs_challenge_response)
            newsock.name = 'candidate_for_lhs_challenged'
            return newsock
        elif msg['cmd'] == 'request_to_enter_as_rhs':
            self.streamHandler.set_handler(newsock,self.handle_rhs_challenge_response)
            newsock.name = 'candidate_for_rhs_challenged'
            return newsock
        elif msg['cmd'] == 'ping':
            # already sent challenge instead of ack
            newsock.close()
            return None
        else:
            mpd_print(1, 'INVALID msg from new connection :%s:  msg=:%s:' % (newaddr,msg) )
            newsock.close()
            return None
        return None
    def handle_lhs_challenge_response(self,sock):
        msg = sock.recv_dict_msg()
        if (not msg)   or  \
           (not msg.has_key('cmd'))   or  (not msg.has_key('response'))  or  \
           (not msg.has_key('ifhn'))  or  (not msg.has_key('port'))  or  \
           (not msg['response'] == sock.correctChallengeResponse):
            mpd_print(1, 'INVALID msg for lhs response msg=:%s:' % (msg) )
            msgToSend = { 'cmd' : 'invalid_response' }
            sock.send_dict_msg(msgToSend)
            self.streamHandler.del_handler(sock)
            sock.close()
        else:
            msgToSend = { 'cmd' : 'OK_to_enter_as_lhs' }
            sock.send_dict_msg(msgToSend)
            if self.lhsSock:
                self.streamHandler.del_handler(self.lhsSock)
                self.lhsSock.close()
            self.lhsSock = sock
            self.lhsIfhn = msg['ifhn']
            self.lhsPort = int(msg['port'])
            self.streamHandler.set_handler(self.lhsSock,self.lhsHandler)
            self.lhsSock.name = 'lhs'
    def handle_rhs_challenge_response(self,sock):
        msg = sock.recv_dict_msg()
        if (not msg)   or  \
           (not msg.has_key('cmd'))   or  (not msg.has_key('response'))  or  \
           (not msg.has_key('ifhn'))  or  (not msg.has_key('port')):
            mpd_print(1, 'INVALID msg for rhs response msg=:%s:' % (msg) )
            msgToSend = { 'cmd' : 'invalid_response' }
            sock.send_dict_msg(msgToSend)
            self.streamHandler.del_handler(sock)
            sock.close()
        elif msg['response'] != sock.correctChallengeResponse:
            mpd_print(1, 'INVALID response in rhs response msg=:%s:' % (msg) )
            msgToSend = { 'cmd' : 'invalid_response' }
            sock.send_dict_msg(msgToSend)
            self.streamHandler.del_handler(sock)
            sock.close()
        elif msg['response'] == 'bad_generation':
            mpd_print(1, 'someone failed entering my ring gen=%d msg=%s' % \
                      (self.generation,msg) )
            self.streamHandler.del_handler(sock)
            sock.close()
        else:
            msgToSend = { 'cmd' : 'OK_to_enter_as_rhs', 'rhsifhn' : self.rhsIfhn,
                          'rhsip' : self.rhsIfhn, 'rhsport' : self.rhsPort }
            sock.send_dict_msg(msgToSend)
            if self.rhsSock:
                self.streamHandler.del_handler(self.rhsSock)
                self.rhsSock.close()
            self.rhsSock = sock
            self.rhsIfhn   = msg['ifhn']
            self.rhsPort = int(msg['port'])
            self.streamHandler.set_handler(self.rhsSock,self.rhsHandler)
            self.rhsSock.name = 'rhs'

class MPDConListenSock(MPDListenSock):
    def __init__(self,name='console_listen',secretword='',**kargs):
        if os.environ.has_key('MPD_CON_EXT'):
            self.conExt = '_'  + os.environ['MPD_CON_EXT']
        else:
            self.conExt = ''
        self.conFilename = mpd_tmpdir + '/mpd2.console_' + mpd_get_my_username() + self.conExt
        self.secretword = secretword
        consoleAlreadyExists = 0
        if hasattr(socket,'AF_UNIX'):
            sockFamily = socket.AF_UNIX
        else:
            sockFamily = socket.AF_INET
        if os.environ.has_key('MPD_CON_INET_HOST_PORT'):
            sockFamily = socket.AF_INET    # override above-assigned value
            (conHost,conPort) = os.environ['MPD_CON_INET_HOST_PORT'].split(':')
            conPort = int(conPort)
        else:
            (conHost,conPort) = ('',0)
        if os.access(self.conFilename,os.R_OK):    # if console there, see if mpd listening
            if hasattr(socket,'AF_UNIX')  and  sockFamily == socket.AF_UNIX:
                tempSock = MPDSock(family=socket.AF_UNIX)
                try:
                    tempSock.connect(self.conFilename)
                    consoleAlreadyExists = 1
                except Exception, errmsg:
                    os.unlink(self.conFilename)
                tempSock.close()
            else:
                if not conPort:
                    conFile = open(self.conFilename)
                    for line in conFile:
                        line = line.strip()
                        (k,v) = line.split('=')
                        if k == 'port':
                            conPort = int(v)
                    conFile.close()
                tempSock = MPDSock()
                try:
                    tempSock.sock.connect(('localhost',conPort))
                    consoleAlreadyExists = 1
                except Exception, errmsg:
                    os.unlink(self.conFilename)
                tempSock.close()
        if consoleAlreadyExists:
            print 'An mpd is already running with console at %s on %s. ' % \
                  (self.conFilename, socket.gethostname())
            print 'Start mpd with the -n option for a second mpd on same host.'
            if syslog_module_available:
                syslog.syslog(syslog.LOG_ERR,
                              "%s: exiting; an mpd is already using the console" % \
                              (mpd_my_id))
            sys.exit(-1)
        if hasattr(socket,'AF_UNIX')  and  sockFamily == socket.AF_UNIX:
            MPDListenSock.__init__(self,family=sockFamily,socktype=socket.SOCK_STREAM,
                                   filename=self.conFilename,listen=1,name=name)
        else:
            MPDListenSock.__init__(self,family=sockFamily,socktype=socket.SOCK_STREAM,
                                   port=conPort,listen=1,name=name)
            conFD = os.open(self.conFilename,os.O_CREAT|os.O_WRONLY|os.O_EXCL,0600)
            self.port = self.sock.getsockname()[1]
            os.write(conFD,'port=%d\n' % (self.port) )
            os.close(conFD)

class MPDConClientSock(MPDSock):
    def __init__(self,name='console_to_mpd',mpdroot='',secretword='',**kargs):
        MPDSock.__init__(self)
        self.sock = 0
        if os.environ.has_key('MPD_CON_EXT'):
            self.conExt = '_'  + os.environ['MPD_CON_EXT']
        else:
            self.conExt = ''
        self.secretword = secretword
        if mpdroot:
            self.conFilename = mpd_tmpdir + '/mpd2.console_root' + self.conExt
            self.sock = MPDSock(family=socket.AF_UNIX,name=name)
            rootpid = os.fork()
            if rootpid == 0:
                os.execvpe(mpdroot,[mpdroot,self.conFilename,str(self.sock.fileno())],{})
                mpd_print(1,'failed to exec mpdroot (%s)' % mpdroot )
                sys.exit(-1)
            else:
                (pid,status) = os.waitpid(rootpid,0)
                if os.WIFSIGNALED(status):
                    status = status & 0x007f  # AND off core flag
                else:
                    status = os.WEXITSTATUS(status)
                if status != 0:
                    mpd_print(1,'forked process failed; status=%s' % status)
                    sys.exit(-1)
        else:
            self.conFilename = mpd_tmpdir + '/mpd2.console_' + mpd_get_my_username() + self.conExt
            if hasattr(socket,'AF_UNIX'):
                sockFamily = socket.AF_UNIX
            else:
                sockFamily = socket.AF_INET
            if os.environ.has_key('MPD_CON_INET_HOST_PORT'):
                sockFamily = socket.AF_INET    # override above-assigned value
                (conHost,conPort) = os.environ['MPD_CON_INET_HOST_PORT'].split(':')
                conPort = int(conPort)
            else:
                (conHost,conPort) = ('',0)
            self.sock = MPDSock(family=sockFamily,socktype=socket.SOCK_STREAM,name=name)
            if hasattr(socket,'AF_UNIX')  and  sockFamily == socket.AF_UNIX:
                if hasattr(signal,'alarm'):
                    oldAlarmTime = signal.alarm(8)
                else:    # assume python2.3 or later
                    oldTimeout = socket.getdefaulttimeout()
                    socket.setdefaulttimeout(8)
                try:
                    self.sock.connect(self.conFilename)
                except Exception, errmsg:
                    self.sock.close()
                    self.sock = 0
                if hasattr(signal,'alarm'):
                    signal.alarm(oldAlarmTime)
                else:    # assume python2.3 or later
                    socket.setdefaulttimeout(oldTimeout)
                if self.sock:
                    # this is done by mpdroot otherwise
                    msgToSend = 'realusername=%s secretword=UNUSED\n' % \
                                mpd_get_my_username()
                    self.sock.send_char_msg(msgToSend)
            else:
                if not conPort:
                    conFile = open(self.conFilename)
                    for line in conFile:
                        line = line.strip()
                        (k,v) = line.split('=')
                        if k == 'port':
                            conPort = int(v)
                    conFile.close()
                if conHost:
                    conIfhn = socket.gethostbyname_ex(conHost)[2][0]
                else:
                    conIfhn = 'localhost'
                self.sock = MPDSock(name=name)
                if hasattr(signal,'alarm'):
                    oldAlarmTime = signal.alarm(8)
                else:    # assume python2.3 or later
                    oldTimeout = socket.getdefaulttimeout()
                    socket.setdefaulttimeout(8)
                try:
                    self.sock.connect((conIfhn,conPort))
                except Exception, errmsg:
                    mpd_print(1,"failed to connect to host %s port %d" % \
                              (conIfhn,conPort) )
                    self.sock.close()
                    self.sock = 0
                if hasattr(signal,'alarm'):
                    signal.alarm(oldAlarmTime)
                else:    # assume python2.3 or later
                    socket.setdefaulttimeout(oldTimeout)
                if not self.sock:
                    print '%s: cannot connect to local mpd (%s); possible causes:' % \
                          (mpd_my_id,self.conFilename)
                    print '  1. no mpd is running on this host'
                    print '  2. an mpd is running but was started without a "console" (-n option)'
                    print 'In case 1, you can start an mpd on this host with:'
                    print '    mpd &'
                    print 'and you will be able to run jobs just on this host.'
                    print 'For more details on starting mpds on a set of hosts, see'
                    print 'the MPICH2 Installation Guide.'
                    sys.exit(-1)
                msgToSend = { 'cmd' : 'con_init' }
                self.sock.send_dict_msg(msgToSend)
                msg = self.sock.recv_dict_msg()
                if not msg:
                    mpd_print(1,'expected con_challenge from mpd; got eof')
                    sys.exit(-1)
                if msg['cmd'] != 'con_challenge':
                    mpd_print(1,'expected con_challenge from mpd; got msg=:%s:' % (msg) )
                    sys.exit(-1)
                randVal = self.secretword + str(msg['randnum'])
                response = md5new(randVal).digest()
                msgToSend = { 'cmd' : 'con_challenge_response', 'response' : response,
                              'realusername' : mpd_get_my_username() }
                self.sock.send_dict_msg(msgToSend)
                msg = self.sock.recv_dict_msg()
                if not msg  or  msg['cmd'] != 'valid_response':
                    mpd_print(1,'expected valid_response from mpd; got msg=:%s:' % (msg) )
                    sys.exit(-1)
        if not self.sock:
            print '%s: cannot connect to local mpd (%s); possible causes:' % \
                  (mpd_my_id,self.conFilename)
            print '  1. no mpd is running on this host'
            print '  2. an mpd is running but was started without a "console" (-n option)'
            print 'In case 1, you can start an mpd on this host with:'
            print '    mpd &'
            print 'and you will be able to run jobs just on this host.'
            print 'For more details on starting mpds on a set of hosts, see'
            print 'the MPICH2 Installation Guide.'
            sys.exit(-1)

class MPDParmDB(dict):
    def __init__(self,orderedSources=[]):
        dict.__init__(self)
        self.orderedSources = orderedSources
        self.db = {}
        for src in orderedSources:  # highest to lowest
            self.db[src] = {}
    def __setitem__(self,sk_tup,val):
        if type(sk_tup) != TupleType  or  len(sk_tup) != 2:
            mpd_print_tb(1,"must use a 2-tuple as key in a parm db; invalid: %s" % (sk_tup) )
            sys.exit(-1)
        s,k = sk_tup
        for src in self.orderedSources:
            if src == s:
                self.db[src][k] = val
                break
        else:
            mpd_print_tb(1,"invalid src specified for insert into parm db; src=%s" % (src) )
            sys.exit(-1)
    def __getitem__(self,key):
        for src in self.orderedSources:
            if self.db[src].has_key(key):
                return self.db[src][key]
        raise KeyError, "key %s not found in parm db" % (key)
    def has_key(self,key):
        for src in self.orderedSources:
            if self.db[src].has_key(key):
                return 1
        return 0
    def printall(self):
        print "MPDRUN's PARMDB; values from all sources:"
        for src in self.orderedSources:
            print '  %s (source)' % (src)
            for key in self.db[src].keys():
                print '    %s = %s' % (key,self.db[src][key])
    def printdef(self):
        print "MPDRUN's PARMDB; default values only:"
        printed = {}
        for src in self.orderedSources:
            for key in self.db[src]:
                if not printed.has_key(key):
                    printed[key] = 1
                    print '  %s  %s = %s' % (src,key,self.db[src][key])
    def get_parms_from_env(self,parmsToOverride):
        for k in parmsToOverride.keys():
            if os.environ.has_key(k):
                self[('env',k)] = os.environ[k]
    def get_parms_from_rcfile(self,parmsToOverride,errIfMissingFile=0):
        if os.environ.has_key('MPD_CONF_FILE') and os.access(os.environ['MPD_CONF_FILE'], os.R_OK):
            parmsRCFilename = os.environ['MPD_CONF_FILE']
        elif hasattr(os,'getuid')  and  os.getuid() == 0:    # if ROOT
            parmsRCFilename = os.path.abspath('/etc/mpd.conf')
        elif os.environ.has_key('HOME') and os.access(os.path.join(os.environ['HOME'], '.mpd.conf'), os.R_OK):
            parmsRCFilename = os.path.join(os.environ['HOME'],'.mpd.conf')
        elif os.environ.has_key('HOMEPATH'):    # e.g. win32
            parmsRCFilename = os.path.join(os.environ['HOMEPATH'],'.mpd.conf')
        else:
            print 'unable to find mpd.conf file'
            sys.exit(-1)
        if sys.platform == 'win32':
            mode = 0x80   # fake it
        else:
            try:
                mode = os.stat(parmsRCFilename)[0]
            except:
                mode = ''
	# sometimes a missing file is OK, e.g. when user running with root's mpd
        if not mode  and  not errIfMissingFile:
            return
        if not mode:
            print 'configuration file %s not found' % (parmsRCFilename)
            print 'A file named .mpd.conf file must be present in the user\'s home'
            print 'directory (/etc/mpd.conf if root) with read and write access'
            print 'only for the user, and must contain at least a line with:'
            print 'MPD_SECRETWORD=<secretword>'
            print 'One way to safely create this file is to do the following:'
            print '  cd $HOME'
            print '  touch .mpd.conf'
            print '  chmod 600 .mpd.conf'
            print 'and then use an editor to insert a line like'
            print '  MPD_SECRETWORD=mr45-j9z'
            print 'into the file.  (Of course use some other secret word than mr45-j9z.)' 
            sys.exit(-1)
        if  (mode & 0x3f):
            print 'configuration file %s is accessible by others' % (parmsRCFilename)
            print 'change permissions to allow read and write access only by you'
            sys.exit(-1)
        parmsRCFile = open(parmsRCFilename)
        for line in parmsRCFile:
            lineWithoutComments = line.split('#')[0]    # will at least be ''
            lineWithoutComments = lineWithoutComments.strip()
            if not lineWithoutComments:
                continue
            splitLine = lineWithoutComments.split('=')
            if not splitLine[0]:    # ['']
                print 'warning: unrecognized (null) key in %s' % (parmsRCFilename)
                continue
            if len(splitLine) == 2:
                (k,v) = splitLine
                origKey = k
                if k == 'secretword':    # for bkwd-compat
                    k = 'MPD_SECRETWORD'
                if k in parmsToOverride.keys():
                    if k != 'MPD_SECRETWORD'  and  v.isdigit():
                        v = int(v)
                    self[('rcfile',k)] = v
            else:
                mpd_print(1, 'line in mpd conf is not key=val pair; line=:%s:' % (line) )

class MPDTest(object):
    def __init__(self):
        pass
    def run(self,cmd='',expIn = '',chkEC=0,expEC=0,chkOut=0,expOut='',ordOut=0,
            grepOut=0, exitOnFail=1):
        rv = {}
        if chkOut and grepOut:
            print "grepOut and chkOut are mutually exclusive"
            sys.exit(-1)
        outLines = []
        if subprocess_module_available:
            import re
            cmd = re.split(r'\s+',cmd)
            runner = subprocess.Popen(cmd,bufsize=0,env=os.environ,close_fds=True,
                                      stdin=subprocess.PIPE,stdout=subprocess.PIPE,
                                      stderr=subprocess.PIPE)
            if expIn:
                runner.stdin.write(expIn)
            runner.stdin.close()
            for line in runner.stdout:
                outLines.append(line[:-1])    # strip newlines
            for line in runner.stderr:
                outLines.append(line[:-1])    # strip newlines
            rv['pid'] = runner.pid
            rv['EC'] = runner.wait()
        elif hasattr(popen2,'Popen4'):    # delete when python2.4+ is common
            runner = popen2.Popen4(cmd)
            if expIn:
                runner.tochild.write(expIn)
            runner.tochild.close()
            for line in runner.fromchild:
                outLines.append(line[:-1])    # strip newlines
            rv['pid'] = runner.pid
            rv['EC'] = runner.wait()
        else:
            mpd_print(1,'can not run with either subprocess or popen2-Popen4')
            sys.exit(-1)
        rv['OUT'] = outLines[:]
        if chkEC  and  expEC != rv['EC']:
            print "bad exit code from test: %s" % (cmd)
            print "   expected exitcode=%d ; got %d" % (expEC,rv['EC'])
            print "output from cmd:"
            for line in outLines:
                print line
            if exitOnFail:
                sys.exit(-1)
        if chkOut:
            orderOK = 1
            expOut = expOut.split('\n')[:-1]  # leave off trailing ''
            for line in outLines[:]:    # copy of outLines
                if line in expOut:
                    if ordOut and line != expOut[0]:
                        orderOK = 0
                        break  # count rest of outLines as bad
                    expOut.remove(line)
                    outLines.remove(line)
            if not orderOK:
                print "lines out of order in output for test: %s" % (cmd)
                for line in outLines:
                    print line
                if exitOnFail:
                    sys.exit(-1)
            if expOut:
                print "some required lines not found in output for test: %s" % (cmd)
                for line in outLines:
                    print line
                if exitOnFail:
                    sys.exit(-1)
            if outLines:
                print "extra lines in output for test: %s" % (cmd)
                for line in outLines:
                    print line
                if exitOnFail:
                    sys.exit(-1)
        elif grepOut:
            foundCnt = 0
            for expLine in expOut:
                for outLine in outLines:
                    if outLine.find(expLine) >= 0:
                        foundCnt += 1
            if foundCnt < len(expOut):
                print "some lines not matched for test: %s" % (cmd)
                for line in outLines:
                     print line
                if exitOnFail:
                    sys.exit(-1)
        return rv

#### experimental code for zeroconf
def mpd_init_zc(ifhn,my_level):
    import threading, Zeroconf
    global mpd_zc
    mpd_zc = Zeroconf.Zeroconf()
    class ListenerForPeers(object):
        def __init__(self):
            mpd_zc.peers = {}
            mpd_zc.peersLock = threading.Lock()
            mpd_zc.peers_available_event = threading.Event()
        def removeService(self, zc, service_type, name):
            mpd_zc.peersLock.acquire()
            del mpd_zc.peers[name]
            print "removed", name ; sys.stdout.flush()
            mpd_zc.peersLock.release()
        def addService(self, zc, service_type, name):
            info = zc.getServiceInfo(service_type, name)
            if info:
                if info.properties['username'] != mpd_get_my_username():
                    return
                mpd_zc.peersLock.acquire()
                mpd_zc.peers[name] = info
                print "added peer:", name, info.properties ; sys.stdout.flush()
                mpd_zc.peersLock.release()
                mpd_zc.peers_available_event.set()
            else:
                print "OOPS NO INFO FOR", name ; sys.stdout.flush()
    service_type = "_mpdzc._tcp.local."
    listenerForPeers = ListenerForPeers()
    browser = Zeroconf.ServiceBrowser(mpd_zc,service_type,listenerForPeers)
    ##  sleep(1.5)  # give browser a chance to find some peers
def mpd_find_zc_peer(peer_level):
    print "finding a peer at level %d..." % (peer_level) ; sys.stdout.flush()
    mpd_zc.peers_available_event.wait(5)
    for (peername,info) in mpd_zc.peers.items():
        if info.properties['mpdid'] == mpd_my_id:
            continue
        if info.properties['level'] != peer_level:
            continue
        peerAddr = str(socket.inet_ntoa(info.getAddress()))
        peerPort = info.getPort()
        return(peerAddr,peerPort)
    return ('',0)
def mpd_register_zc(ifhn,level):
    import Zeroconf
    service_type = "_mpdzc._tcp.local."
    service_ifhn = socket.inet_aton(ifhn)
    service_host = socket.gethostname()
    service_port = int(mpd_my_id.split('_')[1])
    svc = Zeroconf.ServiceInfo(service_type,
                               mpd_my_id + service_type,
                               address = service_ifhn,
                               port = service_port,
                               weight = 0, priority = 0,
                               properties = { 'description': 'mpd',
                                              'mpdid' : mpd_my_id,
                                              'level' : level,
                                              'username' : mpd_get_my_username() }
                               )
    mpd_zc.registerService(svc)
def mpd_close_zc():
    if mpd_zc:
        mpd_zc.close()


# code for testing

def _handle_msg(sock):
    msg = sock.recv_dict_msg()
    print 'recvd msg=:%s:' % (msg)

if __name__ == '__main__':
    sh = MPDStreamHandler()
    (tsock1,tsock2) = mpd_sockpair()
    tsock1.name = 'tsock1_connected_to_tsock2'
    sh.set_handler(tsock1,_handle_msg)
    tsock2.send_dict_msg( {'msgtype' : 'hello'} )
    sh.handle_active_streams()
    # just to demo a listen sock
    lsock = MPDListenSock('',9999,name='listen_sock')
    print lsock.name, lsock.getsockname()[1]

    ### import sys
    ### sys.excepthook = mpd_uncaught_except_tb
    ### i = 1/0
