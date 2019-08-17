#!/usr/bin/perl
use strict;

our %g_opts;


my $src_list = parse_options(\%g_opts);

if(-d ".test"){
    print "Removing old .test folder...\n";
    system "rm -rf .test";
}
mkdir ".test";

#-------------------------------------------
my ($cnt, $n_ok, $n_fail, $n_xfail, $n_skip) = (0, 0, 0, 0, 0);
my $alltests=[];
my $i=0;
foreach my $f (@$src_list){
    my $testlist = gather_tests($f);
    push @$alltests, @$testlist;
}

foreach my $test (@$alltests){
    my $test_opts = $test->{opts};
    my ($title, $result);
    $cnt ++;
    $i++;
    print "Test $i: $test->{title} ... ";

    if($test_opts->{SKIP}){
        $result = "SKIP";
        $n_skip++;
    }
    else{
        $result = run_test($test, $i);
    }
    $test->{result} = $result;
    print "$result\n";
}

print "Ran $cnt tests, $n_ok OK, $n_fail FAIL, $n_xfail XFAIL, $n_skip SKIP\n";
my $date = `date "+%Y-%m-%d-%H-%M"`;
$date =~ s/\r?\n//;

my $junitfile = "unit_test.xml";
open Out, ">$junitfile" or die "Can't write $junitfile.\n";
print "  --> [$junitfile]\n";
print Out "<testsuites>\n";
print Out "  <testsuite failures=\"$n_fail\"\n";
print Out "             skipped=\"$n_skip\"\n";
print Out "             tests=\"$cnt\"\n";
print Out "             date=\"$date\"\n";
print Out "             name=\"unit_test\">\n";
my $i = -1;
foreach my $test (@$alltests){
    $i++;
    my $name= "$i - $test->{title}";
    if($test->{result} eq "SKIP"){
        print Out "    <testcase name=\"$name\">\n";
        my $msg = $test->{opts}->{SKIP};
        print Out "      <skipped type=\"TodoTestSkipped\"\n";
        print Out "             message=\"$msg\"></skipped>\n";
        print Out "    </testcase>\n";
    }
    elsif($test->{result} eq "OK"){
        print Out "    <testcase name=\"$name\" ></testcase>\n";
    }
    else{
        print Out "    <testcase name=\"$name\" >\n";
        print Out "      <failure type=\"TestFailed\"\n";
        print Out "               message=\"$test->{result}: $name\">\n";
        print Out "    </failure>\n";
        print Out "    </testcase>\n";
    }
}
print Out "    <system-out></system-out>\n";
print Out "    <system-err></system-err>\n";
print Out "  </testsuite>\n";
print Out "</testsuites>\n";

close Out;

# ---- subroutines --------------------------------------------
sub parse_options {
    my ($g_opts) = @_;
    my @src_list;

    foreach my $a (@ARGV){
        if($a=~/^(\w+)=(.*)/){
            $g_opts->{uc($1)}=$2;
        }
        elsif($a=~/^(.*\.c)$/){
            push @src_list, "../$a";
        }
    }

    if(!@src_list){
        my $dir = "test";
        if($g_opts->{SRCDIR}){
            $dir = "$g_opts->{SRCDIR}";
        }
        open In, "find $dir -name '*.c' |" or die "Can't open find $dir -name '*.c' |.\n";
        while(<In>){
            chomp;
            push @src_list, $_;
        }
        close In;
    }
    return \@src_list;
}

sub gather_tests {
    my ($f) = @_;
    my @tests;
    my @common;
    my %opts = %g_opts;
    my $source = [];
    my ($title) = $f=~/^(.+)\.c$/;
    open In, "$f" or die "Can't open $f.\n";
    while(<In>){
        if(/^\/\*\s+(\w+):\s*(.+?)\s*\*\//){
            if($1 eq "TEST"){
                if(!@common){
                    @common = @$source;
                }
                else{
                    my $opt_list = opt_multiplex(\%opts);
                    if(@$opt_list == 1){
                        push @tests, {title=> $title, opts=>$opt_list->[0], source=>$source};
                    }
                    else{
                        foreach my $t_opt (@$opt_list){
                            push @tests, {title=> "$title - $t_opt->{subtitle}", opts=>$t_opt, source=>$source};
                        }
                    }
                    $source = [];
                    push @$source, @common;
                }
                $title = $2;
            }
            $opts{$1}=$2;
        }
        elsif(/^int\s+(main|test_\w+)\(\s*\)\s*$/){
            push @$source, "int main()\n";
        }
        else{
            push @$source, $_;
        }
    }
    close In;
    my $opt_list = opt_multiplex(\%opts);
    if(@$opt_list == 1){
        push @tests, {title=> $title, opts=>$opt_list->[0], source=>$source};
    }
    else{
        foreach my $t_opt (@$opt_list){
            push @tests, {title=> "$title - $t_opt->{subtitle}", opts=>$t_opt, source=>$source};
        }
    }

    return \@tests;
}

sub run_test {
    my ($test, $i) = @_;
    my $name = "test-$i";
    my $test_opts = $test->{opts};
    open Out, ">.test/$name.c" or die "Can't write .test/$name.c.\n";
    foreach my $l (@{$test->{source}}){
        print Out $l;
    }
    close Out;
    my $cc = $test_opts->{CC};
    if(!$cc){
        $cc = "gcc";
    }
    my $extra;

    if($test_opts->{CFLAGS}){
        $cc .= " $test_opts->{CFLAGS}";
    }

    if($test_opts->{SOURCE}){
        $extra .= " $test_opts->{SOURCE}";
    }

    if($test_opts->{LDFLAGS}){
        $extra .= " $test_opts->{LDFLAGS}";
        if($test_opts->{LDFLAGS} =~ /\.la\b/){
            $cc = "../libtool --silent --mode=link $cc";
        }
    }

    my $cmd = "$cc -o .test/$name .test/$name.c $extra && .test/$name";

    my $result;
    if($test_opts->{DEBUG}){
        print "$cmd\n";
    }
    system $cmd;

    if($? == -1){
        $n_fail ++;
        $result = "FAIL";
        $result .= " - failing command: $cmd";
    }
    elsif($? & 0xff){
        $n_fail ++;
        $result = "KILLED";
        $result .= " - failing command: $cmd";
    }
    else{
        my $ret = $?>>8;
        if($ret == 0){
            $result = "OK";
            $n_ok++;
        }
        elsif($test_opts->{XFAIL}){
            $n_xfail++;
            $result = "FAIL (expected)";
        }
        else{
            $n_fail++;
            $result = "FAIL ($ret)";
            $result .= " - failing command: $cmd";
        }
    }
    return $result;
}

sub opt_multiplex {
    my ($opts) = @_;
    my (@all_opts, @CFLAGS_list, @LDFLAGS_list);
    if($opts->{CFLAGS}){
        @CFLAGS_list = split /\s*\|\s*/, $opts->{CFLAGS};
    }
    else{
        @CFLAGS_list = (undef);
    }
    if($opts->{LDFLAGS}){
        @LDFLAGS_list = split /\s*\|\s*/, $opts->{LDFLAGS};
    }
    else{
        @LDFLAGS_list = (undef);
    }
    foreach my $cflags (@CFLAGS_list){
        foreach my $ldflags (@LDFLAGS_list){
            my %t_opts = %$opts;
            my $subtitle = "[$cflags $ldflags]";
            $t_opts{CFLAGS} = $cflags;
            $t_opts{LDFLAGS} = $ldflags;
            $t_opts{subtitle} = $subtitle;
            push @all_opts, \%t_opts;
        }
    }
    return \@all_opts;
}

