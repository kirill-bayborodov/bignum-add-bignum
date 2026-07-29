; -----------------------------------------------------------------------------
; @file    bignum_add_bignum.asm
; @author  git@bayborodov.com
; @version 1.0.1
; @date    29.07.2026
;
; @brief   Экстремально оптимизированная реализация сложения больших чисел.
;
; @details
;   Использует System V AMD64 ABI.
;   Оптимизации:
;   - Pointer Swapping (rsi всегда указывает на самое длинное число)
;   - Branchless Overlap Check (проверка перекрытия без ветвлений)
;   - CF Preservation (сохранение флага переноса через lea/dec)
;   - Early Exit (мгновенное копирование остатка через SSE, если carry = 0)
;   - Lazy Zeroing (быстрое обнуление хвоста через pxor/movdqu)
; -----------------------------------------------------------------------------

section .text
global bignum_add_bignum

; --- Константы ---
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
    ; Аргументы:
    ; rdi = bignum_t *result
    ; rsi = const bignum_t *a
    ; rdx = const bignum_t *b

    push    rbp
    mov     rbp, rsp
    push    r12
    push    r13
    push    r14
    push    r15

    ; 1. Проверка на NULL
    test    rdi, rdi
    je      .err_null
    test    rsi, rsi
    je      .err_null
    test    rdx, rdx
    je      .err_null

    ; 2. Branchless проверка перекрытия буферов (разрешаем in-place)
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

    ; 3. Загрузка длин и проверка Capacity
    mov     r8, qword [rsi + BIGNUM_OFFSET_LEN]  ; r8 = a->len
    mov     r9, qword [rdx + BIGNUM_OFFSET_LEN]  ; r9 = b->len

    cmp     r8, BIGNUM_CAPACITY
    ja      .err_cap
    cmp     r9, BIGNUM_CAPACITY
    ja      .err_cap

    ; 4. Pointer Swapping: гарантируем, что rsi указывает на самое длинное число
    cmp     r8, r9
    jae     .no_swap
    ; Меняем местами указатели
    mov     rax, rsi
    mov     rsi, rdx
    mov     rdx, rax
    ; Меняем местами длины
    mov     rax, r8
    mov     r8, r9
    mov     r9, rax
.no_swap:
    ; Теперь r8 >= r9. rsi = longest, rdx = shortest.

    mov     r12, rdi        ; Сохраняем указатель на result

    ; 5. Подготовка к сложению общей части (min_len = r9)
    mov     rcx, r9
    shr     rcx, 2          ; rcx = min_len / 4
    mov     r10, r9
    and     r10, 3          ; r10 = min_len % 4

    test    rcx, rcx
    jz      .tail_b_setup

    clc                     ; Очищаем флаг переноса (CF = 0)
    align 16
.loop_unroll_4:
    ; Развернутый цикл. CF аппаратно передается между adc
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

    ; Сдвигаем указатели. LEA НЕ ПОРТИТ ФЛАГ CF!
    lea     rsi, [rsi + 32]
    lea     rdx, [rdx + 32]
    lea     rdi, [rdi + 32]

    ; Уменьшаем счетчик. DEC НЕ ПОРТИТ ФЛАГ CF!
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
    ; Если после всех сложений остался перенос, нужно добавить еще одно слово
    jnc     .sub_done
    cmp     r8, BIGNUM_CAPACITY
    jae     .err_overflow   ; Если длина уже максимальная, то это переполнение
    mov     qword [rdi], 1
    inc     r8              ; Увеличиваем длину результата

.sub_done:
    ; 6. Установка начальной длины
    mov     qword [r12 + BIGNUM_OFFSET_LEN], r8

    ; 7. Lazy Zeroing (обнуление неиспользуемого хвоста)
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
    ; 8. Нормализация результата (удаление ведущих нулей)
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
    ; Восстанавливаем регистры и стек
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    ret
