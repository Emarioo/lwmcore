

#define TEST_PRE()
    li sp, 0x1000
#endmacro

/*
    Tests r0 against VALUE
*/
#define TEST(VALUE)
    li r1, %VALUE%
    li r2, #counter
    call test_eq
#endmacro

#define TEST_POST()
    call test_finish
#endmacro

; #if CPU_BIT_WIDTH == 16
    #define LOAD ldh
    #define STORE ldh
; #elif CPU_BIT_WIDTH == 32
    ; #define LOAD ldl
    ; #define STORE ldl
; #else
    ; #define LOAD ldq
    ; #define STORE ldq
; #endif


