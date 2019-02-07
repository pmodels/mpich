dnl PAC_ARG_THREAD_PACKAGE
dnl  - Provide configure option to select a thread package. Defaults to posix.
AC_DEFUN([PAC_ARG_THREAD_PACKAGE], [
     AC_ARG_WITH([thread-package],
     [  --with-thread-package=package     Thread package to use. Supported thread packages include:
        posix or pthreads - POSIX threads (default, if required)
        solaris - Solaris threads (Solaris OS only)
        abt or argobots - Argobots threads
        win - windows threads
        uti - POSIX threads plus Utility Thread Offloading library
        none - no threads
     ],,with_thread_package=posix)])
