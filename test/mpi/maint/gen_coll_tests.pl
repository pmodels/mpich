#!/usr/bin/perl
use strict;
our @coll_list = qw(barrier bcast gather gatherv scatter scatterv allgather allgatherv alltoall alltoallv alltoallw reduce allreduce reduce_scatter reduce_scatter_block scan exscan neighbor_allgather neighbor_allgatherv neighbor_alltoall neighbor_alltoallv neighbor_alltoallw);
our %coll_intra_tests;
our %coll_inter_tests;
our @test_list;
sub add_tests {
    my ($cvar, $enums) = @_;
    my ($tests, $coll, $intra);
    my @env_a;
    if($cvar=~/MPIR_CVAR_(\w+)_(INTRA|INTER)/){
        ($coll, $intra)=($1, $2);
        push @env_a, "env=MPIR_CVAR_$coll\_DEVICE_COLLECTIVE=0";
        if($coll=~/^I(\w+)/){
            $coll = $1;
            push @env_a, "env=MPIR_CVAR_$coll\_DEVICE_COLLECTIVE=0";
            push @env_a, "env=MPIR_CVAR_$coll\_$intra\_ALGORITHM=nb";
        }
        if($intra eq "INTRA"){
            $tests = $coll_intra_tests{lc($coll)};
        }
        else{
            $tests = $coll_inter_tests{lc($coll)};
        }
    }
    if(!@$enums){
        return;
    }
    foreach my $t (@$tests){
        foreach my $v (@$enums){
            my @env_b = ("env=$cvar=$v");
            if($v=~/gentran_tree/){
                add_tests_gentran_tree($coll, "$t @env_a @env_b");
            }
            elsif($v=~/gentran_ring/){
                add_tests_gentran_ring($coll, "$t @env_a @env_b");
            }
            elsif($v=~/gentran_brucks/){
                add_tests_gentran_brucks($coll, "$t @env_a @env_b");
            }
            elsif($v=~/gentran_recexch/){
                add_tests_gentran_recexch($coll, "$t @env_a @env_b");
            }
            elsif($v=~/gentran_scattered/){
                add_tests_gentran_scattered($coll, "$t @env_a @env_b");
            }
            else{
                push @test_list, "$t @env_a @env_b";
            }
        }
    }
}

sub add_tests_gentran_scattered {
    my ($coll, $t) = @_;
    foreach my $n1 (1,2,4){
        foreach my $n2 (4, 8){
            my @env;
            push @env, "env=MPIR_CVAR_I$coll\_SCATTERED_BATCH_SIZE=$n1";
            push @env, "env=MPIR_CVAR_I$coll\_SCATTERED_OUTSTANDING_TASKS=$n2";
            push @test_list, "$t @env";
        }
    }
}

sub add_tests_gentran_recexch {
    my ($coll, $t) = @_;
    foreach my $kval (2,3,4){
        my @env;
        push @env, "env=MPIR_CVAR_I$coll\_RECEXCH_KVAL=$kval";
        push @test_list, "$t @env";
    }
}

sub add_tests_gentran_brucks {
    my ($coll, $t) = @_;
    foreach my $kval (2,3,4){
        my @env;
        push @env, "env=MPIR_CVAR_I$coll\_BRUCKS_KVAL=$kval";
        if($coll=~/ALLTOALL/){
            push @test_list, "$t @env env=MPIR_CVAR_I$coll\_BRUCKS_BUFFER_PER_NBR=0";
            push @test_list, "$t @env env=MPIR_CVAR_I$coll\_BRUCKS_BUFFER_PER_NBR=1";
        }
        else{
            push @test_list, "$t @env";
        }
    }
}

sub add_tests_gentran_ring {
    my ($coll, $t) = @_;
    my @env;
    push @env, "env=MPIR_CVAR_I$coll\_RING_CHUNK_SIZE=4096";
    push @test_list, "$t @env";
}

sub add_tests_gentran_tree {
    my ($coll, $t) = @_;
    foreach my $tree (("kary","knomial_1","knomial_2")){
        foreach my $kval (2,3,4){
            my @env;
            push @env, "env=MPIR_CVAR_I$coll\_TREE_TYPE=$tree";
            push @env, "env=MPIR_CVAR_I$coll\_TREE_KVAL=$kval";
            push @test_list, "$t @env";
        }
    }
}

foreach my $coll (@coll_list){
    $coll_intra_tests{$coll}=[];
    $coll_inter_tests{$coll}=[];
}
my $list = $coll_intra_tests{bcast};
push @$list, "bcasttest 10", "bcastzerotype 5";
my $list = $coll_intra_tests{gather};
push @$list, "gather 4", "gather2 4";
my $list = $coll_intra_tests{scatter};
push @$list, "scattern 4", "scatter2 4", "scatter3 4";
my $list = $coll_intra_tests{allgather};
push @$list, "allgather2 10", "allgather3 10";
my $list = $coll_intra_tests{allgatherv};
push @$list, "allgatherv2 10", "allgatherv3 10", "allgatherv4 4 timeLimit=600";
my $list = $coll_intra_tests{alltoall};
push @$list, "alltoall1 8";
my $list = $coll_intra_tests{alltoallv};
push @$list, "alltoallv 8", "alltoallv0 10";
my $list = $coll_intra_tests{reduce};
push @$list, "reduce 5", "reduce 10", "red3 10", "red4 10";
my $list = $coll_intra_tests{allreduce};
push @$list, "allred 4 arg=100", "allred 7", "allredmany 4", "allred2 4", "allred3 10", "allred4 4", "allred5 5", "allred6 4", "allred6 7";
my $list = $coll_intra_tests{reduce_scatter};
push @$list, "redscat 4", "redscat 6", "redscat2 4", "redscat2 10", "redscat3 8";
my $list = $coll_intra_tests{reduce_scatter_block};
push @$list, "red_scat_block 4 mpiversion=2.2";
push @$list, "red_scat_block 5 mpiversion=2.2";
push @$list, "red_scat_block 8 mpiversion=2.2";
push @$list, "red_scat_block2 4 mpiversion=2.2";
push @$list, "red_scat_block2 5 mpiversion=2.2";
push @$list, "red_scat_block2 10 mpiversion=2.2";
push @$list, "redscatblk3 8 mpiversion=2.2";
push @$list, "redscatblk3 10 mpiversion=2.2";
my $list = $coll_intra_tests{scan};
push @$list, "scantst 4";
foreach my $a ("allgather", "allgatherv", "alltoall", "alltoallv", "alltoallw"){
    push @{$coll_intra_tests{"neighbor_$a"}}, "neighb_$a 4 mpiversion=3.0";
}
my $srcdir = ".";
my $mpich_dir = "../..";
foreach my $c (@coll_list){
    foreach my $coll ($c, "i$c"){
        my ($cvar, $enums);
        open In, "$mpich_dir/src/mpi/coll/$coll/$coll.c" or die "Can't open $mpich_dir/src/mpi/coll/$coll/$coll.c.\n";
        while(<In>){
            my $COLL = uc($coll);
            if(/-\s+name\s*:\s*(MPIR_CVAR_$COLL\_(INTRA|INTER)_ALGORITHM)/){
                $cvar = $1;
            }
            elsif($cvar && /^\s*description\s*:/){
                $enums = [];
            }
            elsif(/^\s*$/){
                if($cvar && $enums && @$enums){
                    add_tests($cvar, $enums);
                }
                undef $enums;
                undef $cvar;
            }
            elsif($enums  && /^\s*(\w+)\s*-/){
                my ($v) = ($1);
                if($v=~/^gentran_/){
                    push @$enums, $v;
                }
            }
        }
        close In;
    }
}
open Out, ">coll/testlist.cvar" or die "Can't write coll/testlist.cvar.\n";
print "  --> [coll/testlist.cvar]\n";
foreach my $l (@test_list){
    print Out "$l\n";
}
close Out;
