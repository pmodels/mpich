[![Build Status](https://travis-ci.org/halimamer/izem.svg?branch=master)](https://travis-ci.org/halimamer/izem)

			Izem Release %VERSION%

Izem is a shared-memory synchronization library that offers several
synchronization mechanisms (e.g. locks and conditional variables)
and also concurrent data-structures (e.g. lists and queues).

1. Getting Started
2. Testing
3. Reporting Problems
4. Etymology

-------------------------------------------------------------------------

1. Getting Started
==================

The following instructions take you through a sequence of steps to get
the default configuration up and running.

(a) You will need the following prerequisites.

    - REQUIRED: This tar file izem-%VERSION%.tar.gz

    - REQUIRED: A C compiler (gcc is sufficient)

    Also, you need to know what shell you are using since different shell
    has different command syntax. Command "echo $SHELL" prints out the
    current shell used by your terminal program.

(b) Unpack the tar file and go to the top level directory:

      tar xzf izem-%VERSION%.tar.gz
      cd izem-%VERSION%

    If your tar doesn't accept the z option, use

      gunzip izem-%VERSION%.tar.gz
      tar xf izem-%VERSION%.tar
      cd izem-%VERSION%

(c) Choose an installation directory, say
    /home/<USERNAME>/izem-install, which is assumed to non-existent
    or empty.

(d) Specify the installation directory:

    for csh and tcsh:

      ./configure --prefix=/home/<USERNAME>/izem-install |& tee c.txt

    for bash and sh:

      ./configure --prefix=/home/<USERNAME>/izem-install 2>&1 | tee c.txt

    Bourne-like shells, sh and bash, accept "2>&1 |".  Csh-like shell,
    csh and tcsh, accept "|&". If a failure occurs, the configure
    command will display the error. Most errors are straight-forward
    to follow.

(e) Build Izem:

    for csh and tcsh:

      make |& tee m.txt

    for bash and sh:

      make 2>&1 | tee m.txt

    This step should succeed if there were no problems with the
    preceding step. Check file m.txt. If there were problems, do a
    "make clean" and then run make again with V=1.

      make V=1 |& tee m.txt       (for csh and tcsh)

      OR

      make V=1 2>&1 | tee m.txt   (for bash and sh)

    Then go to step (3) below, for reporting the issue to the Izem
    developers and other users.

(f) Install Izem:

    for csh and tcsh:

      make install |& tee mi.txt

    for bash and sh:

      make install 2>&1 | tee mi.txt

    This step collects all required executables and scripts in the bin
    subdirectory of the directory specified by the prefix argument to
    configure.

-------------------------------------------------------------------------

2. Testing
===================

We package a test suite in the Izem distribution. You can run the test
suite in the test directory using:

     make check

     OR

     make testing

The distribution also includes some Izem examples. You can run
them in the examples directory using:

     make check

     OR

     make testing

If you run into any problems on running the test suite or examples,
please follow step (3) below for reporting them to the Izem
developers and other users.

-------------------------------------------------------------------------

3. Reporting Problems
=====================

If you have problems with the installation or usage of Izem, please
contact Halim Amer at aamer (at) anl (dot) gov

-------------------------------------------------------------------------

4. Etymology
=====================

Izem means Lion in Berber. "I" here is pronounced "E" in English (the
whole word will sound similar to "Eden").
