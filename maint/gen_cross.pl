#!/usr/bin/perl
use strict;

my $method = "direct";
for my $a (@ARGV) {
    if ($a eq "-h") {
        print "  Run this script on the target system to generate a cross file for\n";
        print "  MPICH configure. Set compiler via CC and FC environment variables.\n";
        print "  You may also use the generated file as template and manually set\n";
        print "  the type sizes.\n\n";
        print "  Accepted options:\n\n";
        print "    -h\n";
        print "        Print this message\n\n";
        print "    -hybrid\n";
        print "        Link Fortran with C code to check sizes. May require compiler\n";
        print "        flags, e.g. setting FC='gfortran -fallow-argument-mismatch'.\n\n";
        print "    -direct\n";
        print "        Use Fortran SIZEOF. Most Fortran compilers support this.\n\n";
        print "    -asm\n";
        print "        Parse assembly output using heuristic. May work on build system.\n\n";
        exit 1;
    } elsif ($a eq "-hybrid") {
        $method = "hybrid";
    } elsif ($a eq "-direct") {
        $method = "direct";
    } elsif ($a eq "-asm") {
        $method = "asm";
    }
}

my $CC = $ENV{CC};
if(!$CC){
    $CC = "gcc";
}

my $FC = $ENV{FC};
if(!$FC){
    $FC = "gfortran";
}

print "CC: $CC\n";
print "FC: $FC\n";

# global hash to store crossfile values
our %cross_hash;

my @c_types=("void *", "char", "short", "int", "long", "long long", "size_t", "off_t", "float", "double", "long double");
get_c_sizes_direct(\@c_types);

# tuple list of [ range, kind, size ]
my $range_kind_list = get_f90_kinds_direct();
my @f77_types=qw(INTEGER REAL DOUBLE_PRECISION LOGICAL);
my $i = -1;
foreach my $t (@$range_kind_list){
    $i++;
    push @f77_types, "KIND-$i-$t->[1]";
}

if ($method eq "hybrid") {
    get_f77_sizes_hybrid(\@f77_types);
} elsif ($method eq "direct") {
    get_f77_sizes_direct(\@f77_types);
} elsif ($method eq "asm") {
    get_f77_sizes_asm(\@f77_types);
}

get_f77_true_false_hybrid();

my @f90_names=qw(ADDRESS_KIND OFFSET_KIND INTEGER_KIND REAL_MODEL DOUBLE_MODEL INTEGER_MODEL);
get_f90_names_direct(\@f90_names);

open Out, ">cross_values.txt" or die "Can't write cross_values.txt.\n";
print "  --> [cross_values.txt]\n";
foreach my $t (@c_types){
    my $name = get_type_name($t);
    my $val = $cross_hash{$name};
    my $var = "ac_cv_sizeof_$name";
    print Out "$var=$val\n";
}

foreach my $T (@f77_types){
    my $size = $cross_hash{$T};
    if($T=~/KIND-(\d+)-(\d+)/){
        $range_kind_list->[$1]->[2]=$size;
    }
    else{
        print Out "CROSS_F77_SIZEOF_$T=$size\n";
    }
}

foreach my $t ("TRUE_VALUE", "FALSE_VALUE"){
    print Out "CROSS_F77_$t=$cross_hash{$t}\n";
}

foreach my $name (@f90_names){
    print Out "CROSS_F90_$name=$cross_hash{$name}\n";
}

my (@t1, @t2);
foreach my $t (@$range_kind_list){
    push @t1, "$t->[0], $t->[1]";
    push @t2, "{ $t->[0] , $t->[1] , $t->[2] }";
}
my $t1 = join(', ', @t1);
my $t2 = join(', ', @t2);
print Out "CROSS_F90_ALL_INTEGER_MODELS=\"$t1\"\n";
print Out "CROSS_F90_INTEGER_MODEL_MAP=\"$t2\"\n";
close Out;

system "rm -f t.f t.f90 t.s t t.mod t.o tlib.o";
system "cat cross_values.txt";
print "\n";
print "Crossfile generated in cross_values.txt.\n";
print "Pass to MPICH configure with --with-cross=path/to/cross_values.txt.\n";

# ---- subroutines --------------------------------------------
sub get_c_sizes_direct {
    my ($typelist) = @_;
    open Out, ">t.c" or die "Can't write t.c.\n";
    print Out "#include <stdio.h>\n";
    print Out "int main(){\n";
    my $i = -1;
    foreach my $type (@$typelist){
        $i++;
        print Out "    printf(\"A$i: %lu\\n\", sizeof($type));\n";
    }
    print Out "    return 0;\n";
    print Out "}\n";
    close Out;
    my $t=`$CC t.c -o t && ./t`;
    while($t=~/A(\d+):\s+(\d+)/g){
        my $name = get_type_name($typelist->[$1]);
        $cross_hash{$name} = $2;
    }
}

sub get_c_sizes_asm {
    my ($typelist) = @_;
    open Out, ">t.c" or die "Can't write t.c.\n";
    my $i = -1;
    foreach my $type (@$typelist){
        $i++;
        print Out "$type A$i;\n";
    }
    close Out;
    my $t=`$CC -S t.c -o t.s`;
    open In, "t.s" or die "Can't open t.s.\n";
    while(<In>){
        if(/.comm\s+A(\d+),(\d+),(\d+)/){
            my $name = get_type_name($typelist->[$1]);
            $cross_hash{$name} = $2;
        }
    }
    close In;
}

sub get_f77_sizes_direct {
    my ($typelist) = @_;
    my $sp = ' 'x6;
    my $sp = ' 'x6;
    open Out, ">t.f" or die "Can't write t.f.\n";
    print Out "$sp PROGRAM t\n";
    my $i = -1;
    foreach my $T (@$typelist){
        $i++;
        if($T=~/KIND-(\d+)-(\d+)/){
            print Out "$sp INTEGER (KIND=$2) :: A$i\n";
        }
        else{
            my $t = $T;
            $t=~s/_/ /g;
            print Out "$sp $t :: A$i\n";
        }
    }
    print Out "\n";
    my $i = -1;
    foreach my $T (@$typelist){
        $i++;
        print Out "$sp print *, 'A$i:', sizeof(A$i)\n";
    }
    print Out "$sp END\n";
    close Out;
    my $t=`$FC t.f -o t && ./t`;
    while($t=~/A(\d+):\s+(\d+)/g){
        my $name = $typelist->[$1];
        $cross_hash{$name}=$2;
    }
}

sub get_f77_sizes_asm {
    my ($typelist) = @_;
    my $sp;
    open Out, ">t.f90" or die "Can't write t.f90.\n";
    print "  --> [t.f90]\n";
    print Out "MODULE t\n";
    my $i = -1;
    foreach my $T (@$typelist){
        $i++;
        if($T=~/KIND-(\d+)-(\d+)/){
            print Out "$sp INTEGER (KIND=$2) :: A$i\n";
        }
        else{
            my $t = $T;
            $t=~s/_/ /g;
            print Out "$sp $t :: A$i\n";
        }
    }
    print Out "\n";
    print Out "END MODULE t\n";
    close Out;
    my $t=`$FC -S t.f90 -o t.s`;
    open In, "t.s" or die "Can't open t.s.\n";
    while(<In>){
        if(/.size\s+__t_MOD_a(\d+),\s*(\d+)/){
            my $name = $typelist->[$1];
            $cross_hash{$name}=$2;
        } elsif(/.zerofill\s+.*__t_MOD_a(\d+),\s*(\d+),\s*(\d+)/){
            my $name = $typelist->[$1];
            $cross_hash{$name}=$2;
        }
    }
    close In;
}

sub get_f77_sizes_hybrid {
    my ($typelist) = @_;
    open Out, ">t.c" or die "Can't write t.c.\n";
    print Out "int cisize_(char *, char *);\n";
    print Out "int cisize_(char *p1, char *p2){return (int)(p2-p1);}\n";
    close Out;
    system "$CC -c -o tlib.o t.c";
    my $sp = ' 'x6;
    my $sp = ' 'x6;
    open Out, ">t.f" or die "Can't write t.f.\n";
    print Out "$sp PROGRAM t\n";
    print Out "$sp INTEGER cisize\n";
    my $i = -1;
    foreach my $T (@$typelist){
        $i++;
        if($T=~/KIND-(\d+)-(\d+)/){
            print Out "$sp INTEGER (KIND=$2) :: A$i(2)\n";
        }
        else{
            my $t = $T;
            $t=~s/_/ /g;
            print Out "$sp $t :: A$i(2)\n";
        }
    }
    print Out "\n";
    my $i = -1;
    foreach my $T (@$typelist){
        $i++;
        print Out "$sp print *, 'A$i:', cisize(A$i(1), A$i(2))\n";
    }
    print Out "$sp END\n";
    close Out;
    my $t = `$FC t.f tlib.o -o t && ./t`;
    while($t=~/A(\d+):\s+(\d+)/g){
        my $name = $typelist->[$1];
        $cross_hash{$name}=$2;
    }
}

sub get_f77_true_false_hybrid {
    open Out, ">t.c" or die "Can't write t.c.\n";
    print Out "#include <stdio.h>\n";
    print Out "void ftest_(int *, int *);\n";
    print Out "void ftest_(int * t, int * f){printf(\"T:%d F:%d\\n\", *t, *f);}\n";
    close Out;
    system "$CC -c -o tlib.o t.c";
    my $sp = ' 'x6;
    my $sp = ' 'x6;
    open Out, ">t.f" or die "Can't write t.f.\n";
    print Out "$sp PROGRAM t\n";
    print Out "$sp LOGICAL itrue, ifalse\n";
    print Out "$sp itrue = .TRUE.\n";
    print Out "$sp ifalse = .FALSE.\n";
    print Out "$sp call ftest(itrue, ifalse)\n";
    print Out "$sp END\n";
    close Out;
    my $t = `$FC t.f tlib.o -o t && ./t`;
    if($t=~/T:(-?\d+)\s*F:(-?\d+)/){
        $cross_hash{TRUE_VALUE}=$1;
        $cross_hash{FALSE_VALUE}=$2;
    }
}

sub get_f90_kinds_direct {
    open Out, ">t.f90" or die "Can't write t.f90.\n";
    print Out "PROGRAM t\n";
    print Out "INTEGER I, k, lastkind\n";
    print Out "\n";
    print Out "lastkind=SELECTED_INT_KIND(1)\n";
    print Out "DO I=2,30\n";
    print Out "k = SELECTED_INT_KIND(I)\n";
    print Out "IF (k .ne. lastkind) THEN\n";
    print Out "PRINT *, 'K:',I-1, ', ', lastkind\n";
    print Out "lastkind = k\n";
    print Out "ENDIF\n";
    print Out "IF (k .le. 0) THEN\n";
    print Out "exit\n";
    print Out "ENDIF\n";
    print Out "ENDDO\n";
    print Out "IF (k .ne. lastkind) THEN\n";
    print Out "PRINT *, 31, ', ', k\n";
    print Out "ENDIF\n";
    print Out "END\n";
    close Out;
    my $t=`$FC t.f90 -o t && ./t`;
    my @range_kind_size;
    while($t=~/K:\s*(\d+)\s*,\s*(\d+)/g){
        push @range_kind_size, [$1, $2];
    }
    return \@range_kind_size;
}

sub get_f90_names_direct {
    my ($namelist) = @_;
    my %byte_map=("ADDRESS"=>$cross_hash{void_p}, "OFFSET"=>8, "INTEGER"=>$cross_hash{INTEGER});
    my %size_map=(1=>2, 2=>4, 4=>8, 8=>16, 16=>30);
    open Out, ">t.f90" or die "Can't write t.f90.\n";
    print Out "PROGRAM t\n";
    foreach my $name (@$namelist){
        if($name=~/(\w+)_MODEL/){
            my $T=$1;
            if($T eq "DOUBLE"){
                $T="DOUBLE PRECISION";
            }
            print Out "$T :: A_$1\n";
        }
    }
    print Out "\n";

    foreach my $name (@$namelist){
        if($name=~/(\w+)_KIND/){
            my $n = $size_map{$byte_map{$1}};
            print Out "PRINT *, '$name:', SELECTED_INT_KIND($n)\n";
        }
        elsif($name=~/(REAL|DOUBLE)_MODEL/){
            print Out "PRINT *, '$name:', precision(A_$1), ',', range(A_$1)\n";
        }
        elsif($name=~/(INTEGER)_MODEL/){
            print Out "PRINT *, '$name:', range(A_$1)\n";
        }
    }
    print Out "END\n";
    close Out;
    my $t = `$FC t.f90 -o t && ./t`;
    while($t=~/(\w+):\s*([\d, ]+)/g){
        $cross_hash{$1}=$2;
        $cross_hash{$1}=~s/ +//g;
    }
}

sub get_type_name {
    my ($type) = @_;
    my $t = $type;
    $t=~s/ /_/g;
    $t=~s/\*/p/g;
    return $t;
}

