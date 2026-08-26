/**
 * @file    test_bignum_add_bignum.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    29.07.2026
 *
 * @brief   Детерминированные тесты для модуля bignum_add_bignum.
 */

#include "bignum_add_bignum.h"
#include <bignum_cmp.h>
#include <bignum_init.h>
#include <bignum_init_from_array.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>


#define RUN_TEST(test_func) \
    do { \
        printf("Running %s...\n", #test_func); \
        if (test_func()) { \
            printf("  %s: PASSED\n", #test_func); \
            tests_passed++; \
        } else { \
            printf("  %s: FAILED\n", #test_func); \
            tests_failed++; \
        } \
    } while (0)

static int tests_passed = 0;
static int tests_failed = 0;

// --- Тесты на "счастливые пути" ---

int test_simple_add(void) {
    bignum_t a, b, result, expected;
    bignum_init(&a); bignum_init(&b); bignum_init(&result); bignum_init(&expected);
    
    bignum_init_from_array(&a, (uint64_t[]){10}, 1);
    bignum_init_from_array(&b, (uint64_t[]){5}, 1);
    bignum_init_from_array(&expected, (uint64_t[]){15}, 1);

    bignum_add_bignum_status_t status = bignum_add_bignum(&result, &a, &b);
    return status == BIGNUM_ADD_BIGNUM_SUCCESS && bignum_cmp(&result, &expected) == BIGNUM_CMP_EQ && result.len == 1;
}

int test_add_with_carry(void) {
    bignum_t a, b, result, expected;
    bignum_init(&a); bignum_init(&b); bignum_init(&result); bignum_init(&expected);
    
    bignum_init_from_array(&a, (uint64_t[]){0xFFFFFFFFFFFFFFFFULL}, 1);
    bignum_init_from_array(&b, (uint64_t[]){1}, 1);
    bignum_init_from_array(&expected, (uint64_t[]){0, 1}, 2); // 2^64

    bignum_add_bignum_status_t status = bignum_add_bignum(&result, &a, &b);
    return status == BIGNUM_ADD_BIGNUM_SUCCESS && bignum_cmp(&result, &expected) == BIGNUM_CMP_EQ && result.len == 2;
}

int test_cascade_carry(void) {
    bignum_t a, b, result, expected;
    bignum_init(&a); bignum_init(&b); bignum_init(&result); bignum_init(&expected);
    
    uint64_t arr_a[] = {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL};
    bignum_init_from_array(&a, arr_a, 3);
    bignum_init_from_array(&b, (uint64_t[]){1}, 1);
    bignum_init_from_array(&expected, (uint64_t[]){0, 0, 0, 1}, 4);

    bignum_add_bignum_status_t status = bignum_add_bignum(&result, &a, &b);
    return status == BIGNUM_ADD_BIGNUM_SUCCESS && bignum_cmp(&result, &expected) == BIGNUM_CMP_EQ && result.len == 4;
}

int test_add_different_lengths(void) {
    bignum_t a, b, result, expected;
    bignum_init(&a); bignum_init(&b); bignum_init(&result); bignum_init(&expected);
    
    bignum_init_from_array(&a, (uint64_t[]){10, 20, 30}, 3);
    bignum_init_from_array(&b, (uint64_t[]){5}, 1);
    bignum_init_from_array(&expected, (uint64_t[]){15, 20, 30}, 3);

    bignum_add_bignum_status_t status1 = bignum_add_bignum(&result, &a, &b);
    
    // Проверяем коммутативность (b + a)
    bignum_t result2;
    bignum_init(&result2);
    bignum_add_bignum_status_t status2 = bignum_add_bignum(&result2, &b, &a);

    return status1 == BIGNUM_ADD_BIGNUM_SUCCESS && status2 == BIGNUM_ADD_BIGNUM_SUCCESS &&
           bignum_cmp(&result, &expected) == BIGNUM_CMP_EQ &&
           bignum_cmp(&result2, &expected) == BIGNUM_CMP_EQ;
}

// --- Тесты на граничные случаи и нормализацию ---

int test_add_zeros(void) {
    bignum_t a, b, result, expected;
    bignum_init(&a); bignum_init(&b); bignum_init(&result); bignum_init(&expected);
    
    bignum_init_from_array(&a, (uint64_t[]){0}, 0);
    bignum_init_from_array(&b, (uint64_t[]){0}, 0);
    bignum_init_from_array(&expected, (uint64_t[]){0}, 0);

    bignum_add_bignum_status_t status = bignum_add_bignum(&result, &a, &b);
    return status == BIGNUM_ADD_BIGNUM_SUCCESS && bignum_cmp(&result, &expected) == BIGNUM_CMP_EQ && result.len == 0;
}

int test_in_place_add(void) {
    bignum_t a, b, expected;
    bignum_init(&a); bignum_init(&b); bignum_init(&expected);
    
    bignum_init_from_array(&a, (uint64_t[]){10}, 1);
    bignum_init_from_array(&b, (uint64_t[]){5}, 1);
    bignum_init_from_array(&expected, (uint64_t[]){15}, 1);

    // a += b
    bignum_add_bignum_status_t status1 = bignum_add_bignum(&a, &a, &b);
    
    // b += b (in-place self addition)
    bignum_t expected_b;
    bignum_init_from_array(&expected_b, (uint64_t[]){10}, 1);
    bignum_add_bignum_status_t status2 = bignum_add_bignum(&b, &b, &b);

    return status1 == BIGNUM_ADD_BIGNUM_SUCCESS && bignum_cmp(&a, &expected) == BIGNUM_CMP_EQ &&
           status2 == BIGNUM_ADD_BIGNUM_SUCCESS && bignum_cmp(&b, &expected_b) == BIGNUM_CMP_EQ;
}

// --- Тесты на обработку ошибок ---

int test_err_null_pointer(void) {
    bignum_t a, b, result;
    bignum_init(&a); bignum_init(&b); bignum_init(&result);
    bignum_init_from_array(&a, (uint64_t[]){1}, 1);
    bignum_init_from_array(&b, (uint64_t[]){1}, 1);
    
    bool r1 = (bignum_add_bignum(NULL, &a, &b) == BIGNUM_ADD_BIGNUM_ERROR_NULL_PTR);
    bool r2 = (bignum_add_bignum(&result, NULL, &b) == BIGNUM_ADD_BIGNUM_ERROR_NULL_PTR);
    bool r3 = (bignum_add_bignum(&result, &a, NULL) == BIGNUM_ADD_BIGNUM_ERROR_NULL_PTR);
    return r1 && r2 && r3;
}

int test_err_capacity_exceeded(void) {
    bignum_t a, b, result;
    bignum_init(&a); bignum_init(&b); bignum_init(&result);
    bignum_init_from_array(&a, (uint64_t[]){1}, 1);
    bignum_init_from_array(&b, (uint64_t[]){1}, 1);
    
    a.len = BIGNUM_CAPACITY + 1; // Искусственно портим длину
    bignum_add_bignum_status_t status = bignum_add_bignum(&result, &a, &b);
    a.len = 1; // Восстанавливаем
    return status == BIGNUM_ADD_BIGNUM_ERROR_CAPACITY_EXCEEDED;
}

int test_err_buffer_overlap(void) {
    bignum_t a, b;
    bignum_init(&a); bignum_init(&b);
    bignum_init_from_array(&a, (uint64_t[]){10}, 1);
    bignum_init_from_array(&b, (uint64_t[]){5}, 1);
    
    // Создаем указатель, который частично перекрывает a
    bignum_t *overlap_res = (bignum_t *)((unsigned char *)&a + 1);
    return bignum_add_bignum(overlap_res, &a, &b) == BIGNUM_ADD_BIGNUM_ERROR_BUFFER_OVERLAP;
}

int test_err_overflow(void) {
    bignum_t a, b, result;
    bignum_init(&a); bignum_init(&b); bignum_init(&result);
    
    uint64_t arr_a[BIGNUM_CAPACITY];
    for(size_t i = 0; i < BIGNUM_CAPACITY; ++i) {
        arr_a[i] = 0xFFFFFFFFFFFFFFFFULL;
    }
    
    bignum_init_from_array(&a, arr_a, BIGNUM_CAPACITY);
    bignum_init_from_array(&b, (uint64_t[]){1}, 1);

    // Сложение должно вызвать перенос за пределы BIGNUM_CAPACITY
    bignum_add_bignum_status_t status = bignum_add_bignum(&result, &a, &b);
    return status == BIGNUM_ADD_BIGNUM_ERROR_OVERFLOW;
}

int main() {
    printf("\n--- Launching Deterministic Tests for bignum_add_bignum ---\n");

    printf("\n--- Running Happy Path Tests ---\n");
    RUN_TEST(test_simple_add);
    RUN_TEST(test_add_with_carry);
    RUN_TEST(test_cascade_carry);
    RUN_TEST(test_add_different_lengths);

    printf("\n--- Running Boundary and Normalization Tests ---\n");
    RUN_TEST(test_add_zeros);
    RUN_TEST(test_in_place_add);

    printf("\n--- Running Error Handling Tests ---\n");
    RUN_TEST(test_err_null_pointer);
    RUN_TEST(test_err_capacity_exceeded);
    RUN_TEST(test_err_buffer_overlap);
    RUN_TEST(test_err_overflow);

    printf("\n--- Test Summary ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n----------------------\n");

    return tests_failed > 0 ? 1 : 0;
}
