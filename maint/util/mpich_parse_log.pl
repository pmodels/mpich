#!/usr/bin/perl
use strict;

our $binary_path;
our @nm_list;
our @addr_list;


my $log_file = $ARGV[0];

my ($addr_base, $addr_ext);
open In, "$log_file" or die "Can't open $log_file.\n";
while(<In>){
    if(/mpl_dbg_func.c\s+\d+\s+([0-9a-f]+)-([0-9a-f]+).*\s+(\/\S+)/){
        ($addr_base, $addr_ext, $binary_path) = (hex($1), hex($2), $3);
        last;
    }
}
close In;
print "$addr_base - $addr_ext - $binary_path\n";

get_nm_list($binary_path);

my $level = 0;
open In, "$log_file" or die "Can't open $log_file.\n";
while(<In>){
    if(/(.*)\[ADDR:0x(.+)\]\s+(Entering|Leaving)\s+\[FUNCTION:0x(.+)\]/){
        my ($pre, $action) = ($1, $3);
        my $addr = hex($2) - $addr_base;
        my $func = hex($4) - $addr_base;
        my $func_name = get_name($func);
        if($action eq "Entering"){
            print $1, '. ' x $level, "-> $func_name\n";
            $level++;
        }
        if($action eq "Leaving" and $level>0){
            $level--;
            print $1, '. ' x $level, "<- $func_name\n";
        }
    }
    else{
        print;


    }
}
close In;

# ---- subroutines --------------------------------------------
sub get_nm_list {
    my ($binary_path) = @_;
    @addr_list = ();
    @nm_list = ();
    open In, "nm --numeric-sort --defined-only $binary_path |" or die "Can't open nm --numeric-sort --defined-only $binary_path |.\n";
    while(<In>){
        if(/^([0-9a-f]+)\s+[tT]\s+(\S+)/){
            push @addr_list, hex($1);
            push @nm_list, $2;
        }
    }
    close In;
}

sub get_name {
    my ($addr) = @_;
    if($addr < $addr_list[0]){
        return "(nil)";
    }

    my $i1 = 0;
    my $i2 = $#addr_list;

    while($i1 <= $i2){
        my $i3 = ($i1 + $i2) / 2;
        if($addr < $addr_list[$i3]){
            $i2 = $i3 - 1;
        }
        else{
            $i1 = $i3 + 1;
        }
    }

    if($i2 >= 0){
        return $nm_list[$i2];
    }
    return "(nil)";
}

