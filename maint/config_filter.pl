#!/usr/bin/perl
use strict;

# This script is to be run by AC_CONFIG_COMMANDS in mpich's configure.ac,
# after all the AC_OUTPUT files are generated -- in particular, mpi.h and
# mpichconf.h.
# Configure script's function is to gather user options and do system feature
# detections. That includes checking the sizes of language datatypes. This
# script does further processing based on the results in mpichconf.h. It does
# following tasks:
# * Putting in the size into MPI datatype handles in mpi.h.
# * typedef MPI_Aint, MPI_Offset, MPI_Count in mpi.h.
# * Putting in handle decimal values in mpif.h.
# * Define HAVE_datatype macros for all types that have non-zero sizes.
# * Define MPIR_xxx_CTYPE for FORTRAN and CXX datatypes.
# * Define MPIR_xxx_FMT_[dx] and MPIR_xxx_MAX for MPI_Aint, MPI_Offset, and MPI_Count.

our %sized_types;

my $srcdir=".";
if($0=~/(.*)\/maint\/.*/){
    $srcdir = $1;
}

our (%size_hash, %ctype_hash, %have_hash);
load_mpichconf("src/include/mpichconf.h");
customize_size_hash();

our %sized_types;
get_sized_types();
match_type_fint_etc();
match_binding_types();

our (%MPI_hash, @MPI_list);
my $lines = load_mpi_h("src/include/mpi.h");
dump_lines("src/include/mpi.h", $lines);

my $lines = load_mpir_ext_h("src/include/mpir_ext.h");
dump_lines("src/include/mpir_ext.h", $lines);

if($have_hash{FORTRAN_BINDING}){
    my $lines = load_mpif_h();
    dump_lines("src/binding/fortran/mpif_h/mpif.h", $lines);
    dump_lines("src/include/mpif.h", $lines);

    my $file = "src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.f90";
    my $lines = load_f08_compile_constants($file);
    dump_lines($file, $lines);
    my $file = "src/binding/fortran/use_mpi_f08/mpi_c_interface_types.f90";
    my $lines = load_f08_c_interface_types($file);
    dump_lines($file, $lines);
}

# ---- subroutines -------------------------

sub load_mpichconf {
    my ($mpichconf_h) = @_;
    open In, "$mpichconf_h" or die "Can't open $mpichconf_h.\n";
    while(<In>){
        if(/^#define SIZEOF_(\w+)\s+(\d+)/){
            my ($name, $size) = ($1, $2);
            $size_hash{$name} = $size;
            if($name=~/^INT\d+_T/){
                $size_hash{"U$name"}=$size;
            }
            elsif($name=~/^(char|short|int|long)/i){
                $size_hash{"UNSIGNED_$name"}=$size;
                $size_hash{"SIGNED_$name"}=$size;
            }
        }
        elsif(/^#define MPIR_(\w+)_CTYPE /){
            $ctype_hash{$1} = 1;
            if(/^#define MPIR_INTEGER(\d+)_CTYPE /){
                $size_hash{"INTEGER$1"} = $1;
            }
            elsif(/^#define MPIR_REAL(\d+)_CTYPE /){
                $size_hash{"REAL$1"} = $1;
                $size_hash{"COMPLEX".($1*2)} = $1*2;
            }
        }
        elsif(/^#define HAVE_(\w+) (\w+)/){
            $have_hash{$1}=$2;
        }
    }
    close In;
}

sub customize_size_hash {
    $size_hash{BYTE}=1;
    $size_hash{PACKED}=1;
    $size_hash{LB}="-";
    $size_hash{UB}="-";

    if(! $size_hash{CHAR} > 0){
        warn "ASSERTION FAIL: size of CHAR not found\n";
    }
    if(! $size_hash{SHORT} > 0){
        warn "ASSERTION FAIL: size of SHORT not found\n";
    }
    if(! $size_hash{INT} > 0){
        warn "ASSERTION FAIL: size of INT not found\n";
    }
    if(! $size_hash{LONG} > 0){
        warn "ASSERTION FAIL: size of LONG not found\n";
    }
    if(! $size_hash{FLOAT} > 0){
        warn "ASSERTION FAIL: size of FLOAT not found\n";
    }
    if(! $size_hash{DOUBLE} > 0){
        warn "ASSERTION FAIL: size of DOUBLE not found\n";
    }
    $size_hash{"UNSIGNED"}=$size_hash{INT};
    $size_hash{"WCHAR"}=$size_hash{WCHAR_T};

    $size_hash{"C_BOOL"}=$size_hash{_BOOL};
    $size_hash{"C_FLOAT16"}=$size_hash{_FLOAT16};

    $size_hash{"C_FLOAT_COMPLEX"}=$size_hash{FLOAT__COMPLEX};
    $size_hash{"C_DOUBLE_COMPLEX"}=$size_hash{DOUBLE__COMPLEX};
    $size_hash{"C_LONG_DOUBLE_COMPLEX"}=$size_hash{LONG_DOUBLE__COMPLEX};

    if($have_hash{FORTRAN_BINDING}){
        $size_hash{"INTEGER"} = $size_hash{F77_INTEGER};
        $size_hash{"REAL"} = $size_hash{F77_REAL};
        $size_hash{"DOUBLE_PRECISION"} = $size_hash{F77_DOUBLE_PRECISION};
        if(! $size_hash{INTEGER} > 0){
            warn "ASSERTION FAIL: size of INTEGER not found\n";
        }
        if(! $size_hash{REAL} > 0){
            warn "ASSERTION FAIL: size of REAL not found\n";
        }
        if(! $size_hash{DOUBLE_PRECISION} > 0){
            warn "ASSERTION FAIL: size of DOUBLE_PRECISION not found\n";
        }

        $size_hash{CHARACTER}=1;
        $size_hash{LOGICAL}=$size_hash{INTEGER};
        $size_hash{COMPLEX} = $size_hash{REAL}*2;
        $size_hash{DOUBLE_COMPLEX} = $size_hash{DOUBLE_PRECISION}*2;
        foreach my $i (1,2,4,8,16){
            if($have_hash{"F77_INTEGER_$i"}){
                $size_hash{"INTEGER$i"}=$i;
            }
        }
        foreach my $i (2,4,8,16){
            if($have_hash{"F77_REAL_$i"}){
                $size_hash{"REAL$i"}=$i;
                my $i2 = $i*2;
                $size_hash{"COMPLEX$i2"}=$i2;
            }
        }
    }

    if($have_hash{CXX_BINDING}){
        $size_hash{"CXX_BOOL"} = $size_hash{BOOL};
        $size_hash{"CXX_FLOAT_COMPLEX"} = $size_hash{FLOATCOMPLEX};
        $size_hash{"CXX_DOUBLE_COMPLEX"} = $size_hash{DOUBLECOMPLEX};
        $size_hash{"CXX_LONG_DOUBLE_COMPLEX"} = $size_hash{LONGDOUBLECOMPLEX};
    }

    if($size_hash{LONG_DOUBLE}==0){
        $size_hash{LONG_DOUBLE_INT}=0;
        $size_hash{LONG_DOUBLE__COMPLEX}=0;
        $size_hash{C_LONG_DOUBLE_COMPLEX}=0;
        $size_hash{CXX_LONG_DOUBLE_COMPLEX} = 0;
    }
}

sub get_sized_types {
    foreach my $t (qw(char int short long long_long)){
        my $size = $size_hash{uc($t)};
        if(!$sized_types{"i$size"}){
            $sized_types{"i$size"}=$t;
            if($t=~/\w+_\w+/){
                $sized_types{"i$size"}=~s/_/ /g;
            }
        }
    }

    foreach my $t (qw(_Float16 float double long_double)){
        my $size = $size_hash{uc($t)};
        if(!$sized_types{"r$size"}){
            $sized_types{"r$size"}=$t;
            if($t=~/\w+_\w+/){
                $sized_types{"r$size"}=~s/_/ /g;
            }
        }
    }
}

sub match_type_fint_etc {
    $ctype_hash{FINT} = $sized_types{"i".$size_hash{FINT}};
    $ctype_hash{AINT} = $sized_types{"i".$size_hash{AINT}};
    $ctype_hash{OFFSET} = $sized_types{"i".$size_hash{OFFSET}};
    $ctype_hash{COUNT} = $sized_types{"i".$size_hash{COUNT}};
    if($size_hash{INTEGER}){
        $ctype_hash{FINT} = $sized_types{"i".$size_hash{INTEGER}};
    }

    my $has_error;
    foreach my $t ("FINT", "AINT", "OFFSET", "COUNT"){
        if(!$ctype_hash{$t}){
            warn "Couldn't find c type for MPI_$t\n";
            $has_error++;
        }
    }

    if($has_error){
        die;
    }
}

sub match_binding_types {
    if($have_hash{FORTRAN_BINDING}){
        foreach my $t (qw(CHARACTER INTEGER INTEGER1 INTEGER2 INTEGER4 INTEGER8 INTEGER16)){
            $ctype_hash{$t} = $sized_types{"i".$size_hash{$t}};
        }

        foreach my $t (qw(REAL DOUBLE_PRECISION REAL2 REAL4 REAL8 REAL16)){
            $ctype_hash{$t} = $sized_types{"r".$size_hash{$t}};
        }

        $ctype_hash{LOGICAL} = "MPI_Fint";

        $ctype_hash{COMPLEX} = "float _Complex";
        $ctype_hash{COMPLEX16} = "double _Complex";
        $ctype_hash{COMPLEX32} = "long double _Complex";
    }

    if($have_hash{CXX_BINDING}){
        if($size_hash{C_BOOL}>0){
            if($size_hash{CXX_BOOL}==$size_hash{C_BOOL}){
                $ctype_hash{CXX_BOOL}="_Bool";
            }
            else{
                $ctype_hash{CXX_BOOL} = "unsigned ".$sized_types{"i".$size_hash{CXX_BOOL}};
            }
        }

        if($size_hash{C_FLOAT_COMPLEX}>0 && $size_hash{C_FLOAT_COMPLEX}==$size_hash{CXX_FLOAT_COMPLEX}){
            $ctype_hash{CXX_FLOAT_COMPLEX} = "float _Complex";
        }
        if($size_hash{C_DOUBLE_COMPLEX}>0 && $size_hash{C_DOUBLE_COMPLEX}==$size_hash{CXX_DOUBLE_COMPLEX}){
            $ctype_hash{CXX_DOUBLE_COMPLEX} = "double _Complex";
        }
        if($size_hash{C_LONG_DOUBLE_COMPLEX}>0 && $size_hash{C_LONG_DOUBLE_COMPLEX}==$size_hash{CXX_LONG_DOUBLE_COMPLEX}){
            $ctype_hash{CXX_LONG_DOUBLE_COMPLEX} = "long double _Complex";
        }
    }
}

sub load_mpi_h {
    my ($mpi_h) = @_;
    my @lines;
    open In, "$mpi_h" or die "Can't open $mpi_h.\n";
    while(<In>){
        if(/^#define\s+(MPIX?)_(\w+)\s*\(\(MPI_Datatype\)(0x[0-9a-f]+)\)/){
            my ($prefix, $name, $id) = ($1, $2, $3);
            if($name ne "DATATYPE_NULL"){
                my $t_name=$name;
                $t_name=~s/(UNSIGNED|SIGNED)_//;
                my $size = $size_hash{$t_name};
                if(!$size and $name=~/^2(\w+)/){
                    $size = 2 * $size_hash{$1};
                    $size_hash{$name}=$size;
                }

                if($id=~/0x8c/ and $size ne "0"){
                    $size_hash{$name} = "-";
                }
                elsif($size eq "-"){
                }
                elsif(!$size){
                }
                else{
                    substr($id,6,2)=sprintf("%02x", $size);
                    s/0x[0-9a-f]+/$id/;
                }
                $MPI_hash{$prefix.'_'.$name} = $id;
                push @MPI_list, $name;
            }
        }
        elsif(/^#define\s+(MPIX?)_(\w+)\s+(MPIX?)_(\w+)/){
            my ($prefix1, $name1, $prefix2, $name2) = ($1, $2, $3, $4);
            $MPI_hash{$prefix1.'_'.$name1} = $MPI_hash{$prefix2.'_'.$name2};
            $size_hash{$name1} = $size_hash{$name2};
        }
        elsif(/PLACEHOLDER FOR config_filter/){
            my $type = $ctype_hash{FINT};
            push @lines, "typedef ".$ctype_hash{FINT}." MPI_Fint;\n";
            my $type = $ctype_hash{AINT};
            push @lines, "typedef ".$ctype_hash{AINT}." MPI_Aint;\n";
            my $type = $ctype_hash{OFFSET};
            push @lines, "typedef ".$ctype_hash{OFFSET}." MPI_Offset;\n";
            my $type = $ctype_hash{COUNT};
            push @lines, "typedef ".$ctype_hash{COUNT}." MPI_Count;\n";
            push @lines, "\n";
            next;
        }
        elsif(/^#ifdef\s+(HAVE_MPIX?_(\w+))/){
            my ($have, $name) = ($1, $2);
            my $t = 0;
            if($size_hash{$name}){
                $t = 1;
            }
            $_="#if $t /\x2a $have \x2a/\n";
        }

        push @lines, $_;
    }
    close In;

    return \@lines;
}

sub dump_lines {
    my ($file, $lines) = @_;
    open Out, ">$file" or die "Can't write $file.\n";
    print Out @$lines;
    close Out;
}

sub load_mpir_ext_h {
    my ($mpir_ext_h) = @_;
    my @lines;
    open In, "$mpir_ext_h" or die "Can't open $mpir_ext_h.\n";
    while(<In>){
        if(/PLACEHOLDER for config_filter/){
            foreach my $a (sort keys %MPI_hash){
                if($a =~/MPI\w?_(\w+)/){
                    if(!$size_hash{$1}){
                        push @lines, "/* #define HAVE_$1 1 */\n";
                    }
                    else{
                        push @lines, "#define HAVE_$1 1\n";
                    }
                }
            }
            push @lines, "\n";
            my (@klist, $maxlen);
            foreach my $k (sort keys %ctype_hash){
                if($ctype_hash{$k}){
                    if($maxlen < length($k)){
                        $maxlen = length($k);
                    }
                    push @klist, $k;
                }
            }
            foreach my $k (@klist){
                my $sp = ' ' x ($maxlen - length($k));
                push @lines, "#define MPIR_$k\_CTYPE $sp$ctype_hash{$k}\n";
            }
            push @lines, "\n";
            push @lines, "\x2f* MPIR_Ucount is needed in mpir_status.h *\x2f\n";
            push @lines, "#define MPIR_Ucount  unsigned $ctype_hash{COUNT}\n";
            push @lines, "\n";
            if($size_hash{AINT}==4){
                push @lines, "#define MPIR_AINT_FMT_d \"%\" PRId32\n";
                push @lines, "#define MPIR_AINT_FMT_x \"%\" PRIx32\n";
                push @lines, "#define MPIR_AINT_MAX 2147483647\n";
            }
            elsif($size_hash{AINT}==8){
                push @lines, "#define MPIR_AINT_FMT_d \"%\" PRId64\n";
                push @lines, "#define MPIR_AINT_FMT_x \"%\" PRIx64\n";
                push @lines, "#define MPIR_AINT_MAX 9223372036854775807\n";
            }
            else{
                die "sizeof MPI_AINT is neither 4 or 8 bytes\n";
            }
            if($size_hash{OFFSET}==4){
                push @lines, "#define MPIR_OFFSET_FMT_d \"%\" PRId32\n";
                push @lines, "#define MPIR_OFFSET_FMT_x \"%\" PRIx32\n";
                push @lines, "#define MPIR_OFFSET_MAX 2147483647\n";
            }
            elsif($size_hash{OFFSET}==8){
                push @lines, "#define MPIR_OFFSET_FMT_d \"%\" PRId64\n";
                push @lines, "#define MPIR_OFFSET_FMT_x \"%\" PRIx64\n";
                push @lines, "#define MPIR_OFFSET_MAX 9223372036854775807\n";
            }
            else{
                die "sizeof MPI_OFFSET is neither 4 or 8 bytes\n";
            }
            if($size_hash{COUNT}==4){
                push @lines, "#define MPIR_COUNT_FMT_d \"%\" PRId32\n";
                push @lines, "#define MPIR_COUNT_FMT_x \"%\" PRIx32\n";
                push @lines, "#define MPIR_COUNT_MAX 2147483647\n";
            }
            elsif($size_hash{COUNT}==8){
                push @lines, "#define MPIR_COUNT_FMT_d \"%\" PRId64\n";
                push @lines, "#define MPIR_COUNT_FMT_x \"%\" PRIx64\n";
                push @lines, "#define MPIR_COUNT_MAX 9223372036854775807\n";
            }
            else{
                die "sizeof MPI_COUNT is neither 4 or 8 bytes\n";
            }
            push @lines, "\n";
            push @lines, "#define MPI_AINT_FMT_DEC_SPEC MPIR_AINT_FMT_d\n";
            push @lines, "#define MPI_AINT_FMT_HEX_SPEC MPIR_AINT_FMT_x\n";
        }
        else{
            push @lines, $_;
        }
    }
    close In;
    return \@lines;
}

sub load_mpif_h {
    my ($has_exclaim, $has_real8);
    if($ENV{has_exclaim} eq "yes"){
        $has_exclaim = 1;
    }
    if($ENV{has_real8} eq "yes"){
        $has_real8 = 1;
    }

    my @lines;
    open In, "src/binding/fortran/mpif_h/mpif.h" or die "Can't open src/binding/fortran/mpif_h/mpif.h.\n";
    while(<In>){
        if($has_exclaim){
            s/^C/!/;
        }
        if($has_real8){
            s/DOUBLE PRECISION/REAL*8/g;
        }
        if(/PARAMETER\s*\((MPIX?_\w+)=-\)/){
            if($MPI_hash{$1}){
                my $t = hex($MPI_hash{$1});
                if($MPI_hash{$1}=~/0x[89a-f]/){
                    $t -= 0x100000000;
                }
                s/-/$t/;
            }
        }
        push @lines, $_;
    }
    close In;
    return \@lines;
}

sub load_f08_compile_constants {
    my @lines;
    open In, "src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.f90" or die "Can't open src/binding/fortran/use_mpi_f08/mpi_f08_compile_constants.f90.\n";
    while(<In>){
        if(/Datatypes PLACEHOLDER for config_filter/){
            push @lines, "! Datatypes\n";
            foreach my $name (sort keys %MPI_hash){
                my $t = hex($MPI_hash{$name});
                if($MPI_hash{$name}=~/0x[89a-f]/){
                    $t -= 0x100000000;
                }
                my $s = sprintf("%-25s", $name);
                push @lines, "type(MPI_Datatype), parameter  :: $s = MPI_Datatype($t)\n";


            }
            next;
        }
        push @lines, $_;
    }
    close In;
    return \@lines;
}

sub load_f08_c_interface_types {
    my @lines;
    open In, "src/binding/fortran/use_mpi_f08/mpi_c_interface_types.f90" or die "Can't open src/binding/fortran/use_mpi_f08/mpi_c_interface_types.f90.\n";
    while(<In>){
        if(/c_Aint.* PLACEHOLDER for config_filter/){
            foreach my $a (qw(Aint Offset Count)){
                my $t = "c_".$ctype_hash{uc($a)};
                $t=~s/ /_/g;
                push @lines, "integer,parameter :: c_$a = $t\n";
            }
            next;
        }
        push @lines, $_;
    }
    close In;
    return \@lines;
}

