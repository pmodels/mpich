#!/usr/bin/env perl

use strict;
use warnings;

use Cwd qw( realpath );

my $arg = 0;
my $source = "";
my $version = "";
my $root = `echo \$PWD`;
$root =~ s/\n$//;

my $logfile = "release.log";

sub usage()
{
    print "Usage: dist.pl [--source source] [version]\n";
    exit;
}

sub check_package()
{
    my $pack = shift;

    print "===> Checking for package $pack... ";
    if ("`which $pack`" eq "") {
	print "not found\n";
	exit
    }
    print "done\n";
}

sub run_cmd()
{
    my $cmd = shift;

    $cmd = "$cmd" . " 2>&1 | tee -a $logfile > /dev/null";
    system("$cmd");
}

sub debug()
{
    my $line = shift;

    print "$line";
}

while ($arg <= $#ARGV) {
    if ("$ARGV[$arg]" eq "--source") {
	$source = "$ARGV[++$arg]";
    }
    elsif (("$ARGV[$arg]" eq "--help") || ("$ARGV[$arg]" eq "-h")) {
	usage();
    }
    else {
	$version = "$ARGV[$arg]";
    }

    $arg++;
}

if (!$source || !$version) {
    usage();
}

&check_package("doctext");
&check_package("svn");
&check_package("latex");
&check_package("autoconf");

my $current_ver = `svn cat ${source}/maint/Version`;

if ("$current_ver" ne "$version\n") {
    &debug("\n\tWARNING: Version mismatch\n\n");
}

system("rm -f ${root}/release.log");

# Check out the appropriate source
&debug("===> Checking out SVN source... ");
&run_cmd("rm -rf mpich2-${version}");
&run_cmd("svn export -q ${source} mpich2-${version}");
&debug("done\n");

# Remove packages that are not being released
&debug("===> Removing packages that are not being released... ");
chdir("${root}/mpich2-${version}");
&run_cmd("rm -rf src/mpid/globus doc/notes src/pm/mpd/Zeroconf.py src/mpid/ch3/channels/gasnet src/mpid/ch3/channels/sshm src/pmi/simple2");

chdir("${root}/mpich2-${version}/src/mpid/ch3/channels/nemesis/nemesis/net_mod");
&run_cmd("rm -rf newtcp_module/* sctp_module/* ib_module/* psm_module/*");
&run_cmd("echo \"\# Stub Makefile\" > newtcp_module/Makefile.sm");
&run_cmd("echo \"\# Stub Makefile\" > sctp_module/Makefile.sm");
&run_cmd("echo \"\# Stub Makefile\" > ib_module/Makefile.sm");
&run_cmd("echo \"\# Stub Makefile\" > psm_module/Makefile.sm");
&debug("done\n");

# Create configure
&debug("===> Creating configure in the main package... ");
chdir("${root}/mpich2-${version}");
&run_cmd("maint/updatefiles 2>&1 | tee -a ${root}/release.log > /dev/null");
&debug("done\n");

# Remove unnecessary files
&debug("===> Removing unnecessary files in the main package... ");
chdir("${root}/mpich2-${version}");
&run_cmd("rm -rf maint/config.log maint/config.status unusederr.txt autom4te.cache src/mpe2/src/slog2sdk/doc/jumpshot-4/tex");
&debug("done\n");

# Get docs
&debug("===> Creating secondary package for the docs... ");
chdir("${root}");
&run_cmd("cp -a mpich2-${version} mpich2-${version}-tmp");
&debug("done\n");

&debug("===> Configuring and making the secondary package... ");
chdir("${root}/mpich2-${version}-tmp");
&run_cmd("maint/updatefiles 2>&1 | tee -a ${root}/release.log > /dev/null");
&run_cmd("./configure --without-mpe --disable-f90 --disable-f77 --disable-cxx 2>&1 | tee -a ${root}/release.log > /dev/null");
&run_cmd("(make mandoc && make htmldoc && make latexdoc) 2>&1 | tee -a ${root}/release.log > /dev/null");
&debug("done\n");

&debug("===> Create ROMIO docs... ");
chdir("${root}/mpich2-${version}-tmp/src/mpi/romio/doc");
&run_cmd("make 2>&1 | tee -a ${root}/release.log > /dev/null");
&debug("done\n");

&debug("===> Copying docs over... ");
chdir("${root}/mpich2-${version}-tmp");
&run_cmd("cp -a man ${root}/mpich2-${version}");
&run_cmd("cp -a www ${root}/mpich2-${version}");
&run_cmd("cp -a doc/userguide/user.pdf ${root}/mpich2-${version}/doc/userguide");
&run_cmd("cp -a doc/installguide/install.pdf ${root}/mpich2-${version}/doc/installguide");
&run_cmd("cp -a doc/smpd/smpd_pmi.pdf ${root}/mpich2-${version}/doc/smpd");
&run_cmd("cp -a doc/logging/logging.pdf ${root}/mpich2-${version}/doc/logging");
&run_cmd("cp -a doc/windev/windev.pdf ${root}/mpich2-${version}/doc/windev");
&run_cmd("cp -a src/mpi/romio/doc/users-guide.pdf ${root}/mpich2-${version}/src/mpi/romio/doc");
chdir("${root}");
&run_cmd("rm -rf mpich2-${version}-tmp");
&debug("done\n");

&debug( "===> Create MPE docs... ");
chdir("${root}/mpich2-${version}/src/mpe2/maint");
&run_cmd("make -f Makefile4man 2>&1 | tee -a ${root}/release.log > /dev/null");
&debug("done\n");


# Create the tarball
&debug("===> Creating the final tarball... ");
chdir("${root}");
&run_cmd("tar -czvf mpich2-${version}.tgz mpich2-${version} 2>&1 | tee -a ${root}/release.log > /dev/null");
&debug("done\n");
