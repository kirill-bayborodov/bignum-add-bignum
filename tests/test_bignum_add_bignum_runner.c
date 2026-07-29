/**
 * @file    test_bignum_add_bignum_runner.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    29.07.2026
 *
 * @brief Интеграционный тест‑раннер для библиотеки libbignum_add_bignum.a.
 */
#include "bignum_add_bignum.h"
#include <assert.h>
#include <stdio.h>

int main() {
    printf("Running test: test_bignum_add_bignum_runner... ");
    bignum_t res = {.words = {0}, .len = 0};
    bignum_t a = {.words = {12345}, .len = 1};
    bignum_t b = {.words = {10000}, .len = 1};
    
    bignum_add_bignum_status_t status = bignum_add_bignum(&res, &a, &b);
    assert(status == BIGNUM_ADD_BIGNUM_SUCCESS);
    assert(res.len == 1 && res.words[0] == 22345);
    
    printf("PASSED\n");
    return 0;
}
