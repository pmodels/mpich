#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
This program is not to be executed from the command line.  It is 
exec'd by mpdroot to verify the version of python before executing
a 'regular' mpd pgm, e.g. mpdallexit.
"""

from sys import version, exit
if version[0] == '1' and version[1] == '.':
    print "mpdchkpyver: your python version must be >= 2.2 ;"
    print "  current version is:", version
    exit(-1)

##  These must be after 1.x version check
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.4 $"
__credits__ = ""

from sys     import argv, exit
from os      import environ, execvpe
from mpdlib  import mpd_check_python_version

if __name__ == '__main__':
    if len(argv) == 1  or  argv[1] == '-h'  or  argv[1] == '--help':
        print __doc__
	exit(-1)
    else:
        vinfo = mpd_check_python_version()
        if vinfo:
            print "mpdchkpyver: your python version must be >= 2.2 ; current version is:", vinfo
            exit(-1)
        if len(argv) > 1:
            mpdpgm = argv[1] + '.py'
            # print "CHKPYVER: PGM=:%s: ARGV[1:]=:%s:" % (mpdpgm,argv[1:])
            try:
                execvpe(mpdpgm,argv[1:],environ)    # client
            except Exception, errinfo:
                print 'mpdchkpyver: failed to exec %s; info=%s' % (mpdpgm,errinfo)
                exit(-1)
