# Nightly Tests

Each night we run an extensive series of tests on a set of
(platform,configuration) pairs. These results can be found on [the
summary page](http://www.mpich.org/static/cron/tests).

##Compilers Tested
Below is a list of compilers that are regularly tested.

- GNU
- Intel
- PGI
- Absoft

SUN compilers, as well as other vendor compilers, such as the IBM XL
compilers, are not included in our nightly tests but are tested on an
occasional basis, depending mostly on the availability of a test
platform.

## Test Results

- [Special Tests](http://www.mpich.org/static/cron/specialtests) -
  This shows the results of special tests, such as configuring,
  building, and testing with memory tracing (`--enable-g=mem`),
  logging, or shared libraries, on one system (usually Linux). See
  [Running the Special Tests](Testing_MPICH.md#running-the-special-tests)
  for information on running or modifying these tests. It is a good idea
  to add tests to this file in response to user bug reports to ensure
  that fixed bugs stay fixed.

- [Configure Options Tests](http://www.mpich.org/static/cron/errors/testoptions.htm) -
  This shows the results of configuring, building, and installing with
  a variety of configure options, on one system (usually Linux). See
  [Running the configure tests](Testing_MPICH.md#running-the-configure-tests)
  for information on running or modifying these tests. It is a good idea
  to add tests to this file in response to user bug reports to ensure
  that fixed bugs stay fixed.

We are currently missing the following test result pages from the old
website:

- **Results from the MPICH test suite** - This shows more diverse
  tests but run against only the MPICH test suite. As an added
  benefit, these results package up all relevant generated files,
  include config.log, from each configured subdirectory.



## See Also

- [Writing New Tests](Writing_New_Tests.md)
- [Testing Crons](Testing_Crons.md)
