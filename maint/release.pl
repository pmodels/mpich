#!/usr/bin/env perl
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#
#
# Known limitations:
#
#    1. ABI mismatch checks are run using a git diff in mpi.h.in and
#    the binding directory. This can come up with false positives, and
#    is only meant to be a worst-case guess.
#

use strict;
use warnings;

use Cwd qw( cwd getcwd realpath );
use Getopt::Long;
use File::Temp qw( tempdir );

my $arg = 0;
my $source = "";
my $psource = "";
my $version = "";
my $append_commit_id;
my $root = cwd();
my $with_autoconf = "";
my $with_automake = "";
my $git_repo_path = ".";

# Default to MPICH
my $pack = "mpich";

my $logfile = "release.log";

sub usage
{
    print "Usage: $0 [OPTIONS]\n\n";
    print "OPTIONS:\n";

    print "\t--source          git tree-ish specifying source to be packaged\n";
    print "\t--psource         git tree-ish for the previous version source for ABI compliance (required)\n";

    # what package we are creating
    print "\t--package         package to create (optional)\n";

    # version string associated with the tarball
    print "\t--version         tarball version (required)\n";

    # append a valid git commit ID (SHA1 or git-describe output)
    print "\t--append-commit-id   append git commit ID (optional)\n";

    print "\t--git-repository  path to root of the git repository\n";

    print "\n";

    exit 1;
}

sub check_package
{
    my $pack = shift;

    print "===> Checking for package $pack... ";
    if ($with_autoconf and ($pack eq "autoconf")) {
        # the user specified a dir where autoconf can be found
        if (not -x "$with_autoconf/$pack") {
            print "not found\n";
            exit;
        }
    }
    if ($with_automake and ($pack eq "automake")) {
        # the user specified a dir where automake can be found
        if (not -x "$with_automake/$pack") {
            print "not found\n";
            exit;
        }
    }
    else {
        if (`which $pack` eq "") {
            print "not found\n";
            exit;
        }
    }
    print "done\n";
}

sub check_autotools_version
{
    my $tool = shift;
    my $req_ver = shift;
    my $curr_ver;

    $curr_ver = `$tool --version | head -1 | cut -f4 -d' ' | xargs echo -n`;
    if ("$curr_ver" ne "$req_ver") {
	print("\tWARNING: $tool version mismatch ($req_ver) required\n\n");
    }
}

# will also chdir to the top level of the git repository
sub check_git_repo {
    my $repo_path = shift;

    print "===> chdir to $repo_path\n";
    chdir $repo_path;

    print "===> Checking git repository sanity... ";
    unless (`git rev-parse --is-inside-work-tree 2> /dev/null` eq "true\n") {
        print "ERROR: $repo_path is not a git repository\n";
        exit 1;
    }
    # I'm not strictly sure that this is true, but it's not too burdensome right
    # now to restrict it to complete (non-bare repositories).
    unless (`git rev-parse --is-bare-repository 2> /dev/null` eq "false\n") {
        print "ERROR: $repo_path is a *bare* repository (need working tree)\n";
        exit 1;
    }

    # last sanity check
    unless (-e "maint/extracterrmsgs") {
        print "ERROR: does not appear to be a valid MPICH repository\n" .
              "(missing maint/extracterrmsgs)\n";
        exit 1;
    }

    print "done\n";
}


sub run_cmd
{
    my $cmd = shift;

    #print("===> running cmd=|$cmd| from ".getcwd()."\n");
    system("$cmd >> $root/$logfile 2>&1");
    if ($?) {
        die "unable to execute ($cmd), \$?=$?.  Stopped";
    }
}

GetOptions(
    "source=s" => \$source,
    "psource=s" => \$psource,
    "package:s"  => \$pack,
    "version=s" => \$version,
    "append-commit-id!" => \$append_commit_id,
    "with-autoconf" => \$with_autoconf,
    "with-automake" => \$with_automake,
    "git-repository=s" => \$git_repo_path,
    "help"     => \&usage,

    # old deprecated args, retained with usage() to help catch non-updated cron
    # jobs and other stale scripts/users
    "append-svnrev!" => sub {usage();},
) or die "unable to parse options, stopped";

if (scalar(@ARGV) != 0) {
    usage();
}

if (!$source || !$version || !$psource) {
    usage();
}

check_package("doctext");
check_package("txt2man");
check_package("git");
check_package("latex");
check_package("autoconf");
check_package("automake");
print("\n");

## IMPORTANT: Changing the autotools versions can result in ABI
## breakage. So make sure the ABI string in the release tarball is
## updated when you do that.
check_autotools_version("autoconf", "2.69");
check_autotools_version("automake", "1.12.4");
check_autotools_version("libtool", "2.4.2");
print("\n");

# chdirs to $git_repo_path if valid
check_git_repo($git_repo_path);
print("\n");

my $current_ver = `git show ${source}:maint/version.m4 | grep MPICH_VERSION_m4 | \
                   sed -e 's/^.*\\[MPICH_VERSION_m4\\],\\[\\(.*\\)\\].*/\\1/g'`;
if ("$current_ver" ne "$version\n") {
    print("\tWARNING: Version mismatch\n\n");
}

if ($psource) {
    # Check diff
    my $d = `git diff ${psource}:src/include/mpi.h.in ${source}:src/include/mpi.h.in`;
    $d .= `git diff ${psource}:src/binding ${source}:src/binding`;
    if ("$d" ne "") {
	print("\tWARNING: ABI mismatch\n\n");
    }
}

if ($append_commit_id) {
    my $desc = `git describe --always ${source}`;
    chomp $desc;
    $version .= "-${desc}";
}

my $tdir = tempdir(CLEANUP => 1);

# Clean up the log file
system("rm -f ${root}/$logfile");

# Check out the appropriate source
print("===> Exporting $pack source from git... ");
run_cmd("rm -rf ${pack}-${version}");
my $expdir = "${tdir}/${pack}-${version}";
run_cmd("mkdir -p ${expdir}");
run_cmd("git archive ${source} --prefix='${pack}-${version}/' | tar -x -C $tdir");
print("done\n");

print("===> Create release date and version information... ");
chdir($expdir);

my $date = `date`;
chomp $date;
system(qq(perl -p -i -e 's/\\[MPICH_RELEASE_DATE_m4\\],\\[unreleased development copy\\]/[MPICH_RELEASE_DATE_m4],[$date]/g' ./maint/version.m4));
# the main version.m4 file will be copied to hydra's version.m4, including the
# above modifications
print("done\n");

# Remove packages that are not being released
print("===> Removing packages that are not being released... ");
chdir($expdir);
run_cmd("rm -rf doc/notes src/pm/mpd/Zeroconf.py");

chdir("${expdir}/src/mpid/ch3/channels/nemesis/netmod");
my @nem_modules = qw(elan);
run_cmd("rm -rf ".join(' ', @nem_modules));
for my $module (@nem_modules) {
    run_cmd("rm -rf $module");
    run_cmd(q{perl -p -i -e '$_="" if m|^\s*include \$.*netmod/}.${module}.q{/Makefile.mk|' Makefile.mk});
}
print("done\n");

# Create configure
print("===> Creating configure in the main package... ");
chdir($expdir);
{
    my $cmd = "./autogen.sh";
    $cmd .= " --with-autoconf=$with_autoconf" if $with_autoconf;
    $cmd .= " --with-automake=$with_automake" if $with_automake;
    run_cmd($cmd);
}
print("done\n");

# Disable unnecessary tests in the release tarball
print("===> Disabling unnecessary tests in the main package... ");
chdir($expdir);
run_cmd("perl -p -i -e 's/^perf\$/#perf/' test/mpi/testlist.in");
run_cmd("perl -p -i -e 's/^large_message /#large_message /' test/mpi/pt2pt/testlist");
run_cmd("perl -p -i -e 's/^large-count /#large-count /' test/mpi/datatype/testlist");
print("done\n");

# Remove unnecessary files
print("===> Removing unnecessary files in the main package... ");
chdir($expdir);
run_cmd("rm -rf README.vin maint/config.log maint/config.status unusederr.txt");
run_cmd("find . -name autom4te.cache | xargs rm -rf");
print("done\n");

# Get docs
print("===> Creating secondary package for the docs... ");
run_cmd("cp -a ${expdir} ${expdir}-tmp");
print("done\n");

print("===> Configuring and making the secondary package... ");
chdir("${expdir}-tmp");
{
    my $cmd = "./autogen.sh";
    $cmd .= " --with-autoconf=$with_autoconf" if $with_autoconf;
    $cmd .= " --with-automake=$with_automake" if $with_automake;
    run_cmd($cmd);
}
run_cmd("./configure --disable-fc --disable-f77 --disable-cxx");
run_cmd("(make mandoc && make htmldoc && make latexdoc)");
print("done\n");

print("===> Copying docs over... ");
chdir("${expdir}-tmp");
run_cmd("cp -a man ${expdir}");
run_cmd("cp -a www ${expdir}");
run_cmd("cp -a doc/userguide/user.pdf ${expdir}/doc/userguide");
run_cmd("cp -a doc/installguide/install.pdf ${expdir}/doc/installguide");
run_cmd("cp -a doc/smpd/smpd_pmi.pdf ${expdir}/doc/smpd");
run_cmd("cp -a doc/logging/logging.pdf ${expdir}/doc/logging");
run_cmd("cp -a doc/windev/windev.pdf ${expdir}/doc/windev");
run_cmd("rm -rf ${expdir}-tmp");
print("done\n");

print("===> Creating ROMIO docs... ");
chdir("${expdir}/src/mpi");
chdir("romio/doc");
run_cmd("make");
run_cmd("rm -f users-guide.blg users-guide.toc users-guide.aux users-guide.bbl users-guide.log users-guide.dvi");
print("done\n");

# Create the tarball
print("===> Creating the final ${pack} tarball... ");
chdir("${tdir}");
run_cmd("tar -czvf ${pack}-${version}.tar.gz ${pack}-${version}");
run_cmd("rm -rf ${expdir}");
run_cmd("cp -a ${pack}-${version}.tar.gz ${root}/");
print("done\n\n");

# make sure we are outside of the tempdir so that the CLEANUP logic can run
chdir("${tdir}/..");
