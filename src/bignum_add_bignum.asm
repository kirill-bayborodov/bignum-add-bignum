; -----------------------------------------------------------------------------
; @file    bignum_add_bignum.asm
; @author  git@bayborodov.com
; @version 1.0.1
; @date    29.07.2026
;
; @brief   Optimized x86-64 implementation of fixed-capacity big-integer addition.
;
; @details
;   Uses the System V AMD64 ABI.
;   Optimizations include pointer swapping, carry preservation with LEA/DEC,
;   early tail-copy exit when carry is clear, and SIMD lazy zeroing.
; -----------------------------------------------------------------------------

section .text
global bignum_add_bignum

; --- Constants ---
BIGNUM_CAPACITY         equ 32
BIGNUM_OFFSET_LEN       equ 256
BUF_SIZE                equ 264

BIGNUM_ADD_BIGNUM_SUCCESS                 equ  0
BIGNUM_ADD_BIGNUM_ERROR_NULL_PTR          equ -1
BIGNUM_ADD_BIGNUM_ERROR_CAPACITY_EXCEEDED equ -2
BIGNUM_ADD_BIGNUM_ERROR_BUFFER_OVERLAP    equ -3
BIGNUM_ADD_BIGNUM_ERROR_OVERFLOW          equ -4

align 16
bignum_add_bignum:
    ; Arguments:
    ; rdi = bignum_t *result
    ; rsi = const bignum_t *a
    ; rdx = const bignum_t *b

    push    rbp
    mov     rbp, rsp
    push    r12
    push    r14
    push    r15

    ; 1. Reject NULL arguments.
    test    rdi, rdi
    je      .err_null
    test    rsi, rsi
    je      .err_null
    test    rdx, rdx
    je      .err_null

    ; 2. Check partial buffer overlap; exact in-place aliases are allowed.
    cmp     rdi, rsi
    je      .overlap_ok_a
    mov     rax, rdi
    sub     rax, rsi
    mov     rcx, rax
    sar     rcx, 63
    xor     rax, rcx
    sub     rax, rcx        ; rax = abs(result - a)
    cmp     rax, BUF_SIZE
    jb      .err_overlap
.overlap_ok_a:

    cmp     rdi, rdx
    je      .overlap_ok_b
    mov     rax, rdi
    sub     rax, rdx
    mov     rcx, rax
    sar     rcx, 63
    xor     rax, rcx
    sub     rax, rcx        ; rax = abs(result - b)
    cmp     rax, BUF_SIZE
    jb      .err_overlap
.overlap_ok_b:

    ; 3. Load lengths and validate capacity.
    mov     r8, qword [rsi + BIGNUM_OFFSET_LEN]  ; r8 = a->len
    mov     r9, qword [rdx + BIGNUM_OFFSET_LEN]  ; r9 = b->len

    cmp     r8, BIGNUM_CAPACITY
    ja      .err_cap
    cmp     r9, BIGNUM_CAPACITY
    ja      .err_cap

    ; 4. Swap pointers so RSI addresses the longer operand.
    cmp     r8, r9
    jae     .no_swap
    ; Swap operand pointers.
    mov     rax, rsi
    mov     rsi, rdx
    mov     rdx, rax
    ; Swap operand lengths.
    mov     rax, r8
    mov     r8, r9
    mov     r9, rax
.no_swap:
    ; R8 >= R9; RSI is longest and RDX is shortest.

    mov     r12, rdi        ; Preserve the result pointer.

    ; 5. Prepare the common addition loop (min_len = r9).
    mov     rcx, r9
    shr     rcx, 2          ; rcx = min_len / 4
    mov     r10, r9
    and     r10, 3          ; r10 = min_len % 4

    test    rcx, rcx
    jz      .tail_b_setup

    clc                     ; Clear carry (CF = 0).
    align 16
.loop_unroll_4:
    ; Unrolled loop; ADC propagates carry between words.
    mov     r11, [rsi]
    adc     r11, [rdx]
    mov     [rdi], r11

    mov     r11, [rsi + 8]
    adc     r11, [rdx + 8]
    mov     [rdi + 8], r11

    mov     r11, [rsi + 16]
    adc     r11, [rdx + 16]
    mov     [rdi + 16], r11

    mov     r11, [rsi + 24]
    adc     r11, [rdx + 24]
    mov     [rdi + 24], r11

    ; LEA advances pointers without changing CF.
    lea     rsi, [rsi + 32]
    lea     rdx, [rdx + 32]
    lea     rdi, [rdi + 32]

    ; DEC advances the loop without changing CF.
    dec     rcx
    jnz     .loop_unroll_4

.tail_b_setup:
    setc    r14b            ; Сохраняем CF (перенос) в r14b
    test    r10, r10
    jz      .tail_a_setup
    shr     r14b, 1         ; Восстанавливаем CF (выталкиваем бит обратно во флаг)

    align 16
.tail_b_loop:
    mov     r11, [rsi]
    adc     r11, [rdx]
    mov     [rdi], r11
    lea     rsi, [rsi + 8]
    lea     rdx, [rdx + 8]
    lea     rdi, [rdi + 8]
    dec     r10
    jnz     .tail_b_loop
    setc    r14b            ; Сохраняем CF после хвоста b

.tail_a_setup:
    mov     r15, r8
    sub     r15, r9         ; r15 = max_len - min_len (ВНИМАНИЕ: портит CF!)
    jz      .restore_cf_and_check_final

    shr     r14b, 1         ; Восстанавливаем CF перед обработкой хвоста a

    align 16
.tail_a_loop:
    ; АЛГОРИТМИЧЕСКАЯ ОПТИМИЗАЦИЯ: Если CF == 0 (переноса больше нет),
    ; мы можем просто скопировать остаток!
    jnc     .fast_copy

    mov     r11, [rsi]
    adc     r11, 0
    mov     [rdi], r11
    lea     rsi, [rsi + 8]
    lea     rdi, [rdi + 8]
    dec     r15
    jnz     .tail_a_loop
    jmp     .check_final_carry

.fast_copy:
    ; Быстрое копирование оставшихся r15 слов из rsi в rdi с использованием SSE
    ; Если мы здесь, значит CF=0, финального переноса точно не будет.
    test    r15, r15
    jz      .sub_done
    mov     rcx, r15
    shr     rcx, 1          ; rcx = количество 16-байтных блоков
    jz      .fast_copy_odd

    align 16
.fast_copy_sse:
    movdqu  xmm0, [rsi]
    movdqu  [rdi], xmm0
    lea     rsi, [rsi + 16]
    lea     rdi, [rdi + 16]
    dec     rcx
    jnz     .fast_copy_sse

.fast_copy_odd:
    test    r15, 1
    jz      .sub_done
    mov     rax, [rsi]
    mov     [rdi], rax
    jmp     .sub_done       ; Пропускаем check_final_carry, так как переноса нет

.restore_cf_and_check_final:
    shr     r14b, 1         ; Восстанавливаем CF, испорченный инструкцией sub r15, r9
.check_final_carry:
    ; Publish one additional word when the final carry fits in capacity.
    jnc     .sub_done
    cmp     r8, BIGNUM_CAPACITY
    jae     .err_overflow   ; Если длина уже максимальная, то это переполнение
    mov     qword [rdi], 1
    inc     r8              ; Увеличиваем длину результата

.sub_done:
    ; 6. Publish the result length.
    mov     qword [r12 + BIGNUM_OFFSET_LEN], r8

    ; 7. Lazily clear unused capacity words.
.zero_rest:
    mov     rcx, BIGNUM_CAPACITY
    sub     rcx, r8
    jz      .normalize

    lea     rdi, [r12 + r8*8]   ; rdi указывает на начало неиспользованного хвоста
    pxor    xmm0, xmm0
    mov     rax, rcx
    shr     rax, 1              ; rax = количество 16-байтных блоков
    jz      .zero_odd

    align 16
.zero_sse_loop:
    movdqu  [rdi], xmm0
    lea     rdi, [rdi + 16]
    dec     rax
    jnz     .zero_sse_loop

.zero_odd:
    test    rcx, 1
    jz      .normalize
    mov     qword [rdi], 0

.normalize:
    ; 8. Normalize by removing leading zero words.
    mov     rdi, r12
    mov     rcx, r8
    test    rcx, rcx
    jz      .success

    align 16
.norm_loop:
    mov     rax, [rdi + rcx*8 - 8]
    test    rax, rax
    jnz     .norm_found
    dec     rcx
    jnz     .norm_loop

.norm_found:
    mov     qword [rdi + BIGNUM_OFFSET_LEN], rcx

.success:
    mov     eax, BIGNUM_ADD_BIGNUM_SUCCESS
    jmp     .epilogue

.err_null:
    mov     eax, BIGNUM_ADD_BIGNUM_ERROR_NULL_PTR
    jmp     .epilogue

.err_cap:
    mov     eax, BIGNUM_ADD_BIGNUM_ERROR_CAPACITY_EXCEEDED
    jmp     .epilogue

.err_overlap:
    mov     eax, BIGNUM_ADD_BIGNUM_ERROR_BUFFER_OVERLAP
    jmp     .epilogue

.err_overflow:
    mov     eax, BIGNUM_ADD_BIGNUM_ERROR_OVERFLOW

.epilogue:
    ; Restore callee-saved registers and stack.
    pop     r15
    pop     r14
    pop     r12
    pop     rbp
    ret
    
section .note.GNU-stack noalloc noexec nowrite progbits