#!/usr/bin/env perl

use strict;
use warnings;

use Cwd qw( realpath );
use Getopt::Long;

my $arg = 0;
my $source = "";
my $version = "";
my $pack = "";
my $root = $ENV{PWD};

my $logfile = "release.log";

sub usage
{
    print "Usage: $0 [--source source] {--package package} [version]\n";
    exit;
}

sub check_package
{
    my $pack = shift;

    print "===> Checking for package $pack... ";
    if (`which $pack` eq "") {
	print "not found\n";
	exit;
    }
    print "done\n";
}

sub run_cmd
{
    my $cmd = shift;

    # FIXME: Allow for verbose output; just have to remove the
    # redirection to /dev/null
    system("$cmd 2>&1 | tee -a $root/$logfile > /dev/null");
    if ($?) {
        die "unable to execute ($cmd), \$?=$?.  Stopped";
    }
}

sub debug
{
    my $line = shift;

    print "$line";
}

sub create_docs
{
    my $pack = shift;

    if ($pack eq "romio") {
	chdir("romio/doc");
	run_cmd("make");
	run_cmd("rm -f users-guide.blg users-guide.toc users-guide.aux users-guide.bbl users-guide.log users-guide.dvi");
    }
    elsif ($pack eq "mpe") {
	chdir("mpe2/maint");
	run_cmd("make -f Makefile4man");
    }
}

sub create_mpich2
{
    # Check out the appropriate source
    debug("===> Checking out mpich2 SVN source... ");
    run_cmd("rm -rf mpich2-${version}");
    run_cmd("svn export -q ${source} mpich2-${version}");
    debug("done\n");

    # Remove packages that are not being released
    debug("===> Removing packages that are not being released... ");
    chdir("${root}/mpich2-${version}");
    run_cmd("rm -rf src/mpid/globus doc/notes src/pm/mpd/Zeroconf.py src/mpid/ch3/channels/gasnet src/mpid/ch3/channels/sshm src/pmi/simple2");

    chdir("${root}/mpich2-${version}/src/mpid/ch3/channels/nemesis/nemesis/net_mod");
    my @nem_modules = qw(newtcp sctp ib psm);
    run_cmd("rm -rf ".join(' ', map({$_ . "_module/*"} @nem_modules)));
    for my $module (@nem_modules) {
	# system to avoid problems with shell redirect in run_cmd
	system(qq(echo "# Stub Makefile" > ${module}_module/Makefile.sm));
    }
    debug("done\n");

    # Create configure
    debug("===> Creating configure in the main package... ");
    chdir("${root}/mpich2-${version}");
    run_cmd("./maint/updatefiles");
    debug("done\n");

    # Remove unnecessary files
    debug("===> Removing unnecessary files in the main package... ");
    chdir("${root}/mpich2-${version}");
    run_cmd("rm -rf README.in maint/config.log maint/config.status unusederr.txt autom4te.cache src/mpe2/src/slog2sdk/doc/jumpshot-4/tex");
    debug("done\n");

    # Get docs
    debug("===> Creating secondary package for the docs... ");
    chdir("${root}");
    run_cmd("cp -a mpich2-${version} mpich2-${version}-tmp");
    debug("done\n");

    debug("===> Configuring and making the secondary package... ");
    chdir("${root}/mpich2-${version}-tmp");
    run_cmd("./maint/updatefiles");
    run_cmd("./configure --without-mpe --disable-f90 --disable-f77 --disable-cxx");
    run_cmd("(make mandoc && make htmldoc && make latexdoc)");
    debug("done\n");

    debug("===> Copying docs over... ");
    chdir("${root}/mpich2-${version}-tmp");
    run_cmd("cp -a man ${root}/mpich2-${version}");
    run_cmd("cp -a www ${root}/mpich2-${version}");
    run_cmd("cp -a doc/userguide/user.pdf ${root}/mpich2-${version}/doc/userguide");
    run_cmd("cp -a doc/installguide/install.pdf ${root}/mpich2-${version}/doc/installguide");
    run_cmd("cp -a doc/smpd/smpd_pmi.pdf ${root}/mpich2-${version}/doc/smpd");
    run_cmd("cp -a doc/logging/logging.pdf ${root}/mpich2-${version}/doc/logging");
    run_cmd("cp -a doc/windev/windev.pdf ${root}/mpich2-${version}/doc/windev");
    chdir("${root}");
    run_cmd("rm -rf mpich2-${version}-tmp");
    debug("done\n");

    debug("===> Creating ROMIO docs... ");
    chdir("${root}/mpich2-${version}/src/mpi");
    create_docs("romio");
    debug("done\n");

    debug( "===> Creating MPE docs... ");
    chdir("${root}/mpich2-${version}/src");
    create_docs("mpe");
    debug("done\n");

    # Create the tarball
    debug("===> Creating the final mpich2 tarball... ");
    chdir("${root}");
    run_cmd("tar -czvf mpich2-${version}.tgz mpich2-${version}");
    run_cmd("rm -rf mpich2-${version}");
    debug("done\n\n");
}

sub create_romio
{
    # Check out the appropriate source
    debug("===> Checking out romio SVN source... ");
    run_cmd("rm -rf romio-${version} romio");
    run_cmd("svn export -q ${source}/src/mpi/romio");
    debug("done\n");

    debug("===> Creating configure... ");
    chdir("${root}/romio");
    run_cmd("autoreconf");
    debug("done\n");

    debug("===> Creating ROMIO docs... ");
    chdir("${root}");
    create_docs("romio");
    debug("done\n");

    # Create the tarball
    debug("===> Creating the final romio tarball... ");
    chdir("${root}");
    run_cmd("mv romio romio-${version}");
    run_cmd("tar -czvf romio-${version}.tgz romio-${version}");
    run_cmd("rm -rf romio-${version}");
    debug("done\n\n");
}

sub create_mpe
{
    # Check out the appropriate source
    debug("===> Checking out mpe2 SVN source... ");
    run_cmd("rm -rf mpe2-${version} mpe2");
    run_cmd("svn export -q ${source}/src/mpe2");
    debug("done\n");

    debug("===> Creating configure... ");
    chdir("${root}/mpe2");
    run_cmd("./maint/updatefiles");
    debug("done\n");

    debug("===> Creating MPE docs... ");
    chdir("${root}");
    create_docs("mpe");
    debug("done\n");

    # Create the tarball
    debug("===> Creating the final mpe2 tarball... ");
    chdir("${root}");
    run_cmd("mv mpe2 mpe2-${version}");
    run_cmd("tar -czvf mpe2-${version}.tgz mpe2-${version}");
    run_cmd("rm -rf mpe2-${version}");
    debug("done\n\n");
}

GetOptions(
    "source=s" => \$source,
    "package:s"  => \$pack,
    "help"     => \&usage,
) or die "unable to parse options, stopped";

if (scalar(@ARGV) != 1) {
    usage();
}

$version = $ARGV[0];

if (!$pack) {
    $pack = "mpich2";
}

if (!$source || !$version) {
    usage();
}

check_package("doctext");
check_package("svn");
check_package("latex");
check_package("autoconf");
debug "\n";

my $current_ver = `svn cat ${source}/maint/Version`;

if ("$current_ver" ne "$version\n") {
    debug("\n\tWARNING: Version mismatch\n\n");
}

system("rm -f ${root}/$logfile");

if ($pack eq "mpich2") {
    create_mpich2();
}
elsif ($pack eq "romio") {
    create_romio();
}
elsif ($pack eq "mpe") {
    create_mpe();
}
elsif ($pack eq "all") {
    create_mpich2();
    create_romio();
#     create_mpe();
}
else {
    die "Unknown package: $pack";
}
