#ifndef CH4_CUDA_KERNEL_OPS_H_INCLUDED
#define CH4_CUDA_KERNEL_OPS_H_INCLUDED

#include <cuda_runtime.h>
#include <cuda.h>


#ifndef MPIR_INTEGER16_CTYPE
#define MPIR_INTEGER16_CTYPE int
#endif

#ifndef MPIR_FC_REAL_CTYPE
#define MPIR_FC_REAL_CTYPE int
#endif

#ifndef MPIR_FC_DOUBLE_CTYPE
#define MPIR_FC_DOUBLE_CTYPE int
#endif

#ifndef _Float16
#define _Float16 int
#endif
/*
#ifndef s_complex
typedef struct {
    int re;
    int im;
} s_complex;
#endif

#ifndef d_complex
typedef struct {
    int re;
    int im;
} d_complex;
#endif

#ifndef ld_complex
typedef struct {
    int re;
    int im;
} ld_complex;
#endif

#ifndef s_fc_complex
typedef struct {
    int re;
    int im;
} s_fc_complex;
#endif

#ifndef d_fc_complex
typedef struct {
    int re;
    int im;
} d_fc_complex;
#endif

#ifndef _Complex
#define _Complex
#endif
*/
#define generate_kernel_call_prototype(type_name_, c_type_, op_type_) void call_##type_name_##_##op_macro_(c_type_ *a, c_type_ *b, int len)

generate_kernel_call_prototype(mpir_typename_int, int, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_int, int, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_int, int, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_int, int, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_int, int, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_int, int, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_int, int, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_int, int, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_int, int, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_int, int, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_long, long, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_long, long, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_long, long, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_long, long, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_long, long, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_long, long, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_long, long, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_long, long, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_long, long, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_long, long, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_short, short, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_short, short, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_short, short, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_short, short, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_short, short, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_short, short, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_short, short, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_short, short, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_short, short, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_short, short, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_unsigned_short, unsigned short, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_unsigned_short, unsigned short, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_unsigned_short, unsigned short, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_unsigned_short, unsigned short, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_unsigned_short, unsigned short, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_unsigned_short, unsigned short, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_unsigned_short, unsigned short, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_unsigned_short, unsigned short, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_unsigned_short, unsigned short, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_unsigned_short, unsigned short, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_unsigned, unsigned, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_unsigned, unsigned, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_unsigned, unsigned, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_unsigned, unsigned, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_unsigned, unsigned, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_unsigned, unsigned, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_unsigned, unsigned, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_unsigned, unsigned, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_unsigned, unsigned, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_unsigned, unsigned, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_unsigned_long, unsigned long, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_unsigned_long, unsigned long, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_unsigned_long, unsigned long, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_unsigned_long, unsigned long, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_unsigned_long, unsigned long, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_unsigned_long, unsigned long, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_unsigned_long, unsigned long, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_unsigned_long, unsigned long, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_unsigned_long, unsigned long, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_unsigned_long, unsigned long, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_long_long, long long, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_long_long, long long, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_long_long, long long, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_long_long, long long, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_long_long, long long, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_long_long, long long, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_long_long, long long, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_long_long, long long, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_long_long, long long, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_long_long, long long, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_unsigned_long_long, unsigned long long, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_unsigned_long_long, unsigned long long, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_unsigned_long_long, unsigned long long, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_unsigned_long_long, unsigned long long, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_unsigned_long_long, unsigned long long, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_unsigned_long_long, unsigned long long, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_unsigned_long_long, unsigned long long, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_unsigned_long_long, unsigned long long, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_unsigned_long_long, unsigned long long, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_unsigned_long_long, unsigned long long, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_signed_char, signed char, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_signed_char, signed char, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_signed_char, signed char, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_signed_char, signed char, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_signed_char, signed char, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_signed_char, signed char, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_signed_char, signed char, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_signed_char, signed char, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_signed_char, signed char, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_signed_char, signed char, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_unsigned_char, unsigned char, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_unsigned_char, unsigned char, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_unsigned_char, unsigned char, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_unsigned_char, unsigned char, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_unsigned_char, unsigned char, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_unsigned_char, unsigned char, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_unsigned_char, unsigned char, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_unsigned_char, unsigned char, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_unsigned_char, unsigned char, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_unsigned_char, unsigned char, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_int8_t, int8_t, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_int8_t, int8_t, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_int8_t, int8_t, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_int8_t, int8_t, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_int8_t, int8_t, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_int8_t, int8_t, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_int8_t, int8_t, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_int8_t, int8_t, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_int8_t, int8_t, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_int8_t, int8_t, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_int16_t, int16_t, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_int16_t, int16_t, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_int16_t, int16_t, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_int16_t, int16_t, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_int16_t, int16_t, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_int16_t, int16_t, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_int16_t, int16_t, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_int16_t, int16_t, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_int16_t, int16_t, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_int16_t, int16_t, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_int32_t, int32_t, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_int32_t, int32_t, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_int32_t, int32_t, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_int32_t, int32_t, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_int32_t, int32_t, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_int32_t, int32_t, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_int32_t, int32_t, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_int32_t, int32_t, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_int32_t, int32_t, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_int32_t, int32_t, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_int64_t, int64_t, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_int64_t, int64_t, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_int64_t, int64_t, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_int64_t, int64_t, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_int64_t, int64_t, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_int64_t, int64_t, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_int64_t, int64_t, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_int64_t, int64_t, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_int64_t, int64_t, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_int64_t, int64_t, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_uint8_t, uint8_t, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_uint8_t, uint8_t, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_uint8_t, uint8_t, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_uint8_t, uint8_t, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_uint8_t, uint8_t, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_uint8_t, uint8_t, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_uint8_t, uint8_t, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_uint8_t, uint8_t, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_uint8_t, uint8_t, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_uint8_t, uint8_t, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_uint16_t, uint16_t, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_uint16_t, uint16_t, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_uint16_t, uint16_t, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_uint16_t, uint16_t, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_uint16_t, uint16_t, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_uint16_t, uint16_t, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_uint16_t, uint16_t, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_uint16_t, uint16_t, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_uint16_t, uint16_t, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_uint16_t, uint16_t, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_uint32_t, uint32_t, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_uint32_t, uint32_t, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_uint32_t, uint32_t, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_uint32_t, uint32_t, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_uint32_t, uint32_t, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_uint32_t, uint32_t, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_uint32_t, uint32_t, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_uint32_t, uint32_t, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_uint32_t, uint32_t, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_uint32_t, uint32_t, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_uint64_t, uint64_t, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_uint64_t, uint64_t, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_uint64_t, uint64_t, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_uint64_t, uint64_t, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_uint64_t, uint64_t, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_uint64_t, uint64_t, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_uint64_t, uint64_t, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_uint64_t, uint64_t, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_uint64_t, uint64_t, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_uint64_t, uint64_t, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_char, char, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_char, char, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_char, char, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_char, char, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_char, char, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_char, char, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_char, char, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_char, char, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_char, char, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_char, char, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_integer, MPI_Fint, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_integer, MPI_Fint, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_integer, MPI_Fint, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_integer, MPI_Fint, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_integer, MPI_Fint, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_integer, MPI_Fint, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_integer, MPI_Fint, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_integer, MPI_Fint, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_integer, MPI_Fint, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_integer, MPI_Fint, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_aint, MPI_Aint, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_aint, MPI_Aint, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_aint, MPI_Aint, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_aint, MPI_Aint, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_aint, MPI_Aint, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_aint, MPI_Aint, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_aint, MPI_Aint, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_aint, MPI_Aint, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_aint, MPI_Aint, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_aint, MPI_Aint, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_offset, MPI_Offset, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_offset, MPI_Offset, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_offset, MPI_Offset, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_offset, MPI_Offset, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_offset, MPI_Offset, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_offset, MPI_Offset, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_offset, MPI_Offset, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_offset, MPI_Offset, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_offset, MPI_Offset, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_offset, MPI_Offset, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_count, MPI_Count, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_count, MPI_Count, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_count, MPI_Count, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_count, MPI_Count, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_count, MPI_Count, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_count, MPI_Count, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_count, MPI_Count, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_count, MPI_Count, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_count, MPI_Count, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_count, MPI_Count, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_character, char, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_character, char, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_character, char, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_character, char, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_character, char, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_character, char, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_character, char, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_character, char, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_character, char, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_character, char, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_integer1, MPIR_INTEGER1_CTYPE, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_integer1, MPIR_INTEGER1_CTYPE, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_integer1, MPIR_INTEGER1_CTYPE, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_integer1, MPIR_INTEGER1_CTYPE, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_integer1, MPIR_INTEGER1_CTYPE, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_integer1, MPIR_INTEGER1_CTYPE, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_integer1, MPIR_INTEGER1_CTYPE, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_integer1, MPIR_INTEGER1_CTYPE, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_integer1, MPIR_INTEGER1_CTYPE, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_integer1, MPIR_INTEGER1_CTYPE, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_integer2, MPIR_INTEGER2_CTYPE, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_integer2, MPIR_INTEGER2_CTYPE, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_integer2, MPIR_INTEGER2_CTYPE, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_integer2, MPIR_INTEGER2_CTYPE, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_integer2, MPIR_INTEGER2_CTYPE, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_integer2, MPIR_INTEGER2_CTYPE, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_integer2, MPIR_INTEGER2_CTYPE, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_integer2, MPIR_INTEGER2_CTYPE, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_integer2, MPIR_INTEGER2_CTYPE, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_integer2, MPIR_INTEGER2_CTYPE, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_integer4, MPIR_INTEGER4_CTYPE, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_integer4, MPIR_INTEGER4_CTYPE, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_integer4, MPIR_INTEGER4_CTYPE, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_integer4, MPIR_INTEGER4_CTYPE, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_integer4, MPIR_INTEGER4_CTYPE, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_integer4, MPIR_INTEGER4_CTYPE, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_integer4, MPIR_INTEGER4_CTYPE, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_integer4, MPIR_INTEGER4_CTYPE, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_integer4, MPIR_INTEGER4_CTYPE, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_integer4, MPIR_INTEGER4_CTYPE, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_integer8, MPIR_INTEGER8_CTYPE, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_integer8, MPIR_INTEGER8_CTYPE, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_integer8, MPIR_INTEGER8_CTYPE, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_integer8, MPIR_INTEGER8_CTYPE, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_integer8, MPIR_INTEGER8_CTYPE, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_integer8, MPIR_INTEGER8_CTYPE, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_integer8, MPIR_INTEGER8_CTYPE, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_integer8, MPIR_INTEGER8_CTYPE, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_integer8, MPIR_INTEGER8_CTYPE, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_integer8, MPIR_INTEGER8_CTYPE, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_integer16, MPIR_INTEGER16_CTYPE, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_integer16, MPIR_INTEGER16_CTYPE, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_integer16, MPIR_INTEGER16_CTYPE, MPIR_LBXOR);
generate_kernel_call_prototype(mpir_typename_integer16, MPIR_INTEGER16_CTYPE, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_integer16, MPIR_INTEGER16_CTYPE, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_integer16, MPIR_INTEGER16_CTYPE, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_integer16, MPIR_INTEGER16_CTYPE, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_integer16, MPIR_INTEGER16_CTYPE, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_integer16, MPIR_INTEGER16_CTYPE, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_integer16, MPIR_INTEGER16_CTYPE, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_float, float, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_float, float, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_float, float, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_float, float, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_float, float, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_float, float, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_float, float, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_double, double, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_double, double, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_double, double, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_double, double, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_double, double, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_double, double, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_double, double, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_real, MPIR_FC_REAL_CTYPE, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_real, MPIR_FC_REAL_CTYPE, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_real, MPIR_FC_REAL_CTYPE, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_real, MPIR_FC_REAL_CTYPE, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_real, MPIR_FC_REAL_CTYPE, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_real, MPIR_FC_REAL_CTYPE, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_real, MPIR_FC_REAL_CTYPE, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_double_precision, MPIR_FC_DOUBLE_CTYPE, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_double_precision, MPIR_FC_DOUBLE_CTYPE, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_double_precision, MPIR_FC_DOUBLE_CTYPE, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_double_precision, MPIR_FC_DOUBLE_CTYPE, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_double_precision, MPIR_FC_DOUBLE_CTYPE, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_double_precision, MPIR_FC_DOUBLE_CTYPE, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_double_precision, MPIR_FC_DOUBLE_CTYPE, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_long_double, long double, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_long_double, long double, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_long_double, long double, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_long_double, long double, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_long_double, long double, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_long_double, long double, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_long_double, long double, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_real4, MPIR_REAL4_CTYPE, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_real4, MPIR_REAL4_CTYPE, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_real4, MPIR_REAL4_CTYPE, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_real4, MPIR_REAL4_CTYPE, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_real4, MPIR_REAL4_CTYPE, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_real4, MPIR_REAL4_CTYPE, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_real4, MPIR_REAL4_CTYPE, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_real8, MPIR_REAL8_CTYPE, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_real8, MPIR_REAL8_CTYPE, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_real8, MPIR_REAL8_CTYPE, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_real8, MPIR_REAL8_CTYPE, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_real8, MPIR_REAL8_CTYPE, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_real8, MPIR_REAL8_CTYPE, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_real8, MPIR_REAL8_CTYPE, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_real16, MPIR_REAL16_CTYPE, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_real16, MPIR_REAL16_CTYPE, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_real16, MPIR_REAL16_CTYPE, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_real16, MPIR_REAL16_CTYPE, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_real16, MPIR_REAL16_CTYPE, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_real16, MPIR_REAL16_CTYPE, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_real16, MPIR_REAL16_CTYPE, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_float16, _Float16, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_float16, _Float16, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_float16, _Float16, MPIR_LLXOR);
generate_kernel_call_prototype(mpir_typename_float16, _Float16, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_float16, _Float16, MPIR_LSUM);
generate_kernel_call_prototype(mpir_typename_float16, _Float16, MPL_MAX);
generate_kernel_call_prototype(mpir_typename_float16, _Float16, MPL_MIN);

generate_kernel_call_prototype(mpir_typename_logical, MPI_Fint, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_logical, MPI_Fint, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_logical, MPI_Fint, MPIR_LLXOR);

generate_kernel_call_prototype(mpir_typename_c_bool, _Bool, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_c_bool, _Bool, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_c_bool, _Bool, MPIR_LLXOR);

generate_kernel_call_prototype(mpir_typename_cxx_bool_value, MPIR_CXX_BOOL_CTYPE, MPIR_LLAND);
generate_kernel_call_prototype(mpir_typename_cxx_bool_value, MPIR_CXX_BOOL_CTYPE, MPIR_LLOR);
generate_kernel_call_prototype(mpir_typename_cxx_bool_value, MPIR_CXX_BOOL_CTYPE, MPIR_LLXOR);
/*
generate_kernel_call_prototype(mpir_typename_complex, s_fc_complex, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_complex, s_fc_complex, MPIR_LSUM);

generate_kernel_call_prototype(mpir_typename_c_float_complex, float _Complex, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_c_float_complex, float _Complex, MPIR_LSUM);

generate_kernel_call_prototype(mpir_typename_c_double_complex, double _Complex, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_c_double_complex, double _Complex, MPIR_LSUM);

generate_kernel_call_prototype(mpir_typename_c_long_double_complex, long double _Complex, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_c_long_double_complex, long double _Complex, MPIR_LSUM);

generate_kernel_call_prototype(mpir_typename_double_complex, d_fc_complex, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_double_complex, d_fc_complex, MPIR_LSUM);

generate_kernel_call_prototype(mpir_typename_complex8, s_complex, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_complex8, s_complex, MPIR_LSUM);

generate_kernel_call_prototype(mpir_typename_complex16, d_complex, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_complex16, d_complex, MPIR_LSUM);

generate_kernel_call_prototype(mpir_typename_cxx_complex_value, s_complex, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_cxx_complex_value, s_complex, MPIR_LSUM);

generate_kernel_call_prototype(mpir_typename_cxx_double_complex_value, d_complex, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_cxx_double_complex_value, d_complex, MPIR_LSUM);

generate_kernel_call_prototype(mpir_typename_cxx_long_double_complex_value, ld_complex, MPIR_LPROD);
generate_kernel_call_prototype(mpir_typename_cxx_long_double_complex_value, ld_complex, MPIR_LSUM);
*/
generate_kernel_call_prototype(mpir_typename_byte, unsigned char, MPIR_LBAND);
generate_kernel_call_prototype(mpir_typename_byte, unsigned char, MPIR_LBOR);
generate_kernel_call_prototype(mpir_typename_byte, unsigned char, MPIR_LBXOR);
#endif /* CH4_CUDA_KERNEL_OPS_H_INCLUDED */
