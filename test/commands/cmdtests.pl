# -*- Mode: perl; -*-
#
# Test the commands provided as part of MPICH
#
# note: my test run returns 32 errors, need investigate -- hzhou
#
# mpicc, mpicxx - handle -Dname="foo bar" and -Dname='"foo bar"'
# (not done yet - see mpich1 test/command/runtests)
# mpiexec - environment handling; stdout, stderr redirection
#
# Configuration values
my $mpiexec = "mpiexec";
my $mpicc = "mpicc";
my $mpicxx = "mpicxx";
my $mpif77 = "mpif77";

my $srcdir      = ".";
if ($0=~/(.*)\//) {
    $srcdir = $1;
}

my %opt;
foreach my $a (@ARGV){
    if ($a=~/^-(cxx|f77)/) {
        $opt{$1} = 1;
    }
    elsif ($a=~/^-bindir=(.+)/){
        $mpiexec = "$1/mpiexec";
        $mpicc = "$1/mpicc";
        $mpicxx = "$1/mpicxx";
        $mpif77 = "$1/mpif77";
    }
    elsif ($a=~/^--help/){
        print "perl $0 [-cxx] [-f77] [-bindir=path_to_mpiexec]\n";
        exit 0;
    }
}

# Get a way to kill processes
my $killall = "killall";

# Global variables
my $errors = 0;

$gVerbose = 0;

$xmlOutput = 0;
$xmlFile = "";
$xmlOutputContinue = 0;
# testoutput is used to keep a copy of all output in case there is an
# error and XML output containing the messages is desired.
$testoutput = "";

# The MPD process manager has trouble with stdin.  While this should be
# fixed, for the moment, we provide a way to turn off those tests
$testStdin = 0;
if (defined($ENV{"MPIEXEC_HAS_STDIN"})) { $testStdin = 1; }

# Set a default for the timeout 
# (The stdintest has hung sometimes; a correctly functioning mpiexec
# will abort when this timelimit is exceeded)
if (!defined($ENV{"MPIEXEC_TIMEOUT"})) {
    $ENV{"MPIEXEC_TIMEOUT"} = 20;
}

#
my $myusername = "";
if (defined($ENV{'LOGNAME'})) {
    $myusername = $ENV{'LOGNAME'};
}

# -------------------------------------------------------------------------
if (defined($ENV{'XMLFILE'})) {
    $xmlOutput = 1;
    $xmlFile   = $ENV{'XMLFILE'};
}
if (defined($ENV{'XMLCONTINUE'}) && $ENV{'XMLCONTINUE'} eq "YES") {
    $xmlOutputContinue = 1;
}

foreach $_ (@ARGV) {
    if (/-debug/ || /-verbose/) { 
	$gVerbose = 1;
    }
    if (/-xmlfile=(.*)/) {
	$xmlFile = $1;
	$xmlOutput = 1;
    }
    else {
	print STDERR "Unrecognized argument $_\n";
    }
}
# -------------------------------------------------------------------------
if ($xmlOutput) {
    if ($xmlOutputContinue) {
	open XMLFD, ">>$xmlFile" || die "Cannot append to $xmlFile";
    }
    else {
	open XMLFD, ">$xmlFile" || die "Cannot open $xmlFile for writing";
    }
}
# -------------------------------------------------------------------------
# mpiexec env handling
# We assume that we can run non-MPI programs
%SaveENV = %ENV;

$ENV{TestEnvVar} = "test var name";
%EnvBase = ('PMI_FD' => 1, 'PMI_RANK' => 0, 'PMI_SIZE' => 1, 
	    'PMI_DEBUG' => 0, 
	    'MPI_APPNUM' => 0, 'MPI_UNIVERSE_SIZE' => 1, 
	    'PMI_PORT' => 1, 
	    'MPICH_INTERFACE_HOSTNAME' => 1,
	    'MPICH_INTERFACE_HOSTNAME_R0' => 1,
	    'MPICH_INTERFACE_HOSTNAME_R1' => 1,
	    'MPICH_INTERFACE_HOSTNAME_R2' => 1,
	    'MPICH_INTERFACE_HOSTNAME_R3' => 1,
	    # These are suspicious
	    'PMI_SPAWNED' => 0, 
	    'PMI_TOTALVIEW' => 0,
	    );
# Other environment variables should be rejected

# Processes on cygwin always have SYSTEMROOT and WINDIR set
%EnvForced = ( 'SYSTEMROOT' => 1, 'WINDIR' => 1 );

%EnvExpected = ();

print "Try some environment args\n" if $gVerbose;

# Do we get the environment?
%EnvSeen = ();
%EnvExpected = ( 'PATH' => $ENV{PATH} );

&Announce( "$mpiexec printenv" );
open MOUT, "$mpiexec printenv 2>&1 |" || die "Could not run $mpiexec";
while (<MOUT>) {
    # We check for the error output from gforker mpiexec; we can
    # add others here as well
    if (/([^=]+)=(.*)/ && ! /Return code/) {
	$EnvSeen{$1} = $2;
    }
    else {
	&ReportError( "Unexpected output from mpiexec: $_" );
	$errors ++;
    }
}
close MOUT;
$rc = $?;
if ($rc != 0) {
    $errors ++;
    &ReportError( "Non-zero return ($rc) from mpiexec\n" );
}
# Check that all vars in save are seen
%copyEnv = %SaveENV;
foreach my $key (keys(%EnvSeen)) {
    if (defined($EnvBase{$key})) { next; }
    delete $copyEnv{$key};
}
foreach my $key (keys(%copyEnv)) {
    print "Enviroment variable $key not delivered to target program\n";
    $errors ++;
}

%EnvSeen = ();
%EnvExpected = ( 'PATH' => $ENV{PATH} );
&Announce( "$mpiexec -envnone -envlist PATH printenv" );
open MOUT, "$mpiexec -envnone -envlist PATH printenv 2>&1 |" || die "Could not run $mpiexec";
while (<MOUT>) {
    # We check for the error output from gforker mpiexec; we can
    # add others here as well
    if (/([^=]+)=(.*)/ && ! /Return code/) {
	$EnvSeen{$1} = $2;
    }
    else {
	&ReportError( "Unexpected output from mpiexec: $_" );
	$errors ++;
    }
}
close MOUT;
$rc = $?;
if ($rc != 0) {
    $errors ++;
    &ReportError( "Non-zero return ($rc) from mpiexec\n" );
}
# Check that only PATH and the PMI variables are set
$errors += &CheckEnvVars;

%EnvSeen = ();
%EnvExpected = ( 'PATH' => $ENV{PATH} );
&Announce( "$mpiexec -genvnone -genvlist PATH printenv " );
open MOUT, "$mpiexec -genvnone -genvlist PATH printenv 2>&1 |" || die "Could not run $mpiexec";
while (<MOUT>) {
    # We check for the error output from gforker mpiexec; we can
    # add others here as well
    if (/([^=]+)=(.*)/ && ! /Return code/) {
	$EnvSeen{$1} = $2;
    }
    else {
	&ReportError( "Unexpected output from mpiexec: $_" );
	$errors ++;
    }
}
close MOUT;
$rc = $?;
if ($rc != 0) {
    $errors ++;
    &ReportError( "Non-zero return ($rc) from mpiexec\n" );
}
# Check that only PATH and the PMI variables are set
$errors += &CheckEnvVars;

%EnvSeen = ();
%EnvExpected = ( 'PATH' => $ENV{PATH},
		 'TestEnvVar' => $ENV{TestEnvVar} );
&Announce ( "$mpiexec -genvnone -genvlist PATH printenv : -envlist TestEnvVar,PATH printenv" );
open MOUT, "$mpiexec -genvnone -genvlist PATH printenv : -envlist TestEnvVar,PATH printenv 2>&1 |" || die "Could not run $mpiexec";
while (<MOUT>) {
    # We check for the error output from gforker mpiexec; we can
    # add others here as well
    if (/Return code/) { next; }
    if (/([^=]+)=(.*)/) {
	$EnvSeen{$1} = $2;
    }
    else {
	&ReportError( "Unexpected output from mpiexec: $_" );
	$errors ++;
    }
}
close MOUT;
$rc = $?;
if ($rc != 0) {
    $errors ++;
    &ReportError( "Non-zero return ($rc) from mpiexec\n" );
}
# Check that only PATH and the PMI variables are set
$errors += &CheckEnvVars;

# This test creates long env variables
my $varvalue = "a";
for (my $i=0; $i<11; $i++) {
    $varvalue .= $varvalue;
}

$ENV{TESTVAR} = $varvalue;
%EnvSeen = ();
%EnvExpected = ( 'PATH' => $ENV{PATH},
		 'TESTVAR' => $ENV{TESTVAR},
		 );
&Announce( "$mpiexec -envnone -envlist PATH,TESTVAR printenv" );
open MOUT, "$mpiexec -envnone -envlist PATH,TESTVAR printenv 2>&1 |" || die "Could not run $mpiexec";
while (<MOUT>) {
    # We check for the error output from gforker mpiexec; we can
    # add others here as well
    if (/([^=]+)=(.*)/ && ! /Return code/) {
	$EnvSeen{$1} = $2;
    }
    else {
	&ReportError( "Unexpected output from mpiexec: $_" );
	$errors ++;
    }
}
close MOUT;
$rc = $?;
if ($rc != 0) {
    $errors ++;
    &ReportError( "Non-zero return ($rc) from mpiexec\n" );
}
# Check that only PATH, TESTVAR, and the PMI variables are set, and
# that they have the correct values
$errors += &CheckEnvVars;

delete $ENV{TESTVAR};

# Intel reports problems with
# export TMP_ENV_VAR=1
# mpiexec -genvlist TMP_ENV_VAR -n 1 -host host1 ./a.out : -n 1 -host host2 \
#   ./a.out
# or using the equivalent config file:
# mpiexec -configfile a.out.cfgfile
# where a.out.cfgfile contains
#   -genvlist TMP_ENV_VAR
#   -n 1 -host host1 ./a.out
#   -n 1 -host host2 ./a.out
#
$ENV{'TMP_ENV_VAR'} = 1;
$myhost = `hostname`;
$myhost =~ s/\r?\n//;
&Announce( "$mpiexec -genvlist TMP_ENV_VAR -n 1 -host $myhost ./checkenv1 : -n 1 -host $myhost ./checkenv1" );
if (! -x "checkenv1" ) {
    system "make checkenv1";
    if (! -x "checkenv1") {
	&ReportError( "Unable to build checkenv1 program\n" );
	$errors ++;
    }
}
open MOUT, "$mpiexec -genvlist TMP_ENV_VAR -n 1 -host $myhost ./checkenv1 : -n 1 -host $myhost ./checkenv1 |" || die "Could not run $mpiexec";
$foundNoErrors = 0;
while (<MOUT>) {
    if (/ No Errors/) {
	$foundNoErrors = 1;
    }
    else {
	&ReportError( "Unexpected output from mpiexec: $_" );
	$errors ++;
    }
}
close MOUT;
$rc = $?;
if ($rc != 0) {
    $errors ++;
    &ReportError( "Non-zero return ($rc) from mpiexec\n" );
}
if (! $foundNoErrors) {
    $errors ++;
    &ReportError( "checkenv1 did not return No Errors\n" );
}
delete $ENV{'TMP_ENV_VAR'};

# -------------------------------------------------------------------------
# mpiexec stdout/stderr handling
&Announce( "$mpiexec ./stdiotest 2>err.txt 1>out.txt" );
if (! -x "stdiotest" ) {
    system "make stdiotest";
    if (! -x "stdiotest") {
	&ReportError( "Unable to build stdiotest program\n" );
	$errors ++;
    }
}
if (-x "stdiotest") {
    unlink "err.txt";
    unlink "out.txt";
    system "$mpiexec ./stdiotest 2>err.txt 1>out.txt";
    # Compare the expected output
    if (-s "err.txt" && -s "out.txt") {
	# We check for the expected output line.  We allow (but warn about)
	# output that was not generated by the program, since that
	# makes it impossible to write Unix-style pipes that include
	# parallel programs.
        open TFD, "<err.txt";
 	$sawOutput = 0;
	$sawExtraLine = 0;
	$sawExtraChars = 0;
	while (<TFD>) {
	    if (/(.*)This is stderr(.*)/) {
		my $pre = $1;
		my $post = $2;
		$sawOutput++;
		if ($pre ne "" || $post ne "") { $sawExtraChars++; }
	    }
	    else {
		print STDERR "Unexpected text in stderr: $_" if $showWarnings;
		$sawExtraLine ++;
	    }
	}
	close TFD;
	if ($sawOutput != 1) {
	    &ReportError( "Saw expected stderr line $sawOutput times\n" );
	    $errors ++;
	}
	open TFD, "<out.txt";
 	$sawOutput = 0;
	$sawExtraLine = 0;
	$sawExtraChars = 0;
	while (<TFD>) {
	    if (/(.*)This is stdout(.*)/) {
		my $pre = $1;
		my $post = $2;
		$sawOutput++;
		if ($pre ne "" || $post ne "") { $sawExtraChars++; }
	    }
	    else {
		if (/This is stderr/) {
		    &ReportError( "stderr output in stdout file\n" );
		    $errors++;
		}
		print STDERR "Unexpected text in stderr: $_" if $showWarnings;
		$sawExtraLine ++;
	    }
	}
	close TFD;
	if ($sawOutput != 1) {
	    &ReportError( "Saw expected stdout line $sawOutput times\n" );
	    $errors ++;
	}
    }
    else {
	if (! -s "out.txt") {
	    &ReportError( "Program stdiotest did not create stdout file\n" );
	    $errors ++;
	}
	if (! -s "err.txt") {
	    &ReportError( "Program stdiotest did not create stderr file\n" );
	    $errors ++;
	}
    }
    unlink "err.txt";
    unlink "out.txt";
}
# -------------------------------------------------------------------------
# mpiexec stdin handling
if ($testStdin) {
    &Announce( "$mpiexec ./stdintest <in.txt 2>err.txt 1>out.txt" );
    if (! -x "stdintest" ) {
	system "make stdintest";
	if (! -x "stdintest") {
	    &ReportError( "Unable to build stdintest program\n" );
	    $errors ++;
	}
    }
    if (-x "stdintest") {
	unlink "in.txt";
	unlink "err.txt";
	unlink "out.txt";
	# Create the input file
	open TFD, ">in.txt" || die "Cannot create test input file";
	for (my $i = 0; $i < 1024; $i++) {
	    print TFD "This is line $i\n";
	}
	close TFD;
	
	$rc = system "$mpiexec ./stdintest <in.txt 2>err.txt 1>out.txt";
	if ($rc != 0) {
	    &ReportError( "Program stdintest failed with return $rc" );
	    $errors ++;
	}
	# Because some mpiexec programs fail to properly handle these stdio
	# tests, we add a step to kill the test program if mpiexec failed
	# to remove it.
	if ($killall ne "true" && $myusername ne "") {
	    system "$killall -u $myusername stdintest";
	}
	# Compare the expected output
	if (-s "out.txt") {
	    &CheckEchoedOutput;
	}
	else {
	    &ReportError( "Program stdintest did not create stdout file\n" );
	    $errors ++;
	}
	if (-s "err.txt") {
	    &ReportError( "Program stdintest created a non-empty stderr file\n" );
	    $errors ++;
	    system "cat err.txt";
	}
	unlink "err.txt";
	unlink "out.txt";
	
	# Try with a parallel job
	&Announce( "$mpiexec -n 3 ./stdintest <in.txt 2>err.txt 1>out.txt" );
	$rc = system "$mpiexec -n 3 ./stdintest <in.txt 2>err.txt 1>out.txt";
	if ($rc != 0) {
	    &ReportError( "Program stdintest (3 processes) failed with return $rc" );
	    $errors ++;
	}
	# Because some mpiexec programs fail to properly handle these stdio
	# tests, we add a step to kill the test program if mpiexec failed
	# to remove it.
	if ($killall ne "true" && $myusername ne "") {
	    system "$killall -u $myusername stdintest";
	}
	# Compare the expected output
	if (-s "out.txt") {
	    &CheckEchoedOutput;
	}
	else {
	    &ReportError( "Program stdintest did not create stdout file\n" );
	    $errors ++;
	}
	if (-s "err.txt") {
	    &ReportError( "Program stdintest created a non-empty stderr file\n" );
	    $errors ++;
	    system "cat err.txt";
	}
	unlink "err.txt";
	unlink "out.txt";

	unlink "in.txt";
    }
    #
    # Test for allowing stdin to have unread data
    &Announce( "$mpiexec ./stdintest2 <in.txt 2>err.txt 1>out.txt" );
    if (! -x "stdintest2" ) {
	system "make stdintest2";
	if (! -x "stdintest2") {
	    &ReportError( "Unable to build stdintest2 program\n" );
	    $errors ++;
	}
    }
    if (-x "stdintest2") {
	unlink "in.txt";
	unlink "err.txt";
	unlink "out.txt";
	# Create the input file
	open TFD, ">in.txt" || die "Cannot create test input file";
	for (my $i = 0; $i < 1024; $i++) {
	    print TFD "This is line $i\n";
	}
	close TFD;
	$rc = system "$mpiexec ./stdintest2 <in.txt 2>err.txt 1>out.txt";
	if ($rc != 0) {
	    &ReportError( "Program stdintest2 failed with return $rc" );
	    $errors ++;
	}
	# Because some mpiexec programs fail to properly handle these stdio
	# tests, we add a step to kill the test program if mpiexec failed
	# to remove it.
	if ($killall ne "true" && $myusername ne "") {
	    system "$killall -u $myusername stdintest2";
	}
	# Compare the expected output
	if (-s "out.txt") {
	    # Check for No Errors ONLY
	    open TFD, "<out.txt" || die "Cannot create test input file";
	    $found_no_errors = 0;
	    my $found_other = 0;
	    while (<TFD>) {
		if (/No Errors/) {
		    $found_no_errors = 1;
		}
		else {
		    if (!$found_other) {
			&ReportError( "Found unexpected text %_" );
		    }
		    $found_other = 1;
		    $errors ++;
		}
	    }
	    close TFD;
	    if (! $found_no_errors) {
		&ReportError( "Did not find No Errors" );
		$errors++;
	    }
	}
	if (-s "err.txt") {
	    &ReportError( "Program stdintest2 created a non-empty stderr file\n" );
	    $errors ++;
	    system "cat err.txt";
	}
	unlink "err.txt";
	unlink "out.txt";
	unlink "in.txt";
    }
}
# -------------------------------------------------------------------------
# Compliation script testing, particularly for special command-line arguments
$cmd = "mpicc";
#$outlog = "/dev/null";
$outlog = "out.log";
unlink $outlog;
&Announce( "$mpicc -Dtestname=\\\"foo\\\" $srcdir/rtest.c" );
system "$mpicc -Dtestname=\\\"foo\\\" $srcdir/rtest.c > $outlog 2>&1";
$rc = $?;
if ($rc != 0) {
    &ReportError( "Error with escaped double quotes in $cmd\n" );
    system( "cat $outlog" );
    $errors ++;
}

unlink $outlog;
&Announce( "$mpicc -Dtestname='\"foo bar\"' $srcdir/rtest.c" );
system "$mpicc -Dtestname='\"foo bar\"' $srcdir/rtest.c  > $outlog 2>&1";
$rc = $?;
if ($rc != 0) {
    &ReportError( "Error with double inside of single quotes in $cmd\n" );
    system( "cat $outlog" );
    $errors ++;
}
unlink "a.out";

# Run this test only if mpicxx is valid
if ($opt{cxx}) {
    $cmd = "mpicxx";
    unlink $outlog;
    &Announce( "$mpicxx -Dtestname=\\\"foo\\\" $srcdir/rtestx.cxx" );
    system "$mpicxx -Dtestname=\\\"foo\\\" $srcdir/rtestx.cxx  > $outlog 2>&1";
    $rc = $?;
    if ($rc != 0) {
	&ReportError( "Error with escaped double quotes in $cmd\n" );
	system( "cat $outlog" );
	$errors ++;
    }
    unlink $outlog;
    &Announce( "$mpicxx -Dtestname='\"foo bar\"' $srcdir/rtestx.cxx" );
    system "$mpicxx -Dtestname='\"foo bar\"' $srcdir/rtestx.cxx > $outlog 2>&1";
    $rc = $?;
    if ($rc != 0) {
	&ReportError( "Error with double inside of single quotes in $cmd\n" );
	system( "cat $outlog" );
	$errors ++;
    }
    unlink "a.out";
}
# Run this test only if mpif77 is valid
if ($opt{f77}) {
    $cmd = "mpif77";
    unlink $outlog;
    &Announce( "$mpif77 -Dtestname=\\\"foo\\\" $srcdir/rtestf.F" );
    system "$mpif77 -Dtestname=\\\"foo\\\" $srcdir/rtestf.F  > $outlog 2>&1";
    $rc = $?;
    if ($rc != 0) {
	&ReportError( "Error with escaped double quotes in $cmd\n" );
	system( "cat $outlog" );
	$errors ++;
    }
    unlink $outlog;
    &Announce( "$mpif77 -Dtestname='\"foo bar\"' $srcdir/rtestf.F" );
    system "$mpif77 -Dtestname='\"foo bar\"' $srcdir/rtestf.F > $outlog 2>&1";
    $rc = $?;
    if ($rc != 0) {
	&ReportError( "Error with double inside of single quotes in $cmd\n" );
	system( "cat $outlog" );
	$errors ++;
    }
    unlink "a.out";
}

# FIXME: 
# Still to do:
# Tests for other options to mpiexec (other combinations of env options,
# all MPI-2 mandated options)
#
# ------------------------------------------------------------------------
# Test Summary
if ($errors != 0) {
    print " Found $errors errors\n";
}
else {
    print " No Errors\n";
}
if ($xmlOutput) {
    $newline = "\r\n";
    my $workdir = `pwd`;
    $workdir =~ s/\r?\n//;
    print XMLFD "<MPITEST>$newline<NAME>cmdtest</NAME>$newline";
    print XMLFD "<WORKDIR>$workdir</WORKDIR>$newline";
    if ($errors != 0) {
	print XMLFD "<STATUS>fail</STATUS>$newline";
	$testoutput =~ s/</\*AMP\*lt;/g;
	$testoutput =~ s/>/\*AMP\*gt;/g;
	$testoutput =~ s/&/\*AMP\*amp;/g;
	$testoutput =~ s/\*AMP\*/&/g;
	print XMLFD "<TESTDIFF>$newline$testoutput</TESTDIFF>$newline";
    }
    else {
	print XMLFD "<STATUS>pass</STATUS>$newline";
    }
    print XMLFD "</MPITEST>$newline</MPITESTRESULTS>$newline";
    close XMLFD;
}
exit 0;
# ------------------------------------------------------------------------
# Testing routines
# Check for environment variables
# For simplicity, uses global variables:
#    EnvSeen - variables seen
#    EnvBase - variables part of the PMI interface
#    EnvExpected - other variables
sub CheckEnvVars {
    my $errcount = 0;
    foreach my $key (keys(%EnvSeen)) {
	if (defined($EnvBase{$key})) { next; }
	if (defined($EnvExpected{$key})) { 
	    my $expectedValue = $EnvExpected{$key};
	    my $observedValue = $EnvSeen{$key};
	    if ($expectedValue ne $observedValue) {
		$errcount++;
		&ReportError( "Environment variable $key has value $observedValue but $expectedValue expected\n" );
	    }
	    next; 
	}
	if (defined($EnvForced{$key})) { next; }
	$value = $EnvSeen{$key};
	&ReportError( "Unexpected environment variable $key with $value\n" );
	$errcount ++;
    }
    return $errcount;
}
    
sub Announce {
    my $msg = $_[0];

    print STDOUT $msg . "\n";
    $testoutput .= $msg . "\n";
}

sub ReportError {
    my $msg = $_[0];
    if ( !($msg =~ /\n/) ) { $msg .= "\n"; }
    print STDERR $msg;
    $testoutput .= $msg;
}

# This routine is used to check the output from a program that reads 
# from stdin and echos the output.
sub CheckEchoedOutput {
    # We check for the expected output line.  We allow (but warn about)
    # output that was not generated by the program, since that
    # makes it impossible to write Unix-style pipes that include
    # parallel programs.
    open TFD, "<out.txt";
    my $sawOutput = 0;
    my $sawExtraLine = 0;
    my $sawExtraChars = 0;
    my $expectedLinenum = 0;
    my $linesSeen = 0;

    while (<TFD>) {
	if (/(.*)This is line (\d+)(.*)/) {
	    my $pre = $1;
	    my $linenum = $2;
	    my $post = $3;
	    $sawOutput++;
	    $linesSeen++;
	    if ($pre ne "" || $post ne "") { $sawExtraChars++; }
	    if ($linenum != $expectedLinenum) {
		&ReportError( "Unexpected linenumber in output; expected $expectedLinenum but say $linenum\n" );
		$errors++;
	    }
	    $expectedLinenum++;
	}
	else {
	    print STDERR "Unexpected text in stdout: $_" if $showWarnings;
	    $sawExtraLine ++;
	}
    }
    close TFD;
    if ($linesSeen != 1024) {
	&ReportError( "Did not see entire input file (only $linesSeen lines)\n" );
	$errors++;
    }
}
