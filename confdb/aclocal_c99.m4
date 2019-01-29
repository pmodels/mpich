dnl PAC_CHECK_C99 -- checks a list of C99 features, emit error if not available.
AC_DEFUN([PAC_CHECK_C99], [
    dnl 
    AC_PROG_CC_C99
    if test '$ac_cv_prog_cc_c99' = 'no' ; then
        dnl C99 is not fully supported. Add minimum tests below:
        PAC99_INLINE
        PAC99_VAR_DECL
        PAC99_VAR_IN_FORLOOP
        PAC99_INLINE_COMMENT
        PAC99_BOOL
        PAC99_STDINT
        PAC99_INTTYPES
        PAC99_VARIADIC_MACRO
        PAC99_COMPOUND_LITERAL
        PAC99_DESIGNATED_INIT
        PAC99__FUNC__
        PAC99_SNPRINTF
        PAC99_RESTRICT
    else
        dnl According to Autoconf manual, following are tested:
        dnl _Bool, flexible arrays, inline, long long int, 
        dnl mixed code and declarations, named initialization of structs, 
        dnl restrict, varargs macros, variable declarations in for loops 
        dnl and variable length arrays

        dnl Add any additional tests if needed
        :
    fi
])

dnl ***************************
AC_DEFUN([PAC99_TEST], [
    AC_MSG_CHECKING([C99 $1 feature])
    AC_COMPILE_IFELSE([AC_LANG_SOURCE[$2]],
        [AC_MSG_RESULT(yes)],
        [AC_MSG_RESULT(no)
         AC_MSG_ERROR([C99 $1 not supported])]
    )
])
    
dnl ***************************
AC_DEFUN([PAC99_INLINE], [
    # NOTE: may require -O in CFLAGS    
    PAC99_TEST([inline], [
        inline int A(){
            return 42;
        }
        int main(){
            return A()-42;
        }
    ])
])

AC_DEFUN([PAC99_VAR_DECL], [
    PAC99_TEST([in-code variable declaration], [
        int main(){
            int a=0;
            a+=2;
            int b=a*20;
            return a+b-42;
        }
    ])
])

AC_DEFUN([PAC99_VAR_IN_FORLOOP], [
    PAC99_TEST([variable declaration in for-loop], [
        int main(){
            int a=2;
            for (int i=0;i<4;i++) {
                a+=10;
            }
            return a-42;
        }
    ])
])

AC_DEFUN([PAC99_INLINE_COMMENT], [
    PAC99_TEST([inline comment], [
        int main(){
            // inline comment
            return 42-42;
        }
    ])
])

AC_DEFUN([PAC99_BOOL], [
    PAC99_TEST([bool], [
        #include <stdbool.h>
        int main(){
            bool a = true;
            if(a) return 0;
            return 42;
        }
    ])
])

AC_DEFUN([PAC99_STDINT], [
    PAC99_TEST([stdint], [
        #include <stdint.h>
        int main(){
            int8_t a;
            uint64_t b;
            intptr_t p;
            return sizeof(p)/sizeof(p)*40+sizeof(a)+sizeof(b)/8-42;
        }
    ])
])

AC_DEFUN([PAC99_INTTYPES], [
    PAC99_TEST([inttypes], [
        #include <string.h>
        #include <inttypes.h>
        int main(){
            if(strlen("str-" PRIu64 "-cat")>8){
                return 0;
            }
            return 42;
        }
    ])
])

AC_DEFUN([PAC99_VARIADIC_MACRO], [
    PAC99_TEST([variadic macro], [
        #define A(...) B(__VA_ARGS__)
        int B(int a, int b){return a+b;}
        int main(){
            return A(40,2)-42;
        }
    ])
])

AC_DEFUN([PAC99_COMPOUND_LITERAL], [
    PAC99_TEST([compound literal], [
        struct A {int a; float b;};
        int B(struct A c){ return c.a+(int)c.b; }
        int main(){
            return B((struct A){40, 2,1}) - 42;
        }
    ])
])

AC_DEFUN([PAC99_DESIGNATED_INIT], [
    PAC99_TEST([designated init], [
        struct A {int a; int b;};
        int main(){
            struct A c={.b=2, .a=10};
            return c.a*4+c.b - 42;
        }
    ])
])

AC_DEFUN([PAC99__FUNC__], [
    PAC99_TEST([__func__], [
        #include <string.h>
        int main(){
            if(strcmp("main", __func__)==0){
                return 0;
            }
            return 42;
        }
    ])
])

AC_DEFUN([PAC99_SNPRINTF], [
    PAC99_TEST([snprintf], [
        #include <stdio.h>
        int main(){
            char buf[6];
            int ret;
            ret = snprintf(buf, 6, "Hello %s!", "world");
            return ret - 12;
        }
    ])
])

AC_DEFUN([PAC99_RESTRICT], [
    PAC99_TEST([restrict], [
        int A(int a, int * restrict p){
            return a+*p;
        }
        int main(){
            int a = 40;
            return A(2,&a) - 42;
        }
    ])
])

