#!/usr/bin/perl
use strict;

our $verbose = "VERBOSE";
our (@dirs, @files);
our %opts;

parse_args();
if($opts{verbose} eq "terse" or $opts{terse}){
    $verbose = "TERSE";
}

if (!@files) {
    foreach my $dir (@dirs) {
        get_files($dir);
    }
}

my $n = @files;
print "processing $n files\n";
my $count = 0;
foreach my $f (@files){
    my @lines;
    my @temp;
    my ($got_function, $got_return, $ID);
    open In, "$f" or die "Can't open $f: $!\n";
    my $i=0;
    while(<In>){
        if($got_function && /^[^#\s{]/){
            if ($ID && !$got_return && /^}/) {
                push @lines, "    MPIR_FUNC_${verbose}_EXIT($ID);\n";
                $i+=1;
            }
            undef $got_function;
            undef $got_return;
            undef $ID;
        }

        if(/^[^#\/\s].*\s+(\w+)\s*\(.*\)\s*$/){
            $got_function = $1;
        }
        elsif(/^[^#\/\s].*\s+(\w+)\s*\(.*$/){
            $got_function = $1;
        }

        if(/^{/ && $got_function){
            my $l = ["{\n"];
            $ID = uc($got_function);
            push @$l, "    MPIR_FUNC_${verbose}_STATE_DECL($ID);\n";
            push @$l, "    MPIR_FUNC_${verbose}_ENTER($ID);\n";
            push @lines, $l;
            $i+=3;
            $count++;
        }
        elsif($ID && /^(\s+)return/ && $got_function && !/\\$/){
            my $sp = $1;
            my $l = [$sp."MPIR_FUNC_${verbose}_EXIT($ID);\n", $_];
            if ($lines[-1] =~ /^\s*(if|else)/ && $lines[-1] !~ /[\{;]$/) {
                my $n = @lines;
                warn "$f:$n missing brace\n";
            }
            $i+=2;
            push @lines, $l;
            $got_return++;
        }
        else{
            $i++;
            push @lines, $_;
        }
    }
    close In;

    if(@lines){
        open Out, ">$f" or die "Can't write $f: $!\n";
        foreach my $l (@lines){
            if(ref($l) eq "ARRAY"){
                foreach my $ll (@$l){
                    print Out $ll;
                }
            }
            else{
                print Out $l;
            }
        }
        close Out;
    }
}

print "Added tracing logs to $count functions.\n";

# ---- subroutines --------------------------------------------
sub parse_args {
    my @un_recognized;
    foreach my $a (@ARGV){
        if($a=~/^-(\w+)=(.*)/){
            $opts{$1} = $2;
        }
        elsif($a=~/^-(\w+)/){
            $opts{$1} = 1;
        }
        elsif(-d $a){
            push @dirs, $a;
        }
        elsif(-f $a){
            push @files, $a;
        }
        else{
            push @un_recognized, $a;
        }
    }
    if(@un_recognized){
        usage();
        die "Unrecognized options: @un_recognized\n";
    }
    if(!@files and !@dirs) {
        if (-d "./src/mpi") {
            push @dirs, "src/mpi", "src/mpid", "src/util";
        }
        else {
            push @dirs, ".";
        }
    }
}

sub usage {
    print "Usage: $0 [-terse] [dirs]\n";
}

sub get_files {
    my $dir = shift;
    open In, "find $dir -name '*.[ch]' |" or die "Can't run find |: $!\n";
    while(<In>){
        chomp;
        if (/\/(romio|ucx\/ucx|ofi\/libfabric)\//) {
            next;
        }
        push @files, $_;
    }
    close In;
}
