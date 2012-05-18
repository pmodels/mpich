#!/usr/bin/env ruby

# This script tests that hydra gives the correct output and error codes when
# child programs exit in various ways.  If hydra's output format ever changes
# then this test will need to be updated.

require 'test/unit'

class TestExitStatus < Test::Unit::TestCase
  def test_exit0
    output = `mpiexec -n 1 sh -c 'exit 0'`
    assert $?.exitstatus == 0
    assert output =~ /^$/;
  end

  def test_exit1
    errcode = 1
    output = `mpiexec -n 1 sh -c 'exit #{errcode}'`
    assert($?.exitstatus == errcode)
    assert(output =~ /EXIT CODE: #{errcode}$/, "actual error code (#{errcode}) not reported")

    # also test the exact output format in this test and assume it is OK for now
    # in most other cases
    expected = <<-EOS
=====================================================================================
=   BAD TERMINATION OF ONE OF YOUR APPLICATION PROCESSES
=   EXIT CODE: #{errcode}
=   CLEANING UP REMAINING PROCESSES
=   YOU CAN IGNORE THE BELOW CLEANUP MESSAGES
=====================================================================================
    EOS
    assert(expected.strip == output.strip)
  end

  def test_exit254
    errcode = 254
    output = `mpiexec -n 1 sh -c 'exit #{errcode}'`
    assert($?.exitstatus == errcode)
    assert(output =~ /EXIT CODE: #{errcode}$/, "actual error code (#{errcode}) not reported")
  end

  def test_exit255
    errcode = 255
    output = `mpiexec -n 1 sh -c 'exit #{errcode}'`
    assert($?.exitstatus == errcode)
    assert(output =~ /EXIT CODE: #{errcode}$/, "actual error code (#{errcode}) not reported")
  end

  def test_segfault
    # fake a segfault by sending SIGSEGV to ourself
    output = `mpiexec -n 1 perl -e 'kill 11, $$'`

    # arguably this should really be 139 (128+11) to match up with regular shell
    # behavior, although I don't know what the clearest way is to express this
    # to the user
    errcode = 11
    assert($?.exitstatus == errcode)
    assert(output =~ /EXIT CODE: #{errcode}$/, "actual error code (#{errcode}) not reported")

    # the "Segmentation fault: 11" string is probably platform-specific, we
    # should compare accordingly
    expected = <<-EOS
=====================================================================================
=   BAD TERMINATION OF ONE OF YOUR APPLICATION PROCESSES
=   EXIT CODE: #{errcode}
=   CLEANING UP REMAINING PROCESSES
=   YOU CAN IGNORE THE BELOW CLEANUP MESSAGES
=====================================================================================
YOUR APPLICATION TERMINATED WITH THE EXIT STRING: Segmentation fault: 11 (signal 11)
This typically refers to a problem with your application.
Please see the FAQ page for debugging suggestions
      EOS
    # strip whitespace to be a little less sensitive
    assert(expected.strip == output.strip)
  end

  # TODO:
  # - libc abort()
  # - MPI_Abort()
  # - something with more than one process
end

