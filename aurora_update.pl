my $dry_run = 0;

open In, "last_main" or die "Can't read last_main\n";
my $last_main = <In>;
close In;
chomp $last_main;
print "  last_main: $last_main\n";

open In, "git log --oneline --no-merges --pretty=format:%h $last_main..main |" or die "git log failed\n";
my @commits = <In>;
close In;

my $n_commits = @commits;
for (my $i=0; $i < $n_commits; $i++) {
    my $commit = $commits[$n_commits - $i - 1];
    chomp $commit;
    print "pick $i / $n_commits - $commit\n";
    if (!$dry_run) {
        system("git cherry-pick --allow-empty $commit") == 0 or die "cherry-pick failed\n";
    }
}

my $main_head = substr(`git rev-parse main`, 0, 10);
print "      write: $main_head > last_main\n";
if (!$dry_run) {
    open Out, "> last_main" or die "Can't write last_main\n";
    print Out "$main_head\n";
    close Out;
} else {
    print "[This was a dry run]\n";
}
