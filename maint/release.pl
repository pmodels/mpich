#!/usr/bin/env perl
#
# (C) 2008 by Argonne National Laboratory.
#     See COPYRIGHT in top-level directory.
#

use strict;
use warnings;

use Cwd qw( cwd getcwd realpath );
use Getopt::Long;
use File::Temp qw( tempdir );

my $arg = 0;
my $branch = "";
my $version = "";
my $append_commit_id;
my $root = cwd();
my $with_autoconf = "";
my $with_automake = "";
my $git_repo = "";

my $logfile = "release.log";

sub usage
{
    print "Usage: $0 [OPTIONS]\n\n";
    print "OPTIONS:\n";

    print "\t--git-repo           path to root of the git repository (required)\n";
    print "\t--branch             git branch to be packaged (required)\n";
    print "\t--version            tarball version (required)\n";
    print "\t--append-commit-id   append git commit description (optional)\n";

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
	print("\tERROR: $tool version mismatch ($req_ver) required\n\n");
	exit;
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
    "branch=s" => \$branch,
    "version=s" => \$version,
    "append-commit-id!" => \$append_commit_id,
    "with-autoconf" => \$with_autoconf,
    "with-automake" => \$with_automake,
    "git-repo=s" => \$git_repo,
    "help"     => \&usage,

    # old deprecated args, retained with usage() to help catch non-updated cron
    # jobs and other stale scripts/users
    "append-svnrev!" => sub {usage();},
) or die "unable to parse options, stopped";

if (scalar(@ARGV) != 0) {
    usage();
}

if (!$branch || !$version) {
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
check_autotools_version("automake", "1.15");
check_autotools_version("libtool", "2.4.6");
print("\n");


my $tdir = tempdir(CLEANUP => 1);
my $local_git_clone = "${tdir}/mpich-clone";


# clone git repo
print("===> Cloning git repo... ");
run_cmd("git clone ${git_repo} ${local_git_clone}");
print("done\n");

# chdirs to $local_git_clone if valid
check_git_repo($local_git_clone);
print("\n");

my $current_ver = `git show ${branch}:maint/version.m4 | grep MPICH_VERSION_m4 | \
                   sed -e 's/^.*\\[MPICH_VERSION_m4\\],\\[\\(.*\\)\\].*/\\1/g'`;
if ("$current_ver" ne "$version\n") {
    print("\tWARNING: Version mismatch\n\n");
}

if ($append_commit_id) {
    my $desc = `git describe --always ${branch}`;
    chomp $desc;
    $version .= "-${desc}";
}

my $expdir = "${tdir}/mpich-${version}";

# Clean up the log file
system("rm -f ${root}/$logfile");

# Check out the appropriate branch
print("===> Exporting code from git... ");
run_cmd("rm -rf ${expdir}");
run_cmd("mkdir -p ${expdir}");
run_cmd("git archive ${branch} --prefix='mpich-${version}/' | tar -x -C $tdir");
print("done\n");

print("===> Create release date and version information... ");
chdir($expdir);

my $date = `date`;
chomp $date;
system(qq(perl -p -i -e 's/\\[MPICH_RELEASE_DATE_m4\\],\\[unreleased development copy\\]/[MPICH_RELEASE_DATE_m4],[$date]/g' ./maint/version.m4));
# the main version.m4 file will be copied to hydra's version.m4, including the
# above modifications
print("done\n");

# Remove content that is not being released
print("===> Removing content that is not being released... ");
chdir($expdir);

chdir("${expdir}/src/mpid/ch3/channels/nemesis/netmod");
my @nem_modules = qw(elan);
run_cmd("rm -rf ".join(' ', @nem_modules));
for my $module (@nem_modules) {
    run_cmd("rm -rf $module");
    run_cmd(q{perl -p -i -e '$_="" if m|^\s*include \$.*netmod/}.${module}.q{/Makefile.mk|' Makefile.mk});
}
print("done\n");

# Create configure
print("===> Creating configure in the main codebase... ");
chdir($expdir);
{
    my $cmd = "./autogen.sh";
    $cmd .= " --with-autoconf=$with_autoconf" if $with_autoconf;
    $cmd .= " --with-automake=$with_automake" if $with_automake;
    run_cmd($cmd);
}
print("done\n");

# Disable unnecessary tests in the release tarball
print("===> Disabling unnecessary tests in the main codebase... ");
chdir($expdir);
run_cmd(q{perl -p -i -e 's/^\@perfdir\@/#\@perfdir@/' test/mpi/testlist.in});
run_cmd(q{perl -p -i -e 's/^\@ftdir\@/#\@ftdir@/' test/mpi/testlist.in});
run_cmd("perl -p -i -e 's/^large_message /#large_message /' test/mpi/pt2pt/testlist");
run_cmd("perl -p -i -e 's/^large-count /#large-count /' test/mpi/datatype/testlist");
print("done\n");

# Remove unnecessary files
print("===> Removing unnecessary files in the main codebase... ");
chdir($expdir);
run_cmd("rm -rf README.vin maint/config.log maint/config.status unusederr.txt");
run_cmd("find . -name autom4te.cache | xargs rm -rf");
print("done\n");

# Get docs
print("===> Creating secondary codebase for the docs... ");
run_cmd("mkdir ${expdir}-build");
chdir("${expdir}-build");
run_cmd("${expdir}/configure --disable-fortran --disable-cxx");
run_cmd("(make mandoc && make htmldoc && make latexdoc)");
print("done\n");

print("===> Copying docs over... ");
run_cmd("cp -a man ${expdir}");
run_cmd("cp -a www ${expdir}");
run_cmd("cp -a doc/userguide/user.pdf ${expdir}/doc/userguide");
run_cmd("cp -a doc/installguide/install.pdf ${expdir}/doc/installguide");
run_cmd("cp -a doc/logging/logging.pdf ${expdir}/doc/logging");
print("done\n");

print("===> Creating ROMIO docs... ");
chdir("${expdir}/src/mpi");
chdir("romio/doc");
run_cmd("make");
run_cmd("rm -f users-guide.blg users-guide.toc users-guide.aux users-guide.bbl users-guide.log users-guide.dvi");
print("done\n");

# Create the main tarball
print("===> Creating the final mpich tarball... ");
chdir("${tdir}");
run_cmd("tar -czvf mpich-${version}.tar.gz mpich-${version}");
run_cmd("cp -a mpich-${version}.tar.gz ${root}/");
print("done\n");

# Create the hydra tarball
print("===> Creating the final hydra tarball... ");
run_cmd("cp -a ${expdir}/src/pm/hydra hydra-${version}");
run_cmd("tar -czvf hydra-${version}.tar.gz hydra-${version}");
run_cmd("cp -a hydra-${version}.tar.gz ${root}/");
print("done\n\n");

# make sure we are outside of the tempdir so that the CLEANUP logic can run
chdir("${tdir}/..");
