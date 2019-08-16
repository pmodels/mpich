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
my $i=0;
foreach my $f (@$src_list){
    my ($base_name) = $f=~/^.*\/(.+)\.c$/;
    my $testlist = gather_tests($f);

    foreach my $test (@$testlist){
        my $test_opts = $test->{opts};
        my ($title, $result);
        $cnt ++;
        if($test_opts->{TEST}){
            $title = $test_opts->{TEST};
        }
        else{
            $title = $base_name;
        }

        $i++;
        print "Test $i: $title ... ";

        if($test_opts->{skip}){
            $result = "SKIP";
            $n_skip++;
        }
        else{
            $result = run_test($test, $i);
        }
        print "$result\n";
    }
}

print "Ran $cnt tests, $n_ok OK, $n_fail FAIL, $n_xfail XFAIL, $n_skip SKIP\n";

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
        @src_list = glob("$dir/*.c");
    }
    return \@src_list;
}

sub gather_tests {
    my ($f) = @_;
    my @tests;
    my @common;
    my %opts = %g_opts;
    my $source = [];
    open In, "$f" or die "Can't open $f.\n";
    while(<In>){
        if(/^\/\*\s+(\w+):\s*(.+?)\s*\*\//){
            if($1 eq "TEST"){
                if(!@common){
                    @common = @$source;
                }
                else{
                    my $opt_list = opt_multiplex(\%opts);
                    foreach my $t_opt (@$opt_list){
                        push @tests, {opts=>$t_opt, source=>$source};
                        $source = [];
                        push @$source, @common;
                    }
                }
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
    foreach my $t_opt (@$opt_list){
        push @tests, {opts=>$t_opt, source=>$source};
        $source = [];
        push @$source, @common;
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
            $t_opts{CFLAGS} = $cflags;
            $t_opts{LDFLAGS} = $ldflags;
            push @all_opts, \%t_opts;
        }
    }
    return \@all_opts;
}

