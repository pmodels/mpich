#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage: mpdkilljob  jobnum  [mpdid]  # as obtained from mpdlistjobs
   or: mpdkilljob  -a jobalias      # as obtained from mpdlistjobs
    mpdid is mpd where process with 'rank' 0 starts
    mpdid of form 1@linux02_32996 (may need \@ in csh)
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.17 $"
__credits__ = ""


import sys, os, signal

from  mpdlib  import  mpd_set_my_id, mpd_uncaught_except_tb, mpd_print, \
                      mpd_handle_signal, mpd_get_my_username, MPDConClientSock, MPDParmDB


def mpdkilljob():
    import sys    # to get access to excepthook in next line
    sys.excepthook = mpd_uncaught_except_tb
    if len(sys.argv) < 2  or  sys.argv[1] == '-h'  or  sys.argv[1] == '--help':
        usage()
    signal.signal(signal.SIGINT, sig_handler)
    mpd_set_my_id(myid='mpdkilljob')
    mpdid = ''
    if sys.argv[1] == '-a':
        jobalias = sys.argv[2]
        jobnum = '0'
    else:
        jobalias = ''
        jobid = sys.argv[1]
        sjobid = jobid.split('@')
        jobnum = sjobid[0]
        if len(sjobid) > 1:
            mpdid = sjobid[1]

    parmdb = MPDParmDB(orderedSources=['cmdline','xml','env','rcfile','thispgm'])
    parmsToOverride = {
                        'MPD_USE_ROOT_MPD'            :  0,
                        'MPD_SECRETWORD'              :  '',
                      }
    for (k,v) in parmsToOverride.items():
        parmdb[('thispgm',k)] = v
    parmdb.get_parms_from_env(parmsToOverride)
    parmdb.get_parms_from_rcfile(parmsToOverride)
    if (hasattr(os,'getuid')  and  os.getuid() == 0)  or  parmdb['MPD_USE_ROOT_MPD']:
        fullDirName = os.path.abspath(os.path.split(sys.argv[0])[0])  # normalize
        mpdroot = os.path.join(fullDirName,'mpdroot')
        conSock = MPDConClientSock(mpdroot=mpdroot,secretword=parmdb['MPD_SECRETWORD'])
    else:
        conSock = MPDConClientSock(secretword=parmdb['MPD_SECRETWORD'])

    msgToSend = { 'cmd':'mpdkilljob', 'jobnum' : jobnum, 'mpdid' : mpdid,
                  'jobalias' : jobalias, 'username' : mpd_get_my_username() }
    conSock.send_dict_msg(msgToSend)
    msg = conSock.recv_dict_msg(timeout=5.0)
    if not msg:
        mpd_print(1,'no msg recvd from mpd before timeout')
        sys.exit(-1)
    if msg['cmd'] != 'mpdkilljob_ack':
        if msg['cmd'] == 'already_have_a_console':
            print 'mpd already has a console (e.g. for long ringtest); try later'
        else:
            print 'unexpected message from mpd: %s' % (msg)
        sys.exit(-1)
    if not msg['handled']:
        print 'job not found'
        sys.exit(-1)
    conSock.close()

def sig_handler(signum,frame):
    mpd_handle_signal(signum,frame)  # not nec since I exit next
    sys.exit(-1)

def usage():
    print __doc__
    sys.exit(-1)

if __name__ == '__main__':
    mpdkilljob()
