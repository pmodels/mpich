#!/usr/bin/perl
use strict;
use IO::Select;
use Socket;
use Time::HiRes qw(gettimeofday);
our %env_vars;
our %cmdline_vars;
our $cwd;
our %config;
our @alltests;
our $verbose;
sub dump_junit {
    my ($alltests, $junitfile) = @_;
    my $date = `date "+%Y-%m-%d-%H-%M"`;
    $date =~ s/\r?\n//;
    my $n = @$alltests;
    my ($err_count, $skip_count);
    foreach my $test (@$alltests){
        if($test->{skip}){
            $skip_count ++;
        }
        elsif($test->{found_error}){
            $err_count ++;
        }
    }
    open Out, ">$junitfile" or die "Can't write $junitfile.\n";
    print "  --> [$junitfile]\n";
    print Out "<testsuites>\n";
    print Out "  <testsuite failures=\"$err_count\"\n";
    print Out "             errors=\"0\"\n";
    print Out "             skipped=\"$skip_count\"\n";
    print Out "             tests=\"$n\"\n";
    print Out "             date=\"$date\"\n";
    print Out "             name=\"summary_junit_xml\">\n";
    my $i = -1;
    foreach my $test (@$alltests){
        $i++;
        my $name_short = "$i - $test->{dir}/$test->{prog} $test->{np}";
        my $name="$name_short $test->{arg} $test->{env}";
        if($test->{skip}){
            print Out "    <testcase name=\"$name\">\n";
            print Out "      <skipped type=\"TodoTestSkipped\"\n";
            print Out "             message=\"$test->{skip}\"><![CDATA[ok $name_short \x23 SKIP $test->{skip}]]></skipped>\n";
            print Out "    </testcase>\n";
        }
        elsif(!$test->{found_error}){
            print Out "    <testcase name=\"$name\" time=\"$test->{runtime}\"></testcase>\n";
        }
        else{
            my $testtag = "failure";
            if($test->{xfail}){
                $testtag = "skipped";
            }
            print Out "    <testcase name=\"$name\" time=\"$test->{runtime}\">\n";
            print Out "      <$testtag type=\"TestFailed\"\n";
            print Out "               message=\"not ok $name_short$test->{xfail}\"><![CDATA[\n";
            print Out "not ok $name_short$test->{xfail}\n";
            print Out "  ---\n";
            print Out "  Directory: $test->{dir}\n";
            print Out "  File: $test->{prog}\n";
            print Out "  Num-procs: $test->{np}\n";
            print Out "  Timeout: $test->{timeout}\n";
            my $t = localtime;
            print Out "  Date: \"$t\"\n";
            print Out "  Error: $test->{found_error}\n";
            print Out "  ...\n";
            print Out "\x23\x23 Test output (expected 'No Errors'):\n";
            foreach my $line (split m/\r?\n/, $test->{output}){
                chomp $line;
                print Out "\x23\x23 $line\n";
            }
            print Out "    ]]></$testtag>\n";
            print Out "    </testcase>\n";
        }
    }
    print Out "    <system-out></system-out>\n";
    print Out "    <system-err></system-err>\n";
    print Out "  </testsuite>\n";
    print Out "</testsuites>\n";
    close Out;
}

sub dump_tap {
    my ($alltests, $tapfile) = @_;
    my $n = @$alltests;
    open Out, ">$tapfile" or die "Can't write $tapfile.\n";
    print "  --> [$tapfile]\n";
    print Out "1..$n\n";
    my $i = -1;
    foreach my $test (@$alltests){
        $i++;
        if($test->{skip}){
            print Out "ok $i - $test->{dir}/$test->{prog} $test->{np}  \x23 SKIP $test->{skip}\n";
        }
        elsif(!$test->{found_error}){
            print Out "ok $i - $test->{dir}/$test->{prog} $test->{np} \x23 time=$test->{runtime}\n";
        }
        else{
            print Out "not ok $i - $test->{dir}/$test->{prog} $test->{np}$test->{xfail} \x23 time=$test->{runtime}\n";
            print Out "  ---\n";
            print Out "  Directory: $test->{dir}\n";
            print Out "  File: $test->{prog}\n";
            print Out "  Num-procs: $test->{np}\n";
            print Out "  Timeout: $test->{timeout}\n";
            my $t = localtime;
            print Out "  Date: \"$t\"\n";
            print Out "  Error: $test->{found_error}\n";
            print Out "  ...\n";
            print Out "\x23\x23 Test output (expected 'No Errors'):\n";
            foreach my $line (split m/\r?\n/, $test->{output}){
                chomp $line;
                print Out "\x23\x23 $line\n";
            }
        }
    }
    close Out;
}

sub dump_xml {
    my ($alltests, $xmlfile) = @_;
    my $date = `date "+%Y-%m-%d-%H-%M"`;
    $date =~ s/\r?\n//;
    open Out, ">$xmlfile" or die "Can't write $xmlfile.\n";
    print "  --> [$xmlfile]\n";
    print Out "<?xml version='1.0' ?>\n";
    print Out "<?xml-stylesheet href=\"TestResults.xsl\" type=\"text/xsl\" ?>\n";
    print Out "<MPITESTRESULTS>\n";
    print Out "<DATE>$date<\/DATE>\n";
    print Out "<MPISOURCE>$config{MPI_SOURCE}<\/MPISOURCE>\n";
    foreach my $test (@$alltests){
        print Out "<MPITEST>\n";
        print Out "<NAME>$test->{prog}<\/NAME>\n";
        print Out "<NP>$test->{np}<\/NP>\n";
        print Out "<WORKDIR>$test->{dir}<\/WORKDIR>\n";
        if($test->{skip}){
        }
        elsif(!$test->{found_error}){
            print Out "<STATUS>pass<\/STATUS>\n";
        }
        else{
            my $xout = $test->{output};
            $xout =~ s/</\*AMP\*lt;/g;
            $xout =~ s/>/\*AMP\*gt;/g;
            $xout =~ s/&/\*AMP\*amp;/g;
            $xout =~ s/\*AMP\*/&/g;
            print Out "<STATUS>fail<\/STATUS>\n";
            print Out "<TESTDIFF>$xout<\/TESTDIFF>\n";
        }
        print Out "<TIME>$test->{runtime}<\/TIME>\n";
        print Out "</MPITEST>\n";
    }
    if(!$config{noxmlclose}){
        print Out "</MPITESTRESULTS>\n";
    }
    close Out;
}

sub run_worker {
    my ($IN, $OUT) = @_;
    while(<$IN>){
        if(/run\s+(\d+)/){
            my $test = $alltests[$1];
            RunMPIProgram($test);
            print $OUT "found_error: $test->{found_error}\n";
            print $OUT "runtime: $test->{runtime}\n";
            print $OUT "output:\n";
            print $OUT $test->{output}, "\n";
            print $OUT "--\n";
        }
    }
}

sub LoadTests {
    my ($dir) = @_;
    my $srcdir = $config{srcdir};
    my @include_list=split /\s+/, $config{tests};
    my %loaded_listfile;
    while(my $f=shift @include_list){
        if(-d $f){
            LoadTests($f);
            next;
        }
        my $listfile;
        if(-e "$dir/$f"){
            $listfile = "$dir/$f";
        }
        elsif(-e "$srcdir/$dir/$f"){
            $listfile = "$srcdir/$dir/$f";
        }
        if(!$listfile){
            print "[$listfile] not found\n";
            next;
        }
        elsif($loaded_listfile{$f}){
            next;
        }
        $loaded_listfile{$f} = 1;
        if($verbose){
            print "Looking in $dir/$f\n";
        }
        my %macros;
        open In, "$listfile" or die "Can't open $listfile.\n";
        while(<In>){
            s/#.*//g;
            s/\r?\n//;
            s/^\s*//;
            if(/^\s*$/){
                next;
            }
            if(/\$\(\w/){
                $_ = expand_macro($_, \%macros);
            }
            if(/^set:\s*(\w+)\s*=\s*(.*)/){
                $macros{$1} = $2;
                next;
            }
            my $test;
            if(/^!(\S+):(\S+)/){
                system "cd $1 && make $2";
                next;
            }
            elsif(/^include\s+(\S+)/){
                push @include_list, $1;
                next;
            }
            elsif(/^(\S+)/ and -d "$dir/$1"){
                my $d = $1;
                if($config{include_dir} && !($d=~/$config{include_dir}/)){
                    next;
                }
                if($config{exclude_dir} && ($d=~/$config{exclude_dir}/)){
                    next;
                }
                push @include_list, "$dir/$d";
                next;
            }
            elsif($config{run_xfail_only} or $config{include_pattern} or $config{exclude_pattern}){
                if($config{run_xfail_only}){
                    if(!/xfail=/){
                        next;
                    }
                    else{
                        s/xfail=\S*//;
                    }
                }
                if($config{include_pattern}){
                    if(!(/$config{include_pattern}/)){
                        next;
                    }
                }
                if($config{exclude_pattern}){
                    if(/$config{exclude_pattern}/){
                        next;
                    }
                }
                $test = parse_testline($_);
            }
            else{
                $test = parse_testline($_);
            }
            $test->{dir} = $dir;
            $test->{line} = $_;
            push @alltests, $test;
            $test->{id}=@alltests;
        }
        close In;
    }
}

sub parse_testline {
    my ($line) = @_;
    my %test = (line=> $line);
    my @args = split(/\s+/,$line);
    my $programname = shift @args;
    my $np = shift @args;
    if(!$np){
        $np = $config{np_default};
    }
    if($config{np_max}>0 && $np > $config{np_max}){
        $np = $config{np_max};
    }
    $test{prog} = $programname;
    $test{np} = $np;
    foreach my $a (@args){
        if($a =~ /([^=]+)=(.*)/){
            my ($key, $value) = ($1, $2);
            if($key eq "env"){
                if($value=~/([^=]+)=(.*)/){
                    $test{env} .= " $value";
                }
                else{
                    warn "Environment value not in a=b form: $line";
                }
            }
            elsif($key=~/^(resultTest|init|timeLimit|arg|env|mpiexecarg|xfail|mpiversion|strict|mpix)$/){
                if(exists $test{$key}){
                    $test{$key}.=" $value";
                }
                else{
                    $test{$key}=$value;
                }
            }
            else{
                print STDERR "Unrecognized key $key in test line: $line\n";
            }
        }
        elsif($a eq "skip_id"){
            $test{skip_id} = 1;
        }
    }
    if(exists $test{xfail} && $test{xfail} eq ""){
        print STDERR "\"xfail=\" requires an argument\n";
    }
    if(filter_mpiversion($test{mpiversion})){
        $test{skip} = "requires MPI version $test{mpiversion}";
    }
    elsif(filter_strict($test{strict})){
        $test{skip} = "non-strict test, strict MPI mode requested";
    }
    elsif(filter_xfail($test{xfail})){
        $test{skip} = "xfail tests disabled: xfail=$test{xfail}";
    }
    elsif(filter_mpix($test{mpix})){
        $test{skip} = "tests MPIX extensions, MPIX testing disabled";
    }
    return \%test;
}

sub filter_mpix {
    my ($mpix_required) = @_;
    if (lc($mpix_required) eq "true" && !$config{run_mpix}) {
        return 1;
    }
    return 0;
}

sub filter_xfail {
    my ($xfail) = @_;
    if($config{run_strict}){
        return 0;
    }
    if($xfail && !$config{run_xfail}){
        return 1;
    }
    return 0;
}

sub filter_strict {
    my ($strict_ok) = @_;
    if(lc($strict_ok) eq "false" && $config{run_strict}){
        return 1;
    }
    return 0;
}

sub filter_mpiversion {
    my ($version_required) = @_;
    if(!$version_required){
        return 0;
    }
    if($config{MPIMajorVersion} eq "unknown" or $config{MPIMinorVersion} eq "unknown"){
        return 0;
    }
    my ($major, $minor) = split /\./, $version_required;
    if($major > $config{MPIMajorVersion}){
        return 1;
    }
    if($major == $config{MPIMajorVersion} && $minor > $config{MPIMinorVersion}){
        return 1;
    }
    return 0;
}

sub expand_macro {
    my ($line, $macros) = @_;
    my @paren_stack;
    my $segs=[];
    while(1){
        if($line=~/\G$/sgc){
            last;
        }
        elsif($line=~/\G\$\(/sgc){
            push @paren_stack, $segs;
            $segs=[];
            push @paren_stack, "\$\(";
        }
        elsif(!@paren_stack){
            if($line=~/\G([^\$]|\$(?![\(\.]))+/sgc){
                push @$segs, $&;
            }
        }
        else{
            if($line=~/\G\(/sgc){
                push @paren_stack, $segs;
                $segs=[];
                push @paren_stack, "(";
            }
            elsif($line=~/\G\)/sgc){
                my $t=join('', @$segs);
                my $open=pop @paren_stack;
                $segs=pop @paren_stack;
                if($open eq "(" or $t!~/^\w/){
                    push @$segs, "$open$t)";
                }
                else{
                    push @$segs, get_macro($t, $macros);
                }
            }
            elsif($line=~/\G([^\$\(\)]|\$(?![\(\.]))+/sgc){
                push @$segs, $&;
            }
        }
    }
    while(@paren_stack){
        my $t = join('', @$segs);
        my $open = pop @paren_stack;
        $segs = pop @paren_stack;
        push @$segs, $open;
        push @$segs, $t;
    }
    return join('', @$segs);
}

sub get_macro {
    my ($s, $macros) = @_;
    if($s=~/^(\w+):(.*)/){
        my $p=$2;
        my $t=get_macro_word($1, $macros);
        my @plist=split /,\s*/, $p;
        my $i=1;
        foreach my $pp (@plist){
            $t=~s/\$$i/$pp/g;
            $i++;
        }
        return $t;
    }
    elsif($s=~/^(\w+)/){
        return get_macro_word($1, $macros);
    }
}

sub get_macro_word {
    my ($name, $macros) = @_;
    return $macros->{$name};
}

sub RunMPIProgram {
    my ($test) = @_;
    if($test->{dir} && ! -x "$test->{dir}/$test->{prog}"){
        BuildMPIProgram($test);
    }
    if($test->{found_error}){
        return;
    }
    my $cmd = get_test_cmd($test);
    if($verbose){
        print "\n---- TEST $test->{id} ----\n";
        print "  [$test->{dir}] $cmd\n";
    }
    my %saveEnv;
    if($test->{env}){
        %saveEnv = %ENV;
        if($verbose){
            print "  ENV: $test->{env}\n";
        }
        foreach my $val (split /\s+/, $test->{env}){
            if($val =~ /([^=]+)=(.*)/){
                $ENV{$1} = $2;
            }
        }
    }
    my $t1 = gettimeofday();
    if($test->{dir} ne "."){
        $cmd = "cd $test->{dir} && $cmd";
    }
    open( my $MPIOUT, "$cmd 2>&1 |" ) || die "Could not run $test->{prog}\n";
    my ($found_noerror, $err_count, @output);
    my ($hydra_err);
    while(<$MPIOUT>){
        push @output, $_;
        if($verbose){
            print $_;
        }
        if(/FORTRAN STOP/){
            next;
        }
        elsif(/^\s*(No Errors|Test Passed)\s*$/i){
            $found_noerror += 1;
            next;
        }
        elsif(/(requesting checkpoint|checkpoint completed)\s*$/){
            next;
        }
        elsif(/application called MPI_Abort/){
            $hydra_err++;
        }
        else{
            $err_count++;
        }
    }
    my $rc = close( $MPIOUT );
    $test->{output} = join('', @output);
    my $t2 = gettimeofday();
    $test->{runtime} = $t2-$t1;
    if($test->{env}){
        %ENV = %saveEnv;
    }
    if(!$test->{resultTest}){
        if(!$found_noerror){
            $test->{found_error} = "Expect \"No Error\"";
        }
        elsif($err_count>0){
            $test->{found_error} = "Encounter unexpected output ($err_count counts)";
        }
        if(!$rc or $?){
            my $sig=$? &0x7f;
            my $status = $?>>8;
            if($sig){
                $test->{found_error}="Program exited with signal $sig";
            }
            elsif($status){
                $test->{found_error}="Program exited with status $status";
            }
            else{
                $test->{found_error}="Program exited with non-zero return code";
            }
        }
    }
    elsif($test->{resultTest} eq "TestStatusZero"){
        if(!$rc){
            $test->{found_error}="Expect zero return status";
        }
    }
    elsif($test->{resultTest} eq "TestStatus"){
        print "resultTest: $test->{resultTest}\n";
        if($err_count>0 && !$found_noerror){
            $test->{found_error} = "Unexpected output";
        }
        if($rc){
            $test->{found_error} = "Expect Non-Zero return status";
        }
    }
    elsif($test->{resultTest} eq "TestErrFatal"){
        if($rc){
            $test->{found_error} = "Expect Non-Zero return status";
        }
    }
    elsif($test->{resultTest} eq "TestStatusNoErrors"){
        if(!$found_noerror){
            $test->{found_error} = "Expect \"No Error\"";
        }
        elsif($err_count>0){
            $test->{found_error} = "Encounter unexpected output ($err_count counts)";
        }
        elsif($rc){
            $test->{found_error} = "Expect Non-Zero return status";
        }
    }
    if($verbose){
        if($test->{found_error}){
            print "Failed: [$test->{line}] - $test->{found_error}\n";
        }
        else{
            print "Success: [$test->{line}]\n";
        }
    }
    return $test->{found_error};
}

sub get_test_cmd {
    my ($test) = @_;
    my $cmd = "$config{mpiexec} $config{np_arg} $test->{np}";
    if($config{ppn_arg} && $config{ppn_max}>0){
        my $nn = $config{ppn_max};
        if($nn > $test->{np}){
            $nn = $test->{np};
        }
        my $arg = $config{ppn_arg};
        $arg=~s/\%d/$nn/;
        $cmd .= " $arg";
    }
    my $timeout = $config{timeout_default};
    if(defined($test->{timeLimit}) && $test->{timeLimit} =~ /^\d+$/){
        $timeout = $test->{timeLimit};
    }
    if($timeout){
        $timeout *= $config{timeout_multiplier};
        $test->{timeout} = $timeout;
        $test->{env}.=" MPIEXEC_TIMEOUT=$timeout";
    }
    if($test->{mpiexecarg}){
        $cmd.=" $test->{mpiexecarg}";
    }
    if($config{program_wrapper}){
        $cmd.=" $config{program_wrapper}";
    }
    if(-x "$test->{dir}/$test->{prog}"){
        $cmd.=" ./$test->{prog}";
    }
    else{
        $cmd.=" $test->{prog}";
    }
    if($test->{arg}){
        $cmd.=" $test->{arg}";
    }
    if($test->{id} and !$test->{skip_id}){
        $cmd .=  " -id=$test->{id}";
    }
    return $cmd;
}

sub BuildMPIProgram {
    my ($test) = @_;
    my $prog = $test->{prog};
    my $dir = $test->{dir};
    my $cmd = "make -C $dir $prog";
    if($verbose){
        print "  $cmd ...\n";
    }
    my $output = `$cmd 2>&1`;
    if(! -x "$dir/$prog"){
        if($config{verbose}){
            print "Failed to build $prog; $output\n";
        }
        $test->{output} = $output;
        $test->{found_error} = "Failed to build $prog";
    }
}

sub post_config {
    if($config{mpiversion}=~/(\d+)\.(\d+)/){
        $config{MPIMajorVersion} = $1;
        $config{MPIMinorVersion} = $2;
    }
    foreach my $k ("run_strict", "run_mpix", "run_xfail", "run_batch"){
        if($config{$k} && $config{$k} =~/^(no|false)$/i){
            $config{$k} = undef;
        }
    }
    $verbose = $config{verbose};
}

sub load_commandline {
    foreach my $a (@ARGV){
        if($a=~/^--?help/){
            print STDERR "runtests [-tests=testfile] [-np=nprocesses] [-maxnp=max-nprocesses] [-srcdir=location-of-tests] [-ppn=max-proc-per-node] [-ppnarg=string] [-timelimitarg=string] [-mpiversion=major.minor] [-xmlfile=filename ] [-tapfile=filename ] [-junitfile=filename ] [-noxmlclose] [-verbose] [-showprogress] [-batch] [-dir=execute_in_dir] [-help]\n";
            exit(1);
        }
        elsif($a=~/^--?dir=(.*)/){
            chdir $1 or die "Can't chdir $1\n";
        }
        elsif($a=~/^--?run=(.*)/){
            my $test = parse_testline($1);
            $test->{dir} = ".";
            $test->{line} = $1;
            push @alltests, $test;
        }
        elsif($a =~/^--?([\w\-]+)=(.*)/){
            my ($k, $v) = ($1, $2);
            if($cmdline_vars{$k}){
                $config{$cmdline_vars{$k}} = $v;
            }
            else{
                warn "Unrecognized command line option [$a]\n";
            }
        }
        elsif($a =~/^--?([\w\-]+)$/){
            my $k = $1;
            if($cmdline_vars{$k}){
                $config{$cmdline_vars{$k}} = 1;
            }
            else{
                warn "Unrecognized command line option [$a]\n";
            }
        }
        else{
            die "Unrecognized command line argument [$a]\n";
        }
    }
}

sub load_environment {
    while (my ($k, $v) = each %env_vars){
        if(defined $ENV{$k}){
            $config{$v} = $ENV{$k};
        }
    }
}

sub load_config {
    my $config_dir = ".";
    if ($0=~/(.*)\//){
        $config_dir = $1;
    }
    my $f = "$config_dir/runtests.config";
    if(-f $f){
        open In, "$f" or die "Can't open $f.\n";
        while(<In>){
            if(/^\s*(\w+)\s*=\s*(.+)/){
                $config{$1} = $2;
            }
        }
        close In;
    }
}

sub probe_max_cores {
    my $cpu_count = `grep -c -P '^processor\\s+:' /proc/cpuinfo`;
    if($cpu_count=~/^(\d+)/){
        return $1;
    }
    return 0;
}

%env_vars = (
    MPI_SOURCE => "MPI_SOURCE",
    MPITEST_MPIVERSION => "mpiversion",
    MPITEST_PPNARG => "ppn_arg",
    MPITEST_PPNMAX => "ppn_max",
    MPITEST_TIMEOUT => "timeout_default",
    MPITEST_TIMEOUT_MULTIPLIER => "timeout_multiplier",
    MPITEST_TIMELIMITARG => "timeout_arg",
    MPITEST_BATCH => "run_batch",
    MPITEST_BATCHDIR => "batchdir",
    MPITEST_STOPTEST => "stopfile",
    MPITEST_NUM_JOBS => "j",
    MPITEST_INCLUDE_DIR => "include_dir",
    MPITEST_EXCLUDE_DIR => "exclude_dir",
    MPITEST_INCLUDE_PATTERN => "include_pattern",
    MPITEST_EXCLUDE_PATTERN => "exclude_pattern",
    MPITEST_PROGRAM_WRAPPER => "program_wrapper",
    VERBOSE => "verbose",
    V => "verbose",
    RUNTESTS_VERBOSE => "verbose",
    RUNTESTS_SHOWPROGRESS => "show_progress",
    NOXMLCLOSE => "noxmlclose",
);
%cmdline_vars = (
    j => "j",
    srcdir => "srcdir",
    tests => "tests",
    mpiexec => "mpiexec",
    nparg => "np_arg",
    np => "np_default",
    maxnp => "np_max",
    ppnarg => "ppn_arg",
    ppn => "ppn_max",
    batch => "run_batch",
    batchdir => "batch_dir",
    timelimitarg => "timeout_arg",
    verbose => "verbose",
    showprogress => "show_progress",
    xmlfile => "xmlfile",
    tapfile => "tapfile",
    junitfile => "junitfile",
    noxmlclose => "noxmlclose",
    "include-pattern" => "include_pattern",
    "exclude-pattern" => "exclude_pattern",
    "include-dir" => "include_dir",
    "exclude-dir" => "exclude_dir",
);
$cwd = `pwd`;
chomp $cwd;
$config{tests} = "testlist";
$config{srcdir} = ".";
$config{mpiexec} = "mpiexec";
$config{j} = 16;
$config{np_arg} = "-n";
$config{np_default} = 2;
$config{np_max}     = -1;
$config{ppn_arg}  = '';
$config{ppn_max}  = -1;
$config{timeout_default} = 180;
$config{timeout_multiplier} = 1.0;
$config{run_batch} = 0;
$config{batch_dir} = ".";
$config{verbose} = 0;
$config{show_progress} = 0;
$config{"stopfile"} = "$cwd/.stoptest";
$config{clean_programs} = 1;
$config{MPIMajorVersion} = "unknown";
$config{MPIMinorVersion} = "unknown";
$config{exclude_pattern} = 'type=MPI_(?!INT |DOUBLE )';
if(!$ENV{DTP_NUM_OBJS}){
    $ENV{DTP_NUM_OBJS} = 5;
}
$config{j} = probe_max_cores() -1;
load_config();
load_environment();
load_commandline();
post_config();
my $test = {prog=>"date", np=>1, resultTest=>"TestStatusZero"};
if(RunMPIProgram($test)){
    print "$test->{found_error}\n";
    die "Can't run mpiexec [$config{mpiexec}]!\n";
}
else{
    print "Looking good.\n";
}
if(!@alltests){
    LoadTests(".");
    my $n_tests = @alltests;
    print "$n_tests tests loaded\n";
    my $n_skip;
    foreach my $t (@alltests){
        if($t->{skip}){
            $n_skip++;
        }
    }
    if($n_skip){
        print "$n_skip tests skipped\n";
    }
}
print "Building test programs...\n";
my @dirs;
my %dirs;
my %dir_test_count;
foreach my $test (@alltests){
    my $d = $test->{dir};
    if(!$dirs{$d}){
        push @dirs, $d;
        $dirs{$d} = {};
    }
    $dirs{$d}->{$test->{prog}} ++;
    $dir_test_count{$d}++;
}
foreach my $d (@dirs){
    my @prog_list = sort keys %{$dirs{$d}};
    my $n = @prog_list;
    print "  $d $n programs - $dir_test_count{$d} tests\n";
    my $make="make";
    if($d ne "."){
        $make.=" -C $d";
    }
    if($config{j}){
        $make.=" -j $config{j}";
    }
    my $t = join ' ', @prog_list;
    `$make clean 2>&1`;
    if($config{verbose}){
        print "    $make $t...\n";
    }
    `$make $t 2>&1`;
}
print "Running tests...\n";
my $time_start=time;
my $time_last=time;
if($config{j}<=1){
    my $i = -1;
    foreach my $test (@alltests){
        $i++;
        RunMPIProgram($test);
        my $time_now = time;
        if($time_now-$time_last>10){
            $time_last = $time_now;
            my $t = $time_now-$time_start;
            if($t>=3600){
                $t=sprintf "%02d:%02d:%02d", $t/3600, $t/60%60, $t%60;
            }
            else{
                $t=sprintf "   %02d:%02d", $t/60, $t%60;
            }
            printf "  [$t] test #%5d: $test->{dir} - $test->{line}\n", $test->{id};
        }
    }
}
else{
    print "  maximum jobs: $config{j}\n";
    my $N_j = $config{j};
    my @worker_load;
    my @worker_pid;
    my @worker_socket;
    my $io_select = IO::Select->new();
    for (my $i = 0; $i<$N_j; $i++) {
        socketpair(my $child, my $parent, AF_UNIX, SOCK_STREAM, PF_UNSPEC) or die "socketpair\n";
        select(((select($child), $|=1))[0]);
        select(((select($parent), $|=1))[0]);
        my $pid = fork();
        if($pid){
            close $parent;
            push @worker_load, 0;
            push @worker_pid, $pid;
            push @worker_socket, $child;
            $io_select->add($child);
        }
        else{
            close $child;
            run_worker($parent, $parent);
            exit;
        }
    }
    my $i_test = 0;
    my $n = @alltests;
    my $total_load = 0;
    while(1){
        my $ran_test = 0;
        if($total_load < $N_j){
            while($i_test < $n and ($alltests[$i_test]->{skip} or $alltests[$i_test]->{ran})){
                $i_test++;
            }
            for (my $j = $i_test; $j<$n; $j++) {
                my $test=$alltests[$j];
                if($test->{skip} or $test->{ran}){
                    next;
                }
                my $np = $test->{np};
                if($total_load+$np <= $N_j || $total_load==0){
                    for (my $k = 0; $k<$N_j; $k++) {
                        if(!$worker_load[$k]){
                            $worker_load[$k] = "$j - $np";
                            my $h = $worker_socket[$k];
                            print $h "run $j\n";
                            $test->{ran} = $k+1;
                            $total_load+=$np;
                            $ran_test = 1;
                            goto L1;
                        }
                    }
                }
            }
        }
        L1:
        if(!$ran_test){
            my @ready = $io_select->can_read(1);
            foreach my $h (@ready){
                for (my $i = 0; $i<$N_j; $i++) {
                    if($h==$worker_socket[$i]){
                        if($worker_load[$i]=~/(\d+) - (\d+)/){
                            my ($j, $np) = ($1, $2);
                            my $test = $alltests[$j];
                            my $expect_output;
                            while(<$h>){
                                if(/^(found_error|runtime):\s*(.*)/){
                                    $test->{$1}=$2;
                                }
                                elsif(/^output:/){
                                    $expect_output = 1;
                                }
                                elsif(/^--$/){
                                    last;
                                }
                                elsif($expect_output){
                                    $test->{output}.=$_;
                                }
                            }
                            my $time_now = time;
                            if($time_now-$time_last>10){
                                $time_last = $time_now;
                                my $t = $time_now-$time_start;
                                if($t>=3600){
                                    $t=sprintf "%02d:%02d:%02d", $t/3600, $t/60%60, $t%60;
                                }
                                else{
                                    $t=sprintf "   %02d:%02d", $t/60, $t%60;
                                }
                                printf "  [$t] test #%5d: $test->{dir} - $test->{line}\n", $test->{id};
                            }
                            $worker_load[$i]=undef;
                            $total_load -= $np;
                        }
                        last;
                    }
                }
            }
        }
        if($i_test>=$n && $total_load==0){
            last;
        }
    }
}
if($config{xmlfile}){
    dump_xml(\@alltests, $config{xmlfile});
}
if($config{tapfile}){
    dump_tap(\@alltests, $config{tapfile});
}
if($config{junitfile}){
    dump_junit(\@alltests, $config{junitfile});
}
my $n = @alltests;
my $err_count = 0;
print "\n---- SUMMARY ----\n";
foreach my $test (@alltests){
    if($test->{found_error}){
        my $output = $test->{output};
        $output =~s/^\W+//;
        if(length($output)>20){
            $output = substr($output, 0, 20) ."...";
        }
        print "Failed: $test->{prog}: [$test->{found_error}] - $output\n";
        $err_count++;
    }
}
if($err_count){
    print "$err_count tests failed out of $n\n";
}
else{
    print " All $n tests passed!\n";
}
