/**
 * @file    bench_bignum_add_bignum.c
 * @brief   Микробенчмарк для профилирования bignum_add_bignum.
 * @author  git@bayborodov.com
 * @version 1.0.0
 * @date    29.07.2026
 *
 * @details
 *   Вызывает функцию bignum_add_bignum на случайных
 *   больших числах многократно, чтобы perf успел
 *   собрать достаточное число сэмплов.
 *
 * # Сборка
 *  gcc -g -O2 -I include -I libs/bignum-common/include -no-pie -fno-omit-frame-pointer \
 *    benchmarks/bench_bignum_add_bignum.c build/bignum_add_bignum.o \
 *    -o bin/bench_bignum_add_bignum
 *
 * # Запуск perf с записью стека через frame-pointer
 * /usr/local/bin/perf record -F 9999 -o benchmarks/reports/report_bench_bignum_add_bignum -g -- bin/bench_bignum_add_bignum
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <bignum.h>
#include "bignum_add_bignum.h"

// --- Локальные определения для компиляции ---
#define BIGNUM_CAPACITY 32

// Количество итераций
#define ITERATIONS (100000000u * 20)

// Количество предварительно сгенерированных наборов данных
#define PREGEN_DATA_COUNT 8192

/** Заполняет bignum случайными словами и устанавливает len. */
static void init_random_bignum(bignum_t *num) {
    // Ограничиваем длину до BIGNUM_CAPACITY - 1, чтобы избежать частых 
    // ошибок переполнения (OVERFLOW) при сложении двух больших чисел,
    // что могло бы исказить профиль (ранний выход из функции).
    int used = (rand() % (BIGNUM_CAPACITY - 1)) + 1;
    num->len = used;
    for (int i = 0; i < used; ++i) {
        num->words[i] = ((uint64_t)rand() << 32) | rand();
    }
    for (int i = used; i < BIGNUM_CAPACITY; ++i) {
        num->words[i] = 0;
    }
}

int main(void) {
    // --- Фаза 1: Предварительная генерация данных ---
    printf("Pregenerating %u data sets...\n", PREGEN_DATA_COUNT);

    bignum_t* a = malloc(sizeof(bignum_t) * PREGEN_DATA_COUNT);
    bignum_t* b = malloc(sizeof(bignum_t) * PREGEN_DATA_COUNT);

    if (!a || !b) {
        perror("Failed to allocate memory for test data");
        return 1;
    }

    srand((unsigned)time(NULL));
    for (unsigned i = 0; i < PREGEN_DATA_COUNT; ++i) {
        init_random_bignum(&a[i]);
        init_random_bignum(&b[i]);
    }

    // --- Фаза 2: "Горячий" цикл для профилирования ---
    printf("Starting benchmark with %u iterations...\n", ITERATIONS);

    uint32_t progress_step = (ITERATIONS >= 100) ? (ITERATIONS / 100) : 1;
    uint32_t next_progress = progress_step;
    int percent = 0;

    for (uint32_t i = 0; i < ITERATIONS; ++i) {
        unsigned data_idx = i % PREGEN_DATA_COUNT;

        bignum_t res_dst; // Без инициализации

        // Вызов целевой функции
        bignum_add_bignum(&res_dst, &a[data_idx], &b[data_idx]);

        // Проверяем результат, чтобы компилятор не вырезал вызов
        if (res_dst.len == 0xDEADBEEF) {
            printf("Error marker hit.\n");
            return 1;
        }

        /* Отрисовка прогресс-бара */
        if (i + 1 == next_progress) {
            percent++;
            int pos = percent / 2;
            char bar[51];
            for (int j = 0; j < 50; ++j) {
                if (j < pos) bar[j] = '=';
                else if (j == pos && percent < 100) bar[j] = '>';
                else bar[j] = ' ';
            }
            bar[50] = '\0';
            printf("\r[%s] %d%%", bar, percent);
            fflush(stdout);
            next_progress += progress_step;
        }
    }
    printf("\nBenchmark finished.\n");

    // --- Фаза 3: Очистка ---
    free(a);
    free(b);

    return 0;
}
