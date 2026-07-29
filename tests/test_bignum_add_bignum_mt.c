/**
 * @file    test_bignum_add_bignum_mt.c
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    29.07.2026
 *
 * @brief   Динамический тест на потокобезопасность для bignum_add_bignum.
 */

#include "bignum_add_bignum.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#define NUM_THREADS 10
#define NUM_ITERATIONS 100000

typedef struct {
    int thread_id;
    bignum_t a;
    bignum_t b;
    bignum_t expected;
    int ok;
} thread_data_t;

static int is_zero(const bignum_t *x) {
    return (x->len == 0) || (x->len == 1 && x->words[0] == 0);
}

static int bignum_are_equal(const bignum_t* x, const bignum_t* y) {
    if (is_zero(x) && is_zero(y)) return 1;
    if (x->len != y->len) return 0;
    return memcmp(x->words, y->words, x->len * sizeof(uint64_t)) == 0;
}

void* thread_func(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    data->ok = 1;

    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        bignum_t res;
        memset(&res, 0, sizeof(res));

        bignum_add_bignum_status_t status = bignum_add_bignum(&res, &data->a, &data->b);

        if (status != BIGNUM_ADD_BIGNUM_SUCCESS || !bignum_are_equal(&res, &data->expected)) {
            data->ok = 0;
            break;
        }
    }
    return NULL;
}

int main(void) {
    printf("\n--- Starting MT test for bignum_add_bignum ---\n");
    pthread_t threads[NUM_THREADS];
    thread_data_t data[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; ++i) {
        data[i].thread_id = i;
        
        memset(&data[i].a, 0, sizeof(data[i].a));
        data[i].a.len = 2;
        data[i].a.words[0] = 0xFFFFFFFFFFFFFFFFULL;
        data[i].a.words[1] = i;

        memset(&data[i].b, 0, sizeof(data[i].b));
        data[i].b.len = 1;
        data[i].b.words[0] = 1;

        memset(&data[i].expected, 0, sizeof(data[i].expected));

        bignum_add_bignum_status_t st = bignum_add_bignum(&data[i].expected, &data[i].a, &data[i].b);
        if (st != BIGNUM_ADD_BIGNUM_SUCCESS) {
            fprintf(stderr, "Failed to compute expected for thread %d\n", i);
            return 1;
        }

        data[i].ok = 1;
        if (pthread_create(&threads[i], NULL, thread_func, &data[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    int all_ok = 1;
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
        if (!data[i].ok) {
            printf("Thread %d failed!\n", i);
            all_ok = 0;
        }
    }

    if (!all_ok) {
        fprintf(stderr, "--- MT test for bignum_add_bignum FAILED ---\n");
        return 1;
    }

    printf("\n--- MT test for bignum_add_bignum passed ---\n");
    return 0;
}
