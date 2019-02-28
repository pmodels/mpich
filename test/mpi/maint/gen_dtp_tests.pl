#!/usr/bin/perl
use strict;

# parses   `dtp_tests.txt`
# produces `testlist.dtp` in each respective folder
# produces `dtpools/include/dtpools_types.h`

our @dtp_types;
our %dtp_types;
our @basic_typelist;
our @struct_typelist;
our @pair_typelist;
our $g_verbose;
our %dir_hash;
sub parse_struct_type {
    my ($t) = @_;
    my ($n, @types, @counts);
    foreach my $s (split /,/, $t){
        if($s=~/^(\w+):(\d+)/){
            $n++;
            push @types, $1;
            push @counts, $2;
        }
        elsif($s=~/^(\w+)$/){
            $n++;
            push @types, $1;
            push @counts, 1;
        }
        else{
            warn "dtp: error parsing struct type field: $t\n";
        }
    }
    return ($n, join(',',@types), join(',', @counts));
}

sub check_extra_type {
    my ($t) = @_;
    if(!$dtp_types{$t}){
        $dtp_types{$t}=1;
        push @dtp_types, $t;
    }
}

sub add_pair_default {
    push @pair_typelist, "MPI_INT,MPI_INT";
}

sub add_struct_default {
    push @struct_typelist, "MPI_CHAR:64,MPI_INT:32,MPI_INT:16,MPI_FLOAT:8";
}

sub add_types_default {
    push @basic_typelist, qw( MPI_INT MPI_DOUBLE );
}

push @dtp_types, qw(
    MPI_CHAR
    MPI_BYTE
    MPI_WCHAR
    MPI_SHORT
    MPI_INT
    MPI_LONG
    MPI_LONG_LONG_INT
    MPI_UNSIGNED_CHAR
    MPI_UNSIGNED_SHORT
    MPI_UNSIGNED
    MPI_UNSIGNED_LONG
    MPI_UNSIGNED_LONG_LONG
    MPI_FLOAT
    MPI_DOUBLE
    MPI_LONG_DOUBLE
    MPI_INT8_T
    MPI_INT16_T
    MPI_INT32_T
    MPI_INT64_T
    MPI_UINT8_T
    MPI_UINT16_T
    MPI_UINT32_T
    MPI_UINT64_T
    MPI_C_COMPLEX
    MPI_C_FLOAT_COMPLEX
    MPI_C_DOUBLE_COMPLEX
    MPI_C_LONG_DOUBLE_COMPLEX
    MPI_FLOAT_INT
    MPI_DOUBLE_INT
    MPI_LONG_INT
    MPI_2INT
    MPI_SHORT_INT
    MPI_LONG_DOUBLE_INT
);
foreach my $t (@dtp_types){
    $dtp_types{$t}=1;
}
my $srcdir = ".";
my $last_a;
foreach my $a (@ARGV){
    if($last_a eq "-type"){
        push @basic_typelist, split /[,\s]+/, $a;
    }
    elsif($last_a eq "-struct"){
        push @struct_typelist, $a;
    }
    elsif($last_a eq "-pair"){
        push @pair_typelist, $a;
    }
    elsif($a =~ /--?verbose/){
        $g_verbose = 1;
    }
    $last_a = $a;
}
if(!@basic_typelist){
    add_types_default();
}
if(!@struct_typelist){
    add_struct_default();
}
if(!@pair_typelist){
    add_pair_default();
}
foreach my $t (@basic_typelist){
    check_extra_type($t);
}
if($g_verbose){
    print "basic_typelist: @basic_typelist\n";
    print "struct_typelist: @struct_typelist\n";
    print "pair_typelist: @pair_typelist\n";
}
open In, "$srcdir/dtp_tests.txt" or die "Can't open $srcdir/dtp_tests.txt.\n";
while(<In>){
    chomp;
    if(/^([\w\/]+):.*/){
        my $path = $1;
        my @parts=split /:/;
        my $test_type;
        if(/:(STRUCT|PAIR):/){
            $test_type = $1;
        }
        elsif(/^(pt2pt)\//){
            $test_type = $1;
        }
        if($parts[0]=~/(.*)\/(.+)/){
            my ($dir, $name) = ($1, $2);
            if(!$dir_hash{"$dir-test"}){
                $dir_hash{"$dir-test"}=[];
            }
            my $out_test=$dir_hash{"$dir-test"};
            my (@macros, $got_USE_DTP);
            foreach my $m (split /\s+/, $parts[1]){
                push @macros, "-D$m";
                if($m=~/USE_DTP_POOL_TYPE__STRUCT/){
                    $got_USE_DTP = 1;
                }
            }
            if($g_verbose){
                print "Generate DTP tests for: $dir/$name ... \"\n";
            }
            if($test_type eq "STRUCT" || $test_type eq "PAIR"){
                if(!$got_USE_DTP){
                    push @macros,  "-DUSE_DTP_POOL_TYPE__STRUCT";
                }
                my ($type,$timelimit,$procs)=@parts[2,3,4];
                if($test_type eq "PAIR"){
                    foreach my $t (@pair_typelist){
                        my ($n, $types, $counts)=parse_struct_type($t);
                        push @$out_test, "$name $procs arg=-numtypes=$n arg=-types=$types arg=-counts=$counts $timelimit @macros";
                    }
                }
                elsif($test_type eq "STRUCT"){
                    foreach my $t (@struct_typelist){
                        my ($n, $types, $counts)=parse_struct_type($t);
                        push @$out_test, "$name $procs arg=-numtypes=$n arg=-types=$types arg=-counts=$counts $timelimit @macros";
                    }
                }
            }
            else{
                my ($counts,$timelimit,$procs)=@parts[2,3,4];
                my @counts = split /\s+/, $counts;
                my $n_counts = @counts;
                foreach my $type (@basic_typelist){
                    for (my $i = 0; $i<$n_counts; $i++) {
                        my $cnt = $counts[$i];
                        if($test_type eq "pt2pt"){
                            my $n = $i+2>$n_counts?$n_counts:$i+2;
                            for (my $i2 = $i; $i2<$n; $i2++) {
                                my $cnt2 = $counts[$i];
                                push @$out_test, "$name $procs arg=-type=$type arg=-sendcnt=$cnt arg=-recvcnt=$cnt2 $timelimit @macros";
                            }
                        }
                        else{
                            push @$out_test, "$name $procs arg=-type=$type arg=-count=$cnt $timelimit @macros";
                        }
                    }
                }
            }
        }
    }
}
close In;
while (my ($k, $v) = each %dir_hash){
    if($k=~/(.*)-test/){
        open Out, ">$srcdir/$1/testlist.dtp" or die "Can't write $srcdir/$1/testlist.dtp.\n";
        print "  --> [$srcdir/$1/testlist.dtp]\n";
        foreach my $l (@$v){
            print Out "$l\n";
        }
        close Out;
    }
}
open Out, ">$srcdir/dtpools/include/dtpools_types.h" or die "Can't write $srcdir/dtpools/include/dtpools_types.h.\n";
print "  --> [$srcdir/dtpools/include/dtpools_types.h]\n";
print Out "#ifndef DTPOOLS_TYPE_LIST\n";
print Out "#define DTPOOLS_TYPE_LIST \\\n";
foreach my $t (@dtp_types){
    print Out "    {\"$t\", $t}, \\\n";
}
print Out "    {\"MPI_DATATYPE_NULL\", MPI_DATATYPE_NULL}\n";
print Out "\n";
my $t = join(', ', @dtp_types);
print Out "#define DTP_MPI_DATATYPE $t\n";
print Out "\n";
print Out "#endif\n";
close Out;
