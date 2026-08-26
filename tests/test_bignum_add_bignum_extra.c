/**
 * @file    test_bignum_add_bignum_extra.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    29.07.2026
 *
 * @brief   Дополнительные тесты (фаззинг и надежность) для bignum_add_bignum.
 */

#include "bignum_add_bignum.h"
#include <bignum_cmp.h>
#include <bignum_init.h>
#include <bignum_init_from_array.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


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

#define FUZZ_ITERATIONS 10000

static int tests_passed = 0;
static int tests_failed = 0;

int test_robustness_a_len_exceeds_capacity() {
    bignum_t a, b, result;
    bignum_init_from_array(&a, (uint64_t[]){1}, 1);
    bignum_init_from_array(&b, (uint64_t[]){1}, 1);
    a.len = BIGNUM_CAPACITY + 1;
    bignum_add_bignum_status_t status = bignum_add_bignum(&result, &a, &b);
    a.len = 1;
    return status == BIGNUM_ADD_BIGNUM_ERROR_CAPACITY_EXCEEDED;
}

int test_robustness_b_len_exceeds_capacity() {
    bignum_t a, b, result;
    bignum_init_from_array(&a, (uint64_t[]){1}, 1);
    bignum_init_from_array(&b, (uint64_t[]){1}, 1);
    b.len = BIGNUM_CAPACITY + 1;
    bignum_add_bignum_status_t status = bignum_add_bignum(&result, &a, &b);
    b.len = 1;
    return status == BIGNUM_ADD_BIGNUM_ERROR_CAPACITY_EXCEEDED;
}

static void print_bignum(const char* name, const bignum_t* num) {
    fprintf(stderr, "%s (len=%zu): { ", name, num->len);
    if (num->len == 0) {
        fprintf(stderr, "0 ");
    } else {
        for (size_t i = 0; i < num->len; ++i) {
            fprintf(stderr, "0x%016lX ", num->words[i]);
        }
    }
    fprintf(stderr, "}\n");
}

int test_fuzzing_robustness(void) {
    unsigned int seed = time(NULL) ^ getpid();
    srand(seed);
    printf("Fuzzing with seed: %u\n", seed);

    for (int i = 0; i < FUZZ_ITERATIONS; ++i) {
        bignum_t a, b, res1, res2;
        bignum_init(&a); bignum_init(&b); bignum_init(&res1); bignum_init(&res2);

        // Ограничиваем длину, чтобы избежать частых переполнений
        a.len = rand() % BIGNUM_CAPACITY;
        b.len = rand() % BIGNUM_CAPACITY;

        for (size_t j = 0; j < a.len; ++j) a.words[j] = ((uint64_t)rand() << 32) | rand();
        for (size_t j = 0; j < b.len; ++j) b.words[j] = ((uint64_t)rand() << 32) | rand();

        // Нормализация
        while (a.len > 0 && a.words[a.len - 1] == 0) a.len--;
        while (b.len > 0 && b.words[b.len - 1] == 0) b.len--;

        bignum_add_bignum_status_t st1 = bignum_add_bignum(&res1, &a, &b);
        bignum_add_bignum_status_t st2 = bignum_add_bignum(&res2, &b, &a);

        if (st1 == BIGNUM_ADD_BIGNUM_SUCCESS) {
            // 1. Коммутативность: a + b == b + a
            if (st2 != BIGNUM_ADD_BIGNUM_SUCCESS || bignum_cmp(&res1, &res2) != BIGNUM_CMP_EQ) {
                fprintf(stderr, "Fuzzing failed: Commutativity broken\n");
                return 0;
            }
            // 2. Монотонность: res >= a и res >= b
            if (bignum_cmp(&res1, &a) == BIGNUM_CMP_LESS || bignum_cmp(&res1, &b) == BIGNUM_CMP_LESS) {
                fprintf(stderr, "Fuzzing failed: Result is smaller than operands\n");
                print_bignum("a", &a);
                print_bignum("b", &b);
                print_bignum("res", &res1);
                return 0;
            }
        }
    }
    return 1;
}

int main() {
    printf("\n--- Launching Extra Tests for bignum_add_bignum  ---\n");

    printf("\n--- Running Robustness Tests ---\n");
    RUN_TEST(test_robustness_a_len_exceeds_capacity);
    RUN_TEST(test_robustness_b_len_exceeds_capacity);

    printf("\n--- Running Fuzzing Test ---\n");
    RUN_TEST(test_fuzzing_robustness);

    printf("\n--- Test Summary ---\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("\n----------------------\n");

    return tests_failed > 0 ? 1 : 0;
}
