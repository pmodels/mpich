#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

from popen2 import Popen4

def run(cmd='',expexit=0,ignexit=0,expout='',ignout=0,ordout=0):
    rc = 0
    runner = Popen4(cmd)
    outLines = []
    for line in runner.fromchild:
        outLines.append(line[:-1])    # strip newlines
    exitCode = runner.wait()
    if expexit != exitCode:
	rc = -1
        print "bad exit code from test: %s" % (cmd)
        print expexit,exitCode
        print "  expected %d ; actual %d" % (expexit,exitCode)
    if not ignout:
        expout = expout.split('\n')[:-1]  # leave off trailing ''
        for line in outLines[:]:    # copy of outLines
            if line in expout:
                if ordout and line != expout[0]:
	            rc = -1
                    print 'lines not in order'
                    break  # count rest as bad
	        expout.remove(line)
                outLines.remove(line)
        if expout or outLines:
	    rc = -1
            print "bad output for test: %s" % (cmd)
	if expout:
	    rc = -1
	    print 'lines not found:'
	    for line in expout:
	        print '  ', line
	if outLines:
	    rc = -1
	    print 'extra lines:'
	    for line in outLines:
	        print '  ', line
    return(rc)


if __name__ == '__main__':
    import os
    pyExt = '.py'   # '' or '.py'
    os.environ['MPD_CON_EXT'] = 'testing'
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (pyExt) )
    rc = os.system("mpdboot%s" % (pyExt) )
    if rc != 0:
        print 'mpdboot failed; exiting'
        exit(-1)
    # os.system("mpdtrace%s" % (pyExt) )
    run(cmd='mpiexec%s -n 1 echo there : -n 1 echo hi' % (pyExt),
        expout='hi\nthere\n')  # fails with ordout=1
    if rc == 0:
        print 'echo test successful'
    os.system("mpdallexit%s 1> /dev/null 2> /dev/null" % (pyExt) )
