my ($branch, $from_commit, $to_commit) = @ARGV;
if (!$from_commit or $branch !~ /^(aurora|aurora_test)$/) {
    die "Usage: perl aurora_update.pl (aurora|aurora_test) from_commit to_commit\n";
}

if (!$to_commit) {
    if ($branch eq "aurora") {
        die "Missing explicit to_commit\n";
    }
    # default to main HEAD
    $to_commit = substr(`git rev-parse main`, 0, 10);
}

my $cur_commit = substr(`git rev-parse $branch`, 0, 10);

# -- a list of commits that we should skip
my %skip_commits = (
    "053d58109c" => 1,
);

my @logs;
my $failed;

# -- grab commits
open In, "git log --oneline --no-merges --pretty=\"format:%h %as %s\" $from_commit..$to_commit |" or die "git log failed\n";
my @commits = <In>;
close In;

# -- switch branch and apply commits
if ($branch eq "aurora") {
    system("git checkout aurora")==0 or die "git switch aurora failed\n";
}

my $n_commits = @commits;
for (my $i=0; $i < $n_commits; $i++) {
    my $line = $commits[$n_commits - $i - 1];
    push @logs, $line;
    my $commit;
    if ($line =~ /^(\S+) /) {
        $commit = $1;
    }
    if ($skip_commits{$commit}) {
        next;
    }
    print "pick $i / $n_commits - $commit\n";
    if (system("git cherry-pick --allow-empty $commit") != 0) {
        print "cherry-pick failed\n";
        $failed = 1;
        last;
    }
}

# -- reset branch if failed
if ($failed) {
    system("git reset --hard $cur_commit")==0 or die "git reset --hard $cur_commit failed\n";
}

if ($branch eq "aurora") {
    system("git checkout aurora_test")==0 or die "git switch aurora_test failed\n";
}

# -- save log
my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
my $logfile = sprintf("%4d%02d%02d_$branch.log", $year + 1900, $mon + 1, $mday);

open LOG, "> update_logs/$logfile" or die "Can't open update_logs/$logfile\n";
print LOG "UPDATE $branch $from_commit $to_commit\n\n";
print LOG "    current $branch HEAD $cur_commit\n\n";
for my $l (@logs) {
    print LOG $l;
}
close LOG;

# -- commit log if success
if (!$failed) {
    system("git add -f update_logs/$logfile && git commit -m \"add update_logs/$logfile\"")==0 or die "Commit log file failed.\n";
}
